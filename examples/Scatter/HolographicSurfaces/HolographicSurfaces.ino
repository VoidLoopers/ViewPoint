/**
 * HolographicSurfaces.ino
 * @version 1.0.0
 * @date 03-10-26
 *
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * HolographicSurfaces.ino - 3D mathematical surfaces as depth-graded wireframes
 *
 * The torus, Klein bottle, Mobius strip, and breathing sphere are among the most
 * beautiful objects in differential geometry — pure mathematical forms that exist
 * as parametric equations mapping two variables into three-dimensional space. This
 * sketch evaluates those equations in real time, applies Euler rotation matrices on
 * all three axes, and projects the results onto a 2D scatter plane with depth-band
 * trace coloring that creates a holographic illusion of volume.
 *
 * Five depth bands map each wireframe segment's z-coordinate to graduated ARGB
 * opacity and color temperature. Near surfaces appear bright and warm; far surfaces
 * recede into dim blue shadow — the same atmospheric perspective that painters have
 * used since the Renaissance, computed here at 30 frames per second. The gallery
 * cycles through shapes with a smooth scale-to-zero morph transition.
 *
 * This demonstrates:
 * - Scatter (XY) Frames mode for complete wireframe redraw each frame
 * - ARGB depth-band traces for pseudo-3D volume rendering
 * - 3D Euler rotation (Rx * Ry * Rz) applied to parametric surface vertices
 * - addBreak() for hundreds of disconnected wireframe segments
 * - Parametric surface evaluation: torus, Klein bottle, Mobius strip, sphere
 *
 * Verified hardware:
 * - Teensy 4.1
 *
 * Known issues:
 * - None recorded for this sketch
 *
 * Board-specific requirements:
 * - Uses only core ViewPoint APIs and standard Arduino runtime features.
 * - A faster 32-bit board is recommended for dense wireframe scenes; only Teensy 4.1 has been hardware-verified for this sketch so far.
 *
 * Hardware:
 * - No external hardware required.
 *
 * Sketch outline:
 * - Setup: Creates 5 depth-band traces with ARGB colors, configures Scatter/Frames
 * - Loop: Evaluates current surface along iso-parameter curves, rotates, projects
 * - Each segment is binned by depth into the appropriate trace band
 * - Surfaces cycle with a cosine-bell morph that scales to zero at transition
 *
 * Things to try:
 * - Set U_LINES to 14 and V_LINES to 16 for denser, more detailed wireframes
 * - Change ROTATION_X/Y/Z independently to favor tumbling on one axis
 * - Set SHAPE_HOLD_SEC to 4.0 for faster cycling through the gallery
 * - Try TORUS_R of 2.5 and TORUS_r of 1.8 for a fat horn torus
 * - Set MORPH_SEC to 0.0 for instant shape transitions
 * - Increase LINE_DETAIL to 32 for smoother curves (more data per frame)
 */

#include <ViewPoint.h>
using namespace viewpoint;

// ─── Experimentation Levers ───
// Set to 0 for light/white backgrounds. Depth-band traces use near-white colors
// designed for dark themes — they will be invisible on a light background.
#define DARK_MODE           0
#define FRAME_DELAY_MS      33      // ~30fps
#define U_LINES             8       // iso-parameter curves in u direction
#define V_LINES             10      // iso-parameter curves in v direction
#define LINE_DETAIL         20      // vertices per curve segment
#define ROTATION_X          0.31    // rad/s around X axis
#define ROTATION_Y          0.53    // rad/s around Y axis
#define ROTATION_Z          0.19    // rad/s around Z axis
#define SHAPE_HOLD_SEC      8.0     // seconds to display each surface
#define MORPH_SEC           1.2     // scale-to-zero transition duration
#define DISPLAY_SCALE       18.0    // world-to-screen magnification

// Surface geometry
#define TORUS_R             2.0     // major radius (center to tube center)
#define TORUS_r             0.75    // minor radius (tube thickness)
#define MOBIUS_W            0.7     // Mobius strip half-width
#define KLEIN_A             2.5     // Klein bottle tube distance

#define NUM_BANDS           5
#define NUM_SHAPES          4
#define Z_EXTENT            3.5     // expected z-range for depth normalization

// ─── State Variables ───
float ax = 0, ay = 0, az = 0;
unsigned long frame_count = 0;

const char* band_name[] = {"Front", "Near", "Mid", "Far", "Back"};

struct Vec3 { float x, y, z; };

// Euler rotation: Rz(az) * Ry(ay) * Rx(ax)
Vec3 rotate3d(Vec3 p) {
    float cx = cos(ax), sx = sin(ax);
    float cy = cos(ay), sy = sin(ay);
    float cz = cos(az), sz = sin(az);

    // Rx
    float y1 = cx * p.y - sx * p.z;
    float z1 = sx * p.y + cx * p.z;
    // Ry
    float x2 = cy * p.x + sy * z1;
    float z2 = -sy * p.x + cy * z1;
    // Rz
    return {cz * x2 - sz * y1, sz * x2 + cz * y1, z2};
}

// Torus: u and v both sweep 0..2pi
Vec3 eval_torus(float u, float v) {
    return {
        (TORUS_R + TORUS_r * cos(v)) * cos(u),
        (TORUS_R + TORUS_r * cos(v)) * sin(u),
        TORUS_r * sin(v)
    };
}

// Klein bottle figure-8 immersion
// A non-orientable surface that passes through itself in 3D
Vec3 eval_klein(float u, float v) {
    float cu = cos(u), su = sin(u);
    float cv = cos(v), sv = sin(v);
    float cv2 = cos(v * 0.5f), sv2 = sin(v * 0.5f);
    float r = KLEIN_A + cu * sv2 - su * sin(v);
    return {
        r * cv * 0.5f,
        r * sv * 0.5f,
        su * sv2 + cu * sin(v) * 0.5f
    };
}

// Mobius strip: u sweeps 0..2pi, v maps to strip width [-W, W]
Vec3 eval_mobius(float u, float v) {
    float s = (v / (2.0f * M_PI) - 0.5f) * 2.0f * MOBIUS_W;
    float half_twist = u * 0.5f;
    return {
        (1.2f + s * cos(half_twist)) * cos(u),
        (1.2f + s * cos(half_twist)) * sin(u),
        s * sin(half_twist)
    };
}

// Breathing sphere: sinusoidal surface displacement creates organic pulsation
Vec3 eval_sphere(float u, float v, float t) {
    float phi = v * 0.5f;   // v in 0..2pi maps to latitude 0..pi
    float r = 1.7f + 0.25f * sin(4.0f * u + t * 2.0f)
                   + 0.18f * sin(3.0f * phi + t * 1.7f)
                   + 0.12f * sin(2.0f * u - 5.0f * phi + t * 0.9f);
    return {
        r * sin(phi) * cos(u),
        r * sin(phi) * sin(u),
        r * cos(phi)
    };
}

Vec3 eval_surface(int shape, float u, float v, float t) {
    switch (shape) {
        case 0:  return eval_torus(u, v);
        case 1:  return eval_klein(u, v);
        case 2:  return eval_mobius(u, v);
        default: return eval_sphere(u, v, t);
    }
}

void draw_segment(Vec3 a, Vec3 b, float scale) {
    float sx_a = a.x * scale, sy_a = a.y * scale;
    float sx_b = b.x * scale, sy_b = b.y * scale;

    // Depth band from average z-coordinate of the segment
    float z_norm = ((a.z + b.z) * 0.5f) / Z_EXTENT + 0.5f;
    z_norm = constrain(z_norm, 0.0f, 0.999f);
    int band = (int)(z_norm * NUM_BANDS);
    band = constrain(band, 0, NUM_BANDS - 1);

    const char* tn = band_name[band];
    view.addData(tn, sx_a, sy_a);
    view.addData(tn, sx_b, sy_b);
    view.addBreak(tn);
}

void setup() {
    view.begin(PlotType::Scatter, Mode::Frames);
    view.setTitle("Holographic Surfaces");
    view.setDelay(FRAME_DELAY_MS);
    view.setPlotTitle("Holographic Surfaces");
    view.setAxisLabels("", "");

    float r = DISPLAY_SCALE * 2.2f;
    view.setHorizontalRange(-r, r, 1);
    view.setVerticalRange(-r, r, 1);

    // Depth bands: front = prominent, back = recedes
#if DARK_MODE
    view.trace("Front").setColor(color(255, 255, 245, 230));
    view.trace("Near").setColor(color(210, 220, 235, 250));
    view.trace("Mid").setColor(color(150, 165, 190, 235));
    view.trace("Far").setColor(color(90, 110, 140, 215));
    view.trace("Back").setColor(color(45, 70, 95, 180));
#else
    view.trace("Front").setColor(color(255, 20, 50, 120));
    view.trace("Near").setColor(color(200, 40, 70, 155));
    view.trace("Mid").setColor(color(140, 80, 110, 185));
    view.trace("Far").setColor(color(80, 130, 150, 210));
    view.trace("Back").setColor(color(40, 170, 185, 220));
#endif
}

void loop() {
    float dt_sec = FRAME_DELAY_MS / 1000.0f;
    float time = frame_count * dt_sec;

    ax += ROTATION_X * dt_sec;
    ay += ROTATION_Y * dt_sec;
    az += ROTATION_Z * dt_sec;
    // Prevent float accumulation drift
    if (ax > 2.0f * M_PI) ax -= 2.0f * M_PI;
    if (ay > 2.0f * M_PI) ay -= 2.0f * M_PI;
    if (az > 2.0f * M_PI) az -= 2.0f * M_PI;

    // Shape cycling with cosine-bell morph: 1 → 0 → 1
    float shape_period = SHAPE_HOLD_SEC + MORPH_SEC;
    float cycle_pos = viewpoint::wrapPositive(time, shape_period * NUM_SHAPES);
    int shape_idx = (int)(cycle_pos / shape_period) % NUM_SHAPES;
    float phase = viewpoint::wrapPositive(cycle_pos, shape_period);

    float morph = 1.0f;
    int draw_shape = shape_idx;
    if (phase > SHAPE_HOLD_SEC) {
        float t = (phase - SHAPE_HOLD_SEC) / MORPH_SEC;
        morph = 0.5f + 0.5f * cos(t * 2.0f * M_PI);
        if (t >= 0.5f) draw_shape = (shape_idx + 1) % NUM_SHAPES;
    }

    float scale = DISPLAY_SCALE * morph;
    if (scale < 0.1f) { view.send(); frame_count++; return; }

    // Draw iso-parameter lines in u direction (constant v, varying u)
    for (int j = 0; j < U_LINES; j++) {
        float v = j * 2.0f * M_PI / U_LINES;
        Vec3 prev = {0, 0, 0};
        for (int i = 0; i <= LINE_DETAIL; i++) {
            float u = i * 2.0f * M_PI / LINE_DETAIL;
            Vec3 p = eval_surface(draw_shape, u, v, time);
            p = rotate3d(p);
            if (i > 0) draw_segment(prev, p, scale);
            prev = p;
        }
    }

    // Draw iso-parameter lines in v direction (constant u, varying v)
    for (int j = 0; j < V_LINES; j++) {
        float u = j * 2.0f * M_PI / V_LINES;
        Vec3 prev = {0, 0, 0};
        for (int i = 0; i <= LINE_DETAIL; i++) {
            float v = i * 2.0f * M_PI / LINE_DETAIL;
            Vec3 p = eval_surface(draw_shape, u, v, time);
            p = rotate3d(p);
            if (i > 0) draw_segment(prev, p, scale);
            prev = p;
        }
    }

    view.send();
    frame_count++;
}
