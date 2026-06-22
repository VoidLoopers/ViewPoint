/**
 * ScatterMultiTarget.ino
 * @date 03-10-26
 *
 * ViewPoint Auto-Scale Test: Scatter — Multi-Target Tracker
 *
 * Simulates a radar/sonar tracking display where multiple independent
 * targets move through 2D space. Targets appear, maneuver, cross paths,
 * and disappear. False alarm clutter bursts randomly across the field.
 *
 * Each target has its own kinematics: linear drift, circular orbit,
 * evasive jinking, or stationary hover. The auto-scaler must encompass
 * all active targets while handling sudden appearances at the field edge
 * and disappearances that should allow scale contraction.
 *
 * Auto-scale challenges:
 * - A new target appearing at (500, 500) when all others are near origin
 * - All targets converging then one breaks away at high speed
 * - False alarms creating momentary outliers in random quadrants
 * - Targets at vastly different ranges (10 vs 10000 units)
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch scale when a distant target spawns at the field edge
 * - Notice how false alarm clutter spikes affect different strategies
 * - Compare ROBUST_PERCENTILE (ignores clutter) vs EXPAND_ONLY (traps it)
 * - Observe scale contraction when distant targets are dropped
 */

#include <ViewPoint.h>
using namespace viewpoint;

#define MAX_TARGETS 5

struct Target {
    float x, y;           // Position
    float vx, vy;         // Velocity
    float ax, ay;         // Acceleration (for maneuvers)
    bool active;
    unsigned long spawnTime;
    unsigned long maneuverTime;
    int behavior;         // 0=linear, 1=orbit, 2=jink, 3=hover
    float orbitRadius;
    float orbitPhase;
    float orbitRate;
};

Target targets[MAX_TARGETS];

// Clutter state
float clutterIntensity = 0.0;
float targetClutterIntensity = 0.0;
unsigned long nextClutterChange = 0;

// Target management
unsigned long nextSpawnCheck = 0;
unsigned long sampleCount = 0;

const char* traceNames[] = {"Tgt-1", "Tgt-2", "Tgt-3", "Tgt-4", "Tgt-5"};
const uint32_t traceColors[] = {
    colors::Crimson, colors::DodgerBlue, colors::LimeGreen,
    colors::Gold, colors::HotPink
};

void spawnTarget(int idx);
void updateTarget(Target& t);
void assignManeuver(Target& t);

void setup() {
    randomSeed(analogRead(0));
    view.begin(PlotType::Scatter, Mode::Continuous);
    view.setDelay(3);

    view.addHorizontalReferenceLine(0.0);
    view.addVerticalReferenceLine(0.0);

    view.setAxisLabels("East (m)", "North (m)");
    view.setPlotTitle("Multi-Target Tracker");

    for (int i = 0; i < MAX_TARGETS; i++) {
        view.trace(traceNames[i]).setColor(traceColors[i]);
        targets[i].active = false;
    }
    view.trace("Clutter").setColor(colors::DimGray);

    // Start with 2 targets
    spawnTarget(0);
    spawnTarget(1);

    nextSpawnCheck = millis() + 3000;
    nextClutterChange = millis() + 2000;
}

void loop() {
    unsigned long now = millis();

    // Manage target population
    if (now >= nextSpawnCheck) {
        int activeCount = 0;
        for (int i = 0; i < MAX_TARGETS; i++) {
            if (targets[i].active) activeCount++;
        }

        // Spawn or kill targets
        int action = random(100);
        if (activeCount < 2 || (activeCount < MAX_TARGETS && action < 40)) {
            for (int i = 0; i < MAX_TARGETS; i++) {
                if (!targets[i].active) {
                    spawnTarget(i);
                    break;
                }
            }
        } else if (activeCount > 2 && action > 80) {
            // Kill a random active target
            int victim = random(MAX_TARGETS);
            for (int tries = 0; tries < MAX_TARGETS; tries++) {
                if (targets[victim].active) {
                    targets[victim].active = false;
                    view.addBreak(traceNames[victim]);  // End track cleanly
                    break;
                }
                victim = (victim + 1) % MAX_TARGETS;
            }
        }
        nextSpawnCheck = now + 2000 + random(4000);
    }

    // Clutter intensity changes
    if (now >= nextClutterChange) {
        int mode = random(4);
        if (mode == 0) targetClutterIntensity = 0.0;   // Clear
        else if (mode == 1) targetClutterIntensity = 0.3;  // Light
        else if (mode == 2) targetClutterIntensity = 0.7;  // Heavy
        else targetClutterIntensity = 1.0;                 // Storm
        nextClutterChange = now + 1500 + random(4000);
    }
    clutterIntensity += (targetClutterIntensity - clutterIntensity) * 0.01;

    // Update and emit target positions
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (!targets[i].active) continue;
        updateTarget(targets[i]);

        // Measurement noise proportional to range
        float range = sqrt(targets[i].x * targets[i].x + targets[i].y * targets[i].y);
        float noise = max(1.0f, range * 0.005f);
        float nx = targets[i].x + (random(2000) - 1000) / 1000.0 * noise;
        float ny = targets[i].y + (random(2000) - 1000) / 1000.0 * noise;

        view.addData(traceNames[i], constrain(nx, -1e6f, 1e6f), constrain(ny, -1e6f, 1e6f));
    }

    // Emit clutter returns
    if (random(100) < (int)(clutterIntensity * 60)) {
        // Clutter at random locations, biased toward active target region
        float clutterRange = 100.0 + random(10000) / 10.0;
        float angle = random(6283) / 1000.0;
        float cx = clutterRange * cos(angle);
        float cy = clutterRange * sin(angle);
        view.addData("Clutter", constrain(cx, -1e6f, 1e6f), constrain(cy, -1e6f, 1e6f));
    }

    view.send();
    sampleCount++;
}

void spawnTarget(int idx) {
    Target& t = targets[idx];
    view.addBreak(traceNames[idx]);  // Disconnect from previous track
    t.active = true;
    t.spawnTime = millis();

    // Spawn at field edge or near origin
    int spawnType = random(4);
    float dist;
    float angle = random(6283) / 1000.0;
    if (spawnType == 0) dist = 10.0 + random(50);             // Near
    else if (spawnType == 1) dist = 100.0 + random(400);      // Medium
    else if (spawnType == 2) dist = 500.0 + random(2000);     // Far
    else dist = 5000.0 + random(5000);                         // Very far (scale buster)

    t.x = dist * cos(angle);
    t.y = dist * sin(angle);

    // Initial velocity toward general center
    float speed = 1.0 + random(2000) / 100.0;
    float heading = atan2(-t.y, -t.x) + (random(2000) - 1000) / 1000.0 * 0.8;
    t.vx = speed * cos(heading);
    t.vy = speed * sin(heading);
    t.ax = 0;
    t.ay = 0;

    t.behavior = random(4);
    t.orbitRadius = 20.0 + random(500);
    t.orbitPhase = random(6283) / 1000.0;
    t.orbitRate = 0.005 + random(50) / 1000.0;
    t.maneuverTime = millis() + 2000 + random(5000);
}

void updateTarget(Target& t) {
    unsigned long now = millis();

    // Reassign maneuver periodically
    if (now >= t.maneuverTime) {
        assignManeuver(t);
        t.maneuverTime = now + 1500 + random(5000);
    }

    switch (t.behavior) {
        case 0:  // Linear drift
            t.x += t.vx;
            t.y += t.vy;
            break;

        case 1: {  // Circular orbit
            t.orbitPhase += t.orbitRate;
            if (t.orbitPhase > 2.0 * M_PI) t.orbitPhase -= 2.0 * M_PI;
            // Orbit around current nominal position
            float cx = t.x + t.orbitRadius * cos(t.orbitPhase) * 0.01;
            float cy = t.y + t.orbitRadius * sin(t.orbitPhase) * 0.01;
            t.x = cx;
            t.y = cy;
            // Slow drift
            t.x += t.vx * 0.1;
            t.y += t.vy * 0.1;
            break;
        }

        case 2:  // Evasive jinking
            t.ax = (random(2000) - 1000) / 100.0;
            t.ay = (random(2000) - 1000) / 100.0;
            t.vx += t.ax * 0.01;
            t.vy += t.ay * 0.01;
            t.vx = constrain(t.vx, -50.0f, 50.0f);
            t.vy = constrain(t.vy, -50.0f, 50.0f);
            t.x += t.vx;
            t.y += t.vy;
            break;

        case 3:  // Hover (nearly stationary)
            t.x += (random(200) - 100) / 100.0;
            t.y += (random(200) - 100) / 100.0;
            break;
    }
}

void assignManeuver(Target& t) {
    t.behavior = random(4);
    if (t.behavior == 1) {
        t.orbitRadius = 10.0 + random(300);
        t.orbitRate = 0.005 + random(40) / 1000.0;
    }
    // Possible velocity change
    if (random(100) < 40) {
        float speed = 1.0 + random(3000) / 100.0;
        float heading = random(6283) / 1000.0;
        t.vx = speed * cos(heading);
        t.vy = speed * sin(heading);
    }
}
