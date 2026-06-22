/******************************************************************************
 * @file ViewPoint.h
 * @brief Main include file for the ViewPoint Arduino library
 *
 * ViewPoint is an Arduino library for high-speed serial plotting to the
 * ViewPoint desktop application.
 *
 * Basic Usage:
 *   #include <ViewPoint.h>
 *
 *   void setup() {
 *       view.begin();
 *       view.setVerticalRange(-10, 10);
 *   }
 *
 *   void loop() {
 *       view.addData("Analog", analogRead(A0));
 *       view.send();
 *   }
 *
 * Multiple Traces:
 *   void setup() {
 *       view.begin();
 *   }
 *
 *   void loop() {
 *       view.addData("Temperature", readTemp());
 *       view.addData("Humidity", readHumidity());
 *       view.send();
 *   }
 *
 * Frames Mode (FFT, etc.):
 *   float fftData[512];
 *
 *   void setup() {
 *       view.begin(frames, 512);
 *       view.setHorizontalRange(0, 1000, 10);  // 0-1000 Hz
 *       view.setVerticalRange(-90, 0, 9);       // -90 to 0 dB
 *       view.setUnits("Hz", "dB");
 *   }
 *
 *   void loop() {
 *       computeFFT(fftData);
 *       view.addData("FFT", fftData, 512);
 *       view.send();
 *   }
 *
 * Protocol Negotiation:
 *   When the ViewPoint desktop app connects, it sends its protocol version.
 *   On receipt the library responds with its version and begins emitting
 *   configuration commands. Until the app sends its version, the library
 *   only emits data lines — so non-ViewPoint receivers (like the Arduino
 *   IDE Serial Plotter) still see a clean CSV stream.
 *
 *   // Check protocol state:
 *   if (view.isNegotiated()) {
 *       // Full protocol available
 *   }
 *
 *   // Access protocol for version checks:
 *   if (view.protocol().supports(1, 2)) {
 *       // App supports V1R2+ features
 *   }
 *
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

#ifndef _VIEWPOINT_H_
#define _VIEWPOINT_H_

// Foundation includes
#include "viewpoint/Types.h"
#include "viewpoint/Colors.h"
#include "viewpoint/Axis.h"

// Core class includes
#include "viewpoint/Trace.h"
#include "viewpoint/Plot.h"
#include "viewpoint/Protocol.h"
#include "viewpoint/Command.h"
#include "viewpoint/Plotter.h"

/**
 * These defines are simply to avoid having to type `viewpoint::Mode::Frames` 
 * and instead allow typing `frames` only. If this causes namespace issues in
 * your sketch write `#define USE_VIEWPOINT_WITH_NAMESPACE` above the viewpoint 
 * include:
 * 
 * ```cpp
 * #define USE_VIEWPOINT_WITH_NAMESPACE
 * #include "ViewPoint.h"
 * ```
 */
#ifndef USE_VIEWPOINT_WITH_NAMESPACE

#define frames      viewpoint::Mode::Frames
#define continuous  viewpoint::Mode::Continuous

#define cartesian   viewpoint::PlotType::Cartesian
#define scatter     viewpoint::PlotType::Scatter
#define xy          viewpoint::PlotType::Scatter
#define polar       viewpoint::PlotType::Polar

#define persistence viewpoint::DisplayMode::Persistence
#define spectrogram viewpoint::DisplayMode::Spectrogram
#define gradient    viewpoint::DisplayMode::Gradient

#endif

// Global instance — uses inline function to avoid multiple-definition
// errors while keeping the library fully header-only. This ensures
// user #define settings (e.g. VIEWPOINT_TX_BUF_SIZE) are visible
// to all library code.
namespace viewpoint { namespace detail {
    inline Plotter& globalInstance() {
        static Plotter instance;
        return instance;
    }
}}
static viewpoint::Plotter& view = viewpoint::detail::globalInstance();

#endif // _VIEWPOINT_H_
