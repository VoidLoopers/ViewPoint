/******************************************************************************
 * @file Command.h
 * @brief Command encoding and transmission for ViewPoint
 *
 * Part of the ViewPoint Arduino library for high-speed serial plotting.
 *
 * This file provides:
 * 1. CommandEncoder - Sends configuration commands to the ViewPoint app
 * 2. CommandId enum - Identifies all supported commands
 * 3. CommandSpec - Maps commands to legacy single-char and binary encodings
 *
 * Commands use human-readable format with named arguments:
 *   [command]arg1=value1,arg2=value2
 *
 * Data uses positional channels:
 *   1.234,5.678,9.012
 *
 * See Protocol.h for:
 * - Version negotiation
 * - CommandBuilder fluent API
 * - cmd:: namespace with command string constants
 *
 *   This file is a part of the VoidLoop ViewPoint project, which is one of the 
 *   projects brought to the education and hobbyist community by VoidLoop.    
 *   Please see www.voidloop.com for updates.
 *
 * Contributors:
 * Initial vision, feature definition, debugging and funding: Greg Kovacs
 * Architecture, Development, debugging and support: Zachariah Magee
 * 
 * Thanks to our many collaborators and friends!
 * 
 * MIT License (MIT)
 * 
 * Copyright (c) 2026 VoidLoop
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
******************************************************************************/

#ifndef _VIEWPOINT_COMMAND_H_
#define _VIEWPOINT_COMMAND_H_

#include "Types.h"
#include "Platform.h"
#include "Axis.h"
#include "Plot.h"
#include "Protocol.h"
#include <Arduino.h>

namespace viewpoint {

// ---------------------------------------------------------------------------
// Command Identifiers
// ---------------------------------------------------------------------------

/**
 * @brief All logical commands, independent of wire encoding
 *
 * These correspond to the cmd:: constants in Protocol.h and the
 * CommandID enum in the Kotlin app. The order must match kCommandSpecs[].
 */
enum class CommandId : uint8_t {
    CommandShake,   // Handshake / version negotiation
    PacketSize,     // Set data packet size
    Cartesian,      // Cartesian plot type
    Scatter,        // Scatter plot type
    Polar,          // Polar plot type
    Frames,         // Frame mode
    Scrolling,      // Continuous/scrolling mode
    AddTraces,      // Associate traces with plots
    DerivedTrace,
    Units,          // Axis units
    DisplayModes,   // Spectrogram, persistence, etc.
    PlotLabels,     // Title and axis labels
    TraceColor,     // Individual trace color
    GridColors,     // Grid line colors
    TraceLabels,    // Trace legend labels
    LogScale,       // Logarithmic scale
    ReferenceLine,  // Custom reference lines
    Axis,           // Axis range configuration
    Timestamp,      // Frame/data timestamp
    Message,        // Info message
    Error,          // Error message
    Sketch,         // Sketch title

    Count           // Keep last - used for array sizing
};

// ---------------------------------------------------------------------------
// Legacy/Binary Command Specs
// ---------------------------------------------------------------------------

/**
 * @brief Command encoding specification for multiple wire formats
 *
 * Supports:
 * - humanReadable: Full command string (default format)
 * - binaryCode: Compact binary encoding (reserved for future use)
 *
 * The single-character "legacy" column was removed in 1.8 — no shipped V1
 * parser ever consumed it, and several of its glyphs ('!', '?') overlapped
 * with the protocol-level ACK/NAK characters which made the table confusing.
 */
struct CommandSpec {
    const char* humanReadable;   // Full string, e.g. "[viewpoint]"
    uint8_t     binaryCode;      // Binary code, e.g. 0x01
};

/**
 * @brief Lookup table mapping CommandId to all wire encodings
 *
 * Index by static_cast<size_t>(CommandId::XXX) to get spec.
 * Must stay in sync with CommandId enum order.
 */
constexpr CommandSpec kCommandSpecs[] = {
    // human readable        binary
    { "[viewpoint]",         0x01 },  // CommandShake
    { "[packet size]",       0x02 },  // PacketSize
    { "[cartesian]",         0x03 },  // Cartesian
    { "[scatter]",           0x04 },  // Scatter
    { "[polar]",             0x05 },  // Polar
    { "[frames]",            0x06 },  // Frames
    { "[continuous]",        0x07 },  // Scrolling
    { "[add traces]",        0x08 },  // AddTraces
    { "[trace filter]",      0x0A },  // DerivedTrace
    { "[units]",             0x0B },  // Units
    { "[display mode]",      0x0C },  // DisplayModes
    { "[plot labels]",       0x0D },  // PlotLabels
    { "[trace color]",       0x0E },  // TraceColor
    { "[grid colors]",       0x0F },  // GridColors
    { "[trace labels]",      0x10 },  // TraceLabels
    { "[log scale]",         0x11 },  // LogScale
    { "[reference line]",    0x12 },  // ReferenceLine
    { "[axis]",              0x13 },  // Axis
    { "[time]",              0x14 },  // Timestamp
    { "[message]",           0x15 },  // Message
    { "[error]",             0x16 },  // Error
    { "[sketch]",            0x17 },  // Sketch
};

static_assert(
    sizeof(kCommandSpecs) / sizeof(kCommandSpecs[0]) ==
    static_cast<size_t>(CommandId::Count),
    "kCommandSpecs must match CommandId enum"
);






// ---------------------------------------------------------------------------
// CommandEncoder
// ---------------------------------------------------------------------------

/**
 * @brief Sends ViewPoint configuration commands over serial
 *
 * This class provides methods for sending all supported commands in the
 * human-readable format with named arguments. It uses the cmd:: constants
 * from Protocol.h to ensure consistency with the app's parser.
 *
 * For simple use cases, methods like sendPlotType() and sendAxis() handle
 * all the formatting. For more complex or version-dependent commands,
 * use the CommandBuilder in Protocol.h directly.
 */
class CommandEncoder {
public:

    // Constructor


    CommandEncoder() : serial_(nullptr), dataFormat_(DataFormat::Text) {}

    explicit CommandEncoder(Stream& serial)
        : serial_(&serial), dataFormat_(DataFormat::Text) {}

    void setStream(Stream& serial) {
        serial_ = &serial;
        cmd_.setStream(serial_);
    }

    void setDataFormat(DataFormat format) {
        dataFormat_ = format;
    }


    // Protocol markers


    /**
     * @brief Send start marker indicating commands are coming
     *
     * Sends library version to begin handshake:
     *   [viewpoint]version=V1R1
     */
    void sendStart() {
        if (!serial_) return;
        cmd_.begin(cmd::VIEWPOINT)
            .arg("version", version::STRING)
            .send();
    }

    /**
     * @brief Send end marker indicating configuration complete
     */
    void sendEnd() {
        if (!serial_) return;
        cmd_.begin(cmd::VIEWPOINT)
            .arg("completed", true)
            .send();
    }

    /**
     * @brief Send frame complete marker
     */
    void sendFrameComplete() {
        if (!serial_) return;
        cmd_.begin(cmd::FRAMES)
            .arg("complete", true)
            .send();
    }



    // Plot type commands


    void sendPlotType(PlotType type, int numPlots = 1) {
        if (!serial_) return;

        switch (type) {
            case PlotType::Cartesian:
                cmd_.begin(cmd::CARTESIAN).arg("plots", numPlots).send();
                break;
            case PlotType::Scatter:
                cmd_.begin(cmd::SCATTER).arg("plots", numPlots).send();
                break;
            case PlotType::Polar:
                // Polar uses a separate command path with its own arguments
                // (angular unit, offset, step). The handshake calls
                // sendPolarConfig() instead — see Plotter::sendFullConfiguration.
                break;
        }
    }

    void sendPolarConfig(AngularUnit unit, float offset = 0, int step = 0) {
        if (!serial_) return;
        cmd_.begin(cmd::POLAR)
            .arg("degrees", unit == AngularUnit::Degrees)
            .arg("offset", offset)
            .argIf(step > 0, "step", step)
            .send();
    }


    // Mode commands


    void sendMode(Mode mode) {
        if (!serial_) return;
        cmd_.begin(mode == Mode::Frames ? cmd::FRAMES : cmd::CONTINUOUS)
            .send();
    }


    // Packet size


    void sendPacketSize(int size) {
        if (!serial_) return;
        cmd_.begin(cmd::PACKET_SIZE)
            .arg("size", size)
            .send();
    }


    // Axis configuration


    void sendAxis(int plotId, const Axis& axis) {
        if (!serial_ || !axis.isSet()) return;

        char axisChar = axis.axisChar();

        // Send range with divisions or steps
        if (axis.hasRange()) {
            cmd_.begin(cmd::AXIS)
                .arg("plot", plotId)
                .arg("axis", axisChar)
                .arg("min", axis.minimum())
                .arg("max", axis.maximum())
                .argIf(axis.hasStep(), "minor", axis.minorStep())
                .argIf(axis.hasStep(), "major", axis.majorStep())
                .argIf(!axis.hasStep() && axis.hasDivisions(), "divisions", axis.divisions())
                .send();
        } else if (axis.hasStep()) {
            cmd_.begin(cmd::AXIS)
                .arg("plot", plotId)
                .arg("axis", axisChar)
                .arg("minor", axis.minorStep())
                .arg("major", axis.majorStep())
                .send();
        } else if (axis.hasDivisions()) {
            cmd_.begin(cmd::AXIS)
                .arg("plot", plotId)
                .arg("axis", axisChar)
                .arg("divisions", axis.divisions())
                .send();
        }

        // Send logarithmic if enabled
        if (axis.logarithmic()) {
            cmd_.begin(cmd::LOG_SCALE)
                .arg("plot", plotId)
                .arg("axis", axisChar)
                .argIf(!axis.mapData(), "mapdata", false)
                .send();
        }

        // Send reference lines
        for (int i = 0; i < axis.referenceLineCount(); i++) {
            const ReferenceLine& ref = axis.referenceLine(i);
            if (ref.set) {
                cmd_.begin(cmd::REFERENCE_LINE)
                    .arg("id", i)
                    .arg("plot", plotId)
                    .arg("axis", axisChar)
                    .arg("value", ref.value)
                    .argHex("color", ref.color)
                    .arg("stroke", ref.strokeWeight)
                    .send();
            }
        }
    }


    // Units and labels


    void sendUnits(int plotId, const char* xUnits, const char* yUnits) {
        if (!serial_) return;
        if ((!xUnits || xUnits[0] == '\0') && (!yUnits || yUnits[0] == '\0')) return;

        bool hasX = xUnits && xUnits[0] != '\0';
        bool hasY = yUnits && yUnits[0] != '\0';
        cmd_.begin(cmd::UNITS)
            .arg("plot", plotId)
            .argIf(hasX, "x", xUnits)
            .argIf(hasY, "y", yUnits)
            .send();
    }

    void sendPlotLabels(int plotId, const char* title, const char* xLabel, const char* yLabel) {
        if (!serial_) return;
        if ((!title || title[0] == '\0') && (!xLabel || xLabel[0] == '\0') && (!yLabel || yLabel[0] == '\0')) return;

        bool hasTitle = title && title[0] != '\0';
        bool hasX = xLabel && xLabel[0] != '\0';
        bool hasY = yLabel && yLabel[0] != '\0';
        cmd_.begin(cmd::PLOT_LABELS)
            .arg("plot", plotId)
            .argIf(hasTitle, "title", title)
            .argIf(hasX, "x", xLabel)
            .argIf(hasY, "y", yLabel)
            .send();
    }


    // Title


    void sendTitle(int plotId, const char* title) {
        if (!serial_ || !title || title[0] == '\0') return;
        cmd_.begin(cmd::PLOT_LABELS)
            .arg("plot", plotId)
            .arg("title", title)
            .send();
    }


    // Trace configuration


    void sendTraceColor(int traceId, uint32_t color) {
        if (!serial_ || !hasColor(color)) return;
        cmd_.begin(cmd::TRACE_COLOR)
            .arg("trace", traceId)
            .argHex("color", color)
            .send();
    }

    // Note: sendTraceColors() bulk format (colors=HEX1,HEX2,...) is not
    // supported by the V1 app parser. Use individual sendTraceColor() calls
    // instead. Kept for API compatibility.
    void sendTraceColors(const uint32_t* colors, int count) {
        if (!serial_ || !colors || count == 0) return;

        cmd_.begin(cmd::TRACE_COLOR)
            .argHexList("colors", colors, count)
            .send();
    }
    void sendTraceLabels(const char* const* labels, int count) {
        if (!serial_ || !labels || count == 0) return;
        cmd_.begin(cmd::TRACE_LABELS)
            .argList("labels", labels, count)
            .send();
    }

    void sendTraceFilter(
        int traceId,
        const char* type,
        float param,
        uint32_t color = NO_COLOR,
        const char* label = nullptr
    ) {
        if (!serial_ || !type || type[0] == '\0') return;
        cmd_.begin(cmd::TRACE_FILTER)
            .arg("trace", traceId)
            .arg("type", type)
            .arg("param", param)
            .argHexIf(hasColor(color), "color", color)
            .argIf(label && label[0] != '\0', "label", label)
            .send();
    }

    /**
     * @brief Remove a derived trace by source trace ID
     *
     * Sends [trace filter]trace=N,type=none which tells the app to
     * remove all derived traces for the specified source trace.
     */
    void sendRemoveDerivedTrace(int sourceTraceId) {
        if (!serial_) return;
        cmd_.begin(cmd::TRACE_FILTER)
            .arg("trace", sourceTraceId)
            .arg("type", "none")
            .send();
    }

    // Grid colors


    void sendGridColors(int plotId, const GridColors& colors) {
        if (!serial_ || !colors.set) return;
        cmd_.begin(cmd::GRID_COLORS)
            .arg("plot", plotId)
            .argHex("labels", colors.labels)
            .argHex("minor", colors.minor)
            .argHex("major", colors.major)
            .argIf(colors.strokeWeight != 1.0f, "stroke", colors.strokeWeight)
            .send();
    }


    // Display mode


    void sendDisplayMode(int plotId, const DisplayModeConfig& config) {
        if (!serial_ || !config.set || config.mode == DisplayMode::None) return;

        const char* modeStr = "none";
        switch (config.mode) {
            case DisplayMode::Spectrogram: modeStr = "spectrogram"; break;
            case DisplayMode::Persistence: modeStr = "persistence"; break;
            case DisplayMode::Gradient:    modeStr = "gradient";    break;
            default: break;
        }

        bool hasColors = config.mode == DisplayMode::Spectrogram && config.colorCount > 0;
        cmd_.begin(cmd::DISPLAY_MODE)
            .arg("plot", plotId)
            .arg("mode", modeStr)
            .arg("trace", config.traceId);
        if (hasColors) {
            cmd_.argHexList("colors", config.colors, config.colorCount);
        }
        cmd_.send();
    }


    // Trace to plot association
    void sendAddTraces(int plotId, const int* traceIds, int count) {
        if (!serial_ || !traceIds || count == 0) return;
        cmd_.begin(cmd::ADD_TRACES)
            .arg("plot", plotId)
            .argList("traces", traceIds, count)
            .send();
    }


    // Timestamp
    void sendTimestamp(const char* timestamp) {
        if (!serial_ || !timestamp || timestamp[0] == '\0') return;
        cmd_.begin(cmd::TIMESTAMP)
            .arg("raw", timestamp)
            .send();
    }

    void sendInfo(const char* msg) {
        cmd_.begin(cmd::MESSAGE);
        cmd_.arg("message", msg);
        cmd_.send();
    }

    void sendInfo(const char* title, const char* msg) {
        cmd_.begin(cmd::MESSAGE);
        cmd_.arg("title", title);
        cmd_.arg("message", msg);
        cmd_.send();
    }

    // Error
    void sendError(const char* message) {
        if (!serial_ || !message) return;
        cmd_.begin(cmd::ERROR)
            .arg("message", message)
            .send();
    }

    void sendError(const char* title, const char* message) {
        if (!serial_ || !message) return;
        cmd_.begin(cmd::ERROR)
            .argIf(title && title[0] != '\0', "title", title)
            .arg("message", message)
            .send();
    }


    // Sketch title
    void sendSketchTitle(const char* title) {
        if (!serial_ || !title || title[0] == '\0') return;
        cmd_.begin(cmd::SKETCH)
            .arg("title", title)
            .send();
    }


    // Full plot configuration
    void sendPlotConfig(const Plot& plot) {
        int id = plot.id();

        // Title, Axis labels
        if (plot.hasTitle() || plot.horizontal().hasLabel() || plot.vertical().hasLabel()) {
            sendPlotLabels(id, plot.title(), plot.horizontal().label(), plot.vertical().label());
        }

        // Axes
        sendAxis(id, plot.horizontal());
        sendAxis(id, plot.vertical());

        // Units
        if (plot.horizontal().hasUnits() || plot.vertical().hasUnits()) {
            sendUnits(id, plot.horizontal().units(), plot.vertical().units());
        }

        // Grid colors
        if (plot.hasGridColors()) {
            sendGridColors(id, plot.gridColors());
        }

        // Display mode
        if (plot.hasDisplayMode()) {
            sendDisplayMode(id, plot.displayMode());
        }

        // Associated traces
        if (plot.traceCount() > 0) {
            sendAddTraces(id, plot.traceIds(), plot.traceCount());
        }
    }


    // Data transmission


    /**
     * @brief Enable or disable write buffering
     *
     * When enabled, data writes are batched in a memory buffer and flushed
     * in bulk, dramatically reducing per-call overhead on RTOS platforms.
     * When disabled, data is written directly to serial (lowest latency).
     *
     * Frames mode enables buffering automatically; Continuous mode disables it.
     */
    void setBuffered(bool enabled) {
        if (!enabled) flushBuf();
        buffered_ = enabled;
    }

    /**
     * @brief Start a data line
     */
    void beginData() {
        dataIndex_ = 0;
    }

    /**
     * @brief Send a single value for current trace
     */
    void sendValue(float value) {
        if (!serial_) return;

        if (dataIndex_ > 0) {
            putChar(',');
        }
        printFloat(value);
        dataIndex_++;
    }

    /**
     * @brief Send empty value (NaN on host)
     */
    void sendEmpty() {
        if (!serial_) return;

        if (dataIndex_ > 0) {
            putChar(',');
        }
        // Empty channel
        dataIndex_++;
    }

    /**
     * @brief Send paired values (two channels)
     */
    void sendPair(float v1, float v2) {
        if (!serial_) return;

        if (dataIndex_ > 0) {
            putChar(',');
        }
        printFloat(v1);
        putChar(',');
        printFloat(v2);
        dataIndex_ += 2;
    }

    /**
     * @brief End data line
     */
    void endData() {
        if (!serial_) return;
        if (buffered_) {
            bufPut('\n');
        } else {
            serial_->println();
        }
    }

    /**
     * @brief Flush buffered data to serial
     */
    void flush() {
        flushBuf();
    }

private:

    // ── Write helpers ───────────────────────────────────────────────
    //
    // putChar / putStr / putFloat dispatch to either the transmit buffer
    // (Frames mode) or directly to serial (Continuous mode).

    void putChar(char c) {
        if (buffered_) {
            bufPut(c);
        } else {
            serial_->print(c);
        }
    }

    void putStr(const char* s) {
        if (buffered_) {
            bufPut(s);
        } else {
            serial_->print(s);
        }
    }

    void putFloat(float value, int precision) {
        if (buffered_) {
            bufFloat(value, precision);
        } else {
            serial_->print(value, precision);
        }
    }

    // ── Transmit buffer ─────────────────────────────────────────────
    //
    // Batches data writes to reduce per-call overhead on RTOS platforms
    // (ESP32, Giga R1 / Mbed, RP2040). Individual serial_->print() calls
    // each incur mutex/scheduling overhead; buffering amortizes this
    // across many values.
    //
    // The buffer is always present (member array), but its size is tied to
    // the ViewPoint memory profile so MINIMAL targets don't pay for a
    // 512 B buffer they will never fill. Override with:
    //
    //     #define VIEWPOINT_TX_BUF_SIZE 256
    //     #include <ViewPoint.h>

#ifndef VIEWPOINT_TX_BUF_SIZE
  #if defined(VIEWPOINT_MINIMAL)
    #define VIEWPOINT_TX_BUF_SIZE 32
  #elif defined(VIEWPOINT_LITE)
    #define VIEWPOINT_TX_BUF_SIZE 128
  #else
    #define VIEWPOINT_TX_BUF_SIZE 512
  #endif
#endif
    static constexpr size_t TX_BUF_SIZE = VIEWPOINT_TX_BUF_SIZE;
    char txBuf_[TX_BUF_SIZE];
    size_t txLen_ = 0;
    bool buffered_ = false;

    void bufPut(char c) {
        txBuf_[txLen_++] = c;
        if (txLen_ >= TX_BUF_SIZE) flushBuf();
    }

    void bufPut(const char* s) {
        while (*s) bufPut(*s++);
    }

    void bufFloat(float value, int precision) {
        char tmp[16];
        detail::floatToStr(tmp, sizeof(tmp), value, precision);
        bufPut(tmp);
    }

    void flushBuf() {
        if (txLen_ > 0 && serial_) {
            serial_->write(reinterpret_cast<const uint8_t*>(txBuf_), txLen_);
            txLen_ = 0;
        }
    }

    // Helper methods


    void printFloat(float value) {
        if (value == 0.0f) {
            putChar('0');
        } else if (isnan(value)) {
            putStr("nan");
        } else if (isinf(value)) {
            putStr("inf");
        } else if (fabs(value) >= 4294967040.0f || fabs(value) < 0.001f) {
            char buf[16];
            detail::floatToStr(buf, sizeof(buf), value, 6);
            putStr(buf);
        } else if (fabs(value) >= 1000000.0f) {
            putFloat(value, 2);
        } else if (fabs(value) < 0.1f) {
            putFloat(value, 6);
        } else if (fabs(value) < 10.0f) {
            putFloat(value, 4);
        } else {
            putFloat(value, 2);
        }
    }

    Stream* serial_;
    CommandBuilder cmd_;
    DataFormat dataFormat_;
    int dataIndex_ = 0;
};

} // namespace viewpoint

#endif // _VIEWPOINT_COMMAND_H_
