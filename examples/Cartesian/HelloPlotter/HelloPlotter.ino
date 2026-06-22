/**
 * HelloPlotter.ino
 * @version V1R1 R0.2
 * @date 01-27-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * This is the "Hello World" of ViewPoint - the absolute minimum code
 * needed to get data on screen. Just reads an analog pin and plots it.
 *
 * Effective sample rate: ~500 Hz (2ms inter-sample delay, the default).
 * This rate is approximate - the actual interval is 2ms plus the time for
 * analogRead (~10µs) and serial overhead. The timing is not precise because
 * it relies on delay() rather than a fixed-interval timer. For applications
 * that need a known, stable sample rate, see the TimedPlotter example.
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
 * - The sketch also runs with a floating input if you want to inspect ambient noise.
 *
 * Things to try:
 * - Connect a potentiometer to A0 and turn it
 * - Touch the A0 pin with your finger to see 60Hz mains pickup
 * - Add a second trace with view.addData("A1", analogRead(A1));
 */

#include <ViewPoint.h>

void setup() {
    // Default settings: Cartesian, Continuous mode, 2ms inter-sample delay.
    // This gives ~500 samples/sec - fast enough to see most signals, but
    // the timing is not precise. See TimedPlotter for controlled sample rates.
    view.begin();
    view.setTitle("Hello Plotter");
}

void loop() {
    view.addData("A0", analogRead(A0));
    view.send();
}

