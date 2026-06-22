/**
 * ScatterTextBannerSlow.ino
 * @date 03-10-26
 *
 * ViewPoint Scatter Test: Frames — Animated Text Banner (Slow)
 *
 * Slow, corrected-orientation version of ScatterTextBanner. The text
 * reads left-to-right and the animation runs at a relaxed pace so each
 * phase can be observed clearly.
 *
 * Draws "VoidLoop" then "ViewPoint" then both stacked, orbiting and
 * scaling with a smooth HSV color gradient cycling across two traces.
 *
 * Animation phases:
 * - Phase 1: "VoidLoop" orbits the plot (slow clockwise)
 * - Phase 2: "ViewPoint" orbits in the opposite direction
 * - Phase 3: "VoidLoop ViewPoint" on two lines, breathing scale
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch the color gradient shift across the letters
 * - Notice how auto-scale tracks the text as it orbits far from origin
 * - See the scale jump when switching between single-word and two-word
 * - Try different auto-scale strategies with the structured geometry
 */

#include <ViewPoint.h>
using namespace viewpoint;

// ======================== Vector Font ========================
// Each character is stored as polylines in a 5-wide x 7-tall grid.
// A stroke is a sequence of {x,y} pairs. Strokes are separated by
// {-1,-1} sentinel. The character ends with {-2,-2}.

struct Pt { int8_t x, y; };

// clang-format off

const Pt CHAR_V[] = {
    {0,7},{2,0},{4,7},  {-2,-2}
};

const Pt CHAR_O[] = {
    {1,0},{3,0},{4,1},{4,6},{3,7},{1,7},{0,6},{0,1},{1,0},  {-2,-2}
};

const Pt CHAR_I[] = {
    {1,7},{3,7},  {-1,-1},
    {2,7},{2,0},  {-1,-1},
    {1,0},{3,0},  {-2,-2}
};

const Pt CHAR_D[] = {
    {0,0},{0,7},{2,7},{4,5},{4,2},{2,0},{0,0},  {-2,-2}
};

const Pt CHAR_L[] = {
    {0,7},{0,0},{4,0},  {-2,-2}
};

const Pt CHAR_LOWER_L[] = {
    {2,7},{2,0},  {-1,-1},
    {1,0},{3,0},  {-2,-2}
};

const Pt CHAR_P[] = {
    {0,0},{0,7},{3,7},{4,6},{4,4},{3,3},{0,3},  {-2,-2}
};

const Pt CHAR_E[] = {
    {4,7},{0,7},{0,0},{4,0},  {-1,-1},
    {0,3},{3,3},  {-2,-2}
};

const Pt CHAR_W[] = {
    {0,7},{1,0},{2,4},{3,0},{4,7},  {-2,-2}
};

const Pt CHAR_N[] = {
    {0,0},{0,7},{4,0},{4,7},  {-2,-2}
};

const Pt CHAR_T[] = {
    {0,7},{4,7},  {-1,-1},
    {2,7},{2,0},  {-2,-2}
};

const Pt CHAR_SPACE[] = {
    {-2,-2}
};

// clang-format on

struct CharDef {
    char ch;
    const Pt* pts;
    uint8_t width;
};

const CharDef font[] = {
    {'V', CHAR_V, 5},
    {'o', CHAR_O, 5},
    {'i', CHAR_I, 4},
    {'d', CHAR_D, 5},
    {'L', CHAR_L, 5},
    {'l', CHAR_LOWER_L, 4},
    {'p', CHAR_P, 5},
    {'e', CHAR_E, 5},
    {'w', CHAR_W, 5},
    {'P', CHAR_P, 5},
    {'n', CHAR_N, 5},
    {'t', CHAR_T, 5},
    {' ', CHAR_SPACE, 3},
};
const int FONT_SIZE = sizeof(font) / sizeof(font[0]);

const Pt* findChar(char c) {
    for (int i = 0; i < FONT_SIZE; i++) {
        if (font[i].ch == c) return font[i].pts;
    }
    return CHAR_SPACE;
}

uint8_t charWidth(char c) {
    for (int i = 0; i < FONT_SIZE; i++) {
        if (font[i].ch == c) return font[i].width;
    }
    return 3;
}

// ======================== Strings ========================

const char* STR_VOIDLOOP   = "VoidLoop";
const char* STR_VIEWPOINT  = "ViewPoint";

// ======================== Animation ========================

float textScale = 1.5;
float textX = 0.0;
float textY = 0.0;
float textAngle = 0.0;

unsigned long frameCount = 0;

// 3x longer phases for relaxed pace
#define PHASE1_FRAMES 600
#define PHASE2_FRAMES 600
#define PHASE3_FRAMES 900
#define TOTAL_FRAMES  (PHASE1_FRAMES + PHASE2_FRAMES + PHASE3_FRAMES)

// ======================== Helpers ========================

uint32_t hsvToColor(float h, float s, float v) {
    while (h >= 360.0) h -= 360.0;
    while (h < 0.0) h += 360.0;
    float c = v * s;
    float x = c * (1.0 - fabs(fmod(h / 60.0, 2.0) - 1.0));
    float m = v - c;
    float r, g, b;
    if      (h < 60)  { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else              { r = c; g = 0; b = x; }
    return color((uint8_t)((r + m) * 255), (uint8_t)((g + m) * 255), (uint8_t)((b + m) * 255));
}

// Transform a font-grid point to world space.
void transformPt(float gx, float gy, float ox, float oy,
                 float sc, float angle, float& wx, float& wy) {
    float px = (gx + ox) * sc;
    float py = (gy + oy) * sc;
    float ca = cos(angle);
    float sa = sin(angle);
    wx = px * ca - py * sa + textX;
    wy = px * sa + py * ca + textY;
}

float stringWidth(const char* str) {
    float w = 0;
    for (int i = 0; str[i]; i++) {
        w += charWidth(str[i]) + 1;
    }
    return w > 0 ? w - 1 : 0;
}

void drawString(const char* str, const char* traceName,
                float originX, float originY, float sc, float angle) {
    float cursorX = 0;

    for (int i = 0; str[i]; i++) {
        char c = str[i];
        const Pt* pts = findChar(c);
        uint8_t w = charWidth(c);

        if (c == ' ') {
            cursorX += w + 1;
            continue;
        }

        bool inStroke = false;
        for (int p = 0; ; p++) {
            int8_t px = pts[p].x;
            int8_t py = pts[p].y;

            if (px == -2 && py == -2) {
                if (inStroke) view.addBreak(traceName);
                break;
            }

            if (px == -1 && py == -1) {
                if (inStroke) view.addBreak(traceName);
                inStroke = false;
                continue;
            }

            float gx = (float)px + cursorX;
            float gy = (float)py;
            float wx, wy;
            transformPt(gx, gy, originX, originY, sc, angle, wx, wy);

            view.addData(traceName,
                constrain(wx, -1e6f, 1e6f),
                constrain(wy, -1e6f, 1e6f));
            inStroke = true;
        }

        cursorX += w + 1;
    }
}

// ======================== Setup & Loop ========================

void setup() {
    randomSeed(analogRead(0));
    view.begin(PlotType::Scatter, Mode::Frames);
    view.setDelay(50);   // Slower frame rate

    view.addHorizontalReferenceLine(0.0);
    view.addVerticalReferenceLine(0.0);

    view.setAxisLabels("X", "Y");
    view.setPlotTitle("Text Banner");

    view.trace("Text1").setColor(colors::Cyan);
    view.trace("Text2").setColor(colors::OrangeRed);
}

void loop() {
    unsigned long phase = frameCount % TOTAL_FRAMES;

    // Slow hue cycling (0.4 deg/frame vs 1.2)
    float hueBase = fmod(frameCount * 0.4, 360.0);
    view.trace("Text1").setColor(hsvToColor(hueBase, 0.9, 1.0));
    view.trace("Text2").setColor(hsvToColor(hueBase + 150.0, 0.9, 1.0));

    if (phase < PHASE1_FRAMES) {
        // Phase 1: "VoidLoop" orbiting clockwise
        float frac = (float)phase / PHASE1_FRAMES;
        float orbitAngle = frac * 2.0 * M_PI;
        float orbitRadius = 15.0 + 10.0 * sin(frac * 4.0 * M_PI);

        textX = orbitRadius * cos(orbitAngle);
        textY = orbitRadius * sin(orbitAngle);
        textAngle = orbitAngle * 0.3;
        textScale = 1.2 + 0.6 * sin(frac * 6.0 * M_PI);

        float sw = stringWidth(STR_VOIDLOOP);
        float ox = -sw * 0.5;
        float oy = -3.5;

        char buf1[5] = {0}, buf2[5] = {0};
        for (int i = 0; i < 4; i++) buf1[i] = STR_VOIDLOOP[i];
        for (int i = 0; i < 4; i++) buf2[i] = STR_VOIDLOOP[i + 4];

        drawString(buf1, "Text1", ox, oy, textScale, textAngle);

        float halfW = 0;
        for (int i = 0; i < 4; i++) halfW += charWidth(STR_VOIDLOOP[i]) + 1;
        drawString(buf2, "Text2", ox + halfW, oy, textScale, textAngle);

    } else if (phase < PHASE1_FRAMES + PHASE2_FRAMES) {
        // Phase 2: "ViewPoint" orbiting counter-clockwise
        float frac = (float)(phase - PHASE1_FRAMES) / PHASE2_FRAMES;
        float orbitAngle = -frac * 2.0 * M_PI;
        float orbitRadius = 20.0 + 15.0 * sin(frac * 3.0 * M_PI);

        textX = orbitRadius * cos(orbitAngle);
        textY = orbitRadius * sin(orbitAngle);
        textAngle = -orbitAngle * 0.2;
        textScale = 1.5 + 1.0 * sin(frac * 5.0 * M_PI);

        float sw = stringWidth(STR_VIEWPOINT);
        float ox = -sw * 0.5;
        float oy = -3.5;

        char buf1[5] = {0}, buf2[6] = {0};
        for (int i = 0; i < 4; i++) buf1[i] = STR_VIEWPOINT[i];
        for (int i = 0; i < 5; i++) buf2[i] = STR_VIEWPOINT[i + 4];

        drawString(buf1, "Text1", ox, oy, textScale, textAngle);

        float halfW = 0;
        for (int i = 0; i < 4; i++) halfW += charWidth(STR_VIEWPOINT[i]) + 1;
        drawString(buf2, "Text2", ox + halfW, oy, textScale, textAngle);

    } else {
        // Phase 3: "VoidLoop" on top, "ViewPoint" below, breathing
        float frac = (float)(phase - PHASE1_FRAMES - PHASE2_FRAMES) / PHASE3_FRAMES;
        float breathe = sin(frac * 4.0 * M_PI);

        textScale = 1.0 + 0.5 * breathe;
        textAngle = 0.15 * sin(frac * 2.0 * M_PI);

        textX = 12.0 * sin(frac * 2.0 * M_PI);
        textY = 6.0 * sin(frac * 4.0 * M_PI);

        float sw1 = stringWidth(STR_VOIDLOOP);
        drawString(STR_VOIDLOOP, "Text1", -sw1 * 0.5, 1.0, textScale, textAngle);

        float sw2 = stringWidth(STR_VIEWPOINT);
        drawString(STR_VIEWPOINT, "Text2", -sw2 * 0.5, -8.0, textScale, textAngle);
    }

    view.send();
    frameCount++;
}
