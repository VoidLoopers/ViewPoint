/**
 * EmptyAndSparse.ino
 * @date 03-10-26
 *
 * ViewPoint Extrema Test: Cartesian — Empty, Sparse, and Asymmetric Data
 *
 * Tests how the app handles degenerate data patterns: frames with zero
 * samples, traces that rarely send data, asymmetric trace lengths (one
 * trace has 500 points, another has 3), and rapid creation of traces
 * that never receive data. These patterns can break assumptions about
 * data alignment, array sizing, and trace lifecycle in the renderer.
 *
 * Phases (7 seconds each):
 * - EMPTY_SENDS:     Calls send() with no addData() calls
 * - ONE_POINT:       Each trace sends exactly 1 sample per send()
 * - ASYMMETRIC:      Trace A sends 500 pts, Trace B sends 3 pts per frame
 * - SPARSE_BURSTS:   Sends nothing for 2s, then 50 rapid samples, repeat
 * - GHOST_TRACES:    Creates new named traces every loop but never sends data on them
 * - INTERLEAVED_NIL: Sends data on alternating traces each sample (A, _, A, _, ...)
 *
 * Mode: Mixed (Continuous for most, Frames for asymmetric)
 * Hardware: Any Arduino
 *
 * Things to try:
 * - Does the app crash or show errors during EMPTY_SENDS?
 * - Is the single-point visible in ONE_POINT?
 * - Does ASYMMETRIC cause trace misalignment?
 * - Do GHOST_TRACES accumulate in the legend indefinitely?
 */

#include <ViewPoint.h>
using namespace viewpoint;

// --- Phase definitions ---
enum Phase : uint8_t {
    EMPTY_SENDS,
    ONE_POINT,
    ASYMMETRIC,
    SPARSE_BURSTS,
    GHOST_TRACES,
    INTERLEAVED_NIL,
    PHASE_COUNT
};

// --- Timing ---
const unsigned long PHASE_DURATION_MS = 7000;

// --- State ---
Phase currentPhase = EMPTY_SENDS;
unsigned long phaseStartTime = 0;
unsigned long sampleCount = 0;
float sinePhase = 0.0;
int ghostIndex = 0;
bool inBurst = false;
unsigned long burstTimer = 0;
int burstSamplesSent = 0;

// --- Buffers for asymmetric test ---
const int BIG_SIZE = 500;
const int SMALL_SIZE = 3;
float bigBuffer[BIG_SIZE];
float smallBuffer[SMALL_SIZE];

// --- Forward declarations ---
void enterPhase(Phase p);

void setup() {
    randomSeed(analogRead(0));

    view.begin(Mode::Continuous);
    view.setDelay(2);
    view.setPlotTitle("Empty and Sparse Data Patterns");
    view.setAxisLabels("Sample", "Value");

    view.trace("Alpha").setColor(colors::Crimson);
    view.trace("Beta").setColor(colors::DodgerBlue);

    enterPhase(EMPTY_SENDS);
}

void loop() {
    unsigned long now = millis();

    // Phase transition
    if (now - phaseStartTime >= PHASE_DURATION_MS) {
        Phase next = static_cast<Phase>((currentPhase + 1) % PHASE_COUNT);
        enterPhase(next);
    }

    sinePhase += 0.05;
    if (sinePhase > 2.0 * M_PI) sinePhase -= 2.0 * M_PI;

    switch (currentPhase) {
        case EMPTY_SENDS: {
            // Send with no data added — completely empty
            view.send();
            break;
        }

        case ONE_POINT: {
            // Exactly one data point per trace per send
            view.addData("Alpha", sin(sinePhase) * 5.0);
            view.addData("Beta", cos(sinePhase) * 3.0);
            view.send();
            break;
        }

        case ASYMMETRIC: {
            // This test switches to frames briefly
            // Big trace gets 500 points, small trace gets 3
            for (int i = 0; i < BIG_SIZE; i++) {
                float t = i * 0.0125;
                bigBuffer[i] = sin(t + sinePhase) * 10.0;
            }
            for (int i = 0; i < SMALL_SIZE; i++) {
                smallBuffer[i] = cos(sinePhase + i) * 5.0;
            }

            view.addData("Alpha", bigBuffer, BIG_SIZE);
            view.addData("Beta", smallBuffer, SMALL_SIZE);
            view.send();
            break;
        }

        case SPARSE_BURSTS: {
            unsigned long elapsed = now - burstTimer;

            if (!inBurst) {
                // Silent period — send nothing
                if (elapsed >= 2000) {
                    inBurst = true;
                    burstTimer = now;
                    burstSamplesSent = 0;
                }
                // Still call send() during silence (empty sends)
                view.send();
            } else {
                // Burst of 50 rapid samples
                view.addData("Alpha", sin(sinePhase) * (5.0 + random(-20, 20) / 10.0));
                view.addData("Beta", cos(sinePhase) * 3.0);
                view.send();
                burstSamplesSent++;

                if (burstSamplesSent >= 50) {
                    inBurst = false;
                    burstTimer = now;
                }
            }
            break;
        }

        case GHOST_TRACES: {
            // Create new named traces every 10 loops but never send data on them
            if (sampleCount % 10 == 0 && ghostIndex < 14) {
                char name[16];
                snprintf(name, sizeof(name), "Ghost%d", ghostIndex);
                view.trace(name).setColor(colors::DarkGray);
                ghostIndex++;
            }

            // Only send on the original two traces
            view.addData("Alpha", sin(sinePhase) * 5.0);
            view.addData("Beta", cos(sinePhase) * 3.0);
            view.send();
            break;
        }

        case INTERLEAVED_NIL: {
            // Alternate which trace gets data each sample
            if (sampleCount % 2 == 0) {
                view.addData("Alpha", sin(sinePhase) * 5.0);
            } else {
                view.addData("Beta", cos(sinePhase) * 3.0);
            }
            view.send();
            break;
        }
    }

    sampleCount++;
}

void enterPhase(Phase p) {
    currentPhase = p;
    phaseStartTime = millis();
    sampleCount = 0;
    inBurst = false;
    burstTimer = millis();
    burstSamplesSent = 0;

    // Reset mode to continuous for most phases
    if (p == ASYMMETRIC) {
        view.setMode(Mode::Frames);
        view.setPacketSize(BIG_SIZE);
        view.setDelay(50);
    } else {
        view.setMode(Mode::Continuous);
        view.setDelay(2);
    }

    const char* names[] = {"Empty Sends", "One Point", "Asymmetric",
                           "Sparse Bursts", "Ghost Traces", "Interleaved Nil"};
    view.sendInfo("Phase", names[p]);
}
