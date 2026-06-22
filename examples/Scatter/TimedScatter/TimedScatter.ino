/**
 * TimedScatter.ino
 * @version V1R1
 * @date 03-25-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * Demonstrates how to maintain a precise, consistent sample rate for XY scatter
 * data using micros(). This is the next step after HelloScatter - same idea
 * (read A0 and A1 as X-Y pairs), but with proper timing.
 *
 * Why it matters: HelloScatter relies on the library's built-in 1ms delay
 * (~1000 sample pairs/sec, approximate). That's fine for seeing the shape of
 * your data, but if you need consistent sample spacing - for example, to detect
 * periodic orbits in phase space, or to animate at a predictable speed - you
 * need controlled timing.
 *
 * How it works: The loop runs as fast as possible, but only samples when the
 * interval has elapsed. Both channels are read at the same instant, which keeps
 * the X-Y pairing coherent.
 *
 * Verified hardware:
 * - Teensy 4.1
 *
 * Build-verified:
 * - Arduino Giga R1
 * - ESP32 Dev Module
 * - Raspberry Pi Pico
 * - Arduino UNO Q
 * - Arduino Uno
 *
 * Known issues:
 * - None recorded for this sketch
 *
 * Board-specific requirements:
 * - Uses only core ViewPoint APIs and standard Arduino analog inputs.
 * - On the generic ESP32 Dev Module target, the example defaults to GPIO34 and GPIO35; override VIEWPOINT_SCATTER_X_PIN and VIEWPOINT_SCATTER_Y_PIN if your board uses different analog inputs.
 * - Only Teensy 4.1 has been hardware-verified for this sketch so far; the additional boards above are compile-verified only.
 *
 * Hardware:
 * - Connect potentiometers or sensors to A0 and A1 on most Arduino-compatible boards.
 * - On the generic ESP32 Dev Module target, the default input pins are GPIO34 and GPIO35.
 *
 * Things to try:
 * - Change SAMPLE_RATE to see how density of points on the scatter plot changes
 * - Connect a joystick to the configured X/Y inputs and draw patterns at a known rate
 * - Try 100 Hz for slow, visible point-by-point drawing
 * - Add view.setAxisLabels(VIEWPOINT_SCATTER_X_LABEL, VIEWPOINT_SCATTER_Y_LABEL) for board-accurate labels
 */

#include <ViewPoint.h>

#ifndef VIEWPOINT_SCATTER_X_PIN
    #if defined(ARDUINO_ARCH_ESP32)
        #define VIEWPOINT_SCATTER_X_PIN 34
    #else
        #define VIEWPOINT_SCATTER_X_PIN A0
    #endif
#endif

#ifndef VIEWPOINT_SCATTER_Y_PIN
    #if defined(ARDUINO_ARCH_ESP32)
        #define VIEWPOINT_SCATTER_Y_PIN 35
    #else
        #define VIEWPOINT_SCATTER_Y_PIN A1
    #endif
#endif

#ifndef VIEWPOINT_SCATTER_X_LABEL
    #if defined(ARDUINO_ARCH_ESP32)
        #define VIEWPOINT_SCATTER_X_LABEL "GPIO34"
    #else
        #define VIEWPOINT_SCATTER_X_LABEL "A0"
    #endif
#endif

#ifndef VIEWPOINT_SCATTER_Y_LABEL
    #if defined(ARDUINO_ARCH_ESP32)
        #define VIEWPOINT_SCATTER_Y_LABEL "GPIO35"
    #else
        #define VIEWPOINT_SCATTER_Y_LABEL "A1"
    #endif
#endif

#ifndef VIEWPOINT_SCATTER_TRACE_LABEL
    #define VIEWPOINT_SCATTER_TRACE_LABEL "XY"
#endif

const float    SAMPLE_RATE      = 1000.0;  // Hz
const uint32_t SAMPLE_INTERVAL  = (uint32_t)(1000000.0 / SAMPLE_RATE);

uint32_t lastSampleTime = 0;

void setup() {
    view.begin(viewpoint::PlotType::Scatter, viewpoint::Mode::Continuous);
    view.setDelay(0);
    view.setTitle("Timed Scatter");
    view.setAxisLabels(VIEWPOINT_SCATTER_X_LABEL, VIEWPOINT_SCATTER_Y_LABEL);
    view.setUnits("counts", "counts");
}

void loop() {
    uint32_t now = micros();

    if (now - lastSampleTime >= SAMPLE_INTERVAL) {
        lastSampleTime += SAMPLE_INTERVAL;

        if (now - lastSampleTime >= SAMPLE_INTERVAL * 2) {
            lastSampleTime = now;
        }

        // Read both channels as close together as possible
        int x = analogRead(VIEWPOINT_SCATTER_X_PIN);
        int y = analogRead(VIEWPOINT_SCATTER_Y_PIN);

        view.addData(VIEWPOINT_SCATTER_TRACE_LABEL, x, y);
        view.send();
    }
}
