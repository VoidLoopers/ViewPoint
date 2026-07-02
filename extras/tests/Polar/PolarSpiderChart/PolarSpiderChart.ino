/**
 * PolarSpiderChart.ino
 * @date 03-10-26
 *
 * ViewPoint Polar Test: Frames — Multi-Parameter Process Spider Chart
 *
 * Simulates a spider (radar) chart for a turbine generator monitoring
 * system. Eight process parameters are each assigned a fixed compass
 * bearing and plotted as radial spokes from the origin. A polygon
 * connecting the spoke tips shows the overall machine "health shape."
 *
 * Three traces, each using addBreak differently:
 *
 * "Spokes" — Eight discrete radial lines (origin → value), with
 *   addBreak() between each spoke. Without breaks, the renderer would
 *   draw lines from one spoke tip across the chart to the next origin.
 *
 * "Profile" — A closed polygon connecting the tips of all eight
 *   parameters. No breaks — this is a continuous closed shape.
 *
 * "Alarm" — Alarm threshold arcs drawn at the alarm radius for each
 *   parameter, with addBreak() between non-adjacent sectors when a
 *   parameter is in alarm (red zone). Only alarming sectors are drawn.
 *
 * Process conditions:
 * - NOMINAL:     All parameters near 50-70% of scale, tight polygon
 * - WARMING:     Temperature and vibration drift upward over time
 * - OVERLOAD:    Current and power spike, RPM sags, pressure rises
 * - BEARING_WEAR: Vibration spikes to 95%+, oil pressure drops
 * - SURGE:       Pressure oscillates wildly, flow reverses (drops to ~5%)
 * - SENSOR_FAULT: One or two parameters jump to 0% or 100% (stuck sensor)
 * - RECOVERY:    All parameters drift back toward nominal
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch the polygon shape distort during OVERLOAD vs BEARING_WEAR
 * - Note how alarm spokes appear only for out-of-limit parameters
 * - See how the radial scale handles SURGE oscillations
 * - Compare the visual clarity of spokes (with breaks) vs what happens
 *   if you mentally connect them (the polygon trace shows that shape)
 */

#include <ViewPoint.h>
using namespace viewpoint;

// --- Process parameters ---
#define NUM_PARAMS 8

const char* paramNames[] = {
    "Temp", "Pressure", "Flow", "Vibration",
    "RPM", "Current", "OilPres", "Power"
};

// Each parameter's angular position (evenly spaced)
const float paramAngle[NUM_PARAMS] = {
    0.0, 45.0, 90.0, 135.0, 180.0, 225.0, 270.0, 315.0
};

// Alarm thresholds (percentage of full scale)
const float alarmHigh[NUM_PARAMS] = {
    85.0, 90.0, 95.0, 80.0,    // Temp, Pressure, Flow, Vibration
    92.0, 88.0, 20.0, 90.0     // RPM, Current, OilPres (low alarm!), Power
};

// Whether alarm triggers on HIGH (true) or LOW (false)
const bool alarmIsHigh[NUM_PARAMS] = {
    true, true, false, true,    // Temp high, Pres high, Flow low(!), Vib high
    false, true, false, true    // RPM low(!), Current high, OilPres low(!), Power high
};

// --- State ---
float paramValue[NUM_PARAMS];
float targetValue[NUM_PARAMS];
float paramDriftRate[NUM_PARAMS];

enum Condition : uint8_t {
    NOMINAL,
    WARMING,
    OVERLOAD,
    BEARING_WEAR,
    SURGE,
    SENSOR_FAULT,
    RECOVERY,
    NUM_CONDITIONS
};

Condition currentCondition = NOMINAL;

// Surge oscillation state
float surgePhase = 0.0;
float surgeFreq = 0.3;

// Sensor fault tracking
int faultedSensor1 = -1;
int faultedSensor2 = -1;

// Timing
unsigned long nextConditionChange = 0;
unsigned long nextPerturbation = 0;
unsigned long frameCount = 0;

// Frame size: spokes (2 pts each + break handled via addBreak) +
//             profile (NUM_PARAMS + 1 to close) + alarm segments
#define MAX_POINTS_PER_FRAME  200

void setCondition(Condition c);
bool isAlarming(int param);

void setup() {
    randomSeed(analogRead(0));
    view.begin(PlotType::Polar, Mode::Frames, MAX_POINTS_PER_FRAME);
    view.setDelay(50);

    view.setAngularOffset(90);
    view.setAngularStep(45);
    view.setPlotTitle("Turbine Generator Health");

    view.trace("Spokes").setColor(colors::DimGray);
    view.trace("Profile").setColor(colors::DodgerBlue);
    view.trace("Alarm").setColor(colors::Crimson);

    // Initialize all parameters to nominal
    for (int i = 0; i < NUM_PARAMS; i++) {
        paramValue[i] = 50.0 + random(2000) / 100.0;
        targetValue[i] = paramValue[i];
        paramDriftRate[i] = 0.02 + random(30) / 1000.0;
    }

    nextConditionChange = millis() + 4000 + random(4000);
    nextPerturbation = millis() + 1000 + random(2000);
}

void loop() {
    unsigned long now = millis();

    // Condition transitions
    if (now >= nextConditionChange) {
        Condition next = (Condition)random(NUM_CONDITIONS);
        setCondition(next);
        nextConditionChange = now + 4000 + random(8000);
    }

    // Small random perturbations
    if (now >= nextPerturbation) {
        int p = random(NUM_PARAMS);
        targetValue[p] += (random(2000) - 1000) / 100.0;
        targetValue[p] = constrain(targetValue[p], 0.0f, 100.0f);
        nextPerturbation = now + 500 + random(2000);
    }

    // Surge oscillation
    if (currentCondition == SURGE) {
        surgePhase += surgeFreq;
        if (surgePhase > 2.0 * M_PI) surgePhase -= 2.0 * M_PI;
        // Pressure oscillates
        targetValue[1] = 50.0 + 45.0 * sin(surgePhase);
        // Flow drops and recovers
        targetValue[2] = 30.0 + 25.0 * sin(surgePhase + M_PI / 3.0);
        // Vibration follows pressure
        targetValue[3] = 40.0 + 35.0 * abs(sin(surgePhase * 1.5));
    }

    // Sensor fault: force faulted sensors to stuck values
    if (currentCondition == SENSOR_FAULT) {
        if (faultedSensor1 >= 0)
            targetValue[faultedSensor1] = (random(2) == 0) ? 0.0 : 100.0;
        if (faultedSensor2 >= 0)
            targetValue[faultedSensor2] = (random(2) == 0) ? 100.0 : 0.0;
    }

    // Update parameter values toward targets
    for (int i = 0; i < NUM_PARAMS; i++) {
        paramValue[i] += (targetValue[i] - paramValue[i]) * paramDriftRate[i];
        // Measurement noise
        float noise = 0.5 * (random(2000) - 1000) / 1000.0;
        float displayed = constrain(paramValue[i] + noise, 0.0f, 100.0f);
        paramValue[i] = displayed;
    }

    // === TRACE 1: Spokes (with breaks) ===
    // Each spoke is a discrete radial line from origin to the parameter value.
    // addBreak() prevents the renderer from connecting one spoke tip to the
    // next spoke origin.
    for (int i = 0; i < NUM_PARAMS; i++) {
        float val = constrain(paramValue[i], 0.0f, 100.0f);
        view.addData("Spokes", 0.0, paramAngle[i]);
        view.addData("Spokes", val, paramAngle[i]);
        view.addBreak("Spokes");
    }

    // === TRACE 2: Profile polygon (no breaks) ===
    // A closed polygon connecting all parameter tips in order.
    for (int i = 0; i < NUM_PARAMS; i++) {
        float val = constrain(paramValue[i], 0.0f, 100.0f);
        view.addData("Profile", val, paramAngle[i]);
    }
    // Close the polygon by repeating the first point
    view.addData("Profile", constrain(paramValue[0], 0.0f, 100.0f), paramAngle[0]);

    // === TRACE 3: Alarm indicators (with breaks) ===
    // For each parameter in alarm, draw a short radial tick at the alarm
    // threshold radius. Breaks separate each alarm indicator so they don't
    // connect across the chart.
    for (int i = 0; i < NUM_PARAMS; i++) {
        if (isAlarming(i)) {
            float alarmRadius = alarmHigh[i];
            float halfTick = 8.0;  // Angular half-width of alarm tick

            // Draw a short arc at the alarm radius
            view.addData("Alarm", alarmRadius, paramAngle[i] - halfTick);
            view.addData("Alarm", alarmRadius, paramAngle[i]);
            view.addData("Alarm", alarmRadius, paramAngle[i] + halfTick);

            // Then a radial line from alarm radius to the parameter value
            // to show how far into the alarm zone it is
            view.addData("Alarm", constrain(paramValue[i], 0.0f, 100.0f), paramAngle[i]);
            view.addBreak("Alarm");
        }
    }

    view.send();
    frameCount++;
}

bool isAlarming(int param) {
    if (alarmIsHigh[param]) {
        return paramValue[param] > alarmHigh[param];
    } else {
        // Low alarm: triggers when value drops BELOW threshold
        return paramValue[param] < alarmHigh[param];
    }
}

void setCondition(Condition c) {
    currentCondition = c;
    faultedSensor1 = -1;
    faultedSensor2 = -1;

    // Set drift rates (some conditions change slowly, others fast)
    float baseRate;
    switch (c) {
        case WARMING:    baseRate = 0.01;  break;
        case SURGE:      baseRate = 0.08;  break;
        case OVERLOAD:   baseRate = 0.05;  break;
        default:         baseRate = 0.03;  break;
    }
    for (int i = 0; i < NUM_PARAMS; i++) {
        paramDriftRate[i] = baseRate + random(20) / 1000.0;
    }

    switch (c) {
        case NOMINAL:
            for (int i = 0; i < NUM_PARAMS; i++) {
                targetValue[i] = 50.0 + random(2000) / 100.0;  // 50-70%
            }
            break;

        case WARMING:
            // Temperature and vibration climb, others stable
            targetValue[0] = 75.0 + random(2000) / 100.0;  // Temp rising
            targetValue[3] = 65.0 + random(2000) / 100.0;  // Vib rising
            targetValue[1] = 55.0 + random(1000) / 100.0;  // Pressure slightly up
            targetValue[2] = 60.0 + random(1500) / 100.0;  // Flow nominal
            targetValue[4] = 55.0 + random(1000) / 100.0;  // RPM steady
            targetValue[5] = 60.0 + random(1000) / 100.0;  // Current up slightly
            targetValue[6] = 50.0 + random(1500) / 100.0;  // Oil pres nominal
            targetValue[7] = 60.0 + random(1000) / 100.0;  // Power up
            break;

        case OVERLOAD:
            targetValue[0] = 70.0 + random(1500) / 100.0;  // Temp high
            targetValue[1] = 80.0 + random(1500) / 100.0;  // Pressure high
            targetValue[2] = 70.0 + random(2000) / 100.0;  // Flow high
            targetValue[3] = 60.0 + random(2000) / 100.0;  // Vib moderate
            targetValue[4] = 35.0 + random(1500) / 100.0;  // RPM sagging
            targetValue[5] = 85.0 + random(1500) / 100.0;  // Current very high
            targetValue[6] = 40.0 + random(1500) / 100.0;  // Oil pres dropping
            targetValue[7] = 90.0 + random(1000) / 100.0;  // Power maxed
            break;

        case BEARING_WEAR:
            targetValue[0] = 65.0 + random(1500) / 100.0;  // Temp elevated
            targetValue[1] = 55.0 + random(1000) / 100.0;  // Pressure normal
            targetValue[2] = 55.0 + random(1000) / 100.0;  // Flow normal
            targetValue[3] = 88.0 + random(1200) / 100.0;  // Vib VERY high
            targetValue[4] = 50.0 + random(1000) / 100.0;  // RPM nominal
            targetValue[5] = 62.0 + random(1000) / 100.0;  // Current up
            targetValue[6] = 15.0 + random(1000) / 100.0;  // Oil pres LOW
            targetValue[7] = 55.0 + random(1000) / 100.0;  // Power normal
            break;

        case SURGE:
            surgePhase = 0.0;
            surgeFreq = 0.1 + random(400) / 1000.0;
            // Pressure, flow, vibration will oscillate in loop()
            targetValue[0] = 70.0 + random(1000) / 100.0;
            targetValue[4] = 45.0 + random(1000) / 100.0;  // RPM unstable
            targetValue[5] = 70.0 + random(1000) / 100.0;
            targetValue[6] = 35.0 + random(1500) / 100.0;
            targetValue[7] = 50.0 + random(2000) / 100.0;
            break;

        case SENSOR_FAULT:
            // Pick 1-2 sensors to fault
            faultedSensor1 = random(NUM_PARAMS);
            if (random(2) == 0) {
                faultedSensor2 = random(NUM_PARAMS);
                while (faultedSensor2 == faultedSensor1) faultedSensor2 = random(NUM_PARAMS);
            }
            // Other parameters stay near current values
            for (int i = 0; i < NUM_PARAMS; i++) {
                if (i != faultedSensor1 && i != faultedSensor2) {
                    targetValue[i] = paramValue[i] + (random(1000) - 500) / 100.0;
                    targetValue[i] = constrain(targetValue[i], 10.0f, 90.0f);
                }
            }
            break;

        case RECOVERY:
            for (int i = 0; i < NUM_PARAMS; i++) {
                targetValue[i] = 50.0 + random(1500) / 100.0;  // Drift back to nominal
            }
            break;
    }
}
