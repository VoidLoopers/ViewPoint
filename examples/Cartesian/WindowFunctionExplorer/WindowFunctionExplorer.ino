/**
 * WindowFunctionExplorer.ino
 * @version 1.0.0
 * @date 03-10-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * WindowFunctionExplorer.ino - Compare FFT window functions side-by-side
 *
 * Two closely-spaced sine tones are processed through four different FFT windows
 * simultaneously. Each window occupies its own plot so you can directly compare
 * mainlobe width, sidelobe suppression, and amplitude accuracy. The second tone
 * sweeps slowly toward and away from the first, revealing the moment each window
 * can no longer resolve the two peaks -- the resolution/leakage tradeoff made visible.
 *
 * This demonstrates:
 * - Frames mode with four stacked plots
 * - Four independent FFT objects with different window functions
 * - DDS signal generation with two summed carriers plus noise
 * - Frequency sweeping to explore spectral resolution limits
 *
 * Verified hardware:
 * - Teensy 4.1
 *
 * Known issues:
 * - None recorded for this sketch
 *
 * Board-specific requirements:
 * - Requires SignalCore and enough RAM/flash for four simultaneous FFT pipelines.
 * - Only Teensy 4.1 has been hardware-verified for this sketch so far; the underlying DSP library has also been hardware-tested on Arduino Giga R1.
 *
 * This sketch uses the SignalCore DSP library: https://docs.voidloop.com/libraries/SignalCore/
 *
 * Hardware:
 * - No external hardware required.
 *
 * Sketch outline:
 * - Setup: configure 4 plots and 4 FFT objects with different windows
 * - Loop: generate samples from two tones + noise, compute all 4 FFTs, send results
 * - The second tone sweeps from SWEEP_LOW to SWEEP_HIGH Hz and back each cycle
 *
 * Things to try:
 * - Set NOISE_AMPLITUDE to 0 to see the pure window shapes without a noise floor
 * - Change TONE_1_FREQ to 2000 Hz and narrow the sweep range to 2100-2500 Hz
 * - Increase FFT_SIZE to 2048 for finer frequency resolution on all windows
 * - Try SWEEP_STEP of 5.0 to slow the sweep and study each frequency spacing
 * - Set TONE_2_AMPLITUDE to 0.3 to see how each window handles unequal amplitudes
 */

#include <ViewPoint.h>
#include <SignalCore.h>

using namespace viewpoint;
using namespace signalcore;

// ─── Configuration Constants ───
#define SAMPLE_RATE       10000
#define FFT_SIZE          1024
#define FFT_OUTPUT_SIZE   (FFT_SIZE / 2 + 1)

#define TONE_1_FREQ       1000.0
#define TONE_1_AMPLITUDE  1.0
#define TONE_2_AMPLITUDE  1.0
#define NOISE_AMPLITUDE   1.0

#define SWEEP_LOW         1100.0
#define SWEEP_HIGH        1500.0
#define SWEEP_STEP        10.0

#define DB_FLOOR          -100.0
#define FULL_SCALE        2.0

// ─── State Variables ───
float tone_2_freq = SWEEP_LOW;
float sweep_direction = 1.0;

SignalGenerator tone_1(SAMPLE_RATE);
SignalGenerator tone_2(SAMPLE_RATE);

FFT<(FFTSize)FFT_SIZE> fft_rect;
FFT<(FFTSize)FFT_SIZE> fft_hamming;
FFT<(FFTSize)FFT_SIZE> fft_blackman;
FFT<(FFTSize)FFT_SIZE> fft_flattop;

float samples[FFT_SIZE];

void setup() {
    view.begin(Mode::Frames, FFT_OUTPUT_SIZE);
    view.setTitle("Window Function Explorer");
    view.setDelay(30);
    view.setNumberOfPlots(4);

    // All plots share the same axis configuration
    float nyquist_khz = SAMPLE_RATE / 2000.0;
    for (int i = 0; i < 4; i++) {
        view.plot(i).setVerticalRange(DB_FLOOR, 10, 10);
        view.plot(i).setHorizontalRange(0, nyquist_khz, 10);
        // view.plot(i).setAxisLabels("Frequency", "Power");
        view.plot(i).setUnits("kHz", "dB");
    }

    view.plot(0).setTitle("Rectangle  (narrow mainlobe, worst sidelobes)");
    view.plot(1).setTitle("Hamming  (good sidelobe suppression, moderate width)");
    view.plot(2).setTitle("Blackman-Harris  (best sidelobe suppression, widest mainlobe)");
    view.plot(3).setTitle("Flat-Top  (best amplitude accuracy, wide mainlobe)");

    view.trace("Rectangle").setColor(colors::Cyan);
    view.trace("Hamming").setColor(colors::LimeGreen);
    view.trace("Blackman-Harris").setColor(colors::OrangeRed);
    view.trace("Flat-Top").setColor(colors::Gold);

    // Each FFT gets its own window -- the educational core of this sketch
    fft_rect.setWindowType(WindowType::Rectangle);
    fft_hamming.setWindowType(WindowType::Hamming);
    fft_blackman.setWindowType(WindowType::BlackmanHarris);
    fft_flattop.setWindowType(WindowType::FlatTop);

    tone_1.setCarrier(SignalShape::Sine, TONE_1_FREQ);
    tone_1.setNoise(NOISE_AMPLITUDE);
    tone_2.setCarrier(SignalShape::Sine, tone_2_freq);
}

void loop() {
    // Generate time-domain samples: sum of two tones plus noise from tone_1
    for (int i = 0; i < FFT_SIZE; i++) {
        samples[i] = TONE_1_AMPLITUDE * tone_1.nextSample(0)
                   + TONE_2_AMPLITUDE * tone_2.nextSample(0);
    }

    // Each FFT internally copies the source buffer before windowing,
    // so all four can safely process the same samples array.
    fft_rect.calculate(samples);
    fft_rect.postScaleDB(DB_FLOOR, FULL_SCALE);
    view.addData("Rectangle", (float*)fft_rect, FFT_OUTPUT_SIZE);

    fft_hamming.calculate(samples);
    fft_hamming.postScaleDB(DB_FLOOR, FULL_SCALE);
    view.addData("Hamming", (float*)fft_hamming, FFT_OUTPUT_SIZE);

    fft_blackman.calculate(samples);
    fft_blackman.postScaleDB(DB_FLOOR, FULL_SCALE);
    view.addData("Blackman-Harris", (float*)fft_blackman, FFT_OUTPUT_SIZE);

    fft_flattop.calculate(samples);
    fft_flattop.postScaleDB(DB_FLOOR, FULL_SCALE);
    view.addData("Flat-Top", (float*)fft_flattop, FFT_OUTPUT_SIZE);

    view.send();

    // Sweep the second tone back and forth to reveal resolution differences
    tone_2_freq += sweep_direction * SWEEP_STEP;
    if (tone_2_freq >= SWEEP_HIGH) {
        tone_2_freq = SWEEP_HIGH;
        sweep_direction = -1.0;
    } else if (tone_2_freq <= SWEEP_LOW) {
        tone_2_freq = SWEEP_LOW;
        sweep_direction = 1.0;
    }
    tone_2.setCarrier(SignalShape::Sine, tone_2_freq);
}
