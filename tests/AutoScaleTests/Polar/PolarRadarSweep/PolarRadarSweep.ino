/**
 * PolarRadarSweep.ino
 * @date 03-08-26
 *
 * ViewPoint Auto-Scale Test: Polar — Radar with Variable-Range Targets
 *
 * Simulates a rotating radar display with targets that appear and
 * disappear at varying distances. Each frame is a complete 360-degree
 * sweep showing range returns for each angular bin.
 *
 * The radial auto-scaler must adapt when:
 * - All targets are close → small radial scale
 * - A distant target appears → radial scale must expand
 * - Targets disappear → scale may contract
 * - Ground clutter raises the noise floor at short ranges
 * - A target suddenly jumps from 1 km to 100 km
 *
 * Target behaviors:
 * - Patrol: moves in a circle at fixed range
 * - Inbound: approaches from max range to minimum
 * - Outbound: retreats from minimum range to beyond horizon
 * - Stationary: fixed position, varying radar cross-section
 * - Pop-up: suddenly appears at random range, then fades
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch how the radial scale handles a far target appearing
 * - Note the "pop-up" targets that suddenly extend the range
 * - Compare EXPAND_ONLY (never loses a far track) vs WINDOWED (adapts)
 */

#include <ViewPoint.h>
using namespace viewpoint;

#define NUM_BINS     72        // 5-degree angular resolution
#define DEG_PER_BIN  (360.0 / NUM_BINS)

// Target state
struct Target {
    float range;         // Current range (km)
    float bearing;       // Current bearing (degrees)
    float rangeRate;     // km per frame
    float bearingRate;   // degrees per frame
    float rcs;           // Radar cross-section (relative signal strength)
    bool active;
    unsigned long spawnTime;
    unsigned long lifetime;
};

#define MAX_TARGETS 6
Target targets[MAX_TARGETS];

// Noise floor
float noiseFloor = 0.5;       // Baseline noise level
float clutterRange = 10.0;    // Range within which clutter is strong

unsigned long frameCount = 0;
unsigned long nextTargetEvent = 0;
unsigned long nextNoiseChange = 0;

void spawnTarget(int idx);
void updateTargets();
float getRangeReturn(float bearing);

void setup() {
    randomSeed(analogRead(0));
    view.begin(PlotType::Polar, Mode::Frames, NUM_BINS);
    view.setDelay(50);

    view.setAngularOffset(90);
    view.setAngularStep(30);
    view.setPlotTitle("Radar Sweep Test");

    view.trace("Returns").setColor(colors::LimeGreen);

    // Initialize targets
    for (int i = 0; i < MAX_TARGETS; i++) {
        targets[i].active = false;
    }

    // Start with 2 targets
    spawnTarget(0);
    spawnTarget(1);

    nextTargetEvent = millis() + 3000 + random(3000);
    nextNoiseChange = millis() + 5000 + random(5000);
}

void loop() {
    unsigned long now = millis();

    // Target management
    if (now >= nextTargetEvent) {
        int action = random(4);
        switch (action) {
            case 0: {
                // Spawn new target in empty slot
                for (int i = 0; i < MAX_TARGETS; i++) {
                    if (!targets[i].active) {
                        spawnTarget(i);
                        break;
                    }
                }
                break;
            }
            case 1:
                // Kill a random active target
                for (int i = MAX_TARGETS - 1; i >= 0; i--) {
                    if (targets[i].active) {
                        targets[i].active = false;
                        break;
                    }
                }
                break;
            case 2: {
                // Range jump: move a target to a completely different range
                for (int i = 0; i < MAX_TARGETS; i++) {
                    if (targets[i].active) {
                        targets[i].range = 1.0 + pow(10.0, random(400) / 100.0);  // 1 to 10,000
                        targets[i].rangeRate = 0;
                        break;
                    }
                }
                break;
            }
            case 3:
                // Change an active target's behavior
                for (int i = 0; i < MAX_TARGETS; i++) {
                    if (targets[i].active) {
                        targets[i].rangeRate = (random(200) - 100) / 10.0;
                        targets[i].bearingRate = (random(200) - 100) / 50.0;
                        break;
                    }
                }
                break;
        }

        nextTargetEvent = now + 2000 + random(5000);
    }

    // Noise floor changes
    if (now >= nextNoiseChange) {
        noiseFloor = 0.1 + random(300) / 100.0;
        clutterRange = 5.0 + random(50);
        nextNoiseChange = now + 4000 + random(6000);
    }

    updateTargets();

    // Generate sweep frame
    for (int bin = 0; bin < NUM_BINS; bin++) {
        float bearing = bin * DEG_PER_BIN + DEG_PER_BIN * 0.5;
        float rangeReturn = getRangeReturn(bearing);
        view.addData("Returns", rangeReturn, bearing);
    }

    view.send();
    frameCount++;
}

void spawnTarget(int idx) {
    Target& t = targets[idx];
    t.active = true;
    t.bearing = random(360);
    t.spawnTime = millis();
    t.lifetime = 10000 + random(30000);

    // Random target type
    int type = random(5);
    switch (type) {
        case 0:  // Close patrol
            t.range = 5.0 + random(20);
            t.rangeRate = 0;
            t.bearingRate = 0.5 + random(200) / 100.0;
            t.rcs = 5.0 + random(2000) / 100.0;
            break;
        case 1:  // Distant stationary
            t.range = 100.0 + random(900);
            t.rangeRate = 0;
            t.bearingRate = 0;
            t.rcs = 10.0 + random(4000) / 100.0;
            break;
        case 2:  // Inbound
            t.range = 200.0 + random(800);
            t.rangeRate = -(5.0 + random(200) / 10.0);
            t.bearingRate = (random(100) - 50) / 50.0;
            t.rcs = 8.0 + random(3000) / 100.0;
            break;
        case 3:  // Outbound
            t.range = 10.0 + random(50);
            t.rangeRate = 5.0 + random(200) / 10.0;
            t.bearingRate = (random(100) - 50) / 50.0;
            t.rcs = 5.0 + random(2000) / 100.0;
            break;
        case 4:  // Pop-up at extreme range
            t.range = 500.0 + random(9500);  // Up to 10,000 km
            t.rangeRate = 0;
            t.bearingRate = 0;
            t.rcs = 20.0 + random(8000) / 100.0;
            t.lifetime = 3000 + random(5000);  // Short-lived
            break;
    }
}

void updateTargets() {
    unsigned long now = millis();
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (!targets[i].active) continue;

        targets[i].range += targets[i].rangeRate * 0.05;  // Scale by frame rate
        targets[i].bearing += targets[i].bearingRate;

        // Wrap bearing
        if (targets[i].bearing < 0) targets[i].bearing += 360;
        if (targets[i].bearing >= 360) targets[i].bearing -= 360;

        // Kill if range goes negative or lifetime expired
        if (targets[i].range < 0.5) targets[i].active = false;
        if (now - targets[i].spawnTime > targets[i].lifetime) targets[i].active = false;
    }
}

float getRangeReturn(float bearing) {
    float maxReturn = noiseFloor * (random(200) / 100.0);  // Base noise

    // Ground clutter near the radar
    // (not affecting radial auto-scale since it's a short range)

    // Check each target
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (!targets[i].active) continue;

        // Angular proximity (simple beam width model)
        float angleDiff = abs(targets[i].bearing - bearing);
        if (angleDiff > 180) angleDiff = 360 - angleDiff;

        float beamWidth = DEG_PER_BIN * 1.5;
        if (angleDiff < beamWidth) {
            float weight = 1.0 - (angleDiff / beamWidth);
            float ret = targets[i].range * weight * (targets[i].rcs / 10.0);
            if (ret > maxReturn) maxReturn = ret;
        }
    }

    return constrain(maxReturn, 0.0f, 1e6f);
}
