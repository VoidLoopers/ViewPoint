/**
 * WindRoseChart.ino
 * @version V1R1 R0.2
 * @date 01-27-26
 * 
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * WindRoseChart.ino — Wind Direction Histogram (Wind Rose)
 * @version ViewPoint V1R1
 *
 * Displays a wind rose showing how often wind comes from different directions.
 *
 * How it works:
 * - Wind directions are grouped into angular bins around the compass.
 * - Each bin counts how many recent wind readings came from that direction.
 * - The length of each radial line represents how frequently wind came from that direction.
 *
 * Behavior:
 * - Uses a sliding window of recent samples so the plot reflects current conditions.
 * - The prevailing wind direction and variability drift slowly over time,
 *   simulating changing weather patterns.
 *
 * Visualization:
 * - Polar plot with North at the top and angles increasing clockwise.
 * - Radius is scaled from 0–100 based on relative frequency.
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
 */

#include <ViewPoint.h>
using namespace viewpoint;

// Compass bin configuration
#define NUM_DIRECTIONS 24
#define DEGREES_PER_BIN (360.0f / NUM_DIRECTIONS)

// Sliding window size (number of recent readings kept)
#define WINDOW 600
#define READINGS_PER_FRAME 10

// Baseline wind behavior
#define BASE_DOMINANT_DIRECTION 225.0f   // degrees (SW)
#define BASE_DIRECTION_SPREAD   45.0f    // degrees

// Slow variation to simulate changing weather
#define DOMINANT_WANDER_AMPLITUDE 40.0f
#define SPREAD_WANDER_AMPLITUDE   15.0f
#define DRIFT_RATE                0.01f

// Histogram counts
int wind_counts[NUM_DIRECTIONS] = {0};

// Ring buffer storing recent bin indices
uint8_t ring[WINDOW];
int ringHead = 0;
int ringCount = 0;

int max_count = 1;

static inline float wrap360(float deg) {
    while (deg < 0) deg += 360.0f;
    while (deg >= 360.0f) deg -= 360.0f;
    return deg;
}

static inline void addReadingToWindow(int bin) {
    if (ringCount < WINDOW) {
        ring[ringHead] = (uint8_t)bin;
        ringHead = (ringHead + 1) % WINDOW;
        ringCount++;
        wind_counts[bin]++;
    } else {
        int oldBin = ring[ringHead];
        if (wind_counts[oldBin] > 0) wind_counts[oldBin]--;
        ring[ringHead] = (uint8_t)bin;
        ringHead = (ringHead + 1) % WINDOW;
        wind_counts[bin]++;
    }
}

static inline void recomputeMaxCount() {
    max_count = 1;
    for (int i = 0; i < NUM_DIRECTIONS; i++) {
        if (wind_counts[i] > max_count) max_count = wind_counts[i];
    }
}

void setup() {
    view.begin(PlotType::Polar, Mode::Frames, NUM_DIRECTIONS);
    view.setTitle("Wind Rose Chart");

    // North at top, clockwise angles
    view.setAngularOffset(90);
    view.setAngularStep(DEGREES_PER_BIN);

    view.setRadialRange(0, 100);
    view.setPlotTitle("Wind Rose");
}

void loop() {
    static float phase = 0.0f;
    phase += DRIFT_RATE;

    float dominant =
        BASE_DOMINANT_DIRECTION +
        DOMINANT_WANDER_AMPLITUDE * sinf(phase);

    float spread =
        BASE_DIRECTION_SPREAD +
        SPREAD_WANDER_AMPLITUDE * (0.5f + 0.5f * sinf(phase * 0.6f));

    for (int r = 0; r < READINGS_PER_FRAME; r++) {
        float direction = dominant;
        for (int i = 0; i < 6; i++) {
            direction += random((int)-spread, (int)spread + 1) / 3.0f;
        }
        direction = wrap360(direction);

        int bin = (int)(direction / DEGREES_PER_BIN) % NUM_DIRECTIONS;
        addReadingToWindow(bin);
    }

    recomputeMaxCount();

    for (int i = 0; i < NUM_DIRECTIONS; i++) {
        float angle = i * DEGREES_PER_BIN + DEGREES_PER_BIN * 0.5f;
        float radius = (wind_counts[i] * 100.0f) / (float)max_count;
        view.addData("Wind", 0.0, radius); // start from the origin
        view.addData("Wind", radius, angle);
    }

    view.send();
}