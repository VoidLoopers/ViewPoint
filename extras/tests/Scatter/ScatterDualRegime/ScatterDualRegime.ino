/**
 * ScatterDualRegime.ino
 * @date 03-10-26
 *
 * ViewPoint Auto-Scale Test: Scatter — Alternating Continuous & Frames Behavior
 *
 * Tests the plotter's ability to handle a scatter stream that alternates
 * between two distinct data regimes within Continuous mode:
 *
 * STREAMING: Slow, point-by-point orbit/trajectory data (1 sample per send).
 *   The path evolves smoothly as a damped oscillator, spiral, or random walk.
 *
 * BURST: Rapid multi-point clouds fired as clusters of addData calls between
 *   sends, flooding the plotter with dense scatter data (simulating a sensor
 *   that buffers then dumps). Each burst contains 50-200 points.
 *
 * The mode transitions test how the auto-scaler handles abrupt changes in
 * data density and spatial distribution. A streaming orbit near (-5, 3) that
 * suddenly receives a 200-point burst at (400, -800) is the worst case.
 *
 * - SPIRAL: Expanding/contracting spiral trajectory, single points
 * - RANDOM_WALK: Brownian motion path, single points
 * - ORBIT: Elliptical orbit with precession, single points
 * - BURST_TIGHT: Dense cluster near current position
 * - BURST_SCATTER: Wide cloud at random location
 * - BURST_RING: Ring of points at large radius (annular detection)
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch scale jump when BURST_SCATTER fires far from the streaming region
 * - Notice how BURST_RING creates a hollow pattern the auto-scaler must bound
 * - Compare DEBOUNCED (may miss fast bursts) vs PEAK_DECAY (captures then fades)
 * - Observe persistence trails: streaming path vs burst cloud overlay
 */

#include <ViewPoint.h>
using namespace viewpoint;

// Current position (carried across all regimes)
float posX = 0.0;
float posY = 0.0;

// Streaming parameters
float spiralRadius = 5.0;
float spiralPhase = 0.0;
float spiralGrowth = 0.1;
float walkStepSize = 0.5;
float orbitA = 8.0;
float orbitB = 5.0;
float orbitPhase = 0.0;
float orbitPrecession = 0.01;
float precessionAngle = 0.0;

// Regime control
enum Regime : uint8_t {
    SPIRAL,
    RANDOM_WALK,
    ORBIT,
    BURST_TIGHT,
    BURST_SCATTER,
    BURST_RING,
    NUM_REGIMES
};

Regime currentRegime = SPIRAL;
unsigned long regimeStart = 0;
unsigned long regimeDuration = 5000;
unsigned long burstCooldown = 0;

// Drift targets for parameters
float targetSpiralGrowth = 0.1;
float targetWalkStep = 0.5;
float targetOrbitA = 8.0;
float targetOrbitB = 5.0;

void enterRegime(Regime r);
void streamingStep();
void fireBurst();

void setup() {
    randomSeed(analogRead(0));
    view.begin(PlotType::Scatter, Mode::Continuous);
    view.setDisplayMode(DisplayMode::Persistence);
    view.setDelay(2);

    view.addHorizontalReferenceLine(0.0);
    view.addVerticalReferenceLine(0.0);

    view.setAxisLabels("X", "Y");
    view.setPlotTitle("Dual Regime Test");

    view.trace("Stream").setColor(colors::Cyan);
    view.trace("Burst").setColor(colors::OrangeRed);

    enterRegime(SPIRAL);
}

void loop() {
    unsigned long now = millis();

    // Regime transitions
    if (now - regimeStart >= regimeDuration) {
        // Streaming regimes are more common, bursts are brief
        int pick = random(100);
        Regime next;
        if (pick < 25) next = SPIRAL;
        else if (pick < 45) next = RANDOM_WALK;
        else if (pick < 65) next = ORBIT;
        else if (pick < 78) next = BURST_TIGHT;
        else if (pick < 90) next = BURST_SCATTER;
        else next = BURST_RING;
        enterRegime(next);
    }

    // Parameter drift
    spiralGrowth += (targetSpiralGrowth - spiralGrowth) * 0.005;
    walkStepSize += (targetWalkStep - walkStepSize) * 0.01;
    orbitA += (targetOrbitA - orbitA) * 0.003;
    orbitB += (targetOrbitB - orbitB) * 0.003;

    if (currentRegime <= ORBIT) {
        streamingStep();
    } else {
        // Burst regimes: fire burst then immediately transition
        if (now >= burstCooldown) {
            fireBurst();
            burstCooldown = now + 200;  // Brief pause between bursts in a regime
        }
    }
}

void enterRegime(Regime r) {
    // Break both traces so new regime doesn't connect to previous path
    view.addBreak("Stream");
    view.addBreak("Burst");

    currentRegime = r;
    regimeStart = millis();
    burstCooldown = millis();

    switch (r) {
        case SPIRAL:
            regimeDuration = 4000 + random(6000);
            spiralRadius = 1.0 + random(2000) / 100.0;
            targetSpiralGrowth = (random(2) == 0 ? 1 : -1) * (0.01 + random(50) / 100.0);
            break;

        case RANDOM_WALK:
            regimeDuration = 3000 + random(5000);
            targetWalkStep = 0.2 + random(3000) / 100.0;
            break;

        case ORBIT:
            regimeDuration = 4000 + random(6000);
            targetOrbitA = 3.0 + random(5000) / 100.0;
            targetOrbitB = 3.0 + random(5000) / 100.0;
            orbitPrecession = 0.005 + random(30) / 1000.0;
            break;

        case BURST_TIGHT:
            regimeDuration = 500 + random(1500);
            break;

        case BURST_SCATTER:
            regimeDuration = 300 + random(1000);
            break;

        case BURST_RING:
            regimeDuration = 400 + random(1200);
            break;
    }
}

void streamingStep() {
    float x, y;

    switch (currentRegime) {
        case SPIRAL:
            spiralPhase += 0.08;
            if (spiralPhase > 2.0 * M_PI) spiralPhase -= 2.0 * M_PI;
            spiralRadius += spiralGrowth;
            spiralRadius = constrain(spiralRadius, 0.5f, 500.0f);
            x = posX + spiralRadius * cos(spiralPhase);
            y = posY + spiralRadius * sin(spiralPhase);
            // Slow center drift
            posX += (random(200) - 100) / 5000.0;
            posY += (random(200) - 100) / 5000.0;
            break;

        case RANDOM_WALK:
            posX += (random(2000) - 1000) / 1000.0 * walkStepSize;
            posY += (random(2000) - 1000) / 1000.0 * walkStepSize;
            x = posX;
            y = posY;
            break;

        case ORBIT:
            orbitPhase += 0.05;
            if (orbitPhase > 2.0 * M_PI) orbitPhase -= 2.0 * M_PI;
            precessionAngle += orbitPrecession;
            if (precessionAngle > 2.0 * M_PI) precessionAngle -= 2.0 * M_PI;
            {
                float rawX = orbitA * cos(orbitPhase);
                float rawY = orbitB * sin(orbitPhase);
                float cp = cos(precessionAngle);
                float sp = sin(precessionAngle);
                x = posX + rawX * cp - rawY * sp;
                y = posY + rawX * sp + rawY * cp;
            }
            // Slow center drift
            posX += (random(200) - 100) / 10000.0;
            posY += (random(200) - 100) / 10000.0;
            break;

        default:
            x = posX;
            y = posY;
            break;
    }

    view.addData("Stream", constrain(x, -1e6f, 1e6f), constrain(y, -1e6f, 1e6f));
    view.send();
}

void fireBurst() {
    int burstSize;
    float cx, cy;

    switch (currentRegime) {
        case BURST_TIGHT:
            // Dense cluster near current streaming position
            burstSize = 50 + random(100);
            cx = posX + (random(2000) - 1000) / 100.0;
            cy = posY + (random(2000) - 1000) / 100.0;
            for (int i = 0; i < burstSize; i++) {
                float bx = cx + (random(2000) - 1000) / 500.0;
                float by = cy + (random(2000) - 1000) / 500.0;
                view.addData("Burst", constrain(bx, -1e6f, 1e6f), constrain(by, -1e6f, 1e6f));
            }
            break;

        case BURST_SCATTER:
            // Wide cloud at random, possibly distant, location
            burstSize = 80 + random(120);
            cx = (random(2) == 0 ? 1 : -1) * (50.0 + random(50000) / 100.0);
            cy = (random(2) == 0 ? 1 : -1) * (50.0 + random(50000) / 100.0);
            for (int i = 0; i < burstSize; i++) {
                float spread = 5.0 + random(5000) / 100.0;
                float bx = cx + (random(2000) - 1000) / 1000.0 * spread;
                float by = cy + (random(2000) - 1000) / 1000.0 * spread;
                view.addData("Burst", constrain(bx, -1e6f, 1e6f), constrain(by, -1e6f, 1e6f));
            }
            break;

        case BURST_RING: {
            // Ring of points at large radius (annular detection pattern)
            burstSize = 60 + random(140);
            float ringRadius = 30.0 + random(30000) / 100.0;
            float ringWidth = ringRadius * 0.05;
            for (int i = 0; i < burstSize; i++) {
                float angle = random(6283) / 1000.0;
                float r = ringRadius + (random(2000) - 1000) / 1000.0 * ringWidth;
                float bx = posX + r * cos(angle);
                float by = posY + r * sin(angle);
                view.addData("Burst", constrain(bx, -1e6f, 1e6f), constrain(by, -1e6f, 1e6f));
            }
            break;
        }

        default:
            break;
    }

    view.send();
}
