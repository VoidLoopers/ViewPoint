/**
 * EtchASketch.ino
 * @version 1.0.0
 * @date 01-27-26
 * 
 * ViewPoint is brought to you by VoidLoop. See www.voidloop.com for more details.
 * EtchASketch.ino - Classic Drawing Toy Simulation
 *
 * Use two rotary encoders to draw on a scatter plot, just like
 * the classic Etch A Sketch toy:
 * - Left encoder (horizontal knob) controls X position
 * - Right encoder (vertical knob) controls Y position
 * Press either encoder switch to "shake and erase" - clears the drawing.
 *
 * This demonstrates:
 * - Scatter plot with DisplayMode::Persistence for drawing trails
 * - Dual quadrature encoder input
 * - Reference lines for origin crosshair
 * - Symmetric display range centered at origin
 *
 * Verified hardware:
 * - Teensy 4.1
 *
 * Known issues:
 * - Other boards are not supported by this sketch in its current form.
 *
 * Board-specific requirements:
 * - Uses fixed Teensy 4.1 encoder pin mappings and interrupt-capable GPIO.
 * - The sketch expects the two encoder assemblies listed below.
 *
 * Hardware:
 * - Left encoder: A=41, B=40, S=39 (X / horizontal control).
 * - Right encoder: A=22, B=21, S=23 (Y / vertical control).
 *
 * Things to try:
 * - Draw your name or simple shapes
 * - Try drawing a house, star, or spiral
 * - Notice you can't lift the "pen" - continuous line only!
 * - Press switch to shake and erase
 * - Adjust POSITION_PER_CLICK for finer/coarser control
 */

#include <ViewPoint.h>

using namespace viewpoint;

// Display Configuration
#define DISPLAY_RANGE 100.0         // Plot from -100 to +100 on both axes
#define POSITION_PER_CLICK 0.5      // Position change per encoder click

// Left Encoder Pin Configuration (X/Horizontal)
#define LEFT_ENC_A 41
#define LEFT_ENC_B 40
#define LEFT_ENC_S 39

// Right Encoder Pin Configuration (Y/Vertical)
#define RIGHT_ENC_A 22
#define RIGHT_ENC_B 21
#define RIGHT_ENC_S 23

// Switch state for clear() logic
bool last_left_sw  = true;   // INPUT_PULLUP => released is HIGH (true)
bool last_right_sw = true;
// Encoder state (volatile for ISR access)
volatile long left_encoder_count = 0;
volatile long right_encoder_count = 0;

// Last encoder readings for delta tracking
long last_left_count = 0;
long last_right_count = 0;

// Position tracking
float pos_x = 0.0;      // Current X position
float pos_y = 0.0;      // Current Y position

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

    // Initialize ViewPoint in Scatter mode
    view.begin(PlotType::Scatter, Mode::Continuous, 2000);
    view.setTitle("Etch-A-Sketch");

    // Enable persistence for drawing trail effect
    // view.setDisplayMode(DisplayMode::Persistence);

    // Symmetric axes centered at origin
    view.setHorizontalRange(-DISPLAY_RANGE, DISPLAY_RANGE);
    view.setVerticalRange(-DISPLAY_RANGE, DISPLAY_RANGE);

    // Add crosshair reference lines at origin
    view.addHorizontalReferenceLine(0.0);
    view.addVerticalReferenceLine(0.0);

    // Style the drawing trace
    view.trace("Pen").setColor(0xFF0000);   // Red pen (like classic Etch A Sketch)

    // Labels
    view.setAxisLabels("X", "Y");
    view.setPlotTitle("Etch A Sketch");

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

    // Update X position from left encoder (horizontal knob)
    pos_x += left_delta * POSITION_PER_CLICK;
    pos_x = constrain(pos_x, -DISPLAY_RANGE, DISPLAY_RANGE);

    // Update Y position from right encoder (vertical knob)
    // Invert direction so clockwise = right / up feels natural
    pos_y -= right_delta * POSITION_PER_CLICK;
    pos_y = constrain(pos_y, -DISPLAY_RANGE, DISPLAY_RANGE);

    // Update Y position from right encoder (vertical knob)
    // pos_y += right_delta * POSITION_PER_CLICK;
    // pos_y = constrain(pos_y, -DISPLAY_RANGE, DISPLAY_RANGE);

    // Send current position to draw
    view.addData("Pen", pos_x, pos_y);
    view.send();
}

// Clear the drawing and reset position to center
void clearDrawing() {
    // Reset position to center (origin)
    pos_x = 0.0;
    pos_y = 0.0;

    // Sync last counts with current to prevent jump on next read
    noInterrupts();
    last_left_count = left_encoder_count;
    last_right_count = right_encoder_count;
    interrupts();
}

// Left encoder ISR (X/horizontal control)
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

// Right encoder ISR (Y/vertical control)
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
