#ifndef FONT_H
#define FONT_H

#include <Arduino.h>
#include "matrix.h"
#include "ili9341.h"

// FONT GLYPH ORDERING
// [space]!"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJLMNOPQRSTUVWXYZ[\]^_`
//                             32 - 96 (ASCII)

#define FOFFSET 32
#define FNGLYPH 64

typedef struct {
    const uint8_t **g;

    uint8_t spacing;
    uint8_t scale;
    uint16_t color;
} font_t;

int f_setup(font_t *f, const uint8_t *glyphs[], uint16_t color, uint8_t spacing, uint16_t scale);
int f_draw(font_t *f, const char *str, int len, vec2 pos, uint16_t color);

uint8_t f_getc_idx(char c);

#endif // FONT_H