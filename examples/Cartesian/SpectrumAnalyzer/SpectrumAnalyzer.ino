/**
 * SpectrumAnalyzer.ino
 * @version 1.0.0
 * @date 01-27-26
 * 
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * Find ViewPoint library documentation at https://docs.voidloop.com/viewpoint/
 * SpectrumAnalyzer.ino - Real-time FFT spectrum analyzer
 *
 * Generates a square wave signal with noise, computes its FFT, and displays
 * the frequency spectrum. The carrier frequency sweeps slowly, so you can
 * watch the peak move across the display.
 *
 * This demonstrates:
 * - Frames mode for FFT visualization
 * - Proper axis configuration for frequency/dB display
 * - Using the DSP library for signal generation and FFT
 *
 * This sketch uses the SignalCore DSP library: https://docs.voidloop.com/libraries/SignalCore/
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
 * - Change the waveform from square to sine (SignalShape::Sine in setup_signals)
 * - Adjust the noise level (NOISE_AMPLITUDE)
 * - Try different FFT window types (WindowType::Hann, WindowType::Blackman, etc.)
 * - Change the frequency sweep range and speed
 * - Change the size of the FFT (up to 4096)
 * 
 * Try from within the App
 * - Open the properties panel and find the `Display Modes` section (bottom)
 * - Enable each display mode one at a time, experiment with different settings
 * - Display Modes include: Persistence, Spectrogram, and Color Gradient
 * - Note: Display Modes can be enabled from the library as well. See setup for an example (commented out)
 * -    If a display mode is enabled from the library, it can be overridden, changed, or modified in-app.
 */

#include <ViewPoint.h>
#include <SignalCore.h>

using namespace viewpoint;
using namespace signalcore;

// FFT Configuration
#define SAMPLE_RATE 250000      // Samples per second
#define FFT_SIZE 1024           // FFT size (must be power of 2)
#define FFT_OUTPUT_SIZE (FFT_SIZE / 2 + 1)

// Signal Configuration
#define CARRIER_START 5000      // Starting frequency (Hz)
#define CARRIER_END 50000       // Ending frequency (Hz)
#define CARRIER_STEP 500        // Frequency step per frame (Hz)
#define NOISE_AMPLITUDE 3.0     // Noise level (% of full scale)

// Signal generator and FFT
SignalGenerator sig_gen(SAMPLE_RATE);
FFT<FFTSize::N1024> fft;
float samples[FFT_SIZE];

// Signal parameters
float signal_amplitude = 1.65;  // Half of 3.3V full scale
float full_scale = 1.65;
uint32_t carrier_frequency = CARRIER_START;

void setup() {
    // Initialize ViewPoint in Frames mode
    // Packet size matches FFT output (N/2 + 1 bins)
    view.begin(Mode::Frames, FFT_OUTPUT_SIZE);
    view.setTitle("Spectrum Analyzer");
    view.setNumberOfPlots(1);
    view.setDelay(40);
    // Configure the plot for spectrum analysis
    view.setPlotTitle("Spectrum Analyzer");

    view.setHorizontalRange(0, SAMPLE_RATE / 2000, 10);  // 0 to Nyquist in kHz
    view.setVerticalRange(-60, 10, 7);                   // dB scale
    view.setAxisLabels("Frequency", "Power");
    view.setUnits("kHz", "dB");

    // Enable spectrogram display mode for trace 0
    // view.plot(0).setDisplayMode(DisplayMode::Spectrogram, 0);

    // Setup signal generator
    setup_signals();

}

void loop() {
    // Generate samples and compute FFT
    generate_signals();

    // Send FFT result to ViewPoint
    // The FFT class has an operator float* that returns the result array
    view.addData("Spectrum", (float*)fft, FFT_OUTPUT_SIZE);
    view.send();

    // Sweep the carrier frequency
    carrier_frequency += CARRIER_STEP;
    if (carrier_frequency > CARRIER_END) {
        carrier_frequency = CARRIER_START;
    }

    // Update signal generator with new frequency
    sig_gen.setCarrier(SignalShape::Square, carrier_frequency);
}

void setup_signals() {
    // Configure square wave carrier
    sig_gen.setCarrier(SignalShape::Square, carrier_frequency);

    // Add some amplitude modulation for visual interest.
    // setAM's amplitude is a PERCENT depth (0..100), unlike setCarrier's
    // linear gain — 1 here means 1% modulation depth.
    sig_gen.setAM(SignalShape::Sine, 2, 1);  // 2 Hz AM at 1% depth

    // Add noise to make it realistic
    sig_gen.setNoise(NOISE_AMPLITUDE);

    // Use Blackman-Harris window for good frequency resolution
    fft.setWindowType(WindowType::BlackmanHarris);
}

void generate_signals() {
    // Generate time-domain samples
    for (int i = 0; i < FFT_SIZE; i++) {
        samples[i] = signal_amplitude * sig_gen.nextSample(0);
    }

    // Compute FFT
    fft.calculate(samples);

    // Scale to dB with -120 dB floor
    fft.postScaleDB(-120, full_scale);
}
