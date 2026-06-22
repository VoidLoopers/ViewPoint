/**
 * CartesianLogSpan.ino
 * @date 03-08-26
 *
 * ViewPoint Auto-Scale Test: Cartesian — Signal Spanning Orders of Magnitude
 *
 * Generates a signal that sweeps through 7 orders of magnitude, from
 * ±0.001 to ±10,000. This is the nightmare scenario for linear auto-
 * scaling: when the signal is at 10,000 the ±0.001 phase is invisible,
 * and when the signal is at 0.001 the scale from the 10,000 phase
 * wastes 99.99999% of the axis.
 *
 * The signal moves through amplitude decades in a non-monotonic
 * sequence with random timing. Transitions can be:
 * - Gradual: smooth exponential ramp between decades
 * - Instant: jump 4 decades in a single sample
 * - Oscillating: amplitude "breathes" between two decades
 *
 * This tests whether the auto-scaler can:
 * - Track a signal that varies over >7 decades
 * - Respond quickly to sudden decade jumps
 * - Not thrash when the signal oscillates between decades
 * - Eventually contract after being expanded by a large signal
 *
 * Mode: Continuous
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch the signal disappear when the scale is large
 * - Try enabling logarithmic Y scale if your strategy supports it
 * - Compare PEAK_DECAY vs WINDOWED: which recovers faster?
 */

#include <ViewPoint.h>
using namespace viewpoint;

// Decade levels (center amplitudes)
const float DECADES[] = {0.001, 0.01, 0.1, 1.0, 10.0, 100.0, 1000.0, 10000.0};
const int NUM_DECADES = 8;

// Current state
float amplitude = 1.0;
float targetAmplitude = 1.0;
int currentDecade = 3;    // Index into DECADES[]
float wavePhase = 0.0;
float waveFreq = 0.1;

// Transition mode
enum TransitionMode {
    GRADUAL,
    INSTANT,
    OSCILLATING,
    STEPPED,
    NUM_TRANSITIONS
};

TransitionMode transMode = GRADUAL;
float rampRate = 0.005;

// Oscillation state
int oscDecadeLow = 0;
int oscDecadeHigh = 0;
float oscPhase = 0.0;
float oscFreq = 0.001;

// Stepped ramp
float steppedTarget = 1.0;
unsigned long nextSteppedJump = 0;

// Scheduling
unsigned long nextModeChange = 0;
unsigned long sampleCount = 0;

void pickNewMode();

void setup() {
    randomSeed(analogRead(0));
    view.begin(Mode::Continuous);
    view.enableLogarithmicScale();
    view.setDelay(2);

    view.setPlotTitle("Log Span Test");
    view.setAxisLabels("Sample", "Value");

    view.trace("Signal").setColor(colors::Aquamarine);
    view.addHorizontalReferenceLine(0.0);

    nextModeChange = millis() + 4000 + random(4000);
}

void loop() {
    unsigned long now = millis();

    // Mode change
    if (now >= nextModeChange) {
        pickNewMode();
        nextModeChange = now + 3000 + random(8000);
    }

    // Update amplitude based on transition mode
    switch (transMode) {
        case GRADUAL:
            // Exponential approach to target
            if (amplitude < targetAmplitude) {
                amplitude *= (1.0 + rampRate);
                if (amplitude > targetAmplitude) amplitude = targetAmplitude;
            } else {
                amplitude *= (1.0 - rampRate);
                if (amplitude < targetAmplitude) amplitude = targetAmplitude;
            }
            break;

        case INSTANT:
            amplitude = targetAmplitude;
            break;

        case OSCILLATING: {
            oscPhase += oscFreq;
            float mix = 0.5 + 0.5 * sin(oscPhase);
            float logLow = log10(DECADES[oscDecadeLow]);
            float logHigh = log10(DECADES[oscDecadeHigh]);
            float logAmp = logLow + mix * (logHigh - logLow);
            amplitude = pow(10.0, logAmp);
            break;
        }

        case STEPPED:
            if (now >= nextSteppedJump) {
                // Random decade step
                int step = random(NUM_DECADES);
                steppedTarget = DECADES[step] * (0.5 + random(100) / 100.0);
                amplitude = steppedTarget;
                nextSteppedJump = now + 500 + random(2000);
            }
            break;
    }

    // Safety clamp
    amplitude = constrain(amplitude, 1e-6f, 1e6f);

    // Generate waveform at current amplitude
    wavePhase += waveFreq;
    if (wavePhase > 2.0 * M_PI) wavePhase -= 2.0 * M_PI;

    float value = amplitude * sin(wavePhase);

    // Add noise proportional to amplitude (maintains SNR across decades)
    value += amplitude * 0.03 * (random(2000) - 1000) / 1000.0;

    view.addData("Signal", constrain(value, -1e6f, 1e6f));
    view.send();
    sampleCount++;
}

void pickNewMode() {
    transMode = (TransitionMode)random(NUM_TRANSITIONS);

    switch (transMode) {
        case GRADUAL: {
            // Pick a random target decade
            int target = random(NUM_DECADES);
            targetAmplitude = DECADES[target] * (0.5 + random(150) / 100.0);
            rampRate = 0.001 + random(50) / 10000.0;
            break;
        }

        case INSTANT: {
            // Jump to a random decade
            int target = random(NUM_DECADES);
            targetAmplitude = DECADES[target] * (0.5 + random(150) / 100.0);
            amplitude = targetAmplitude;  // Immediate
            break;
        }

        case OSCILLATING: {
            // Pick two decades to oscillate between
            oscDecadeLow = random(NUM_DECADES / 2);
            oscDecadeHigh = NUM_DECADES / 2 + random(NUM_DECADES / 2);
            oscPhase = 0;
            oscFreq = 0.0005 + random(30) / 10000.0;
            break;
        }

        case STEPPED: {
            nextSteppedJump = millis();  // Start immediately
            break;
        }
    }

    // Occasionally change wave frequency too
    if (random(3) == 0) {
        waveFreq = 0.03 + random(200) / 1000.0;
    }
}
