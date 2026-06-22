/******************************************************************************
 * @file Platform.h
 * @brief Platform detection and compatibility shims for ViewPoint
 *
 * Provides portable alternatives for functions that are not available on all
 * Arduino cores. Each shim is compiled only on platforms that need it.
 *
 * Supported platform families:
 *   AVR          (__AVR__)                 - UNO, Mega, Nano, Leonardo, etc.
 *   megaAVR      (ARDUINO_ARCH_MEGAAVR)    - Nano Every, UNO WiFi Rev2
 *   Teensy       (TEENSYDUINO)             - Teensy 3.x / 4.x
 *   ESP32        (ARDUINO_ARCH_ESP32)      - All ESP32 variants
 *   ESP8266      (ARDUINO_ARCH_ESP8266)    - ESP8266 / NodeMCU
 *   SAMD         (ARDUINO_ARCH_SAMD)       - Zero, MKR family
 *   RP2040       (ARDUINO_ARCH_RP2040)     - Raspberry Pi Pico
 *   Renesas      (ARDUINO_ARCH_RENESAS)    - UNO R4 Minima / WiFi
 *   Mbed         (ARDUINO_ARCH_MBED)       - GIGA R1, Nano 33 BLE, Portenta
 *   STM32        (ARDUINO_ARCH_STM32)      - STM32 (STM32duino core)
 *   nRF52        (ARDUINO_ARCH_NRF52840)   - Nano 33 BLE (Adafruit core)
 *
 * Contributors:
 * Initial vision, feature definition, debugging and funding: Greg Kovacs
 * Architecture, Development, debugging and support: Zachariah Magee
 *
 * @copyright Copyright (c) VoidLoop
 ******************************************************************************/

#ifndef _VIEWPOINT_PLATFORM_H_
#define _VIEWPOINT_PLATFORM_H_

#include <Arduino.h>

namespace viewpoint {
namespace detail {

// ── Portable float-to-string ───────────────────────────────────────
//
// AVR libc's printf does not support %f, so dtostrf (from <stdlib.h>)
// is the only lightweight option on 8-bit AVR.
//
// Most 32-bit cores (ARM, RISC-V, Xtensa) fully support snprintf %f
// but some — notably Renesas (UNO R4) and Mbed (GIGA, Nano 33 BLE) —
// do not provide dtostrf.
//
// This helper picks the right implementation at compile time.

inline void floatToStr(char* buf, size_t bufSize, float value, int precision) {
#if defined(__AVR__)
    // AVR: dtostrf is always available; snprintf lacks %f support
    (void)bufSize;
    dtostrf(value, 1, precision, buf);
#else
    // 32-bit cores: snprintf %f is universally supported
    snprintf(buf, bufSize, "%.*f", precision, (double)value);
#endif
}

// ── Cooperative yield ─────────────────────────────────────────────
//
// RTOS-based cores (ESP32, ESP8266, RP2040) run background tasks
// (WiFi, BLE, watchdog) that must be serviced regularly. On these
// platforms, long-running loops without yield() can trigger watchdog
// resets or starve network stacks.
//
// Bare-metal cores (AVR, Teensy, STM32) have no scheduler to yield
// to, so this compiles to a no-op.

inline void platformYield() {
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266) || \
    defined(ARDUINO_ARCH_RP2040)
    yield();
#endif
}

// ── Platform-aware delay ─────────────────────────────────────────
//
// Wraps delay() with an explicit yield for RTOS platforms. A zero-ms
// delay still yields once so background tasks get a timeslice.

inline void platformDelay(unsigned long ms) {
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266) || \
    defined(ARDUINO_ARCH_RP2040)
    if (ms == 0) { yield(); return; }
#endif
    delay(ms);
}

} // namespace detail

// ── Positive wrap helper ─────────────────────────────────────────
//
// Several examples need a portable positive modulo for angles, hue values,
// and animation phases. This is effectively fmod(value, period) with
// negative results normalized into the [0, period) range, but implemented
// without direct libc fmod/fmodf calls because some Arduino cores link
// those functions inconsistently across MCUs and board packages.

inline float wrapPositive(float value, float period) {
    while (value >= period) value -= period;
    while (value < 0.0f) value += period;
    return value;
}

inline double wrapPositive(double value, double period) {
    while (value >= period) value -= period;
    while (value < 0.0) value += period;
    return value;
}

} // namespace viewpoint

#endif // _VIEWPOINT_PLATFORM_H_
