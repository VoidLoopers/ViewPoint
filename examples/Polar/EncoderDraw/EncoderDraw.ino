/**
 * EncoderDraw.ino
 * @version 1.0.0
 * @date 01-27-26
 * 
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * EncoderDraw.ino - Dual Encoder Polar Drawing
 *
 * Draw points on a polar plot using two rotary encoders:
 * - Left encoder controls the angle (theta)
 * - Right encoder controls the radius (rho)
 * Press either encoder switch to clear and reset to center.
 *
 * This demonstrates:
 * - Dual quadrature encoder input
 * - Angular offset for north-up orientation
 * - Configurable degrees/radians display
 *
 * Verified hardware:
 * - Teensy 4.1
 *
 * Known issues:
 * - Other boards are not supported by this sketch in its current form.
 *
 * Board-specific requirements:
 * - Uses fixed Teensy 4.1 encoder pin mappings and interrupt-capable GPIO.
 * - The sketch expects the VoidLoop encoder pinout listed below.
 *
 * Hardware:
 * - Left encoder: A=41, B=40, S=39 (theta / angle control).
 * - Right encoder: A=22, B=21, S=23 (rho / radius control).
 *
 * Things to try:
 * - Draw a spiral by rotating both encoders
 * - Draw circles by holding radius steady and spinning angle
 * - Draw radial lines by holding angle and moving radius
 * - Press switch to clear and start over
 * - Toggle USE_RADIANS for different angle display
 */

#include <ViewPoint.h>

using namespace viewpoint;

// Display Configuration
const bool USE_RADIANS = false;     // false = degrees, true = radians

// Encoder Movement Scaling
#define DEGREES_PER_CLICK 2.0       // Angle change per encoder click
#define RADIUS_PER_CLICK 1.0        // Radius change per encoder click
#define MAX_RADIUS 100.0            // Maximum radial distance
#define MIN_RADIUS 0.0              // Minimum radial distance

// Left Encoder Pin Configuration (Angle/Theta)
#define LEFT_ENC_A 41
#define LEFT_ENC_B 40
#define LEFT_ENC_S 39

// Right Encoder Pin Configuration (Radius/Rho)
#define RIGHT_ENC_A 22
#define RIGHT_ENC_B 21
#define RIGHT_ENC_S 23

// Encoder state (volatile for ISR access)
volatile long left_encoder_count = 0;
volatile long right_encoder_count = 0;

// Last encoder readings for delta tracking
long last_left_count = 0;
long last_right_count = 0;

// Position tracking
float theta = 0.0;      // Current angle in degrees (internal)
float rho = 50.0;       // Current radius (start at middle)

// Debounce for switches
unsigned long last_clear_time = 0;
#define DEBOUNCE_MS 200

void setup() {
    // Configure encoder pins with internal pullups
    pinMode(LEFT_ENC_A, INPUT_PULLUP);
    pinMode(LEFT_ENC_B, INPUT_PULLUP);
    pinMode(LEFT_ENC_S, INPUT_PULLUP);
    pinMode(RIGHT_ENC_A, INPUT_PULLUP);
    pinMode(RIGHT_ENC_B, INPUT_PULLUP);
    pinMode(RIGHT_ENC_S, INPUT_PULLUP);

    // Attach interrupts for full quadrature decoding
    attachInterrupt(digitalPinToInterrupt(LEFT_ENC_A), leftEncoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(LEFT_ENC_B), leftEncoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(RIGHT_ENC_A), rightEncoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(RIGHT_ENC_B), rightEncoderISR, CHANGE);

    // Initialize ViewPoint in Polar mode
    view.begin(PlotType::Polar, Mode::Continuous);
    view.setTitle("Encoder Draw");

    // North-up orientation (0 degrees at top)
    view.setAngularOffset(90);
    view.setAngularStep(30);        // 12 divisions like clock

    // Set angular units based on configuration
    if (USE_RADIANS) {
        view.setAngularUnits(AngularUnit::Radians);
    }

    // Radial range
    view.setRadialRange(MIN_RADIUS, MAX_RADIUS);

    // Style the drawing trace
    view.trace("Pen").setColor(0x00FF00);   // Green pen

    view.setPlotTitle("Encoder Draw");
    view.setDelay(10);  // Fast updates for smooth drawing
}

void loop() {
    // Check for clear button press (either switch = "shake to erase")
    static bool last_left_pressed  = false;
    static bool last_right_pressed = false;

    bool left_pressed_now  = (digitalRead(LEFT_ENC_S)  == LOW);
    bool right_pressed_now = (digitalRead(RIGHT_ENC_S) == LOW);

    bool left_rising  = (!last_left_pressed  && left_pressed_now);
    bool right_rising = (!last_right_pressed && right_pressed_now);

    last_left_pressed  = left_pressed_now;
    last_right_pressed = right_pressed_now;

    if ((left_rising || right_rising) && (millis() - last_clear_time > DEBOUNCE_MS)) {
        clearDrawing();
        last_clear_time = millis();
    }

    // Atomically read encoder counts
    noInterrupts();
    long left_count = left_encoder_count;
    long right_count = right_encoder_count;
    interrupts();

    // Calculate delta from last reading
    long left_delta = left_count - last_left_count;
    long right_delta = right_count - last_right_count;
    last_left_count = left_count;
    last_right_count = right_count;

    // Update angle from left encoder
    theta += left_delta * DEGREES_PER_CLICK;

    // Wrap angle to 0-360 range
    while (theta >= 360.0) theta -= 360.0;
    while (theta < 0.0) theta += 360.0;

    // Update radius from right encoder
    rho += right_delta * RADIUS_PER_CLICK;
    rho = constrain(rho, MIN_RADIUS, MAX_RADIUS);

    // Convert angle to display units if needed
    float display_angle = theta;
    if (USE_RADIANS) {
        display_angle = theta * PI / 180.0;
    }

    // Send current position to draw
    view.addData("Pen", rho, display_angle);
    view.send();
}

// Clear the drawing and reset position
void clearDrawing() {
    // Reset position to center
    theta = 0.0;
    rho = 50.0;

    // Sync last counts with current to prevent jump on next read
    noInterrupts();
    last_left_count = left_encoder_count;
    last_right_count = right_encoder_count;
    interrupts();
}

// Left encoder ISR (theta/angle control)
void leftEncoderISR() {
    static uint8_t last_state = 0;

    uint8_t a = digitalRead(LEFT_ENC_A);
    uint8_t b = digitalRead(LEFT_ENC_B);
    uint8_t current_state = (a << 1) | b;

    // Quadrature state transition table
    const int8_t transition[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
    uint8_t index = (last_state << 2) | current_state;

    left_encoder_count += transition[index];
    last_state = current_state;
}

// Right encoder ISR (rho/radius control)
void rightEncoderISR() {
    static uint8_t last_state = 0;

    uint8_t a = digitalRead(RIGHT_ENC_A);
    uint8_t b = digitalRead(RIGHT_ENC_B);
    uint8_t current_state = (a << 1) | b;

    // Quadrature state transition table
    const int8_t transition[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
    uint8_t index = (last_state << 2) | current_state;

    right_encoder_count += transition[index];
    last_state = current_state;
}
