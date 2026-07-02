/**
 * RapidFireNoDelay.ino
 * @date 03-10-26
 *
 * ViewPoint Extrema Test: Cartesian — Maximum Throughput Stress
 *
 * Sends data as fast as the microcontroller can push it over serial,
 * with zero delay between sends. Tests the app's serial buffer handling,
 * rendering pipeline, and whether it drops, garbles, or chokes on a
 * sustained data firehose. Starts with a single trace, then ramps up
 * to 4 traces at full speed. Periodically inserts a deliberate pause
 * to see if the app recovers cleanly from buffer saturation.
 *
 * Phases (10 seconds each):
 * - SINGLE_FIREHOSE:  1 trace, delay=0, full speed
 * - DUAL_FIREHOSE:    2 traces, delay=0
 * - QUAD_FIREHOSE:    4 traces, delay=0
 * - BURST_PAUSE:      100 rapid samples then 50ms pause, repeating
 * - RECOVERY:         Normal operation at delay=5 (sanity check)
 *
 * Mode: Continuous
 * Hardware: Any Arduino (faster MCU = more stress)
 *
 * Things to try:
 * - Monitor CPU usage in the desktop app during firehose phases
 * - Check if data remains coherent (sine shape preserved)
 * - Watch for rendering lag or frozen display
 * - See if the app drops data or garbles trace association
 */

#include <ViewPoint.h>
using namespace viewpoint;

// --- Phase definitions ---
enum Phase : uint8_t {
    SINGLE_FIREHOSE,
    DUAL_FIREHOSE,
    QUAD_FIREHOSE,
    BURST_PAUSE,
    RECOVERY,
    PHASE_COUNT
};

// --- Timing ---
const unsigned long PHASE_DURATION_MS = 10000;

// --- State ---
Phase currentPhase = SINGLE_FIREHOSE;
unsigned long phaseStartTime = 0;
float phase1 = 0.0, phase2 = 0.0, phase3 = 0.0, phase4 = 0.0;
unsigned long burstCount = 0;
unsigned long totalSamples = 0;

// --- Forward declarations ---
void enterPhase(Phase p);
int activeTraceCount();

void setup() {
    randomSeed(analogRead(0));

    view.begin(Mode::Continuous);
    view.setDelay(0);  // No delay — maximum speed
    view.setPlotTitle("Rapid Fire No Delay");
    view.setAxisLabels("Sample", "Value");

    view.trace("Fast1").setColor(colors::Crimson);
    view.trace("Fast2").setColor(colors::DodgerBlue);
    view.trace("Fast3").setColor(colors::LimeGreen);
    view.trace("Fast4").setColor(colors::Gold);

    enterPhase(SINGLE_FIREHOSE);
}

void loop() {
    unsigned long now = millis();

    // Phase transition
    if (now - phaseStartTime >= PHASE_DURATION_MS) {
        Phase next = static_cast<Phase>((currentPhase + 1) % PHASE_COUNT);
        enterPhase(next);
    }

    // Handle burst/pause pattern
    if (currentPhase == BURST_PAUSE) {
        burstCount++;
        if (burstCount > 100) {
            delay(50);
            burstCount = 0;
            return;
        }
    }

    // Generate data — keep frequencies different so we can distinguish traces
    phase1 += 0.1;
    if (phase1 > 2.0 * M_PI) phase1 -= 2.0 * M_PI;
    float v1 = sin(phase1) * 10.0;

    int count = activeTraceCount();

    view.addData("Fast1", v1);

    if (count >= 2) {
        phase2 += 0.07;
        if (phase2 > 2.0 * M_PI) phase2 -= 2.0 * M_PI;
        view.addData("Fast2", sin(phase2) * 8.0);
    }

    if (count >= 3) {
        phase3 += 0.13;
        if (phase3 > 2.0 * M_PI) phase3 -= 2.0 * M_PI;
        view.addData("Fast3", sin(phase3) * 6.0);
    }

    if (count >= 4) {
        phase4 += 0.03;
        if (phase4 > 2.0 * M_PI) phase4 -= 2.0 * M_PI;
        view.addData("Fast4", cos(phase4) * 12.0);
    }

    view.send();
    totalSamples++;
}

void enterPhase(Phase p) {
    currentPhase = p;
    phaseStartTime = millis();
    burstCount = 0;

    switch (p) {
        case SINGLE_FIREHOSE:
        case DUAL_FIREHOSE:
        case QUAD_FIREHOSE:
        case BURST_PAUSE:
            view.setDelay(0);
            break;

        case RECOVERY:
            view.setDelay(5);
            break;
    }

    const char* names[] = {"1-Trace Firehose", "2-Trace Firehose",
                           "4-Trace Firehose", "Burst/Pause", "Recovery"};
    view.sendInfo("Phase", names[p]);
}

int activeTraceCount() {
    switch (currentPhase) {
        case SINGLE_FIREHOSE: return 1;
        case DUAL_FIREHOSE:   return 2;
        case QUAD_FIREHOSE:
        case BURST_PAUSE:     return 4;
        case RECOVERY:        return 4;
        default:              return 1;
    }
}
