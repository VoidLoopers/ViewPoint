/******************************************************************************
 * @file Plot.h
 * @brief Plot configuration for ViewPoint
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


#ifndef _VIEWPOINT_PLOT_H_
#define _VIEWPOINT_PLOT_H_

#include "Types.h"
#include "Colors.h"
#include "Axis.h"
#include <string.h>

namespace viewpoint {

class Plot {
public:
    
    Plot() : id_(-1) {
        clear();
    }

    explicit Plot(int id) : id_(id) {
        clear();
    }

    
    // Axis access
    Axis& horizontal() { return horizontal_; }
    const Axis& horizontal() const { return horizontal_; }

    Axis& vertical() { return vertical_; }
    const Axis& vertical() const { return vertical_; }

    
    /**
     * @brief Set vertical axis range
     */
    void setVerticalRange(float min, float max) {
        vertical_.setRange(min, max);
        dirty_ = true;
    }

    void setVerticalRange(float min, float max, int divisions) {
        vertical_.setRange(min, max, divisions);
        dirty_ = true;
    }

    void setVerticalRange(float min, float max, float minor, float major) {
        vertical_.setRange(min, max, minor, major);
        dirty_ = true;
    }

    /**
     * @brief Set horizontal axis range
     */
    void setHorizontalRange(float min, float max) {
        horizontal_.setRange(min, max);
        dirty_ = true;
    }

    void setHorizontalRange(float min, float max, int divisions) {
        horizontal_.setRange(min, max, divisions);
        dirty_ = true;
    }

    void setHorizontalRange(float min, float max, float minor, float major) {
        horizontal_.setRange(min, max, minor, major);
        dirty_ = true;
    }

    /**
     * @brief Set axis units
     */
    void setUnits(const char* horizontalUnits, const char* verticalUnits) {
        horizontal_.setUnits(horizontalUnits);
        vertical_.setUnits(verticalUnits);
        dirty_ = true;
    }

    /**
     * @brief Set axis labels
     */
    void setAxisLabels(const char* horizontalLabel, const char* verticalLabel) {
        horizontal_.setLabel(horizontalLabel);
        vertical_.setLabel(verticalLabel);
        dirty_ = true;
    }

    
    // Plot configuration
    

    /**
     * @brief Set the plot title
     */
    void setTitle(const char* title) {
        if (title) {
            strncpy(title_, title, VIEWPOINT_LABEL_SIZE - 1);
            title_[VIEWPOINT_LABEL_SIZE - 1] = '\0';
        } else {
            title_[0] = '\0';
        }
        dirty_ = true;
    }

    /**
     * @brief Enable logarithmic scale on vertical axis
     * @param mapData If true (default), app maps linear data to log scale.
     *               If false, data is already in log-space (e.g., dB).
     */
    void enableLogarithmicScale(bool mapData = true) {
        vertical_.enableLogarithmic(mapData);
        dirty_ = true;
    }

    /**
     * @brief Disable logarithmic scale on vertical axis
     */
    void disableLogarithmicScale() {
        vertical_.disableLogarithmic();
        dirty_ = true;
    }

    /**
     * @brief Enable logarithmic scale on specified axis
     * @param mapData If true (default), app maps linear data to log scale.
     *               If false, data is already in log-space (e.g., dB).
     */
    void enableLogarithmicScale(AxisId axis, bool mapData = true) {
        if (axis == AxisId::X) {
            horizontal_.enableLogarithmic(mapData);
        } else {
            vertical_.enableLogarithmic(mapData);
        }
        dirty_ = true;
    }

    /**
     * @brief Disable logarithmic scale on specified axis
     */
    void disableLogarithmicScale(AxisId axis) {
        if (axis == AxisId::X) {
            horizontal_.disableLogarithmic();
        } else {
            vertical_.disableLogarithmic();
        }
        dirty_ = true;
    }

    
    // Grid colors
    /**
     * @brief Set minor and major grid line colors
     */
    void setGridColors(uint32_t minor, uint32_t major) {
        gridColors_.minor = minor;
        gridColors_.major = major;
        gridColors_.set = true;
        dirty_ = true;
    }

    /**
     * @brief Set label, minor, and major grid colors
     */
    void setGridColors(uint32_t labels, uint32_t minor, uint32_t major) {
        gridColors_.labels = labels;
        gridColors_.minor = minor;
        gridColors_.major = major;
        gridColors_.set = true;
        dirty_ = true;
    }

    /**
     * @brief Set grid line stroke weight
     */
    void setGridStroke(float weight) {
        gridColors_.strokeWeight = weight;
        gridColors_.set = true;
        dirty_ = true;
    }

    
    // Reference lines


    /**
     * @brief Add a *horizontal* line to the *vertical axis*.
     *
     * The line is drawn flat across the plot (constant y), so it lives on
     * the vertical axis. The argument is the y-value where the line should
     * appear in your plot's coordinate space.
     *
     * @param value   y-value where the line crosses the plot
     * @param isMajor If true, render with the major-grid style; minor otherwise
     * @return Reference-line index, or -1 if the axis is at capacity
     */
    int addHorizontalReferenceLine(float value, bool isMajor = false) {
        dirty_ = true;
        return vertical_.addReferenceLine(value, isMajor);
    }

    int addHorizontalReferenceLine(float value, uint32_t color, float stroke = 1.0f) {
        dirty_ = true;
        return vertical_.addReferenceLine(value, color, stroke);
    }

    bool updateHorizontalReferenceLine(int index, float value, uint32_t color = NO_COLOR, float stroke = -1.0f) {
        bool updated = vertical_.updateReferenceLine(index, value, color, stroke);
        if (updated) dirty_ = true;
        return updated;
    }

    /**
     * @brief Add a *vertical* line to the *horizontal axis*.
     *
     * The line is drawn straight up and down (constant x), so it lives on
     * the horizontal axis. The argument is the x-value where the line should
     * appear in your plot's coordinate space.
     *
     * @param value   x-value where the line crosses the plot
     * @param isMajor If true, render with the major-grid style; minor otherwise
     * @return Reference-line index, or -1 if the axis is at capacity
     */
    int addVerticalReferenceLine(float value, bool isMajor = false) {
        dirty_ = true;
        return horizontal_.addReferenceLine(value, isMajor);
    }

    int addVerticalReferenceLine(float value, uint32_t color, float stroke = 1.0f) {
        dirty_ = true;
        return horizontal_.addReferenceLine(value, color, stroke);
    }

    bool updateVerticalReferenceLine(int index, float value, uint32_t color = NO_COLOR, float stroke = -1.0f) {
        bool updated = horizontal_.updateReferenceLine(index, value, color, stroke);
        if (updated) dirty_ = true;
        return updated;
    }

    
    // Display modes
    /**
     * @brief Set display mode for this plot
     */
    void setDisplayMode(DisplayMode mode) {
        displayMode_.mode = mode;
        displayMode_.set = true;
        dirty_ = true;
    }

    /**
     * @brief Set display mode with specific trace
     */
    void setDisplayMode(DisplayMode mode, int traceId) {
        displayMode_.mode = mode;
        displayMode_.traceId = traceId;
        displayMode_.set = true;
        dirty_ = true;
    }

    /**
     * @brief Set display mode with colors (for spectrogram)
     */
    void setDisplayMode(DisplayMode mode, int traceId, const uint32_t* colors, int colorCount) {
        displayMode_.mode = mode;
        displayMode_.traceId = traceId;
        int count = (colorCount > 8) ? 8 : colorCount;
        for (int i = 0; i < count; i++) {
            displayMode_.colors[i] = colors[i];
        }
        displayMode_.colorCount = count;
        displayMode_.set = true;
        dirty_ = true;
    }

    
    // Trace association
    

    /**
     * @brief Add a trace ID to this plot
     * @return true if added, false if at capacity
     */
    bool addTrace(int traceId) {
        if (traceCount_ >= VIEWPOINT_MAX_TRACES_PER_PLOT) {
            return false;
        }
        // Check if already added
        for (int i = 0; i < traceCount_; i++) {
            if (traceIds_[i] == traceId) return true;
        }
        traceIds_[traceCount_++] = traceId;
        dirty_ = true;
        return true;
    }

    /**
     * @brief Add multiple trace IDs
     */
    void addTraces(const int* ids, int count) {
        for (int i = 0; i < count; i++) {
            if (!addTrace(ids[i])) break;
        }
    }

    /**
     * @brief Clear all associated traces
     */
    void clearTraces() {
        for (int i = 0; i < VIEWPOINT_MAX_TRACES_PER_PLOT; i++) {
            traceIds_[i] = -1;
        }
        traceCount_ = 0;
        dirty_ = true;
    }


    // Clear / Reset
    void clear() {
        title_[0] = '\0';
        horizontal_ = Axis(AxisId::X);
        vertical_ = Axis(AxisId::Y);
        gridColors_ = GridColors();
        displayMode_ = DisplayModeConfig();
        clearTraces();
        dirty_ = true;
    }

    
    // Accessors 
    int id() const { return id_; }
    void setId(int id) { id_ = id; }

    bool hasTitle() const { return title_[0] != '\0'; }
    const char* title() const { return title_; }

    bool hasGridColors() const { return gridColors_.set; }
    const GridColors& gridColors() const { return gridColors_; }

    bool hasDisplayMode() const { return displayMode_.set; }
    const DisplayModeConfig& displayMode() const { return displayMode_; }

    int traceCount() const { return traceCount_; }
    int traceId(int index) const {
        if (index >= 0 && index < traceCount_) {
            return traceIds_[index];
        }
        return -1;
    }
    const int* traceIds() const { return traceIds_; }

    /**
     * @brief Check if plot has any configuration set
     */
    bool isConfigured() const {
        return hasTitle() ||
               horizontal_.isSet() ||
               vertical_.isSet() ||
               hasGridColors() ||
               hasDisplayMode() ||
               traceCount_ > 0;
    }

    bool isDirty() const { return dirty_; }
    void clearDirty() { dirty_ = false; }

private:
    int id_;
    char title_[VIEWPOINT_LABEL_SIZE];

    Axis horizontal_;
    Axis vertical_;

    GridColors gridColors_;
    DisplayModeConfig displayMode_;

    int traceIds_[VIEWPOINT_MAX_TRACES_PER_PLOT];
    int traceCount_ = 0;
    bool dirty_ = false;
};

} // namespace viewpoint

#endif // _VIEWPOINT_PLOT_H_
