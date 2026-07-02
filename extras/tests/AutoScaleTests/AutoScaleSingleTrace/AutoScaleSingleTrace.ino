/**
 * AutoScaleSingleTrace.ino
 * @date 03-08-26
 *
 * ViewPoint Auto-Scale Stress Test: Single Plot, Single Trace
 *
 * Generates a single waveform with unpredictable range shifts designed
 * to challenge the plotter's auto-scaling algorithm. The signal cycles
 * through random behavioral states: calm oscillation, explosive growth,
 * sudden DC offsets, extreme spikes, and chaotic noise bursts.
 *
 * State transitions are randomized so the pattern never repeats the
 * same way twice. Changes occur both at and between packet boundaries
 * (default 500 points) to stress edge cases.
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch how the auto-scale responds to sudden range expansion vs contraction
 * - Note behavior when spikes appear: does the scale jump and recover?
 * - Try adding view.setVerticalRange() to compare fixed vs auto scale
 */

#include <ViewPoint.h>
using namespace viewpoint;

// Behavioral states for the waveform
enum State {
    CALM,       // Small sine wave, tight range
    GROWING,    // Amplitude ramps up over time
    WILD,       // Large random jumps, no structure
    SHIFTED,    // Stable oscillation with large DC offset
    SPIKY,      // Mostly calm with extreme outlier spikes
    SETTLING,   // Exponential decay back toward zero
    NUM_STATES
};

// State machine
State currentState = CALM;
unsigned long stateStart = 0;
unsigned long stateDuration = 0;

// Waveform parameters (modified per-state)
float amplitude = 1.0;
float dcOffset = 0.0;
float frequency = 0.1;  // radians per sample
float phase = 0.0;
float noiseLevel = 0.0;

// Settling decay
float settleAmplitude = 1.0;

// Sample counter for mid-packet transitions
unsigned long sampleCount = 0;

void pickNewState();
void enterState(State s);
float generateSample();

void setup() {
    randomSeed(analogRead(0));
    view.begin(Mode::Continuous);
    view.setDelay(2);

    // No vertical range set — auto-scale must figure it out
    view.setPlotTitle("AutoScale Single Trace");
    view.setAxisLabels("Sample", "Value");
    view.trace("Signal").setColor(colors::DodgerBlue);

    stateStart = millis();
    stateDuration = 2000 + random(3000);  // 2-5 seconds initial
}

void loop() {
    unsigned long now = millis();

    // Check for state transition (time-based with randomness)
    if (now - stateStart >= stateDuration) {
        pickNewState();
    }

    // Mid-packet surprise: 0.3% chance per sample of forcing an immediate state change
    if (random(1000) < 3) {
        pickNewState();
    }

    float value = generateSample();
    view.addData("Signal", value);
    view.send();

    sampleCount++;
}

void pickNewState() {
    // Avoid picking the same state twice in a row
    State next;
    do {
        next = (State)random(NUM_STATES);
    } while (next == currentState);

    enterState(next);
}

void enterState(State s) {
    currentState = s;
    stateStart = millis();

    switch (s) {
        case CALM:
            // Tight, predictable oscillation
            amplitude = 0.5 + random(100) / 100.0;     // 0.5 - 1.5
            dcOffset = 0.0;
            frequency = 0.05 + random(100) / 1000.0;   // 0.05 - 0.15
            noiseLevel = 0.05;
            stateDuration = 3000 + random(4000);        // 3-7 seconds
            break;

        case GROWING:
            // Start small, ramp up
            amplitude = 0.1;
            dcOffset = 0.0;
            frequency = 0.08 + random(80) / 1000.0;
            noiseLevel = 0.1;
            stateDuration = 4000 + random(4000);        // 4-8 seconds
            break;

        case WILD:
            // Chaotic, large range
            amplitude = 20.0 + random(80);              // 20 - 100
            dcOffset = (random(200) - 100);             // -100 to +100
            frequency = 0.2 + random(300) / 1000.0;
            noiseLevel = amplitude * 0.5;
            stateDuration = 2000 + random(3000);        // 2-5 seconds
            break;

        case SHIFTED:
            // Moderate amplitude but large DC shift
            amplitude = 2.0 + random(500) / 100.0;     // 2 - 7
            dcOffset = (random(2) == 0 ? 1 : -1) * (50.0 + random(200));  // ±50 to ±250
            frequency = 0.06 + random(60) / 1000.0;
            noiseLevel = 0.2;
            stateDuration = 3000 + random(5000);        // 3-8 seconds
            break;

        case SPIKY:
            // Calm baseline with random extreme spikes
            amplitude = 1.0;
            dcOffset = 0.0;
            frequency = 0.1;
            noiseLevel = 0.1;
            stateDuration = 4000 + random(4000);        // 4-8 seconds
            break;

        case SETTLING:
            // Decay from current amplitude toward zero
            settleAmplitude = amplitude > 1.0 ? amplitude : 10.0 + random(40);
            dcOffset *= 0.5;  // Halve the DC offset
            frequency = 0.07;
            noiseLevel = 0.05;
            stateDuration = 3000 + random(3000);        // 3-6 seconds
            break;

        default:
            break;
    }
}

float generateSample() {
    phase += frequency;
    if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;

    float base = 0.0;

    switch (currentState) {
        case CALM:
            base = dcOffset + amplitude * sin(phase);
            break;

        case GROWING: {
            // Ramp amplitude over the state duration
            unsigned long elapsed = millis() - stateStart;
            float progress = (float)elapsed / (float)stateDuration;
            float growingAmp = amplitude + progress * (50.0 + random(50));
            base = dcOffset + growingAmp * sin(phase);
            break;
        }

        case WILD:
            // Structured chaos: sine + large random offset each sample
            base = dcOffset + amplitude * sin(phase)
                 + (random(2000) - 1000) / 10.0;
            break;

        case SHIFTED:
            base = dcOffset + amplitude * sin(phase);
            break;

        case SPIKY:
            base = dcOffset + amplitude * sin(phase);
            // 2% chance of an extreme spike
            if (random(100) < 2) {
                float spikeSign = (random(2) == 0) ? 1.0 : -1.0;
                base += spikeSign * (50.0 + random(200));
            }
            break;

        case SETTLING: {
            unsigned long elapsed = millis() - stateStart;
            float decay = exp(-3.0 * (float)elapsed / (float)stateDuration);
            base = dcOffset * decay + settleAmplitude * decay * sin(phase);
            break;
        }

        default:
            break;
    }

    // Add noise
    float noise = noiseLevel * (random(2000) - 1000) / 1000.0;
    return base + noise;
}
