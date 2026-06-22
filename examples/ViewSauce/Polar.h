#ifndef VIEWSAUCE_POLAR_H
#define VIEWSAUCE_POLAR_H

#include "Config.h"

// ═════════════════════════════════════════════════════════════════════
//  Scene 5: Polar Sweep  (Polar / Continuous)
// ═════════════════════════════════════════════════════════════════════

#define POLAR_SWEEP_DELAY_MS  15   // ~67 samples/s

float sweep_angle = 0.0f;

void setup_polar_sweep() {
    view.setPlotType(PlotType::Polar);
    view.setMode(Mode::Continuous);
    view.setDelay(POLAR_SWEEP_DELAY_MS);
    view.setNumberOfPlots(1);
    view.setTitle("ViewSauce");
    view.setPlotTitle("Radar Sweep");
    view.setAngularOffset(90);     // north up
    view.setAngularStep(30);       // compass-style grid
    view.setRadialRange(0, 100);

    view.trace("Sweep").setColor(colors::Lime);
    sweep_angle = 0.0f;
}

void loop_polar_sweep() {
    float angle_rad = sweep_angle * M_PI / 180.0f;

    // 3-petal rose: r = 50 + 40*cos(3*theta) with noise
    float radius = 50.0f + 40.0f * cos(3.0f * angle_rad);
    radius += random_float(-3.0f, 3.0f);
    radius = constrain(radius, 0.0f, 100.0f);

    view.addData("Sweep", radius, sweep_angle);
    view.send();

    sweep_angle += 2.0f;
    if (sweep_angle >= 360.0f) sweep_angle -= 360.0f;
}

// ═════════════════════════════════════════════════════════════════════
//  Scene 6: Polar Wind Rose  (Polar / Frames)
// ═════════════════════════════════════════════════════════════════════

#define WIND_NUM_DIRS        24
#define WIND_DEG_PER_BIN     (360.0f / WIND_NUM_DIRS)
#define WIND_WINDOW          300
#define WIND_READS_PER_FRAME 5
#define WIND_DELAY_MS        100   // 10 fps

int   wind_counts[WIND_NUM_DIRS] = {0};
uint8_t wind_ring[WIND_WINDOW];
int   wind_head   = 0;
int   wind_count  = 0;
int   wind_max    = 1;
float wind_phase  = 0.0f;

static inline float wrap360(float deg) {
    while (deg < 0.0f) deg += 360.0f;
    while (deg >= 360.0f) deg -= 360.0f;
    return deg;
}

static inline void wind_add_reading(int bin) {
    if (wind_count < WIND_WINDOW) {
        wind_ring[wind_head] = (uint8_t)bin;
        wind_head = (wind_head + 1) % WIND_WINDOW;
        wind_count++;
        wind_counts[bin]++;
    } else {
        int old_bin = wind_ring[wind_head];
        if (wind_counts[old_bin] > 0) wind_counts[old_bin]--;
        wind_ring[wind_head] = (uint8_t)bin;
        wind_head = (wind_head + 1) % WIND_WINDOW;
        wind_counts[bin]++;
    }
}

static void wind_recompute_max() {
    wind_max = 1;
    for (int i = 0; i < WIND_NUM_DIRS; i++)
        if (wind_counts[i] > wind_max) wind_max = wind_counts[i];
}

void setup_polar_wind_rose() {
    view.setPlotType(PlotType::Polar);
    view.setMode(Mode::Frames);
    view.setDelay(WIND_DELAY_MS);
    view.setPacketSize(WIND_NUM_DIRS);
    view.setTitle("ViewSauce");
    view.setPlotTitle("Wind Rose");
    view.setAngularOffset(90);
    view.setAngularStep((int)WIND_DEG_PER_BIN);  // setAngularStep takes whole degrees
    view.setRadialRange(0, 100);

    view.trace("Wind").setColor(colors::Cyan);

    // Reset histogram
    for (int i = 0; i < WIND_NUM_DIRS; i++) wind_counts[i] = 0;
    wind_head = 0;
    wind_count = 0;
    wind_max = 1;
    wind_phase = 0.0f;
}

void loop_polar_wind_rose() {
    wind_phase += 0.01f;

    float dominant = 225.0f + 40.0f * sin(wind_phase);
    float spread   = 45.0f + 15.0f * (0.5f + 0.5f * sin(wind_phase * 0.6f));

    for (int r = 0; r < WIND_READS_PER_FRAME; r++) {
        float direction = dominant;
        for (int i = 0; i < 6; i++)
            direction += random((int)-spread, (int)spread + 1) / 3.0f;
        direction = wrap360(direction);
        int bin = (int)(direction / WIND_DEG_PER_BIN) % WIND_NUM_DIRS;
        wind_add_reading(bin);
    }

    wind_recompute_max();

    for (int i = 0; i < WIND_NUM_DIRS; i++) {
        float angle = i * WIND_DEG_PER_BIN + WIND_DEG_PER_BIN * 0.5f;
        float radius = (wind_counts[i] * 100.0f) / (float)wind_max;
        view.addData("Wind", 0.0f, angle);
        view.addData("Wind", radius, angle);
    }

    view.send();
}

#endif // VIEWSAUCE_POLAR_H
