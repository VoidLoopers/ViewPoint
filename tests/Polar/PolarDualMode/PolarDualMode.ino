/**
 * PolarDualMode.ino
 * @date 03-10-26
 *
 * ViewPoint Polar Test: Both Modes — Surveillance Radar Dual-Mode Operation
 *
 * Simulates a surveillance radar that alternates between two operational
 * modes by calling view.begin() to switch between them:
 *
 * SEARCH mode (Continuous):
 *   The antenna sweeps 360 degrees, streaming range returns point-by-point.
 *   Each send() pushes one angular sample. This mode builds up a full
 *   picture over time as the antenna rotates.
 *
 * TRACK mode (Frames):
 *   The radar locks onto a sector and builds a high-resolution frame
 *   of the tracked region. Each send() delivers a complete 360-degree
 *   snapshot with the tracked sector at high resolution and the rest
 *   at background level.
 *
 * The system alternates between modes every 6-12 seconds, exercising
 * ViewPoint's ability to reinitialize mid-run. Each mode has different
 * radial characteristics: Search produces sparse points at varying
 * ranges, while Track produces dense frames with a focused pattern.
 *
 * Targets drift, appear, and disappear independently of mode changes.
 * The auto-scaler sees both streaming and frame-based data over time.
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch the visual transition when switching from Search to Track
 * - Note how the radial scale adapts differently in each mode
 * - See if Track mode's high-resolution sector reveals more detail
 * - Compare how different scaling strategies handle the mode switch
 */

#include <ViewPoint.h>
using namespace viewpoint;

// --- Radar configuration ---
#define SEARCH_BINS      72       // 5-degree resolution in search
#define TRACK_BINS       360      // 1-degree resolution in track
#define TRACK_SECTOR     60.0     // Track sector width (degrees)

enum RadarMode : uint8_t {
    SEARCH,
    TRACK
};

RadarMode radarMode = SEARCH;

// --- Target management ---
struct RadarTarget {
    float range;
    float bearing;
    float rangeRate;
    float bearingRate;
    float rcs;
    bool active;
};

#define MAX_TARGETS 8
RadarTarget targets[MAX_TARGETS];

// --- Search mode state ---
float searchAngle = 0.0;
float searchStepDeg = 360.0 / SEARCH_BINS;

// --- Track mode state ---
float trackCenterBearing = 0.0;   // Center of tracked sector

// --- Environment ---
float noiseFloor = 0.3;
float clutterLevel = 1.0;

// --- Timing ---
unsigned long modeStartTime = 0;
unsigned long modeDuration = 0;
unsigned long nextTargetEvent = 0;
unsigned long nextEnvironmentChange = 0;
unsigned long frameCount = 0;

// --- Forward declarations ---
void enterSearchMode();
void enterTrackMode();
void spawnTarget(int idx);
void updateTargets();
float getReturn(float bearing, float beamWidth);
float findStrongestBearing();

void setup() {
    randomSeed(analogRead(0));

    // Initialize targets
    for (int i = 0; i < MAX_TARGETS; i++) {
        targets[i].active = false;
    }

    // Start with 3 targets
    spawnTarget(0);
    spawnTarget(1);
    spawnTarget(2);

    enterSearchMode();

    nextTargetEvent = millis() + 2000 + random(3000);
    nextEnvironmentChange = millis() + 5000 + random(5000);
}

void loop() {
    unsigned long now = millis();

    // Mode switch
    if (now - modeStartTime >= modeDuration) {
        if (radarMode == SEARCH) {
            enterTrackMode();
        } else {
            enterSearchMode();
        }
    }

    // Target management (independent of mode)
    if (now >= nextTargetEvent) {
        int action = random(5);
        switch (action) {
            case 0: case 1: {
                // Spawn
                for (int i = 0; i < MAX_TARGETS; i++) {
                    if (!targets[i].active) {
                        spawnTarget(i);
                        break;
                    }
                }
                break;
            }
            case 2: {
                // Kill random target
                int idx = random(MAX_TARGETS);
                targets[idx].active = false;
                break;
            }
            case 3: {
                // Range jump
                for (int i = 0; i < MAX_TARGETS; i++) {
                    if (targets[i].active) {
                        targets[i].range = 5.0 + pow(10.0, random(300) / 100.0);
                        break;
                    }
                }
                break;
            }
            case 4: {
                // Change velocity
                for (int i = 0; i < MAX_TARGETS; i++) {
                    if (targets[i].active) {
                        targets[i].rangeRate = (random(200) - 100) / 20.0;
                        targets[i].bearingRate = (random(200) - 100) / 50.0;
                        break;
                    }
                }
                break;
            }
        }
        nextTargetEvent = now + 1500 + random(3000);
    }

    // Environment changes
    if (now >= nextEnvironmentChange) {
        noiseFloor = 0.1 + random(200) / 100.0;
        clutterLevel = 0.5 + random(300) / 100.0;
        nextEnvironmentChange = now + 4000 + random(6000);
    }

    updateTargets();

    // Generate data based on current mode
    if (radarMode == SEARCH) {
        loopSearch();
    } else {
        loopTrack();
    }

    frameCount++;
}

void loopSearch() {
    // One angular sample per call
    float beamWidth = searchStepDeg * 1.2;
    float ret = getReturn(searchAngle, beamWidth);

    view.addData("Radar", constrain(ret, 0.0f, 1e6f), searchAngle);
    view.send();

    searchAngle += searchStepDeg;
    if (searchAngle >= 360.0) searchAngle -= 360.0;
}

void loopTrack() {
    // Full 360-degree frame, but high resolution in tracked sector
    for (int i = 0; i < TRACK_BINS; i++) {
        float angle = (float)i;
        float beamWidth = 1.5;  // Narrow beam in track

        // Check if in tracked sector
        float diff = angle - trackCenterBearing;
        while (diff > 180) diff -= 360;
        while (diff < -180) diff += 360;

        float ret;
        if (abs(diff) < TRACK_SECTOR * 0.5) {
            // High-gain tracked sector
            ret = getReturn(angle, beamWidth) * 1.5;
        } else {
            // Background: low-resolution noise floor
            ret = noiseFloor * (0.5 + random(100) / 100.0);
        }

        view.addData("Radar", constrain(ret, 0.0f, 1e6f), angle);
    }
    view.send();
}

void enterSearchMode() {
    radarMode = SEARCH;
    modeStartTime = millis();
    modeDuration = 6000 + random(6000);

    view.begin(PlotType::Polar, Mode::Continuous);
    view.setDelay(2);
    view.setAngularOffset(90);
    view.setAngularStep(30);
    view.setPlotTitle("Radar — SEARCH Mode");
    view.trace("Radar").setColor(colors::LimeGreen);

    searchAngle = 0.0;
}

void enterTrackMode() {
    radarMode = TRACK;
    modeStartTime = millis();
    modeDuration = 6000 + random(6000);

    // Track the bearing with the strongest return
    trackCenterBearing = findStrongestBearing();

    view.begin(PlotType::Polar, Mode::Frames, TRACK_BINS);
    view.setDelay(40);
    view.setAngularOffset(90);
    view.setAngularStep(30);
    view.setPlotTitle("Radar — TRACK Mode");
    view.trace("Radar").setColor(colors::Gold);
}

void spawnTarget(int idx) {
    RadarTarget& t = targets[idx];
    t.active = true;
    t.bearing = random(360);
    t.rcs = 2.0 + random(5000) / 100.0;

    int type = random(4);
    switch (type) {
        case 0:  // Close, orbiting
            t.range = 5.0 + random(30);
            t.rangeRate = 0;
            t.bearingRate = 0.5 + random(200) / 100.0;
            break;
        case 1:  // Far, stationary
            t.range = 50.0 + random(450);
            t.rangeRate = 0;
            t.bearingRate = 0;
            break;
        case 2:  // Inbound
            t.range = 100.0 + random(400);
            t.rangeRate = -(1.0 + random(100) / 10.0);
            t.bearingRate = (random(100) - 50) / 50.0;
            break;
        case 3:  // Outbound
            t.range = 10.0 + random(50);
            t.rangeRate = 1.0 + random(100) / 10.0;
            t.bearingRate = (random(100) - 50) / 50.0;
            break;
    }
}

void updateTargets() {
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (!targets[i].active) continue;

        targets[i].range += targets[i].rangeRate * 0.05;
        targets[i].bearing += targets[i].bearingRate;

        if (targets[i].bearing < 0) targets[i].bearing += 360;
        if (targets[i].bearing >= 360) targets[i].bearing -= 360;

        if (targets[i].range < 1.0 || targets[i].range > 1000.0) {
            targets[i].active = false;
        }
    }
}

float getReturn(float bearing, float beamWidth) {
    float maxReturn = noiseFloor * (random(200) / 100.0);

    // Ground clutter at close range
    maxReturn += clutterLevel * 0.5 * (random(100) / 100.0);

    for (int i = 0; i < MAX_TARGETS; i++) {
        if (!targets[i].active) continue;

        float angleDiff = abs(targets[i].bearing - bearing);
        if (angleDiff > 180) angleDiff = 360 - angleDiff;

        if (angleDiff < beamWidth) {
            float weight = 1.0 - (angleDiff / beamWidth);
            float ret = targets[i].range * weight * (targets[i].rcs / 5.0);
            // Add scintillation (RCS fluctuation)
            ret *= (0.7 + random(60) / 100.0);
            if (ret > maxReturn) maxReturn = ret;
        }
    }

    return maxReturn;
}

float findStrongestBearing() {
    float bestBearing = 0.0;
    float bestReturn = 0.0;

    for (int i = 0; i < MAX_TARGETS; i++) {
        if (targets[i].active && targets[i].rcs > bestReturn) {
            bestReturn = targets[i].rcs;
            bestBearing = targets[i].bearing;
        }
    }

    return bestBearing;
}
