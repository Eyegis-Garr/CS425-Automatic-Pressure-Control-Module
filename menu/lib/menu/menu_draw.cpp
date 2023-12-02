#include "menu.h"

MenuDrawCallback MENU_DRAW[4] = {
    m_list_options,
    m_set_value,
    m_toggle_value,
    m_print_value
};

int m_draw(Adafruit_ILI9341 *disp, menu_t *m, int clear) {
    if (!m) return -1;

    uint16_t fc = (clear) ? 0 : 0xFFFF;

    // render menu frame
    disp->drawRect(m->center.x - 160, m->center.y - 120, m->w, m->h, 0xFFFF);

    // render title
    int x, y, x1, y1, w, h;
    disp->setCursor(20, 20);
    disp->setTextSize(3);
    disp->print(m->title);
    disp->setTextSize(1);

    MENU_DRAW[m->draw_flag](disp, m, clear);

    return 0;
}

int m_list_options(Adafruit_ILI9341 *disp, menu_t *m, int clear) {
    if (!m) return -1;

    vec2 tpos = (vec2) { m->center.x, m->center.y - m->opt_offset - m->opt_div + 80 };
    int idx, bound;
    vec2 a = A_COLUMN, b = B_COLUMN;
    uint16_t c;

    disp->setTextSize(2);
    for (int i = 0; i < m->nopts - 1; i += 2) {
        idx = i + m->coffset;

        if (clear) c = 0;
        else if (idx == m->cursor) c = m->cur_color;
        else c = 0xFFFF;

        // draw A frame
        disp->drawRect(a.x, tpos.y, COLUMN_W, COLUMN_H, c);
        // draw option
        disp->setCursor(a.x + 5, tpos.y + 5);
        disp->print(m->options[i].name);

        // draw B frame
        disp->drawRect(b.x + 10, tpos.y, COLUMN_W, COLUMN_H, c);
        // draw option
        disp->setCursor(b.x + 10 + 5, tpos.y + 5);
        disp->print(m->options[i + 1].name);

        tpos.y += m->opt_div;
    }

    return 0;
}

int m_set_value(Adafruit_ILI9341 *disp, menu_t *m, int clear) {
    if (!m) return -1;

    

    return 0;
}

int m_toggle_value(Adafruit_ILI9341 *disp, menu_t *m, int clear) {
    if (!m) return -1;



    return 0;
}
int m_print_value(Adafruit_ILI9341 *disp, menu_t *m, int clear) {
    if (!m) return -1;



    return 0;
}
