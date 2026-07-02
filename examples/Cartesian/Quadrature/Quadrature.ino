/**
 * Quadrature.ino
 * @version 1.0.0
 * @date 03-28-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * Quadrature.ino - Animated sine and cosine waveforms with amplitude modulation
 *
 * Generates sine and cosine waveforms in quadrature and slowly modulates their
 * amplitude to create a breathing animation. It is a compact example for comparing
 * Continuous and Frames mode with purely synthetic data.
 *
 * This demonstrates:
 * - Cartesian plotting with generated signals
 * - Continuous mode versus Frames mode
 * - Slow amplitude modulation on paired traces
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
 * - Switch between Continuous and Frames mode (see setup comments).
 * - Change the amplitude modulation rate (AMPLITUDE_STEP).
 * - Add a third trace: view.addData("Tan", amplitude * tan(theta));
 * - Try different colors using view.trace(0).setColor(colors::Red);
 */

#include <ViewPoint.h>
#include <math.h>

using namespace viewpoint;
// #include "Arduino.h"

// Configuration
const int PACKET_SIZE = 500;       // Points per frame (for Frames mode)
const float THETA_STEP = 0.05f;    // Angular step - smaller = smoother
const float AMPLITUDE_STEP = 0.01f; // How fast amplitude changes
const float MAX_AMPLITUDE = 2.0f;

// State
float theta = 0;
float amplitude = 0;

void setup() {
    // --- Choose your mode ---
    // Default: Continuous mode: data scrolls in from the right, ideal for real-time monitoring
    view.begin();

    view.setTitle("Quadrature");

    // Frames mode: entire frame replaced at once, ideal for FFT or animations
    // view.begin(Mode::Frames, PACKET_SIZE);
    // view.setDelay(16);
}

void loop() {
    // In Frames mode, we fill an entire packet before sending
    // In Continuous mode, we send after each point (or small batch)

    for (int i = 0; i < PACKET_SIZE; i++) {
        // Generate quadrature signals with amplitude modulation
        float sine = amplitude * sin(theta);
        float cosine = amplitude * cos(theta);

        // Add data to both traces
        view.addData("Sine", sine);
        view.addData("Cosine", cosine);

        // In Continuous mode, send after adding each point pair
        // This gives smooth scrolling behavior 
        view.send();

        // Advance phase (wrap to avoid float precision loss at large values)
        theta = viewpoint::wrapPositive(theta + THETA_STEP, 2.0f * (float)M_PI);
    }

    // In Frames mode, you would call view.send() here instead (once per frame)
    // view.send();


    // Slowly modulate the amplitude (creates "breathing" effect)
    amplitude = viewpoint::wrapPositive(amplitude + AMPLITUDE_STEP, MAX_AMPLITUDE);
}
