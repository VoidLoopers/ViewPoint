/**
 * TimedPolar.ino
 * @version 1.0.0
 * @date 03-25-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * Demonstrates how to maintain a precise sample rate and angular sweep speed
 * on a polar plot using micros(). This is the next step after HelloPolar.
 *
 * Why it matters: HelloPolar steps 1 degree per sample with a ~2ms delay,
 * giving approximately 1.4 revolutions/sec - but the exact speed depends on
 * analogRead time and serial overhead. This sketch separates the sample rate
 * from the sweep speed, so both are explicit and controllable.
 *
 * How it works: The loop samples at a fixed rate. The angle advances based
 * on the angular step per sample, which is derived from the desired sweep
 * speed and sample rate:
 *   angular_step = (SWEEP_RPM * 360) / (60 * SAMPLE_RATE)
 *
 * At the defaults (1000 Hz, 60 RPM = 1 rev/sec), each sample advances 0.36
 * degrees, giving 1000 points per revolution - smooth and precisely timed.
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
 * - Connect a potentiometer or sensor to A0.
 *
 * Things to try:
 * - Change SWEEP_RPM to 30 (slow) or 120 (fast) and watch the sweep speed
 * - Change SAMPLE_RATE to 500 for fewer points per revolution (coarser)
 * - Add view.setRadialRange(0, 1023) to fix the scale
 * - Try ANGULAR_STEP = 1.0 for a fixed 1-degree step (like HelloPolar, but timed)
 */

#include <ViewPoint.h>

const float    SAMPLE_RATE   = 1000.0;  // Hz
const float    SWEEP_RPM     = 60.0;    // Revolutions per minute (60 RPM = 1 rev/sec)
const uint32_t SAMPLE_INTERVAL = (uint32_t)(1000000.0 / SAMPLE_RATE);

// Degrees per sample: at 1000 Hz and 60 RPM, this is 0.36 deg/sample
const float ANGULAR_STEP = (SWEEP_RPM * 360.0) / (60.0 * SAMPLE_RATE);

uint32_t lastSampleTime = 0;
float theta = 0.0;

void setup() {
    view.begin(viewpoint::PlotType::Polar);
    view.setDelay(0);
    view.setTitle("Timed Polar");
}

void loop() {
    uint32_t now = micros();

    if (now - lastSampleTime >= SAMPLE_INTERVAL) {
        lastSampleTime += SAMPLE_INTERVAL;

        if (now - lastSampleTime >= SAMPLE_INTERVAL * 2) {
            lastSampleTime = now;
        }

        view.addData("A0", analogRead(A0), theta);
        view.send();

        theta += ANGULAR_STEP;
        if (theta >= 360.0) theta -= 360.0;
    }
}
