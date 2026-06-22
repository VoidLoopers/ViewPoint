/**
 * ScatterPhaseWarp.ino
 * @date 03-08-26
 *
 * ViewPoint Auto-Scale Test: Scatter — Morphing Parametric Curves (Frames)
 *
 * Generates Lissajous-family parametric curves that morph between shapes
 * each frame. The frequency ratio, amplitude ratio, phase offset, and
 * rotation angle all change randomly, causing the curve to expand,
 * contract, rotate, and reshape unpredictably.
 *
 * Auto-scale challenges:
 * - X and Y amplitudes change independently (asymmetric ranges)
 * - When fx ≈ fy the curve fills a square; when fx/fy is rational it
 *   collapses to a closed figure that may not fill the frame
 * - Rotation mixes X and Y extents so both axes see the larger dimension
 * - Occasional "zoom" frames where amplitude drops 100x or jumps 100x
 *
 * Real-world analogy: Oscilloscope XY mode with two unknown signals.
 * The phase relationship and amplitude ratio reveal frequency coupling
 * and is a classic test/measurement visualization.
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch how scale responds when the curve collapses to a line
 * - Note frame-to-frame jumps when amplitudes change by 100x
 * - Compare EXPAND_ONLY vs PEAK_DECAY behavior
 */

#include <ViewPoint.h>
using namespace viewpoint;

#define FRAME_SIZE 500

// Parametric curve parameters
float fx = 3.0;
float fy = 2.0;
float ampX = 10.0;
float ampY = 10.0;
float delta = M_PI / 2.0;  // Phase offset
float rotation = 0.0;       // In-plane rotation angle

// Targets
float targetFx = 3.0;
float targetFy = 2.0;
float targetAmpX = 10.0;
float targetAmpY = 10.0;
float targetDelta = M_PI / 2.0;
float targetRotation = 0.0;

unsigned long frameCount = 0;
unsigned long nextMajorChange = 0;

// Common frequency ratios that produce interesting Lissajous figures
const float ratios[][2] = {
    {1, 1}, {1, 2}, {2, 3}, {3, 4}, {3, 5},
    {4, 5}, {5, 6}, {1, 3}, {2, 5}, {3, 7}
};
const int NUM_RATIOS = 10;

void setup() {
    randomSeed(analogRead(0));
    view.begin(PlotType::Scatter, Mode::Frames);
    view.setDelay(30);

    view.addHorizontalReferenceLine(0.0);
    view.addVerticalReferenceLine(0.0);

    view.setAxisLabels("X", "Y");
    view.setPlotTitle("Phase Warp Test");

    view.trace("Curve").setColor(colors::SpringGreen);

    nextMajorChange = 0;
}

void loop() {
    // Morph parameters between frames
    if (frameCount >= nextMajorChange) {
        int change = random(7);
        switch (change) {
            case 0: {
                // New frequency ratio
                int r = random(NUM_RATIOS);
                float baseFreq = 1.0 + random(400) / 100.0;
                targetFx = baseFreq * ratios[r][0];
                targetFy = baseFreq * ratios[r][1];
                break;
            }
            case 1:
                // Amplitude explosion
                targetAmpX = 100.0 + random(9900);
                targetAmpY = 100.0 + random(9900);
                break;
            case 2:
                // Amplitude collapse
                targetAmpX = 0.01 + random(100) / 1000.0;
                targetAmpY = 0.01 + random(100) / 1000.0;
                break;
            case 3:
                // Asymmetric: X large, Y small
                targetAmpX = 500.0 + random(4500);
                targetAmpY = 0.5 + random(500) / 100.0;
                break;
            case 4:
                // Asymmetric: Y large, X small
                targetAmpX = 0.5 + random(500) / 100.0;
                targetAmpY = 500.0 + random(4500);
                break;
            case 5:
                // Phase shift (changes curve shape)
                targetDelta = random(628) / 100.0;  // 0 to 2π
                break;
            case 6:
                // Rotation change
                targetRotation = random(628) / 100.0;
                break;
        }

        nextMajorChange = frameCount + 3 + random(15);

        // 20% chance of instant snap instead of gradual morph
        if (random(100) < 20) {
            fx = targetFx;
            fy = targetFy;
            ampX = targetAmpX;
            ampY = targetAmpY;
            delta = targetDelta;
            rotation = targetRotation;
        }
    }

    // Gradual morph toward targets
    fx += (targetFx - fx) * 0.1;
    fy += (targetFy - fy) * 0.1;
    ampX += (targetAmpX - ampX) * 0.05;
    ampY += (targetAmpY - ampY) * 0.05;
    delta += (targetDelta - delta) * 0.08;
    rotation += (targetRotation - rotation) * 0.03;

    float ca = cos(rotation);
    float sa = sin(rotation);
    float step = 2.0 * M_PI / FRAME_SIZE;

    // Generate frame
    for (int i = 0; i < FRAME_SIZE; i++) {
        float param = i * step;
        float rawX = ampX * sin(fx * param + delta);
        float rawY = ampY * sin(fy * param);

        // Apply rotation
        float x = rawX * ca - rawY * sa;
        float y = rawX * sa + rawY * ca;

        view.addData("Curve", constrain(x, -1e6f, 1e6f), constrain(y, -1e6f, 1e6f));
    }

    view.send();
    frameCount++;
}
