/**
 * ReconfigInLoop.ino
 * @date 03-10-26
 *
 * ViewPoint Command Test: Configuration Commands Inside loop()
 *
 * Most sketches configure the plotter once in setup() and only send
 * data in loop(). This test sends configuration commands mixed with
 * data in the main loop—changing titles, axis labels, units, ranges,
 * grid colors, and reference lines while data is actively streaming.
 * This stresses the app's ability to apply config changes mid-stream
 * without losing data or corrupting state.
 *
 * Every 2 seconds, one configuration parameter changes:
 * - Title cycles through 5 different strings
 * - Axis labels swap between scientific and casual names
 * - Units change (V, mV, dBm, %, arbitrary)
 * - Vertical range toggles between auto and fixed
 * - Grid colors cycle through color schemes
 * - Reference lines are added, moved, and removed
 *
 * Mode: Continuous
 * Hardware: Any Arduino
 *
 * Things to try:
 * - Does the title update in real-time?
 * - Do axis labels change without a restart?
 * - Does toggling between auto-scale and fixed range work smoothly?
 * - Do reference lines accumulate or replace correctly?
 * - Is there visible data loss during config updates?
 */

#include <ViewPoint.h>
using namespace viewpoint;

// --- Timing ---
const unsigned long CONFIG_INTERVAL_MS = 2000;

// --- Config parameter to change ---
enum ConfigParam : uint8_t {
    CFG_TITLE,
    CFG_LABELS,
    CFG_UNITS,
    CFG_RANGE,
    CFG_GRID_COLORS,
    CFG_REF_LINES,
    CFG_COUNT
};

// --- State ---
ConfigParam nextConfig = CFG_TITLE;
unsigned long lastConfigTime = 0;
float phase = 0.0;
int titleIdx = 0;
int labelIdx = 0;
int unitsIdx = 0;
int rangeIdx = 0;
int gridIdx = 0;
int refIdx = 0;
float refLineValue = 0.0;

void setup() {
    randomSeed(analogRead(0));

    view.begin(Mode::Continuous);
    view.setDelay(2);
    view.setPlotTitle("ReconfigInLoop — Watch Things Change");
    view.setAxisLabels("Time", "Value");
    view.setUnits("s", "V");

    view.trace("Stream").setColor(colors::Coral);
    view.trace("Noise").setColor(colors::SlateBlue);

    lastConfigTime = millis();
}

void loop() {
    unsigned long now = millis();

    // Apply a different config change every interval
    if (now - lastConfigTime >= CONFIG_INTERVAL_MS) {
        applyConfigChange();
        nextConfig = static_cast<ConfigParam>((nextConfig + 1) % CFG_COUNT);
        lastConfigTime = now;
    }

    // Continuous data generation
    phase += 0.04;
    if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;

    float signal = sin(phase) * 5.0 + sin(phase * 3.0) * 1.5;
    float noise = signal + (random(-100, 100) / 50.0);

    view.addData("Stream", signal);
    view.addData("Noise", noise);
    view.send();
}

void applyConfigChange() {
    switch (nextConfig) {
        case CFG_TITLE: {
            const char* titles[] = {
                "ReconfigInLoop — Title A",
                "Dynamic Config Test",
                "Watch This Change!",
                "Config Stress Test",
                "Title #5"
            };
            titleIdx = (titleIdx + 1) % 5;
            view.setPlotTitle(titles[titleIdx]);
            view.sendInfo("Config", "Title changed");
            break;
        }

        case CFG_LABELS: {
            const char* xLabels[] = {"Time", "Samples", "Index", "Elapsed"};
            const char* yLabels[] = {"Voltage", "Amplitude", "Level", "Magnitude"};
            labelIdx = (labelIdx + 1) % 4;
            view.setAxisLabels(xLabels[labelIdx], yLabels[labelIdx]);
            view.sendInfo("Config", "Labels changed");
            break;
        }

        case CFG_UNITS: {
            const char* xUnits[] = {"s", "ms", "us", "samples", ""};
            const char* yUnits[] = {"V", "mV", "dBm", "%", "arb"};
            unitsIdx = (unitsIdx + 1) % 5;
            view.setUnits(xUnits[unitsIdx], yUnits[unitsIdx]);
            view.sendInfo("Config", "Units changed");
            break;
        }

        case CFG_RANGE: {
            rangeIdx = (rangeIdx + 1) % 3;
            switch (rangeIdx) {
                case 0:
                    // No fixed range (auto-scale)
                    // Can't "unset" range, so set very wide
                    view.setVerticalRange(-100, 100);
                    view.sendInfo("Config", "Range: wide");
                    break;
                case 1:
                    view.setVerticalRange(-10, 10, 4);
                    view.sendInfo("Config", "Range: -10 to 10");
                    break;
                case 2:
                    view.setVerticalRange(-3, 3, 6);
                    view.sendInfo("Config", "Range: -3 to 3 (clips!)");
                    break;
            }
            break;
        }

        case CFG_GRID_COLORS: {
            gridIdx = (gridIdx + 1) % 3;
            switch (gridIdx) {
                case 0:
                    view.setGridColors(0x333333, 0x666666);
                    break;
                case 1:
                    view.setGridColors(0x003300, 0x006600);  // Green theme
                    break;
                case 2:
                    view.setGridColors(0x330000, 0x660000);  // Red theme
                    break;
            }
            view.sendInfo("Config", "Grid colors changed");
            break;
        }

        case CFG_REF_LINES: {
            refIdx = (refIdx + 1) % 4;
            // Move the reference line to a new position each time
            refLineValue = sin(refIdx * 1.5) * 5.0;
            // Note: reference lines accumulate — this tests if the app handles growing lists
            view.addHorizontalReferenceLine(refLineValue, colors::Red, 1.5);
            view.sendInfo("Config", "Ref line added");
            break;
        }
    }
}
