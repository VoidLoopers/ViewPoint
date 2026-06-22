/******************************************************************************
 * @file Colors.h
 * @brief Color constants for the ViewPoint library
 *
 * Part of the ViewPoint Arduino library for high-speed serial plotting.
 * Colors are stored as 24-bit RGB values (0xRRGGBB) or 32-bit ARGB values (0xAARRGGBB)
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

#ifndef _VIEWPOINT_COLORS_H_
#define _VIEWPOINT_COLORS_H_

#include <stdint.h>

namespace viewpoint {

// Default trace color palette
constexpr uint32_t DEFAULT_PALETTE[] = {
    0xFFFF00,  // Yellow
    0x00FFFF,  // Cyan
    0xFF00FF,  // Magenta
    0x00FF00,  // Green
    0xFF5500,  // Orange
    0x5555FF,  // Light Blue
    0xFF5555,  // Light Red
    0x55FF55,  // Light Green
};

constexpr size_t DEFAULT_PALETTE_SIZE = sizeof(DEFAULT_PALETTE) / sizeof(DEFAULT_PALETTE[0]);

inline uint32_t defaultTraceColor(int index) {
    return DEFAULT_PALETTE[index % DEFAULT_PALETTE_SIZE];
}

constexpr uint32_t color(uint8_t r, uint8_t g, uint8_t b) {
    return (0xFFu << 24) |
           (uint32_t(r) << 16) |
           (uint32_t(g) << 8)  |
           (uint32_t(b));
}
constexpr uint32_t color(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return (uint32_t(a) << 24) |
           (uint32_t(r) << 16) |
           (uint32_t(g) << 8)  |
           uint32_t(b);
}
constexpr uint32_t RGB(uint32_t rrggbb)  { return 0xFF000000u | (rrggbb & 0x00FFFFFFu); }
constexpr uint32_t ARGB(uint32_t aarrggbb) { return aarrggbb; }


// Named colors - Pinks
namespace colors {

constexpr uint32_t Pink             = 0xFFC0CB;
constexpr uint32_t LightPink        = 0xFFB6C1;
constexpr uint32_t HotPink          = 0xFF69B4;
constexpr uint32_t DeepPink         = 0xFF1493;
constexpr uint32_t MediumVioletRed  = 0xC71585;
constexpr uint32_t PaleVioletRed    = 0xDB7093;


// Named colors - Reds
constexpr uint32_t LightSalmon   = 0xFFA07A;
constexpr uint32_t Salmon        = 0xFA8072;
constexpr uint32_t DarkSalmon    = 0xE9967A;
constexpr uint32_t LightCoral    = 0xF08080;
constexpr uint32_t IndianRed     = 0xCD5C5C;
constexpr uint32_t Crimson       = 0xDC143C;
constexpr uint32_t FireBrick     = 0xB22222;
constexpr uint32_t Red           = 0xFF0000;
constexpr uint32_t DarkRed       = 0x8B0000;


// Named colors - Oranges
constexpr uint32_t Coral         = 0xFF7F50;
constexpr uint32_t Tomato        = 0xFF6347;
constexpr uint32_t OrangeRed     = 0xFF4500;
constexpr uint32_t Gold          = 0xFFD700;
constexpr uint32_t Orange        = 0xFFA500;
constexpr uint32_t DarkOrange    = 0xFF8C00;


// Named colors - Yellows
constexpr uint32_t LightYellow           = 0xFFFFE0;
constexpr uint32_t LemonChiffon          = 0xFFFACD;
constexpr uint32_t LightGoldenrodYellow  = 0xFAFAD2;
constexpr uint32_t PapayaWhip            = 0xFFEFD5;
constexpr uint32_t Moccasin              = 0xFFE4B5;
constexpr uint32_t PeachPuff             = 0xFFDAB9;
constexpr uint32_t PaleGoldenrod         = 0xEEE8AA;
constexpr uint32_t Khaki                 = 0xF0E68C;
constexpr uint32_t DarkKhaki             = 0xBDB76B;
constexpr uint32_t Yellow                = 0xFFFF00;
constexpr uint32_t YellowOrange          = 0xFFB343;


// Named colors - Greens
constexpr uint32_t LawnGreen         = 0x7CFC00;
constexpr uint32_t Chartreuse        = 0x7FFF00;
constexpr uint32_t LimeGreen         = 0x32CD32;
constexpr uint32_t Lime              = 0x00FF00;
constexpr uint32_t ForestGreen       = 0x228B22;
constexpr uint32_t Green             = 0x008000;
constexpr uint32_t DarkGreen         = 0x006400;
constexpr uint32_t GreenYellow       = 0xADFF2F;
constexpr uint32_t YellowGreen       = 0x9ACD32;
constexpr uint32_t SpringGreen       = 0x00FF7F;
constexpr uint32_t MediumSpringGreen = 0x00FA9A;
constexpr uint32_t LightGreen        = 0x90EE90;
constexpr uint32_t PaleGreen         = 0x98FB98;
constexpr uint32_t DarkSeaGreen      = 0x8FBC8F;
constexpr uint32_t MediumSeaGreen    = 0x3CB371;
constexpr uint32_t SeaGreen          = 0x2E8B57;
constexpr uint32_t Olive             = 0x808000;
constexpr uint32_t DarkOliveGreen    = 0x556B2F;
constexpr uint32_t OliveDrab         = 0x6B8E23;
constexpr uint32_t PhosphorGreen     = 0x60C030;


// Named colors - Cyans
constexpr uint32_t LightCyan        = 0xE0FFFF;
constexpr uint32_t Cyan             = 0x00FFFF;
constexpr uint32_t Aqua             = 0x00FFFF;
constexpr uint32_t Aquamarine       = 0x7FFFD4;
constexpr uint32_t MediumAquamarine = 0x66CDAA;
constexpr uint32_t PaleTurquoise    = 0xAFEEEE;
constexpr uint32_t Turquoise        = 0x40E0D0;
constexpr uint32_t MediumTurquoise  = 0x48D1CC;
constexpr uint32_t DarkTurquoise    = 0x00CED1;
constexpr uint32_t LightSeaGreen    = 0x20B2AA;
constexpr uint32_t CadetBlue        = 0x5F9EA0;
constexpr uint32_t DarkCyan         = 0x008B8B;
constexpr uint32_t Teal             = 0x008080;


// Named colors - Blues
constexpr uint32_t PowderBlue       = 0xB0E0E6;
constexpr uint32_t LightBlue        = 0xADD8E6;
constexpr uint32_t LightSkyBlue     = 0x87CEFA;
constexpr uint32_t SkyBlue          = 0x87CEEB;
constexpr uint32_t DeepSkyBlue      = 0x00BFFF;
constexpr uint32_t LightSteelBlue   = 0xB0C4DE;
constexpr uint32_t DodgerBlue       = 0x1E90FF;
constexpr uint32_t CornflowerBlue   = 0x6495ED;
constexpr uint32_t SteelBlue        = 0x4682B4;
constexpr uint32_t RoyalBlue        = 0x4169E1;
constexpr uint32_t Blue             = 0x0000FF;
constexpr uint32_t MediumBlue       = 0x0000CD;
constexpr uint32_t DarkBlue         = 0x00008B;
constexpr uint32_t Navy             = 0x000080;
constexpr uint32_t MidnightBlue     = 0x191970;
constexpr uint32_t MediumSlateBlue  = 0x7B68EE;
constexpr uint32_t SlateBlue        = 0x6A5ACD;
constexpr uint32_t DarkSlateBlue    = 0x483D8B;


// Named colors - Purples
constexpr uint32_t Lavender      = 0xE6E6FA;
constexpr uint32_t Thistle       = 0xD8BFD8;
constexpr uint32_t Plum          = 0xDDA0DD;
constexpr uint32_t Violet        = 0xEE82EE;
constexpr uint32_t Orchid        = 0xDA70D6;
constexpr uint32_t Fuchsia       = 0xFF00FF;
constexpr uint32_t Magenta       = 0xFF00FF;
constexpr uint32_t MediumOrchid  = 0xBA55D3;
constexpr uint32_t MediumPurple  = 0x9370DB;
constexpr uint32_t BlueViolet    = 0x8A2BE2;
constexpr uint32_t DarkViolet    = 0x9400D3;
constexpr uint32_t DarkOrchid    = 0x9932CC;
constexpr uint32_t DarkMagenta   = 0x8B008B;
constexpr uint32_t Purple        = 0x800080;
constexpr uint32_t Indigo        = 0x4B0082;


// Named colors - Browns
constexpr uint32_t Cornsilk         = 0xFFF8DC;
constexpr uint32_t BlanchedAlmond   = 0xFFEBCD;
constexpr uint32_t Bisque           = 0xFFE4C4;
constexpr uint32_t NavajoWhite      = 0xFFDEAD;
constexpr uint32_t Wheat            = 0xF5DEB3;
constexpr uint32_t BurlyWood        = 0xDEB887;
constexpr uint32_t Tan              = 0xD2B48C;
constexpr uint32_t RosyBrown        = 0xBC8F8F;
constexpr uint32_t SandyBrown       = 0xF4A460;
constexpr uint32_t Goldenrod         = 0xDAA520;
constexpr uint32_t DarkGoldenrod     = 0xB8860B;
constexpr uint32_t Peru             = 0xCD853F;
constexpr uint32_t Chocolate        = 0xD2691E;
constexpr uint32_t SaddleBrown      = 0x8B4513;
constexpr uint32_t Sienna           = 0xA0522D;
constexpr uint32_t Brown            = 0xA52A2A;
constexpr uint32_t Maroon           = 0x800000;


// Named colors - Grayscale
constexpr uint32_t White      = 0xFFFFFF;
constexpr uint32_t Snow       = 0xFFFAFA;
constexpr uint32_t Ivory      = 0xFFFFF0;
constexpr uint32_t WhiteSmoke = 0xF5F5F5;
constexpr uint32_t Gainsboro  = 0xDCDCDC;
constexpr uint32_t LightGray  = 0xD3D3D3;
constexpr uint32_t Silver     = 0xC0C0C0;
constexpr uint32_t DarkGray   = 0xA9A9A9;
constexpr uint32_t Gray       = 0x808080;
constexpr uint32_t DimGray    = 0x696969;
constexpr uint32_t SlateGray  = 0x708090;
constexpr uint32_t DarkSlateGray = 0x2F4F4F;
constexpr uint32_t Black      = 0x000000;


// Grid colors (dark theme defaults)
constexpr uint32_t GridBackground = 0x1E1E1E;
constexpr uint32_t GridMinor      = 0x333333;
constexpr uint32_t GridMajor      = 0x666666;
constexpr uint32_t GridLabels     = 0x969696;

} // namespace colors

} // namespace viewpoint

#endif // _VIEWPOINT_COLORS_H_
