/**
 * LiveLissajous_Teensy.ino
 * @version V1R2
 * @date 03-28-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * LiveLissajous_Teensy.ino - Real-time Lissajous figures from two ADC channels
 *
 * Board: Teensy 4.1
 * Coming soon: LiveLissajous_Giga, LiveLissajous_Pico
 *
 * Lissajous figures are the oldest trick in the oscilloscope playbook. Feed two
 * sinusoidal signals into X and Y channels: if they share the same frequency,
 * you see an ellipse whose tilt reveals the phase difference. At a 2:1 frequency
 * ratio you get a figure-eight, at 3:2 a pretzel. The shape tells you the
 * frequency relationship and phase offset at a glance — no math needed. This
 * sketch reads two real analog signals using dual-channel hardware-timed DMA
 * and plots them against each other for phase-coherent visualization.
 *
 * This demonstrates:
 * - Scatter plot with Continuous mode and Persistence display
 * - Dual-channel hardware-timed DMA using Pedvide's ADC library
 * - Real-time phase and frequency relationship visualization
 * - Symmetric axis configuration for undistorted figures
 *
 * Verified hardware:
 * - Teensy 4.1
 *
 * Known issues:
 * - Other boards are not supported by this sketch in its current form.
 *
 * Board-specific requirements:
 * - Uses Pedvide's ADC library, AnalogBufferDMA, and dual ADC capture on Teensy 4.1.
 * - The two analog channels must stay within the 0-3.3 V input range.
 *
 * Hardware:
 * - Connect the two input signals to A0 and A1.
 * - Use two generator channels, or one source plus a phase-shift network, if you want a known phase relationship.
 *
 * Sketch outline:
 * - Setup: Configure dual-channel DMA, Scatter, Frames, symmetric axes
 * - Loop: Wait for both DMA buffers, convert, subtract DC bias, plot as XY pairs
 *
 * Things to try:
 * - Enable Color Graded mode or persistence from the ViewPoint UI
 * - Same frequency, 0° phase: diagonal line. 90°: circle. 45°: tilted ellipse.
 * - Set one generator to 2x the other — see the figure-eight appear
 * - Slowly detune one frequency by 0.1 Hz — the figure rotates in real time
 * - Increase the amplitude of one channel — the ellipse stretches along that axis
 * - Touch one input with your finger — 60 Hz interference creates a rotating pattern
 */

#include <ViewPoint.h>
#include <ADC.h>
#include <AnalogBufferDMA.h>

using namespace viewpoint;

// ─── Configuration Constants ───
#define ADC_PIN_X       A0
#define ADC_PIN_Y       A1
#define ADC_BITS        10
#define VREF            3.3
#define ADC_COUNTS      ((1 << ADC_BITS) - 1)
#define DC_BIAS         (VREF / 2.0)    // Assumed mid-supply bias on both inputs
#define SAMPLE_RATE     800'000           // 50 kHz — deterministic via hardware PIT timer
#define FPS             50
#define DMA_BUF_SIZE    (int)(SAMPLE_RATE / FPS)             // ~1.25 ms per buffer at 800 kHz

// Display range centered on zero after DC removal
#define AXIS_RANGE      1.8             // ±1.8 V covers full 3.3 V swing centered

// ─── ADC + DMA Setup ───
// Both ADC modules run from independent PIT timers at the same frequency.
// Sub-microsecond inter-channel skew is negligible for audio-frequency Lissajous.
ADC *adc = new ADC();
DMAMEM static volatile uint16_t __attribute__((aligned(32))) dma_x1[DMA_BUF_SIZE];
DMAMEM static volatile uint16_t __attribute__((aligned(32))) dma_x2[DMA_BUF_SIZE];
DMAMEM static volatile uint16_t __attribute__((aligned(32))) dma_y1[DMA_BUF_SIZE];
DMAMEM static volatile uint16_t __attribute__((aligned(32))) dma_y2[DMA_BUF_SIZE];
AnalogBufferDMA abdma_x(dma_x1, DMA_BUF_SIZE, dma_x2, DMA_BUF_SIZE);
AnalogBufferDMA abdma_y(dma_y1, DMA_BUF_SIZE, dma_y2, DMA_BUF_SIZE);

void setup() {
    pinMode(ADC_PIN_X, INPUT_DISABLE);
    pinMode(ADC_PIN_Y, INPUT_DISABLE);

    adc->adc0->setAveraging(1);
    adc->adc0->setResolution(ADC_BITS);
    adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED);
    adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::HIGH_SPEED);
    adc->adc1->setAveraging(1);
    adc->adc1->setResolution(ADC_BITS);
    adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED);
    adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::HIGH_SPEED);

    abdma_x.init(adc, ADC_0);
    abdma_y.init(adc, ADC_1);

    adc->adc0->startSingleRead(ADC_PIN_X);
    adc->adc1->startSingleRead(ADC_PIN_Y);
    adc->adc0->startTimer(SAMPLE_RATE);
    adc->adc1->startTimer(SAMPLE_RATE);

    view.begin(PlotType::Scatter, Mode::Frames, DMA_BUF_SIZE);
    view.setDelay(0);
    
    view.setPlotTitle("Lissajous (A0 vs A1)");
    view.setAxisLabels("Channel A0", "Channel A1");
    view.setUnits("V", "V");
    view.setVerticalRange(-AXIS_RANGE, AXIS_RANGE);
    view.setHorizontalRange(-AXIS_RANGE, AXIS_RANGE);

    view.addHorizontalReferenceLine(0.0, colors::DimGray);
    view.addVerticalReferenceLine(0.0, colors::DimGray);

    view.trace("Lissajous").setColor(colors::SpringGreen);
}

void loop() {
    if (!abdma_x.interrupted() || !abdma_y.interrupted()) return;

    volatile uint16_t *buf_x = abdma_x.bufferLastISRFilled();
    volatile uint16_t *buf_y = abdma_y.bufferLastISRFilled();

    if ((uint32_t)buf_x >= 0x20200000u)
        arm_dcache_delete((void *)buf_x, sizeof(dma_x1));
    if ((uint32_t)buf_y >= 0x20200000u)
        arm_dcache_delete((void *)buf_y, sizeof(dma_y1));

    for (int i = 0; i < DMA_BUF_SIZE; i++) {
        float x = (float)buf_x[i] * VREF / ADC_COUNTS - DC_BIAS;
        float y = (float)buf_y[i] * VREF / ADC_COUNTS - DC_BIAS;

        view.addData("Lissajous", x, y);
    }

    view.send();


    abdma_x.clearInterrupt();
    abdma_y.clearInterrupt();
}
