/**
 * ViewSauce.ino
 * @version 1.0.0
 * @date 03-22-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * ViewSauce.ino - Full-feature showcase cycling through every ViewPoint capability
 *
 * A "greatest hits" demo that cycles through all three plot types (Cartesian,
 * Polar, Scatter), both modes (Continuous and Frames), display modes
 * (Persistence, Spectrogram, Gradient), multi-plot layouts, and advanced
 * visualizations including real-time FFT spectrum analysis, 3D wireframe
 * rendering, and particle-physics fireworks.
 *
 * The sketch runs as a state machine with 10 scenes, each showcasing
 * different ViewPoint features. Each scene transition calls reset() to
 * clear all traces and configuration, then fully reconfigures from scratch.
 * This ensures clean state between scenes with no leaking settings.
 *
 * This demonstrates:
 * - All 3 plot types: Cartesian, Scatter (XY), and Polar
 * - Both modes: Continuous streaming and Frames (complete replacement)
 * - All 3 display modes: Persistence, Spectrogram, Gradient
 * - Multi-plot layouts (up to 4 stacked plots)
 * - Real-time FFT with DSP library (SignalGenerator + FFT)
 * - Runtime reconfiguration: plot type, mode, and layout switching mid-sketch
 * - Dynamic trace coloring, reference lines, custom grid colors
 * - Analytical kinematics, parametric surfaces, vector font rendering
 *
 * Verified hardware:
 * - Teensy 4.1
 *
 * Known issues:
 * - None recorded for this sketch
 *
 * Board-specific requirements:
 * - Uses SignalCore plus multiple frame-based scenes and benefits from a fast 32-bit board with generous RAM.
 * - Only Teensy 4.1 has been hardware-verified for the complete scene sweep so far.
 *
 * This sketch uses the SignalCore DSP library: https://docs.voidloop.com/libraries/SignalCore/
 *
 * Hardware:
 * - No external hardware required.
 *
 * Sketch outline:
 * - Scene 0:  Animated "VoidLoop" + "ViewPoint" text banner (Scatter/Frames) — AnimatedTextBanner.ino
 * - Scene 1:  ECG monitor with pink grid (Cartesian/Continuous) — HeartbeatECG.ino
 * - Scene 2:  4-plot sensor dashboard (Cartesian/Continuous) — MultiSensorDashboard.ino
 * - Scene 3:  Sweeping frequency spectrum via FFT (Cartesian/Frames) — SpectrumAnalyzer.ino
 * - Scene 4:  3 display modes side-by-side (Cartesian/Frames) — SpectrumAnalyzer.ino
 * - Scene 5:  Radar rose sweep (Polar/Continuous)
 * - Scene 6:  Wind rose histogram (Polar/Frames) — WindRoseChart.ino
 * - Scene 7:  Morphing Lissajous figures (Scatter/Frames) — LissajousCurve.ino
 * - Scene 8:  Rotating 3D holographic surfaces (Scatter/Frames) — HolographicSurfaces.ino
 * - Scene 9:  Particle-physics fireworks finale (Scatter/Frames) — Fireworks.ino
 *
 * Things to try:
 * - Adjust SCENE_DURATION[] in Config.h to change pacing per scene
 * - Change SPEC_CARRIER_START/END in Cartesian.h for different sweep ranges
 * - Set HOLO_NUM_SHAPES to 1 in Scatter.h to stay on a single surface
 * - Change FW_LAUNCH_INTERVAL to 0.3 for a rapid-fire grand finale
 * - Modify LISS_MORPH_SECS to speed up or slow down Lissajous transitions
 *
 * Most scenes have a standalone example with more detail and experimentation
 * levers — see Examples/Cartesian, Examples/Scatter, and Examples/Polar.
 */

#define VIEWPOINT_FULL   // Force full memory profile (16 traces, 8 plots)
#include <ViewPoint.h>
#include <SignalCore.h>
using namespace viewpoint;
using namespace signalcore;

#include "Config.h"
#include "Font.h"
#include "Intro.h"
#include "Cartesian.h"
#include "Polar.h"
#include "Scatter.h"

// ─── Scene Setup Dispatcher ──────────────────────────────────────────
void setup_scene(Scene s) {
    view.reset();
    scene_frame = 0;
    switch (s) {
        case SCENE_INTRO:           setup_intro();              break;
        case SCENE_ECG:             setup_ecg();                break;
        case SCENE_DASHBOARD:       setup_dashboard();          break;
        case SCENE_SPECTRUM:        setup_spectrum();           break;
        case SCENE_DISPLAY_MODES:   setup_display_modes();      break;
        case SCENE_POLAR_SWEEP:     setup_polar_sweep();        break;
        case SCENE_POLAR_WIND_ROSE: setup_polar_wind_rose();    break;
        case SCENE_LISSAJOUS:       setup_lissajous();          break;
        case SCENE_HOLOGRAPHIC:     setup_holographic();        break;
        case SCENE_FIREWORKS:       setup_fireworks();          break;
        default: break;
    }
}

// ─── Scene Loop Dispatcher ───────────────────────────────────────────
void loop_scene(Scene s) {
    switch (s) {
        case SCENE_INTRO:           loop_intro();               break;
        case SCENE_ECG:             loop_ecg();                 break;
        case SCENE_DASHBOARD:       loop_dashboard();           break;
        case SCENE_SPECTRUM:        loop_spectrum();            break;
        case SCENE_DISPLAY_MODES:   loop_display_modes();       break;
        case SCENE_POLAR_SWEEP:     loop_polar_sweep();         break;
        case SCENE_POLAR_WIND_ROSE: loop_polar_wind_rose();     break;
        case SCENE_LISSAJOUS:       loop_lissajous();           break;
        case SCENE_HOLOGRAPHIC:     loop_holographic();         break;
        case SCENE_FIREWORKS:       loop_fireworks();           break;
        default: break;
    }
}

// ─── Arduino Entry Points ────────────────────────────────────────────

void setup() {
    randomSeed(analogRead(0));
    view.begin(PlotType::Scatter, Mode::Frames);
    setup_scene(SCENE_INTRO);
    scene_start_ms = millis();
}

void loop() {
    unsigned long now = millis();
    unsigned long elapsed = now - scene_start_ms;

    // Scene transition
    if (elapsed >= SCENE_DURATION[current_scene]) {
        current_scene = (Scene)((current_scene + 1) % SCENE_COUNT);
        scene_start_ms = now;
        setup_scene(current_scene);
    }

    // Run current scene
    loop_scene(current_scene);
    scene_frame++;
}
