/**
 * RapidColorCycle.ino
 * @date 03-10-26
 *
 * ViewPoint Command Test: Rapid Trace Color Changes
 *
 * Changes trace colors every frame while data is streaming. Tests whether
 * the app can apply color updates in real-time without flickering, lag,
 * or desynchronization between the color command and the data it applies
 * to. Includes several color change patterns: smooth HSV rainbow cycling,
 * random jumps, synchronized multi-trace changes, and rapid toggling
 * between two colors.
 *
 * Phases (5 seconds each):
 * - RAINBOW:     Smooth HSV hue rotation, color changes every 50ms
 * - RANDOM_JUMP: Random color each frame from the full 24-bit space
 * - STROBE:      Alternate between two high-contrast colors every frame
 * - SYNC_MULTI:  All 3 traces change color simultaneously
 * - SLOW_FADE:   Gradual brightness fade from white to black (grayscale)
 *
 * Mode: Continuous
 * Hardware: Any Arduino
 *
 * Things to try:
 * - Does RAINBOW produce a smooth visual transition?
 * - Does STROBE cause rendering artifacts?
 * - Do the legend colors stay in sync with the plot?
 * - Does SLOW_FADE eventually make traces invisible?
 */

#include <ViewPoint.h>
using namespace viewpoint;

// --- Phase definitions ---
enum Phase : uint8_t {
    RAINBOW,
    RANDOM_JUMP,
    STROBE,
    SYNC_MULTI,
    SLOW_FADE,
    PHASE_COUNT
};

// --- Timing ---
const unsigned long PHASE_DURATION_MS = 5000;
const unsigned long COLOR_UPDATE_MS = 50;  // 20 color changes per second

// --- State ---
Phase currentPhase = RAINBOW;
unsigned long phaseStartTime = 0;
unsigned long lastColorUpdate = 0;
float hue = 0.0;
int strobeFlip = 0;
float fadeLevel = 255.0;
float sinePhase1 = 0.0;
float sinePhase2 = 0.0;
float sinePhase3 = 0.0;

// --- Forward declarations ---
uint32_t hsvToRgb(float h, float s, float v);
void updateColors();

void setup() {
    randomSeed(analogRead(0));

    view.begin(Mode::Continuous);
    view.setDelay(3);
    view.setPlotTitle("Rapid Color Cycling");
    view.setAxisLabels("Time", "Value");

    view.trace("A").setColor(colors::Red);
    view.trace("B").setColor(colors::Green);
    view.trace("C").setColor(colors::Blue);

    phaseStartTime = millis();
    lastColorUpdate = millis();
}

void loop() {
    unsigned long now = millis();

    // Phase transition
    if (now - phaseStartTime >= PHASE_DURATION_MS) {
        currentPhase = static_cast<Phase>((currentPhase + 1) % PHASE_COUNT);
        phaseStartTime = now;
        hue = 0.0;
        fadeLevel = 255.0;

        const char* names[] = {"Rainbow", "Random Jump", "Strobe",
                               "Sync Multi", "Slow Fade"};
        view.sendInfo("Phase", names[currentPhase]);
    }

    // Update colors at interval
    if (now - lastColorUpdate >= COLOR_UPDATE_MS) {
        updateColors();
        lastColorUpdate = now;
    }

    // Send data — three sine waves at different frequencies
    sinePhase1 += 0.05;
    sinePhase2 += 0.08;
    sinePhase3 += 0.03;
    if (sinePhase1 > 2.0 * M_PI) sinePhase1 -= 2.0 * M_PI;
    if (sinePhase2 > 2.0 * M_PI) sinePhase2 -= 2.0 * M_PI;
    if (sinePhase3 > 2.0 * M_PI) sinePhase3 -= 2.0 * M_PI;

    view.addData("A", sin(sinePhase1) * 5.0);
    view.addData("B", sin(sinePhase2) * 3.0 + 2.0);
    view.addData("C", sin(sinePhase3) * 4.0 - 1.0);
    view.send();
}

void updateColors() {
    switch (currentPhase) {
        case RAINBOW: {
            hue += 3.0;
            if (hue >= 360.0) hue -= 360.0;
            // Each trace offset by 120 degrees
            view.trace("A").setColor(hsvToRgb(hue, 1.0, 1.0));
            view.trace("B").setColor(hsvToRgb(fmod(hue + 120.0, 360.0), 1.0, 1.0));
            view.trace("C").setColor(hsvToRgb(fmod(hue + 240.0, 360.0), 1.0, 1.0));
            break;
        }

        case RANDOM_JUMP: {
            uint32_t c1 = color(random(0, 256), random(0, 256), random(0, 256));
            uint32_t c2 = color(random(0, 256), random(0, 256), random(0, 256));
            uint32_t c3 = color(random(0, 256), random(0, 256), random(0, 256));
            view.trace("A").setColor(c1);
            view.trace("B").setColor(c2);
            view.trace("C").setColor(c3);
            break;
        }

        case STROBE: {
            strobeFlip = !strobeFlip;
            uint32_t on = 0xFFFFFF;   // White
            uint32_t off = 0x000000;  // Black
            view.trace("A").setColor(strobeFlip ? on : off);
            view.trace("B").setColor(strobeFlip ? off : on);
            view.trace("C").setColor(strobeFlip ? on : off);
            break;
        }

        case SYNC_MULTI: {
            // All three traces get the same color, cycling through a palette
            hue += 5.0;
            if (hue >= 360.0) hue -= 360.0;
            uint32_t c = hsvToRgb(hue, 0.8, 1.0);
            view.trace("A").setColor(c);
            view.trace("B").setColor(c);
            view.trace("C").setColor(c);
            break;
        }

        case SLOW_FADE: {
            fadeLevel -= 0.5;
            if (fadeLevel < 0.0) fadeLevel = 0.0;
            uint8_t v = (uint8_t)fadeLevel;
            view.trace("A").setColor(color(v, 0, 0));      // Red fading
            view.trace("B").setColor(color(0, v, 0));      // Green fading
            view.trace("C").setColor(color(0, 0, v));      // Blue fading
            break;
        }
    }
}

// Simple HSV to RGB conversion
// h: 0-360, s: 0-1, v: 0-1
uint32_t hsvToRgb(float h, float s, float v) {
    float c = v * s;
    float x = c * (1.0 - fabs(fmod(h / 60.0, 2.0) - 1.0));
    float m = v - c;

    float r = 0, g = 0, b = 0;
    if      (h < 60)  { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else              { r = c; g = 0; b = x; }

    uint8_t ri = (uint8_t)((r + m) * 255);
    uint8_t gi = (uint8_t)((g + m) * 255);
    uint8_t bi = (uint8_t)((b + m) * 255);

    return color(ri, gi, bi);
}
