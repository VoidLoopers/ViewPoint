/**
 * MultiSensorDashboard.ino
 * @version 1.0.0
 * @date 01-27-26
 * 
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * MultiSensorDashboard.ino - Industrial-style multi-sensor monitoring
 *
 * Simulates a multi-sensor monitoring system with 4 stacked plots showing:
 * 1. Temperature - slow drift with occasional spikes
 * 2. Vibration - fast oscillation (machinery simulation)
 * 3. Pressure - step changes (valve operations)
 * 4. Status - binary on/off states
 *
 * Each plot has its own appropriate scale, units, and visual style.
 * This demonstrates how to create a professional monitoring dashboard.
 *
 * This demonstrates:
 * - Multiple plots with vastly different scales
 * - Per-plot units, labels, and configuration
 * - Different signal types on one display
 * - Reference lines for alarm thresholds
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
 * - You can replace the simulated values with real temperature, vibration, pressure, or status inputs if desired.
 *
 * Things to try:
 * - Change update rates for different sensors
 * - Add more sensors (humidity, flow rate, etc.)
 * - Connect real sensors instead of simulated values
 */

#include <ViewPoint.h>
// All ViewPoint types and helpers live in the `viewpoint::` namespace.
// The Hello* sketches lean on the lower-case shortcut macros (cartesian,
// scatter, polar, xy, frames, continuous, ...) defined in ViewPoint.h so
// beginners can write `view.begin(scatter)` without typing `viewpoint::`.
// In this sketch we pull the whole namespace in so we can use the enum
// forms (`Mode::Continuous`, `colors::Red`, etc.) directly. If you ever
// hit a name collision with another library, switch back to the explicit
// `viewpoint::` form (or `#define USE_VIEWPOINT_WITH_NAMESPACE` before
// the include to disable the shortcut macros).
using namespace viewpoint;

// Simulation state
float temperature = 25.0;      // Degrees C
float vibration = 0.0;         // mm/s
float pressure = 1013.0;       // hPa (millibars)
int status = 1;                // 0 = off, 1 = on

// Simulation parameters
float tempTarget = 25.0;
float vibrationPhase = 0;
float pressureTarget = 1013.0;
unsigned long lastStatusChange = 0;

void setup() {
    view.begin(Mode::Continuous);
    view.setTitle("Multi-Sensor Dashboard");
    view.setDelay(20); // 20 Hz update rate

    // Create 4 stacked plots. When traces are not explicitly assigned to
    // a plot via `view.plot(i).addTrace(traceId)`, the host auto-maps
    // trace order to plot order: the first trace appears on the top plot,
    // the next on the second, and so on. We rely on that here so the
    // setup reads as four parallel "configure a plot, supply a value"
    // blocks. To pin a trace to a specific plot, call
    // `view.plot(i).addTrace(traceId)` explicitly.
    view.setNumberOfPlots(4);

    // ===== Plot 0: Temperature =====
    view.plot(0).setTitle("Temperature");
    view.plot(0).setVerticalRange(0, 50, 5, 10);  // 0-50°C, minor=5, major=10
    view.plot(0).setAxisLabels("Time", "Temp");
    view.plot(0).setUnits("s", "C");

    // Temperature alarm thresholds
    view.plot(0).vertical().addReferenceLine(35.0, color(33, 255, 102, 0), 1.5);  // Warning (orange)
    view.plot(0).vertical().addReferenceLine(40.0, color(33, 255, 0, 0), 2.0);  // Alarm (red)

    // ===== Plot 1: Vibration =====
    view.plot(1).setTitle("Vibration");
    view.plot(1).setVerticalRange(-10, 10, 2, 5);  // ±10 mm/s
    view.plot(1).setAxisLabels("Time", "Velocity");
    view.plot(1).setUnits("s", "mm/s");
    view.plot(1).addHorizontalReferenceLine(0.0f);  // zero line

    // ===== Plot 2: Pressure =====
    view.plot(2).setTitle("Pressure");
    view.plot(2).setVerticalRange(900, 1100, 20, 50);  // 900-1100 hPa
    view.plot(2).setAxisLabels("Time", "Pressure");
    view.plot(2).setUnits("s", "hPa");

    // Normal atmospheric pressure reference
    view.plot(2).vertical().addReferenceLine(1013.25, color(49, 0, 170, 0), 1.5);

    // ===== Plot 3: Status =====
    view.plot(3).setTitle("System Status");
    view.plot(3).setVerticalRange(-0.5, 1.5, 1, 0.5);  // 0 or 1
    view.plot(3).setAxisLabels("Time", "State");
    view.plot(3).setUnits("s", "");

    view.trace("Temp").setColor(colors::Red);       // Red for temperature
    view.trace("Vib").setColor(colors::Green);      // Green for vibration
    view.trace("Press").setColor(colors::Blue);     // Blue for pressure
    view.trace("Status").setColor(colors::Orange);  // Orange for status
}

void loop() {
    // Update simulated sensor values
    updateSimulation();

    // Send all sensor values
    view.addData("Temp", temperature);
    view.addData("Vib", vibration);
    view.addData("Press", pressure);
    view.addData("Status", (float)status);

    view.send();
}

void updateSimulation() {
    unsigned long now = millis();

    // Temperature: slow drift toward target with occasional spikes
    if (random(100) < 2) {
        // Random target change (simulates process changes)
        tempTarget = 20 + random(20);  // 20-40°C
    }
    temperature += (tempTarget - temperature) * 0.01;  // Slow approach
    temperature += (random(-10, 10) / 100.0);  // Small noise

    // Vibration: sinusoidal oscillation (machinery)
    vibrationPhase += 0.3;
    vibration = 5.0 * sin(vibrationPhase);
    vibration += 2.0 * sin(vibrationPhase * 3.7);  // Harmonics
    vibration += (random(-10, 10) / 10.0);  // Noise

    // Pressure: step changes (valve operations)
    if (random(100) < 1) {
        // Random pressure target (simulates valve operation)
        pressureTarget = 950 + random(100);  // 950-1050 hPa
    }
    pressure += (pressureTarget - pressure) * 0.05;  // Medium approach
    pressure += (random(-5, 5) / 10.0);  // Small noise

    // Status: random on/off changes
    if (now - lastStatusChange > 3000 + random(5000)) {
        status = !status;
        lastStatusChange = now;
    }
}
