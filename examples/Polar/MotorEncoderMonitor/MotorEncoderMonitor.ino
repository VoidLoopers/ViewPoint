/**
 * MotorEncoderMonitor.ino
 * @version V1R1 R0.2
 * @date 01-27-26
 * 
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * MotorEncoderMonitor.ino - Rotational Position/Velocity Display
 *
 * Visualizes motor encoder position and velocity on a polar plot.
 * Uses quadrature encoder input to track shaft rotation in real-time,
 * showing both angular position and rotational velocity.
 *
 * This demonstrates:
 * - Polar plot for rotational position display
 * - Quadrature encoder reading
 * - Multiple traces (position marker, velocity indicator)
 * - Real-time hardware input processing
 *
 * Verified hardware:
 * - Teensy 4.1
 *
 * Known issues:
 * - Other boards are not supported by this sketch in its current form.
 *
 * Board-specific requirements:
 * - Uses fixed Teensy 4.1 encoder pin mappings and interrupt-capable GPIO.
 * - The sketch expects a quadrature encoder on the left encoder connector listed below.
 *
 * Hardware:
 * - Quadrature encoder: S=39, A=41, B=40.
 * - Adjust COUNTS_PER_REV if your encoder resolution differs from the default.
 *
 * Things to try:
 * - Rotate encoder and watch position track on polar display
 * - Fast rotation shows larger velocity radius
 * - Connect a DC motor with encoder for continuous rotation
 * - Change COUNTS_PER_REV for your encoder resolution
 */

#include <ViewPoint.h>

using namespace viewpoint;

// Encoder Pin Configuration (Left Encoder from VoidLoop Board Collection)
#define ENCODER_PIN_A 41
#define ENCODER_PIN_B 40
#define ENCODER_PIN_S 39        // Switch/button (optional)

// Encoder Configuration
#define COUNTS_PER_REV 400      // Encoder counts per revolution (adjust for your encoder)
#define DEGREES_PER_COUNT (360.0 / COUNTS_PER_REV)

// Display Configuration
#define POSITION_RADIUS 80      // Fixed radius for position indicator
#define MAX_VELOCITY 100        // Maximum velocity display radius
#define VELOCITY_SCALE 0.1      // Scale factor: counts/sec to display radius

// Encoder state
volatile long encoder_count = 0;
long last_count = 0;
unsigned long last_time = 0;

// Velocity calculation
float velocity = 0;             // Counts per second

void setup() {
    // Configure encoder pins with pullups
    pinMode(ENCODER_PIN_A, INPUT_PULLUP);
    pinMode(ENCODER_PIN_B, INPUT_PULLUP);
    pinMode(ENCODER_PIN_S, INPUT_PULLUP);

    // Attach interrupt for encoder A channel
    // Both edges for full quadrature resolution
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), encoderISR, CHANGE);

    // Initialize ViewPoint in Polar mode
    view.begin(PlotType::Polar, Mode::Continuous);
    view.setTitle("Motor Encoder Monitor");

    // Put 0 degrees at top (like a compass/clock)
    view.setAngularOffset(90);
    view.setAngularStep(30);    // 12 divisions like a clock

    // Radial range for velocity display
    view.setRadialRange(0, MAX_VELOCITY);

    // Create traces for position and velocity
    view.trace("Position").setColor(0x00FF00);  // Green
    view.trace("Velocity").setColor(0xFF6600);  // Orange

    view.setPlotTitle("Encoder Monitor");

    // Update rate
    view.setDelay(5);

    // Initialize timing
    last_time = micros();
}

void loop() {
    // Calculate time delta
    unsigned long current_time = micros();
    float dt = (current_time - last_time) / 1000000.0;  // Convert to seconds

    // Get current encoder count (atomic read)
    noInterrupts();
    long current_count = encoder_count;
    interrupts();

    // Calculate velocity (counts per second)
    if (dt > 0) {
        long delta_count = current_count - last_count;
        velocity = delta_count / dt;
    }

    // Convert encoder count to angle (0-360)
    float angle = viewpoint::wrapPositive((float)(current_count * DEGREES_PER_COUNT), 360.0f);
    if (angle < 0) angle += 360;

    // Position trace: fixed radius at current angle
    view.addData("Position", POSITION_RADIUS, angle); // Add twice to keep in pace with Velocity
    view.addData("Position", POSITION_RADIUS, angle); // which is recentered with each new point

    // Velocity trace: radius proportional to speed, same angle
    // Positive velocity = outward, negative = smaller radius
    float vel_radius = constrain(abs(velocity) * VELOCITY_SCALE, 0, MAX_VELOCITY);
    view.addData("Velocity", 0.0, angle); // Center back to the origin7
    view.addData("Velocity", vel_radius, angle);

    view.send();

    // Store for next iteration
    last_count = current_count;
    last_time = current_time;
}

// Interrupt service routine for encoder
// Full quadrature decoding for maximum resolution
void encoderISR() {
    static uint8_t last_state = 0;

    uint8_t a = digitalRead(ENCODER_PIN_A);
    uint8_t b = digitalRead(ENCODER_PIN_B);
    uint8_t current_state = (a << 1) | b;

    // State transition table for quadrature decoding
    // Determines direction based on state change
    const int8_t transition[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
    uint8_t index = (last_state << 2) | current_state;

    encoder_count += transition[index];
    last_state = current_state;
}
