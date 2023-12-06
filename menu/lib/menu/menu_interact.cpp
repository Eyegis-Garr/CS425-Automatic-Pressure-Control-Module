#include "menu.h"

MenuInteract MENU_INTERACT[NUM_MTYPES] = {
    m_interact_default,
    m_interact_set,
    NULL,
    NULL,
    NULL,
    m_interact_msg
};

int m_test_touch(TSPoint t, menu_t *m) {
    if (!m) return M_NOP;
    vec2 tpos = (vec2) { m->center.x, m->center.y - m->opt_offset - m->opt_div + 80 };

    for (int i = 0; i < m->nopts - 1; i += 2) {
        if (TEST_RECT(t.x, t.y, A_COLUMN.x, tpos.y, COLUMN_W, COLUMN_H)) {        // CURRENT A OPTION
            m->cursor = i;
            return M_SELECT;
        } else if (i + 1 < m->nopts && TEST_RECT(t.x, t.y, B_COLUMN.x, tpos.y, COLUMN_W, COLUMN_H)) { // CURRENT B OPTION
            m->cursor = i + 1;
            return M_SELECT;
        }

        tpos.y += m->opt_div;
    }

    if (TEST_RECT(t.x, t.y, A_BTN.x, A_BTN.y, COLUMN_W, COLUMN_H) && ~m->flags & M_NOBACK) {   
        return M_BACK;
    } else if (TEST_RECT(t.x, t.y, B_BTN.x, B_BTN.y, COLUMN_W, COLUMN_H) && ~m->flags & M_NOEXIT) {
        return M_EXIT;
    }

    return M_NOP;
}

int m_test_touch_set(TSPoint t, menu_t *m) {
    if (!m) return M_NOP;
    vec2 tpos = (vec2) { m->center.x, m->center.y - m->opt_offset - m->opt_div + 80 };
    int cx_offset = CENTER.x - (S_BTN_WIDTH * 2);
    int spacing, centering;

    for (int i = 0; i < 4; i += 1) {
        spacing = S_BTN_SPACING * i;
        centering = cx_offset + (S_BTN_WIDTH * i);
        if (TEST_RECT(t.x, t.y, centering + spacing, tpos.y, S_BTN_WIDTH, S_BTN_WIDTH)) {  // positive
            m->cursor = i + 1;
            return M_UPDATED;
        } else if (TEST_RECT(t.x, t.y, centering + spacing, tpos.y + (2 * m->opt_div), S_BTN_WIDTH, S_BTN_WIDTH)) {  // negative
            m->cursor = -1 * (i + 1);
            return M_UPDATED;
        }
    }

    tpos.y += 3 * m->opt_div;

    if (TEST_RECT(t.x, t.y, A_COLUMN.x, tpos.y, COLUMN_W, COLUMN_H)) {          // cancel button
        return M_BACK;
    } else if (TEST_RECT(t.x, t.y, B_COLUMN.x, tpos.y, COLUMN_W, COLUMN_H)) {   // confirm button
        return M_CONFIRM;
    }

    return M_NOP;
}

int m_test_touch_msg(TSPoint t, menu_t *m) {
    if (!m) return M_NOP;

    if (TEST_RECT(t.x, t.y, A_BTN.x, A_BTN.y - 100, COLUMN_W, COLUMN_H)) {
        return M_CONFIRM;
    } else if (TEST_RECT(t.x, t.y, B_BTN.x, B_BTN.y - 100, COLUMN_W, COLUMN_H)) {
        return M_BACK;
    }

    return M_NOP;
}

int m_interact(menu_t *m, TSPoint p) {
    if (!m) return M_NOP;

    return MENU_INTERACT[m->m_type](m, p);
}

int m_interact(menu_t *m, int command) {
    if (!m) return M_NOP;


    return M_NOP;
}

int m_interact_msg(menu_t *m, TSPoint p) {
    return m_test_touch_msg(p, m);
}

int m_interact_default(menu_t *m, TSPoint p) {
  int code = m_test_touch(p, m);
  if (code == M_SELECT) {
    if (m->cb) m->cb(m, &m->options[m->cursor]);
  }

  return code;
}

int m_interact_set(menu_t *m, TSPoint p) {
    int code = m_test_touch_set(p, m), d;
    int vbuf[4] = {
        m->options[0].value / 1000 % 10,
        m->options[0].value / 100  % 10,
        m->options[0].value / 10   % 10,
        m->options[0].value % 10
    };

    if (code == M_UPDATED) {
        d = (m->cursor > 0) ? 1 : -1;
        vbuf[abs(m->cursor) - 1] = mod(vbuf[abs(m->cursor) - 1] + d, 10);
        m->options[0].value = 1000 * vbuf[0] + 100 * vbuf[1] + 10 * vbuf[2] + vbuf[3];
        return M_UPDATED;
    } if (code == M_BACK) {     // cancel
        return M_BACK;
    } if (code == M_CONFIRM) {  // confirm
        if (m->cb) m->cb(m, &m->options[code]);
        return M_CONFIRM;
    }

    return M_NOP;
}



