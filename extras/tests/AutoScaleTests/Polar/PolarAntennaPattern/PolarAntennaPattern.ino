/**
 * PolarAntennaPattern.ino
 * @date 03-08-26
 *
 * ViewPoint Auto-Scale Test: Polar — Morphing Antenna Radiation Pattern
 *
 * Simulates an antenna radiation pattern that transitions between
 * different configurations. The gain pattern is plotted as radius vs
 * angle on a polar display, and the peak gain varies dramatically
 * between configurations.
 *
 * Pattern types:
 * - Omnidirectional:  Nearly uniform gain, low peak (~1)
 * - Dipole:           Figure-8 pattern, moderate gain (~2.15)
 * - Cardioid:         Heart-shaped, one null (~5)
 * - Yagi-like:        Narrow main lobe, high gain (~15-30)
 * - Phased array:     Very narrow beam, very high gain (~100-500)
 * - Broken:           Random nulls and sidelobes (fault condition)
 *
 * The radial auto-scaler must handle:
 * - Omnidirectional (max ~1) → Phased array (max ~500): 500x range change
 * - Sidelobes that are 20-40 dB below the main lobe
 * - Patterns with deep nulls (radius → 0)
 * - Sudden transitions between pattern types
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch how the scale handles the omnidirectional → phased array jump
 * - Note whether sidelobes are visible or compressed at the bottom
 * - Compare linear vs log radial scaling behavior
 */

#include <ViewPoint.h>
using namespace viewpoint;

#define NUM_POINTS  360   // 1-degree resolution

enum PatternType {
    OMNI,
    DIPOLE,
    CARDIOID,
    YAGI,
    PHASED_ARRAY,
    BROKEN,
    NUM_PATTERNS
};

PatternType currentPattern = OMNI;
PatternType targetPattern = DIPOLE;
float morphProgress = 0.0;   // 0 = current, 1 = target

float currentGain[NUM_POINTS];
float targetGain[NUM_POINTS];

float mainLobeDir = 0.0;     // Main lobe direction (degrees)
float mainLobeDirTarget = 0.0;

unsigned long frameCount = 0;
unsigned long nextPatternChange = 0;
unsigned long nextSteerChange = 0;

void computePattern(float* gain, PatternType type, float mainDir, float peakGain);
float patternValue(PatternType type, float angleDeg, float mainDir);

void setup() {
    randomSeed(analogRead(0));
    view.begin(PlotType::Polar, Mode::Frames, NUM_POINTS);
    view.setDelay(30);

    view.setAngularOffset(90);
    view.setAngularStep(30);
    view.setPlotTitle("Antenna Pattern Test");

    view.trace("Gain").setColor(colors::Gold);

    // Initialize with omnidirectional
    computePattern(currentGain, OMNI, 0, 1.0);
    computePattern(targetGain, DIPOLE, 0, 2.15);

    nextPatternChange = millis() + 4000 + random(4000);
    nextSteerChange = millis() + 2000 + random(3000);
}

void loop() {
    unsigned long now = millis();

    // Pattern change
    if (now >= nextPatternChange) {
        // Current becomes wherever we are in the morph
        for (int i = 0; i < NUM_POINTS; i++) {
            currentGain[i] = currentGain[i] + morphProgress * (targetGain[i] - currentGain[i]);
        }
        morphProgress = 0.0;

        currentPattern = targetPattern;
        targetPattern = (PatternType)random(NUM_PATTERNS);

        // Compute new target with random peak gain
        float peakGain;
        switch (targetPattern) {
            case OMNI:          peakGain = 0.8 + random(40) / 100.0;          break;
            case DIPOLE:        peakGain = 1.5 + random(200) / 100.0;         break;
            case CARDIOID:      peakGain = 3.0 + random(700) / 100.0;         break;
            case YAGI:          peakGain = 10.0 + random(40);                  break;
            case PHASED_ARRAY:  peakGain = 50.0 + random(450);                break;
            case BROKEN:        peakGain = 2.0 + random(2000) / 100.0;        break;
            default:            peakGain = 1.0;
        }

        computePattern(targetGain, targetPattern, mainLobeDirTarget, peakGain);

        nextPatternChange = now + 3000 + random(7000);
    }

    // Beam steering
    if (now >= nextSteerChange) {
        mainLobeDirTarget += (random(2) == 0 ? 1 : -1) * (30 + random(90));
        if (mainLobeDirTarget < 0) mainLobeDirTarget += 360;
        if (mainLobeDirTarget >= 360) mainLobeDirTarget -= 360;

        // Recompute target pattern at new direction
        float peakGain;
        switch (targetPattern) {
            case OMNI:          peakGain = 0.8 + random(40) / 100.0;          break;
            case DIPOLE:        peakGain = 1.5 + random(200) / 100.0;         break;
            case CARDIOID:      peakGain = 3.0 + random(700) / 100.0;         break;
            case YAGI:          peakGain = 10.0 + random(40);                  break;
            case PHASED_ARRAY:  peakGain = 50.0 + random(450);                break;
            case BROKEN:        peakGain = 2.0 + random(2000) / 100.0;        break;
            default:            peakGain = 1.0;
        }
        computePattern(targetGain, targetPattern, mainLobeDirTarget, peakGain);

        nextSteerChange = now + 1500 + random(4000);
    }

    // Advance morph
    morphProgress += 0.02 + random(30) / 1000.0;
    if (morphProgress > 1.0) morphProgress = 1.0;

    // Interpolate and send
    for (int i = 0; i < NUM_POINTS; i++) {
        float gain = currentGain[i] + morphProgress * (targetGain[i] - currentGain[i]);
        // Add measurement jitter
        gain += gain * 0.02 * (random(2000) - 1000) / 1000.0;
        gain = max(0.0f, gain);

        float angle = (float)i;
        view.addData("Gain", constrain(gain, 0.0f, 1e6f), angle);
    }

    view.send();
    frameCount++;
}

void computePattern(float* gain, PatternType type, float mainDir, float peakGain) {
    for (int i = 0; i < NUM_POINTS; i++) {
        float angle = (float)i;
        gain[i] = peakGain * patternValue(type, angle, mainDir);
    }
}

float patternValue(PatternType type, float angleDeg, float mainDir) {
    float theta = (angleDeg - mainDir) * M_PI / 180.0;

    switch (type) {
        case OMNI:
            // Nearly uniform with slight ripple
            return 0.9 + 0.1 * cos(4.0 * theta);

        case DIPOLE:
            // Figure-8: cos^2(theta)
            return max(0.0f, (float)(cos(theta) * cos(theta)));

        case CARDIOID:
            // Cardioid: (1 + cos(theta)) / 2
            return max(0.0f, (float)((1.0 + cos(theta)) / 2.0));

        case YAGI: {
            // Narrow main lobe with sidelobes
            float mainLobe = pow(max(0.0f, (float)cos(theta)), 8);
            float sidelobes = 0.05 * pow(max(0.0f, (float)cos(3.0 * theta)), 4);
            float backLobe = 0.02 * pow(max(0.0f, (float)cos(theta + M_PI)), 2);
            return mainLobe + sidelobes + backLobe;
        }

        case PHASED_ARRAY: {
            // Very narrow beam
            float mainLobe = pow(max(0.0f, (float)cos(theta)), 32);
            float sidelobes = 0.01 * (0.5 + 0.5 * cos(8.0 * theta));
            return mainLobe + sidelobes;
        }

        case BROKEN: {
            // Random pattern with holes
            // Use a deterministic "random" based on angle
            float base = 0.3 + 0.7 * abs(sin(3.0 * theta + 1.5));
            float nulls = max(0.0f, (float)(1.0 - 2.0 * pow(sin(5.0 * theta), 8)));
            return base * nulls;
        }

        default:
            return 1.0;
    }
}
