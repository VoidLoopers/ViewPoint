/**
 * PolarLogRadialContinuous.ino
 * @date 03-09-26
 *
 * ViewPoint Log-Scale Test: Polar — Continuous Mode with Logarithmic Radial Axis
 *
 * Streams point-by-point polar data where the radial axis uses logarithmic
 * scaling. Simulates a scanning radar or rotating sensor where the detected
 * signal level varies over many decades depending on target distance,
 * cross-section, and angle.
 *
 * This tests whether the log-scale renderer correctly:
 * - Maps streaming radial values to logarithmic positions
 * - Handles point-by-point updates on a log polar display
 * - Renders targets at vastly different decades in the same sweep
 * - Displays the trace path correctly as it spirals through decades
 *
 * - Radar sweep: rotating angle with targets at various decades
 * - Spiral scan: radius grows exponentially while angle advances
 * - Random targets: sudden jumps to new decade/angle combinations
 * - Signal fade: gradual decay from strong to weak over many sweeps
 *
 * Mode: Continuous
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch the radar sweep: targets at different decades should all be visible
 * - Notice spiral scan: should appear as evenly-spaced spiral on log radial
 * - Compare target visibility across decades (all should be distinguishable)
 * - Watch signal fade: gradual movement toward center on log scale
 */

#include <ViewPoint.h>
using namespace viewpoint;

// Scan state
float scanAngle = 0.0;
float scanSpeed = 0.5;  // degrees per sample

// Signal mode
enum ScanMode {
    RADAR_SWEEP,
    SPIRAL_SCAN,
    RANDOM_TARGETS,
    SIGNAL_FADE,
    NUM_SCAN_MODES
};

ScanMode scanMode = RADAR_SWEEP;
unsigned long nextModeChange = 0;

// Radar targets (angle, logRadius, width)
struct Target {
    float angle;
    float logRadius;
    float width;  // Angular width in degrees
    bool active;
};

const int MAX_TARGETS = 8;
Target targets[MAX_TARGETS];
float radarNoise = -3.0;  // Log noise floor

// Spiral state
float spiralRadius = 0.01;
float spiralGrowth = 0.0;

// Fade state
float fadeLevel = 1000.0;
float fadeRate = 0.0;
float fadeAngleSpeed = 0.0;

// Random target state
unsigned long nextTargetJump = 0;
float rtRadius = 1.0;
float rtAngle = 0.0;

void pickScanMode();

void setup() {
    randomSeed(analogRead(0));
    view.begin(PlotType::Polar, Mode::Continuous);
    view.enableLogarithmicScale();
    view.setDelay(2);

    view.setPlotTitle("Log Polar Continuous: Radar Scan");
    view.setAngularOffset(90);
    view.setAngularStep(45);

    view.trace("Scan").setColor(colors::LimeGreen);
    view.trace("Peak").setColor(colors::Red);

    // Initialize targets
    for (int i = 0; i < MAX_TARGETS; i++) {
        targets[i].angle = random(360);
        targets[i].logRadius = random(6) - 2;  // 0.01 to 10,000
        targets[i].width = 5.0 + random(20);
        targets[i].active = random(2);
    }

    nextModeChange = millis() + 5000 + random(5000);
}

void loop() {
    unsigned long now = millis();

    if (now >= nextModeChange) {
        pickScanMode();
        nextModeChange = now + 4000 + random(8000);
    }

    float radius = 0.0;
    float angle = 0.0;
    float peakRadius = 0.0;
    float peakAngle = 0.0;

    switch (scanMode) {
        case RADAR_SWEEP: {
            scanAngle += scanSpeed;
            if (scanAngle >= 360.0) scanAngle -= 360.0;
            angle = scanAngle;

            // Start with noise floor
            float logR = radarNoise + 0.5 * (random(100) / 100.0);

            // Check each target
            float bestLogR = logR;
            for (int i = 0; i < MAX_TARGETS; i++) {
                if (!targets[i].active) continue;
                float angleDiff = scanAngle - targets[i].angle;
                while (angleDiff > 180.0) angleDiff -= 360.0;
                while (angleDiff < -180.0) angleDiff += 360.0;

                if (abs(angleDiff) < targets[i].width) {
                    float strength = 1.0 - abs(angleDiff) / targets[i].width;
                    float targetLogR = targets[i].logRadius * strength + radarNoise * (1.0 - strength);
                    logR = max(logR, targetLogR);
                    if (targetLogR > bestLogR) {
                        bestLogR = targetLogR;
                        peakAngle = targets[i].angle;
                    }
                }
            }

            radius = pow(10.0, logR);
            peakRadius = pow(10.0, bestLogR);

            // Slowly drift targets
            if (random(500) == 0) {
                int t = random(MAX_TARGETS);
                targets[t].angle += (random(20) - 10);
                targets[t].logRadius += (random(200) - 100) / 200.0;
                targets[t].logRadius = constrain(targets[t].logRadius, -2.0f, 4.0f);
            }
            if (random(1000) == 0) {
                int t = random(MAX_TARGETS);
                targets[t].active = !targets[t].active;
            }
            break;
        }

        case SPIRAL_SCAN: {
            scanAngle += scanSpeed;
            if (scanAngle >= 360.0) scanAngle -= 360.0;
            angle = scanAngle;

            spiralRadius *= (1.0 + spiralGrowth);
            if (spiralRadius > 1e5) spiralRadius = 0.01;
            if (spiralRadius < 1e-4) spiralRadius = 0.01;

            radius = spiralRadius * (0.9 + random(20) / 100.0);
            peakRadius = spiralRadius;
            peakAngle = angle;
            break;
        }

        case RANDOM_TARGETS: {
            if (now >= nextTargetJump) {
                rtRadius = pow(10.0, random(7) - 3);
                rtAngle = random(360);
                nextTargetJump = now + 100 + random(900);
            }
            angle = rtAngle + (random(20) - 10) * 0.5;
            radius = rtRadius * (0.7 + random(60) / 100.0);
            peakRadius = rtRadius;
            peakAngle = rtAngle;
            break;
        }

        case SIGNAL_FADE: {
            scanAngle += fadeAngleSpeed;
            if (scanAngle >= 360.0) scanAngle -= 360.0;
            angle = scanAngle;

            fadeLevel *= (1.0 - fadeRate);
            if (fadeLevel < 1e-4) {
                fadeLevel = pow(10.0, random(5) + 1);
            }

            radius = fadeLevel * (0.9 + random(20) / 100.0);
            peakRadius = fadeLevel;
            peakAngle = angle;
            break;
        }
    }

    radius = constrain(radius, 1e-6f, 1e6f);
    peakRadius = constrain(peakRadius, 1e-6f, 1e6f);
    if (angle < 0) angle += 360.0;
    if (angle >= 360.0) angle -= 360.0;

    view.addData("Scan", radius, angle);
    view.addData("Peak", peakRadius, peakAngle);
    view.send();
}

void pickScanMode() {
    scanMode = (ScanMode)random(NUM_SCAN_MODES);

    switch (scanMode) {
        case RADAR_SWEEP:
            scanSpeed = 0.3 + random(10) / 10.0;
            radarNoise = -3.0 + random(200) / 100.0;
            // Refresh some targets
            for (int i = 0; i < MAX_TARGETS; i++) {
                if (random(3) == 0) {
                    targets[i].angle = random(360);
                    targets[i].logRadius = random(6) - 2;
                    targets[i].width = 3.0 + random(25);
                    targets[i].active = true;
                }
            }
            break;

        case SPIRAL_SCAN:
            spiralRadius = pow(10.0, random(3) - 2);
            spiralGrowth = 0.0005 + random(20) / 10000.0;
            scanSpeed = 0.5 + random(20) / 10.0;
            break;

        case RANDOM_TARGETS:
            nextTargetJump = millis();
            break;

        case SIGNAL_FADE:
            fadeLevel = pow(10.0, random(5) + 1);
            fadeRate = 0.0001 + random(10) / 100000.0;
            fadeAngleSpeed = 0.2 + random(30) / 10.0;
            break;
    }
}
