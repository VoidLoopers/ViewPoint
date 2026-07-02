/**
 * ModeSwitching.ino
 * @date 03-10-26
 *
 * ViewPoint Command Test: Continuous ↔ Frames Mode Switching
 *
 * Toggles between Continuous and Frames mode every 4 seconds while
 * maintaining the same underlying signal. This tests the app's ability
 * to handle a mode change mid-session—resetting internal buffers,
 * switching between scrolling and frame-replacement rendering, and
 * adapting to the different data cadence of each mode.
 *
 * The signal is a dual-frequency sine so you can visually confirm it
 * looks the same in both modes. In Frames mode, a full 256-point
 * waveform is sent per frame. In Continuous mode, one sample at a time.
 *
 * Cycle: Continuous → Frames → Continuous → ...
 *
 * Mode: Alternating
 * Hardware: Any Arduino
 *
 * Things to try:
 * - Does the waveform look the same in both modes?
 * - Is there a visible gap or glitch at the transition?
 * - Does the X-axis reset or jump when switching?
 * - Does auto-scale behave consistently across mode changes?
 */

#include <ViewPoint.h>
using namespace viewpoint;

// --- Constants ---
const unsigned long SWITCH_INTERVAL_MS = 4000;
const int FRAME_SIZE = 256;

// --- State ---
Mode currentMode = Mode::Continuous;
unsigned long lastSwitch = 0;
float phase = 0.0;
float frameBuffer[FRAME_SIZE];

void setup() {
    randomSeed(analogRead(0));

    view.begin(Mode::Continuous);
    view.setDelay(2);
    view.setPlotTitle("Mode Switching Test");
    view.setAxisLabels("Time", "Signal");
    view.setUnits("ms", "V");

    view.trace("Waveform").setColor(colors::DodgerBlue);
    view.trace("Envelope").setColor(colors::Gold);

    lastSwitch = millis();
}

void loop() {
    unsigned long now = millis();

    // Toggle mode
    if (now - lastSwitch >= SWITCH_INTERVAL_MS) {
        if (currentMode == Mode::Continuous) {
            currentMode = Mode::Frames;
            view.setMode(Mode::Frames);
            view.setPacketSize(FRAME_SIZE);
            view.setDelay(30);
            view.sendInfo("Mode", "Switched to Frames");
        } else {
            currentMode = Mode::Continuous;
            view.setMode(Mode::Continuous);
            view.setDelay(2);
            view.sendInfo("Mode", "Switched to Continuous");
        }
        lastSwitch = now;
    }

    if (currentMode == Mode::Continuous) {
        // Single sample at a time
        phase += 0.08;
        if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;

        float signal = sin(phase) * 5.0 + sin(phase * 3.0) * 2.0;
        float envelope = abs(sin(phase * 0.1)) * 6.0;

        view.addData("Waveform", signal);
        view.addData("Envelope", envelope);
        view.send();
    } else {
        // Full frame
        float startPhase = phase;
        for (int i = 0; i < FRAME_SIZE; i++) {
            float p = startPhase + i * 0.08;
            frameBuffer[i] = sin(p) * 5.0 + sin(p * 3.0) * 2.0;
        }
        phase = startPhase + FRAME_SIZE * 0.08;
        // Wrap phase
        while (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;

        view.addData("Waveform", frameBuffer, FRAME_SIZE);

        // Envelope as single value in frame context
        float envFrame[FRAME_SIZE];
        for (int i = 0; i < FRAME_SIZE; i++) {
            float p = startPhase + i * 0.08;
            envFrame[i] = abs(sin(p * 0.1)) * 6.0;
        }
        view.addData("Envelope", envFrame, FRAME_SIZE);

        view.send();
    }
}
