/**
 * CartesianLogMultiPlot.ino
 * @date 03-09-26
 *
 * ViewPoint Log-Scale Test: Cartesian — Multi-Plot Log vs Linear Comparison
 *
 * Creates 3 stacked plots receiving the same signal data:
 * - Plot 0: Logarithmic Y axis (mapData=true, default)
 * - Plot 1: Logarithmic Y axis (mapData=false, pre-mapped dB values)
 * - Plot 2: Linear Y axis (for visual comparison)
 *
 * The signal sweeps through decades with transitions between
 * steady-state, ramping, and burst regimes. This lets you directly
 * compare how the same signal renders on log vs linear axes and
 * validates both mapData modes.
 *
 * - All three plots show the same underlying signal
 * - Plot 0 sends raw linear values; plotter maps to log
 * - Plot 1 sends pre-computed dB values; plotter displays as-is
 * - Plot 2 sends raw linear values on a linear axis
 *
 * Mode: Continuous
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Compare decade transitions across all three plots
 * - Verify Plot 0 and Plot 1 show equivalent log representations
 * - Notice how small signals vanish on the linear plot but remain
 *   visible on the log plots
 * - Watch burst events: log scale shows proportional response
 */

#include <ViewPoint.h>
using namespace viewpoint;

// Signal generation
float signalAmplitude = 1.0;
float targetAmplitude = 1.0;

enum SignalPhase {
    STEADY,
    RAMPING,
    BURSTING,
    DECADE_WALK,
    NUM_PHASES
};

SignalPhase phase = STEADY;
unsigned long nextPhaseChange = 0;

// Steady state
float steadyBase = 1.0;

// Ramp state
float rampStart = 0.0;
float rampEnd = 0.0;
float rampProgress = 0.0;
float rampSpeed = 0.0;

// Burst state
float burstBase = 0.0;
float burstMag = 0.0;
unsigned long burstTimer = 0;
bool burstActive = false;

// Decade walk
int walkDecade = 0;
int walkDirection = 1;
unsigned long nextWalkStep = 0;

// Waveform
float wavePhase = 0.0;
const float WAVE_FREQ = 0.08;

void pickPhase();

void setup() {
    randomSeed(analogRead(0));

    view.begin(Mode::Continuous);
    view.setDelay(2);
    view.setNumberOfPlots(3);

    // Plot 0: Log axis with automatic data mapping
    view.plot(0).setTitle("Log Scale (mapData=true)");
    view.plot(0).setAxisLabels("Sample", "Amplitude");
    view.plot(0).enableLogarithmicScale();

    // Plot 1: Log axis labels only (we send dB values)
    view.plot(1).setTitle("Log Scale (mapData=false, dB)");
    view.plot(1).setAxisLabels("Sample", "Level (dB)");
    view.plot(1).enableLogarithmicScale(false);

    // Plot 2: Linear axis for comparison
    view.plot(2).setTitle("Linear Scale (reference)");
    view.plot(2).setAxisLabels("Sample", "Amplitude");

    view.trace("Log").setColor(colors::DodgerBlue);
    view.trace("dB").setColor(colors::LimeGreen);
    view.trace("Linear").setColor(colors::OrangeRed);

    nextPhaseChange = millis() + 3000 + random(4000);
}

void loop() {
    unsigned long now = millis();

    if (now >= nextPhaseChange) {
        pickPhase();
        nextPhaseChange = now + 3000 + random(6000);
    }

    // Update signal amplitude based on current phase
    switch (phase) {
        case STEADY:
            signalAmplitude = steadyBase * (1.0 + 0.05 * (random(200) - 100) / 100.0);
            break;

        case RAMPING: {
            rampProgress += rampSpeed;
            if (rampProgress > 1.0) rampProgress = 1.0;
            float logStart = log10(max(rampStart, 1e-6f));
            float logEnd = log10(max(rampEnd, 1e-6f));
            float logVal = logStart + rampProgress * (logEnd - logStart);
            signalAmplitude = pow(10.0, logVal);
            break;
        }

        case BURSTING:
            if (burstActive) {
                signalAmplitude = burstMag * (0.8 + random(40) / 100.0);
                if (now >= burstTimer) burstActive = false;
            } else {
                signalAmplitude = burstBase * (0.9 + random(20) / 100.0);
                if (random(100) < 3) {
                    burstActive = true;
                    burstMag = burstBase * pow(10.0, 1 + random(4));
                    burstTimer = now + 30 + random(150);
                }
            }
            break;

        case DECADE_WALK:
            if (now >= nextWalkStep) {
                walkDecade += walkDirection;
                if (walkDecade >= 5 || walkDecade <= -3) walkDirection = -walkDirection;
                walkDecade = constrain(walkDecade, -3, 5);
                nextWalkStep = now + 800 + random(1200);
            }
            signalAmplitude = pow(10.0, walkDecade) * (0.8 + random(40) / 100.0);
            break;
    }

    signalAmplitude = constrain(signalAmplitude, 1e-6f, 1e6f);

    // Generate waveform
    wavePhase += WAVE_FREQ;
    if (wavePhase > 2.0 * M_PI) wavePhase -= 2.0 * M_PI;
    float waveform = 0.5 + 0.5 * sin(wavePhase);  // 0 to 1 range
    float output = signalAmplitude * (0.5 + waveform * 0.5);  // Always positive for log

    output = constrain(output, 1e-6f, 1e6f);

    // Convert to dB for Plot 1
    float dBValue = 20.0 * log10(max(output, 1e-6f));

    // Send to all three plots
    view.addData("Log", output);
    view.addData("dB", dBValue);
    view.addData("Linear", output);
    view.send();
}

void pickPhase() {
    phase = (SignalPhase)random(NUM_PHASES);

    switch (phase) {
        case STEADY:
            steadyBase = pow(10.0, random(7) - 3);
            break;

        case RAMPING:
            rampStart = signalAmplitude;
            rampEnd = pow(10.0, random(7) - 3);
            rampProgress = 0.0;
            rampSpeed = 0.002 + random(10) / 5000.0;
            break;

        case BURSTING:
            burstBase = pow(10.0, random(3) - 2);
            burstActive = false;
            break;

        case DECADE_WALK:
            walkDecade = (int)(log10(max(signalAmplitude, 1e-6f)));
            walkDirection = random(2) ? 1 : -1;
            nextWalkStep = millis();
            break;
    }
}
