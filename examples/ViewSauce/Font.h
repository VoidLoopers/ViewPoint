#ifndef VIEWSAUCE_FONT_H
#define VIEWSAUCE_FONT_H

/*
 * Vector font — each character is a set of polyline strokes on a 5-wide by 7-tall
 * grid. Vertices are {x, y} pairs. {-1,-1} separates strokes within a character.
 * {-2,-2} marks the end of the character definition.
 *
 * Adapted from AnimatedTextBanner.ino
 */

struct Pt { int8_t x, y; };

// clang-format off
const Pt CHAR_V[] = { {0,7},{2,0},{4,7}, {-2,-2} };
const Pt CHAR_O[] = { {1,0},{3,0},{4,1},{4,6},{3,7},{1,7},{0,6},{0,1},{1,0}, {-2,-2} };
const Pt CHAR_I[] = { {1,7},{3,7}, {-1,-1}, {2,7},{2,0}, {-1,-1}, {1,0},{3,0}, {-2,-2} };
const Pt CHAR_D[] = { {0,0},{0,7},{2,7},{4,5},{4,2},{2,0},{0,0}, {-2,-2} };
const Pt CHAR_L[] = { {0,7},{0,0},{4,0}, {-2,-2} };
const Pt CHAR_LOWER_L[] = { {2,7},{2,0}, {-1,-1}, {1,0},{3,0}, {-2,-2} };
const Pt CHAR_P[] = { {0,0},{0,7},{3,7},{4,6},{4,4},{3,3},{0,3}, {-2,-2} };
const Pt CHAR_E[] = { {4,7},{0,7},{0,0},{4,0}, {-1,-1}, {0,3},{3,3}, {-2,-2} };
const Pt CHAR_W[] = { {0,7},{1,0},{2,4},{3,0},{4,7}, {-2,-2} };
const Pt CHAR_N[] = { {0,0},{0,7},{4,0},{4,7}, {-2,-2} };
const Pt CHAR_T[] = { {0,7},{4,7}, {-1,-1}, {2,7},{2,0}, {-2,-2} };
const Pt CHAR_SPACE[] = { {-2,-2} };
// clang-format on

struct CharDef { char ch; const Pt* pts; uint8_t width; };

const CharDef font[] = {
    {'V', CHAR_V, 5}, {'o', CHAR_O, 5}, {'i', CHAR_I, 4}, {'d', CHAR_D, 5},
    {'L', CHAR_L, 5}, {'l', CHAR_LOWER_L, 4}, {'p', CHAR_P, 5},
    {'e', CHAR_E, 5}, {'w', CHAR_W, 5}, {'P', CHAR_P, 5},
    {'n', CHAR_N, 5}, {'t', CHAR_T, 5}, {' ', CHAR_SPACE, 3},
};
const int FONT_SIZE = sizeof(font) / sizeof(font[0]);

const Pt* find_char(char c) {
    for (int i = 0; i < FONT_SIZE; i++)
        if (font[i].ch == c) return font[i].pts;
    return CHAR_SPACE;
}

uint8_t char_width(char c) {
    for (int i = 0; i < FONT_SIZE; i++)
        if (font[i].ch == c) return font[i].width;
    return 3;
}

float string_width(const char* str) {
    float w = 0;
    for (int i = 0; str[i]; i++)
        w += char_width(str[i]) + 1;
    return w > 0 ? w - 1 : 0;
}

#endif // VIEWSAUCE_FONT_H
