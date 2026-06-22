/**
 * AutoScaleMultiPlot.ino
 * @date 03-08-26
 *
 * ViewPoint Auto-Scale Stress Test: Multiple Plots, One Trace Each
 *
 * Three independent plots, each with its own trace and distinct scaling
 * behavior. Each plot's auto-scaler operates independently, so this tests
 * whether the plotter correctly isolates per-plot range tracking.
 *
 * Plot behaviors:
 * - Plot 0 "Seismic": Long quiet periods punctuated by violent quakes
 *   that grow in magnitude, then aftershocks taper off.
 * - Plot 1 "Breathing": Smooth amplitude modulation that slowly expands
 *   its envelope, occasionally snapping to a completely different scale.
 * - Plot 2 "Chaos": Logistic-map-inspired signal that transitions between
 *   periodic orbits and full chaos with random parameter perturbation.
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Compare how each plot's auto-scale responds independently
 * - Notice Plot 2 bifurcation transitions (order emerging from chaos)
 * - Watch Plot 0 during a quake vs the long quiet intervals
 */

#include <ViewPoint.h>
using namespace viewpoint;

// ===== Plot 0: Seismic =====
float seismicValue = 0.0;
float seismicMagnitude = 0.0;     // Current quake energy
float seismicDecay = 0.995;       // Per-sample decay rate
unsigned long seismicNextQuake = 0;
bool seismicQuaking = false;
float seismicPhase = 0.0;

// ===== Plot 1: Breathing =====
float breathPhase = 0.0;
float breathEnvelope = 1.0;
float breathEnvTarget = 1.0;
float breathEnvRate = 0.0001;
unsigned long breathNextShift = 0;

// ===== Plot 2: Chaos (logistic map) =====
float chaosX = 0.4;              // State variable [0, 1]
float chaosR = 3.2;              // Control parameter [2.5, 4.0]
float chaosRTarget = 3.5;
float chaosScale = 10.0;         // Output scaling
float chaosOffset = 0.0;
unsigned long chaosNextPerturb = 0;

void updateSeismic();
void updateBreathing();
void updateChaos();

void setup() {
    randomSeed(analogRead(0) ^ (analogRead(2) << 10));
    view.begin(Mode::Continuous);
    view.setDelay(2);

    view.setNumberOfPlots(3);

    // Plot 0: Seismic
    view.plot(0).setTitle("Seismic");
    view.plot(0).setAxisLabels("Time", "Accel");
    view.plot(0).setUnits("s", "g");

    // Plot 1: Breathing
    view.plot(1).setTitle("Breathing");
    view.plot(1).setAxisLabels("Time", "Amplitude");
    view.plot(1).setUnits("s", "V");

    // Plot 2: Chaos
    view.plot(2).setTitle("Chaos");
    view.plot(2).setAxisLabels("Time", "X");
    view.plot(2).setUnits("s", "");

    view.trace("Seismic").setColor(colors::OrangeRed);
    view.trace("Breathing").setColor(colors::MediumSeaGreen);
    view.trace("Chaos").setColor(colors::BlueViolet);

    unsigned long now = millis();
    seismicNextQuake = now + 3000 + random(5000);
    breathNextShift = now + 5000 + random(8000);
    chaosNextPerturb = now + 2000 + random(4000);
}

void loop() {
    updateSeismic();
    updateBreathing();
    updateChaos();

    view.addData("Seismic", seismicValue);
    view.addData("Breathing", breathEnvelope * sin(breathPhase));
    view.addData("Chaos", chaosOffset + chaosScale * (chaosX - 0.5));

    view.send();
}

// ===== Plot 0: Seismic — quiet periods with violent quakes =====
void updateSeismic() {
    unsigned long now = millis();

    if (seismicQuaking) {
        // Quake in progress: decaying oscillation with random frequency
        seismicPhase += 0.3 + random(200) / 1000.0;
        seismicValue = seismicMagnitude * sin(seismicPhase);

        // Add chaotic component
        seismicValue += seismicMagnitude * 0.3 * (random(2000) - 1000) / 1000.0;

        // Decay
        seismicMagnitude *= seismicDecay;

        // Quake ends when magnitude drops below threshold
        if (seismicMagnitude < 0.05) {
            seismicQuaking = false;
            seismicValue = 0.0;
            seismicNextQuake = now + 4000 + random(8000);  // 4-12 seconds of quiet
        }
    } else {
        // Quiet period: near-zero with micro-noise
        seismicValue = (random(100) - 50) / 2000.0;  // ±0.025

        // Occasional micro-tremor (1% chance)
        if (random(100) < 1) {
            seismicValue += (random(2) == 0 ? 1 : -1) * (0.5 + random(100) / 100.0);
        }

        // Time for a quake?
        if (now >= seismicNextQuake) {
            seismicQuaking = true;
            seismicPhase = random(628) / 100.0;  // Random starting phase

            // Quake magnitude: varies wildly
            int severity = random(5);
            switch (severity) {
                case 0: seismicMagnitude = 0.5 + random(100) / 100.0;   break;  // Minor
                case 1: seismicMagnitude = 2.0 + random(800) / 100.0;   break;  // Moderate
                case 2: seismicMagnitude = 10.0 + random(30);           break;  // Strong
                case 3: seismicMagnitude = 40.0 + random(60);           break;  // Violent
                case 4: seismicMagnitude = 100.0 + random(200);         break;  // Catastrophic
            }

            // Decay rate affects how long the quake rings
            seismicDecay = 0.990 + random(9) / 1000.0;  // 0.990 - 0.999
        }
    }
}

// ===== Plot 1: Breathing — smooth modulation with sudden scale changes =====
void updateBreathing() {
    unsigned long now = millis();

    // Core oscillation
    breathPhase += 0.08 + 0.02 * sin(now / 5000.0);
    if (breathPhase > 2.0 * M_PI) breathPhase -= 2.0 * M_PI;

    // Envelope slowly approaches target
    breathEnvelope += (breathEnvTarget - breathEnvelope) * breathEnvRate;

    // Small envelope wobble
    breathEnvelope += (random(100) - 50) / 10000.0;

    // Time for a scale shift?
    if (now >= breathNextShift) {
        int shiftType = random(5);
        switch (shiftType) {
            case 0:
                // Tiny signal
                breathEnvTarget = 0.1 + random(100) / 200.0;   // 0.1 - 0.6
                breathEnvRate = 0.001;
                break;
            case 1:
                // Medium signal
                breathEnvTarget = 3.0 + random(700) / 100.0;   // 3 - 10
                breathEnvRate = 0.0005;
                break;
            case 2:
                // Large signal
                breathEnvTarget = 20.0 + random(30);            // 20 - 50
                breathEnvRate = 0.0003;
                break;
            case 3:
                // Snap: instant envelope change
                breathEnvelope = 0.5 + random(10000) / 100.0;  // 0.5 - 100.5
                breathEnvTarget = breathEnvelope;
                breathEnvRate = 0.001;
                break;
            case 4:
                // Very large then rapid collapse
                breathEnvTarget = 80.0 + random(120);           // 80 - 200
                breathEnvRate = 0.002;
                break;
        }

        breathNextShift = now + 4000 + random(10000);  // 4-14 seconds between shifts
    }
}

// ===== Plot 2: Chaos — logistic map with random parameter perturbation =====
void updateChaos() {
    unsigned long now = millis();

    // Logistic map iteration: x_next = r * x * (1 - x)
    chaosX = chaosR * chaosX * (1.0 - chaosX);

    // Clamp to prevent escape (numerical safety)
    if (chaosX < 0.0 || chaosX > 1.0 || isnan(chaosX)) {
        chaosX = 0.3 + random(400) / 1000.0;  // Reset to valid range
    }

    // Slowly drift r toward target
    chaosR += (chaosRTarget - chaosR) * 0.001;

    // Time for a parameter perturbation?
    if (now >= chaosNextPerturb) {
        int perturbType = random(6);
        switch (perturbType) {
            case 0:
                // Fixed point regime (r < 3)
                chaosRTarget = 2.5 + random(50) / 100.0;       // 2.5 - 3.0
                chaosScale = 2.0 + random(500) / 100.0;        // 2 - 7
                chaosOffset = (random(40) - 20);
                break;
            case 1:
                // Period-2 regime
                chaosRTarget = 3.0 + random(45) / 100.0;       // 3.0 - 3.45
                chaosScale = 5.0 + random(1000) / 100.0;       // 5 - 15
                chaosOffset = (random(60) - 30);
                break;
            case 2:
                // Period-4 regime
                chaosRTarget = 3.45 + random(10) / 100.0;      // 3.45 - 3.55
                chaosScale = 10.0 + random(20);                 // 10 - 30
                chaosOffset = (random(100) - 50);
                break;
            case 3:
                // Edge of chaos
                chaosRTarget = 3.55 + random(15) / 100.0;      // 3.55 - 3.70
                chaosScale = 20.0 + random(30);                 // 20 - 50
                chaosOffset = (random(100) - 50);
                break;
            case 4:
                // Full chaos
                chaosRTarget = 3.7 + random(30) / 100.0;       // 3.70 - 4.00
                chaosScale = 30.0 + random(70);                 // 30 - 100
                chaosOffset = (random(200) - 100);
                break;
            case 5:
                // Sudden jump with scale explosion
                chaosRTarget = 3.9 + random(10) / 100.0;       // Near r=4
                chaosScale = 100.0 + random(200);               // 100 - 300
                chaosOffset = (random(400) - 200);
                chaosX = random(1000) / 1000.0;                 // Reinitialize state
                break;
        }

        chaosNextPerturb = now + 3000 + random(6000);  // 3-9 seconds between perturbations
    }
}
