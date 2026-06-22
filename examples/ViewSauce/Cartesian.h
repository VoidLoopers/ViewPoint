#ifndef VIEWSAUCE_CARTESIAN_H
#define VIEWSAUCE_CARTESIAN_H

#include "Config.h"

// ═════════════════════════════════════════════════════════════════════
//  Scene 1: ECG Monitor  (Cartesian / Continuous)
// ═════════════════════════════════════════════════════════════════════

#define ECG_SAMPLE_RATE_MS    4       // 250 Hz
#define ECG_BASE_HEART_RATE   72      // BPM
#define ECG_HR_VARIABILITY    5.0f    // +/- BPM

float ecg_phase         = 0.0f;
float ecg_beat_period   = 60000.0f / ECG_BASE_HEART_RATE;
float ecg_heart_rate    = ECG_BASE_HEART_RATE;

// Synthetic ECG waveform (one complete heartbeat, phase 0-1)
float generate_ecg(float phase) {
    float value = 0.0f;
    // P wave
    if      (phase < 0.10f) { float t = phase / 0.10f;         value = 0.15f * sin(t * M_PI); }
    // PR segment
    else if (phase < 0.16f) { value = 0.0f; }
    // Q wave
    else if (phase < 0.18f) { float t = (phase - 0.16f)/0.02f; value = -0.10f * sin(t * M_PI); }
    // R wave
    else if (phase < 0.22f) { float t = (phase - 0.18f)/0.04f; value =  1.20f * sin(t * M_PI); }
    // S wave
    else if (phase < 0.24f) { float t = (phase - 0.22f)/0.02f; value = -0.20f * sin(t * M_PI); }
    // ST segment
    else if (phase < 0.40f) { value = 0.05f; }
    // T wave
    else if (phase < 0.56f) { float t = (phase - 0.40f)/0.16f; value =  0.30f * sin(t * M_PI); }

    value += random(-10, 10) / 1000.0f;   // baseline noise
    return value;
}

void setup_ecg() {
    view.setPlotType(PlotType::Cartesian);
    view.setMode(Mode::Continuous);
    view.setDelay(ECG_SAMPLE_RATE_MS);
    view.setNumberOfPlots(1);
    view.setTitle("ViewSauce");
    view.setPlotTitle("ECG Monitor");
    view.setVerticalRange(-0.5f, 1.5f, 0.1f, 0.5f);
    view.setAxisLabels("Time", "Voltage");
    view.setUnits("s", "mV");
    view.setGridColors(0xFFCCCC, 0xFF6666);  // pink ECG paper
    view.addHorizontalReferenceLine(0.0f);
    view.trace("Lead II").setColor(color(0, 0xAA, 0));

    ecg_phase = 0.0f;
    ecg_heart_rate = ECG_BASE_HEART_RATE;
    ecg_beat_period = 60000.0f / ecg_heart_rate;
}

void loop_ecg() {
    float ecg_value = generate_ecg(ecg_phase);
    view.addData("Lead II", ecg_value);
    view.send();

    ecg_phase += ECG_SAMPLE_RATE_MS / ecg_beat_period;
    if (ecg_phase >= 1.0f) {
        ecg_phase -= 1.0f;
        float variation = random(-100, 100) / 100.0f * ECG_HR_VARIABILITY;
        ecg_heart_rate = ECG_BASE_HEART_RATE + variation;
        ecg_beat_period = 60000.0f / ecg_heart_rate;
    }
}

// ═════════════════════════════════════════════════════════════════════
//  Scene 2: Multi-Sensor Dashboard  (Cartesian / Continuous / 4 plots)
// ═════════════════════════════════════════════════════════════════════

#define DASH_DELAY_MS  50   // 20 Hz

float dash_temperature    = 25.0f;
float dash_temp_target    = 25.0f;
float dash_vib_phase      = 0.0f;
float dash_pressure       = 1013.0f;
float dash_press_target   = 1013.0f;
int   dash_status         = 1;
unsigned long dash_last_status_change = 0;

void setup_dashboard() {
    view.setPlotType(PlotType::Cartesian);
    view.setMode(Mode::Continuous);
    view.setDelay(DASH_DELAY_MS);
    view.setNumberOfPlots(4);
    view.setTitle("ViewSauce");

    view.trace("Temp").setColor(colors::Red);

    // Plot 0: Temperature
    view.plot(0).setTitle("Temperature");
    view.plot(0).setVerticalRange(0, 50, 5, 10);
    view.plot(0).setAxisLabels("Time", "Temp");
    view.plot(0).setUnits("s", "\xB0""C");
    view.plot(0).vertical().addReferenceLine(35.0f, color(33, 255, 102, 0), 1.5f);
    view.plot(0).vertical().addReferenceLine(40.0f, color(33, 255, 0, 0), 2.0f);

    // Plot 1: Vibration
    view.plot(1).setTitle("Vibration");
    view.plot(1).setVerticalRange(-10, 10, 2, 5);
    view.plot(1).setAxisLabels("Time", "Velocity");
    view.plot(1).setUnits("s", "mm/s");
    view.trace("Vib").setColor(colors::Green);

    // Plot 2: Pressure
    view.plot(2).setTitle("Pressure");
    view.plot(2).setVerticalRange(900, 1100, 20, 50);
    view.plot(2).setAxisLabels("Time", "Pressure");
    view.plot(2).setUnits("s", "hPa");
    view.plot(2).vertical().addReferenceLine(1013.25f, color(49, 0, 170, 0), 1.5f);
    view.trace("Press").setColor(colors::Blue);

    // Plot 3: Status
    view.plot(3).setTitle("System Status");
    view.plot(3).setVerticalRange(-0.5f, 1.5f, 1.0f, 0.5f);
    view.plot(3).setAxisLabels("Time", "State");
    view.plot(3).setUnits("s", "");
    view.trace("Status").setColor(colors::Orange);

    // Reset simulation state
    dash_temperature = 25.0f;
    dash_temp_target = 25.0f;
    dash_vib_phase = 0.0f;
    dash_pressure = 1013.0f;
    dash_press_target = 1013.0f;
    dash_status = 1;
    dash_last_status_change = millis();
}

void loop_dashboard() {
    unsigned long now = millis();

    // Temperature: slow drift
    if (random(100) < 2) dash_temp_target = 20.0f + random(20);
    dash_temperature += (dash_temp_target - dash_temperature) * 0.01f;
    dash_temperature += random(-10, 10) / 100.0f;

    // Vibration: sinusoidal + harmonics + noise
    dash_vib_phase += 0.3f;
    float vib = 5.0f * sin(dash_vib_phase) + 2.0f * sin(dash_vib_phase * 3.7f);
    vib += random(-10, 10) / 10.0f;

    // Pressure: step changes
    if (random(100) < 1) dash_press_target = 950.0f + random(100);
    dash_pressure += (dash_press_target - dash_pressure) * 0.05f;
    dash_pressure += random(-5, 5) / 10.0f;

    // Status: toggling
    if (now - dash_last_status_change > 3000u + (unsigned long)random(5000)) {
        dash_status = !dash_status;
        dash_last_status_change = now;
    }

    view.addData("Temp", dash_temperature);
    view.addData("Vib", vib);
    view.addData("Press", dash_pressure);
    view.addData("Status", (float)dash_status);
    view.send();
}

// ═════════════════════════════════════════════════════════════════════
//  Scene 3: Frequency Spectrum  (Cartesian / Frames / DSP)
// ═════════════════════════════════════════════════════════════════════

#define SPEC_SAMPLE_RATE      250000
#define SPEC_FFT_SIZE         1024
#define SPEC_FFT_OUTPUT_SIZE  (SPEC_FFT_SIZE / 2 + 1)
#define SPEC_CARRIER_START    5000
#define SPEC_CARRIER_END      50000
#define SPEC_CARRIER_STEP     500
#define SPEC_NOISE_AMP        3.0f
#define SPEC_DELAY_MS         40

SignalGenerator spec_sig(SPEC_SAMPLE_RATE);
FFT<(FFTSize)SPEC_FFT_SIZE> spec_fft;
float spec_samples[SPEC_FFT_SIZE];
uint32_t spec_carrier_freq = SPEC_CARRIER_START;

void setup_spectrum() {
    view.setPlotType(PlotType::Cartesian);
    view.setMode(Mode::Frames);
    view.setDelay(SPEC_DELAY_MS);
    view.setPacketSize(SPEC_FFT_OUTPUT_SIZE);
    view.setNumberOfPlots(1);
    view.setTitle("ViewSauce");
    view.setPlotTitle("Frequency Spectrum");
    view.setHorizontalRange(0, SPEC_SAMPLE_RATE / 2000, 10);
    view.setVerticalRange(-70, 10, 8);
    view.setAxisLabels("Frequency", "Power");
    view.setUnits("kHz", "dB");

    view.trace("Spectrum").setColor(colors::DodgerBlue);

    // Initialize signal generator
    spec_carrier_freq = SPEC_CARRIER_START;
    spec_sig.setCarrier(SignalShape::Square, spec_carrier_freq);
    spec_sig.setAM(SignalShape::Sine, 2, 0.01);
    spec_sig.setNoise(SPEC_NOISE_AMP);
    spec_fft.setWindowType(WindowType::BlackmanHarris);
}

void loop_spectrum() {
    float signal_amplitude = 1.65f;
    float full_scale = 1.65f;

    // Generate samples
    for (int i = 0; i < SPEC_FFT_SIZE; i++) {
        spec_samples[i] = signal_amplitude * spec_sig.nextSample(0);
    }

    // Compute FFT and scale to dB
    spec_fft.calculate(spec_samples);
    spec_fft.postScaleDB(-120, full_scale);

    // Send spectrum frame
    view.addData("Spectrum", (float*)spec_fft, SPEC_FFT_OUTPUT_SIZE);
    view.send();

    // Sweep carrier
    spec_carrier_freq += SPEC_CARRIER_STEP;
    if (spec_carrier_freq > (uint32_t)SPEC_CARRIER_END)
        spec_carrier_freq = SPEC_CARRIER_START;
    spec_sig.setCarrier(SignalShape::Square, spec_carrier_freq);
}

// ═════════════════════════════════════════════════════════════════════
//  Scene 4: Display Modes  (Cartesian / Frames / 3 plots)
// ═════════════════════════════════════════════════════════════════════

void setup_display_modes() {
    view.setPlotType(PlotType::Cartesian);
    view.setMode(Mode::Frames);
    view.setDelay(SPEC_DELAY_MS);
    view.setPacketSize(SPEC_FFT_OUTPUT_SIZE);
    view.setNumberOfPlots(3);
    view.setTitle("ViewSauce");

    view.trace("Spectrum").setColor(colors::DodgerBlue);

    // Initialize DSP pipeline (reset cleared all ViewPoint state; DSP globals persist
    // but we re-init for self-containment)
    spec_carrier_freq = SPEC_CARRIER_START;
    spec_sig.setCarrier(SignalShape::Square, spec_carrier_freq);
    spec_sig.setAM(SignalShape::Sine, 2, 0.01);
    spec_sig.setNoise(SPEC_NOISE_AMP);
    spec_fft.setWindowType(WindowType::BlackmanHarris);

    view.plot(0).setTitle("Persistence");
    view.plot(0).setVerticalRange(-70, 10, 8);
    view.plot(0).setHorizontalRange(0, SPEC_SAMPLE_RATE / 2000, 10);
    view.plot(0).setAxisLabels("Frequency", "Power");
    view.plot(0).setUnits("kHz", "dB");
    view.plot(0).setDisplayMode(DisplayMode::Persistence);

    view.plot(1).setTitle("Spectrogram");
    view.plot(1).setVerticalRange(-70, 10, 8);
    view.plot(1).setHorizontalRange(0, SPEC_SAMPLE_RATE / 2000, 10);
    view.plot(1).setAxisLabels("Frequency", "Power");
    view.plot(1).setUnits("kHz", "dB");
    view.plot(1).setDisplayMode(DisplayMode::Spectrogram);

    view.plot(2).setTitle("Gradient");
    view.plot(2).setVerticalRange(-70, 10, 8);
    view.plot(2).setHorizontalRange(0, SPEC_SAMPLE_RATE / 2000, 10);
    view.plot(2).setAxisLabels("Frequency", "Power");
    view.plot(2).setUnits("kHz", "dB");
    view.plot(2).setDisplayMode(DisplayMode::Gradient);
}

void loop_display_modes() {
    // Reuse the same DSP pipeline from spectrum scene
    loop_spectrum();
}

#endif // VIEWSAUCE_CARTESIAN_H
