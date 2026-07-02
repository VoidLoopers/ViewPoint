/**
 * HelloScatter.ino
 * @version 1.0.0
 * @date 05-12-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * This is the "Hello World" of ViewPoint's Scatter (XY) plotting - the absolute
 * minimum code needed to get paired data on screen. Reads two analog pins and
 * plots them as X-Y coordinates.
 *
 * Effective sample rate: ~500 sample pairs/sec (2ms inter-sample delay, the default).
 * The actual interval is 2ms plus two analogRead calls (~20µs total) and serial
 * overhead, but the 2ms delay dominates. For controlled sample rates, see TimedScatter.
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
 * Hardware:
 * - Connect potentiometers or sensors to A0 and A1.
 * - On the generic ESP32 Dev Module, A0/A1 are not real ADC pins — substitute
 *   real ADC pins (e.g. GPIO 34 and GPIO 35) below.
 * - The sketch also runs with floating inputs if you want to inspect ambient noise.
 *
 * Things to try:
 * - Connect two potentiometers for an Etch-A-Sketch effect.
 * - Touch one pin to see 60 Hz pickup as a scatter cloud.
 * - Add view.setAxisLabels("A0", "A1") in setup() to label the axes.
 */

#include <ViewPoint.h>

void setup() {
    // `xy` is the friendly alias for viewpoint::PlotType::Scatter — see
    // ViewPoint.h for the full set of shortcut macros.
    view.begin(xy);
    view.setTitle("Hello Scatter");
}

void loop() {
    // Each call adds one (X, Y) point: A0 is X, A1 is Y.
    view.addData("XY", analogRead(A0), analogRead(A1));
    view.send();
}
