/**
 * LogMapDataModes.ino
 * @date 03-09-26
 *
 * ViewPoint Log-Scale Test: mapData=true vs mapData=false Comparison
 *
 * Creates 2 stacked Cartesian plots comparing the two log scale modes:
 * - Plot 0: enableLogarithmicScale(true) — plotter maps linear data to log
 * - Plot 1: enableLogarithmicScale(false) — sketch sends pre-computed dB,
 *           plotter just labels the axis logarithmically
 *
 * Both plots receive equivalent data: Plot 0 gets raw linear amplitudes,
 * Plot 1 gets the same data converted to dB (20*log10). The visual result
 * should be identical, validating that both mapData modes render correctly.
 *
 * The signal cycles through patterns that stress different aspects:
 * - Steady tone with amplitude drift across decades
 * - Pulse train with peaks 60dB above the noise floor
 * - Chirp with exponentially increasing amplitude
 * - White noise with varying RMS level
 *
 * Mode: Continuous
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Verify both plots look visually identical (same waveform shape)
 * - Watch pulse events: both plots should show the same peak-to-floor ratio
 * - During chirp: the exponential envelope should appear linear on both
 * - Compare axis labels: Plot 0 shows mapped values, Plot 1 shows dB values
 */

#include <ViewPoint.h>
using namespace viewpoint;

// Signal generation
float signalLevel = 1.0;
float targetLevel = 1.0;
float wavePhase = 0.0;
float waveFreq = 0.1;

enum SignalType {
    AMPLITUDE_DRIFT,
    PULSE_TRAIN,
    EXP_CHIRP,
    NOISY_DECADES,
    NUM_SIGNAL_TYPES
};

SignalType sigType = AMPLITUDE_DRIFT;
unsigned long nextTypeChange = 0;

// Drift state
float driftRate = 0.0;

// Pulse state
float pulseBase = 0.0;
float pulsePeak = 0.0;
bool pulseOn = false;
unsigned long pulseTimer = 0;
unsigned long nextPulse = 0;

// Chirp state
float chirpPhase = 0.0;
float chirpAmpPhase = 0.0;
float chirpAmpSpeed = 0.0;
float chirpAmpMin = 0.0;
float chirpAmpMax = 0.0;

// Noise state
float noiseRMS = 1.0;
float noiseTargetRMS = 1.0;

void pickSignalType();

void setup() {
    randomSeed(analogRead(0));

    view.begin(Mode::Continuous);
    view.setDelay(2);
    view.setNumberOfPlots(2);

    // Plot 0: mapData=true (plotter converts linear to log)
    view.plot(0).setTitle("mapData=true (linear input)");
    view.plot(0).setAxisLabels("Sample", "Amplitude");
    view.plot(0).enableLogarithmicScale(true);

    // Plot 1: mapData=false (sketch sends dB, plotter displays as-is with log labels)
    view.plot(1).setTitle("mapData=false (dB input)");
    view.plot(1).setAxisLabels("Sample", "Level (dB)");
    view.plot(1).enableLogarithmicScale(false);

    view.trace("Linear").setColor(colors::DodgerBlue);
    view.trace("dB").setColor(colors::Coral);

    nextTypeChange = millis() + 4000 + random(4000);
}

void loop() {
    unsigned long now = millis();

    if (now >= nextTypeChange) {
        pickSignalType();
        nextTypeChange = now + 3000 + random(6000);
    }

    float linearValue = 0.0;

    switch (sigType) {
        case AMPLITUDE_DRIFT: {
            // Smooth drift toward target in log space
            float logCurrent = log10(max(signalLevel, 1e-6f));
            float logTarget = log10(max(targetLevel, 1e-6f));
            logCurrent += (logTarget - logCurrent) * driftRate;
            signalLevel = pow(10.0, logCurrent);

            // Wave at current amplitude
            wavePhase += waveFreq;
            if (wavePhase > 2.0 * M_PI) wavePhase -= 2.0 * M_PI;
            float wave = 0.5 + 0.5 * sin(wavePhase);  // 0 to 1
            linearValue = signalLevel * (0.3 + 0.7 * wave);

            // Periodically pick new target
            if (random(300) == 0) {
                targetLevel = pow(10.0, random(7) - 3);
                driftRate = 0.002 + random(10) / 1000.0;
            }
            break;
        }

        case PULSE_TRAIN: {
            if (pulseOn) {
                linearValue = pulsePeak * (0.8 + random(40) / 100.0);
                if (now >= pulseTimer) pulseOn = false;
            } else {
                linearValue = pulseBase * (0.8 + random(40) / 100.0);
                if (now >= nextPulse) {
                    pulseOn = true;
                    pulseTimer = now + 20 + random(80);
                    nextPulse = now + 200 + random(800);
                }
            }
            break;
        }

        case EXP_CHIRP: {
            chirpPhase += 0.1;
            if (chirpPhase > 2.0 * M_PI) chirpPhase -= 2.0 * M_PI;

            chirpAmpPhase += chirpAmpSpeed;
            if (chirpAmpPhase > 1.0) chirpAmpPhase = 0.0;

            float logMin = log10(max(chirpAmpMin, 1e-6f));
            float logMax = log10(max(chirpAmpMax, 1e-6f));
            float logAmp = logMin + chirpAmpPhase * (logMax - logMin);
            float amp = pow(10.0, logAmp);

            linearValue = amp * (0.5 + 0.5 * sin(chirpPhase));
            break;
        }

        case NOISY_DECADES: {
            // Noise with shifting RMS level
            float logRMS = log10(max(noiseRMS, 1e-6f));
            float logTargetRMS = log10(max(noiseTargetRMS, 1e-6f));
            logRMS += (logTargetRMS - logRMS) * 0.003;
            noiseRMS = pow(10.0, logRMS);

            linearValue = noiseRMS * (random(2000) - 1000) / 500.0;
            linearValue = abs(linearValue);  // Keep positive for log

            if (random(200) == 0) {
                noiseTargetRMS = pow(10.0, random(7) - 3);
            }
            break;
        }
    }

    // Ensure positive for log conversion
    linearValue = constrain(max(linearValue, 1e-6f), 1e-6f, 1e6f);

    // Convert to dB
    float dBValue = 20.0 * log10(linearValue);

    // Send to both plots
    view.addData("Linear", linearValue);
    view.addData("dB", dBValue);
    view.send();
}

void pickSignalType() {
    sigType = (SignalType)random(NUM_SIGNAL_TYPES);

    switch (sigType) {
        case AMPLITUDE_DRIFT:
            targetLevel = pow(10.0, random(7) - 3);
            driftRate = 0.002 + random(10) / 1000.0;
            waveFreq = 0.05 + random(20) / 100.0;
            break;

        case PULSE_TRAIN:
            pulseBase = pow(10.0, random(3) - 2);       // 0.01 to 10
            pulsePeak = pulseBase * pow(10.0, 2 + random(4));  // 60-100dB above
            pulseOn = false;
            nextPulse = millis() + random(300);
            break;

        case EXP_CHIRP:
            chirpAmpMin = pow(10.0, random(3) - 3);
            chirpAmpMax = pow(10.0, random(3) + 1);
            chirpAmpPhase = 0.0;
            chirpAmpSpeed = 0.001 + random(5) / 1000.0;
            chirpPhase = 0.0;
            break;

        case NOISY_DECADES:
            noiseRMS = pow(10.0, random(4) - 2);
            noiseTargetRMS = pow(10.0, random(7) - 3);
            break;
    }
}
