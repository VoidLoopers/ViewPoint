/**
 * ScatterLogPersistence.ino
 * @date 03-09-26
 *
 * ViewPoint Log-Scale Test: Scatter — Persistence Display with Log Axes
 *
 * Generates orbiting scatter data with Persistence (phosphor) display
 * on logarithmic axes. The orbits expand and contract through decades,
 * leaving afterglow trails that test how persistence interacts with
 * log-scaled coordinate space.
 *
 * This tests the interaction between:
 * - Logarithmic scaling on both X and Y axes
 * - Persistence (phosphor afterglow) rendering
 * - Orbits that cross multiple decades leave long trails in log space
 * - Trail fading should respect logarithmic coordinate distances
 *
 * - Expanding orbit: spiral outward through decades
 * - Breathing orbit: radius oscillates between decades
 * - Figure-eight: Lissajous pattern crossing the origin in log space
 * - Phase jump: orbit suddenly shifts decades (tests trail coherence)
 *
 * Mode: Continuous + Persistence
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch persistence trails stretch and compress in log space
 * - Notice how orbits appear distorted on log axes vs linear
 * - Compare expanding orbit trail: should spiral evenly in log space
 * - Observe phase jumps: does the trail snap or streak across decades?
 */

#include <ViewPoint.h>
using namespace viewpoint;

// Orbit parameters
float orbitAngle = 0.0;
float orbitRadius = 1.0;
float orbitTargetRadius = 1.0;
float orbitCenterX = 10.0;
float orbitCenterY = 10.0;
float targetCenterX = 10.0;
float targetCenterY = 10.0;
float angularSpeed = 0.05;

// Shape parameters
float freqRatioX = 1.0;
float freqRatioY = 1.0;
float phaseOffset = 0.0;

// Mode
enum OrbitMode {
    EXPANDING_SPIRAL,
    BREATHING_ORBIT,
    LISSAJOUS_LOG,
    PHASE_JUMPING,
    NUM_ORBIT_MODES
};

OrbitMode orbitMode = EXPANDING_SPIRAL;
unsigned long nextModeChange = 0;

// Spiral state
float spiralGrowthRate = 0.0;

// Breathing state
float breathPhase = 0.0;
float breathSpeed = 0.0;
float breathMin = 0.0;
float breathMax = 0.0;

// Jump state
unsigned long nextJump = 0;

void pickOrbitMode();

void setup() {
    randomSeed(analogRead(0));
    view.begin(PlotType::Scatter, Mode::Continuous);
    view.setDisplayMode(DisplayMode::Persistence);
    view.setDelay(5);
    view.plot(0).enableLogarithmicScale(AxisId::X);
    view.plot(0).enableLogarithmicScale(AxisId::Y);

    view.setPlotTitle("Log Persistence: Orbiting Decades");
    view.setAxisLabels("X (log)", "Y (log)");

    view.trace("Orbit").setColor(colors::Aquamarine);
    view.trace("Center").setColor(colors::OrangeRed);

    nextModeChange = millis() + 6000 + random(6000);
}

void loop() {
    unsigned long now = millis();

    if (now >= nextModeChange) {
        pickOrbitMode();
        nextModeChange = now + 5000 + random(8000);
    }

    // Smoothly drift center
    float logCX = log10(max(orbitCenterX, 1e-6f));
    float logCY = log10(max(orbitCenterY, 1e-6f));
    float logTCX = log10(max(targetCenterX, 1e-6f));
    float logTCY = log10(max(targetCenterY, 1e-6f));
    logCX += (logTCX - logCX) * 0.002;
    logCY += (logTCY - logCY) * 0.002;
    orbitCenterX = pow(10.0, logCX);
    orbitCenterY = pow(10.0, logCY);

    float x = 0, y = 0;

    switch (orbitMode) {
        case EXPANDING_SPIRAL: {
            orbitAngle += angularSpeed;
            orbitRadius *= (1.0 + spiralGrowthRate);
            if (orbitRadius > 1e4) orbitRadius = 0.01;  // Reset
            if (orbitRadius < 1e-4) orbitRadius = 0.01;

            float dx = orbitRadius * cos(orbitAngle);
            float dy = orbitRadius * sin(orbitAngle);
            // In log space, offset is multiplicative
            x = orbitCenterX * pow(10.0, dx / orbitCenterX * 0.5);
            y = orbitCenterY * pow(10.0, dy / orbitCenterY * 0.5);
            break;
        }

        case BREATHING_ORBIT: {
            orbitAngle += angularSpeed;
            breathPhase += breathSpeed;
            float logMin = log10(max(breathMin, 1e-6f));
            float logMax = log10(max(breathMax, 1e-6f));
            float mix = 0.5 + 0.5 * sin(breathPhase);
            float logR = logMin + mix * (logMax - logMin);
            orbitRadius = pow(10.0, logR);

            // Circular orbit at current radius in log space
            float logOrbR = log10(max(orbitRadius, 1e-6f));
            x = pow(10.0, logCX + logOrbR * cos(orbitAngle));
            y = pow(10.0, logCY + logOrbR * sin(orbitAngle));
            break;
        }

        case LISSAJOUS_LOG: {
            orbitAngle += angularSpeed;
            float logAmpX = log10(max(orbitRadius, 1e-6f));
            float logAmpY = log10(max(orbitRadius * 0.7, 1e-6f));

            float lx = logAmpX * sin(freqRatioX * orbitAngle + phaseOffset);
            float ly = logAmpY * sin(freqRatioY * orbitAngle);
            x = pow(10.0, logCX + lx);
            y = pow(10.0, logCY + ly);
            break;
        }

        case PHASE_JUMPING: {
            orbitAngle += angularSpeed;
            orbitRadius += (orbitTargetRadius - orbitRadius) * 0.02;

            float logR = log10(max(orbitRadius, 1e-6f)) * 0.3;
            x = pow(10.0, logCX + logR * cos(orbitAngle));
            y = pow(10.0, logCY + logR * sin(orbitAngle));

            if (now >= nextJump) {
                // Sudden jump to new decade position
                targetCenterX = pow(10.0, random(6) - 2);
                targetCenterY = pow(10.0, random(6) - 2);
                orbitTargetRadius = pow(10.0, random(3) - 1);
                nextJump = now + 2000 + random(4000);
            }
            break;
        }
    }

    if (orbitAngle > 2.0 * M_PI * 100) orbitAngle -= 2.0 * M_PI * 100;

    x = constrain(x, 1e-6f, 1e6f);
    y = constrain(y, 1e-6f, 1e6f);

    view.addData("Orbit", x, y);
    view.addData("Center", constrain(orbitCenterX, 1e-6f, 1e6f),
                           constrain(orbitCenterY, 1e-6f, 1e6f));
    view.send();
}

void pickOrbitMode() {
    orbitMode = (OrbitMode)random(NUM_ORBIT_MODES);

    switch (orbitMode) {
        case EXPANDING_SPIRAL:
            spiralGrowthRate = 0.001 + random(10) / 5000.0;
            if (random(2)) spiralGrowthRate = -spiralGrowthRate;
            angularSpeed = 0.03 + random(10) / 100.0;
            break;

        case BREATHING_ORBIT:
            breathMin = pow(10.0, random(3) - 2);
            breathMax = pow(10.0, random(3) + 1);
            breathSpeed = 0.005 + random(20) / 1000.0;
            breathPhase = 0.0;
            angularSpeed = 0.04 + random(8) / 100.0;
            break;

        case LISSAJOUS_LOG:
            freqRatioX = 1.0 + random(3);
            freqRatioY = 1.0 + random(3);
            phaseOffset = random(314) / 100.0;
            orbitRadius = pow(10.0, random(3));
            angularSpeed = 0.02 + random(6) / 100.0;
            break;

        case PHASE_JUMPING:
            nextJump = millis() + 1000 + random(2000);
            orbitTargetRadius = pow(10.0, random(3) - 1);
            angularSpeed = 0.05 + random(5) / 100.0;
            break;
    }

    // Occasionally shift center
    if (random(2) == 0) {
        targetCenterX = pow(10.0, random(5) - 1);
        targetCenterY = pow(10.0, random(5) - 1);
    }
}
