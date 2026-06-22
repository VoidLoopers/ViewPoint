#ifndef VIEWSAUCE_SCATTER_H
#define VIEWSAUCE_SCATTER_H

#include "Config.h"

// ═════════════════════════════════════════════════════════════════════
//  Scene 7: Lissajous Figures  (Scatter / Frames)
// ═════════════════════════════════════════════════════════════════════

#define LISS_DELAY_MS         33
#define LISS_POINTS           400
#define LISS_AMPLITUDE        40.0f
#define LISS_MORPH_SECS       5.0f    // seconds per frequency ratio

// Frequency ratio presets { fx_num, fy_num, fx_den, fy_den }
const float liss_ratios[][2] = {
    {1.0f, 2.0f},   // 1:2
    {2.0f, 3.0f},   // 2:3
    {3.0f, 4.0f},   // 3:4
    {3.0f, 2.0f},   // 3:2
};
const int LISS_NUM_RATIOS = sizeof(liss_ratios) / sizeof(liss_ratios[0]);

float liss_delta = 0.0f;   // phase offset for figure rotation

void setup_lissajous() {
    view.setPlotType(PlotType::Scatter);
    view.setMode(Mode::Frames);
    view.setDelay(LISS_DELAY_MS);
    view.setNumberOfPlots(1);
    view.setTitle("ViewSauce");
    view.setPlotTitle("Lissajous Figures");
    view.setHorizontalRange(-50, 50, 1);
    view.setVerticalRange(-50, 50, 1);
    view.setDisplayMode(DisplayMode::Persistence);
    view.addHorizontalReferenceLine(0.0f);
    view.addVerticalReferenceLine(0.0f);

    view.trace("Lissajous").setColor(colors::Cyan);
    liss_delta = 0.0f;
}

void loop_lissajous() {
    float time_s = scene_frame * LISS_DELAY_MS / 1000.0f;

    // Determine which ratio we're on and interpolate
    float cycle_pos = viewpoint::wrapPositive(time_s, LISS_MORPH_SECS * LISS_NUM_RATIOS);
    int idx = (int)(cycle_pos / LISS_MORPH_SECS) % LISS_NUM_RATIOS;
    float fx = liss_ratios[idx][0];
    float fy = liss_ratios[idx][1];

    // Slowly rotate the phase offset for visual variety
    liss_delta += 0.003f;

    // HSV cycling trace color
    float hue = viewpoint::wrapPositive(scene_frame * 0.5f, 360.0f);
    view.trace("Lissajous").setColor(hsv_to_color(hue, 0.85f, 1.0f));

    for (int i = 0; i < LISS_POINTS; i++) {
        float t = (float)i / LISS_POINTS * 2.0f * M_PI;
        float x = LISS_AMPLITUDE * sin(fx * t + liss_delta);
        float y = LISS_AMPLITUDE * sin(fy * t);
        view.addData("Lissajous", x, y);
    }

    view.send();
}

// ═════════════════════════════════════════════════════════════════════
//  Scene 8: Holographic Surfaces  (Scatter / Frames)
// ═════════════════════════════════════════════════════════════════════

#define HOLO_DELAY_MS       33
#define HOLO_U_LINES        8
#define HOLO_V_LINES        10
#define HOLO_LINE_DETAIL    20
#define HOLO_ROT_X          0.31f   // rad/s
#define HOLO_ROT_Y          0.53f
#define HOLO_ROT_Z          0.19f
#define HOLO_SHAPE_HOLD_SEC 8.0f
#define HOLO_MORPH_SEC      1.2f
#define HOLO_DISPLAY_SCALE  18.0f
#define HOLO_TORUS_R        2.0f
#define HOLO_TORUS_r        0.75f
#define HOLO_KLEIN_A        2.5f
#define HOLO_NUM_BANDS      5
#define HOLO_NUM_SHAPES     2       // torus + Klein bottle
#define HOLO_Z_EXTENT       3.5f

struct Vec3 { float x, y, z; };

float holo_ax = 0.0f, holo_ay = 0.0f, holo_az = 0.0f;

const char* holo_band_name[] = {"Front", "Near", "Mid", "Far", "Back"};

Vec3 holo_rotate(Vec3 p) {
    float cx = cos(holo_ax), sx = sin(holo_ax);
    float cy = cos(holo_ay), sy = sin(holo_ay);
    float cz = cos(holo_az), sz = sin(holo_az);
    float y1 = cx * p.y - sx * p.z;
    float z1 = sx * p.y + cx * p.z;
    float x2 = cy * p.x + sy * z1;
    float z2 = -sy * p.x + cy * z1;
    return {cz * x2 - sz * y1, sz * x2 + cz * y1, z2};
}

Vec3 eval_torus(float u, float v) {
    return {
        (HOLO_TORUS_R + HOLO_TORUS_r * cos(v)) * cos(u),
        (HOLO_TORUS_R + HOLO_TORUS_r * cos(v)) * sin(u),
        HOLO_TORUS_r * sin(v)
    };
}

Vec3 eval_klein(float u, float v) {
    float cu = cos(u), su = sin(u);
    float cv = cos(v), sv = sin(v);
    float cv2 = cos(v * 0.5f), sv2 = sin(v * 0.5f);
    float r = HOLO_KLEIN_A + cu * sv2 - su * sin(v);
    return { r * cv * 0.5f, r * sv * 0.5f, su * sv2 + cu * sin(v) * 0.5f };
}

Vec3 holo_eval_surface(int shape, float u, float v) {
    if (shape == 0) return eval_torus(u, v);
    return eval_klein(u, v);
}

void holo_draw_segment(Vec3 a, Vec3 b, float scale) {
    float sx_a = a.x * scale, sy_a = a.y * scale;
    float sx_b = b.x * scale, sy_b = b.y * scale;

    float z_norm = ((a.z + b.z) * 0.5f) / HOLO_Z_EXTENT + 0.5f;
    z_norm = constrain(z_norm, 0.0f, 0.999f);
    int band = constrain((int)(z_norm * HOLO_NUM_BANDS), 0, HOLO_NUM_BANDS - 1);

    const char* tn = holo_band_name[band];
    view.addData(tn, sx_a, sy_a);
    view.addData(tn, sx_b, sy_b);
    view.addBreak(tn);
}

void setup_holographic() {
    view.setPlotType(PlotType::Scatter);
    view.setMode(Mode::Frames);
    view.setDelay(HOLO_DELAY_MS);
    view.setTitle("ViewSauce");
    view.setPlotTitle("Holographic Surfaces");
    view.setAxisLabels("", "");
    view.setDisplayMode(DisplayMode::None);

    float r = HOLO_DISPLAY_SCALE * 2.2f;
    view.setHorizontalRange(-r, r, 1);
    view.setVerticalRange(-r, r, 1);

    view.trace("Front").setColor(color(255, 20, 50, 120));
    view.trace("Near").setColor(color(200, 40, 70, 155));
    view.trace("Mid").setColor(color(140, 80, 110, 185));
    view.trace("Far").setColor(color(80, 130, 150, 210));
    view.trace("Back").setColor(color(40, 170, 185, 220));

    holo_ax = holo_ay = holo_az = 0.0f;
}

void loop_holographic() {
    float dt_sec = HOLO_DELAY_MS / 1000.0f;
    float time = scene_frame * dt_sec;

    holo_ax += HOLO_ROT_X * dt_sec;
    holo_ay += HOLO_ROT_Y * dt_sec;
    holo_az += HOLO_ROT_Z * dt_sec;
    if (holo_ax > 2.0f * M_PI) holo_ax -= 2.0f * M_PI;
    if (holo_ay > 2.0f * M_PI) holo_ay -= 2.0f * M_PI;
    if (holo_az > 2.0f * M_PI) holo_az -= 2.0f * M_PI;

    // Shape cycling with cosine-bell morph
    float shape_period = HOLO_SHAPE_HOLD_SEC + HOLO_MORPH_SEC;
    float cycle_pos = viewpoint::wrapPositive(time, shape_period * HOLO_NUM_SHAPES);
    int shape_idx = (int)(cycle_pos / shape_period) % HOLO_NUM_SHAPES;
    float phase = viewpoint::wrapPositive(cycle_pos, shape_period);

    float morph = 1.0f;
    int draw_shape = shape_idx;
    if (phase > HOLO_SHAPE_HOLD_SEC) {
        float t = (phase - HOLO_SHAPE_HOLD_SEC) / HOLO_MORPH_SEC;
        morph = 0.5f + 0.5f * cos(t * 2.0f * M_PI);
        if (t >= 0.5f) draw_shape = (shape_idx + 1) % HOLO_NUM_SHAPES;
    }

    float scale = HOLO_DISPLAY_SCALE * morph;
    if (scale < 0.1f) { view.send(); return; }

    // Draw iso-parameter lines in u direction
    for (int j = 0; j < HOLO_U_LINES; j++) {
        float v = j * 2.0f * M_PI / HOLO_U_LINES;
        Vec3 prev = {0, 0, 0};
        for (int i = 0; i <= HOLO_LINE_DETAIL; i++) {
            float u = i * 2.0f * M_PI / HOLO_LINE_DETAIL;
            Vec3 p = holo_eval_surface(draw_shape, u, v);
            p = holo_rotate(p);
            if (i > 0) holo_draw_segment(prev, p, scale);
            prev = p;
        }
    }

    // Draw iso-parameter lines in v direction
    for (int j = 0; j < HOLO_V_LINES; j++) {
        float u = j * 2.0f * M_PI / HOLO_V_LINES;
        Vec3 prev = {0, 0, 0};
        for (int i = 0; i <= HOLO_LINE_DETAIL; i++) {
            float v = i * 2.0f * M_PI / HOLO_LINE_DETAIL;
            Vec3 p = holo_eval_surface(draw_shape, u, v);
            p = holo_rotate(p);
            if (i > 0) holo_draw_segment(prev, p, scale);
            prev = p;
        }
    }

    view.send();
}

// ═════════════════════════════════════════════════════════════════════
//  Scene 9: Fireworks Finale  (Scatter / Frames)
// ═════════════════════════════════════════════════════════════════════

#define FW_DELAY_MS           33
#define FW_GRAVITY            20.0f
#define FW_PARTICLES          24
#define FW_BURST_SPEED_MIN    14.0f
#define FW_BURST_SPEED_MAX    24.0f
#define FW_TRAIL_SECONDS      0.06f
#define FW_LAUNCH_INTERVAL    0.8f    // faster for finale
#define FW_BURST_LIFETIME     3.0f
#define FW_GROUND_Y           -50.0f
#define FW_NUM_SLOTS          3

struct Firework {
    float x, launch_speed, burst_y, burst_speed;
    float hue, launch_time, burst_time;
    bool active, has_burst;
};

Firework fw[FW_NUM_SLOTS];
float fw_sim_time    = 0.0f;
float fw_next_launch = 0.3f;
int   fw_next_slot   = 0;

const char* fw_trace[] = {"Burst A", "Burst B", "Burst C"};

void fw_launch(int slot) {
    fw[slot].x            = random_float(-35.0f, 35.0f);
    fw[slot].launch_speed = random_float(45.0f, 70.0f);
    float max_h = fw[slot].launch_speed * fw[slot].launch_speed / (2.0f * FW_GRAVITY);
    fw[slot].burst_y      = FW_GROUND_Y + max_h * random_float(0.60f, 0.90f);
    fw[slot].burst_speed  = random_float(FW_BURST_SPEED_MIN, FW_BURST_SPEED_MAX);
    fw[slot].hue          = random_float(0.0f, 360.0f);
    fw[slot].launch_time  = fw_sim_time;
    fw[slot].burst_time   = 0.0f;
    fw[slot].active       = true;
    fw[slot].has_burst    = false;
}

void fw_draw_skyline() {
    struct Bldg { float l, r, h; };
    const Bldg buildings[] = {
        {-52, -44, 10}, {-40, -30, 24}, {-26, -18, 14},
        {-12,  -4, 34}, {  2,  12, 18}, { 18,  26,  8},
        { 32,  44, 20}, { 48,  54, 11},
    };
    const int n = sizeof(buildings) / sizeof(buildings[0]);

    view.addData("Skyline", buildings[0].l - 6, FW_GROUND_Y);
    for (int i = 0; i < n; i++) {
        view.addData("Skyline", buildings[i].l, FW_GROUND_Y);
        view.addData("Skyline", buildings[i].l, FW_GROUND_Y + buildings[i].h);
        view.addData("Skyline", buildings[i].r, FW_GROUND_Y + buildings[i].h);
        view.addData("Skyline", buildings[i].r, FW_GROUND_Y);
    }
    view.addData("Skyline", buildings[n - 1].r + 6, FW_GROUND_Y);
}

void fw_draw_rocket(int slot) {
    float dt = fw_sim_time - fw[slot].launch_time;
    float y = FW_GROUND_Y + fw[slot].launch_speed * dt - 0.5f * FW_GRAVITY * dt * dt;
    float vy = fw[slot].launch_speed - FW_GRAVITY * dt;

    if (y >= fw[slot].burst_y || vy <= 0.0f) {
        fw[slot].has_burst = true;
        fw[slot].burst_time = fw_sim_time;
        fw[slot].burst_y = y;
        return;
    }

    const char* tn = fw_trace[slot];
    float trail_y = y - min(vy * 0.05f, 3.0f);
    view.addData(tn, fw[slot].x, trail_y);
    view.addData(tn, fw[slot].x, y);
    view.addBreak(tn);
}

void fw_draw_burst(int slot) {
    float dt = fw_sim_time - fw[slot].burst_time;
    if (dt > FW_BURST_LIFETIME) { fw[slot].active = false; return; }

    const char* tn = fw_trace[slot];
    float bx = fw[slot].x, by = fw[slot].burst_y, spd = fw[slot].burst_speed;

    for (int i = 0; i < FW_PARTICLES; i++) {
        float angle = (float)i / FW_PARTICLES * 2.0f * M_PI;
        float sv = spd * (i % 2 == 0 ? 1.0f : 0.65f);
        float vx = sv * cos(angle);
        float vy = sv * sin(angle);

        float px = bx + vx * dt;
        float py = by + vy * dt - 0.5f * FW_GRAVITY * dt * dt;

        float tdt = max(0.0f, dt - FW_TRAIL_SECONDS);
        float tx = bx + vx * tdt;
        float ty = by + vy * tdt - 0.5f * FW_GRAVITY * tdt * tdt;

        if (py < FW_GROUND_Y && ty < FW_GROUND_Y) continue;

        view.addData(tn, constrain(tx, -200.0f, 200.0f),
                         constrain(ty, -200.0f, 200.0f));
        view.addData(tn, constrain(px, -200.0f, 200.0f),
                         constrain(py, -200.0f, 200.0f));
        view.addBreak(tn);
    }
}

void setup_fireworks() {
    view.setPlotType(PlotType::Scatter);
    view.setMode(Mode::Frames);
    view.setDelay(FW_DELAY_MS);
    view.setTitle("ViewSauce");
    view.setPlotTitle("Fireworks!");
    view.setHorizontalRange(-65, 65);
    view.setVerticalRange(-60, 60);
    view.setDisplayMode(DisplayMode::None);

    view.trace("Skyline").setColor(colors::DarkSlateBlue);
    view.trace("Burst A").setColor(colors::Gold);
    view.trace("Burst B").setColor(colors::Cyan);
    view.trace("Burst C").setColor(colors::HotPink);

    // Reset firework state
    fw_sim_time = 0.0f;
    fw_next_launch = 0.3f;
    fw_next_slot = 0;
    for (int i = 0; i < FW_NUM_SLOTS; i++) fw[i].active = false;
}

void loop_fireworks() {
    fw_sim_time = scene_frame * (FW_DELAY_MS / 1000.0f);

    // Launch on schedule
    if (fw_sim_time >= fw_next_launch) {
        int slot = -1;
        for (int i = 0; i < FW_NUM_SLOTS; i++) {
            int idx = (fw_next_slot + i) % FW_NUM_SLOTS;
            if (!fw[idx].active) { slot = idx; break; }
        }
        if (slot < 0) slot = fw_next_slot;
        fw_launch(slot);
        fw_next_slot = (slot + 1) % FW_NUM_SLOTS;
        fw_next_launch = fw_sim_time + FW_LAUNCH_INTERVAL * random_float(0.7f, 1.3f);
    }

    // Update burst colors — brightness fades
    for (int i = 0; i < FW_NUM_SLOTS; i++) {
        if (!fw[i].active) continue;
        float brightness = 1.0f;
        if (fw[i].has_burst) {
            float age = fw_sim_time - fw[i].burst_time;
            brightness = max(0.2f, 1.0f - 0.7f * age / FW_BURST_LIFETIME);
        }
        view.trace(fw_trace[i]).setColor(hsv_to_color(fw[i].hue, 0.85f, brightness));
    }

    fw_draw_skyline();

    for (int i = 0; i < FW_NUM_SLOTS; i++) {
        if (!fw[i].active) continue;
        if (!fw[i].has_burst) fw_draw_rocket(i);
        if (fw[i].has_burst)  fw_draw_burst(i);
    }

    view.send();
}

#endif // VIEWSAUCE_SCATTER_H
