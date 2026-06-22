/******************************************************************************
 * @file Types.h
 * @brief Common types, enums, and constants for the ViewPoint library
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


#ifndef _VIEWPOINT_TYPES_H_
#define _VIEWPOINT_TYPES_H_

#include <stdint.h>
#include <stddef.h>
#include <float.h>


// ── Memory profile auto-detection ──────────────────────────────────
//
// ViewPoint auto-selects a memory profile based on the target board.
// Users can override by defining one of these before #include <ViewPoint.h>:
//
//   #define VIEWPOINT_FULL       — Full defaults (16 traces, 8 plots)
//   #define VIEWPOINT_LITE       — Reduced footprint (~5 KB RAM)
//   #define VIEWPOINT_MINIMAL    — Minimum footprint (~2 KB RAM)
//
// Or override individual VIEWPOINT_MAX_* constants directly.

#if !defined(VIEWPOINT_FULL) && !defined(VIEWPOINT_LITE) && !defined(VIEWPOINT_MINIMAL)

  // Tiny AVR (2-2.5 KB RAM): UNO R3, Nano, Leonardo, Micro
  #if defined(__AVR__) && !defined(__AVR_ATmega2560__) && !defined(__AVR_ATmega1280__)
    #define VIEWPOINT_MINIMAL
    #define _VIEWPOINT_AUTODETECTED

  // Medium AVR (8 KB): Mega 2560 / Mega ADK
  #elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
    #define VIEWPOINT_LITE
    #define _VIEWPOINT_AUTODETECTED

  // UNO R4 (32 KB RAM, Renesas RA4M1)
  #elif defined(ARDUINO_ARCH_RENESAS)
    #define VIEWPOINT_LITE
    #define _VIEWPOINT_AUTODETECTED

  #endif
#endif


// ── Apply profile presets ──────────────────────────────────────────
//
// Presets set defaults that individual VIEWPOINT_MAX_* overrides
// still take precedence over (the #ifndef guards below handle that).

#if defined(VIEWPOINT_MINIMAL)
  // ~2 KB static footprint — for boards with <= 2.5 KB RAM
  #ifndef VIEWPOINT_MAX_TRACES
    #define VIEWPOINT_MAX_TRACES 2
  #endif
  #ifndef VIEWPOINT_MAX_PLOTS
    #define VIEWPOINT_MAX_PLOTS 1
  #endif
  #ifndef VIEWPOINT_MAX_TRACES_PER_PLOT
    #define VIEWPOINT_MAX_TRACES_PER_PLOT 2
  #endif
  #ifndef VIEWPOINT_MAX_DERIVED_TRACES
    #define VIEWPOINT_MAX_DERIVED_TRACES 1
  #endif
  #ifndef VIEWPOINT_MAX_REFERENCE_LINES
    #define VIEWPOINT_MAX_REFERENCE_LINES 2
  #endif
  #ifndef VIEWPOINT_DEFAULT_PACKET_SIZE
    #define VIEWPOINT_DEFAULT_PACKET_SIZE 50
  #endif
  #ifndef VIEWPOINT_LABEL_SIZE
    #define VIEWPOINT_LABEL_SIZE 16
  #endif
  #ifndef VIEWPOINT_UNITS_SIZE
    #define VIEWPOINT_UNITS_SIZE 8
  #endif
  #ifndef VIEWPOINT_TITLE_SIZE
    #define VIEWPOINT_TITLE_SIZE 16
  #endif

  #ifdef _VIEWPOINT_AUTODETECTED
    #warning "ViewPoint: MINIMAL memory profile auto-selected for this board (define VIEWPOINT_LITE or VIEWPOINT_FULL before #include <ViewPoint.h> to override)"
  #endif

#elif defined(VIEWPOINT_LITE)
  // ~5 KB static footprint — for boards with 8-32 KB RAM
  #ifndef VIEWPOINT_MAX_TRACES
    #define VIEWPOINT_MAX_TRACES 4
  #endif
  #ifndef VIEWPOINT_MAX_PLOTS
    #define VIEWPOINT_MAX_PLOTS 2
  #endif
  #ifndef VIEWPOINT_MAX_TRACES_PER_PLOT
    #define VIEWPOINT_MAX_TRACES_PER_PLOT 4
  #endif
  #ifndef VIEWPOINT_MAX_DERIVED_TRACES
    #define VIEWPOINT_MAX_DERIVED_TRACES 4
  #endif
  #ifndef VIEWPOINT_MAX_REFERENCE_LINES
    #define VIEWPOINT_MAX_REFERENCE_LINES 4
  #endif
  #ifndef VIEWPOINT_DEFAULT_PACKET_SIZE
    #define VIEWPOINT_DEFAULT_PACKET_SIZE 100
  #endif
  #ifndef VIEWPOINT_LABEL_SIZE
    #define VIEWPOINT_LABEL_SIZE 24
  #endif
  #ifndef VIEWPOINT_UNITS_SIZE
    #define VIEWPOINT_UNITS_SIZE 12
  #endif
  #ifndef VIEWPOINT_TITLE_SIZE
    #define VIEWPOINT_TITLE_SIZE 32
  #endif

  #ifdef _VIEWPOINT_AUTODETECTED
    #warning "ViewPoint: LITE memory profile auto-selected for this board (define VIEWPOINT_FULL before #include <ViewPoint.h> to override)"
  #endif

#endif // profile presets


// ── Configurable limits (final defaults for FULL / unset values) ───

#ifndef VIEWPOINT_MAX_TRACES
#define VIEWPOINT_MAX_TRACES 16
#endif

#ifndef VIEWPOINT_MAX_PLOTS
#define VIEWPOINT_MAX_PLOTS 8
#endif

#ifndef VIEWPOINT_MAX_TRACES_PER_PLOT
#define VIEWPOINT_MAX_TRACES_PER_PLOT 16
#endif

#ifndef VIEWPOINT_MAX_DERIVED_TRACES
#define VIEWPOINT_MAX_DERIVED_TRACES (VIEWPOINT_MAX_TRACES * 2)
#endif

#ifndef VIEWPOINT_MAX_REFERENCE_LINES
#define VIEWPOINT_MAX_REFERENCE_LINES 8
#endif

#ifndef VIEWPOINT_DEFAULT_PACKET_SIZE
#define VIEWPOINT_DEFAULT_PACKET_SIZE 500
#endif

#ifndef VIEWPOINT_LABEL_SIZE
#define VIEWPOINT_LABEL_SIZE 32
#endif

#ifndef VIEWPOINT_UNITS_SIZE
// recommended to keep units < 8 characters
#define VIEWPOINT_UNITS_SIZE 16
#endif

#ifndef VIEWPOINT_TITLE_SIZE
#define VIEWPOINT_TITLE_SIZE 64
#endif

namespace viewpoint {


// Plot Types
enum class PlotType : uint8_t {
    Cartesian = 0,
    Scatter   = 1,
    Polar     = 2
};


// Plotting Modes
enum class Mode : uint8_t {
    Frames     = 0,   // Complete replacement each packet
    Continuous = 1    // Continuous streaming (scrolling)
};


// Display Modes (visualization effects)
enum class DisplayMode : uint8_t {
    None        = 0,
    Spectrogram = 1,
    Persistence = 2,
    Gradient    = 3
};


// Data Format for transmission
enum class DataFormat : uint8_t {
    Text   = 0,   // Human-readable (default)
    Binary = 1    // Opt-in for speed
};


// Angular units for polar plots
enum class AngularUnit : uint8_t {
    Degrees = 0,
    Radians = 1
};


// Axis identifier
enum class AxisId : uint8_t {
    X = 0,   // Horizontal / Primary
    Y = 1    // Vertical / Secondary
};

enum class LineType : uint8_t {
    Solid = 0,
    Dashed,
    Dotted,
    Pattern
};

// Range structure
struct Range {
    float minimum = 0.0f;
    float maximum = 0.0f;
    bool set = false;

    Range() = default;
    Range(float min_, float max_) : minimum(min_), maximum(max_), set(true) {}

    float span() const { return maximum - minimum; }
    float center() const { return (minimum + maximum) / 2.0f; }
    bool valid() const { return set && (maximum > minimum); }
};


// Step structure (for grid lines)


struct Step {
    float minor = 0.0f;
    float major = 0.0f;
    bool set = false;

    Step() = default;
    Step(float minor_, float major_) : minor(minor_), major(major_), set(true) {}
};


// Reference line structure


struct ReferenceLine {
    float value = 0.0f;
    uint32_t color = 0x666666;
    float strokeWeight = 1.0f;
    bool isMajor = false;
    bool set = false;
    LineType style;

    ReferenceLine() = default;
    ReferenceLine(float val, bool major = false)
        : value(val), isMajor(major), set(true), style(LineType::Solid) {}
    ReferenceLine(float val, uint32_t col, float stroke = 1.0f)
        : value(val), color(col), strokeWeight(stroke), set(true), style(LineType::Solid) {}
    ReferenceLine(float val, uint32_t col, LineType type, float stroke = 1.0f)
        : value(val), color(col), strokeWeight(stroke), set(true), style(type) {}
};


// Grid colors structure
struct GridColors {
    uint32_t labels = 0x969696;
    uint32_t minor  = 0x333333;
    uint32_t major  = 0x666666;
    float strokeWeight = 1.0f;
    bool set = false;
};


// Display mode configuration
struct DisplayModeConfig {
    DisplayMode mode = DisplayMode::None;
    int traceId = 0;
    uint32_t colors[8] = {0};
    int colorCount = 0;
    bool set = false;
};


// Utility constants
constexpr float UNDEFINED_FLOAT = FLT_MAX;
constexpr int UNDEFINED_INT = -1;
constexpr uint32_t NO_COLOR = 0xFFFFFFFF;


// Utility functions
inline bool isDefined(float value) {
    return value != UNDEFINED_FLOAT;
}

inline bool isDefined(int value) {
    return value != UNDEFINED_INT;
}

inline bool hasColor(uint32_t color) {
    return color != NO_COLOR;
}

} // namespace viewpoint

#endif // _VIEWPOINT_TYPES_H_
