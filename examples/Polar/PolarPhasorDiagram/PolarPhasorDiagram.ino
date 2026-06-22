/**
 * PolarPhasorDiagram.ino
 * @version V1R1
 * @date 03-23-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * PolarPhasorDiagram.ino - Three-phase AC power system phasor diagram
 *
 * Simulates the rotating phasor diagram of a balanced three-phase power system
 * under normal and fault conditions. In a healthy grid, three voltage phasors
 * sit 120 degrees apart at equal magnitudes — any deviation from this symmetry
 * signals a fault. The sketch cycles through seven power conditions (balanced,
 * voltage sag, swell, unbalanced, single-phase loss, harmonic distortion, and
 * frequency drift), making the phasor asymmetries immediately visible.
 *
 * This demonstrates:
 * - Polar plot in Frames mode with origin-to-tip phasor lines
 * - addBreak() to draw disconnected line segments within a single trace
 * - Angular offset for north-up compass orientation
 * - Smooth parameter transitions for realistic fault evolution
 *
 * Verified hardware:
 * - Teensy 4.1
 *
 * Known issues:
 * - None recorded for this sketch
 *
 * Board-specific requirements:
 * - Uses only core ViewPoint APIs and standard Arduino runtime features.
 * - A faster 32-bit board is recommended for smooth frame updates; only Teensy 4.1 has been hardware-verified for this sketch so far.
 *
 * Hardware:
 * - No external hardware required.
 *
 * Sketch outline:
 * - Setup configures a polar plot with three color-coded phase traces
 * - Loop advances the system rotation angle each frame
 * - Fault conditions modify per-phase voltage, angle offset, and harmonics
 * - Each phasor draws as an origin-to-tip line using addBreak() separation
 *
 * Things to try:
 * - Set ROTATION_DEG_PER_FRAME to 0 to freeze the phasors and study static angles
 * - Change NOMINAL_VOLTAGE to 120.0 to model North American line-to-neutral
 * - Increase CONDITION_HOLD_MS to 15000 to watch each fault evolve longer
 * - Set HARMONIC_3RD_MAX to 0.4 for extreme THD wobble
 * - Comment out the sub-event block in loop() to see pure fault transitions
 */

#include <ViewPoint.h>
using namespace viewpoint;

// ─── Experimentation Levers ───
#define NOMINAL_VOLTAGE         230.0   // Line-to-neutral RMS (V) — try 120.0 for US grid
#define ROTATION_DEG_PER_FRAME  3.0     // Visual rotation speed — 0 freezes phasors
#define FRAME_DELAY_MS          30      // Frame period — lower = faster animation
#define CONDITION_HOLD_MS       5000    // Minimum ms before switching fault type
#define CONDITION_JITTER_MS     5000    // Random additional hold time
#define HARMONIC_3RD_MAX        0.25    // Peak 3rd harmonic depth (fraction of fundamental)
#define HARMONIC_5TH_MAX        0.12    // Peak 5th harmonic depth
#define NOMINAL_FREQ_HZ         50.0    // System frequency — try 60.0 for US

#define NUM_POINTS_PER_PHASOR   2       // Origin + tip
#define FRAME_SIZE              (3 * NUM_POINTS_PER_PHASOR)

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

struct PhaseParams {
    float voltage;
    float target_voltage;
    float angle_offset;         // Degrees from ideal 120-degree spacing
    float target_angle_offset;
    float harmonic_3rd;         // 3rd harmonic distortion depth
    float harmonic_5th;         // 5th harmonic distortion depth
};

PhaseParams phases[3];
PowerCondition current_condition = BALANCED;

float system_angle = 0.0;      // Rotating reference frame (degrees)
float system_freq_hz = NOMINAL_FREQ_HZ;
float target_freq_hz = NOMINAL_FREQ_HZ;

unsigned long next_condition_change = 0;
unsigned long next_sub_event = 0;

void set_condition(PowerCondition c);

void setup() {
    randomSeed(analogRead(0));

    view.begin(PlotType::Polar, Mode::Frames, FRAME_SIZE);
    view.setTitle("Polar Phasor Diagram");
    view.setDelay(FRAME_DELAY_MS);
    view.setAngularOffset(90);      // North-up orientation
    view.setAngularStep(30);        // 30-degree compass grid
    view.setPlotTitle("Three-Phase Phasor Diagram");

    view.trace("Phase A").setColor(colors::Red);
    view.trace("Phase B").setColor(colors::Gold);
    view.trace("Phase C").setColor(colors::DodgerBlue);

    for (int i = 0; i < 3; i++) {
        phases[i].voltage = NOMINAL_VOLTAGE;
        phases[i].target_voltage = NOMINAL_VOLTAGE;
        phases[i].angle_offset = 0.0;
        phases[i].target_angle_offset = 0.0;
        phases[i].harmonic_3rd = 0.0;
        phases[i].harmonic_5th = 0.0;
    }

    next_condition_change = millis() + CONDITION_HOLD_MS;
    next_sub_event = millis() + 2000;
}

void loop() {
    unsigned long now = millis();

    // Cycle through fault conditions
    if (now >= next_condition_change) {
        set_condition((PowerCondition)random(NUM_CONDITIONS));
        next_condition_change = now + CONDITION_HOLD_MS + random(CONDITION_JITTER_MS);
    }

    // Small per-phase perturbations within the current condition
    if (now >= next_sub_event) {
        int p = random(3);
        phases[p].target_voltage += (random(2000) - 1000) / 100.0;
        phases[p].target_voltage = constrain(phases[p].target_voltage, 10.0f, 500.0f);
        phases[p].target_angle_offset += (random(200) - 100) / 10.0;
        phases[p].target_angle_offset = constrain(phases[p].target_angle_offset, -30.0f, 30.0f);
        next_sub_event = now + 1000 + random(3000);
    }

    // Frequency drift maps to rotation speed deviation
    system_freq_hz += (target_freq_hz - system_freq_hz) * 0.01;
    float rotation_speed = ROTATION_DEG_PER_FRAME * (system_freq_hz / NOMINAL_FREQ_HZ);

    system_angle += rotation_speed;
    if (system_angle >= 360.0) system_angle -= 360.0;

    // Smooth parameter transitions — phasors evolve gradually, not in steps
    for (int i = 0; i < 3; i++) {
        phases[i].voltage += (phases[i].target_voltage - phases[i].voltage) * 0.04;
        phases[i].angle_offset += (phases[i].target_angle_offset - phases[i].angle_offset) * 0.03;
    }

    // Draw three phasors as origin-to-tip lines
    const char* names[] = {"Phase A", "Phase B", "Phase C"};
    float ideal_angles[] = {0.0, 120.0, 240.0};

    for (int i = 0; i < 3; i++) {
        float angle = system_angle + ideal_angles[i] + phases[i].angle_offset;

        // Harmonic distortion causes angular wobble on the phasor tip
        float wobble = phases[i].harmonic_3rd * sin(3.0 * angle * M_PI / 180.0) * 15.0;
        wobble += phases[i].harmonic_5th * sin(5.0 * angle * M_PI / 180.0) * 8.0;
        angle += wobble;

        while (angle < 0) angle += 360.0;
        while (angle >= 360.0) angle -= 360.0;

        float voltage = phases[i].voltage;

        // Measurement noise (~0.5% of reading)
        voltage += voltage * 0.005 * (random(2000) - 1000) / 1000.0;
        voltage = max(0.0f, voltage);

        // Harmonics also modulate magnitude slightly
        float mag_distortion = 1.0 + phases[i].harmonic_3rd * 0.3 *
                               sin(3.0 * system_angle * M_PI / 180.0);
        voltage *= mag_distortion;

        view.addData(names[i], 0.0, angle);
        view.addData(names[i], constrain(voltage, 0.0f, 1e6f), angle);
        view.addBreak(names[i]);
    }

    view.send();
}

/*
 * Configure per-phase parameters for each fault type.
 * Target values are set here; loop() smoothly interpolates toward them.
 */
void set_condition(PowerCondition c) {
    current_condition = c;

    switch (c) {
        case BALANCED:
            for (int i = 0; i < 3; i++) {
                phases[i].target_voltage = NOMINAL_VOLTAGE + (random(1000) - 500) / 100.0;
                phases[i].target_angle_offset = 0.0;
                phases[i].harmonic_3rd = 0.0;
                phases[i].harmonic_5th = 0.0;
            }
            target_freq_hz = NOMINAL_FREQ_HZ;
            break;

        case VOLTAGE_SAG: {
            int sag_phase = random(3);
            for (int i = 0; i < 3; i++) {
                if (i == sag_phase) {
                    // 30-80% drop — motor start or downstream fault
                    float sag_fraction = 0.2 + random(50) / 100.0;
                    phases[i].target_voltage = NOMINAL_VOLTAGE * sag_fraction;
                } else {
                    phases[i].target_voltage = NOMINAL_VOLTAGE + (random(1000) - 500) / 100.0;
                }
                phases[i].target_angle_offset = (random(200) - 100) / 20.0;
                phases[i].harmonic_3rd = 0.0;
                phases[i].harmonic_5th = 0.0;
            }
            target_freq_hz = NOMINAL_FREQ_HZ;
            break;
        }

        case VOLTAGE_SWELL: {
            int swell_phase = random(3);
            for (int i = 0; i < 3; i++) {
                if (i == swell_phase) {
                    // 10-40% rise — load rejection or capacitor switching
                    float swell_fraction = 1.1 + random(30) / 100.0;
                    phases[i].target_voltage = NOMINAL_VOLTAGE * swell_fraction;
                } else {
                    phases[i].target_voltage = NOMINAL_VOLTAGE + (random(600) - 300) / 100.0;
                }
                phases[i].target_angle_offset = (random(100) - 50) / 10.0;
                phases[i].harmonic_3rd = 0.0;
                phases[i].harmonic_5th = 0.0;
            }
            target_freq_hz = NOMINAL_FREQ_HZ;
            break;
        }

        case UNBALANCED:
            for (int i = 0; i < 3; i++) {
                phases[i].target_voltage = 100.0 + random(25000) / 100.0;
                phases[i].target_angle_offset = (random(4000) - 2000) / 100.0;
                phases[i].harmonic_3rd = 0.0;
                phases[i].harmonic_5th = 0.0;
            }
            target_freq_hz = NOMINAL_FREQ_HZ;
            break;

        case SINGLE_PHASE_LOSS: {
            int lost_phase = random(3);
            for (int i = 0; i < 3; i++) {
                if (i == lost_phase) {
                    phases[i].target_voltage = random(500) / 100.0;  // Near-zero (blown fuse)
                } else {
                    // Remaining phases rise from load redistribution
                    phases[i].target_voltage = NOMINAL_VOLTAGE * (1.0 + random(20) / 100.0);
                    phases[i].target_angle_offset = (random(200) - 100) / 10.0;
                }
                phases[i].harmonic_3rd = 0.0;
                phases[i].harmonic_5th = 0.0;
            }
            target_freq_hz = NOMINAL_FREQ_HZ - 1.0 + random(200) / 100.0;
            break;
        }

        case HARMONIC_DISTORTION:
            for (int i = 0; i < 3; i++) {
                phases[i].target_voltage = NOMINAL_VOLTAGE + (random(2000) - 1000) / 100.0;
                phases[i].target_angle_offset = (random(100) - 50) / 10.0;
                phases[i].harmonic_3rd = 0.05 + random((int)(HARMONIC_3RD_MAX * 1000)) / 1000.0;
                phases[i].harmonic_5th = 0.02 + random((int)(HARMONIC_5TH_MAX * 1000)) / 1000.0;
            }
            target_freq_hz = NOMINAL_FREQ_HZ;
            break;

        case FREQUENCY_DRIFT:
            for (int i = 0; i < 3; i++) {
                phases[i].target_voltage = NOMINAL_VOLTAGE + (random(1000) - 500) / 100.0;
                phases[i].target_angle_offset = 0.0;
                phases[i].harmonic_3rd = 0.0;
                phases[i].harmonic_5th = 0.0;
            }
            // Frequency wanders +/- 3 Hz around nominal
            target_freq_hz = NOMINAL_FREQ_HZ - 3.0 + random(600) / 100.0;
            break;
    }
}
