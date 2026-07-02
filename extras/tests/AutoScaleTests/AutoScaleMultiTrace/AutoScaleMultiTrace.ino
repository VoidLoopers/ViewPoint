/**
 * AutoScaleMultiTrace.ino
 * @date 03-08-26
 *
 * ViewPoint Auto-Scale Stress Test: Single Plot, Multiple Traces
 *
 * Three traces share a single plot, each with independent range behavior.
 * The auto-scaler must accommodate all traces simultaneously, which means
 * one calm trace and one explosive trace force the scale to span both.
 *
 * Trace behaviors:
 * - Alpha: Slow wanderer with sudden jumps to new ranges
 * - Beta:  Burst oscillator that alternates between silence and violence
 * - Gamma: Staircase with random step heights and occasional free-fall
 *
 * Each trace evolves independently with random timing, creating scenarios
 * where traces diverge wildly then reconverge. The auto-scaler must track
 * the union of all active ranges without over-expanding or lagging behind.
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Observe how scale behaves when one trace explodes while others are calm
 * - Watch the scale contract after a spike subsides across all traces
 * - Try adding view.addDerivedTrace() to see averaging behavior
 */

#include <ViewPoint.h>
using namespace viewpoint;

// ===== Alpha: Slow wanderer with sudden jumps =====
float alphaValue = 0.0;
float alphaTarget = 0.0;
float alphaDrift = 0.005;    // Approach rate
unsigned long alphaNextJump = 0;

// ===== Beta: Burst oscillator =====
float betaPhase = 0.0;
float betaAmplitude = 0.0;
float betaTargetAmp = 0.0;
float betaFreq = 0.15;
unsigned long betaNextBurst = 0;
bool betaActive = false;

// ===== Gamma: Random staircase =====
float gammaLevel = 0.0;
float gammaTarget = 0.0;
float gammaStepRate = 0.02;
unsigned long gammaNextStep = 0;
bool gammaFalling = false;

unsigned long sampleCount = 0;

void updateAlpha();
void updateBeta();
void updateGamma();

void setup() {
    randomSeed(analogRead(0) ^ (analogRead(1) << 8));
    view.begin(Mode::Continuous);
    view.setDelay(2);

    view.setPlotTitle("AutoScale Multi-Trace");
    view.setAxisLabels("Sample", "Value");

    view.trace("Alpha").setColor(colors::Crimson);
    view.trace("Beta").setColor(colors::LimeGreen);
    view.trace("Gamma").setColor(colors::RoyalBlue);

    unsigned long now = millis();
    alphaNextJump = now + 2000 + random(3000);
    betaNextBurst = now + 1000 + random(2000);
    gammaNextStep = now + 500 + random(1500);
}

void loop() {
    updateAlpha();
    updateBeta();
    updateGamma();

    view.addData("Alpha", alphaValue);
    view.addData("Beta", betaAmplitude * sin(betaPhase));
    view.addData("Gamma", gammaLevel);

    view.send();
    sampleCount++;
}

// ===== Alpha: Slow wanderer with sudden range jumps =====
void updateAlpha() {
    unsigned long now = millis();

    // Gradual drift toward target
    alphaValue += (alphaTarget - alphaValue) * alphaDrift;

    // Small noise
    alphaValue += (random(200) - 100) / 500.0;

    // Time for a jump?
    if (now >= alphaNextJump) {
        // Pick a new range — could be anything
        int behavior = random(5);
        switch (behavior) {
            case 0:
                // Small range near zero
                alphaTarget = (random(200) - 100) / 50.0;  // ±2
                alphaDrift = 0.01;
                break;
            case 1:
                // Large positive
                alphaTarget = 30.0 + random(70);            // 30-100
                alphaDrift = 0.005 + random(10) / 1000.0;
                break;
            case 2:
                // Large negative
                alphaTarget = -(30.0 + random(70));         // -100 to -30
                alphaDrift = 0.005 + random(10) / 1000.0;
                break;
            case 3:
                // Instant snap (no drift)
                alphaValue = (random(400) - 200);           // -200 to +200
                alphaTarget = alphaValue;
                alphaDrift = 0.001;
                break;
            case 4:
                // Return to zero quickly
                alphaTarget = 0.0;
                alphaDrift = 0.05;
                break;
        }

        alphaNextJump = now + 2000 + random(6000);  // 2-8 seconds between jumps
    }
}

// ===== Beta: Burst oscillator =====
void updateBeta() {
    unsigned long now = millis();

    betaPhase += betaFreq;
    if (betaPhase > 2.0 * M_PI) betaPhase -= 2.0 * M_PI;

    if (betaActive) {
        // Ramp amplitude toward target
        betaAmplitude += (betaTargetAmp - betaAmplitude) * 0.02;

        // Random amplitude wobble
        betaAmplitude += (random(100) - 50) / 500.0;

        // Check if burst should end
        if (now >= betaNextBurst) {
            betaActive = false;
            betaNextBurst = now + 1000 + random(4000);  // 1-5 seconds of silence
        }
    } else {
        // Decay toward zero
        betaAmplitude *= 0.98;
        if (abs(betaAmplitude) < 0.01) betaAmplitude = 0.0;

        // Check if a burst should start
        if (now >= betaNextBurst) {
            betaActive = true;

            // Pick burst intensity
            int intensity = random(4);
            switch (intensity) {
                case 0:
                    betaTargetAmp = 1.0 + random(400) / 100.0;  // 1-5 (gentle)
                    break;
                case 1:
                    betaTargetAmp = 10.0 + random(30);           // 10-40 (moderate)
                    break;
                case 2:
                    betaTargetAmp = 50.0 + random(100);          // 50-150 (strong)
                    break;
                case 3:
                    betaTargetAmp = 200.0 + random(300);         // 200-500 (extreme)
                    break;
            }

            // Random sign
            if (random(2) == 0) betaTargetAmp = -betaTargetAmp;

            betaFreq = 0.05 + random(200) / 1000.0;  // Variable frequency
            betaNextBurst = now + 1500 + random(5000); // 1.5-6.5 seconds of burst
        }
    }
}

// ===== Gamma: Random staircase with free-fall =====
void updateGamma() {
    unsigned long now = millis();

    if (gammaFalling) {
        // Free-fall: rapid descent toward a random floor
        gammaLevel += (gammaTarget - gammaLevel) * 0.1;
        if (abs(gammaLevel - gammaTarget) < 0.5) {
            gammaFalling = false;
            gammaNextStep = now + 500 + random(2000);
        }
    } else {
        // Approach current step target slowly
        gammaLevel += (gammaTarget - gammaLevel) * gammaStepRate;

        // Small noise riding on the step
        gammaLevel += (random(100) - 50) / 200.0;
    }

    // Time for a new step?
    if (now >= gammaNextStep && !gammaFalling) {
        int stepType = random(6);
        switch (stepType) {
            case 0:
                // Small step up
                gammaTarget = gammaLevel + 2.0 + random(800) / 100.0;
                gammaStepRate = 0.01 + random(20) / 1000.0;
                break;
            case 1:
                // Small step down
                gammaTarget = gammaLevel - (2.0 + random(800) / 100.0);
                gammaStepRate = 0.01 + random(20) / 1000.0;
                break;
            case 2:
                // Large jump to positive absolute target
                gammaTarget = 50.0 + random(4950);               // 50-5000
                gammaStepRate = 0.005;
                break;
            case 3:
                // Large jump to negative absolute target
                gammaTarget = -(50.0 + random(4950));             // -5000 to -50
                gammaStepRate = 0.005;
                break;
            case 4:
                // Free-fall to a random level
                gammaTarget = (random(600) - 300);
                gammaFalling = true;
                break;
            case 5:
                // Return toward zero
                gammaTarget = (random(20) - 10);
                gammaStepRate = 0.03;
                break;
        }

        if (!gammaFalling) {
            gammaNextStep = now + 800 + random(3000);  // 0.8-3.8 seconds between steps
        }
    }
}
