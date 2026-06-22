/**
 * ViewPoint_AM_Demo_R1.ino
 * @version V1R1
 * @date 03-28-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * ViewPoint_AM_Demo_R1.ino - Amplitude-modulated carrier demo in Frames mode
 *
 * Generates a carrier whose amplitude is modulated by a lower-frequency sine wave
 * and adds broadband noise before sending the full frame to ViewPoint. The result
 * is a compact demo for frame-based plotting, envelope motion, and signal-plus-noise
 * visualization without requiring external hardware.
 *
 * This demonstrates:
 * - Frames mode packet generation
 * - Synthetic AM signal generation
 * - Noise injection for more realistic displays
 *
 * Verified hardware:
 * - Teensy 4.1
 *
 * Known issues:
 * - None recorded for this sketch
 *
 * Board-specific requirements:
 * - Uses only core ViewPoint APIs and standard Arduino math/runtime features.
 * - A 32-bit board is recommended for smooth 4096-point frame updates; only Teensy 4.1 has been hardware-verified for this sketch so far.
 *
 * Hardware:
 * - No external hardware required.
 *
 * Sketch outline:
 * - Setup: Configure Frames mode and display timing.
 * - Loop: Generate a full AM frame with noise and send it to ViewPoint.
 *
 * Things to try:
 * - Increase modulationFrequency to speed up the envelope motion.
 * - Reduce pointsPerFrame for faster updates and lower spectral detail.
 * - Replace the carrier sine with another waveform to compare modulation shapes.
 */

#include <ViewPoint.h>

int   pointsPerFrame = 4096;
float theta = (2 * M_PI) / pointsPerFrame;
float carrierFrequency = 10, modulationFrequency = 1;
float carrier = 0, modulation = 0, noise = 0, signal = 0;
float peakAmplitude = 4;
 
void setup() {
    // Frames mode: entire frame replaced at once, ideal for FFT or animations
    view.begin(frames, pointsPerFrame);
    view.setTitle("AM Demo");
    view.setDelay(5);
}
 
void loop() {
    // In Frames mode, we fill an entire packet before sending
    for (int i = 0; i < pointsPerFrame; i++) {
        // Generate signal with amplitude modulation
        carrier = sin(i * theta * carrierFrequency);
        modulation = sin(i * theta * modulationFrequency);
        //create a random float from the long returned by random() to simulate noise
        noise = random(-1000, +1000)/1000.0; 
        signal = peakAmplitude * modulation * carrier + noise;
        // Add data to the traces
        view.addData("Signal", signal);
    }
    view.send(); //Send the frame
}

