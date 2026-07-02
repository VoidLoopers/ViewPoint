/**
 * ScatterLogBothAxes.ino
 * @date 03-09-26
 *
 * ViewPoint Log-Scale Test: Scatter — Both Axes Logarithmic
 *
 * Generates X-Y scatter data where both axes use logarithmic scaling.
 * This is the classic log-log plot used for power laws, frequency
 * response, and material characterization. The data traces power-law
 * relationships (y = k * x^n) with varying exponents, plus noise
 * and outlier clusters that span many decades on both axes.
 *
 * This tests whether the log-scale renderer correctly:
 * - Applies log scaling independently to X and Y axes
 * - Renders power-law curves as straight lines on log-log plots
 * - Handles data spanning 6+ decades on both axes simultaneously
 * - Displays scatter points at correct positions in log-log space
 *
 * - Power law sweep: varying exponent from 0.5 to 3.0
 * - Frequency response curve: classic Bode-like magnitude plot
 * - Cluster migration: groups of points at different decade pairs
 * - Noise injection: proportional noise in both X and Y
 *
 * Mode: Continuous
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Power laws should appear as straight lines on the log-log plot
 * - Watch the slope change as the exponent varies
 * - Notice decade gridlines on both axes (should be evenly spaced)
 * - Compare cluster positions: do they land at correct decade intersections?
 */

#include <ViewPoint.h>
using namespace viewpoint;

// Power law parameters
float exponent = 1.5;
float targetExponent = 1.5;
float coefficient = 1.0;
float targetCoefficient = 1.0;

// X sweep state
float xSweepPhase = 0.0;
float xSweepSpeed = 0.001;
const float X_LOG_MIN = -2.0;  // 0.01
const float X_LOG_MAX = 4.0;   // 10,000

// Signal mode
enum ScatterMode {
    POWER_LAW_SWEEP,
    BODE_RESPONSE,
    CLUSTER_SCATTER,
    WANDERING_CLOUD,
    NUM_MODES
};

ScatterMode mode = POWER_LAW_SWEEP;
unsigned long nextModeChange = 0;

// Cluster state
struct Cluster {
    float logCenterX;
    float logCenterY;
    float spread;
    bool active;
};
const int MAX_CLUSTERS = 5;
Cluster clusters[MAX_CLUSTERS];
int activeCluster = 0;

// Cloud state
float cloudCenterX = 1.0;
float cloudCenterY = 1.0;
float cloudTargetX = 1.0;
float cloudTargetY = 1.0;
float cloudSpread = 0.5;

void pickMode();

void setup() {
    randomSeed(analogRead(0));
    view.begin(PlotType::Scatter, Mode::Continuous);
    view.setDelay(3);
    view.plot(0).enableLogarithmicScale(AxisId::X);
    view.plot(0).enableLogarithmicScale(AxisId::Y);

    view.setPlotTitle("Log-Log Scatter: Power Laws & Clusters");
    view.setAxisLabels("X (log)", "Y (log)");

    view.trace("Data").setColor(colors::SpringGreen);
    view.trace("Reference").setColor(colors::Plum);

    nextModeChange = millis() + 5000 + random(5000);
}

void loop() {
    unsigned long now = millis();

    if (now >= nextModeChange) {
        pickMode();
        nextModeChange = now + 4000 + random(8000);
    }

    float x = 0, y = 0;
    float refX = 0, refY = 0;

    switch (mode) {
        case POWER_LAW_SWEEP: {
            // Sweep X across decades, compute Y = k * X^n
            xSweepPhase += xSweepSpeed;
            if (xSweepPhase > 1.0) xSweepPhase -= 1.0;

            float logX = X_LOG_MIN + xSweepPhase * (X_LOG_MAX - X_LOG_MIN);
            x = pow(10.0, logX);

            // Smooth approach to target parameters
            exponent += (targetExponent - exponent) * 0.005;
            coefficient += (targetCoefficient - coefficient) * 0.005;

            y = coefficient * pow(x, exponent);

            // Reference point: clean power law (no noise)
            refX = x;
            refY = y;

            // Add proportional noise
            x *= (1.0 + 0.1 * (random(2000) - 1000) / 1000.0);
            y *= (1.0 + 0.15 * (random(2000) - 1000) / 1000.0);
            break;
        }

        case BODE_RESPONSE: {
            // Classic frequency response: flat then rolloff
            xSweepPhase += xSweepSpeed;
            if (xSweepPhase > 1.0) xSweepPhase -= 1.0;

            float logFreq = -1.0 + xSweepPhase * 6.0;  // 0.1 to 100,000
            x = pow(10.0, logFreq);

            float cornerFreq = pow(10.0, 1.0 + coefficient);
            float magnitude = 1.0 / sqrt(1.0 + pow(x / cornerFreq, 2.0 * exponent));
            y = magnitude * 1000.0;  // Scale up for visibility

            refX = x;
            refY = y;

            x *= (1.0 + 0.05 * (random(2000) - 1000) / 1000.0);
            y *= (1.0 + 0.1 * (random(2000) - 1000) / 1000.0);
            break;
        }

        case CLUSTER_SCATTER: {
            // Emit points from active cluster
            Cluster& c = clusters[activeCluster];
            if (c.active) {
                float dx = c.spread * (random(2000) - 1000) / 1000.0;
                float dy = c.spread * (random(2000) - 1000) / 1000.0;
                x = pow(10.0, c.logCenterX + dx);
                y = pow(10.0, c.logCenterY + dy);
                refX = pow(10.0, c.logCenterX);
                refY = pow(10.0, c.logCenterY);
            }

            // Cycle through clusters
            if (random(50) == 0) {
                activeCluster = (activeCluster + 1) % MAX_CLUSTERS;
            }
            break;
        }

        case WANDERING_CLOUD: {
            // Cloud drifts through log-log space
            float logCX = log10(max(cloudCenterX, 1e-6f));
            float logCY = log10(max(cloudCenterY, 1e-6f));
            float logTX = log10(max(cloudTargetX, 1e-6f));
            float logTY = log10(max(cloudTargetY, 1e-6f));
            logCX += (logTX - logCX) * 0.003;
            logCY += (logTY - logCY) * 0.003;
            cloudCenterX = pow(10.0, logCX);
            cloudCenterY = pow(10.0, logCY);

            float dx = cloudSpread * (random(2000) - 1000) / 1000.0;
            float dy = cloudSpread * (random(2000) - 1000) / 1000.0;
            x = pow(10.0, logCX + dx);
            y = pow(10.0, logCY + dy);
            refX = cloudCenterX;
            refY = cloudCenterY;

            // Occasionally pick new target
            if (random(200) == 0) {
                cloudTargetX = pow(10.0, random(6) - 2);
                cloudTargetY = pow(10.0, random(6) - 2);
                cloudSpread = 0.2 + random(80) / 100.0;
            }
            break;
        }
    }

    x = constrain(x, 1e-6f, 1e6f);
    y = constrain(y, 1e-6f, 1e6f);
    refX = constrain(refX, 1e-6f, 1e6f);
    refY = constrain(refY, 1e-6f, 1e6f);

    view.addData("Data", x, y);
    view.addData("Reference", refX, refY);
    view.send();
}

void pickMode() {
    mode = (ScatterMode)random(NUM_MODES);

    switch (mode) {
        case POWER_LAW_SWEEP:
            targetExponent = 0.5 + random(250) / 100.0;
            targetCoefficient = pow(10.0, random(4) - 2);
            xSweepSpeed = 0.0005 + random(20) / 10000.0;
            break;

        case BODE_RESPONSE:
            targetExponent = 1.0 + random(3);  // 1st to 3rd order
            coefficient = random(30) / 10.0;
            xSweepSpeed = 0.001 + random(20) / 10000.0;
            break;

        case CLUSTER_SCATTER:
            for (int i = 0; i < MAX_CLUSTERS; i++) {
                clusters[i].logCenterX = random(6) - 2;
                clusters[i].logCenterY = random(6) - 2;
                clusters[i].spread = 0.1 + random(50) / 100.0;
                clusters[i].active = random(3) > 0;
            }
            activeCluster = 0;
            break;

        case WANDERING_CLOUD:
            cloudTargetX = pow(10.0, random(6) - 2);
            cloudTargetY = pow(10.0, random(6) - 2);
            cloudSpread = 0.3 + random(70) / 100.0;
            break;
    }
}
