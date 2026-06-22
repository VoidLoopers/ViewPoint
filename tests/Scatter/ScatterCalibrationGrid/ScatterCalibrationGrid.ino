/**
 * ScatterCalibrationGrid.ino
 * @date 03-10-26
 *
 * ViewPoint Scatter Test: Frames — Warping Calibration Grid
 *
 * Draws a precision calibration test pattern using scatter lines and
 * addBreak() to create a complex multi-segment image:
 *
 *   - Horizontal and vertical grid lines
 *   - A center crosshair with tick marks
 *   - Corner fiducial L-brackets
 *   - A circle (approximated as polygon) inscribed in the grid
 *
 * The pattern then evolves through distortion scenarios that simulate
 * real optical/mechanical alignment problems:
 *
 * - PRISTINE:       Perfect rectangular grid, centered
 * - BARREL:         Barrel distortion (pincushion if negative)
 * - TRAPEZOID:      Keystone/perspective distortion
 * - ROTATED:        Whole pattern rotated off-axis
 * - SHIFTED:        Grid translated far from origin
 * - SCALED:         Grid size changes dramatically (0.1x to 100x)
 * - SHEARED:        Parallelogram distortion (non-orthogonal axes)
 *
 * Every line segment in the grid is drawn as two points separated by
 * addBreak(), so the renderer sees dozens of independent disconnected
 * segments that together form a recognizable calibration target.
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch the grid warp through barrel distortion
 * - See if the inscribed circle stays smooth under distortion
 * - Notice scale jumps when grid size changes 100x
 * - Observe how the structured geometry stresses auto-scale differently
 *   than organic data (every point is meaningful, no outliers)
 */

#include <ViewPoint.h>
using namespace viewpoint;

// Grid parameters
#define GRID_LINES 7        // Lines per axis (odd = center line exists)
#define CIRCLE_SEGMENTS 32  // Points in inscribed circle
#define TICK_LENGTH 0.05    // Crosshair tick length (fraction of grid)

// Distortion state
float gridSize = 20.0;
float gridCenterX = 0.0;
float gridCenterY = 0.0;
float rotation = 0.0;
float barrelK = 0.0;        // Barrel distortion coefficient
float trapezoidK = 0.0;     // Keystone factor
float shearK = 0.0;         // Shear factor

// Targets
float targetSize = 20.0;
float targetCenterX = 0.0;
float targetCenterY = 0.0;
float targetRotation = 0.0;
float targetBarrelK = 0.0;
float targetTrapezoidK = 0.0;
float targetShearK = 0.0;

unsigned long frameCount = 0;
unsigned long nextDistortion = 0;

enum Distortion : uint8_t {
    PRISTINE,
    BARREL,
    TRAPEZOID,
    ROTATED,
    SHIFTED,
    SCALED,
    SHEARED,
    NUM_DISTORTIONS
};

void pickDistortion();
void distort(float nx, float ny, float& ox, float& oy);
void drawLine(float x0, float y0, float x1, float y1, const char* trace, int segments = 1);

void setup() {
    randomSeed(analogRead(0));
    view.begin(PlotType::Scatter, Mode::Frames);
    view.setDelay(40);

    view.addHorizontalReferenceLine(0.0);
    view.addVerticalReferenceLine(0.0);

    view.setAxisLabels("X", "Y");
    view.setPlotTitle("Calibration Grid");

    view.trace("Grid").setColor(colors::DodgerBlue);
    view.trace("Circle").setColor(colors::LimeGreen);
    view.trace("Crosshair").setColor(colors::Crimson);
    view.trace("Fiducial").setColor(colors::Gold);

    nextDistortion = 0;
}

void loop() {
    if (frameCount >= nextDistortion) {
        pickDistortion();
        nextDistortion = frameCount + 8 + random(20);
    }

    // Drift toward targets
    gridSize += (targetSize - gridSize) * 0.04;
    gridCenterX += (targetCenterX - gridCenterX) * 0.03;
    gridCenterY += (targetCenterY - gridCenterY) * 0.03;
    rotation += (targetRotation - rotation) * 0.03;
    barrelK += (targetBarrelK - barrelK) * 0.04;
    trapezoidK += (targetTrapezoidK - trapezoidK) * 0.04;
    shearK += (targetShearK - shearK) * 0.04;

    float half = 0.5;  // Normalized half-extent
    float step = 1.0 / (GRID_LINES - 1);

    // --- Horizontal grid lines ---
    for (int i = 0; i < GRID_LINES; i++) {
        float ny = -half + i * step;
        drawLine(-half, ny, half, ny, "Grid", 20);
    }

    // --- Vertical grid lines ---
    for (int i = 0; i < GRID_LINES; i++) {
        float nx = -half + i * step;
        drawLine(nx, -half, nx, half, "Grid", 20);
    }

    // --- Inscribed circle ---
    float radius = half * 0.9;
    for (int i = 0; i < CIRCLE_SEGMENTS; i++) {
        float a0 = 2.0 * M_PI * i / CIRCLE_SEGMENTS;
        float a1 = 2.0 * M_PI * (i + 1) / CIRCLE_SEGMENTS;
        float nx0 = radius * cos(a0);
        float ny0 = radius * sin(a0);
        float nx1 = radius * cos(a1);
        float ny1 = radius * sin(a1);

        float ox0, oy0, ox1, oy1;
        distort(nx0, ny0, ox0, oy0);
        distort(nx1, ny1, ox1, oy1);

        view.addData("Circle", ox0, oy0);
        view.addData("Circle", ox1, oy1);
        view.addBreak("Circle");
    }

    // --- Center crosshair with ticks ---
    float tickLen = TICK_LENGTH;
    // Horizontal arm
    drawLine(-half * 0.8, 0, half * 0.8, 0, "Crosshair", 16);
    // Vertical arm
    drawLine(0, -half * 0.8, 0, half * 0.8, "Crosshair", 16);

    // Tick marks along horizontal
    for (int i = 1; i < GRID_LINES - 1; i++) {
        float nx = -half + i * step;
        drawLine(nx, -tickLen, nx, tickLen, "Crosshair");
    }
    // Tick marks along vertical
    for (int i = 1; i < GRID_LINES - 1; i++) {
        float ny = -half + i * step;
        drawLine(-tickLen, ny, tickLen, ny, "Crosshair");
    }

    // --- Corner fiducial L-brackets ---
    float bLen = 0.08;
    float corners[][2] = {{-half, -half}, {half, -half}, {half, half}, {-half, half}};
    float dirs[][4] = {
        { 1, 0, 0, 1},   // Bottom-left: right and up
        {-1, 0, 0, 1},   // Bottom-right: left and up
        {-1, 0, 0,-1},   // Top-right: left and down
        { 1, 0, 0,-1}    // Top-left: right and down
    };
    for (int c = 0; c < 4; c++) {
        float cx = corners[c][0];
        float cy = corners[c][1];
        // Horizontal arm of L
        drawLine(cx, cy, cx + dirs[c][0] * bLen, cy + dirs[c][1] * bLen, "Fiducial");
        // Vertical arm of L
        drawLine(cx, cy, cx + dirs[c][2] * bLen, cy + dirs[c][3] * bLen, "Fiducial");
    }

    view.send();
    frameCount++;
}

void distort(float nx, float ny, float& ox, float& oy) {
    // Apply shear
    float sx = nx + shearK * ny;
    float sy = ny;

    // Apply barrel distortion (radial)
    float r2 = sx * sx + sy * sy;
    float barrelFactor = 1.0 + barrelK * r2;
    float bx = sx * barrelFactor;
    float by = sy * barrelFactor;

    // Apply trapezoid (perspective: top wider/narrower)
    float perspScale = 1.0 + trapezoidK * by;
    float tx = bx * perspScale;
    float ty = by;

    // Apply rotation
    float cr = cos(rotation);
    float sr = sin(rotation);
    float rx = tx * cr - ty * sr;
    float ry = tx * sr + ty * cr;

    // Apply scale and translation
    ox = rx * gridSize + gridCenterX;
    oy = ry * gridSize + gridCenterY;
}

void drawLine(float x0, float y0, float x1, float y1, const char* trace, int segments) {
    // Draw a line as 'segments' sub-segments (important for curved distortion)
    for (int s = 0; s < segments; s++) {
        float t0 = (float)s / segments;
        float t1 = (float)(s + 1) / segments;

        float nx0 = x0 + (x1 - x0) * t0;
        float ny0 = y0 + (y1 - y0) * t0;
        float nx1 = x0 + (x1 - x0) * t1;
        float ny1 = y0 + (y1 - y0) * t1;

        float ox0, oy0, ox1, oy1;
        distort(nx0, ny0, ox0, oy0);
        distort(nx1, ny1, ox1, oy1);

        view.addData(trace, ox0, oy0);
        view.addData(trace, ox1, oy1);
        view.addBreak(trace);
    }
}

void pickDistortion() {
    Distortion d = (Distortion)random(NUM_DISTORTIONS);

    // Reset distortions unless specifically set
    targetBarrelK = 0.0;
    targetTrapezoidK = 0.0;
    targetShearK = 0.0;

    switch (d) {
        case PRISTINE:
            targetSize = 15.0 + random(2000) / 100.0;
            targetCenterX = (random(200) - 100) / 50.0;
            targetCenterY = (random(200) - 100) / 50.0;
            targetRotation = 0.0;
            break;

        case BARREL:
            targetBarrelK = (random(2) == 0 ? 1 : -1) * (0.5 + random(400) / 100.0);
            break;

        case TRAPEZOID:
            targetTrapezoidK = (random(2) == 0 ? 1 : -1) * (0.2 + random(150) / 100.0);
            break;

        case ROTATED:
            targetRotation = (random(2) == 0 ? 1 : -1) * (0.1 + random(150) / 100.0);
            break;

        case SHIFTED:
            targetCenterX = (random(2) == 0 ? 1 : -1) * (20.0 + random(30000) / 100.0);
            targetCenterY = (random(2) == 0 ? 1 : -1) * (20.0 + random(30000) / 100.0);
            break;

        case SCALED:
            // Dramatic scale change
            if (random(2) == 0) {
                targetSize = 0.5 + random(300) / 100.0;      // Tiny
            } else {
                targetSize = 100.0 + random(90000) / 100.0;  // Huge
            }
            // 20% chance of instant snap
            if (random(100) < 20) {
                gridSize = targetSize;
            }
            break;

        case SHEARED:
            targetShearK = (random(2) == 0 ? 1 : -1) * (0.1 + random(80) / 100.0);
            // Often combine with slight rotation
            targetRotation = (random(200) - 100) / 200.0;
            break;
    }
}
