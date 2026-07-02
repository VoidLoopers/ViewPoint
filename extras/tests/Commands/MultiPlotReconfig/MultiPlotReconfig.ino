/**
 * MultiPlotReconfig.ino
 * @date 03-10-26
 *
 * ViewPoint Command Test: Dynamic Multi-Plot Reconfiguration
 *
 * Starts with 1 plot, then adds plots incrementally up to 4, then
 * removes them back down to 1. Each time a plot is added or removed,
 * the layout changes and traces are reassigned. This tests the app's
 * ability to handle a changing number of plots, dynamic trace-to-plot
 * assignment, and per-plot configuration updates during live streaming.
 *
 * Also tests per-plot config changes: each plot gets different titles,
 * ranges, grid colors, and display modes. When plots are removed, the
 * remaining plots should absorb all active traces gracefully.
 *
 * Sequence (5 seconds each step):
 * 1. Single plot, 2 traces
 * 2. Two plots, 1 trace each with different config
 * 3. Three plots, traces distributed with display modes
 * 4. Four plots, maximum layout stress
 * 5. Back to two plots (tests reduction)
 * 6. Back to one plot (all traces reunited)
 * 7. Restart cycle
 *
 * Mode: Continuous
 * Hardware: Any Arduino
 *
 * Things to try:
 * - Does the layout resize smoothly when plots are added?
 * - Do per-plot titles and ranges appear correctly?
 * - Is there data loss during layout transitions?
 * - Does going from 4 plots back to 1 work cleanly?
 */

#include <ViewPoint.h>
using namespace viewpoint;

// --- Timing ---
const unsigned long STEP_DURATION_MS = 5000;

// --- Layout steps ---
enum LayoutStep : uint8_t {
    LAYOUT_1_PLOT,
    LAYOUT_2_PLOTS,
    LAYOUT_3_PLOTS,
    LAYOUT_4_PLOTS,
    LAYOUT_REDUCE_2,
    LAYOUT_REDUCE_1,
    LAYOUT_COUNT
};

// --- State ---
LayoutStep currentLayout = LAYOUT_1_PLOT;
unsigned long stepStartTime = 0;
float phase1 = 0.0, phase2 = 0.0, phase3 = 0.0, phase4 = 0.0;

// --- Forward declarations ---
void applyLayout(LayoutStep layout);
int numPlotsForLayout(LayoutStep layout);

void setup() {
    randomSeed(analogRead(0));

    view.begin(Mode::Continuous);
    view.setDelay(3);

    view.trace("Signal").setColor(colors::Coral);
    view.trace("Filter").setColor(colors::DodgerBlue);
    view.trace("Noise").setColor(colors::LimeGreen);
    view.trace("RMS").setColor(colors::Gold);

    applyLayout(LAYOUT_1_PLOT);
    stepStartTime = millis();
}

void loop() {
    unsigned long now = millis();

    // Step transition
    if (now - stepStartTime >= STEP_DURATION_MS) {
        currentLayout = static_cast<LayoutStep>((currentLayout + 1) % LAYOUT_COUNT);
        applyLayout(currentLayout);
        stepStartTime = now;
    }

    // Generate 4 distinct signals
    phase1 += 0.04;
    phase2 += 0.06;
    phase3 += 0.09;
    phase4 += 0.02;
    if (phase1 > 2.0 * M_PI) phase1 -= 2.0 * M_PI;
    if (phase2 > 2.0 * M_PI) phase2 -= 2.0 * M_PI;
    if (phase3 > 2.0 * M_PI) phase3 -= 2.0 * M_PI;
    if (phase4 > 2.0 * M_PI) phase4 -= 2.0 * M_PI;

    float signal = sin(phase1) * 5.0;
    float filtered = sin(phase2) * 3.0;
    float noise = signal + (random(-200, 200) / 100.0);
    float rms = abs(sin(phase4)) * 4.0;

    // Always send on all 4 traces regardless of plot count
    view.addData("Signal", signal);
    view.addData("Filter", filtered);
    view.addData("Noise", noise);
    view.addData("RMS", rms);
    view.send();
}

void applyLayout(LayoutStep layout) {
    int nPlots = numPlotsForLayout(layout);
    view.setNumberOfPlots(nPlots);

    switch (layout) {
        case LAYOUT_1_PLOT:
            view.plot(0).setTitle("All Traces — Single Plot");
            view.plot(0).setAxisLabels("Time", "Value");
            view.plot(0).setUnits("ms", "V");
            view.sendInfo("Layout", "1 plot");
            break;

        case LAYOUT_2_PLOTS:
            view.plot(0).setTitle("Raw Signals");
            view.plot(0).setAxisLabels("Time", "Raw");
            view.plot(0).setVerticalRange(-8, 8);
            view.plot(1).setTitle("Derived");
            view.plot(1).setAxisLabels("Time", "Processed");
            view.plot(1).setGridColors(0x002200, 0x004400);
            view.sendInfo("Layout", "2 plots");
            break;

        case LAYOUT_3_PLOTS:
            view.plot(0).setTitle("Signal");
            view.plot(0).setVerticalRange(-6, 6, 4);
            view.plot(1).setTitle("Noise");
            view.plot(1).setGridColors(0x220000, 0x440000);
            view.plot(2).setTitle("RMS Level");
            view.plot(2).setVerticalRange(0, 5, 5);
            view.plot(2).addHorizontalReferenceLine(3.0, colors::Red, 1.5);
            view.sendInfo("Layout", "3 plots");
            break;

        case LAYOUT_4_PLOTS:
            view.plot(0).setTitle("Signal");
            view.plot(0).setVerticalRange(-6, 6);
            view.plot(0).setGridColors(0x001122, 0x002244);
            view.plot(1).setTitle("Filtered");
            view.plot(1).setVerticalRange(-4, 4);
            view.plot(1).setGridColors(0x112200, 0x224400);
            view.plot(2).setTitle("Noise");
            view.plot(2).setGridColors(0x220011, 0x440022);
            view.plot(3).setTitle("RMS");
            view.plot(3).setVerticalRange(0, 5);
            view.plot(3).addHorizontalReferenceLine(2.5, colors::Gold, 2.0);
            view.sendInfo("Layout", "4 plots");
            break;

        case LAYOUT_REDUCE_2:
            view.plot(0).setTitle("Combined A");
            view.plot(1).setTitle("Combined B");
            view.sendInfo("Layout", "Reduced to 2");
            break;

        case LAYOUT_REDUCE_1:
            view.plot(0).setTitle("Everything — Back to 1");
            view.sendInfo("Layout", "Reduced to 1");
            break;
    }
}

int numPlotsForLayout(LayoutStep layout) {
    switch (layout) {
        case LAYOUT_1_PLOT:   return 1;
        case LAYOUT_2_PLOTS:  return 2;
        case LAYOUT_3_PLOTS:  return 3;
        case LAYOUT_4_PLOTS:  return 4;
        case LAYOUT_REDUCE_2: return 2;
        case LAYOUT_REDUCE_1: return 1;
        default:              return 1;
    }
}
