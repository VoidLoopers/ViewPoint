/**
 * HeartbeatECG.ino
 * @version V1R1 R0.2
 * @date 01-27-26
 * 
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * HeartbeatECG.ino - Simulated ECG monitor display
 *
 * Generates a realistic ECG (electrocardiogram) waveform and displays it
 * with proper medical-style grid configuration. Standard ECG paper uses:
 * - 1mV = 10mm vertically (10 small boxes)
 * - 0.2mV per small box, 0.5mV per large box
 * - 25mm/sec paper speed (40ms per small box, 200ms per large box)
 *
 * The synthetic ECG includes P, QRS, and T waves with realistic timing.
 * Heart rate varies slightly to simulate natural variability.
 *
 * This demonstrates:
 * - Domain-specific grid configuration
 * - Custom grid colors (pink ECG paper style)
 * - Reference lines for normal ranges
 * - Continuous mode for strip-chart style display
 *
 * Verified hardware:
 * - Teensy 4.1
 *
 * Known issues:
 * - None recorded for this sketch
 *
 * Board-specific requirements:
 * - Uses only core ViewPoint APIs and standard Arduino runtime features.
 * - Only Teensy 4.1 has been hardware-verified for this sketch so far.
 *
 * Hardware:
 * - No external hardware required.
 *
 * Things to try:
 * - Change BASE_HEART_RATE to simulate different heart rates
 * - Adjust HEART_RATE_VARIABILITY for more/less natural variation
 * - Modify the ECG waveform parameters in generateECGSample()
 * - Enable persistence mode for analog monitor feel
 */

#include <ViewPoint.h>
using namespace viewpoint;

// ECG Configuration
const int BASE_HEART_RATE = 72;        // Beats per minute
const float HEART_RATE_VARIABILITY = 5; // +/- BPM variation
const float SAMPLE_RATE_MS = 4;        // Milliseconds per sample (250 Hz)

// ECG voltage ranges (in mV, typical values)
const float ECG_MIN = -0.5;
const float ECG_MAX = 1.5;

// ECG grid colors (pink/red like real ECG paper)
const uint32_t GRID_MINOR = 0xFFCCCC;  // Light pink
const uint32_t GRID_MAJOR = 0xFF6666;  // Darker pink/red

// State
float ecgPhase = 0;           // Phase within heartbeat (0 to 1)
float heartbeatPeriod = 0;    // Current beat period in ms
unsigned long lastSampleTime = 0;
float currentHeartRate = BASE_HEART_RATE;

void setup() {
    view.begin();
    view.setTitle("Heartbeat ECG");

    // Configure plot for ECG display
    view.setPlotTitle("ECG Monitor");

    // Standard ECG: 10mm/mV, so -0.5 to 1.5 mV range
    // Minor divisions at 0.1mV, major at 0.5mV
    view.setVerticalRange(ECG_MIN, ECG_MAX, 0.1, 0.5);

    // Labels and units
    view.setAxisLabels("Time", "Voltage");
    view.setUnits("s", "mV");

    // ECG paper-style grid colors
    view.setGridColors(GRID_MINOR, GRID_MAJOR);

    // Reference lines for normal ranges
    // addHorizontalReferenceLine adds a **horizontal** line to the **vertical** axis
    view.addHorizontalReferenceLine(0.0);  // Baseline/isoelectric line

    // Create trace with appropriate color(R,G,B)
    view.trace("Lead II").setColor(color(0, 0xAA, 0));  // Green like hospital monitors

    // Initialize heart rate
    updateHeartRate();
}

void loop() {
    unsigned long now = millis();

    // Sample at regular intervals
    if (now - lastSampleTime >= SAMPLE_RATE_MS) {
        lastSampleTime = now;

        // Generate ECG sample
        float ecgValue = generateECGSample(ecgPhase);

        // Send to plotter
        view.addData(0, ecgValue);
        view.send();

        // Advance phase
        float phaseIncrement = (SAMPLE_RATE_MS / heartbeatPeriod);
        ecgPhase += phaseIncrement;

        // New heartbeat
        if (ecgPhase >= 1.0) {
            ecgPhase -= 1.0;
            updateHeartRate();
        }
    }
}

// Update heart rate with natural variability
void updateHeartRate() {
    // Add random variability
    float variation = (random(-100, 100) / 100.0) * HEART_RATE_VARIABILITY;
    currentHeartRate = BASE_HEART_RATE + variation;

    // Calculate period in milliseconds
    heartbeatPeriod = 60000.0 / currentHeartRate;
}

// Generate synthetic ECG waveform
// Phase is 0-1 representing one complete heartbeat
float generateECGSample(float phase) {
    float value = 0;

    // Timing as fraction of beat (approximate for 72 BPM)
    // P wave:   0.00 - 0.10
    // PR seg:   0.10 - 0.16
    // QRS:      0.16 - 0.24
    // ST seg:   0.24 - 0.40
    // T wave:   0.40 - 0.56
    // TP seg:   0.56 - 1.00 (isoelectric)

    // P wave (small positive bump)
    if (phase >= 0.00 && phase < 0.10) {
        float t = (phase - 0.00) / 0.10;
        value = 0.15 * sin(t * M_PI);
    }
    // PR segment (flat at baseline)
    else if (phase >= 0.10 && phase < 0.16) {
        value = 0;
    }
    // Q wave (small negative dip)
    else if (phase >= 0.16 && phase < 0.18) {
        float t = (phase - 0.16) / 0.02;
        value = -0.1 * sin(t * M_PI);
    }
    // R wave (tall positive spike)
    else if (phase >= 0.18 && phase < 0.22) {
        float t = (phase - 0.18) / 0.04;
        value = 1.2 * sin(t * M_PI);
    }
    // S wave (small negative dip)
    else if (phase >= 0.22 && phase < 0.24) {
        float t = (phase - 0.22) / 0.02;
        value = -0.2 * sin(t * M_PI);
    }
    // ST segment (slightly elevated)
    else if (phase >= 0.24 && phase < 0.40) {
        value = 0.05;
    }
    // T wave (positive bump)
    else if (phase >= 0.40 && phase < 0.56) {
        float t = (phase - 0.40) / 0.16;
        value = 0.3 * sin(t * M_PI);
    }
    // TP segment (isoelectric/baseline)
    else {
        value = 0;
    }

    // Add small amount of baseline noise
    value += (random(-10, 10) / 1000.0);

    return value;
}
