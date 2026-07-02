/**
 * SignalAnalyzer_A0.ino
 * @version V1R2
 * @date 03-28-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * SignalAnalyzer_Teensy_A0.ino - Dual-domain signal analyzer: oscilloscope + spectrum on A0
 *
 * Board: Teensy 4.1
 * Coming soon: SignalAnalyzer_Giga, SignalAnalyzer_Pico
 *
 * Captures real analog signals from A0 at 800 kSPS via hardware-timed DMA and
 * can display the time domain, frequency domain, or both domains at once.
 * In dual-domain mode, it is the ViewPoint equivalent of a bench oscilloscope
 * with FFT math function — two synchronized views of the same signal, updated
 * in real time. The FFT still analyzes the full 400 kHz Nyquist bandwidth,
 * while the spectrum plot displays the lower-frequency subsection selected by
 * DISPLAY_BANDWIDTH_HZ. Spectral amplitudes are referenced to the ADC
 * full-scale voltage (VREF/2 peak = 0 dBFS).
 *
 * This demonstrates:
 * - Selectable time-domain, frequency-domain, or dual-domain display
 * - 800 kSPS ADC sampling via Pedvide's ADC library and AnalogBufferDMA
 * - Hardware-triggered oscilloscope display with rising/falling edge detection
 * - 4096-point FFT with selectable windowing (Blackman-Harris default)
 * - User-selectable displayed bandwidth, defaulting to 150 kHz
 * - DC removal before FFT to suppress the zero-frequency bin
 * - Beginner-friendly SHOW_TIME_DOMAIN and SHOW_FREQUENCY_DOMAIN toggles
 *
 * This sketch uses the SignalCore DSP library: https://docs.voidloop.com/libraries/SignalCore/
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
 * - Setup: Configure ADC with DMA double-buffering at 800 kHz
 * - Setup: One or two plot layout depending on selected domains
 * - Loop: Wait for DMA buffer, convert to voltage, trigger, analyze, display
 * - Right encoder button (pin 23) cycles through FFT window functions
 *
 * Things to try:
 * - Feed a known sine wave (e.g., 10 kHz from a function generator) and observe
 *   the time domain waveform and its single spectral peak
 * - Inject a square wave — see the fundamental + odd harmonics in the spectrum
 * - Press the right encoder button to cycle through window types — notice how
 *   Blackman-Harris gives narrow peaks while Flat Top gives accurate amplitudes
 * - Change TRIGGER_MODE to TriggerMode::FallingEdge or TriggerMode::None
 * - Set SHOW_TIME_DOMAIN to false for spectrum-only mode
 * - Set SHOW_FREQUENCY_DOMAIN to false for oscilloscope-only mode
 * - Try different FFT_SIZE values (1024, 2048, 4096) to trade frequency
 *   resolution against update rate
 * - Connect a noisy signal and observe the noise floor across frequency
 * - Short A0 to GND to see the ADC's own quantization noise spectrum
 */

#include <ViewPoint.h>
#include <ADC.h>
#include <AnalogBufferDMA.h>
#include <SignalCore.h>

using namespace viewpoint;
using namespace signalcore;

// Select what to display:
// - Time only:      SHOW_TIME_DOMAIN true,  SHOW_FREQUENCY_DOMAIN false
// - Frequency only: SHOW_TIME_DOMAIN false, SHOW_FREQUENCY_DOMAIN true
// - Both domains:   SHOW_TIME_DOMAIN true,  SHOW_FREQUENCY_DOMAIN true
#define SHOW_TIME_DOMAIN        true
#define SHOW_FREQUENCY_DOMAIN   true

static_assert(SHOW_TIME_DOMAIN || SHOW_FREQUENCY_DOMAIN,
              "Enable SHOW_TIME_DOMAIN, SHOW_FREQUENCY_DOMAIN, or both");

// Sampling
constexpr float SAMPLE_RATE     = 800000.0f;    // 800 kSPS — Nyquist at 400 kHz
constexpr int   TARGET_FPS      = 50;
constexpr int   ADC_BUFFER_SIZE = (int)(SAMPLE_RATE / TARGET_FPS);

// FFT
// Frequency resolution = SAMPLE_RATE / FFT_SIZE ~ 195 Hz per bin at 4096
// Supported sizes: 32, 64, 128, 256, 512, 1024, 2048, 4096
constexpr int   FFT_SIZE        = 4096;
constexpr int   FFT_OUTPUT_SIZE = (FFT_SIZE / 2);  // DC through Nyquist

// Spectrum display bandwidth. The FFT is still computed over FFT_SIZE samples,
// but only bins from DC up to this bandwidth are sent to ViewPoint.
constexpr float DISPLAY_BANDWIDTH_HZ = 150000.0f;
constexpr float NYQUIST_HZ           = SAMPLE_RATE / 2.0f;
constexpr int   SPECTRUM_DISPLAY_SAMPLES = (int)((DISPLAY_BANDWIDTH_HZ / NYQUIST_HZ) * FFT_OUTPUT_SIZE);
constexpr int   TIME_DISPLAY_SAMPLES     = SPECTRUM_DISPLAY_SAMPLES;
constexpr int   VIEW_PACKET_SIZE         = SHOW_TIME_DOMAIN ? TIME_DISPLAY_SAMPLES : SPECTRUM_DISPLAY_SAMPLES;

static_assert(DISPLAY_BANDWIDTH_HZ <= NYQUIST_HZ, "DISPLAY_BANDWIDTH_HZ cannot exceed Nyquist");
static_assert(SPECTRUM_DISPLAY_SAMPLES > 1, "DISPLAY_BANDWIDTH_HZ is too small for this FFT size");
static_assert(SPECTRUM_DISPLAY_SAMPLES <= FFT_OUTPUT_SIZE, "SPECTRUM_DISPLAY_SAMPLES cannot exceed FFT_OUTPUT_SIZE");

// Trigger buffer: 2x FFT size gives the trigger enough headroom to search
// while keeping RAM usage reasonable (~32 KB for this float buffer)
constexpr int   TRIGGER_BUFFER  = FFT_SIZE * 2;

// ADC hardware
constexpr int   ADC_PIN         = A0;           // Pin 14
constexpr int   ADC_BITS        = 10;
constexpr float VREF            = 3.3f;
constexpr int   ADC_MAX_VALUE      = (1 << ADC_BITS) - 1;

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
#if SHOW_FREQUENCY_DOMAIN
FFT<(FFTSize)FFT_SIZE> fft;
#endif
float samples[TRIGGER_BUFFER];
#if SHOW_TIME_DOMAIN
float time_display[TIME_DISPLAY_SAMPLES];
#endif

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

#if SHOW_FREQUENCY_DOMAIN
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
#endif

void setup() {
    // INPUT_DISABLE disconnects the digital input buffer, reducing noise
    // coupling from the digital domain into the analog measurement
    pinMode(ADC_PIN, INPUT_DISABLE);
#if SHOW_FREQUENCY_DOMAIN
    pinMode(ENCODER_SW_PIN, INPUT_PULLUP);
#endif

    // Configure ADC0 for maximum speed
    adc->adc0->setAveraging(1);
    adc->adc0->setResolution(ADC_BITS);
    adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_HIGH_SPEED);
    adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::HIGH_VERY_HIGH_SPEED);


    // Start DMA-driven sampling
    abdma.init(adc, ADC_0);
    adc->adc0->startSingleRead(ADC_PIN);
    adc->adc0->startTimer(SAMPLE_RATE);

    // ViewPoint configuration
    view.begin(Mode::Frames, VIEW_PACKET_SIZE);
    view.setTitle("Signal Analyzer");
    view.setDelay(0);

#if SHOW_TIME_DOMAIN && SHOW_FREQUENCY_DOMAIN
    view.setNumberOfPlots(2);
#else
    view.setNumberOfPlots(1);
#endif

#if SHOW_TIME_DOMAIN && SHOW_FREQUENCY_DOMAIN
    // Plot 0: Oscilloscope (time domain)
    float time_span_us = TIME_DISPLAY_SAMPLES * (1.0f / SAMPLE_RATE) * 1e6f;
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
    view.plot(1).setHorizontalRange(0, DISPLAY_BANDWIDTH_HZ / 1000.0f, 10);
    view.plot(1).setAxisLabels("Frequency", "Power");
    view.plot(1).setUnits("kHz", "dBV");
#elif SHOW_TIME_DOMAIN
    // Oscilloscope-only single plot
    float time_span_us = TIME_DISPLAY_SAMPLES * (1.0f / SAMPLE_RATE) * 1e6f;
    float half_span    = time_span_us / 2.0f;
    view.setPlotTitle("Oscilloscope");
    view.setVerticalRange(-0.5, 3.5, 8);
    view.setHorizontalRange(-half_span, half_span, 10);
    view.setAxisLabels("Time", "Voltage");
    view.setUnits("us", "V");
    view.addHorizontalReferenceLine(VREF / 2.0f, colors::DimGray);
    view.addHorizontalReferenceLine(0.0f, colors::DimGray);
#else
    // Spectrum-only single plot
    view.setPlotTitle("Spectrum Analyzer");
    view.setVerticalRange(DB_FLOOR, 10, 9);
    view.setHorizontalRange(0, DISPLAY_BANDWIDTH_HZ / 1000.0f, 10);
    view.setAxisLabels("Frequency", "Power");
    view.setUnits("kHz", "dBV");
#endif

    // Trace styling
#if SHOW_TIME_DOMAIN
    view.trace("Signal").setColor(colors::OrangeRed);
#endif
#if SHOW_FREQUENCY_DOMAIN
    view.trace("Spectrum").setColor(colors::Cyan);

    // Initialize FFT windowing
    fft.setWindowType(windows[current_window].type);
#endif
}

void loop() {
#if SHOW_FREQUENCY_DOMAIN
    // Cycle through FFT window types on encoder button press (falling edge)
    bool button = digitalRead(ENCODER_SW_PIN);
    if (last_button_state == HIGH && button == LOW) {
        current_window = (current_window + 1) % NUM_WINDOWS;
        fft.setWindowType(windows[current_window].type);
    }
    last_button_state = button;
#endif

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

#if SHOW_TIME_DOMAIN
    // Copy the first TIME_DISPLAY_SAMPLES from the triggered window
    memcpy(time_display, triggered, TIME_DISPLAY_SAMPLES * sizeof(float));
#endif

#if SHOW_FREQUENCY_DOMAIN
    // Remove DC offset before FFT so the zero-frequency bin doesn't dominate
    removeDC(triggered, FFT_SIZE);
    fft.calculate(triggered);
    fft.postScaleDB(DB_FLOOR, FULL_SCALE_VP);
#endif

    // Send to ViewPoint
#if SHOW_TIME_DOMAIN
    view.addData("Signal",   time_display,  TIME_DISPLAY_SAMPLES);
#endif
#if SHOW_FREQUENCY_DOMAIN
    view.addData("Spectrum", (float *)fft,  SPECTRUM_DISPLAY_SAMPLES);
#endif
    view.send();

#if SHOW_FREQUENCY_DOMAIN
    // Info bar: peak frequency, current window, trigger state, bin resolution
    float peak_freq_kHz;
    float peak_dB;
    findPeak((float *)fft, SPECTRUM_DISPLAY_SAMPLES, peak_freq_kHz, peak_dB);

    view.sendInfo("Peak=%.1f kHz (%.1f dB) | Win=%s | Res=%.0f Hz | TRG=%s",
                  peak_freq_kHz, peak_dB,
                  windows[current_window].name,
                  SAMPLE_RATE / FFT_SIZE,
                  toString(trigger.mode()));
#else
    view.sendInfo("Time domain | %.0f kSPS | TRG=%s",
                  SAMPLE_RATE / 1000.0f,
                  toString(trigger.mode()));
#endif

    abdma.clearInterrupt();
}

#if SHOW_FREQUENCY_DOMAIN
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
#endif
