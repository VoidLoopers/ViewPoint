/**
 * SignalAnalyzer_A0_100kHz.ino
 * @version V1R0
 * @date 04-03-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * SignalAnalyzer_A0_100kHz.ino - Low-noise dual-domain signal analyzer on A0 (100 kHz BW)
 *
 * Board: Teensy 4.1
 *
 * Trades bandwidth for signal quality: samples at 200 kSPS with 4x hardware
 * averaging and moderate ADC speeds. This yields a 100 kHz Nyquist bandwidth
 * with significantly lower noise than the full-speed 800 kSPS variant, making
 * it better suited for audio-range and low-frequency measurements.
 *
 * Displays both domains simultaneously: a triggered oscilloscope on top and a
 * windowed FFT spectrum analyzer below.
 *
 * This demonstrates:
 * - Dual-plot layout: triggered time domain + FFT frequency domain
 * - 200 kSPS ADC sampling with 4x hardware averaging for low noise
 * - Hardware-triggered oscilloscope display with rising/falling edge detection
 * - 2048-point FFT with selectable windowing (Blackman-Harris default)
 * - 100 kHz bandwidth (Nyquist at 200 kSPS)
 * - DC removal before FFT to suppress the zero-frequency bin
 *
 * Uses the SignalCore DSP library: https://docs.voidloop.com/libraries/SignalCore/
 *
 * Verified hardware:
 * - Teensy 4.1
 *
 * Known issues:
 * - Other boards are not supported by this sketch in its current form.
 *
 * Board-specific requirements:
 * - Uses Pedvide's ADC library (install via the Library manager), AnalogBufferDMA, and Teensy 4.1 DMA-capable analog capture.
 * - Pin 23 is used for the right-encoder-button window-selection shortcut described below.
 *
 * Hardware:
 * - Connect the input signal to A0 (pin 14).
 * - Keep the analog input within the 0-3.3 V range.
 *
 * Sketch outline:
 * - Setup: Configure ADC with DMA double-buffering at 200 kHz, 4x averaging
 * - Setup: Two-plot layout (oscilloscope + spectrum)
 * - Loop: Wait for DMA buffer, convert to voltage, trigger, FFT, display
 * - Right encoder button (pin 23) cycles through FFT window functions
 *
 * Things to try:
 * - Feed a known sine wave (e.g., 10 kHz from a function generator) and observe
 *   the time domain waveform and its single spectral peak
 * - Compare the noise floor against the full-bandwidth SignalAnalyzer_A0 sketch
 * - Press the right encoder button to cycle through window types
 * - Change TRIGGER_MODE to TriggerMode::FallingEdge or TriggerMode::None
 * - Try different FFT_SIZE values (1024, 2048, 4096) to trade frequency
 *   resolution against update rate
 * - Short A0 to GND to see the ADC's own quantization noise spectrum
 */

#include <ViewPoint.h>
#include <ADC.h>
#include <AnalogBufferDMA.h>
#include <SignalCore.h>

using namespace viewpoint;
using namespace signalcore;

// Sampling — 200 kSPS with 4x averaging for low noise, 100 kHz Nyquist
constexpr float SAMPLE_RATE     = 200000.0f;
constexpr int   TARGET_FPS      = 50;

// FFT
// Frequency resolution = SAMPLE_RATE / FFT_SIZE ~ 98 Hz per bin at 2048
// Supported sizes: 32, 64, 128, 256, 512, 1024, 2048, 4096
constexpr int   FFT_SIZE        = 2048;
constexpr int   FFT_OUTPUT_SIZE = (FFT_SIZE / 2);  // DC through Nyquist
constexpr int   DISPLAY_SAMPLES = FFT_OUTPUT_SIZE;

// Trigger buffer: 2x FFT size gives the trigger enough headroom to search
constexpr int   TRIGGER_BUFFER  = FFT_SIZE * 2;

// DMA buffer: must be at least TRIGGER_BUFFER; sized for target frame rate
constexpr int   ADC_BUFFER_SIZE = (TRIGGER_BUFFER > (int)(SAMPLE_RATE / TARGET_FPS))
                                  ? TRIGGER_BUFFER
                                  : (int)(SAMPLE_RATE / TARGET_FPS);

// ADC hardware
constexpr int   ADC_PIN         = A0;           // Pin 14
constexpr int   ADC_BITS        = 10;
constexpr float VREF            = 3.3f;
constexpr int   ADC_MAX_VALUE   = (1 << ADC_BITS) - 1;

// Spectrum display — 0 dBFS referenced to ADC full-scale peak voltage
constexpr float DB_FLOOR        = -80.0f;
constexpr float FULL_SCALE_VP   = VREF / 2.0f;  // 0 dBFS = full-scale sine peak (1.65 V)

// Right encoder button — press to cycle FFT window functions
constexpr int   ENCODER_SW_PIN  = 23;

// Trigger mode — change here or cycle at runtime (see "Things to try")
constexpr TriggerMode TRIGGER_MODE = TriggerMode::RisingEdge;

// ADC + DMA
ADC *adc = new ADC();
DMAMEM static volatile uint16_t __attribute__((aligned(32))) dma_buf1[ADC_BUFFER_SIZE];
DMAMEM static volatile uint16_t __attribute__((aligned(32))) dma_buf2[ADC_BUFFER_SIZE];
AnalogBufferDMA abdma(dma_buf1, ADC_BUFFER_SIZE, dma_buf2, ADC_BUFFER_SIZE);

// DSP state
FFT<(FFTSize)FFT_SIZE> fft;
float samples[TRIGGER_BUFFER];
float time_display[DISPLAY_SAMPLES];

// Trigger
Trigger trigger(TRIGGER_MODE);

// Windowing reduces spectral leakage in FFTs by tapering signal edges.
// Each window trades frequency resolution against side-lobe suppression:
//   Rectangle     — Best resolution, worst leakage
//   Hamming/Hann  — Good general-purpose compromise
//   Blackman-Harris — Very low side lobes, slightly wider main lobe
//   Flat Top      — Best amplitude accuracy, poor frequency resolution
//   Triangle/Welch — Moderate suppression, simple shape
struct WindowEntry {
    WindowType type;
    const char* name;
};

constexpr WindowEntry windows[] = {
    { WindowType::BlackmanHarris, "Blackman-Harris" },
    { WindowType::Hann,            "Hann"            },
    { WindowType::Hamming,         "Hamming"         },
    { WindowType::FlatTop,         "Flat Top"        },
    { WindowType::Rectangle,       "Rectangle"       },
    { WindowType::Triangle,        "Triangle"        },
    { WindowType::Welch,           "Welch"           },
};
constexpr int NUM_WINDOWS = sizeof(windows) / sizeof(windows[0]);

int  current_window    = 0;
bool last_button_state = HIGH;

void setup() {
    // INPUT_DISABLE disconnects the digital input buffer, reducing noise
    // coupling from the digital domain into the analog measurement
    pinMode(ADC_PIN, INPUT_DISABLE);
    pinMode(ENCODER_SW_PIN, INPUT_PULLUP);

    // Configure ADC0 for low noise at 200 kSPS
    adc->adc0->setAveraging(4);
    adc->adc0->setResolution(ADC_BITS);
    adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED);
    adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::HIGH_SPEED);

    // Start DMA-driven sampling
    abdma.init(adc, ADC_0);
    adc->adc0->startSingleRead(ADC_PIN);
    adc->adc0->startTimer(SAMPLE_RATE);

    // ViewPoint configuration
    view.begin(Mode::Frames, DISPLAY_SAMPLES);
    view.setTitle("Signal Analyzer (100 kHz)");
    view.setDelay(0);

    view.setNumberOfPlots(2);

    // Plot 0: Oscilloscope (time domain)
    float time_span_us = DISPLAY_SAMPLES * (1.0f / SAMPLE_RATE) * 1e6f;
    float half_span    = time_span_us / 2.0f;
    view.plot(0).setTitle("Oscilloscope");
    view.plot(0).setVerticalRange(-0.5, 3.5, 8);
    view.plot(0).setHorizontalRange(-half_span, half_span, 10);
    view.plot(0).setAxisLabels("Time", "Voltage");
    view.plot(0).setUnits("us", "V");
    view.plot(0).addHorizontalReferenceLine(VREF / 2.0f, colors::DimGray);
    view.plot(0).addHorizontalReferenceLine(0.0f, colors::DimGray);

    // Plot 1: Spectrum Analyzer (frequency domain)
    view.plot(1).setTitle("Spectrum Analyzer");
    view.plot(1).setVerticalRange(DB_FLOOR, 10, 9);
    view.plot(1).setHorizontalRange(0, SAMPLE_RATE / 2000.0f, 10);
    view.plot(1).setAxisLabels("Frequency", "Power");
    view.plot(1).setUnits("kHz", "dBFS");

    // Trace styling
    view.trace("Signal").setColor(colors::OrangeRed);
    view.trace("Spectrum").setColor(colors::Cyan);

    // Initialize FFT windowing
    fft.setWindowType(windows[current_window].type);
}

void loop() {
    // Cycle through FFT window types on encoder button press (falling edge)
    bool button = digitalRead(ENCODER_SW_PIN);
    if (last_button_state == HIGH && button == LOW) {
        current_window = (current_window + 1) % NUM_WINDOWS;
        fft.setWindowType(windows[current_window].type);
    }
    last_button_state = button;

    // Wait for a complete DMA buffer
    if (!abdma.interrupted()) return;

    volatile uint16_t *buf = abdma.bufferLastISRFilled();

    // Teensy 4.x DMA writes to physical RAM (AXI), bypassing the CPU data cache.
    // Without invalidation the CPU would read stale cached values.
    if ((uint32_t)buf >= 0x20200000u)
        arm_dcache_delete((void *)buf, sizeof(dma_buf1));

    // Convert raw ADC values to voltage (only the portion we need for triggering)
    for (int i = 0; i < TRIGGER_BUFFER; i++)
        samples[i] = (float)buf[i] * VREF / ADC_MAX_VALUE;

    // Find the trigger point. The trigger centers the event in a window of
    // FFT_SIZE samples, so both the oscilloscope and FFT view the same signal.
    float *triggered = trigger.calculateOffset(samples, FFT_SIZE, TRIGGER_BUFFER);

    // Copy the first DISPLAY_SAMPLES from the triggered window
    memcpy(time_display, triggered, DISPLAY_SAMPLES * sizeof(float));

    // Remove DC offset before FFT so the zero-frequency bin doesn't dominate
    removeDC(triggered, FFT_SIZE);
    fft.calculate(triggered);
    fft.postScaleDB(DB_FLOOR, FULL_SCALE_VP);

    // Send to ViewPoint
    view.addData("Signal",   time_display,  DISPLAY_SAMPLES);
    view.addData("Spectrum", (float *)fft,  DISPLAY_SAMPLES);
    view.send();

    // Info bar: peak frequency, current window, trigger state, bin resolution
    float peak_freq_kHz;
    float peak_dB;
    findPeak((float *)fft, DISPLAY_SAMPLES, peak_freq_kHz, peak_dB);

    view.sendInfo("Peak=%.1f kHz (%.1f dB) | Win=%s | Res=%.0f Hz | TRG=%s",
                  peak_freq_kHz, peak_dB,
                  windows[current_window].name,
                  SAMPLE_RATE / FFT_SIZE,
                  toString(trigger.getTriggerMode()));

    abdma.clearInterrupt();
}

// Find the strongest spectral peak (skipping the DC bin)
void findPeak(float *spectrum, int size, float &freq_kHz, float &power_dB) {
    int   peak_bin = 1;
    float peak_val = spectrum[1];
    for (int i = 2; i < size; i++) {
        if (spectrum[i] > peak_val) {
            peak_val = spectrum[i];
            peak_bin = i;
        }
    }
    freq_kHz = (float)peak_bin * SAMPLE_RATE / FFT_SIZE / 1000.0f;
    power_dB = peak_val;
}
