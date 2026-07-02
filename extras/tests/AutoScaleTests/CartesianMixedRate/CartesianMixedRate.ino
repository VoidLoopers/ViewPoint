/**
 * CartesianMixedRate.ino
 * @date 03-08-26
 *
 * ViewPoint Auto-Scale Test: Cartesian — Multi-Rate Sensor Fusion
 *
 * Simulates three sensors publishing at very different rates on the
 * same plot. The auto-scaler must track ranges that are dominated by
 * whichever trace is currently most active.
 *
 * Traces:
 * - "Vibration" (fast, every sample):  High-frequency oscillation, ±small
 * - "Temperature" (medium, every 25 samples):  Slow drift, ±moderate
 * - "Alarm" (rare, every ~200 samples):  Occasional huge spike or step
 *
 * Auto-scale challenges:
 * - Most of the time, only the fast trace has fresh data (scale stays small)
 * - When "Temperature" updates, it may push the scale wider
 * - When "Alarm" fires, it forces massive scale expansion
 * - After the alarm subsides, the scale sees mostly "Vibration" data again
 * - The interleaving of different-rate traces creates sparse-vs-dense regions
 *
 * Real-world analogy: Industrial SCADA system where vibration sensors
 * run at kHz, temperature at Hz, and alarm events are asynchronous.
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch how the scale responds when only "Vibration" is updating
 * - Note the scale jump when "Alarm" fires
 * - Compare DEBOUNCED vs WINDOWED strategies
 */

#include <ViewPoint.h>
using namespace viewpoint;

// Update rates (in loop iterations)
#define VIB_EVERY     1      // Every iteration
#define TEMP_EVERY    25     // Every 25th iteration
#define ALARM_MIN     150    // At least 150 iterations between alarms
#define ALARM_MAX     500    // At most 500 iterations between alarms

// State
float vibPhase = 0.0;
float vibFreq = 0.15;
float vibAmp = 1.0;
float vibTargetAmp = 1.0;

float temperature = 25.0;
float tempTarget = 25.0;
float tempScale = 1.0;     // Multiplier to shift temperature into different ranges

float alarmValue = 0.0;
bool alarmActive = false;
unsigned long alarmDecaySamples = 0;

unsigned long sampleCount = 0;
unsigned long nextAlarm = 0;
unsigned long nextTempShift = 0;
unsigned long nextVibShift = 0;

void setup() {
    randomSeed(analogRead(0));
    view.begin(Mode::Continuous);
    view.setDelay(2);

    view.setPlotTitle("Mixed Rate Test");
    view.setAxisLabels("Sample", "Value");

    view.trace("Vibration").setColor(colors::LimeGreen);
    view.trace("Temperature").setColor(colors::Orange);
    view.trace("Alarm").setColor(colors::Red);

    nextAlarm = ALARM_MIN + random(ALARM_MAX - ALARM_MIN);
    nextTempShift = 50 + random(200);
    nextVibShift = 100 + random(300);
}

void loop() {
    // === Fast trace: Vibration (every sample) ===
    vibAmp += (vibTargetAmp - vibAmp) * 0.005;
    vibPhase += vibFreq;
    if (vibPhase > 2.0 * M_PI) vibPhase -= 2.0 * M_PI;

    float vib = vibAmp * sin(vibPhase);
    vib += vibAmp * 0.05 * (random(2000) - 1000) / 1000.0;
    view.addData("Vibration", constrain(vib, -1e6f, 1e6f));

    // === Medium trace: Temperature (every 25 samples) ===
    if (sampleCount % TEMP_EVERY == 0) {
        temperature += (tempTarget - temperature) * 0.02;
        temperature += 0.1 * (random(200) - 100) / 100.0;

        float tempOut = temperature * tempScale;
        view.addData("Temperature", constrain(tempOut, -1e6f, 1e6f));
    }

    // === Rare trace: Alarm (sporadic) ===
    if (alarmActive) {
        alarmDecaySamples--;
        alarmValue *= 0.995;  // Exponential decay
        if (alarmDecaySamples == 0) {
            alarmActive = false;
            alarmValue = 0.0;
        }
        view.addData("Alarm", constrain(alarmValue, -1e6f, 1e6f));
    } else if (sampleCount >= nextAlarm) {
        // Fire alarm
        alarmActive = true;
        int severity = random(5);
        switch (severity) {
            case 0: alarmValue = 10.0 + random(40);          break;  // Minor
            case 1: alarmValue = 50.0 + random(150);         break;  // Moderate
            case 2: alarmValue = 200.0 + random(800);        break;  // Major
            case 3: alarmValue = 1000.0 + random(4000);      break;  // Critical
            case 4: alarmValue = 5000.0 + random(45000);     break;  // Catastrophic
        }
        // Random sign
        if (random(2) == 0) alarmValue = -alarmValue;
        alarmDecaySamples = 50 + random(200);
        nextAlarm = sampleCount + alarmDecaySamples + ALARM_MIN + random(ALARM_MAX - ALARM_MIN);
    }

    // === Parameter shifts ===

    // Vibration amplitude shifts
    if (sampleCount >= nextVibShift) {
        int vibMode = random(4);
        switch (vibMode) {
            case 0: vibTargetAmp = 0.1 + random(100) / 100.0;     break;  // Quiet
            case 1: vibTargetAmp = 2.0 + random(800) / 100.0;     break;  // Normal
            case 2: vibTargetAmp = 10.0 + random(90);              break;  // High
            case 3: vibTargetAmp = 100.0 + random(400);            break;  // Extreme
        }
        vibFreq = 0.05 + random(200) / 1000.0;
        nextVibShift = sampleCount + 200 + random(600);
    }

    // Temperature shifts
    if (sampleCount >= nextTempShift) {
        int tempMode = random(4);
        switch (tempMode) {
            case 0:
                tempTarget = 20 + random(30);
                tempScale = 1.0;  // Normal range
                break;
            case 1:
                tempTarget = 20 + random(30);
                tempScale = 10.0;  // 10x amplified
                break;
            case 2:
                tempTarget = -40 + random(80);
                tempScale = 50.0;  // Large scale
                break;
            case 3:
                tempTarget = 100 + random(900);
                tempScale = 1.0;  // High temp process
                break;
        }
        nextTempShift = sampleCount + 100 + random(400);
    }

    view.send();
    sampleCount++;
}
