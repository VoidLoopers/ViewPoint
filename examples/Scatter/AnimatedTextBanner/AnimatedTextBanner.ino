/**
 * AnimatedTextBanner.ino
 * @version 1.0.0
 * @date 03-10-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * AnimatedTextBanner.ino - Orbiting vector text animation on a Scatter plot
 *
 * Vector fonts render scalable, rotatable text as connected line segments — the
 * same technique oscilloscopes and CNC controllers use to draw characters without
 * bitmap memory. This sketch draws "VoidLoop" and "ViewPoint" as polyline strokes
 * on a Scatter plot, animating them through three phases: clockwise orbit,
 * counter-clockwise orbit, and a stacked breathing layout. A cycling HSV gradient
 * shifts color across both traces each frame.
 *
 * Scale-to-zero transitions between phases create smooth visual handoffs instead
 * of hard cuts, demonstrating how simple envelope shaping improves animation flow.
 *
 * This demonstrates:
 * - Scatter (XY) plotting in Frames mode
 * - addBreak() for disconnected line segments within a single trace
 * - Dynamic trace color changes via HSV-to-RGB conversion each frame
 * - 2D rotation and scaling transforms applied to polyline geometry
 * - Smooth phase transitions using a scale envelope
 *
 * Verified hardware:
 * - Teensy 4.1
 *
 * Known issues:
 * - None recorded for this sketch
 *
 * Board-specific requirements:
 * - Uses only core ViewPoint APIs and standard Arduino runtime features.
 * - A faster 32-bit board is recommended for smooth frame animation; only Teensy 4.1 has been hardware-verified for this sketch so far.
 *
 * Hardware:
 * - No external hardware required.
 *
 * Sketch outline:
 * - Setup: Configures Scatter/Frames plot with reference crosshairs
 * - Loop: Selects animation phase, computes orbit/scale/rotation per frame
 * - Font engine: Characters stored as polyline vertex lists with sentinel breaks
 * - Envelope: Scale ramps to zero at phase boundaries for smooth transitions
 *
 * Things to try:
 * - Change ORBIT_RADIUS to 30 and watch auto-scale chase the text offscreen
 * - Set ROTATION_COUPLING to 1.0 so text spins at full orbit rate
 * - Increase BREATHE_CYCLES to 8 for rapid pulsing in the stacked phase
 * - Try FRAME_DELAY_MS at 16 for ~60fps, or 100 for a slow-motion crawl
 * - Set TRANSITION_FRAMES to 0 to see the hard cuts the envelope eliminates
 */

#include <ViewPoint.h>
using namespace viewpoint;

/*
 * Vector font — each character is a set of polyline strokes on a 5-wide by 7-tall
 * grid. Vertices are {x, y} pairs. {-1,-1} separates strokes within a character.
 * {-2,-2} marks the end of the character definition.
 */
struct Pt { int8_t x, y; };

// clang-format off
const Pt CHAR_V[] = { {0,7},{2,0},{4,7}, {-2,-2} };
const Pt CHAR_O[] = { {1,0},{3,0},{4,1},{4,6},{3,7},{1,7},{0,6},{0,1},{1,0}, {-2,-2} };
const Pt CHAR_I[] = { {1,7},{3,7}, {-1,-1}, {2,7},{2,0}, {-1,-1}, {1,0},{3,0}, {-2,-2} };
const Pt CHAR_D[] = { {0,0},{0,7},{2,7},{4,5},{4,2},{2,0},{0,0}, {-2,-2} };
const Pt CHAR_L[] = { {0,7},{0,0},{4,0}, {-2,-2} };
const Pt CHAR_LOWER_L[] = { {2,7},{2,0}, {-1,-1}, {1,0},{3,0}, {-2,-2} };
const Pt CHAR_P[] = { {0,0},{0,7},{3,7},{4,6},{4,4},{3,3},{0,3}, {-2,-2} };
const Pt CHAR_E[] = { {4,7},{0,7},{0,0},{4,0}, {-1,-1}, {0,3},{3,3}, {-2,-2} };
const Pt CHAR_W[] = { {0,7},{1,0},{2,4},{3,0},{4,7}, {-2,-2} };
const Pt CHAR_N[] = { {0,0},{0,7},{4,0},{4,7}, {-2,-2} };
const Pt CHAR_T[] = { {0,7},{4,7}, {-1,-1}, {2,7},{2,0}, {-2,-2} };
const Pt CHAR_SPACE[] = { {-2,-2} };
// clang-format on

struct CharDef { char ch; const Pt* pts; uint8_t width; };

const CharDef font[] = {
    {'V', CHAR_V, 5}, {'o', CHAR_O, 5}, {'i', CHAR_I, 4}, {'d', CHAR_D, 5},
    {'L', CHAR_L, 5}, {'l', CHAR_LOWER_L, 4}, {'p', CHAR_P, 5},
    {'e', CHAR_E, 5}, {'w', CHAR_W, 5}, {'P', CHAR_P, 5},
    {'n', CHAR_N, 5}, {'t', CHAR_T, 5}, {' ', CHAR_SPACE, 3},
};
const int FONT_SIZE = sizeof(font) / sizeof(font[0]);

const Pt* find_char(char c) {
    for (int i = 0; i < FONT_SIZE; i++)
        if (font[i].ch == c) return font[i].pts;
    return CHAR_SPACE;
}

uint8_t char_width(char c) {
    for (int i = 0; i < FONT_SIZE; i++)
        if (font[i].ch == c) return font[i].width;
    return 3;
}

// ─── Experimentation Levers ───
#define FRAME_DELAY_MS      16      // ms between frames — try 16 for 60fps
#define ORBIT_RADIUS        15.0    // base orbit distance from origin
#define ORBIT_SWING         10.0    // radial oscillation amplitude
#define ROTATION_COUPLING   0.3     // text tilt relative to orbit rate (0=none, 1=locked)
#define BASE_SCALE          1.2     // minimum text scale
#define SCALE_SWING         0.6     // scale oscillation amplitude
#define BREATHE_CYCLES      4       // breathing pulses in stacked phase
#define HUE_RATE            0.4     // degrees of hue shift per frame
#define TRANSITION_FRAMES   30      // frames to ramp scale at phase boundaries

#define PHASE1_FRAMES       600
#define PHASE2_FRAMES       600
#define PHASE3_FRAMES       900
#define TOTAL_FRAMES        (PHASE1_FRAMES + PHASE2_FRAMES + PHASE3_FRAMES)

// ─── State Variables ───
const char* STR_VOIDLOOP  = "VoidLoop";
const char* STR_VIEWPOINT = "ViewPoint";

float text_x     = 0.0;
float text_y     = 0.0;
float text_angle = 0.0;
float text_scale = 1.5;
unsigned long frame_count = 0;

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

// Transform a font-grid coordinate to world space.
void transform_pt(float gx, float gy, float origin_x, float origin_y,
                   float sc, float angle, float& wx, float& wy) {
    float px = (gx + origin_x) * sc;
    float py = (gy + origin_y) * sc;
    float ca = cos(angle);
    float sa = sin(angle);
    wx = px * ca - py * sa + text_x;
    wy = px * sa + py * ca + text_y;
}

float string_width(const char* str) {
    float w = 0;
    for (int i = 0; str[i]; i++)
        w += char_width(str[i]) + 1;
    return w > 0 ? w - 1 : 0;
}

// Render a string as scatter polylines. Each stroke is separated by addBreak().
void draw_string(const char* str, const char* trace_name,
                 float origin_x, float origin_y, float sc, float angle) {
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
                         origin_x, origin_y, sc, angle, wx, wy);

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
                float origin_x, float origin_y, float sc, float angle) {
    char buf1[10] = {0};
    char buf2[10] = {0};
    for (int i = 0; i < split_at; i++) buf1[i] = str[i];
    for (int i = split_at; str[i]; i++) buf2[i - split_at] = str[i];

    draw_string(buf1, "Text1", origin_x, origin_y, sc, angle);

    // Advance past the first half so the second trace starts in the right place
    float half_w = 0;
    for (int i = 0; i < split_at; i++)
        half_w += char_width(str[i]) + 1;

    draw_string(buf2, "Text2", origin_x + half_w, origin_y, sc, angle);
}

// Compute a 0–1 envelope that ramps up at the start and down at the end of a phase.
// This makes scale shrink to zero at transitions for smooth visual handoffs.
float phase_envelope(unsigned long local_frame, unsigned long phase_length) {
    float env = 1.0;
    if (TRANSITION_FRAMES > 0) {
        if (local_frame < (unsigned long)TRANSITION_FRAMES)
            env = (float)local_frame / TRANSITION_FRAMES;
        else if (local_frame > phase_length - (unsigned long)TRANSITION_FRAMES)
            env = (float)(phase_length - local_frame) / TRANSITION_FRAMES;
    }
    return constrain(env, 0.0f, 1.0f);
}

void setup() {
    randomSeed(analogRead(0));

    view.begin(921600, PlotType::Scatter, Mode::Frames);
    view.setTitle("Animated Text Banner");
    view.setDelay(FRAME_DELAY_MS);
    view.setPlotTitle("Animated Text Banner");
    view.setAxisLabels("X", "Y");

    view.setHorizontalRange(-75, 75, 10);
    view.setVerticalRange(-75, 75, 10);

    view.trace("Text1").setColor(colors::Cyan);
    view.trace("Text2").setColor(colors::OrangeRed);
}

void loop() {
    unsigned long phase = frame_count % TOTAL_FRAMES;

    // Cycle hue across both traces each frame
    float hue_base = viewpoint::wrapPositive(frame_count * HUE_RATE, 360.0f);
    view.trace("Text1").setColor(hsv_to_color(hue_base, 0.9, 1.0));
    view.trace("Text2").setColor(hsv_to_color(hue_base + 150.0, 0.9, 1.0));

    if (phase < PHASE1_FRAMES) {
        // Phase 1: "VoidLoop" orbits clockwise with pulsing scale
        float frac = (float)phase / PHASE1_FRAMES;
        float orbit_angle  = frac * 2.0 * M_PI;
        float orbit_radius = ORBIT_RADIUS + ORBIT_SWING * sin(frac * 4.0 * M_PI);

        text_x     = orbit_radius * cos(orbit_angle);
        text_y     = orbit_radius * sin(orbit_angle);
        text_angle = orbit_angle * ROTATION_COUPLING;
        text_scale = (BASE_SCALE + SCALE_SWING * sin(frac * 6.0 * M_PI))
                     * phase_envelope(phase, PHASE1_FRAMES);

        float sw = string_width(STR_VOIDLOOP);
        draw_split(STR_VOIDLOOP, 4, -sw * 0.5, -3.5, text_scale, text_angle);

    } else if (phase < PHASE1_FRAMES + PHASE2_FRAMES) {
        // Phase 2: "ViewPoint" orbits counter-clockwise with wider swing
        unsigned long local = phase - PHASE1_FRAMES;
        float frac = (float)local / PHASE2_FRAMES;
        float orbit_angle  = -frac * 2.0 * M_PI;
        float orbit_radius = 20.0 + 15.0 * sin(frac * 3.0 * M_PI);

        text_x     = orbit_radius * cos(orbit_angle);
        text_y     = orbit_radius * sin(orbit_angle);
        text_angle = -orbit_angle * 0.2;
        text_scale = (1.5 + 1.0 * sin(frac * 5.0 * M_PI))
                     * phase_envelope(local, PHASE2_FRAMES);

        float sw = string_width(STR_VIEWPOINT);
        draw_split(STR_VIEWPOINT, 4, -sw * 0.5, -3.5, text_scale, text_angle);

    } else {
        // Phase 3: Both words stacked, gentle sway with breathing scale
        unsigned long local = phase - PHASE1_FRAMES - PHASE2_FRAMES;
        float frac = (float)local / PHASE3_FRAMES;
        float breathe = sin(frac * BREATHE_CYCLES * M_PI);

        float env = phase_envelope(local, PHASE3_FRAMES);
        text_scale = (1.0 + 0.5 * breathe) * env;
        text_angle = 0.15 * sin(frac * 2.0 * M_PI);
        text_x     = 12.0 * sin(frac * 2.0 * M_PI);
        text_y     =  6.0 * sin(frac * 4.0 * M_PI);

        float sw1 = string_width(STR_VOIDLOOP);
        draw_string(STR_VOIDLOOP, "Text1", -sw1 * 0.5, 1.0, text_scale, text_angle);

        float sw2 = string_width(STR_VIEWPOINT);
        draw_split(STR_VIEWPOINT, 4, -sw2 * 0.5, -8.0, text_scale, text_angle);
    }

    view.send();
    frame_count++;
}
