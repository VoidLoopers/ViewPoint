/**
 * EntropyEngine.ino
 * @version 1.0.0
 * @date 03-10-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * EntropyEngine.ino - 2D ideal gas simulation demonstrating the second law of thermodynamics
 *
 * Boltzmann's tombstone bears the equation S = k log W — entropy equals the logarithm
 * of the number of microstates. This sketch makes that abstraction visceral. Fifty gas
 * molecules begin confined to a small region in the corner of a box, a low-entropy
 * macrostate with very few compatible microstates. The partition is removed and the
 * molecules expand freely, each one following Newton's laws with perfectly elastic
 * wall reflections. Within seconds the gas fills the box uniformly — the overwhelmingly
 * most probable macrostate — and never spontaneously returns to the corner.
 *
 * Each molecule is drawn as a short velocity trail whose length and direction reveal
 * its instantaneous speed vector. Four thermal bands color-code the Maxwell-Boltzmann
 * speed distribution: cold molecules crawl in blue, thermal molecules drift in green,
 * hot molecules streak in orange, and the rare high-energy tail blazes white. Watch
 * how the speed distribution reaches equilibrium long before the spatial distribution
 * does — a subtlety that separates statistical mechanics from naive diffusion.
 *
 * This demonstrates:
 * - Scatter (XY) Frames mode with per-particle trail segments
 * - addBreak() to draw many independent velocity vectors per frame
 * - Speed-band trace coloring as a thermal camera analog
 * - Periodic system reset to re-observe the irreversible expansion
 *
 * Verified hardware:
 * - Teensy 4.1
 *
 * Known issues:
 * - None recorded for this sketch
 *
 * Board-specific requirements:
 * - Uses only core ViewPoint APIs and standard Arduino runtime features.
 * - A faster 32-bit board is recommended for smooth frame updates; only Teensy 4.1 has been hardware-verified for this sketch so far.
 *
 * Hardware:
 * - No external hardware required.
 *
 * Sketch outline:
 * - Setup: Creates box boundaries as reference lines, initializes confined particles
 * - Loop: Integrates particle positions, reflects off walls, draws velocity trails
 * - Particles are binned by speed into thermal color bands each frame
 * - System resets to the confined state every RESET_SECONDS
 *
 * Things to try:
 * - Set NUM_PARTICLES to 100 for a denser gas (more convincing statistics)
 * - Change INITIAL_REGION to 0.5 to start with half the box filled
 * - Set THERMAL_SPEED to 60 for a hotter gas with faster equilibration
 * - Try WALL_DAMPING of 0.98 to see the gas slowly cool and settle
 * - Set TRAIL_SCALE to 0.0 to see point particles without velocity arrows
 * - Change RESET_SECONDS to 60 to watch the system longer before reset
 * - Set GRAVITY to 0.5 to see particles settle under gravity (barometric formula)
 */

#include <ViewPoint.h>
using namespace viewpoint;

// ─── Experimentation Levers ───
// Set to 0 for light/white backgrounds. The "Plasma" trace is near-white and
// will be invisible on a light background.
#define DARK_MODE           0
#define FRAME_DELAY_MS      33      // ~30fps
#define NUM_PARTICLES       50      // gas molecules — try 20 to 100
#define BOX_HALF            42.0    // half-width of the containment box
#define INITIAL_REGION      0.25    // fraction of box for initial confinement (0-1)
#define THERMAL_SPEED       35.0    // RMS speed — proxy for temperature (kT/m)
#define TRAIL_SCALE         0.04    // velocity trail length in seconds
#define WALL_DAMPING        1.0     // 1.0 = elastic, <1 = inelastic (cooling walls)
#define RESET_SECONDS       18.0    // seconds between entropy resets
#define GRAVITY             0.0     // downward acceleration — try 0.5 for sedimentation

// ─── State Variables ───
struct Molecule {
    float x, y, vx, vy;
};

Molecule gas[NUM_PARTICLES];
unsigned long frame_count = 0;

// Box-Muller transform for Gaussian random velocities (Maxwell-Boltzmann)
float gaussian_random() {
    float u1 = (random(1, 10000) + 0.5f) / 10000.0f;
    float u2 = (random(1, 10000) + 0.5f) / 10000.0f;
    return sqrt(-2.0f * log(u1)) * cos(2.0f * M_PI * u2);
}

float random_float(float lo, float hi) {
    return lo + (hi - lo) * random(0, 1001) / 1000.0f;
}

void confine_particles() {
    float region = BOX_HALF * INITIAL_REGION;
    for (int i = 0; i < NUM_PARTICLES; i++) {
        // All molecules start in the bottom-left corner
        gas[i].x = -BOX_HALF + random_float(0, region * 2.0f);
        gas[i].y = -BOX_HALF + random_float(0, region * 2.0f);
        // Maxwell-Boltzmann: each velocity component is Gaussian
        // with variance kT/m, so speed follows the 2D Maxwell distribution
        gas[i].vx = gaussian_random() * THERMAL_SPEED * 0.707f;
        gas[i].vy = gaussian_random() * THERMAL_SPEED * 0.707f;
    }
}

void setup() {
    randomSeed(analogRead(0));
    confine_particles();

    view.begin(PlotType::Scatter, Mode::Frames);
    view.setTitle("Entropy Engine");
    view.setDelay(FRAME_DELAY_MS);
    view.setPlotTitle("Entropy Engine");
    view.setAxisLabels("X", "Y");

    float margin = BOX_HALF + 5;
    view.setHorizontalRange(-margin, margin);
    view.setVerticalRange(-margin, margin);

    // Box walls
    view.addHorizontalReferenceLine(-BOX_HALF);
    view.addHorizontalReferenceLine(BOX_HALF);
    view.addVerticalReferenceLine(-BOX_HALF);
    view.addVerticalReferenceLine(BOX_HALF);

    // Thermal colormap: blue → green → orange → hottest
    view.trace("Cold").setColor(colors::DodgerBlue);
    view.trace("Warm").setColor(colors::MediumSeaGreen);
    view.trace("Hot").setColor(colors::OrangeRed);
#if DARK_MODE
    view.trace("Plasma").setColor(color(255, 255, 250, 240));
#else
    view.trace("Plasma").setColor(colors::Crimson);
#endif
}

// Draw the fading initial partition boundary
void draw_partition(float time_since_reset) {
    float fade = max(0.0f, 1.0f - time_since_reset / 3.0f);
    if (fade < 0.05f) return;

    uint8_t alpha = (uint8_t)(fade * 120);
    view.trace("Partition").setColor(color(alpha, 180, 80, 80));

    float px = -BOX_HALF + BOX_HALF * 2.0f * INITIAL_REGION;
    float py = -BOX_HALF + BOX_HALF * 2.0f * INITIAL_REGION;

    // Vertical partition wall
    view.addData("Partition", px, -BOX_HALF);
    view.addData("Partition", px, py);
    view.addBreak("Partition");
    // Horizontal partition wall
    view.addData("Partition", -BOX_HALF, py);
    view.addData("Partition", px, py);
    view.addBreak("Partition");
}

void loop() {
    float dt = FRAME_DELAY_MS / 1000.0f;
    float time = frame_count * dt;
    float time_in_cycle = viewpoint::wrapPositive(time, (float)RESET_SECONDS);

    // Reset to low-entropy confined state
    if (time_in_cycle < dt && frame_count > 0) {
        confine_particles();
    }

    for (int i = 0; i < NUM_PARTICLES; i++) {
        // Apply gravity
        gas[i].vy -= GRAVITY * dt;

        // Integrate position
        gas[i].x += gas[i].vx * dt;
        gas[i].y += gas[i].vy * dt;

        // Elastic wall reflections
        if (gas[i].x > BOX_HALF) {
            gas[i].x = 2.0f * BOX_HALF - gas[i].x;
            gas[i].vx *= -WALL_DAMPING;
        }
        if (gas[i].x < -BOX_HALF) {
            gas[i].x = -2.0f * BOX_HALF - gas[i].x;
            gas[i].vx *= -WALL_DAMPING;
        }
        if (gas[i].y > BOX_HALF) {
            gas[i].y = 2.0f * BOX_HALF - gas[i].y;
            gas[i].vy *= -WALL_DAMPING;
        }
        if (gas[i].y < -BOX_HALF) {
            gas[i].y = -2.0f * BOX_HALF - gas[i].y;
            gas[i].vy *= -WALL_DAMPING;
        }

        // Speed determines thermal color band
        float speed = sqrt(gas[i].vx * gas[i].vx + gas[i].vy * gas[i].vy);
        float normalized = speed / (THERMAL_SPEED * 1.8f);

        const char* band;
        if      (normalized < 0.35f) band = "Cold";
        else if (normalized < 0.65f) band = "Warm";
        else if (normalized < 1.0f)  band = "Hot";
        else                         band = "Plasma";

        // Velocity trail: tail is where the molecule was TRAIL_SCALE seconds ago
        float tail_x = gas[i].x - gas[i].vx * TRAIL_SCALE;
        float tail_y = gas[i].y - gas[i].vy * TRAIL_SCALE;

        view.addData(band, constrain(tail_x, -100.0f, 100.0f),
                          constrain(tail_y, -100.0f, 100.0f));
        view.addData(band, constrain(gas[i].x, -100.0f, 100.0f),
                          constrain(gas[i].y, -100.0f, 100.0f));
        view.addBreak(band);
    }

    draw_partition(time_in_cycle);

    view.send();
    frame_count++;
}
