#include "font.h"

int f_setup(font_t *f, const uint8_t *glyphs[], uint8_t pt, uint16_t scale) {
    if (!f) {
        return -1;
    }

    f->g = glyphs;
    f->scale = scale;

    f->spacing = pt;
    f->color = 0;

    return 0;
}

int f_draw(font_t *f, char *str, int len, vec2 pos) {
    if (!f || !str || len <= 0) {
        return -1;
    }

    const uint8_t *glyph;
    uint8_t rd, r, c, idx;

    
    begin_write();
        for (int l = 0; l < len; l += 1) {
            if (str[l] != ' ') {
                idx = f_getc_idx(str[l]);
                glyph = f->g[idx];
                c = pgm_read_byte(glyph++);
                r = pgm_read_byte(glyph++);
                for (int i = 0; i < r * f->scale; i += 1) {
                    for (int k = 0; k < c * f->scale; k += 1) {
                        int z = (i / f->scale) * c + (k / f->scale);
                        rd = pgm_read_byte(glyph + z);
                        if (rd) set_pixel(pos.x + k, pos.y + i, f->color);
                    }
                }
            }

            pos.x += f->spacing * f->scale;
        }
    end_write();

    return 0;
}

uint8_t f_getc_idx(char c) {
    uint8_t idx = 0;

    if ((c >= 32 && c <= 96) || islower(c)) 
        idx = islower(c) ? c - (FOFFSET << 1) - 1 : c - FOFFSET - 1;

    return idx;
}

