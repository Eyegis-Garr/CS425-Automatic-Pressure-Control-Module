#include "menu.h"

MenuDraw MENU_DRAW[4] = {
    m_list_options,
    m_set_value,
    m_toggle_value,
    m_print_value
};

// draw routine entry point
int m_draw(Adafruit_ILI9341 *disp, menu_t *m, int clear) {
    if (!m) return -1;

    uint16_t c = (clear) ? 0 : 0xFFFF;

    // render menu frame
    disp->drawRect(0, 0, m->w, m->h, c);

    // render title
    disp->setCursor(20, 20);
    disp->setTextColor(c);
    disp->setTextSize(3);
    disp->print(m->title);
    disp->setTextSize(1);

    MENU_DRAW[m->m_type](disp, m, clear);

    return 0;
}

// draws options as a list/grid
int m_list_options(Adafruit_ILI9341 *disp, menu_t *m, int clear) {
    if (!m) return -1;

    vec2 tpos = (vec2) { m->center.x, m->center.y - m->opt_offset - m->opt_div + 80 };
    uint16_t c = (clear) ? 0 : 0xFFFF;

    disp->setTextSize(2);
    disp->setTextColor(c);
    for (int i = 0; i < m->nopts - 1; i += 2) {
        if (clear) c = 0;
        else c = 0xFFFF;

        // draw A frame
        disp->drawRect(A_COLUMN.x, tpos.y, COLUMN_W, COLUMN_H, (!clear && i == m->cursor) ? m->cur_color : c);
        // draw option
        disp->setCursor(A_COLUMN.x + 5, tpos.y + 5);
        disp->print(m->options[i].name);

        // draw B frame
        disp->drawRect(B_COLUMN.x + 10, tpos.y, COLUMN_W, COLUMN_H, (!clear && i + 1 == m->cursor) ? m->cur_color : c);
        // draw option
        disp->setCursor(B_COLUMN.x + 10 + 5, tpos.y + 5);
        disp->print(m->options[i + 1].name);

        tpos.y += m->opt_div;
    }

    // BACK
    disp->drawRect(A_BTN.x, A_BTN.y, COLUMN_W, COLUMN_H, c);
    disp->setCursor(A_BTN.x + 5, A_BTN.y + 5);
    disp->print("BACK");

    // EXIT
    disp->drawRect(B_BTN.x + 10, tpos.y + 10, COLUMN_W, COLUMN_H, c);
    disp->setCursor(B_BTN.x + 10 + 5, B_BTN.y + 5);
    disp->print("EXIT");

    return 0;
}

// draws menu for changing values
int m_set_value(Adafruit_ILI9341 *disp, menu_t *m, int clear) {
    if (!m) return -1;

    vec2 tpos = (vec2) { m->center.x, m->center.y - m->opt_offset - m->opt_div + 80 };
    uint16_t c = (clear) ? 0 : 0xFFFF;
    int q, r, s, t;
    int cx_offset = CENTER.x - (S_BTN_WIDTH * 2);
    int spacing, centering;

    disp->setTextSize(3);
    disp->setTextColor(c);

    // extract value digits
    q = m->options[0].value / 1000 % 10;
    r = m->options[0].value / 100  % 10;
    s = m->options[0].value / 10   % 10;
    t = m->options[0].value % 10;
    sprintf(m->options[0].name, "%d%d%d%d", q, r, s, t);

    // iterate for 4 digits
    for (int i = 0; i < 4; i += 1) {
        // compute spacing and centering
        spacing = S_BTN_SPACING * i;
        centering = cx_offset + (S_BTN_WIDTH * i);

        // draws positive boxes and '+'
        disp->drawRect(centering + spacing , tpos.y, S_BTN_WIDTH, S_BTN_WIDTH, c);
        disp->setCursor(centering + spacing + 10, tpos.y + 10);
        disp->print("+");

        if (clear) {
            disp->fillRect(centering + spacing + 15, tpos.y + m->opt_div + 10, S_BTN_WIDTH, S_BTN_WIDTH, c);
        } else {
            disp->setCursor(centering + spacing + 15, tpos.y + m->opt_div + 10);
            disp->print(m->options[0].name[i]);
        }

        // draws negative boxes and '-'
        disp->drawRect(centering + spacing, tpos.y + (2 * m->opt_div), S_BTN_WIDTH, S_BTN_WIDTH, c);
        disp->setCursor(centering + spacing + 10, tpos.y + (2 * m->opt_div) + 10);
        disp->print("-");
    }

    // moves to bottom of menu
    tpos.y += 3 * m->opt_div;

    disp->setTextSize(2);

    // cancel
    disp->drawRect(CENTER.x - COLUMN_W, tpos.y + 10, COLUMN_W, COLUMN_H, c);
    disp->setCursor(CENTER.x - COLUMN_W + 5, tpos.y + 10 + 5);
    // disp->print(m->options[m->nopts - 2].name);
    disp->print("CANCEL");

    // confirm
    disp->drawRect(CENTER.x + 10, tpos.y + 10, COLUMN_W, COLUMN_H, c);
    disp->setCursor(CENTER.x + 10 + 5, tpos.y + 10 + 5);
    // disp->print(m->options[m->nopts - 1].name);
    disp->print("CONFIRM");

    disp->setTextSize(1);

    return 0;
}

// draws menu for toggling values
int m_toggle_value(Adafruit_ILI9341 *disp, menu_t *m, int clear) {
    if (!m) return -1;



    return 0;
}

// draws menu for printing option values
int m_print_value(Adafruit_ILI9341 *disp, menu_t *m, int clear) {
    if (!m) return -1;



    return 0;
}
