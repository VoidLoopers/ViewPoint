/**
 * CartesianLogFrames.ino
 * @date 03-09-26
 *
 * ViewPoint Log-Scale Test: Cartesian — Frames Mode with Log Vertical Axis
 *
 * Generates complete frames of data that span multiple decades on a
 * logarithmic Y axis. Each frame contains a waveform whose amplitude
 * profile changes per-frame: exponential sweeps, decade plateaus,
 * impulse responses, and random decade distributions.
 *
 * This tests whether the log-scale renderer correctly:
 * - Displays full-frame data spanning many decades at once
 * - Renders waveforms where adjacent samples differ by decades
 * - Handles frame-to-frame amplitude changes on a log axis
 * - Shows fine structure within a single decade alongside large spans
 *
 * - Exponential chirp: amplitude grows exponentially across the frame
 * - Decade plateaus: frame divided into bands at different decades
 * - Impulse response: sharp peak decaying across decades
 * - Random scatter: each sample at a randomly chosen decade
 *
 * Mode: Frames
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Compare how exponential chirps look on log vs linear
 * - Notice decade plateaus should appear evenly spaced on log axis
 * - Watch the impulse decay: should be a straight line on log scale
 * - Observe frame-to-frame transitions between patterns
 */

#include <ViewPoint.h>
using namespace viewpoint;

const int FRAME_SIZE = 256;

float frameData[FRAME_SIZE];

// Frame pattern
enum FramePattern {
    EXP_CHIRP,
    DECADE_PLATEAUS,
    IMPULSE_DECAY,
    RANDOM_DECADES,
    SWEEP_AND_RETURN,
    NUM_PATTERNS
};

FramePattern currentPattern = EXP_CHIRP;
int frameCount = 0;
int framesPerPattern = 0;

// Pattern parameters (randomized per cycle)
float chirpLogStart = 0.0;
float chirpLogEnd = 0.0;
int numPlateaus = 0;
float impulseScale = 0.0;
float decayRate = 0.0;

void pickNewPattern();
void generateFrame();

void setup() {
    randomSeed(analogRead(0));
    view.begin(Mode::Frames, FRAME_SIZE);
    view.enableLogarithmicScale();
    view.setDelay(33);  // ~30 fps

    view.setPlotTitle("Log Frames: Multi-Decade Waveforms");
    view.setAxisLabels("Sample Index", "Amplitude");

    view.trace("Frame").setColor(colors::Coral);

    pickNewPattern();
}

void loop() {
    generateFrame();

    view.addData("Frame", frameData, FRAME_SIZE);
    view.send();

    frameCount++;
    if (frameCount >= framesPerPattern) {
        pickNewPattern();
    }
}

void generateFrame() {
    float drift = sin(frameCount * 0.05) * 0.5;

    switch (currentPattern) {
        case EXP_CHIRP: {
            // Amplitude sweeps exponentially across the frame
            float logStart = chirpLogStart;
            float logEnd = chirpLogEnd;
            for (int i = 0; i < FRAME_SIZE; i++) {
                float t = (float)i / (FRAME_SIZE - 1);
                float logVal = logStart + t * (logEnd - logStart) + drift * 0.1;
                float amp = pow(10.0, logVal);
                // Add a sine wave riding on the envelope
                float wave = sin(t * 20.0 * M_PI) * 0.3 + 0.7;
                frameData[i] = constrain(amp * wave, 1e-6f, 1e6f);
            }
            break;
        }

        case DECADE_PLATEAUS: {
            // Frame divided into equal bands at different decades
            int samplesPerPlateau = FRAME_SIZE / numPlateaus;
            for (int i = 0; i < FRAME_SIZE; i++) {
                int plateau = min(i / samplesPerPlateau, numPlateaus - 1);
                float decadeBase = pow(10.0, plateau - 2 + drift * 0.2);
                // Add small variation within the plateau
                float jitter = 1.0 + (random(200) - 100) / 500.0;
                frameData[i] = constrain(decadeBase * jitter, 1e-6f, 1e6f);
            }
            break;
        }

        case IMPULSE_DECAY: {
            // Sharp peak at frame start, exponential decay across decades
            for (int i = 0; i < FRAME_SIZE; i++) {
                float t = (float)i / FRAME_SIZE;
                float amp = impulseScale * exp(-decayRate * t);
                float noise = amp * 0.05 * (random(2000) - 1000) / 1000.0;
                frameData[i] = constrain(amp + noise, 1e-6f, 1e6f);
            }
            // Slowly change decay rate per frame
            decayRate += (random(100) - 50) * 0.01;
            decayRate = constrain(decayRate, 3.0, 20.0);
            break;
        }

        case RANDOM_DECADES: {
            // Each sample lands at a random decade
            for (int i = 0; i < FRAME_SIZE; i++) {
                int decade = random(7) - 3;  // -3 to +3
                float base = pow(10.0, decade);
                float variation = 0.5 + random(100) / 50.0;
                frameData[i] = constrain(base * variation, 1e-6f, 1e6f);
            }
            break;
        }

        case SWEEP_AND_RETURN: {
            // V-shape: sweep up then back down in log space
            float logMin = -2.0 + drift * 0.3;
            float logMax = 4.0 + drift * 0.3;
            for (int i = 0; i < FRAME_SIZE; i++) {
                float t = (float)i / (FRAME_SIZE - 1);
                float triangleT = (t < 0.5) ? (t * 2.0) : (2.0 - t * 2.0);
                float logVal = logMin + triangleT * (logMax - logMin);
                float amp = pow(10.0, logVal);
                float noise = amp * 0.02 * (random(2000) - 1000) / 1000.0;
                frameData[i] = constrain(amp + noise, 1e-6f, 1e6f);
            }
            break;
        }
    }
}

void pickNewPattern() {
    currentPattern = (FramePattern)random(NUM_PATTERNS);
    frameCount = 0;
    framesPerPattern = 30 + random(90);  // 1-4 seconds at 30fps

    switch (currentPattern) {
        case EXP_CHIRP:
            chirpLogStart = random(3) - 3;  // -3 to -1 (decades: 0.001 to 0.1)
            chirpLogEnd = random(3) + 2;    //  2 to  4 (decades: 100 to 10,000)
            if (random(2)) { float tmp = chirpLogStart; chirpLogStart = chirpLogEnd; chirpLogEnd = tmp; }
            break;

        case DECADE_PLATEAUS:
            numPlateaus = 3 + random(5);  // 3 to 7 plateaus
            break;

        case IMPULSE_DECAY:
            impulseScale = pow(10.0, random(4) + 1);  // 10 to 10,000
            decayRate = 5.0 + random(100) / 10.0;
            break;

        case RANDOM_DECADES:
            break;

        case SWEEP_AND_RETURN:
            break;
    }
}
