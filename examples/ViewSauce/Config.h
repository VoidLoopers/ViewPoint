#ifndef VIEWSAUCE_CONFIG_H
#define VIEWSAUCE_CONFIG_H

#include <ViewPoint.h>
using namespace viewpoint;

// ─── Scene Definitions ───────────────────────────────────────────────
enum Scene : uint8_t {
    SCENE_INTRO,            // Scatter/Frames  — animated text banner
    SCENE_ECG,              // Cartesian/Cont   — ECG monitor
    SCENE_DASHBOARD,        // Cartesian/Cont   — multi-sensor dashboard
    SCENE_SPECTRUM,         // Cartesian/Frames — frequency spectrum (DSP)
    SCENE_DISPLAY_MODES,    // Cartesian/Frames — 3 display modes side-by-side
    SCENE_POLAR_SWEEP,      // Polar/Cont       — radar rose sweep
    SCENE_POLAR_WIND_ROSE,  // Polar/Frames     — wind rose histogram
    SCENE_LISSAJOUS,        // Scatter/Frames   — morphing Lissajous
    SCENE_HOLOGRAPHIC,      // Scatter/Frames   — 3D surface wireframes
    SCENE_FIREWORKS,        // Scatter/Frames   — particle fireworks
    SCENE_COUNT
};

// Duration of each scene in milliseconds (tunable)
const uint32_t SCENE_DURATION[] = {
    8000,  // INTRO
    5000,  // ECG
    8000,  // DASHBOARD
    3000,  // SPECTRUM
    18000,  // DISPLAY_MODES
    5000,  // POLAR_SWEEP
    10000,  // POLAR_WIND_ROSE
    15000,  // LISSAJOUS
    18000,  // HOLOGRAPHIC
    8000,  // FIREWORKS
};

// ─── Shared State ────────────────────────────────────────────────────
Scene          current_scene    = SCENE_INTRO;
unsigned long  scene_start_ms   = 0;
unsigned long  scene_frame      = 0;   // frames elapsed within current scene

// ─── Shared Utilities ────────────────────────────────────────────────

// HSV to packed RGB. Hue in degrees, saturation and value 0–1.
uint32_t hsv_to_color(float h, float s, float v) {
    while (h >= 360.0f) h -= 360.0f;
    while (h < 0.0f)    h += 360.0f;
    float c = v * s;
    float x = c * (1.0f - fabsf(viewpoint::wrapPositive(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r, g, b;
    if      (h < 60)  { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else              { r = c; g = 0; b = x; }
    return color((uint8_t)((r + m) * 255),
                 (uint8_t)((g + m) * 255),
                 (uint8_t)((b + m) * 255));
}

float random_float(float lo, float hi) {
    return lo + (hi - lo) * (float)random(0, 1001) / 1000.0f;
}

// Compute a 0–1 envelope that ramps up at the start and down at the end.
// Used for smooth scale-to-zero transitions between animation phases.
float phase_envelope(unsigned long local_frame, unsigned long phase_length,
                     unsigned long transition_frames = 30) {
    if (transition_frames == 0) return 1.0f;
    float env = 1.0f;
    if (local_frame < transition_frames)
        env = (float)local_frame / transition_frames;
    else if (local_frame > phase_length - transition_frames)
        env = (float)(phase_length - local_frame) / transition_frames;
    return constrain(env, 0.0f, 1.0f);
}

#endif // VIEWSAUCE_CONFIG_H
