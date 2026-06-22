/**
 * PolarLogRadialFrames.ino
 * @date 03-09-26
 *
 * ViewPoint Log-Scale Test: Polar — Frames Mode with Logarithmic Radial Axis
 *
 * Generates complete polar frames with logarithmic radial scaling. This
 * simulates antenna radiation patterns, radar cross-section measurements,
 * and acoustic directivity where gain spans many decades (like a dBi plot
 * rendered in linear-to-log space).
 *
 * This tests whether the log-scale renderer correctly:
 * - Maps radial values to logarithmic display on a polar plot
 * - Renders concentric decade rings at even visual spacing
 * - Handles patterns with both strong lobes and deep nulls (many decades)
 * - Displays frame-to-frame pattern changes on a log radial axis
 *
 * - Omni pattern: nearly constant radius across angles
 * - Dipole: figure-eight with deep nulls
 * - Beam: narrow main lobe with wide dynamic range
 * - Multi-lobe: random number of lobes at random strengths
 * - Morphing: smooth transitions between patterns
 *
 * Mode: Frames
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch null depths: on log scale they should be clearly separated
 * - Compare omni vs beam: log scale reveals sidelobe structure
 * - Notice decade rings: they should be evenly spaced radially
 * - Watch morphing transitions: do the patterns blend smoothly?
 */

#include <ViewPoint.h>
using namespace viewpoint;

const int NUM_POINTS = 360;  // 1 degree resolution

float patternA[NUM_POINTS];  // Current pattern
float patternB[NUM_POINTS];  // Target pattern
float morphProgress = 0.0;
float morphSpeed = 0.0;

enum PatternType {
    OMNI,
    DIPOLE,
    BEAM,
    MULTI_LOBE,
    CARDIOID,
    NUM_PATTERN_TYPES
};

PatternType currentType = OMNI;
PatternType targetType = DIPOLE;

// Pattern parameters
float beamWidth = 30.0;
float beamDirection = 0.0;
int numLobes = 3;
float lobeAmplitudes[8];
float lobeDirections[8];
float rotationAngle = 0.0;
float rotationSpeed = 0.0;

int frameCount = 0;

void generatePattern(float* pattern, PatternType type);
void pickNewTarget();

void setup() {
    randomSeed(analogRead(0));
    view.begin(PlotType::Polar, Mode::Frames, NUM_POINTS);
    view.enableLogarithmicScale();
    view.setDelay(50);  // 20 fps

    view.setPlotTitle("Log Polar: Radiation Patterns");
    view.setAngularOffset(90);   // North-up
    view.setAngularStep(30);

    view.trace("Pattern").setColor(colors::DodgerBlue);

    generatePattern(patternA, OMNI);
    pickNewTarget();
}

void loop() {
    // Advance morph
    morphProgress += morphSpeed;

    if (morphProgress >= 1.0) {
        morphProgress = 0.0;
        // Copy B into A
        for (int i = 0; i < NUM_POINTS; i++) {
            patternA[i] = patternB[i];
        }
        currentType = targetType;
        pickNewTarget();
    }

    // Update rotation
    rotationAngle += rotationSpeed;
    if (rotationAngle > 360.0) rotationAngle -= 360.0;

    // Interpolate and send
    for (int i = 0; i < NUM_POINTS; i++) {
        float angle = (float)i + rotationAngle;
        if (angle >= 360.0) angle -= 360.0;

        // Interpolate between patterns in log space
        float logA = log10(max(patternA[i], 1e-6f));
        float logB = log10(max(patternB[i], 1e-6f));
        float logVal = logA + morphProgress * (logB - logA);
        float radius = pow(10.0, logVal);

        // Add per-frame noise
        float noise = 1.0 + 0.05 * (random(2000) - 1000) / 1000.0;
        radius *= noise;

        radius = constrain(radius, 1e-6f, 1e6f);
        view.addData("Pattern", radius, angle);
    }

    view.send();
    frameCount++;
}

void generatePattern(float* pattern, PatternType type) {
    switch (type) {
        case OMNI:
            for (int i = 0; i < NUM_POINTS; i++) {
                float base = pow(10.0, random(3));  // 1 to 1000
                pattern[i] = base * (0.9 + random(20) / 100.0);
            }
            break;

        case DIPOLE:
            for (int i = 0; i < NUM_POINTS; i++) {
                float angle_rad = i * M_PI / 180.0;
                float cosTheta = cos(angle_rad);
                // Dipole: |cos(theta)|, with minimum at 0.001
                float mag = abs(cosTheta);
                float logMag = -3.0 + mag * 4.0;  // -3 (0.001) to +1 (10)
                pattern[i] = pow(10.0, logMag);
            }
            break;

        case BEAM: {
            float dir_rad = beamDirection * M_PI / 180.0;
            float bw_rad = beamWidth * M_PI / 180.0;
            float invBw2 = 1.0 / (2.0 * bw_rad * bw_rad);

            for (int i = 0; i < NUM_POINTS; i++) {
                float angle_rad = i * M_PI / 180.0;
                float diff = angle_rad - dir_rad;
                // Wrap to [-pi, pi]
                while (diff > M_PI) diff -= 2.0 * M_PI;
                while (diff < -M_PI) diff += 2.0 * M_PI;

                float mainLobe = exp(-diff * diff * invBw2);
                // Sidelobes
                float sidelobes = 0.01 * (0.5 + 0.5 * cos(diff * 8.0));
                float total = max(mainLobe, sidelobes);
                float logTotal = -3.0 + total * 5.0;  // Wide dynamic range
                pattern[i] = pow(10.0, logTotal);
            }
            break;
        }

        case MULTI_LOBE:
            for (int i = 0; i < NUM_POINTS; i++) {
                float angle_rad = i * M_PI / 180.0;
                float total = 0.001;  // Noise floor
                for (int l = 0; l < numLobes; l++) {
                    float lobeDir = lobeDirections[l] * M_PI / 180.0;
                    float diff = angle_rad - lobeDir;
                    while (diff > M_PI) diff -= 2.0 * M_PI;
                    while (diff < -M_PI) diff += 2.0 * M_PI;
                    float width = 0.2 + random(30) / 100.0;
                    float lobe = lobeAmplitudes[l] * exp(-diff * diff / (2.0 * width * width));
                    total += lobe;
                }
                pattern[i] = constrain(total, 1e-6f, 1e6f);
            }
            break;

        case CARDIOID:
            for (int i = 0; i < NUM_POINTS; i++) {
                float angle_rad = i * M_PI / 180.0;
                float shape = 0.5 + 0.5 * cos(angle_rad);
                float logShape = -3.0 + shape * 4.0;
                pattern[i] = pow(10.0, logShape);
            }
            break;
    }
}

void pickNewTarget() {
    targetType = (PatternType)random(NUM_PATTERN_TYPES);
    morphSpeed = 0.005 + random(20) / 1000.0;

    // Randomize pattern parameters
    beamWidth = 10.0 + random(50);
    beamDirection = random(360);
    numLobes = 2 + random(6);
    for (int i = 0; i < numLobes; i++) {
        lobeAmplitudes[i] = pow(10.0, random(4));
        lobeDirections[i] = random(360);
    }
    rotationSpeed = (random(100) - 50) / 500.0;

    generatePattern(patternB, targetType);
}
