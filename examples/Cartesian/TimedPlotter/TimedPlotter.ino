/**
 * TimedPlotter.ino
 * @version V1R1
 * @date 03-25-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * Demonstrates how to maintain a precise, consistent sample rate using micros().
 * This is the next step after HelloPlotter - same basic idea (read A0, plot it),
 * but with proper timing so the data is meaningful for analysis.
 *
 * Why it matters: HelloPlotter uses the library's built-in delay (~500 Hz, approximate).
 * That's fine for seeing a signal, but if you want to measure frequency content,
 * correlate with time, or compare across runs, you need a known sample rate.
 * At 1000 Hz, the Nyquist limit is 500 Hz - you can resolve signals up to 500 Hz
 * without aliasing.
 *
 * How it works: The loop runs as fast as possible, but only samples when the
 * interval has elapsed. This gives a stable sample rate independent of how long
 * analogRead or serial overhead takes (as long as they're faster than the interval).
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
 * - Only Teensy 4.1 has been hardware-verified for this sketch so far; the additional boards above are compile-verified only.
 *
 * Hardware:
 * - Optional: connect a potentiometer or sensor to A0.
 * - The sketch also runs with a floating input if you want to inspect ambient noise at a known sample rate.
 *
 * Things to try:
 * - Change SAMPLE_RATE to 2000, 5000, or 10000 and observe the difference
 * - Touch A0 and look for the 60 Hz spike - it should appear at exactly 60 Hz
 * - Add view.addData("A1", analogRead(A1)) for a second channel at the same rate
 * - Use setDelay(0) and time everything yourself for maximum throughput
 */

#include <ViewPoint.h>

const float    SAMPLE_RATE      = 10000.0;  // Hz - change this to set your rate
const uint32_t SAMPLE_INTERVAL  = (uint32_t)(1000000.0 / SAMPLE_RATE);  // microseconds

uint32_t lastSampleTime = 0;

void setup() {
    view.begin();
    view.setDelay(0);  // We handle timing ourselves
    view.setTitle("Timed Plotter");
    view.setAxisLabels("Time", "ADC");
    view.setUnits("", "counts");
}

void loop() {
    uint32_t now = micros();

    if (now - lastSampleTime >= SAMPLE_INTERVAL) {
        lastSampleTime += SAMPLE_INTERVAL;  // Accumulate to avoid drift

        // Guard against falling behind: if we missed samples (e.g., due to
        // a long serial write), reset timing rather than bursting to catch up
        if (now - lastSampleTime >= SAMPLE_INTERVAL * 2) {
            lastSampleTime = now;
        }

        view.addData("A0", analogRead(A0));
        view.send();
    }
}
