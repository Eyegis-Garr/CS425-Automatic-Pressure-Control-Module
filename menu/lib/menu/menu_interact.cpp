#include "menu.h"

MenuInteract MENU_INTERACT[4] = {
    m_interact_default,
    m_interact_set,
    NULL,
    NULL
};

int m_test_touch(TSPoint t, menu_t *m) {
    if (!m) return -1;
    vec2 tpos = (vec2) { m->center.x, m->center.y - m->opt_offset - m->opt_div + 80 };

    for (int i = 0; i < m->nopts - 1; i += 2) {
        if (TEST_RECT(t.x, t.y, A_COLUMN.x, tpos.y, COLUMN_W, COLUMN_H)) {        // CURRENT A OPTION
            return i;
        } else if (TEST_RECT(t.x, t.y, B_COLUMN.x, tpos.y, COLUMN_W, COLUMN_H)) { // CURRENT B OPTION
            
            return i + 1;
        }

        tpos.y += m->opt_div;
    }

    return -1;
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

int m_interact(menu_t *m, TSPoint p) {
    if (!m) return M_NOP;

    return MENU_INTERACT[m->m_type](m, p);
}

int m_interact(menu_t *m, int command) {
    if (!m) return M_NOP;

    
}

int m_interact_default(menu_t *m, TSPoint p) {
  int idx = m_test_touch(p, m);
  if (idx >= 0 && p.z > 70) {
    m->cursor = idx;
    if (m->cb) m->cb(m, &m->options[m->cursor]);
	return M_SELECT;
  }

  return M_NOP;
}

int m_interact_set(menu_t *m, TSPoint p) {
    int idx = m_test_touch_set(p, m), d;
    int vbuf[4] = {
        m->options[0].value / 1000 % 10,
        m->options[0].value / 100  % 10,
        m->options[0].value / 10   % 10,
        m->options[0].value % 10
    };

    if (idx == M_UPDATED) {
        d = (m->cursor > 0) ? 1 : -1;
        vbuf[abs(m->cursor) - 1] = mod(vbuf[abs(m->cursor) - 1] + d, 10);
        m->options[0].value = 1000 * vbuf[0] + 100 * vbuf[1] + 10 * vbuf[2] + vbuf[3];
        return M_UPDATED;
    } if (idx == M_BACK) {  // cancel
        return M_BACK;
    } if (idx == M_CONFIRM) {  // confirm
        if (m->cb) m->cb(m, &m->options[idx]);
        return M_CONFIRM;
    }

    return M_NOP;
}



