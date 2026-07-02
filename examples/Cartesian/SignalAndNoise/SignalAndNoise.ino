/**
 * SignalAndNoise.ino
 * @version 1.0.0
 * @date 01-27-26
 * 
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * SignalAndNoise.ino - Time and frequency domain visualization
 *
 * Shows the same signal in two views:
 * - Top plot: Time-domain waveform (raw samples)
 * - Bottom plot: Frequency-domain spectrum (FFT)
 *
 * This helps understand the relationship between time and frequency
 * representations. Watch how noise in the time domain appears as
 * a raised noise floor in the frequency domain.
 *
 * The noise level slowly varies so you can see the effect on both views.
 *
 * This demonstrates:
 * - Multiple plots showing different views of the same data
 * - Time-domain vs frequency-domain visualization
 * - Dynamic parameter changes (noise level)
 *
 * Verified hardware:
 * - Teensy 4.1
 * - Arduino Giga R1
 *
 * Build-verified:
 * - ESP32 Dev Module
 * - Raspberry Pi Pico
 * - Arduino UNO Q
 * - STM32 Nucleo-F446RE
 *
 * Known issues:
 * - Arduino Uno / Nano class boards: insufficient RAM for this FFT configuration.
 *
 * Board-specific requirements:
 * - Requires SignalCore and enough RAM/flash for the selected FFT size and memory profile.
 * - Teensy 4.1 and Arduino Giga R1 are the hardware-verified boards for this sketch so far; the additional boards above are compile-verified only.
 *
 * Hardware:
 * - No external hardware required.
 *
 * Things to try:
 * - Change CARRIER_FREQ to move the peak in the spectrum
 * - Set noise to 0 to see a clean signal
 * - Try different waveforms (sine vs square) and compare harmonics
 *
 * This sketch uses the SignalCore DSP library: https://docs.voidloop.com/libraries/SignalCore/
 */

#include <ViewPoint.h>
#include <SignalCore.h>

using namespace viewpoint;
using namespace signalcore;

// Configuration
#define SAMPLE_RATE 100000
#define FFT_SIZE 1024
#define FFT_OUTPUT_SIZE (FFT_SIZE / 2 + 1)
#define TIME_DISPLAY_SAMPLES 200  // Show 200 samples in time domain
#define CARRIER_FREQ 10000        // 10 kHz carrier

// float CARRIER_FREQ = 1000.0;
// Signal parameters
float signal_amplitude = 1.0;
float noise_level = 0.0;         // Starts at 0, increases
float max_noise = 10.0;
float noise_step = 0.1;

// Signal generator and FFT
SignalGenerator sig_gen(SAMPLE_RATE);
FFT<(FFTSize)FFT_SIZE> fft;
float samples[FFT_SIZE];

void setup() {
    // Use Frames mode - we update both plots together
    view.begin(Mode::Frames, FFT_OUTPUT_SIZE);
    view.setDelay(20);
    view.setTitle("Signal and Noise");
    // Create 2 stacked plots
    view.setNumberOfPlots(2);

    // Plot 0: Time domain
    view.plot(0).setTitle("Time Domain");
    view.plot(0).setVerticalRange(-2.0, 2.0, 8);
    view.plot(0).setHorizontalRange(0, TIME_DISPLAY_SAMPLES, 10);
    view.plot(0).setAxisLabels("Sample", "Amplitude");
    view.plot(0).setUnits("n", "V");

    // Add zero reference line
    view.addHorizontalReferenceLine(0.0);

    // Plot 1: Frequency domain
    view.plot(1).setTitle("Frequency Domain");
    view.plot(1).setVerticalRange(-80, 10, 9);
    view.plot(1).setHorizontalRange(0, SAMPLE_RATE / 2000, 10);
    view.plot(1).setAxisLabels("Frequency", "Power");
    view.plot(1).setUnits("kHz", "dB");


    // Setup signal generator
    // sig_gen.setCarrier(SignalShape::Sine, CARRIER_FREQ);
    sig_gen.setCarrier(SignalShape::Square, CARRIER_FREQ);
    sig_gen.setNoise(noise_level);
    fft.setWindowType(WindowType::BlackmanHarris);
}

void loop() {
    // Update noise level in signal generator
    sig_gen.setNoise(noise_level);

    // Generate samples
    for (int i = 0; i < FFT_SIZE; i++) {
        samples[i] = signal_amplitude * sig_gen.nextSample(0);
    }

    // Send time domain samples (just the first N for display)
    view.addData("Signal", samples, FFT_OUTPUT_SIZE);

    // Compute FFT
    fft.calculate(samples);
    fft.postScaleDB(-100, signal_amplitude);

    // Send frequency domain
    view.addData("Spectrum", (float*)fft, FFT_OUTPUT_SIZE);

    view.send();

    // Slowly increase noise level, then reset
    noise_level += noise_step;
    if (noise_level > max_noise) {
        noise_level = 0;
    }
}
