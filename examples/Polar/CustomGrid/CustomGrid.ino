/**
 * CustomGrid.ino
 * @version V1R1 R0.2
 * @date 03-28-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * CustomGrid.ino - Polar grid and angular-units demonstration
 *
 * Generates two animated polar traces to demonstrate how grid styling, angular
 * offsets, and unit systems work in ViewPoint. It is specifically meant to show
 * how degrees and radians affect every angular value you send to the plotting API.
 *
 * This demonstrates:
 * - Polar Frames mode with custom grid styling
 * - Degrees versus radians configuration
 * - Angular offsets, steps, and reference circles
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
 * - Toggle USE_RADIANS to compare degree versus radian behavior.
 * - Change the angular step size to see how the polar grid responds.
 * - Enable Persistence in the ViewPoint properties panel.
 * - Set radial divisions to 1 and enable reference circles.
 * - Increase the number of petals to explore harmonic patterns.
 */

#include "ViewPoint.h"

// Toggle to test both angular unit systems
const bool USE_RADIANS = true;

const int packetSize = 360;
const float maxRadius = 120.0f;
float phaseShift = 0.0f;
int petals = 2;

void setup() {
    // Mode::Frames draws the entire shape each frame,
    // making it ideal for parametric or analytical curves
    view.begin(viewpoint::PlotType::Polar, viewpoint::Mode::Frames, packetSize);
    view.setTitle("Custom Grid");
    view.setDelay(50);

    view.setPlotTitle(USE_RADIANS
        ? "Polar Test — Radians Mode"
        : "Polar Test — Degrees Mode");

    if (USE_RADIANS) {
        // Switch the plot into radians mode.
        // From this point on, ALL angular values passed to ViewPoint
        // (offsets, steps, and data angles) must be in radians — except
        // setAngularStep, which today is always in whole degrees (see
        // notes/TODOs.md for the radians-step follow-up).
        view.setRadians();
        view.setAngularOffset(M_PI / 2.0f);
    } else {
        // Default degree mode — all angular values are interpreted as degrees.
        view.setAngularOffset(90.0f);
    }
    view.setAngularStep(45);   // grid every 45 degrees, regardless of unit mode
    

    // Styling
    view.trace("Harmonic").setColor(viewpoint::colors::Cyan);
    view.trace("OuterRim").setColor(viewpoint::colors::Silver);
}

void loop() {
    for (int i = 0; i < packetSize; i++) {
        float thetaValue;  // Angle passed to ViewPoint (units depend on mode)
        float mathTheta;   // Angle passed to cos(), always radians

        if (USE_RADIANS) {
            thetaValue = (i / (float)packetSize) * 2.0f * M_PI;
            mathTheta = thetaValue;
        } else {
            thetaValue = (float)i;               // 0–359 degrees
            mathTheta = thetaValue * M_PI / 180.0f;
        }

        // Harmonic polar curve (rose / spirograph-style shape)
        // Phase shift is applied to animate rotation
        float rho = cos(petals * mathTheta + phaseShift) * maxRadius;

        // Constant-radius reference circle
        float rim = maxRadius;

        view.addData("Harmonic", rho, thetaValue);
        view.addData("OuterRim", rim, thetaValue);
    }

    view.send();

    // Advance phase to animate rotation
    phaseShift += 0.05f;

    // Periodically change the number of petals
    if (phaseShift > 2.0f * M_PI) {
        phaseShift = 0.0f;
        petals = (petals % 6) + 1;
    }
}