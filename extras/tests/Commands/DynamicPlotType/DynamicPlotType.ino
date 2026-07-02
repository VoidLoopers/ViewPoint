/**
 * DynamicPlotType.ino
 * @date 03-10-26
 *
 * ViewPoint Command Test: Dynamic Plot Type Switching
 *
 * Changes the plot type every 3 seconds, cycling through Cartesian,
 * Scatter, and Polar. This tests whether the app can handle a plot
 * type change after initial configuration—most apps assume plot type
 * is set once in setup(). The data format changes with each type:
 * single values for Cartesian, paired XY for Scatter, paired
 * radius/angle for Polar. Also tests if configuration commands
 * (labels, units, ranges) are re-applied correctly after a type switch.
 *
 * Cycle: Cartesian → Scatter → Polar → Cartesian → ...
 *
 * Mode: Continuous
 * Hardware: Any Arduino
 *
 * Things to try:
 * - Does the plot type actually change in the app?
 * - Are axis labels updated to match the new type?
 * - Does data render correctly after each transition?
 * - Is there a visual glitch or flash during the switch?
 * - Does the app handle the data format change (1 value vs 2 values)?
 */

#include <ViewPoint.h>
using namespace viewpoint;

// --- Timing ---
const unsigned long SWITCH_INTERVAL_MS = 3000;

// --- State ---
PlotType currentType = PlotType::Cartesian;
unsigned long lastSwitch = 0;
float phase = 0.0;
float angle = 0.0;
int typeIndex = 0;

// --- Forward declarations ---
void applyTypeConfig(PlotType type);

void setup() {
    randomSeed(analogRead(0));

    view.begin(PlotType::Cartesian, Mode::Continuous);
    view.setDelay(5);

    view.trace("Signal").setColor(colors::Coral);

    applyTypeConfig(PlotType::Cartesian);
    lastSwitch = millis();
}

void loop() {
    unsigned long now = millis();

    // Switch plot type
    if (now - lastSwitch >= SWITCH_INTERVAL_MS) {
        typeIndex = (typeIndex + 1) % 3;

        switch (typeIndex) {
            case 0: currentType = PlotType::Cartesian; break;
            case 1: currentType = PlotType::Scatter;   break;
            case 2: currentType = PlotType::Polar;     break;
        }

        view.setPlotType(currentType);
        applyTypeConfig(currentType);
        lastSwitch = now;
    }

    // Generate data appropriate to current type
    phase += 0.05;
    if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;

    angle += 1.5;
    if (angle >= 360.0) angle -= 360.0;

    switch (currentType) {
        case PlotType::Cartesian: {
            float value = sin(phase) * 5.0 + cos(phase * 3.0) * 2.0;
            view.addData("Signal", value);
            break;
        }

        case PlotType::Scatter: {
            float x = sin(phase) * 5.0;
            float y = cos(phase * 1.5) * 3.0;
            view.addData("Signal", x, y);
            break;
        }

        case PlotType::Polar: {
            float radius = 3.0 + sin(phase * 2.0) * 2.0;
            view.addData("Signal", radius, angle);
            break;
        }
    }

    view.send();
}

void applyTypeConfig(PlotType type) {
    switch (type) {
        case PlotType::Cartesian:
            view.setPlotTitle("Cartesian Mode");
            view.setAxisLabels("Time", "Amplitude");
            view.setUnits("s", "V");
            break;

        case PlotType::Scatter:
            view.setPlotTitle("Scatter Mode");
            view.setAxisLabels("X", "Y");
            view.setUnits("V", "V");
            break;

        case PlotType::Polar:
            view.setPlotTitle("Polar Mode");
            view.setAngularOffset(90);
            view.setAngularStep(30);
            break;
    }

    const char* names[] = {"Cartesian", "Scatter", "Polar"};
    view.sendInfo("Type Switch", names[typeIndex]);
}
