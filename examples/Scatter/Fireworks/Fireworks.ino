/**
 * Fireworks.ino
 * @version V1R1
 * @date 03-10-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * Fireworks.ino - Particle-physics fireworks display over a city skyline
 *
 * Every firework is a miniature projectile-motion problem: a rocket launches
 * upward against gravity, bursts at apogee into radially-ejected particles,
 * and those particles trace parabolic arcs back to ground. All positions are
 * computed analytically from initial velocity and elapsed time — no particle
 * arrays needed — making this an O(1)-memory particle system.
 *
 * Short trail segments drawn behind each particle reveal velocity direction
 * and create the characteristic streaking effect. A city skyline silhouette
 * anchors the scene, and each burst gets a random HSV hue that dims as its
 * particles fade.
 *
 * This demonstrates:
 * - Scatter (XY) plotting in Frames mode with many disconnected segments
 * - addBreak() to draw hundreds of independent line segments per frame
 * - Analytical kinematics: position from v0, t, and gravity (no arrays)
 * - Polar-to-Cartesian conversion for radial burst patterns
 * - Per-frame dynamic trace coloring via HSV with brightness fade
 *
 * Verified hardware:
 * - Teensy 4.1
 *
 * Known issues:
 * - None recorded for this sketch
 *
 * Board-specific requirements:
 * - Uses only core ViewPoint APIs and standard Arduino runtime features.
 * - A faster 32-bit board is recommended for the densest frame bursts; only Teensy 4.1 has been hardware-verified for this sketch so far.
 *
 * Hardware:
 * - No external hardware required.
 *
 * Sketch outline:
 * - Setup: Configures Scatter/Frames, draws skyline, initializes firework slots
 * - Loop: Advances simulation clock, spawns fireworks on a staggered schedule
 * - Launch phase: Rocket drawn as a short ascending line segment
 * - Burst phase: Particles computed from burst center + radial velocity + gravity
 * - Each particle rendered as a trail segment (current position to recent position)
 *
 * Things to try:
 * - Increase GRAVITY to 40 for heavy, drooping bursts that barely clear the rooftops
 * - Set PARTICLES_PER_BURST to 48 for denser explosions (watch the data throughput)
 * - Change LAUNCH_INTERVAL to 0.3 for a rapid-fire grand finale
 * - Set TRAIL_SECONDS to 0.0 to see particle dots without streaks
 * - Try BURST_SPEED_MAX of 40 for massive wide bursts that fill the screen
 */

#include <ViewPoint.h>
using namespace viewpoint;

// ─── Experimentation Levers ───
#define FRAME_DELAY_MS      33      // ms per frame (~30fps) — try 16 for smoother animation
#define GRAVITY             20.0    // downward acceleration (units/s^2) — try 10 to 40
#define PARTICLES_PER_BURST 24      // particles per explosion — try 12 to 48
#define BURST_SPEED_MIN     14.0    // minimum radial ejection speed
#define BURST_SPEED_MAX     24.0    // maximum radial ejection speed
#define TRAIL_SECONDS       0.06    // streak length in seconds — 0 disables trails
#define LAUNCH_INTERVAL     1.4     // average seconds between launches — try 0.3 for finale
#define BURST_LIFETIME      3.0     // seconds particles remain visible after burst
#define GROUND_Y            -50.0   // y-coordinate of the ground plane

#define NUM_SLOTS           3       // concurrent firework slots (one trace each)

// ─── State Variables ───
struct Firework {
    float x;                // horizontal launch position
    float launch_speed;     // initial upward velocity (units/s)
    float burst_y;          // y where the rocket explodes
    float burst_speed;      // radial particle ejection speed
    float hue;              // HSV hue 0-360
    float launch_time;      // sim_time at launch
    float burst_time;       // sim_time at detonation
    bool active;
    bool has_burst;
};

Firework fw[NUM_SLOTS];
float sim_time = 0.0;
float next_launch = 0.3;
int next_slot = 0;
unsigned long frame_count = 0;

const char* burst_trace[] = {"Burst A", "Burst B", "Burst C"};

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

void launch_firework(int slot) {
    fw[slot].x = random_float(-35, 35);
    fw[slot].launch_speed = random_float(45, 70);

    // Burst at 60-90% of the rocket's theoretical max height
    float max_height = fw[slot].launch_speed * fw[slot].launch_speed / (2.0 * GRAVITY);
    fw[slot].burst_y = GROUND_Y + max_height * random_float(0.60, 0.90);

    fw[slot].burst_speed = random_float(BURST_SPEED_MIN, BURST_SPEED_MAX);
    fw[slot].hue = random_float(0, 360);
    fw[slot].launch_time = sim_time;
    fw[slot].burst_time = 0;
    fw[slot].active = true;
    fw[slot].has_burst = false;
}

// City skyline: a continuous polyline tracing building outlines along the ground
void draw_skyline() {
    struct Bldg { float l, r, h; };
    const Bldg buildings[] = {
        {-52, -44, 10}, {-40, -30, 24}, {-26, -18, 14},
        {-12,  -4, 34}, {  2,  12, 18}, { 18,  26,  8},
        { 32,  44, 20}, { 48,  54, 11},
    };
    const int n = sizeof(buildings) / sizeof(buildings[0]);

    view.addData("Skyline", buildings[0].l - 6, GROUND_Y);
    for (int i = 0; i < n; i++) {
        view.addData("Skyline", buildings[i].l, GROUND_Y);
        view.addData("Skyline", buildings[i].l, GROUND_Y + buildings[i].h);
        view.addData("Skyline", buildings[i].r, GROUND_Y + buildings[i].h);
        view.addData("Skyline", buildings[i].r, GROUND_Y);
    }
    view.addData("Skyline", buildings[n - 1].r + 6, GROUND_Y);
}

// Draw the ascending rocket as a short vertical trail segment
void draw_rocket(int slot) {
    float dt = sim_time - fw[slot].launch_time;
    float y = GROUND_Y + fw[slot].launch_speed * dt - 0.5 * GRAVITY * dt * dt;
    float vy = fw[slot].launch_speed - GRAVITY * dt;

    // Rocket has reached burst altitude or started falling
    if (y >= fw[slot].burst_y || vy <= 0) {
        fw[slot].has_burst = true;
        fw[slot].burst_time = sim_time;
        fw[slot].burst_y = y;
        return;
    }

    const char* tn = burst_trace[slot];
    float trail_y = y - min(vy * 0.05f, 3.0f);

    view.addData(tn, fw[slot].x, trail_y);
    view.addData(tn, fw[slot].x, y);
    view.addBreak(tn);
}

// Draw burst particles as short trail segments computed from kinematics
void draw_burst(int slot) {
    float dt = sim_time - fw[slot].burst_time;
    if (dt > BURST_LIFETIME) {
        fw[slot].active = false;
        return;
    }

    const char* tn = burst_trace[slot];
    float bx = fw[slot].x;
    float by = fw[slot].burst_y;
    float spd = fw[slot].burst_speed;

    for (int i = 0; i < PARTICLES_PER_BURST; i++) {
        float angle = (float)i / PARTICLES_PER_BURST * 2.0 * M_PI;

        // Alternating fast/slow particles creates a double-ring effect
        float sv = spd * (i % 2 == 0 ? 1.0f : 0.65f);
        float vx = sv * cos(angle);
        float vy = sv * sin(angle);

        // Current position: p = p0 + v*t - 0.5*g*t^2
        float px = bx + vx * dt;
        float py = by + vy * dt - 0.5 * GRAVITY * dt * dt;

        // Trail: where the particle was TRAIL_SECONDS ago
        float tdt = max(0.0f, dt - (float)TRAIL_SECONDS);
        float tx = bx + vx * tdt;
        float ty = by + vy * tdt - 0.5 * GRAVITY * tdt * tdt;

        // Clip particles that have fallen below ground
        if (py < GROUND_Y && ty < GROUND_Y) continue;

        view.addData(tn, constrain(tx, -200.0f, 200.0f),
                         constrain(ty, -200.0f, 200.0f));
        view.addData(tn, constrain(px, -200.0f, 200.0f),
                         constrain(py, -200.0f, 200.0f));
        view.addBreak(tn);
    }
}

void setup() {
    randomSeed(analogRead(0));

    view.begin(PlotType::Scatter, Mode::Frames);
    view.setTitle("Fireworks");
    view.setDelay(FRAME_DELAY_MS);
    view.setPlotTitle("Fireworks");
    view.setHorizontalRange(-65, 65);
    view.setVerticalRange(-60, 60);

    view.trace("Skyline").setColor(colors::DarkSlateBlue);
    view.trace("Burst A").setColor(colors::Gold);
    view.trace("Burst B").setColor(colors::Cyan);
    view.trace("Burst C").setColor(colors::HotPink);

    for (int i = 0; i < NUM_SLOTS; i++) fw[i].active = false;
}

void loop() {
    sim_time = frame_count * (FRAME_DELAY_MS / 1000.0f);

    // Launch new fireworks on a staggered random schedule
    if (sim_time >= next_launch) {
        int slot = -1;
        for (int i = 0; i < NUM_SLOTS; i++) {
            int idx = (next_slot + i) % NUM_SLOTS;
            if (!fw[idx].active) { slot = idx; break; }
        }
        if (slot < 0) slot = next_slot;

        launch_firework(slot);
        next_slot = (slot + 1) % NUM_SLOTS;
        next_launch = sim_time + LAUNCH_INTERVAL * random_float(0.7, 1.3);
    }

    // Update burst trace colors — brightness fades as particles age
    for (int i = 0; i < NUM_SLOTS; i++) {
        if (!fw[i].active) continue;
        float brightness = 1.0;
        if (fw[i].has_burst) {
            float age = sim_time - fw[i].burst_time;
            brightness = max(0.2f, 1.0f - 0.7f * age / (float)BURST_LIFETIME);
        }
        view.trace(burst_trace[i]).setColor(
            hsv_to_color(fw[i].hue, 0.85, brightness));
    }

    draw_skyline();

    for (int i = 0; i < NUM_SLOTS; i++) {
        if (!fw[i].active) continue;
        if (!fw[i].has_burst) draw_rocket(i);
        if (fw[i].has_burst)  draw_burst(i);
    }

    view.send();
    frame_count++;
}
