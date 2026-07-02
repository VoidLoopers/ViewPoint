/**
 * LissajousCurve.ino
 * @version 1.0.0
 * @date 03-28-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * LissajousCurve.ino - Rotating Lissajous figure in Scatter mode
 *
 * Generates a classic parametric Lissajous figure and continuously rotates it in
 * the XY plane. The sketch is a lightweight way to study how phase, frequency ratio,
 * and amplitude shape a 2D curve while also demonstrating frame-based scatter updates.
 *
 * This demonstrates:
 * - Scatter plotting with generated XY data
 * - Parametric signal generation using sine waves
 * - Smooth in-plane rotation of a 2D figure
 *
 * Verified hardware:
 * - Teensy 4.1
 *
 * Known issues:
 * - None recorded for this sketch
 *
 * Board-specific requirements:
 * - Uses only core ViewPoint APIs and standard Arduino runtime features.
 * - Only Teensy 4.1 has been hardware-verified for this sketch so far.
 *
 * Hardware:
 * - No external hardware required.
 *
 * Things to try:
 * - Change fx and fy to explore different Lissajous shapes.
 * - Set fx == fy to see the curve collapse into simpler forms.
 * - Adjust ampX and ampY to stretch or compress the figure.
 * - Modify rotSpeed to change the rotation rate.
 */

#include "ViewPoint.h"

int packetSize = 500;
float period = 2 * M_PI;
float step = period / packetSize;
float delta = M_PI / 2;

float fx = 3;
float fy = 2;
float ampX = 5;
float ampY = 10;

float rotSpeed = 0.6f;   // radians/sec

void setup() {
  view.begin(scatter, frames);
  view.setTitle("Lissajous Curve");
}

void loop() {
  float a = (millis() * 0.001f) * rotSpeed;
  float ca = cos(a), sa = sin(a);

  for (float t = 0; t <= period; t += step) {
    float x = ampX * sin(fx * t + delta);
    float y = ampY * sin(fy * t);

    // Rotate in XY plane (Z axis)
    float xr = x * ca - y * sa;
    float yr = x * sa + y * ca;

    view.addData("Lissajous", xr, yr);
  }
  view.send();
}
