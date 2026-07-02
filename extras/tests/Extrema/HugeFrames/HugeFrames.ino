/**
 * HugeFrames.ino
 * @date 03-10-26
 *
 * ViewPoint Extrema Test: Cartesian Frames — Extreme Packet Sizes
 *
 * Tests the app's handling of vastly different frame sizes. Starts with
 * a tiny frame (1 sample), ramps through normal sizes, and pushes into
 * very large frames (4096+ samples). After each size holds for several
 * frames, the packet size changes abruptly. Also tests what happens
 * when the declared packet size doesn't match the actual data count.
 *
 * Phases (6 seconds each):
 * - SIZE_1:        Single sample per frame (degenerate case)
 * - SIZE_8:        Tiny frame, 8 samples
 * - SIZE_64:       Small frame
 * - SIZE_512:      Normal frame
 * - SIZE_2048:     Large frame
 * - SIZE_4096:     Very large frame (memory pressure)
 * - SIZE_MISMATCH: Declared 512 but sends 100, then 1000 (mismatched)
 * - SIZE_RANDOM:   Random size each frame between 1 and 2048
 *
 * Mode: Frames
 * Hardware: Any Arduino with sufficient RAM (Teensy 4.1 recommended for 4096)
 *
 * Things to try:
 * - Check if tiny frames (1-8) render at all
 * - Watch for memory issues on large frames
 * - Does SIZE_MISMATCH cause garbled display or crash?
 * - Do random sizes cause rendering artifacts?
 */

#include <ViewPoint.h>
using namespace viewpoint;

// --- Phase definitions ---
enum Phase : uint8_t {
    SIZE_1,
    SIZE_8,
    SIZE_64,
    SIZE_512,
    SIZE_2048,
    SIZE_4096,
    SIZE_MISMATCH,
    SIZE_RANDOM,
    PHASE_COUNT
};

// --- Timing ---
const unsigned long PHASE_DURATION_MS = 6000;

// --- State ---
Phase currentPhase = SIZE_1;
unsigned long phaseStartTime = 0;
unsigned long frameCount = 0;
bool mismatchFlip = false;

// --- Maximum buffer ---
const int MAX_FRAME = 4096;
float buffer[MAX_FRAME];

// --- Forward declarations ---
int currentFrameSize();
void fillBuffer(float* buf, int size);

void setup() {
    randomSeed(analogRead(0));

    view.begin(Mode::Frames, 512);
    view.setDelay(16);
    view.setPlotTitle("Extreme Frame Sizes");
    view.setAxisLabels("Index", "Value");

    view.trace("Signal").setColor(colors::Coral);

    phaseStartTime = millis();
}

void loop() {
    unsigned long now = millis();

    // Phase transition
    if (now - phaseStartTime >= PHASE_DURATION_MS) {
        currentPhase = static_cast<Phase>((currentPhase + 1) % PHASE_COUNT);
        phaseStartTime = now;
        frameCount = 0;

        const char* names[] = {"Size=1", "Size=8", "Size=64", "Size=512",
                               "Size=2048", "Size=4096", "Mismatch", "Random"};
        view.sendInfo("Phase", names[currentPhase]);
    }

    int frameSize = currentFrameSize();

    // Update packet size to match (except in mismatch phase)
    if (currentPhase != SIZE_MISMATCH) {
        view.setPacketSize(frameSize);
    } else {
        view.setPacketSize(512);  // Declare 512 but send different amount
    }

    // Fill and send
    fillBuffer(buffer, frameSize);
    view.addData("Signal", buffer, frameSize);
    view.send();

    frameCount++;
}

int currentFrameSize() {
    switch (currentPhase) {
        case SIZE_1:    return 1;
        case SIZE_8:    return 8;
        case SIZE_64:   return 64;
        case SIZE_512:  return 512;
        case SIZE_2048: return 2048;
        case SIZE_4096: return MAX_FRAME;

        case SIZE_MISMATCH:
            // Alternate between undersized and oversized vs declared 512
            mismatchFlip = !mismatchFlip;
            return mismatchFlip ? 100 : 1000;

        case SIZE_RANDOM:
            return random(1, 2049);

        default:
            return 512;
    }
}

void fillBuffer(float* buf, int size) {
    float freq = 2.0 * M_PI * 3.0 / max(size, 1);  // ~3 cycles per frame
    float amplitude = 5.0 + sin(frameCount * 0.1) * 4.0;  // Slowly varying amplitude

    for (int i = 0; i < size; i++) {
        float t = i * freq;
        buf[i] = sin(t) * amplitude;

        // Add some noise for visual interest
        buf[i] += (random(-100, 100) / 1000.0);
    }
}
