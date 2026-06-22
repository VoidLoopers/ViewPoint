/**
 * PolarPhasorDiagram.ino
 * @date 03-10-26
 *
 * ViewPoint Polar: Frames — Three-Phase Power System Phasor Diagram
 *
 * Simulates the rotating phasor diagram of a three-phase AC power system.
 * Each phase (A, B, C) is a vector from the origin at its instantaneous
 * angle, with magnitude proportional to the RMS voltage. Under normal
 * conditions the three phasors are balanced (equal magnitude, 120 degrees
 * apart). Power system faults introduce dramatic asymmetries:
 *
 * Conditions:
 * - BALANCED:       120-degree spacing, equal magnitudes (~230V per phase)
 * - VOLTAGE_SAG:    One phase drops 30-80% (motor start, fault on feeder)
 * - VOLTAGE_SWELL:  One phase rises 10-40% (load rejection, capacitor bank)
 * - UNBALANCED:     All three phases at different magnitudes and shifted angles
 * - SINGLE_PHASE:   One phase drops to near-zero (blown fuse, open conductor)
 * - HARMONIC_DIST:  Phasors wobble with 3rd/5th harmonic distortion
 * - FREQUENCY_DRIFT: System frequency drifts, phasors rotate at non-50/60 Hz
 *
 * Three traces draw origin-to-tip phasor lines. The auto-scaler must
 * track the largest phasor while keeping smaller ones visible. A balanced
 * system at 230V → single-phase sag to 50V is a 4.6x range contraction
 * on one trace while others hold.
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch the phasors collapse during single-phase loss
 * - Note whether the scale adapts when one phase swells to 350V+
 * - See the wobble during harmonic distortion
 * - Compare SYMMETRIC_ZERO behavior (all phasors centered at origin)
 */

#include <ViewPoint.h>
using namespace viewpoint;

#define NUM_POINTS_PER_PHASOR  2   // Origin + tip
#define FRAME_SIZE            (3 * NUM_POINTS_PER_PHASOR)

enum PowerCondition : uint8_t {
    BALANCED,
    VOLTAGE_SAG,
    VOLTAGE_SWELL,
    UNBALANCED,
    SINGLE_PHASE_LOSS,
    HARMONIC_DISTORTION,
    FREQUENCY_DRIFT,
    NUM_CONDITIONS
};

PowerCondition currentCondition = BALANCED;

// Per-phase parameters
struct PhaseParams {
    float voltage;           // RMS magnitude (V)
    float targetVoltage;
    float angleOffset;       // Degrees from ideal position
    float targetAngleOffset;
    float harmonicDepth;     // 3rd harmonic distortion (fraction)
    float fifthHarmonic;     // 5th harmonic distortion
};

PhaseParams phases[3];

// System parameters
float nominalVoltage = 230.0;  // Line-to-neutral RMS
float systemAngle = 0.0;       // Rotating reference (degrees)
float systemFreqHz = 50.0;     // Nominal 50 Hz
float targetFreqHz = 50.0;
float freqDriftRate = 0.0;

// Animation: how fast phasors rotate visually
// (scaled down so rotation is visible, not 50 Hz real-time)
float rotationSpeed = 3.0;     // Degrees per frame

// Timing
unsigned long nextConditionChange = 0;
unsigned long nextSubEvent = 0;
unsigned long frameCount = 0;

void setCondition(PowerCondition c);

void setup() {
    randomSeed(analogRead(0));
    view.begin(PlotType::Polar, Mode::Frames, FRAME_SIZE);
    view.setDelay(30);

    view.setAngularOffset(90);
    view.setAngularStep(30);
    view.setPlotTitle("Three-Phase Phasor Diagram");

    view.trace("Phase A").setColor(colors::Red);
    view.trace("Phase B").setColor(colors::Gold);
    view.trace("Phase C").setColor(colors::DodgerBlue);

    // Initialize balanced
    for (int i = 0; i < 3; i++) {
        phases[i].voltage = nominalVoltage;
        phases[i].targetVoltage = nominalVoltage;
        phases[i].angleOffset = 0.0;
        phases[i].targetAngleOffset = 0.0;
        phases[i].harmonicDepth = 0.0;
        phases[i].fifthHarmonic = 0.0;
    }

    nextConditionChange = millis() + 4000 + random(4000);
    nextSubEvent = millis() + 2000 + random(3000);
}

void loop() {
    unsigned long now = millis();

    // Condition changes
    if (now >= nextConditionChange) {
        PowerCondition next = (PowerCondition)random(NUM_CONDITIONS);
        setCondition(next);
        nextConditionChange = now + 3000 + random(7000);
    }

    // Sub-events: small perturbations within a condition
    if (now >= nextSubEvent) {
        // Random per-phase jitter
        int p = random(3);
        phases[p].targetVoltage += (random(2000) - 1000) / 100.0;
        phases[p].targetVoltage = constrain(phases[p].targetVoltage, 10.0f, 500.0f);
        phases[p].targetAngleOffset += (random(200) - 100) / 10.0;
        phases[p].targetAngleOffset = constrain(phases[p].targetAngleOffset, -30.0f, 30.0f);
        nextSubEvent = now + 1000 + random(3000);
    }

    // Frequency drift
    systemFreqHz += (targetFreqHz - systemFreqHz) * 0.01;
    // Map frequency deviation to rotation speed change
    rotationSpeed = 3.0 * (systemFreqHz / 50.0);

    // Advance system rotation
    systemAngle += rotationSpeed;
    if (systemAngle >= 360.0) systemAngle -= 360.0;

    // Smooth parameter transitions
    for (int i = 0; i < 3; i++) {
        phases[i].voltage += (phases[i].targetVoltage - phases[i].voltage) * 0.04;
        phases[i].angleOffset += (phases[i].targetAngleOffset - phases[i].angleOffset) * 0.03;
    }

    // Draw three phasors
    const char* names[] = {"Phase A", "Phase B", "Phase C"};
    float idealAngles[] = {0.0, 120.0, 240.0};

    for (int i = 0; i < 3; i++) {
        float angle = systemAngle + idealAngles[i] + phases[i].angleOffset;

        // Harmonic distortion: 3rd and 5th cause phasor to wobble
        float harmonicWobble = phases[i].harmonicDepth * sin(3.0 * angle * M_PI / 180.0) * 15.0;
        harmonicWobble += phases[i].fifthHarmonic * sin(5.0 * angle * M_PI / 180.0) * 8.0;
        angle += harmonicWobble;

        // Wrap angle
        while (angle < 0) angle += 360.0;
        while (angle >= 360.0) angle -= 360.0;

        float voltage = phases[i].voltage;
        // Add measurement noise
        voltage += voltage * 0.005 * (random(2000) - 1000) / 1000.0;
        voltage = max(0.0f, voltage);

        // Harmonic distortion also affects magnitude slightly
        float magDistortion = 1.0 + phases[i].harmonicDepth * 0.3 *
                              sin(3.0 * systemAngle * M_PI / 180.0);
        voltage *= magDistortion;

        // Origin point and tip
        view.addData(names[i], 0.0, angle);
        view.addData(names[i], constrain(voltage, 0.0f, 1e6f), angle);
        view.addBreak(names[i]);
    }

    view.send();
    frameCount++;
}

void setCondition(PowerCondition c) {
    currentCondition = c;

    switch (c) {
        case BALANCED:
            for (int i = 0; i < 3; i++) {
                phases[i].targetVoltage = nominalVoltage + (random(1000) - 500) / 100.0;
                phases[i].targetAngleOffset = 0.0;
                phases[i].harmonicDepth = 0.0;
                phases[i].fifthHarmonic = 0.0;
            }
            targetFreqHz = 50.0;
            break;

        case VOLTAGE_SAG: {
            int sagPhase = random(3);
            for (int i = 0; i < 3; i++) {
                if (i == sagPhase) {
                    // 30-80% drop
                    float sagFraction = 0.2 + random(50) / 100.0;
                    phases[i].targetVoltage = nominalVoltage * sagFraction;
                } else {
                    phases[i].targetVoltage = nominalVoltage + (random(1000) - 500) / 100.0;
                }
                phases[i].targetAngleOffset = (random(200) - 100) / 20.0;
                phases[i].harmonicDepth = 0.0;
                phases[i].fifthHarmonic = 0.0;
            }
            targetFreqHz = 50.0;
            break;
        }

        case VOLTAGE_SWELL: {
            int swellPhase = random(3);
            for (int i = 0; i < 3; i++) {
                if (i == swellPhase) {
                    // 10-40% rise
                    float swellFraction = 1.1 + random(30) / 100.0;
                    phases[i].targetVoltage = nominalVoltage * swellFraction;
                } else {
                    phases[i].targetVoltage = nominalVoltage + (random(600) - 300) / 100.0;
                }
                phases[i].targetAngleOffset = (random(100) - 50) / 10.0;
                phases[i].harmonicDepth = 0.0;
                phases[i].fifthHarmonic = 0.0;
            }
            targetFreqHz = 50.0;
            break;
        }

        case UNBALANCED:
            for (int i = 0; i < 3; i++) {
                phases[i].targetVoltage = 100.0 + random(25000) / 100.0;  // 100-350V
                phases[i].targetAngleOffset = (random(4000) - 2000) / 100.0;  // +/- 20 degrees
                phases[i].harmonicDepth = 0.0;
                phases[i].fifthHarmonic = 0.0;
            }
            targetFreqHz = 50.0;
            break;

        case SINGLE_PHASE_LOSS: {
            int lostPhase = random(3);
            for (int i = 0; i < 3; i++) {
                if (i == lostPhase) {
                    phases[i].targetVoltage = random(500) / 100.0;  // Near-zero (0-5V)
                } else {
                    // Remaining phases may rise due to load redistribution
                    phases[i].targetVoltage = nominalVoltage * (1.0 + random(20) / 100.0);
                    phases[i].targetAngleOffset = (random(200) - 100) / 10.0;
                }
                phases[i].harmonicDepth = 0.0;
                phases[i].fifthHarmonic = 0.0;
            }
            targetFreqHz = 49.0 + random(200) / 100.0;  // Slight frequency disturbance
            break;
        }

        case HARMONIC_DISTORTION:
            for (int i = 0; i < 3; i++) {
                phases[i].targetVoltage = nominalVoltage + (random(2000) - 1000) / 100.0;
                phases[i].targetAngleOffset = (random(100) - 50) / 10.0;
                phases[i].harmonicDepth = 0.05 + random(200) / 1000.0;   // 5-25% THD
                phases[i].fifthHarmonic = 0.02 + random(100) / 1000.0;   // 2-12% 5th
            }
            targetFreqHz = 50.0;
            break;

        case FREQUENCY_DRIFT:
            for (int i = 0; i < 3; i++) {
                phases[i].targetVoltage = nominalVoltage + (random(1000) - 500) / 100.0;
                phases[i].targetAngleOffset = 0.0;
                phases[i].harmonicDepth = 0.0;
                phases[i].fifthHarmonic = 0.0;
            }
            // Frequency wanders between 47 and 53 Hz
            targetFreqHz = 47.0 + random(600) / 100.0;
            break;
    }
}
