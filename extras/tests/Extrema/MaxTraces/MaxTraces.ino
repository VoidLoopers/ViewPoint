/**
 * MaxTraces.ino
 * @date 03-10-26
 *
 * ViewPoint Extrema Test: Cartesian — Maximum Trace Saturation
 *
 * Creates N traces (controlled by VIEWPOINT_MAX_TRACES override) and
 * sends data on all of them simultaneously. Tests the app's ability to
 * render, legend, auto-scale, and color-differentiate a large trace set.
 * Each trace runs at a different frequency and amplitude so their ranges
 * don't perfectly overlap. Periodically, subsets of traces go silent
 * (send no data) or spike to test partial-update handling.
 *
 * To change the trace count, modify the VIEWPOINT_MAX_TRACES define below.
 * The library default is 16. Values above that test the app's limits.
 *
 * Phases (8 seconds each):
 * - ALL_ACTIVE:     All N traces generating sine waves at different frequencies
 * - HALF_SILENT:    Odd-numbered traces stop sending, even continue
 * - ALL_SPIKE:      All traces spike to random amplitudes simultaneously
 * - STAGGERED:      Traces activate one-by-one, evenly spaced across the phase
 * - AMPLITUDE_WAR:  Each trace competes for range dominance with growing amplitude
 *
 * Mode: Continuous
 * Hardware: Any Arduino
 *
 * Things to try:
 * - Check if all trace labels appear in the legend
 * - Watch auto-scale during HALF_SILENT—does it shrink to active traces?
 * - During ALL_SPIKE, does the scale expand fast enough?
 * - During AMPLITUDE_WAR, which trace "wins" the display?
 * - Try 16, 24, 32, 48—where does the app start struggling?
 */

// Override the library's default trace limit BEFORE including ViewPoint.h.
// The library uses #ifndef guards so this takes effect on all internal arrays.
#define VIEWPOINT_MAX_TRACES 24

#include <ViewPoint.h>
using namespace viewpoint;

// --- Constants ---
const int NUM_TRACES = VIEWPOINT_MAX_TRACES;
const unsigned long PHASE_DURATION_MS = 8000;

// --- Phase definitions ---
enum Phase : uint8_t {
    ALL_ACTIVE,
    HALF_SILENT,
    ALL_SPIKE,
    STAGGERED,
    AMPLITUDE_WAR,
    PHASE_COUNT
};

// --- Trace names (generated in setup) ---
char traceNames[NUM_TRACES][12];

// --- State ---
Phase currentPhase = ALL_ACTIVE;
unsigned long phaseStartTime = 0;
float phases[NUM_TRACES];
float frequencies[NUM_TRACES];
float amplitudes[NUM_TRACES];
float spikeTargets[NUM_TRACES];
float warAmplitudes[NUM_TRACES];
unsigned long sampleCount = 0;

// --- Forward declarations ---
void enterPhase(Phase p);
bool isTraceActive(int traceIdx);
float traceValue(int traceIdx);

void setup() {
    randomSeed(analogRead(0));

    view.begin(Mode::Continuous);
    view.setDelay(3);

    // Build title showing actual trace count
    char titleBuf[48];
    snprintf(titleBuf, sizeof(titleBuf), "Max Traces Stress Test (%d Traces)", NUM_TRACES);
    view.setPlotTitle(titleBuf);
    view.setAxisLabels("Sample", "Amplitude");

    // Color palette — wraps for any trace count
    const uint32_t traceColors[] = {
        colors::Red, colors::DodgerBlue, colors::LimeGreen, colors::Gold,
        colors::Crimson, colors::Cyan, colors::Magenta, colors::Orange,
        colors::SpringGreen, colors::HotPink, colors::SlateBlue, colors::Coral,
        colors::Aquamarine, colors::Orchid, colors::Chartreuse, colors::DeepSkyBlue,
        colors::Tomato, colors::MediumSeaGreen, colors::RoyalBlue, colors::Khaki,
        colors::Salmon, colors::Turquoise, colors::Plum, colors::PeachPuff
    };
    const int NUM_COLORS = sizeof(traceColors) / sizeof(traceColors[0]);

    for (int i = 0; i < NUM_TRACES; i++) {
        snprintf(traceNames[i], sizeof(traceNames[i]), "T%d", i);
        view.trace(traceNames[i]).setColor(traceColors[i % NUM_COLORS]);
        phases[i] = random(0, 628) / 100.0;  // Random starting phase
        frequencies[i] = 0.02 + (i * 0.015);  // Spread frequencies
        amplitudes[i] = 1.0 + i * 0.5;        // Spread amplitudes
    }

    enterPhase(ALL_ACTIVE);
}

void loop() {
    unsigned long now = millis();

    // Phase transition
    if (now - phaseStartTime >= PHASE_DURATION_MS) {
        Phase next = static_cast<Phase>((currentPhase + 1) % PHASE_COUNT);
        enterPhase(next);
    }

    // Send data for all active traces
    for (int i = 0; i < NUM_TRACES; i++) {
        if (isTraceActive(i)) {
            float val = traceValue(i);
            view.addData(traceNames[i], val);
        }
    }
    view.send();
    sampleCount++;
}

void enterPhase(Phase p) {
    currentPhase = p;
    phaseStartTime = millis();
    sampleCount = 0;

    switch (p) {
        case ALL_SPIKE:
            for (int i = 0; i < NUM_TRACES; i++) {
                spikeTargets[i] = (random(-100, 100) / 10.0) * pow(10.0, random(0, 4));
            }
            break;

        case AMPLITUDE_WAR:
            for (int i = 0; i < NUM_TRACES; i++) {
                warAmplitudes[i] = 1.0;
            }
            break;

        default:
            break;
    }

    const char* names[] = {"All Active", "Half Silent", "All Spike",
                           "Staggered", "Amplitude War"};
    view.sendInfo("Phase", names[p]);
}

bool isTraceActive(int idx) {
    switch (currentPhase) {
        case HALF_SILENT:
            return (idx % 2 == 0);

        case STAGGERED: {
            unsigned long elapsed = millis() - phaseStartTime;
            // Space activations evenly across 80% of the phase duration
            unsigned long spacing = (PHASE_DURATION_MS * 8UL / 10UL) / max(NUM_TRACES, 1);
            unsigned long activationTime = idx * spacing;
            return elapsed >= activationTime;
        }

        default:
            return true;
    }
}

float traceValue(int idx) {
    phases[idx] += frequencies[idx];
    if (phases[idx] > 2.0 * M_PI) phases[idx] -= 2.0 * M_PI;

    switch (currentPhase) {
        case ALL_ACTIVE:
        case HALF_SILENT:
        case STAGGERED:
            return sin(phases[idx]) * amplitudes[idx];

        case ALL_SPIKE: {
            // Spike then decay
            float t = (millis() - phaseStartTime) / 1000.0;
            float envelope = exp(-t * 0.3) * spikeTargets[idx];
            return envelope + sin(phases[idx]) * amplitudes[idx] * 0.1;
        }

        case AMPLITUDE_WAR: {
            // Each trace grows at a different rate
            float growthRate = 1.0 + (idx * 0.1);
            warAmplitudes[idx] *= (1.0 + growthRate * 0.001);
            float capped = constrain(warAmplitudes[idx], 0.0f, 1e5f);
            warAmplitudes[idx] = capped;
            return sin(phases[idx]) * capped;
        }

        default:
            return 0.0;
    }
}
