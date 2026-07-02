/**
 * CartesianLogContinuous.ino
 * @date 03-09-26
 *
 * ViewPoint Log-Scale Test: Cartesian — Continuous Streaming with Log Vertical Axis
 *
 * Streams a signal through exponential decade sweeps on a logarithmic Y axis.
 * The signal generator produces values from 0.001 to 100,000 using a
 * state-machine that randomly selects sweep patterns: monotonic climb,
 * sawtooth decades, random jumps, and pulsed bursts at specific decades.
 *
 * This tests whether the log-scale renderer correctly:
 * - Maps linear data to logarithmic display space
 * - Renders decade gridlines at appropriate intervals
 * - Handles signals that dwell at a single decade then jump
 * - Displays both positive and near-zero values without clipping
 *
 * - Monotonic sweep: climbs from 0.001 to 100,000 over ~10 seconds
 * - Sawtooth decades: ramps up one decade, drops, ramps next
 * - Random jumps: hops between random decades every 0.5-2 seconds
 * - Pulsed bursts: steady baseline with sharp spikes to high decades
 *
 * Mode: Continuous
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch decade gridlines appear and disappear during sweeps
 * - Notice how the sawtooth pattern creates staircase-like log display
 * - Compare the visual spacing of decades (should be even on log axis)
 * - Observe the burst spikes: do they render at correct log positions?
 */

#include <ViewPoint.h>
using namespace viewpoint;

// Decade sweep parameters
const float DECADE_MIN = 0.001;
const float DECADE_MAX = 100000.0;
const int NUM_DECADES = 9;  // 0.001 to 100,000

// State machine
enum SweepMode {
    MONOTONIC_CLIMB,
    SAWTOOTH_DECADES,
    RANDOM_JUMPS,
    PULSED_BURSTS,
    NUM_SWEEP_MODES
};

SweepMode sweepMode = MONOTONIC_CLIMB;
unsigned long nextModeChange = 0;

// Signal state
float currentValue = 1.0;
float targetValue = 1.0;
float sweepPhase = 0.0;
float sweepRate = 0.0;

// Sawtooth state
int sawtoothDecade = 0;
float sawtoothPhase = 0.0;

// Burst state
float burstBaseline = 0.1;
float burstPeak = 10000.0;
bool bursting = false;
unsigned long burstEnd = 0;
unsigned long nextBurst = 0;

// Noise and modulation
float noiseLevel = 0.05;
float modPhase = 0.0;
float modFreq = 0.02;

void pickNewMode();

void setup() {
    randomSeed(analogRead(0));
    view.begin(Mode::Continuous);
    view.enableLogarithmicScale();
    view.setDelay(2);

    view.setPlotTitle("Log Continuous: Decade Sweeps");
    view.setAxisLabels("Sample", "Amplitude");
    view.setUnits("", "");

    view.trace("Signal").setColor(colors::DeepSkyBlue);
    view.trace("Envelope").setColor(colors::Gold);

    nextModeChange = millis() + 5000 + random(5000);
}

void loop() {
    unsigned long now = millis();

    if (now >= nextModeChange) {
        pickNewMode();
        nextModeChange = now + 4000 + random(8000);
    }

    float envelope = 0.0;

    switch (sweepMode) {
        case MONOTONIC_CLIMB: {
            sweepPhase += sweepRate;
            if (sweepPhase > 1.0) sweepPhase = 0.0;
            float logMin = log10(DECADE_MIN);
            float logMax = log10(DECADE_MAX);
            float logVal = logMin + sweepPhase * (logMax - logMin);
            currentValue = pow(10.0, logVal);
            envelope = currentValue;
            break;
        }

        case SAWTOOTH_DECADES: {
            sawtoothPhase += 0.005;
            if (sawtoothPhase >= 1.0) {
                sawtoothPhase = 0.0;
                sawtoothDecade = (sawtoothDecade + 1) % NUM_DECADES;
            }
            float decadeBase = pow(10.0, sawtoothDecade - 3);  // -3 to +5
            currentValue = decadeBase * (0.1 + sawtoothPhase * 9.0);
            envelope = decadeBase * 5.0;
            break;
        }

        case RANDOM_JUMPS: {
            float diff = targetValue - currentValue;
            currentValue += diff * 0.08;
            if (abs(diff) < targetValue * 0.01) {
                int decade = random(NUM_DECADES);
                targetValue = pow(10.0, decade - 3) * (0.5 + random(100) / 100.0);
            }
            envelope = targetValue;
            break;
        }

        case PULSED_BURSTS: {
            if (bursting) {
                currentValue = burstPeak * (0.8 + random(40) / 100.0);
                if (now >= burstEnd) bursting = false;
            } else {
                currentValue = burstBaseline * (0.8 + random(40) / 100.0);
                if (now >= nextBurst) {
                    bursting = true;
                    burstEnd = now + 50 + random(200);
                    nextBurst = now + 500 + random(2000);
                    burstPeak = pow(10.0, random(4) + 2);  // 100 to 100,000
                }
            }
            envelope = bursting ? burstPeak : burstBaseline;
            break;
        }
    }

    // Add proportional noise
    float noise = currentValue * noiseLevel * (random(2000) - 1000) / 1000.0;

    // Add slow modulation
    modPhase += modFreq;
    if (modPhase > 2.0 * M_PI) modPhase -= 2.0 * M_PI;
    float modulation = 1.0 + 0.3 * sin(modPhase);

    float output = constrain(currentValue * modulation + noise, 1e-6f, 1e6f);
    envelope = constrain(envelope, 1e-6f, 1e6f);

    view.addData("Signal", output);
    view.addData("Envelope", envelope);
    view.send();
}

void pickNewMode() {
    sweepMode = (SweepMode)random(NUM_SWEEP_MODES);

    switch (sweepMode) {
        case MONOTONIC_CLIMB:
            sweepPhase = 0.0;
            sweepRate = 0.0001 + random(10) / 100000.0;
            break;

        case SAWTOOTH_DECADES:
            sawtoothDecade = random(NUM_DECADES);
            sawtoothPhase = 0.0;
            break;

        case RANDOM_JUMPS: {
            int decade = random(NUM_DECADES);
            targetValue = pow(10.0, decade - 3);
            break;
        }

        case PULSED_BURSTS:
            burstBaseline = pow(10.0, random(3) - 2);  // 0.01 to 10
            bursting = false;
            nextBurst = millis() + random(500);
            break;
    }

    noiseLevel = 0.01 + random(10) / 100.0;
    modFreq = 0.005 + random(50) / 1000.0;
}
