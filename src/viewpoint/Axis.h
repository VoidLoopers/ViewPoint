/******************************************************************************
 * @file Axis.h
 * @brief Axis configuration for ViewPoint plots
 *
 *   This file is a part of the VoidLoop ViewPoint project, which is one of the 
 *   projects brought to the education and hobbyist community by VoidLoop.    
 *   Please see www.voidloop.com for updates.
 *
 * Contributors:
 * Initial vision, feature definition, debugging and funding: Greg Kovacs
 * Architecture, Development, debugging and support: Zachariah Magee
 * 
 * Thanks to our many collaborators and friends!
 * 
 * MIT License (MIT)
 * 
 * Copyright (c) 2026 VoidLoop
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
******************************************************************************/


#ifndef _VIEWPOINT_AXIS_H_
#define _VIEWPOINT_AXIS_H_

#include "Types.h"
#include "Colors.h"
#include <string.h>

namespace viewpoint {

enum ScalingStrategy {
    None,               // Fixed min/max (manual)
    ExpandOnly,         // expand to new extrema, never shrink unless reset
    Windowed,           // (default auto) compute bounds from last N samples or last T seconds
    RobustPercentile,   // percentile bounds (ignore spikes/ outliers)
    PeakDecay,          // peak-hold with decay back toward typical range
    Debounced,          // update bounds only every N ms or only if threshold exceeded
    SymmetricZero,      // symmetric around configurable center (default 0)
    BaselineZero,       // lock one side to configurable baseline (default 0), scale the other
};

class Axis {
public:

    Axis() {
        clear();
    }

    explicit Axis(AxisId id) : id_(id) {
        clear();
    }

    /**
     * @breif Set the scaling strategy for auto-scaling
     */
    void setScalingStrategy(ScalingStrategy strategy) {
        strategy_ = strategy;
    }

    // Range configuration
    /**
     * @brief Set the axis range (min and max values)
     */
    void setRange(float min, float max) {
        range_.minimum = min;
        range_.maximum = max;
        range_.set = true;
        strategy_ = ScalingStrategy::None;
    }

    /**
     * @brief Set the axis range with a specific number of divisions
     */
    void setRange(float min, float max, int divisions) {
        setRange(min, max);
        setDivisions(divisions);
    }

    /**
     * @brief Set the axis range with minor and major step values
     */
    void setRange(float min, float max, float minorStep, float majorStep) {
        setRange(min, max);
        setStep(minorStep, majorStep);
    }

    /**
     * @brief Set the number of grid divisions
     */
    void setDivisions(int divisions) {
        divisions_ = divisions;
    }

    /**
     * @brief Set the minor and major grid line step values
     */
    void setStep(float minor, float major) {
        step_.minor = minor;
        step_.major = major;
        step_.set = true;
    }


    // Labels and units
    /**
     * @brief Set the axis label (e.g., "Frequency", "Time")
     */
    void setLabel(const char* label) {
        if (label) {
            strncpy(label_, label, VIEWPOINT_LABEL_SIZE - 1);
            label_[VIEWPOINT_LABEL_SIZE - 1] = '\0';
        } else {
            label_[0] = '\0';
        }
    }

    /**
     * @brief Set the axis units (e.g., "Hz", "ms", "dB")
     */
    void setUnits(const char* units) {
        if (units) {
            strncpy(units_, units, VIEWPOINT_UNITS_SIZE - 1);
            units_[VIEWPOINT_UNITS_SIZE - 1] = '\0';
        } else {
            units_[0] = '\0';
        }
    }


    // Logarithmic scale
    /**
     * @brief Enable logarithmic scale
     * @param mapData If true (default), app maps linear data to log scale.
     *               If false, data is already in log-space (e.g., dB).
     */
    void enableLogarithmic(bool mapData = true) {
        logarithmic_ = true;
        mapData_ = mapData;
    }

    /**
     * @brief Disable logarithmic scale
     */
    void disableLogarithmic() {
        logarithmic_ = false;
        mapData_ = true;
    }


    // Reference lines (custom grid lines)
    /**
     * @brief Add a reference line at a specific value
     * @param value The value where the line should appear
     * @param isMajor If true, style as major grid line; otherwise minor
     * @return Reference-line index, or -1 if at capacity
     */
    int addReferenceLine(float value, bool isMajor = false) {
        if (refLineCount_ >= VIEWPOINT_MAX_REFERENCE_LINES) {
            return -1;
        }
        int index = refLineCount_;
        refLines_[index] = ReferenceLine(value, isMajor);
        refLineCount_++;
        return index;
    }

    /**
     * @brief Add a reference line with custom color and stroke
     * @param value The value where the line should appear
     * @param color Line color (0xRRGGBB)
     * @param strokeWeight Line thickness
     * @return Reference-line index, or -1 if at capacity
     */
    int addReferenceLine(float value, uint32_t color, float strokeWeight = 1.0f) {
        if (refLineCount_ >= VIEWPOINT_MAX_REFERENCE_LINES) {
            return -1;
        }
        int index = refLineCount_;
        refLines_[index] = ReferenceLine(value, color, strokeWeight);
        refLineCount_++;
        return index;
    }

    /**
     * @brief Update an existing reference line
     * @param index Reference-line index returned by addReferenceLine()
     * @param value The new value where the line should appear
     * @param color Optional line color. Pass NO_COLOR to keep the current color.
     * @param strokeWeight Optional line thickness. Pass a negative value to keep the current stroke.
     * @return true if updated, false if the index is invalid
     */
    bool updateReferenceLine(int index, float value, uint32_t color = NO_COLOR, float strokeWeight = -1.0f) {
        if (index < 0 || index >= refLineCount_) {
            return false;
        }

        refLines_[index].value = value;
        if (hasColor(color)) {
            refLines_[index].color = color;
        }
        if (strokeWeight >= 0.0f) {
            refLines_[index].strokeWeight = strokeWeight;
        }
        refLines_[index].set = true;
        return true;
    }

    /**
     * @brief Remove all reference lines
     */
    void clearReferenceLines() {
        for (int i = 0; i < VIEWPOINT_MAX_REFERENCE_LINES; i++) {
            refLines_[i] = ReferenceLine();
        }
        refLineCount_ = 0;
    }


    // Clear / Reset
    /**
     * @brief Reset axis to default state
     */
    void clear() {
        range_ = Range();
        step_ = Step();
        divisions_ = UNDEFINED_INT;
        logarithmic_ = false;
        mapData_ = true;
        label_[0] = '\0';
        units_[0] = '\0';
        clearReferenceLines();
    }


    // Accessors

    AxisId id() const { return id_; }

    bool isSet() const {
        return range_.set || step_.set || isDefined(divisions_) ||
               logarithmic_ || label_[0] != '\0' || units_[0] != '\0' ||
               refLineCount_ > 0;
    }

    bool hasRange() const { return range_.set; }
    const Range& range() const { return range_; }
    float minimum() const { return range_.minimum; }
    float maximum() const { return range_.maximum; }

    bool hasStep() const { return step_.set; }
    const Step& step() const { return step_; }
    float minorStep() const { return step_.minor; }
    float majorStep() const { return step_.major; }

    bool hasDivisions() const { return isDefined(divisions_); }
    int divisions() const { return divisions_; }

    bool logarithmic() const { return logarithmic_; }
    bool mapData() const { return mapData_; }

    bool hasLabel() const { return label_[0] != '\0'; }
    const char* label() const { return label_; }

    bool hasUnits() const { return units_[0] != '\0'; }
    const char* units() const { return units_; }

    int referenceLineCount() const { return refLineCount_; }
    const ReferenceLine& referenceLine(int index) const {
        if (index >= 0 && index < refLineCount_) {
            return refLines_[index];
        }
        static ReferenceLine empty;
        return empty;
    }


    // Character identifier for commands


    char axisChar() const {
        return (id_ == AxisId::X) ? 'x' : 'y';
    }

private:
    AxisId id_ = AxisId::Y;
    Range range_;
    Step step_;
    ScalingStrategy strategy_ = ScalingStrategy::Windowed;
    int divisions_ = UNDEFINED_INT;
    bool logarithmic_ = false;
    bool mapData_ = true;
    char label_[VIEWPOINT_LABEL_SIZE];
    char units_[VIEWPOINT_UNITS_SIZE];
    ReferenceLine refLines_[VIEWPOINT_MAX_REFERENCE_LINES];
    int refLineCount_ = 0;
};

} // namespace viewpoint

#endif // _VIEWPOINT_AXIS_H_
