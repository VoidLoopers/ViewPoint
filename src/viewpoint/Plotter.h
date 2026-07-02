/******************************************************************************
 * @file Plotter.h
 * @brief Main ViewPoint class for serial plotting
 *
 * Part of the ViewPoint Arduino library for high-speed serial plotting.
 *
 * The Plotter class orchestrates:
 * - Protocol negotiation with the ViewPoint app
 * - Plot and trace configuration
 * - Data transmission
 *
 * Version Negotiation:
 * When begin() is called, the library waits for the app to send its version.
 * If received full protocol is available.
 * If not received, falls back to data-only mode for compatibility with
 * non-ViewPoint receivers (like Arduino Serial Plotter).
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


#ifndef _VIEWPOINT_PLOTTER_H_
#define _VIEWPOINT_PLOTTER_H_

#include "Types.h"
#include "Colors.h"
#include "Axis.h"
#include "Trace.h"
#include "Plot.h"
#include "Command.h"
#include "Protocol.h"
#include <Arduino.h>
#include <string.h>
#include <stdarg.h>

namespace viewpoint {

class Plotter {
public:
    
    Plotter()
        : serial_(nullptr)
        , plotType_(PlotType::Cartesian)
        , mode_(Mode::Continuous)
        , packetSize_(VIEWPOINT_DEFAULT_PACKET_SIZE)
        , delayMs_(2)
        , legacyMode_(false)
        , dirtyFlags_(DIRTY_ALL)
        , needsFullResend_(true)
        , plotCount_(0)
        , traceCount_(0)
        , derivedTraceCount_(0)
        , pointsSent_(0)
    {
        // Initialize trace pointers to null
        for (int i = 0; i < VIEWPOINT_MAX_TRACES; i++) {
            traces_[i] = nullptr;
        }

        for (int i = 0; i < VIEWPOINT_MAX_DERIVED_TRACES; i++) {
            derivedTraces_[i].source = 0;
            derivedTraces_[i].filter = Filter::Average;
            derivedTraces_[i].param = defaultDerivedParam(Filter::Average);
            derivedTraces_[i].color = NO_COLOR;
            derivedTraces_[i].label[0] = '\0';
        }
    }

    ~Plotter() {
        // Free any library-owned traces
        for (int i = 0; i < VIEWPOINT_MAX_TRACES; i++) {
            if (traces_[i] && traces_[i]->ownsMemory()) {
                delete traces_[i];
                traces_[i] = nullptr;
            }
        }
    }

    

    // ── begin() — 4 overloads, dispatched by first argument's type ────
    //
    // C++ default arguments collapse the trailing parameters; overloads
    // disambiguate the leading argument. Because PlotType and Mode are
    // enum class (no implicit int conversion), the first argument's type
    // uniquely selects an overload:
    //
    //   unsigned long → #1   PlotType → #3
    //   Stream&       → #2   Mode     → #4
    //
    // Every previously-supported call shape still compiles unchanged.

    /** @brief Initialize with default Serial. All parameters optional. */
    void begin(unsigned long baud  = 115200,
               PlotType      type  = PlotType::Cartesian,
               Mode          mode  = Mode::Continuous,
               int    packetSize   = VIEWPOINT_DEFAULT_PACKET_SIZE) {
        initDefaultSerial(baud);
        setPlotType(type);
        setMode(mode);
        setPacketSize(packetSize);
    }

    /** @brief Initialize with a user-provided Stream. */
    void begin(Stream& serial,
               PlotType      type  = PlotType::Cartesian,
               Mode          mode  = Mode::Continuous,
               int    packetSize   = VIEWPOINT_DEFAULT_PACKET_SIZE) {
        initStream(serial);
        setPlotType(type);
        setMode(mode);
        setPacketSize(packetSize);
    }

    /** @brief Shortcut: begin(scatter), begin(polar, frames), etc. */
    void begin(PlotType type,
               Mode          mode  = Mode::Continuous,
               int    packetSize   = VIEWPOINT_DEFAULT_PACKET_SIZE) {
        begin(115200UL, type, mode, packetSize);
    }

    /** @brief Shortcut: begin(frames), begin(frames, 512), etc. */
    void begin(Mode mode,
               int    packetSize   = VIEWPOINT_DEFAULT_PACKET_SIZE) {
        begin(115200UL, PlotType::Cartesian, mode, packetSize);
    }


    // Lifecycle


    /**
     * @brief Reset all state to initial defaults
     *
     * Frees all library-owned traces, clears all plot configurations,
     * and resets mode/type/packet size to defaults. The next call to
     * send() will trigger a full handshake resend to the app.
     *
     * Does NOT close the serial connection or reset protocol state.
     * Call begin() again only if you need to change the serial stream.
     */
    void reset() {
        // Free library-owned traces
        for (int i = 0; i < VIEWPOINT_MAX_TRACES; i++) {
            if (traces_[i] && traces_[i]->ownsMemory()) {
                delete traces_[i];
            }
            traces_[i] = nullptr;
        }
        traceCount_ = 0;

        // Clear derived traces
        derivedTraceCount_ = 0;

        // Reset plots
        defaultPlot_.clear();
        for (int i = 0; i < VIEWPOINT_MAX_PLOTS; i++) {
            plots_[i].clear();
        }
        plotCount_ = 0;

        // Reset configuration to constructor defaults
        plotType_ = PlotType::Cartesian;
        mode_ = Mode::Continuous;
        packetSize_ = VIEWPOINT_DEFAULT_PACKET_SIZE;
        delayMs_ = 2;
        sketchTitle_[0] = '\0';
        angularUnit_ = AngularUnit::Degrees;
        angularOffset_ = 0;
        angularStep_ = 15;
        pointsSent_ = 0;

        // Force full handshake on next send()
        needsFullResend_ = true;
        dirtyFlags_ = DIRTY_ALL;
    }

    /**
     * @brief Remove a single trace by ID
     *
     * Frees the trace if library-owned. Also removes any derived
     * traces that reference this trace as their source.
     * Triggers a full handshake resend on next send().
     *
     * @param id Trace ID (0-based index)
     */
    void removeTrace(int id) {
        if (id < 0 || id >= VIEWPOINT_MAX_TRACES) return;

        // Free the trace
        if (traces_[id]) {
            if (traces_[id]->ownsMemory()) {
                delete traces_[id];
            }
            traces_[id] = nullptr;
        }

        // Recalculate traceCount_ (last non-null + 1)
        traceCount_ = 0;
        for (int i = VIEWPOINT_MAX_TRACES - 1; i >= 0; i--) {
            if (traces_[i]) {
                traceCount_ = i + 1;
                break;
            }
        }

        // Remove derived traces that reference this source
        int writeIdx = 0;
        for (int i = 0; i < derivedTraceCount_; i++) {
            if (static_cast<int>(derivedTraces_[i].source) != id) {
                if (writeIdx != i) {
                    derivedTraces_[writeIdx] = derivedTraces_[i];
                }
                writeIdx++;
            }
        }
        derivedTraceCount_ = writeIdx;

        needsFullResend_ = true;
    }

    /**
     * @brief Remove all traces
     *
     * Frees all library-owned traces and clears all derived traces.
     * Triggers a full handshake resend on next send().
     */
    void clearTraces() {
        for (int i = 0; i < VIEWPOINT_MAX_TRACES; i++) {
            if (traces_[i] && traces_[i]->ownsMemory()) {
                delete traces_[i];
            }
            traces_[i] = nullptr;
        }
        traceCount_ = 0;
        derivedTraceCount_ = 0;
        needsFullResend_ = true;
    }

    /**
     * @brief Reset all plot configurations
     *
     * Clears the default plot and all multi-plot configurations
     * (titles, axes, grid colors, display modes, trace associations).
     * Triggers a full handshake resend on next send().
     */
    void clearPlots() {
        defaultPlot_.clear();
        for (int i = 0; i < VIEWPOINT_MAX_PLOTS; i++) {
            plots_[i].clear();
        }
        plotCount_ = 0;
        needsFullResend_ = true;
    }


    // Protocol Configuration

    /**
     * @brief Get the Protocol object for advanced use
     *
     * Use this to:
     * - Check version compatibility: protocol().supports(1, 2)
     * - Build custom commands: protocol().command("[custom]")...
     * - Check connection state: protocol().isNegotiated()
     */
    Protocol& protocol() { return protocol_; }

    
    // Configuration
    
    void setPlotType(PlotType type) {
        if (type != plotType_) needsFullResend_ = true;
        plotType_ = type;
        if (plotType_ == PlotType::Polar) setPacketSize(360);
        dirtyFlags_ |= DIRTY_PLOT_TYPE;
    }

    void setMode(Mode mode) {
        mode_ = mode;
        if (mode == Mode::Frames) {
            delayMs_ = 16.666f;  // ~60 fps default frame delay
            encoder_.setBuffered(true);
        } else {
            delayMs_ = 2.0f;     // Default continuous delay
            encoder_.setBuffered(false);
        }
        dirtyFlags_ |= DIRTY_MODE;
    }

    void setPacketSize(int size) {
        packetSize_ = size;
        dirtyFlags_ |= DIRTY_PACKET_SIZE;
    }

    void setDelay(float ms) {
        delayMs_ = ms;
    }

    void setLegacyMode(bool enabled) {
        legacyMode_ = enabled;
    }

    
    // Simple configuration (single plot / default plot)
   
    void setVerticalRange(float min, float max) {
        defaultPlot_.setVerticalRange(min, max);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    void setVerticalRange(float min, float max, int divisions) {
        defaultPlot_.setVerticalRange(min, max, divisions);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    void setVerticalRange(float min, float max, float minor, float major) {
        defaultPlot_.setVerticalRange(min, max, minor, major);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    void setHorizontalRange(float min, float max) {
        defaultPlot_.setHorizontalRange(min, max);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    void setHorizontalRange(float min, float max, int divisions) {
        defaultPlot_.setHorizontalRange(min, max, divisions);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    void setHorizontalRange(float min, float max, float minor, float major) {
        defaultPlot_.setHorizontalRange(min, max, minor, major);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    void setUnits(const char* horizontal, const char* vertical) {
        defaultPlot_.setUnits(horizontal, vertical);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    void setRadialRange(float min, float max) {
        defaultPlot_.setVerticalRange(min, max);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    void setRadialRange(float min, float max, int divisions) {
        defaultPlot_.setVerticalRange(min, max, divisions);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    void setRadialRange(float min, float max, float minor, float major) {
        defaultPlot_.setVerticalRange(min, max, minor, major);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    void setAngularOffset(float offset) {
        angularOffset_ = offset;
        dirtyFlags_ |= DIRTY_POLAR_CONFIG;
    }

    void setAngularStep(int step) {
        angularStep_ = step;
        dirtyFlags_ |= DIRTY_POLAR_CONFIG;
    }

    void setAngularUnits(AngularUnit units) {
        angularUnit_ = units;
        dirtyFlags_ |= DIRTY_POLAR_CONFIG;
    }

    void setRadians() {
        angularUnit_ = AngularUnit::Radians;
        dirtyFlags_ |= DIRTY_POLAR_CONFIG;
    }

    void setAxisLabels(const char* horizontal, const char* vertical) {
        defaultPlot_.setAxisLabels(horizontal, vertical);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    void setTitle(const char* title) {
        if (title) {
            strncpy(sketchTitle_, title, sizeof(sketchTitle_) - 1);
            sketchTitle_[sizeof(sketchTitle_) - 1] = '\0';
        } else {
            sketchTitle_[0] = '\0';
        }
        dirtyFlags_ |= DIRTY_SKETCH_TITLE;
    }

    void setPlotTitle(const char* title) {
        defaultPlot_.setTitle(title);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    void enableLogarithmicScale(bool mapData = true) {
        defaultPlot_.enableLogarithmicScale(mapData);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    void disableLogarithmicScale() {
        defaultPlot_.disableLogarithmicScale();
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    void setGridColors(uint32_t minor, uint32_t major) {
        defaultPlot_.setGridColors(minor, major);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    void setGridColors(uint32_t labels, uint32_t minor, uint32_t major) {
        defaultPlot_.setGridColors(labels, minor, major);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    int addHorizontalReferenceLine(float value, bool isMajor = false) {
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
        return defaultPlot_.addHorizontalReferenceLine(value, isMajor);
    }

    int addHorizontalReferenceLine(float value, uint32_t color, float stroke = 1.0f) {
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
        return defaultPlot_.addHorizontalReferenceLine(value, color, stroke);
    }

    bool updateHorizontalReferenceLine(int index, float value, uint32_t color = NO_COLOR, float stroke = -1.0f) {
        bool updated = defaultPlot_.updateHorizontalReferenceLine(index, value, color, stroke);
        if (updated) dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
        return updated;
    }

    int addVerticalReferenceLine(float value, bool isMajor = false) {
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
        return defaultPlot_.addVerticalReferenceLine(value, isMajor);
    }

    int addVerticalReferenceLine(float value, uint32_t color, float stroke = 1.0f) {
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
        return defaultPlot_.addVerticalReferenceLine(value, color, stroke);
    }

    bool updateVerticalReferenceLine(int index, float value, uint32_t color = NO_COLOR, float stroke = -1.0f) {
        bool updated = defaultPlot_.updateVerticalReferenceLine(index, value, color, stroke);
        if (updated) dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
        return updated;
    }

    void setDisplayMode(DisplayMode mode) {
        defaultPlot_.setDisplayMode(mode);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    void setDisplayMode(DisplayMode mode, int traceId) {
        defaultPlot_.setDisplayMode(mode, traceId);
        dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
    }

    
    // Multiple plots
    void setNumberOfPlots(int count) {
        if (count > 0 && count <= VIEWPOINT_MAX_PLOTS) {
            plotCount_ = count;
            dirtyFlags_ |= DIRTY_TRACE_TOPOLOGY;
        }
    }

    Plot& plot(int index) {
        if (index >= 0 && index < VIEWPOINT_MAX_PLOTS) {
            if (index >= plotCount_) {
                plotCount_ = index + 1;
            }
            plots_[index].setId(index);
            return plots_[index];
        }
        return defaultPlot_;
    }

    
    // Trace management
    /**
     * @brief Create a library-managed trace
     */
    Trace& createTrace(int id) {
        return createTrace(id, packetSize_);
    }

    /**
     * @brief Create a library-managed trace with specific capacity
     */
    Trace& createTrace(int id, size_t capacity) {
        if (id < 0 || id >= VIEWPOINT_MAX_TRACES) {
            encoder_.sendError("Trace ID out of range");
            return dummyTrace_;
        }

        // Delete existing if library-owned
        if (traces_[id] && traces_[id]->ownsMemory()) {
            delete traces_[id];
        }

        traces_[id] = new Trace(id, capacity);
        if (id >= traceCount_) {
            traceCount_ = id + 1;
        }
        dirtyFlags_ |= DIRTY_TRACE_TOPOLOGY | DIRTY_TRACE_LABELS;
        return *traces_[id];
    }

    /**
     * @brief Create a trace with external single-value buffer
     */
    Trace& createTrace(int id, float* buffer, size_t size) {
        if (id < 0 || id >= VIEWPOINT_MAX_TRACES) {
            encoder_.sendError("Trace ID out of range");
            return dummyTrace_;
        }

        if (traces_[id] && traces_[id]->ownsMemory()) {
            delete traces_[id];
        }

        traces_[id] = new Trace(id, buffer, size);
        if (id >= traceCount_) {
            traceCount_ = id + 1;
        }
        dirtyFlags_ |= DIRTY_TRACE_TOPOLOGY | DIRTY_TRACE_LABELS;
        return *traces_[id];
    }

    /**
     * @brief Create a trace with external paired buffers
     */
    Trace& createTrace(int id, float* buf1, float* buf2, size_t size) {
        if (id < 0 || id >= VIEWPOINT_MAX_TRACES) {
            encoder_.sendError("Trace ID out of range");
            return dummyTrace_;
        }

        if (traces_[id] && traces_[id]->ownsMemory()) {
            delete traces_[id];
        }

        traces_[id] = new Trace(id, buf1, buf2, size);
        if (id >= traceCount_) {
            traceCount_ = id + 1;
        }
        dirtyFlags_ |= DIRTY_TRACE_TOPOLOGY | DIRTY_TRACE_LABELS;
        return *traces_[id];
    }

    /**
     * @brief Get existing trace by ID
     */
    Trace& trace(int id) {
        if (id >= 0 && id < VIEWPOINT_MAX_TRACES && traces_[id]) {
            return *traces_[id];
        }
        return getOrCreateTrace(id);
    }

    /**
     * @brief Get trace by label (creates if not found)
     */
    Trace& trace(const char* label) {
        // Search existing traces
        for (int i = 0; i < traceCount_; i++) {
            if (traces_[i] && traces_[i]->hasLabel()) {
                if (strcmp(traces_[i]->label(), label) == 0) {
                    return *traces_[i];
                }
            }
        }
        // Create new trace with label
        Trace& t = createTrace(traceCount_);
        t.setLabel(label);
        return t;
    }
 
    // Adding Data

    /**
     * @brief Add single value by trace ID
     */
    void addData(int traceId, float value) {
        getOrCreateTrace(traceId).addData(value);
    }

    /**
     * @brief Add single value by trace label
     */
    void addData(const char* label, float value) {
        trace(label).addData(value);
    }

    /**
     * @brief Add paired values by trace ID
     */
    void addData(int traceId, float v1, float v2) {
        getOrCreateTrace(traceId).addData(v1, v2);
    }

    /**
     * @brief Add paired values by trace label
     */
    void addData(const char* label, float v1, float v2) {
        trace(label).addData(v1, v2);
    }

    /**
     * @brief Add array of single values by trace ID
     */
    void addData(int traceId, const float* data, size_t count) {
        getOrCreateTrace(traceId).addData(data, count);
    }

    /**
     * @brief Add array of single values by trace label
     */
    void addData(const char* label, const float* data, size_t count) {
        trace(label).addData(data, count);
    }

    /**
     * @brief Add arrays of paired values by trace ID
     */
    void addData(int traceId, const float* d1, const float* d2, size_t count) {
        getOrCreateTrace(traceId).addData(d1, d2, count);
    }

    // Trace breaks (Scatter/Polar)

    /**
     * @brief Insert a break in a trace by ID (Scatter/Polar only)
     *
     * Starts a new line segment without connecting to the previous point.
     */
    void addBreak(int traceId) {
        getOrCreateTrace(traceId).addBreak();
    }

    /**
     * @brief Insert a break in a trace by label (Scatter/Polar only)
     */
    void addBreak(const char* label) {
        trace(label).addBreak();
    }

    void addDerivedTrace(int traceId, Filter filter, uint32_t color = NO_COLOR) {
        addDerivedTrace(traceId, filter, defaultDerivedParam(filter), color, nullptr);
    }

    void addDerivedTrace(
        int traceId,
        Filter filter,
        float param,
        uint32_t color = NO_COLOR,
        const char* label = nullptr
    ) {
        if (traceId < 0 || traceId >= VIEWPOINT_MAX_TRACES) {
            encoder_.sendError("Derived trace source out of range");
            return;
        }

        // Ensure source trace exists before emitting derived commands.
        getOrCreateTrace(traceId);

        if (param <= 0.0f) {
            param = defaultDerivedParam(filter);
        }

        int idx = findDerivedTrace(traceId, filter);
        if (idx < 0) {
            if (derivedTraceCount_ >= VIEWPOINT_MAX_DERIVED_TRACES) {
                encoder_.sendError("Derived trace limit exceeded");
                return;
            }
            idx = derivedTraceCount_++;
        }

        DerivedTrace& derived = derivedTraces_[idx];
        derived.source = static_cast<uint16_t>(traceId);
        derived.filter = filter;
        derived.param = param;
        derived.color = color;

        if (label && label[0] != '\0') {
            strncpy(derived.label, label, VIEWPOINT_LABEL_SIZE - 1);
            derived.label[VIEWPOINT_LABEL_SIZE - 1] = '\0';
        } else {
            derived.label[0] = '\0';
        }

        dirtyFlags_ |= DIRTY_DERIVED_TRACES;
    }

    bool removeDerivedTrace(int traceId, Filter filter) {
        const int idx = findDerivedTrace(traceId, filter);
        if (idx < 0) return false;

        for (int i = idx; i < derivedTraceCount_ - 1; i++) {
            derivedTraces_[i] = derivedTraces_[i + 1];
        }
        if (derivedTraceCount_ > 0) {
            derivedTraceCount_--;
        }
        dirtyFlags_ |= DIRTY_DERIVED_TRACES;
        return true;
    }

    void clearDerivedTraces() {
        derivedTraceCount_ = 0;
        dirtyFlags_ |= DIRTY_DERIVED_TRACES;
    }

    // Direct send (zero-copy)

    /**
     * @brief Send data directly without storing in trace
     */
    void sendData(int traceId, float value) {
        ensureConfigSent();
        encoder_.beginData();
        for (int i = 0; i < traceId; i++) {
            encoder_.sendEmpty();
        }
        encoder_.sendValue(value);
        encoder_.endData();
    }

    void sendData(int traceId, float v1, float v2) {
        ensureConfigSent();
        encoder_.beginData();
        for (int i = 0; i < traceId * 2; i++) {
            encoder_.sendEmpty();
        }
        // Scatter wire order is (y, x) so the host's values1 slot carries Y,
        // matching the buffered flush path in sendBufferedData().
        if (plotType_ == PlotType::Scatter) {
            encoder_.sendPair(v2, v1);
        } else {
            encoder_.sendPair(v1, v2);
        }
        encoder_.endData();
    }

    /**
     * @brief Send an informational message to the host (printf-style).
     *
     * Formats up to ~500 characters and emits a [message]message=... command.
     * Safe to call before the protocol is negotiated; the encoder drops
     * commands silently in that state.
     */
    void sendInfo(const char* fmt, ...) {
        if (!fmt) return;
        char buf[512];
        va_list args;
        va_start(args, fmt);
        const int n = vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        if (n < 0) return;   // formatting failed — bail rather than send garbage
        encoder_.sendInfo(buf);
    }

    /** @brief Send an informational message with a title. */
    void sendInfo(const char* title, const char* msg) {
        encoder_.sendInfo(title, msg);
    }

    /**
     * @brief Send an error message to the host (printf-style).
     *
     * Mirrors sendInfo: formats the message and emits an [error] command.
     */
    void sendError(const char* fmt, ...) {
        if (!fmt) return;
        char buf[512];
        va_list args;
        va_start(args, fmt);
        const int n = vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        if (n < 0) return;
        encoder_.sendError(buf);
    }

    /** @brief Send an error message with a title. */
    void sendError(const char* title, const char* msg) {
        encoder_.sendError(title, msg);
    }

    /**
     * @brief Send all buffered data
     *
     * This method:
     * 1. Updates protocol state (handles version negotiation)
     * 2. Sends configuration if needed (once protocol is ready)
     * 3. Transmits all buffered trace data
     * 4. Sends frame complete marker (in frames mode)
     * 5. Clears trace buffers
     *
     * @param timestamp Optional timestamp string for the data
     */
    void send(const char* timestamp = "") {
        // Drain incoming protocol messages first — this is what flips
        // state into Negotiated once the app sends [viewpoint]version=...
        protocol_.update();

        if (!serial_) {
            dirtyFlags_ = DIRTY_ALL;
            needsFullResend_ = true;
            return;
        }

        // Send configuration if needed (only in negotiated mode)
        ensureConfigSent();

        // Find max data points to send
        size_t pointsToSend = 0;
        for (int i = 0; i < traceCount_; i++) {
            if (traces_[i] && traces_[i]->size() > pointsToSend) {
                pointsToSend = traces_[i]->size();
            }
        }

        if (pointsToSend == 0) {
            pointsToSend = packetSize_;
        }

        // Legacy mode limit
        if (legacyMode_ && pointsToSend > 500) {
            pointsToSend = 500;
        }

        // Send timestamp for frames mode (once per frame)
        if (mode_ == Mode::Frames && timestamp && timestamp[0] != '\0') {
            if (protocol_.isNegotiated()) {
                encoder_.sendTimestamp(timestamp);
            }
        }

        // Send data points
        for (size_t p = 0; p < pointsToSend; p++) {
            // Timestamp for continuous mode (each line)
            if (mode_ == Mode::Continuous && timestamp && timestamp[0] != '\0') {
                if (protocol_.isNegotiated()) {
                    encoder_.sendTimestamp(timestamp);
                }
            }

            // Send data line
            encoder_.beginData();
            for (int t = 0; t < traceCount_; t++) {
                if (traces_[t]) {
                    if (traces_[t]->isPaired()) {
                        if (p < traces_[t]->size()) {
                            if (plotType_ == PlotType::Scatter) {
                                encoder_.sendPair(traces_[t]->get2(p), traces_[t]->get(p));
                            } else {
                                encoder_.sendPair(traces_[t]->get(p), traces_[t]->get2(p));
                            }
                        } else {
                            encoder_.sendEmpty();
                            encoder_.sendEmpty();
                        }
                    } else {
                        if (p < traces_[t]->size()) {
                            encoder_.sendValue(traces_[t]->get(p));
                            if (plotType_ != PlotType::Cartesian) encoder_.sendEmpty();
                        } else {
                            encoder_.sendEmpty();
                        }
                    }
                }
            }
            encoder_.endData();

            pointsSent_++;

            // Delay for continuous mode
            if (mode_ == Mode::Continuous) {
                detail::platformDelay((unsigned long)delayMs_);
                pointsSent_ = 0;
            }

            // Let RTOS platforms service background tasks during large frame sends
            if (mode_ == Mode::Frames && (pointsSent_ % 20 == 0)) {
                detail::platformYield();
            }
        }

        // Flush any remaining buffered data before frame complete
        encoder_.flush();

        // Frame complete
        if (mode_ == Mode::Frames && pointsSent_ >= (size_t)packetSize_) {
            if (!legacyMode_ && protocol_.isNegotiated()) {
                encoder_.sendFrameComplete();
            }
            detail::platformDelay((unsigned long)delayMs_);
            pointsSent_ = 0;
        }

        // Clear trace data
        for (int i = 0; i < traceCount_; i++) {
            if (traces_[i]) {
                traces_[i]->clear();
            }
        }
    }

    /**
     * @brief Update protocol state without sending data
     *
     * Call this in your loop if you need to process incoming
     * protocol messages without sending data.
     */
    void update() {
        protocol_.update();
    }

    /**
     * @brief Check if protocol negotiation is complete
     *
     * Returns true if:
     * - Version handshake completed (Negotiated state), or
     * - Timeout occurred (DataOnly state)
     */
    bool isReady() const {
        return protocol_.isReady() || legacyMode_;
    }

    /**
     * @brief Check if full protocol features are available
     *
     * Returns true only if version handshake completed successfully.
     * In DataOnly mode, this returns false.
     */
    bool isNegotiated() const {
        return protocol_.isNegotiated();
    }


    int packetSize() const { return packetSize_; }
    int traceCount() const { return traceCount_; }
    PlotType plotType() const { return plotType_; }
    Mode mode() const { return mode_; }
    bool isLegacyMode() const { return legacyMode_; }

private:
    // ── Core serial initialization helpers ──────────────────────────

    void initDefaultSerial(unsigned long baud) {
#if defined(ARDUINO_ARCH_ESP32)
        // ESP32 default UART TX buffer is 256 bytes — too small for
        // frame-mode transmissions. Must be called before begin().
        Serial.setTxBufferSize(1024);
#endif
        Serial.begin(baud);
        // We deliberately do not block on !Serial here. USB-CDC boards
        // (UNO Q, UNO R4, GIGA, Nano 33 BLE) take 1-2s to enumerate, and
        // the protocol's wait-for-version state already handles that.
        serial_ = &Serial;
        encoder_.setStream(*serial_);
        protocol_.begin(*serial_);
    }

    void initStream(Stream& serial) {
        serial_ = &serial;
        encoder_.setStream(*serial_);
        protocol_.begin(serial);
    }

    // ── Internal helpers ────────────────────────────────────────────

    Trace& getOrCreateTrace(int id) {
        if (id >= 0 && id < VIEWPOINT_MAX_TRACES) {
            if (!traces_[id]) {
                return createTrace(id);
            }
            return *traces_[id];
        }
        return dummyTrace_;
    }

    static float defaultDerivedParam(Filter filter) {
        switch (filter) {
            case Filter::Average: return 0.3f;
            case Filter::MaxHold: return 0.95f;
            default: return 0.3f;
        }
    }

    static const char* filterType(Filter filter) {
        switch (filter) {
            case Filter::Average: return "average";
            case Filter::MaxHold: return "maxhold";
            default: return "average";
        }
    }

    int findDerivedTrace(int traceId, Filter filter) const {
        for (int i = 0; i < derivedTraceCount_; i++) {
            const DerivedTrace& derived = derivedTraces_[i];
            if (static_cast<int>(derived.source) == traceId && derived.filter == filter) {
                return i;
            }
        }
        return -1;
    }

    void ensureConfigSent() {
        if (legacyMode_ || protocol_.isDataOnly()) return;
        if (!protocol_.isNegotiated()) return;

        pollTraceDirtyFlags();
        pollPlotDirtyFlags();

        if (needsFullResend_ || protocol_.configRequested()) {
            sendFullConfiguration();
            return;
        }

        if (dirtyFlags_ == 0) return;
        sendIncrementalConfig();
    }

    void pollTraceDirtyFlags() {
        for (int i = 0; i < traceCount_; i++) {
            if (traces_[i]) {
                uint8_t d = traces_[i]->metaDirty();
                if (d & Trace::TRACE_DIRTY_LABEL) dirtyFlags_ |= DIRTY_TRACE_LABELS;
                if (d & Trace::TRACE_DIRTY_COLOR) dirtyFlags_ |= DIRTY_TRACE_COLORS;
            }
        }
    }

    void pollPlotDirtyFlags() {
        if (defaultPlot_.isDirty()) dirtyFlags_ |= DIRTY_DEFAULT_PLOT;
        for (int i = 0; i < plotCount_; i++) {
            if (plots_[i].isDirty()) dirtyFlags_ |= DIRTY_PLOT_N(i);
        }
    }

    void clearAllDirtyState() {
        for (int i = 0; i < traceCount_; i++) {
            if (traces_[i]) traces_[i]->clearMetaDirty();
        }
        defaultPlot_.clearDirty();
        for (int i = 0; i < plotCount_; i++) {
            plots_[i].clearDirty();
        }
        dirtyFlags_ = 0;
        needsFullResend_ = false;
    }

    void sendFullConfiguration() {
        if (!serial_ || legacyMode_) return;
        if (!protocol_.isNegotiated()) return;

        // Start marker (library version)
        encoder_.sendStart();

        // Sketch title
        if (sketchTitle_[0] != '\0') {
            encoder_.sendSketchTitle(sketchTitle_);
        }

        // Plot type
        encoder_.sendPlotType(plotType_, plotCount_ > 0 ? plotCount_ : 1);
        if (plotType_ == PlotType::Polar) encoder_.sendPolarConfig(angularUnit_, angularOffset_, angularStep_);


        // encoder_.sendTraceModes();
        // Mode
        encoder_.sendMode(mode_);

        // Packet size
        encoder_.sendPacketSize(packetSize_);

        // Collect trace labels
        const char* labels[VIEWPOINT_MAX_TRACES];
        int labelCount = 0;
        for (int i = 0; i < traceCount_; i++) {
            if (traces_[i]) {
                labels[labelCount++] = traces_[i]->label();
            }
        }
        if (labelCount > 0) {
            encoder_.sendTraceLabels(labels, labelCount);
        }

        // Trace colors
        for (int i = 0; i < traceCount_; i++) {
            if (traces_[i] && hasColor(traces_[i]->color())) {
                encoder_.sendTraceColor(i, traces_[i]->color());
            }
        }

        // Derived traces (app-side computed traces) are only valid in
        // Cartesian + Frames mode in the current desktop implementation.
        if (plotType_ == PlotType::Cartesian && mode_ == Mode::Frames) {
            for (int i = 0; i < derivedTraceCount_; i++) {
                const DerivedTrace& derived = derivedTraces_[i];
                encoder_.sendTraceFilter(
                    static_cast<int>(derived.source),
                    filterType(derived.filter),
                    derived.param > 0.0f ? derived.param : defaultDerivedParam(derived.filter),
                    derived.color,
                    derived.label[0] != '\0' ? derived.label : nullptr
                );
            }
        }

        // Default plot configuration
        if (defaultPlot_.isConfigured()) {
            encoder_.sendPlotConfig(defaultPlot_);
        }

        // Multi-plot configurations
        for (int i = 0; i < plotCount_; i++) {
            if (plots_[i].isConfigured()) {
                encoder_.sendPlotConfig(plots_[i]);
            }
        }

        // End marker
        encoder_.sendEnd();

        clearAllDirtyState();
    }

    void sendIncrementalConfig() {
        if (!serial_) return;

        // Sketch title
        if (dirtyFlags_ & DIRTY_SKETCH_TITLE) {
            if (sketchTitle_[0] != '\0') {
                encoder_.sendSketchTitle(sketchTitle_);
            }
        }

        // Structural commands (order matters)
        if (dirtyFlags_ & (DIRTY_PLOT_TYPE | DIRTY_TRACE_TOPOLOGY)) {
            encoder_.sendPlotType(plotType_, plotCount_ > 0 ? plotCount_ : 1);
        }
        if ((dirtyFlags_ & DIRTY_POLAR_CONFIG) && plotType_ == PlotType::Polar) {
            encoder_.sendPolarConfig(angularUnit_, angularOffset_, angularStep_);
        }
        if (dirtyFlags_ & DIRTY_MODE) {
            encoder_.sendMode(mode_);
        }
        if (dirtyFlags_ & DIRTY_PACKET_SIZE) {
            encoder_.sendPacketSize(packetSize_);
        }

        // Trace labels
        if (dirtyFlags_ & (DIRTY_TRACE_LABELS | DIRTY_TRACE_TOPOLOGY)) {
            const char* labels[VIEWPOINT_MAX_TRACES];
            int labelCount = 0;
            for (int i = 0; i < traceCount_; i++) {
                if (traces_[i]) labels[labelCount++] = traces_[i]->label();
            }
            if (labelCount > 0) encoder_.sendTraceLabels(labels, labelCount);
        }

        // Trace colors (only dirty traces)
        if (dirtyFlags_ & DIRTY_TRACE_COLORS) {
            for (int i = 0; i < traceCount_; i++) {
                if (traces_[i] && (traces_[i]->metaDirty() & Trace::TRACE_DIRTY_COLOR)
                    && hasColor(traces_[i]->color())) {
                    encoder_.sendTraceColor(i, traces_[i]->color());
                }
            }
        }

        // Derived traces
        if (dirtyFlags_ & DIRTY_DERIVED_TRACES) {
            if (plotType_ == PlotType::Cartesian && mode_ == Mode::Frames) {
                for (int i = 0; i < derivedTraceCount_; i++) {
                    const DerivedTrace& d = derivedTraces_[i];
                    encoder_.sendTraceFilter(
                        static_cast<int>(d.source),
                        filterType(d.filter),
                        d.param > 0.0f ? d.param : defaultDerivedParam(d.filter),
                        d.color,
                        d.label[0] != '\0' ? d.label : nullptr
                    );
                }
            }
        }

        // Default plot config
        if (dirtyFlags_ & DIRTY_DEFAULT_PLOT) {
            if (defaultPlot_.isConfigured()) encoder_.sendPlotConfig(defaultPlot_);
        }

        // Per-plot configs (only dirty ones)
        for (int i = 0; i < plotCount_; i++) {
            if (dirtyFlags_ & DIRTY_PLOT_N(i)) {
                if (plots_[i].isConfigured()) encoder_.sendPlotConfig(plots_[i]);
            }
        }

        clearAllDirtyState();
    }


    Stream* serial_;
    Protocol protocol_;           // Version negotiation and command building
    CommandEncoder encoder_;      // Configuration command encoding

    PlotType plotType_;
    Mode mode_;
    int packetSize_;
    float delayMs_;  // milliseconds between sends (Continuous) or frames (Frames)
    bool legacyMode_;

    uint32_t dirtyFlags_;
    bool needsFullResend_;

    // Dirty flag constants
    static constexpr uint32_t DIRTY_PLOT_TYPE      = 1u << 0;
    static constexpr uint32_t DIRTY_MODE           = 1u << 1;
    static constexpr uint32_t DIRTY_PACKET_SIZE    = 1u << 2;
    static constexpr uint32_t DIRTY_TRACE_LABELS   = 1u << 3;
    static constexpr uint32_t DIRTY_TRACE_COLORS   = 1u << 4;
    static constexpr uint32_t DIRTY_DERIVED_TRACES = 1u << 5;
    static constexpr uint32_t DIRTY_POLAR_CONFIG   = 1u << 6;
    static constexpr uint32_t DIRTY_DEFAULT_PLOT   = 1u << 7;
    // Bits 8-15: per-plot dirty flags (DIRTY_PLOT_N)
    static constexpr uint32_t DIRTY_TRACE_TOPOLOGY = 1u << 16;
    static constexpr uint32_t DIRTY_SKETCH_TITLE  = 1u << 17;
    static constexpr uint32_t DIRTY_ALL            = 0xFFFFFFFF;

    static constexpr uint32_t DIRTY_PLOT_N(int n) { return 1u << (8 + n); }

    Plot defaultPlot_;
    Plot plots_[VIEWPOINT_MAX_PLOTS];
    int plotCount_;

    Trace* traces_[VIEWPOINT_MAX_TRACES];
    int traceCount_;
    DerivedTrace derivedTraces_[VIEWPOINT_MAX_DERIVED_TRACES];
    int derivedTraceCount_;

    Trace dummyTrace_;  // Returned on error
    size_t pointsSent_;

    AngularUnit angularUnit_ = AngularUnit::Degrees;
    float angularOffset_ = 0;
    int angularStep_ = 15;

    char sketchTitle_[VIEWPOINT_TITLE_SIZE] = {'\0'};
};

} // namespace viewpoint

#endif // _VIEWPOINT_PLOTTER_H_
