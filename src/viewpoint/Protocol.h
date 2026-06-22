/******************************************************************************
 * @file Protocol.h
 * @brief Version-aware protocol handling for ViewPoint
 *
 * This file implements the ViewPoint protocol negotiation and command
 * composition system. It handles:
 *
 * 1. VERSION NEGOTIATION
 *    - On serial connection, the app sends its version: [viewpoint]version=V1R1
 *    - Library responds with its version
 *    - Commands are composed based on the minimum compatible version
 *    - If no version received within timeout, falls back to data-only mode
 *
 * 2. COMMAND COMPOSITION
 *    - Commands use human-readable format: [command]arg=value,arg2=value2
 *    - CommandBuilder provides fluent API for composing commands
 *    - Version checks allow graceful degradation for older apps
 *
 * 3. CONNECTION STATES
 *    - Disconnected: No serial connection
 *    - AwaitingVersion: Connected, waiting for app version (with timeout)
 *    - Negotiated: Version handshake complete, full protocol available
 *    - DataOnly: Timeout or non-ViewPoint receiver, send data only
 *
 *
 * Please see www.voidloop.com for updates and shop.voidloop.com to purchase
 * the ViewPoint desktop application
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

#ifndef _VIEWPOINT_PROTOCOL_H_
#define _VIEWPOINT_PROTOCOL_H_

#include "Types.h"
#include "Platform.h"
#include <Arduino.h>

namespace viewpoint {


// Library Version Constants


namespace version {
    constexpr uint8_t MAJOR = 1;
    constexpr uint8_t REVISION = 1;
    constexpr const char* STRING = "V1R1";
}


// Command Tokens
// These are the human-readable command identifiers sent over serial.
// They should match the app-side Commands.specs map exactly.


namespace cmd {
    constexpr const char* VIEWPOINT      = "[viewpoint]";
    constexpr const char* PACKET_SIZE    = "[packet size]";
    constexpr const char* CARTESIAN      = "[cartesian]";
    constexpr const char* SCATTER        = "[scatter]";
    constexpr const char* POLAR          = "[polar]";
    constexpr const char* FRAMES         = "[frames]";
    constexpr const char* CONTINUOUS     = "[continuous]";
    constexpr const char* ADD_TRACES     = "[add traces]";
    constexpr const char* TRACE_FILTER   = "[trace filter]";
    constexpr const char* UNITS          = "[units]";
    constexpr const char* DISPLAY_MODE   = "[display mode]";
    constexpr const char* PLOT_LABELS    = "[plot labels]";
    constexpr const char* TRACE_COLOR    = "[trace color]";
    constexpr const char* GRID_COLORS    = "[grid colors]";
    constexpr const char* TRACE_LABELS   = "[trace labels]";
    constexpr const char* LOG_SCALE      = "[log scale]";
    constexpr const char* REFERENCE_LINE = "[reference line]";
    constexpr const char* AXIS           = "[axis]";
    constexpr const char* TIMESTAMP      = "[time]";
    constexpr const char* MESSAGE        = "[message]";
    constexpr const char* ERROR          = "[error]";
    constexpr const char* SKETCH         = "[sketch]";
}

// Tokens the library *receives* from the host. Today only [viewpoint] is
// parsed; future host->library control channels (delay/variable) will be
// re-added here when implemented — see notes/TODOs.md.
namespace rcv {
    constexpr const char* VIEWPOINT     = "[viewpoint]";
};


// Connection State


enum class ConnectionState : uint8_t {
    Disconnected,     // Not connected to serial
    AwaitingVersion,  // Connected, waiting for app to send version
    Negotiated,       // Version handshake complete, full protocol
    DataOnly          // Timeout or non-ViewPoint receiver, data only
};


// Protocol Configuration

#ifndef VIEWPOINT_CONFIG_RESEND_COOLDOWN_MS
#define VIEWPOINT_CONFIG_RESEND_COOLDOWN_MS 200
#endif


// CommandBuilder
//
// Fluent API for composing commands with named arguments.
// Usage:
//   builder.begin("[axis]")
//       .arg("plot", 0)
//       .arg("axis", 'y')
//       .arg("min", -10.0f)
//       .argIf(hasMax, "max", 10.0f)
//       .send();
//
// Output: [axis]plot=0,axis=y,min=-10,max=10


class CommandBuilder {
public:
    CommandBuilder() : serial_(nullptr), argCount_(0) {}

    /**
     * @brief Set the output stream
     */
    void setStream(Stream* serial) {
        serial_ = serial;
    }

    /**
     * @brief Start a new command
     * @param command The command token (e.g., "[axis]")
     * @return Reference to this builder for chaining
     */
    CommandBuilder& begin(const char* command) {
        if (!serial_) return *this;
        serial_->print(command);
        argCount_ = 0;
        return *this;
    }

    // --- Integer arguments ---

    CommandBuilder& arg(const char* name, int value) {
        if (!serial_) return *this;
        printSeparator();
        serial_->print(name);
        serial_->print('=');
        serial_->print(value);
        return *this;
    }

    /**
     * @brief Add integer argument only if condition is true
     * @param condition If false, argument is skipped
     * @param name Argument name
     * @param value Argument value
     *
     * Use this for version-dependent arguments or optional values:
     *   .argIf(protocol.supports(1,2), "newArg", value)
     *   .argIf(hasValue, "optional", value)
     */
    CommandBuilder& argIf(bool condition, const char* name, int value) {
        if (condition) arg(name, value);
        return *this;
    }

    // --- Float arguments ---

    CommandBuilder& arg(const char* name, float value) {
        if (!serial_) return *this;
        printSeparator();
        serial_->print(name);
        serial_->print('=');
        printFloat(value);
        return *this;
    }

    CommandBuilder& argIf(bool condition, const char* name, float value) {
        if (condition) arg(name, value);
        return *this;
    }

    // --- String arguments ---

    CommandBuilder& arg(const char* name, const char* value) {
        if (!serial_ || !value) return *this;
        printSeparator();
        serial_->print(name);
        serial_->print('=');
        serial_->print(value);
        return *this;
    }

    CommandBuilder& argIf(bool condition, const char* name, const char* value) {
        if (condition) arg(name, value);
        return *this;
    }

    // --- Character arguments ---

    CommandBuilder& arg(const char* name, char value) {
        if (!serial_) return *this;
        printSeparator();
        serial_->print(name);
        serial_->print('=');
        serial_->print(value);
        return *this;
    }

    CommandBuilder& argIf(bool condition, const char* name, char value) {
        if (condition) arg(name, value);
        return *this;
    }

    // --- Boolean arguments ---

    CommandBuilder& arg(const char* name, bool value) {
        if (!serial_) return *this;
        printSeparator();
        serial_->print(name);
        serial_->print('=');
        serial_->print(value ? "true" : "false");
        return *this;
    }

    CommandBuilder& argIf(bool condition, const char* name, bool value) {
        if (condition) arg(name, value);
        return *this;
    }

    // --- Hex color arguments ---

    CommandBuilder& argHex(const char* name, uint32_t value) {
        if (!serial_) return *this;
        printSeparator();
        serial_->print(name);
        serial_->print('=');
        printHex(value);
        return *this;
    }

    CommandBuilder& argHexIf(bool condition, const char* name, uint32_t value) {
        if (condition) argHex(name, value);
        return *this;
    }

    // --- List arguments (comma-separated within the value) ---
    CommandBuilder& argHexList(const char* name, const uint32_t* values, int count) {
        if (!serial_ || !values || count == 0) return *this;
        printSeparator();
        serial_->print(name);
        serial_->print('=');
        for (int i = 0; i < count; i++) {
            printHex(values[i]);
            if (i < count - 1) serial_->print(',');
        }
        return *this;
    }

    CommandBuilder& argList(const char* name, const int* values, int count) {
        if (!serial_ || !values || count == 0) return *this;
        printSeparator();
        serial_->print(name);
        serial_->print('=');
        for (int i = 0; i < count; i++) {
            serial_->print(values[i]);
            if (i < count - 1) serial_->print(',');
        }
        return *this;
    }

    CommandBuilder& argList(const char* name, const char* const* values, int count) {
        if (!serial_ || !values || count == 0) return *this;
        printSeparator();
        serial_->print(name);
        serial_->print('=');
        for (int i = 0; i < count; i++) {
            if (values[i]) {
                serial_->print(values[i]);
            } else {
                serial_->print('-');  // Placeholder for null labels
            }
            if (i < count - 1) serial_->print(',');
        }
        return *this;
    }

    /**
     * @brief Finish and send the command (adds newline)
     */
    void send() {
        if (!serial_) return;
        serial_->println();
        argCount_ = 0;

    }

private:
    Stream* serial_;
    int argCount_;

    void printSeparator() {
        if (argCount_++ > 0) {
            serial_->print(',');
        }
    }

    void printFloat(float value) {
        if (value == 0.0f) {
            serial_->print('0');
        } else if (isnan(value)) {
            serial_->print("nan");
        } else if (isinf(value)) {
            serial_->print("inf");
        } else if (fabs(value) >= 4294967040.0f || fabs(value) < 0.001f) {
            char buf[16];
            detail::floatToStr(buf, sizeof(buf), value, 6);
            serial_->print(buf);
        } else if (fabs(value) >= 1000000.0f) {
            serial_->print(value, 2);
        } else if (fabs(value) < 0.1f) {
            serial_->print(value, 6);
        } else if (fabs(value) < 10.0f) {
            serial_->print(value, 4);
        } else {
            serial_->print(value, 2);
        }
    }

    void printHex(uint32_t value) {
        // 24-bit RGB (high byte is 0x00)
        if ((value & 0xFF000000u) == 0u && value <= 0x00FFFFFFu) {
            uint8_t r = (value >> 16) & 0xFF;
            uint8_t g = (value >> 8)  & 0xFF;
            uint8_t b = value & 0xFF;

            char buf[7];

            bool shortR = ((r & 0xF0) >> 4) == (r & 0x0F);
            bool shortG = ((g & 0xF0) >> 4) == (g & 0x0F);
            bool shortB = ((b & 0xF0) >> 4) == (b & 0x0F);

            if (shortR && shortG && shortB) {
                snprintf(buf, sizeof(buf), "%1X%1X%1X", r & 0x0F, g & 0x0F, b & 0x0F);
            } else {
                snprintf(buf, sizeof(buf), "%02X%02X%02X", r, g, b);
            }

            serial_->print(buf);
            return;
        }

        // 32-bit ARGB
        uint8_t a = (value >> 24) & 0xFF;
        uint8_t r = (value >> 16) & 0xFF;
        uint8_t g = (value >> 8)  & 0xFF;
        uint8_t b = value & 0xFF;

        char buf[9];

        if (a != 0xFF) {
            snprintf(buf, sizeof(buf), "%02X%02X%02X%02X", a, r, g, b);
            serial_->print(buf);
            return;
        }

        bool shortR = ((r & 0xF0) >> 4) == (r & 0x0F);
        bool shortG = ((g & 0xF0) >> 4) == (g & 0x0F);
        bool shortB = ((b & 0xF0) >> 4) == (b & 0x0F);

        if (shortR && shortG && shortB) {
            snprintf(buf, sizeof(buf), "%1X%1X%1X", r & 0x0F, g & 0x0F, b & 0x0F);
        } else {
            snprintf(buf, sizeof(buf), "%02X%02X%02X", r, g, b);
        }

        serial_->print(buf);
    }
};


// Protocol
//
// Main protocol handler that manages:
// - Version negotiation with the ViewPoint app
// - Connection state tracking
// - Command building with version awareness
//
// Usage:
//   Protocol protocol;
//   protocol.begin(Serial);
//
//   // In loop:
//   protocol.update();  // Handles version negotiation
//
//   if (protocol.isReady()) {
//       protocol.command("[axis]")
//           .arg("plot", 0)
//           .arg("min", -10.0f)
//           .send();
//   }


class Protocol {
public:
    Protocol()
        : serial_(nullptr)
        , state_(ConnectionState::Disconnected)
        , appMajor_(0)
        , appRevision_(0)
        , configSentAt_(0)
        , versionSent_(false)
        , configRequested_(false)
    {}

    /**
     * @brief Initialize protocol with serial stream
     * @param serial The serial stream to use
     */
    void begin(Stream& serial) {
        serial_ = &serial;
        builder_.setStream(&serial);
        state_ = ConnectionState::AwaitingVersion;
        versionSent_ = false;
    }


    // Update Loop


    /**
     * @brief Update protocol state, handle incoming data
     *
     * Call this in your main loop. It:
     * - Checks for incoming version info from app
     * - Handles timeout to DataOnly mode
     * - Processes acknowledgment signals (! and ?)
     * 
     * @returns boolean: if the config ought to be sent again
     */
    void update() {
        if (!serial_) {
            state_ = ConnectionState::Disconnected;
            versionSent_ = false;
            return;
        }

        // No active "wait timeout → DataOnly" transition. If the app never
        // sends [viewpoint]version=..., the library simply stays in
        // AwaitingVersion and only data lines flow over the serial port —
        // which is exactly what other receivers (e.g. the Arduino IDE
        // Serial Plotter) expect to see. The negotiation/legacy fallback
        // design is tracked in notes/TODOs.md for the next protocol pass.

        // Check for incoming data
        while (serial_->available()) {
            char c = serial_->peek();

            // Check for version message: [viewpoint]version=V1R1,refresh=60
            if (c == '[') {
                String line = serial_->readStringUntil('\n');
                parseIncomingCommand(line.c_str());
            }
            // Check for acknowledgment signals
            else if (c == '!') {
                serial_->read();
                // Acknowledged - config received by app
                configRequested_ = false;
                versionSent_ = true;
            }
            else if (c == '?') {
                serial_->read();
                // Request resend - app didn't receive config
                configRequested_ = true;
            }
            else {
                // Unknown data - consume it
                serial_->read();
            }
        }
    }


    // Version Checks


    /**
     * @brief Check if the app supports a minimum version
     * @param minMajor Minimum major version required
     * @param minRevision Minimum revision required (default: 0)
     * @return true if app version >= min version
     *
     * Use this to conditionally include arguments or commands:
     *   .argIf(protocol.supports(1, 2), "newFeature", value)
     */
    bool supports(uint8_t minMajor, uint8_t minRevision = 0) const {
        if (state_ != ConnectionState::Negotiated) {
            return false;
        }
        return (appMajor_ > minMajor) ||
               (appMajor_ == minMajor && appRevision_ >= minRevision);
    }

    /**
     * @brief Check if the app supports a feature by checking both versions
     *
     * Returns true if the minimum of library and app versions supports
     * the requested version. This ensures compatibility in both directions.
     */
    bool compatible(uint8_t minMajor, uint8_t minRevision = 0) const {
        // Library must support it
        if (version::MAJOR < minMajor) return false;
        if (version::MAJOR == minMajor && version::REVISION < minRevision) return false;

        // App must support it (if negotiated)
        if (state_ == ConnectionState::Negotiated) {
            return supports(minMajor, minRevision);
        }

        // If not negotiated, assume supported (will be data-only anyway)
        return true;
    }


    // State Queries

    bool configRequested() { return configRequested_; }

    ConnectionState state() const { return state_; }

    /**
     * @brief Check if config should be resent
     *
     * Returns true when:
     * - Disconnected (need to send on reconnect)
     * - Version not yet sent
     * - Config was requested (via '?') AND cooldown elapsed
     *
     * The cooldown prevents rapid retransmission before the app
     * has a chance to process and acknowledge the config.
     */
    bool resendConfig() const {
        if (state_ == ConnectionState::Disconnected) return true;
        if (!versionSent_) return true;

        // Only resend if explicitly requested AND cooldown has elapsed
        if (configRequested_ && (millis() - configSentAt_ > VIEWPOINT_CONFIG_RESEND_COOLDOWN_MS)) {
            return true;
        }
        return false;
    }

    bool isDisconnected() const {
        return state_ == ConnectionState::Disconnected;
    }

    bool isReady() const {
        return state_ == ConnectionState::Negotiated ||
               state_ == ConnectionState::DataOnly;
    }

    bool isNegotiated() const {
        return state_ == ConnectionState::Negotiated;
    }

    bool isDataOnly() const {
        return state_ == ConnectionState::DataOnly;
    }

    uint8_t appMajor() const { return appMajor_; }
    uint8_t appRevision() const { return appRevision_; }


    // Command Building


    /**
     * @brief Start building a command
     * @param cmd The command token (e.g., cmd::AXIS)
     * @return Reference to CommandBuilder for chaining
     *
     * Only sends if in Negotiated state. In DataOnly state, commands
     * are silently skipped.
     */
    CommandBuilder& command(const char* cmd) {
        if (state_ == ConnectionState::Negotiated) {
            return builder_.begin(cmd);
        }
        // Return builder anyway but it won't output (serial_ check in builder)
        return nullBuilder_;
    }

    /**
     * @brief Get the command builder for direct use
     */
    CommandBuilder& builder() { return builder_; }


    // Version Handshake


    /**
     * @brief Send library version to app
     *
     * Called automatically during update when app version is received,
     * but can be called manually to initiate handshake.
     */
    void sendVersion() {
        if (!serial_ || versionSent_) return;

        builder_.begin(cmd::VIEWPOINT)
            .arg("version", version::STRING)
            .send();

        versionSent_ = true;
    }

    /**
     * @brief Send configuration complete marker
     *
     * Also marks config as sent and records timestamp for cooldown.
     * This prevents rapid retransmission before the app can acknowledge.
     */
    void sendConfigComplete() {
        if (!serial_ || state_ != ConnectionState::Negotiated) return;

        builder_.begin(cmd::VIEWPOINT)
            .arg("completed", true)
            .send();

        // Mark config as sent - prevents immediate resend
        versionSent_ = true;
        configSentAt_ = millis();
    }


    // Direct Serial Access (for data transmission)


    Stream* serial() { return serial_; }

private:
    Stream* serial_;
    CommandBuilder builder_;
    CommandBuilder nullBuilder_;  // Dummy builder for DataOnly mode

    ConnectionState state_;
    uint8_t appMajor_;
    uint8_t appRevision_;

    uint32_t configSentAt_;
    bool versionSent_;
    bool configRequested_;

    /**
     * @brief Parse a [token]args line received from the app.
     *
     * Today the only command the library reacts to is
     *   [viewpoint]version=VxRy
     * which kicks off the handshake. Other host->library commands
     * (delay, variable, etc.) are tracked in notes/TODOs.md.
     */
    void parseIncomingCommand(const char* line) {
        if (strncmp(line, cmd::VIEWPOINT, strlen(cmd::VIEWPOINT)) != 0) {
            return;
        }
        parseVersion(line);
    }

    void parseVersion(const char* line) {
        configRequested_ = true;

        const char* args = line + strlen(cmd::VIEWPOINT);

        // Parse version: version=V1R1
        const char* versionStart = strstr(args, "version=");
        if (versionStart) {
            versionStart += 8;  // Skip "version="

            // Parse VxRy format
            if (*versionStart == 'V' || *versionStart == 'v') {
                versionStart++;
                appMajor_ = atoi(versionStart);

                const char* revStart = strchr(versionStart, 'R');
                if (!revStart) revStart = strchr(versionStart, 'r');
                if (revStart) {
                    appRevision_ = atoi(revStart + 1);
                }
            }

            // Version received - enter negotiated state
            state_ = ConnectionState::Negotiated;

            // Send our version in response
            sendVersion();
        }
    }
};



} // namespace viewpoint

#endif // _VIEWPOINT_PROTOCOL_H_
