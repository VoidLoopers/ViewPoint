/**
 * ExtremeValues.ino
 * @date 03-10-26
 *
 * ViewPoint Extrema Test: Cartesian — Pathological Float Values
 *
 * Cycles through a series of extreme floating-point values designed to
 * break parsing, rendering, and auto-scaling in the desktop app. Each
 * phase sends a different class of problematic value: NaN, positive and
 * negative infinity, FLT_MAX/-FLT_MAX, denormals (subnormal floats),
 * negative zero, very small values near epsilon, and rapid alternation
 * between extremes. A "normal" sine wave trace runs alongside for
 * visual contrast—if it disappears or distorts, something is wrong.
 *
 * Phases (5 seconds each, cycling):
 * - PHASE_NAN:         Sends NaN (0.0/0.0) every sample
 * - PHASE_INF:         Alternates +Infinity and -Infinity
 * - PHASE_FLT_MAX:     Sends FLT_MAX and -FLT_MAX
 * - PHASE_DENORMAL:    Sends subnormal floats (~1e-40 to 1e-38)
 * - PHASE_NEG_ZERO:    Sends -0.0 vs +0.0
 * - PHASE_EPSILON:     Values at float epsilon boundary (~1.19e-7)
 * - PHASE_ALTERNATING: Rapid jumps between 1e-38, 0, 1e6, NaN, Inf
 * - PHASE_NORMAL:      Clean sine wave (recovery test)
 *
 * Mode: Continuous
 * Hardware: Any Arduino
 *
 * Things to try:
 * - Watch if the normal trace stays visible through all phases
 * - Check if the app crashes or hangs on NaN/Infinity
 * - See if auto-scale recovers after FLT_MAX exposure
 * - Monitor serial output for garbled data during alternating phase
 */

#include <ViewPoint.h>
using namespace viewpoint;

#include <float.h>
#include <math.h>

// --- Phase definitions ---
enum Phase : uint8_t {
    PHASE_NAN,
    PHASE_INF,
    PHASE_FLT_MAX,
    PHASE_DENORMAL,
    PHASE_NEG_ZERO,
    PHASE_EPSILON,
    PHASE_ALTERNATING,
    PHASE_NORMAL,
    PHASE_COUNT
};

// --- Timing ---
const unsigned long PHASE_DURATION_MS = 5000;

// --- State ---
Phase currentPhase = PHASE_NAN;
unsigned long phaseStartTime = 0;
float normalPhase = 0.0;
unsigned long sampleCount = 0;

// --- Forward declarations ---
float generateExtremeValue();
const char* phaseName(Phase p);

void setup() {
    randomSeed(analogRead(0));

    view.begin(Mode::Continuous);
    view.setDelay(2);
    view.setPlotTitle("Extreme Float Values");
    view.setAxisLabels("Sample", "Value");

    view.trace("Extreme").setColor(colors::Crimson);
    view.trace("Normal").setColor(colors::DodgerBlue);

    phaseStartTime = millis();
}

void loop() {
    unsigned long now = millis();

    // Phase transition
    if (now - phaseStartTime >= PHASE_DURATION_MS) {
        currentPhase = static_cast<Phase>((currentPhase + 1) % PHASE_COUNT);
        phaseStartTime = now;
        sampleCount = 0;

        view.sendInfo("Phase Change", phaseName(currentPhase));
    }

    // Generate extreme value for current phase
    float extreme = generateExtremeValue();

    // Normal reference sine (always well-behaved)
    normalPhase += 0.05;
    if (normalPhase > 2.0 * M_PI) normalPhase -= 2.0 * M_PI;
    float normal = sin(normalPhase) * 5.0;

    view.addData("Extreme", extreme);
    view.addData("Normal", normal);
    view.send();

    sampleCount++;
}

float generateExtremeValue() {
    float t = sampleCount * 0.01;

    switch (currentPhase) {
        case PHASE_NAN: {
            // Various ways to produce NaN
            if (sampleCount % 3 == 0) return 0.0f / 0.0f;
            if (sampleCount % 3 == 1) return nanf("");
            return sqrt(-1.0f);
        }

        case PHASE_INF: {
            // Alternate positive and negative infinity
            if (sampleCount % 2 == 0) return INFINITY;
            return -INFINITY;
        }

        case PHASE_FLT_MAX: {
            // Alternate FLT_MAX boundaries
            if (sampleCount % 4 == 0) return FLT_MAX;
            if (sampleCount % 4 == 1) return -FLT_MAX;
            if (sampleCount % 4 == 2) return FLT_MAX * 0.999f;
            return -FLT_MAX * 0.999f;
        }

        case PHASE_DENORMAL: {
            // Subnormal floats - smallest representable values
            float base = FLT_MIN * 0.5f;  // Below FLT_MIN = denormal
            float scale = (sampleCount % 100) / 100.0f;
            return base * (0.01f + scale);
        }

        case PHASE_NEG_ZERO: {
            // Alternate between -0.0 and +0.0
            if (sampleCount % 2 == 0) return -0.0f;
            return 0.0f;
        }

        case PHASE_EPSILON: {
            // Values near float epsilon
            float base = FLT_EPSILON;
            return base * (1.0f + sin(t) * 0.5f);
        }

        case PHASE_ALTERNATING: {
            // Rapid jumps between wildly different values
            int pattern = sampleCount % 7;
            switch (pattern) {
                case 0: return 1e-38f;
                case 1: return 0.0f;
                case 2: return 1e6f;
                case 3: return 0.0f / 0.0f;  // NaN
                case 4: return INFINITY;
                case 5: return -1e6f;
                default: return -INFINITY;
            }
        }

        case PHASE_NORMAL: {
            // Clean sine for recovery observation
            return sin(t) * 10.0;
        }

        default:
            return 0.0f;
    }
}

const char* phaseName(Phase p) {
    switch (p) {
        case PHASE_NAN:         return "NaN";
        case PHASE_INF:         return "Infinity";
        case PHASE_FLT_MAX:     return "FLT_MAX";
        case PHASE_DENORMAL:    return "Denormals";
        case PHASE_NEG_ZERO:    return "Negative Zero";
        case PHASE_EPSILON:     return "Epsilon";
        case PHASE_ALTERNATING: return "Alternating Extremes";
        case PHASE_NORMAL:      return "Normal Recovery";
        default:                return "Unknown";
    }
}
