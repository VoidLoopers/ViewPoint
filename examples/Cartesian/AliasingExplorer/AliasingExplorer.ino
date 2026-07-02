/**
 * AliasingExplorer.ino
 * @version 1.0.0
 * @date 03-10-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * AliasingExplorer.ino - Nyquist sampling theorem and aliasing visualization
 *
 * The Nyquist-Shannon sampling theorem states that a signal must be sampled at
 * more than twice its highest frequency to be faithfully reconstructed. When
 * this rule is violated, the signal "aliases" -- it appears as a phantom lower
 * frequency that never existed in the original. This sketch sweeps a sine wave
 * through and beyond the Nyquist limit so you can watch the alias emerge.
 *
 * This demonstrates:
 * - Frames mode with dual-plot comparison
 * - Zero-order hold reconstruction showing sampling artifacts
 * - Frequency aliasing when signal exceeds Nyquist rate
 * - sendInfo for real-time status display
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
 * Sketch outline:
 * - Setup: Two plots -- true analog signal and sampled/reconstructed version
 * - Loop: Generate both versions, sweep frequency through Nyquist boundary
 * - The sampled plot uses zero-order hold (staircase) reconstruction
 *
 * Things to try:
 * - Watch carefully as frequency crosses 100 Hz (Nyquist) -- the sampled waveform reverses
 * - Increase ADC_SAMPLE_RATE to 400 -- Nyquist moves to 200 Hz, aliasing occurs later
 * - Set SWEEP_STEP to 0.5 for slow-motion crossing of the Nyquist boundary
 * - Change DISPLAY_WINDOW to 0.2 for a wider time view
 * - Notice at exactly Nyquist (100 Hz), the sampled signal is ambiguous
 */

#include <ViewPoint.h>
using namespace viewpoint;

// ─── Configuration Constants ───
#define DISPLAY_POINTS      500         // Points per frame for smooth rendering
#define ADC_SAMPLE_RATE     200         // Hz -- the "slow" ADC we're simulating
#define NYQUIST             (ADC_SAMPLE_RATE / 2)   // 100 Hz
#define DISPLAY_WINDOW      0.1         // Seconds of signal to show per frame
#define FREQ_MIN            20.0        // Hz -- sweep start (well below Nyquist)
#define FREQ_MAX            380.0       // Hz -- sweep end (sweeps through Nyquist twice)
#define SWEEP_STEP          2.0         // Hz per frame

// ─── State Variables ───
float signal_freq = FREQ_MIN;
float true_samples[DISPLAY_POINTS];
float sampled_samples[DISPLAY_POINTS];

void setup() {
    view.begin(Mode::Frames, DISPLAY_POINTS);
    view.setTitle("Aliasing Explorer");
    view.setDelay(50);
    view.setNumberOfPlots(2);

    // Plot 0: the continuous analog signal at full resolution
    view.plot(0).setTitle("True Signal (Analog)");
    view.plot(0).setVerticalRange(-1.5, 1.5, 6);
    view.plot(0).setHorizontalRange(0, DISPLAY_WINDOW * 1000, 10);
    view.plot(0).setAxisLabels("Time", "Amplitude");
    view.plot(0).setUnits("ms", "V");

    // Plot 1: what the ADC actually captures, reconstructed with zero-order hold
    view.plot(1).setTitle("Sampled & Reconstructed (ZOH)");
    view.plot(1).setVerticalRange(-1.5, 1.5, 6);
    view.plot(1).setHorizontalRange(0, DISPLAY_WINDOW * 1000, 10);
    view.plot(1).setAxisLabels("Time", "Amplitude");
    view.plot(1).setUnits("ms", "V");

    // Zero reference on both plots
    view.addHorizontalReferenceLine(0.0);

    view.trace("Analog").setColor(colors::Cyan);
    view.trace("Digital").setColor(colors::OrangeRed);
}

void loop() {
    for (int i = 0; i < DISPLAY_POINTS; i++) {
        float t = (float)i * DISPLAY_WINDOW / DISPLAY_POINTS;

        // True signal sampled at display resolution (effectively infinite)
        true_samples[i] = sin(2.0 * M_PI * signal_freq * t);

        // Zero-order hold: snap each time to the nearest ADC sample instant
        float sample_t = floor(t * ADC_SAMPLE_RATE) / ADC_SAMPLE_RATE;
        sampled_samples[i] = sin(2.0 * M_PI * signal_freq * sample_t);
    }

    view.addData("Analog", true_samples, DISPLAY_POINTS);
    view.addData("Digital", sampled_samples, DISPLAY_POINTS);
    view.send();

    view.sendInfo("f=%d Hz | Nyquist=%d Hz | %s",
                  (int)signal_freq, NYQUIST,
                  signal_freq > NYQUIST ? "ALIASED" : "OK");

    // Sweep frequency upward, wrap around at the top
    signal_freq += SWEEP_STEP;
    if (signal_freq > FREQ_MAX) signal_freq = FREQ_MIN;
}
