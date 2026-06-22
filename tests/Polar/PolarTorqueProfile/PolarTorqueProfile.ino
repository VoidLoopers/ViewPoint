/**
 * PolarTorqueProfile.ino
 * @date 03-10-26
 *
 * ViewPoint Polar Test: Continuous — Rotating Machinery Torque Monitor
 *
 * Simulates a strain-gauge telemetry ring on a rotating shaft, reporting
 * instantaneous torque (radius) vs angular position (theta) as the shaft
 * turns. In healthy operation the torque profile is roughly sinusoidal
 * (gravity sag on a horizontal shaft). When faults develop, characteristic
 * signatures appear: mass imbalance adds a 1x harmonic, misalignment
 * adds 2x, bearing defects produce sharp periodic impulses, and a
 * seized bearing causes torque to spike then drop to near-zero.
 *
 * The auto-scaler must handle:
 * - Healthy: smooth profile, peak torque ~50 Nm
 * - Imbalance: growing 1x oscillation up to ~200 Nm
 * - Misalignment: 2x harmonic, asymmetric peaks to ~300 Nm
 * - Bearing defect: sharp impulse spikes 5-10x above baseline
 * - Seize/release: torque spikes to 1000+ Nm then collapses to ~5 Nm
 * - Load step: sudden baseline shift when driven load engages/disengages
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch the spike during bearing defect impulses
 * - See if the scale recovers after a seize/release event
 * - Note the asymmetry during misalignment (positive vs negative peaks)
 * - Compare PEAK_DECAY (should hold spike briefly) vs WINDOWED
 */

#include <ViewPoint.h>
using namespace viewpoint;

// --- Shaft parameters ---
#define SHAFT_RPM          300.0     // Nominal RPM
#define DEG_PER_SAMPLE     3.0      // Angular advance per sample
#define SAMPLE_DELAY_MS    1

// --- Fault modes ---
enum FaultMode : uint8_t {
    HEALTHY,
    IMBALANCE,
    MISALIGNMENT,
    BEARING_DEFECT,
    SEIZE_RELEASE,
    LOAD_STEP,
    NUM_FAULTS
};

FaultMode currentFault = HEALTHY;

// --- Torque parameters ---
float baseTorque = 50.0;          // Nm baseline
float targetBaseTorque = 50.0;
float imbalanceAmplitude = 0.0;
float targetImbalance = 0.0;
float misalignAmplitude = 0.0;
float targetMisalign = 0.0;

// Bearing defect: impulse at a fixed shaft angle
float defectAngle = 0.0;          // Where the defect fires
float defectAmplitude = 0.0;
float targetDefectAmp = 0.0;
float defectWidth = 5.0;          // Angular width of impulse

// Seize state
bool seized = false;
float seizeTorque = 0.0;
unsigned long seizeStart = 0;
unsigned long seizeDuration = 0;

// Load engagement
float loadOffset = 0.0;
float targetLoadOffset = 0.0;

// Shaft state
float shaftAngle = 0.0;
float gravityPhase = 0.0;         // Phase of gravity sag (random per setup)

// Timing
unsigned long nextFaultChange = 0;
unsigned long nextLoadEvent = 0;
unsigned long sampleCount = 0;

void setFault(FaultMode f);

void setup() {
    randomSeed(analogRead(0));
    view.begin(PlotType::Polar, Mode::Continuous);
    view.setDelay(SAMPLE_DELAY_MS);

    view.setAngularOffset(90);
    view.setAngularStep(30);
    view.setPlotTitle("Shaft Torque Monitor");

    view.trace("Torque").setColor(colors::Coral);
    view.trace("Baseline").setColor(colors::DimGray);

    gravityPhase = random(628) / 100.0;
    nextFaultChange = millis() + 4000 + random(4000);
    nextLoadEvent = millis() + 6000 + random(6000);
}

void loop() {
    unsigned long now = millis();

    // Fault transitions
    if (now >= nextFaultChange) {
        FaultMode next = (FaultMode)random(NUM_FAULTS);
        setFault(next);
        nextFaultChange = now + 3000 + random(8000);
    }

    // Load engage/disengage events
    if (now >= nextLoadEvent) {
        if (random(2) == 0) {
            targetLoadOffset = 20.0 + random(8000) / 100.0;   // Load on
        } else {
            targetLoadOffset = 0.0;                             // Load off
        }
        nextLoadEvent = now + 5000 + random(8000);
    }

    // Smooth parameter transitions
    baseTorque += (targetBaseTorque - baseTorque) * 0.01;
    imbalanceAmplitude += (targetImbalance - imbalanceAmplitude) * 0.02;
    misalignAmplitude += (targetMisalign - misalignAmplitude) * 0.02;
    defectAmplitude += (targetDefectAmp - defectAmplitude) * 0.03;
    loadOffset += (targetLoadOffset - loadOffset) * 0.05;

    // Seize/release logic
    if (seized) {
        unsigned long elapsed = now - seizeStart;
        if (elapsed < seizeDuration) {
            // Torque spikes high then decays
            float progress = (float)elapsed / seizeDuration;
            seizeTorque = (1.0 - progress) * 1000.0 + progress * 3.0;
        } else {
            seized = false;
            seizeTorque = 0.0;
        }
    }

    // Compute torque at current angle
    float rad = shaftAngle * M_PI / 180.0;

    // Base: gravity sag on horizontal shaft
    float torque = baseTorque + loadOffset;
    torque += baseTorque * 0.05 * sin(rad + gravityPhase);

    // 1x imbalance
    torque += imbalanceAmplitude * sin(rad);

    // 2x misalignment
    torque += misalignAmplitude * sin(2.0 * rad);
    // Misalignment is often asymmetric
    torque += misalignAmplitude * 0.3 * sin(2.0 * rad + M_PI / 3.0);

    // Bearing defect impulse
    if (defectAmplitude > 0.1) {
        float diff = shaftAngle - defectAngle;
        while (diff > 180) diff -= 360;
        while (diff < -180) diff += 360;
        if (abs(diff) < defectWidth) {
            float impulse = defectAmplitude * exp(-diff * diff / (defectWidth * 0.3));
            torque += impulse;
        }
    }

    // Seize override
    if (seized) {
        torque = seizeTorque;
    }

    // Measurement noise
    torque += torque * 0.01 * (random(2000) - 1000) / 1000.0;
    torque = max(0.0f, torque);

    // Send torque and a baseline reference
    view.addData("Torque", constrain(torque, 0.0f, 1e6f), shaftAngle);
    view.addData("Baseline", constrain(baseTorque + loadOffset, 0.0f, 1e6f), shaftAngle);

    view.send();

    // Advance shaft angle
    shaftAngle += DEG_PER_SAMPLE;
    if (shaftAngle >= 360.0) shaftAngle -= 360.0;
    sampleCount++;
}

void setFault(FaultMode f) {
    currentFault = f;

    switch (f) {
        case HEALTHY:
            targetBaseTorque = 40.0 + random(2000) / 100.0;
            targetImbalance = 0.0;
            targetMisalign = 0.0;
            targetDefectAmp = 0.0;
            break;

        case IMBALANCE:
            targetBaseTorque = 40.0 + random(2000) / 100.0;
            targetImbalance = 30.0 + random(17000) / 100.0;
            targetMisalign = 0.0;
            targetDefectAmp = 0.0;
            break;

        case MISALIGNMENT:
            targetBaseTorque = 50.0 + random(2000) / 100.0;
            targetImbalance = random(2000) / 100.0;  // Slight residual imbalance
            targetMisalign = 50.0 + random(25000) / 100.0;
            targetDefectAmp = 0.0;
            break;

        case BEARING_DEFECT:
            targetBaseTorque = 40.0 + random(1500) / 100.0;
            targetImbalance = random(1000) / 100.0;
            targetMisalign = random(1000) / 100.0;
            targetDefectAmp = 200.0 + random(60000) / 100.0;
            defectAngle = random(360);
            defectWidth = 3.0 + random(80) / 10.0;
            break;

        case SEIZE_RELEASE:
            seized = true;
            seizeStart = millis();
            seizeDuration = 500 + random(2000);
            // Reset other faults
            targetImbalance = 0.0;
            targetMisalign = 0.0;
            targetDefectAmp = 0.0;
            break;

        case LOAD_STEP:
            // Sudden load change is handled by loadOffset timer
            // But make it bigger here
            targetLoadOffset = (random(2) == 0)
                ? 50.0 + random(15000) / 100.0
                : 0.0;
            targetImbalance = random(3000) / 100.0;
            targetMisalign = random(2000) / 100.0;
            targetDefectAmp = 0.0;
            break;
    }
}
