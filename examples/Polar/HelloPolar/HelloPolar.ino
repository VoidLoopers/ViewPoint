/**
 * HelloPolar.ino
 * @version V1R1 R0.2
 * @date 01-27-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * This is the "Hello World" of ViewPoint's Polar plotting - the absolute minimum
 * code needed to get radial data on screen. Reads an analog pin and sweeps it
 * around the polar plot one degree at a time.
 *
 * Effective sample rate: ~500 samples/sec (2ms inter-sample delay, the default).
 * At 1 degree per sample, one full revolution takes ~720ms (~1.4 revolutions/sec).
 * The timing is not precise because it relies on delay() rather than a
 * fixed-interval timer. For applications that need a known, stable sample rate
 * or a specific angular sweep speed, see the TimedPolar example.
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
 * - The sketch also runs with a floating input if you want to inspect ambient noise as a radial pattern.
 *
 * Things to try:
 * - Connect a potentiometer to A0 and turn it while the sweep runs
 * - Touch the A0 pin to see 60Hz mains pickup as a radial pattern
 * - Change the angular step (e.g., theta += 5) for a faster, coarser sweep
 * - Add view.setRadialRange(0, 1023) in setup() to fix the radial scale
 */

#include <ViewPoint.h>

void setup() {
    // Polar plot, Continuous mode, 2ms inter-sample delay (default).
    // At 1°/sample this sweeps ~1.4 revolutions/sec - enough to see the
    // shape of a signal, but the timing is not precise.
    // See TimedPolar for controlled sample rates and sweep speeds.
    //
    // `polar` is a friendly alias for viewpoint::PlotType::Polar. See
    // ViewPoint.h for the full set of namespace shortcuts (cartesian,
    // scatter, xy, polar, frames, continuous, ...).
    view.begin(polar);
    view.setTitle("Hello Polar");
}

int theta = 0;

void loop() {
    // Each call adds one point: radius = analogRead, angle = theta (degrees)
    view.addData("A0", analogRead(A0), theta);
    view.send();

    theta = (theta + 1) % 360;
}

