#ifndef VIEWSAUCE_INTRO_H
#define VIEWSAUCE_INTRO_H

#include "Config.h"
#include "Font.h"

// ─── Intro Configuration ─────────────────────────────────────────────
#define INTRO_FRAME_DELAY   50      // ms between frames (~20fps)
#define ORBIT_RADIUS        15.0f
#define ORBIT_SWING         10.0f
#define ROTATION_COUPLING   0.3f
#define BASE_SCALE          1.2f
#define SCALE_SWING         0.6f
#define BREATHE_CYCLES      4
#define HUE_RATE            0.4f
#define INTRO_TRANSITION    30      // frames to ramp scale at phase boundaries

// Phase durations in frames (at 20fps)
#define INTRO_PHASE1_FRAMES  120    // 6s — "VoidLoop" orbits CW
#define INTRO_PHASE2_FRAMES  120    // 6s — "ViewPoint" orbits CCW
#define INTRO_PHASE3_FRAMES  100    // 5s — both stacked, breathing
#define INTRO_TOTAL_FRAMES   (INTRO_PHASE1_FRAMES + INTRO_PHASE2_FRAMES + INTRO_PHASE3_FRAMES)

// ─── Intro State ─────────────────────────────────────────────────────
const char* STR_VOIDLOOP  = "VoidLoop";
const char* STR_VIEWPOINT = "ViewPoint";

float intro_text_x     = 0.0f;
float intro_text_y     = 0.0f;
float intro_text_angle = 0.0f;
float intro_text_scale = 1.5f;

// Transform a font-grid coordinate to world space.
void transform_pt(float gx, float gy, float origin_x, float origin_y,
                   float sc, float angle, float tx, float ty,
                   float& wx, float& wy) {
    float px = (gx + origin_x) * sc;
    float py = (gy + origin_y) * sc;
    float ca = cos(angle);
    float sa = sin(angle);
    wx = px * ca - py * sa + tx;
    wy = px * sa + py * ca + ty;
}

// Render a string as scatter polylines with segment breaks.
void draw_string(const char* str, const char* trace_name,
                 float origin_x, float origin_y, float sc, float angle,
                 float tx, float ty) {
    float cursor_x = 0;

    for (int i = 0; str[i]; i++) {
        char c = str[i];
        const Pt* pts = find_char(c);
        uint8_t w = char_width(c);

        if (c == ' ') { cursor_x += w + 1; continue; }

        bool in_stroke = false;
        for (int p = 0; ; p++) {
            int8_t px = pts[p].x;
            int8_t py = pts[p].y;

            if (px == -2 && py == -2) {
                if (in_stroke) view.addBreak(trace_name);
                break;
            }
            if (px == -1 && py == -1) {
                if (in_stroke) view.addBreak(trace_name);
                in_stroke = false;
                continue;
            }

            float wx, wy;
            transform_pt((float)px + cursor_x, (float)py,
                         origin_x, origin_y, sc, angle, tx, ty, wx, wy);

            view.addData(trace_name,
                constrain(wx, -1e6f, 1e6f),
                constrain(wy, -1e6f, 1e6f));
            in_stroke = true;
        }
        cursor_x += w + 1;
    }
}

// Draw a string split across two traces at the given character index.
void draw_split(const char* str, int split_at,
                float origin_x, float origin_y, float sc, float angle,
                float tx, float ty) {
    char buf1[10] = {0};
    char buf2[10] = {0};
    for (int i = 0; i < split_at; i++) buf1[i] = str[i];
    for (int i = split_at; str[i]; i++) buf2[i - split_at] = str[i];

    draw_string(buf1, "Text1", origin_x, origin_y, sc, angle, tx, ty);

    float half_w = 0;
    for (int i = 0; i < split_at; i++)
        half_w += char_width(str[i]) + 1;

    draw_string(buf2, "Text2", origin_x + half_w, origin_y, sc, angle, tx, ty);
}

// ─── Setup / Loop ────────────────────────────────────────────────────

void setup_intro() {
    view.setPlotType(PlotType::Scatter);
    view.setMode(Mode::Frames);
    view.setDelay(INTRO_FRAME_DELAY);
    view.setNumberOfPlots(1);
    view.setTitle("ViewSauce");
    view.setPlotTitle("Animated Text Banner");

    view.setHorizontalRange(-75, 75);
    view.setVerticalRange(-75, 75);
    
    view.trace("Text1").setColor(colors::Cyan);
    view.trace("Text2").setColor(colors::OrangeRed);
}

void loop_intro() {
    unsigned long phase = scene_frame % INTRO_TOTAL_FRAMES;

    // Cycle hue across both traces each frame
    float hue_base = viewpoint::wrapPositive(scene_frame * HUE_RATE, 360.0f);
    view.trace("Text1").setColor(hsv_to_color(hue_base, 0.9f, 1.0f));
    view.trace("Text2").setColor(hsv_to_color(hue_base + 150.0f, 0.9f, 1.0f));

    if (phase < INTRO_PHASE1_FRAMES) {
        // Phase 1: "VoidLoop" orbits clockwise with pulsing scale
        float frac = (float)phase / INTRO_PHASE1_FRAMES;
        float orbit_angle  = frac * 2.0f * M_PI;
        float orbit_radius = ORBIT_RADIUS + ORBIT_SWING * sin(frac * 4.0f * M_PI);

        intro_text_x     = orbit_radius * cos(orbit_angle);
        intro_text_y     = orbit_radius * sin(orbit_angle);
        intro_text_angle = orbit_angle * ROTATION_COUPLING;
        intro_text_scale = (BASE_SCALE + SCALE_SWING * sin(frac * 6.0f * M_PI))
                           * phase_envelope(phase, INTRO_PHASE1_FRAMES, INTRO_TRANSITION);

        float sw = string_width(STR_VOIDLOOP);
        draw_split(STR_VOIDLOOP, 4, -sw * 0.5f, -3.5f, intro_text_scale, intro_text_angle,
                   intro_text_x, intro_text_y);

    } else if (phase < INTRO_PHASE1_FRAMES + INTRO_PHASE2_FRAMES) {
        // Phase 2: "ViewPoint" orbits counter-clockwise with wider swing
        unsigned long local = phase - INTRO_PHASE1_FRAMES;
        float frac = (float)local / INTRO_PHASE2_FRAMES;
        float orbit_angle  = -frac * 2.0f * M_PI;
        float orbit_radius = 20.0f + 15.0f * sin(frac * 3.0f * M_PI);

        intro_text_x     = orbit_radius * cos(orbit_angle);
        intro_text_y     = orbit_radius * sin(orbit_angle);
        intro_text_angle = -orbit_angle * 0.2f;
        intro_text_scale = (1.5f + 1.0f * sin(frac * 5.0f * M_PI))
                           * phase_envelope(local, INTRO_PHASE2_FRAMES, INTRO_TRANSITION);

        float sw = string_width(STR_VIEWPOINT);
        draw_split(STR_VIEWPOINT, 4, -sw * 0.5f, -3.5f, intro_text_scale, intro_text_angle,
                   intro_text_x, intro_text_y);

    } else {
        // Phase 3: Both words stacked, gentle sway with breathing scale
        unsigned long local = phase - INTRO_PHASE1_FRAMES - INTRO_PHASE2_FRAMES;
        float frac = (float)local / INTRO_PHASE3_FRAMES;
        float breathe = sin(frac * BREATHE_CYCLES * M_PI);

        float env = phase_envelope(local, INTRO_PHASE3_FRAMES, INTRO_TRANSITION);
        intro_text_scale = (1.0f + 0.5f * breathe) * env;
        intro_text_angle = 0.15f * sin(frac * 2.0f * M_PI);
        intro_text_x     = 12.0f * sin(frac * 2.0f * M_PI);
        intro_text_y     =  6.0f * sin(frac * 4.0f * M_PI);

        float sw1 = string_width(STR_VOIDLOOP);
        draw_string(STR_VOIDLOOP, "Text1", -sw1 * 0.5f, 1.0f, intro_text_scale, intro_text_angle,
                    intro_text_x, intro_text_y);

        float sw2 = string_width(STR_VIEWPOINT);
        draw_split(STR_VIEWPOINT, 4, -sw2 * 0.5f, -8.0f, intro_text_scale, intro_text_angle,
                   intro_text_x, intro_text_y);
    }

    view.send();
}

#endif // VIEWSAUCE_INTRO_H
