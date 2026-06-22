/**
 * VibrationAnalysisXY.ino
 * @version V1R1 R0.2
 * @date 01-27-26
 * 
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * VibrationAnalysisXY.ino - Rotating Machinery Orbit Plots
 *
 * Displays X-Y orbit plots used in industrial vibration analysis for
 * rotating machinery diagnostics. Different fault conditions produce
 * characteristic orbit patterns.
 *
 * This demonstrates:
 * - Scatter plot with persistence mode for orbit trails
 * - Multiple harmonics creating complex patterns
 * - Industrial vibration analysis concepts
 * - Machine fault signature visualization
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
 * - Set FAULT_TYPE to see different orbit patterns:
 *   - 0 = Balanced (clean circle)
 *   - 1 = Unbalance (ellipse, 1X dominant)
 *   - 2 = Misalignment (figure-8, 2X dominant)
 *   - 3 = Looseness (multi-loop, many harmonics)
 * - Adjust ROTATION_RPM to change rotation speed
 * - Change NOISE_LEVEL to simulate real sensor data
 */

#include <ViewPoint.h>

using namespace viewpoint;

// Rotation Parameters
#define SAMPLE_RATE 10000       // Samples per second
#define ROTATION_RPM 3600       // Shaft rotation speed (60 Hz at 3600 RPM)

// Fault Types - change this to see different orbit patterns
#define FAULT_BALANCED      0   // Clean circular orbit
#define FAULT_UNBALANCE     1   // Elliptical orbit (1X dominant)
#define FAULT_MISALIGNMENT  2   // Figure-8 ish pattern (2X dominant)
#define FAULT_LOOSENESS     3   // Multi-loop pattern (many harmonics)

#define FAULT_TYPE FAULT_LOOSENESS  // Select fault type to simulate

// Signal Parameters
#define AMPLITUDE 10.0          // Base vibration amplitude
#define NOISE_LEVEL 0.5         // Measurement noise

// Calculated fundamental frequency
float fundamental_freq = ROTATION_RPM / 60.0;  // Hz

// Noise is generated inline with random() — no signal generator needed

// Time tracking
float t = 0;
float dt = 1.0 / SAMPLE_RATE;

void setup() {
    // Initialize ViewPoint in Scatter mode
    view.begin(PlotType::Scatter, Mode::Continuous);
    view.setTitle("Vibration Analysis XY");

    // Use persistence for orbit trail visualization
    view.setDisplayMode(DisplayMode::Persistence);

    // Symmetric axes for true orbit shape
    float range = AMPLITUDE * 2.5;
    view.setHorizontalRange(-range, range);
    view.setVerticalRange(-range, range);

    // Add origin crosshair
    view.addHorizontalReferenceLine(0.0);
    view.addVerticalReferenceLine(0.0);

    // Label as vibration plot
    view.setAxisLabels("X Vibration", "Y Vibration");
    view.setPlotTitle("Shaft Orbit Plot");

    // Update rate
    view.setDelay(1);
}

void loop() {
    float x = 0;
    float y = 0;

    // Generate orbit based on fault type
    switch (FAULT_TYPE) {
        case FAULT_BALANCED:
            // Clean circle - X and Y are 90 degrees out of phase
            x = AMPLITUDE * cos(2.0 * M_PI * fundamental_freq * t);
            y = AMPLITUDE * sin(2.0 * M_PI * fundamental_freq * t);
            break;

        case FAULT_UNBALANCE: {
            // Elliptical orbit - different amplitudes in X and Y
            // 1X (fundamental) dominant, typical mass unbalance signature
            x = AMPLITUDE * cos(2.0 * M_PI * fundamental_freq * t);
            y = AMPLITUDE * 0.6 * sin(2.0 * M_PI * fundamental_freq * t);
            // Add slight 2X component
            x += AMPLITUDE * 0.1 * cos(4.0 * M_PI * fundamental_freq * t);
            y += AMPLITUDE * 0.1 * sin(4.0 * M_PI * fundamental_freq * t);
            break;
        }
        case FAULT_MISALIGNMENT: {
            // Figure-8 pattern - strong 2X component
            // Angular misalignment produces 2X vibration
            x = AMPLITUDE * cos(2.0 * M_PI * fundamental_freq * t);
            y = AMPLITUDE * sin(2.0 * M_PI * fundamental_freq * t);
            // Strong 2X component creates the figure-8
            x += AMPLITUDE * 0.5 * cos(4.0 * M_PI * fundamental_freq * t);
            y += AMPLITUDE * 0.5 * sin(4.0 * M_PI * fundamental_freq * t + M_PI/4);
            break;
        }
        case FAULT_LOOSENESS: {
            // Multi-loop pattern - many harmonics present
            // Mechanical looseness produces sub-harmonics and many 1X multiples
            x = AMPLITUDE * cos(2.0 * M_PI * fundamental_freq * t);
            y = AMPLITUDE * sin(2.0 * M_PI * fundamental_freq * t);
            // Add multiple harmonics
            for (int h = 2; h <= 5; h++) {
                float harmonic_amp = AMPLITUDE * (0.3 / h);
                x += harmonic_amp * cos(2.0 * M_PI * fundamental_freq * h * t + h * 0.5);
                y += harmonic_amp * sin(2.0 * M_PI * fundamental_freq * h * t + h * 0.3);
            }
            // Add sub-harmonic (0.5X)
            x += AMPLITUDE * 0.2 * cos(M_PI * fundamental_freq * t);
            y += AMPLITUDE * 0.2 * sin(M_PI * fundamental_freq * t);
            break;
        }
    }

    // Add measurement noise
    x += (random(-1000, 1001) / 1000.0) * NOISE_LEVEL;
    y += (random(-1000, 1001) / 1000.0) * NOISE_LEVEL;

    // Send to ViewPoint
    view.addData("Orbit", x, y);
    view.send();

    // Advance time
    t += dt;

    // Reset to prevent overflow (keep it to a few rotations)
    if (t > 1.0 / fundamental_freq * 10) {
        t = 0;
    }
}
