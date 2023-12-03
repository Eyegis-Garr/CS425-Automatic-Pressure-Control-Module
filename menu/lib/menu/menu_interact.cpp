#include "menu.h"

MenuInteractCallback MENU_INTERACT[4] = {
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
    if (!m) return 0;
    vec2 tpos = (vec2) { m->center.x, m->center.y - m->opt_offset - m->opt_div + 80 };
    int cx_offset = CENTER.x - (S_BTN_WIDTH * 2);
    int spacing, centering;

    for (int i = 0; i < 4; i += 1) {
        spacing = S_BTN_SPACING * i;
        centering = cx_offset + (S_BTN_WIDTH * i);
        if (TEST_RECT(t.x, t.y, centering + spacing, tpos.y,                        \
            S_BTN_WIDTH, S_BTN_WIDTH)) {  // positive
            return i + 1;
        } else if (TEST_RECT(t.x, t.y, centering + spacing, tpos.y + (2 * m->opt_div), \
            S_BTN_WIDTH, S_BTN_WIDTH)) {  // negative
            return -1 * (i + 1);
        }
    }

    tpos.y += 3 * m->opt_div;

    // test for cancel
    if (TEST_RECT(t.x, t.y, A_COLUMN.x, tpos.y, COLUMN_W, COLUMN_H)) {
        return 6;
    } else if (TEST_RECT(t.x, t.y, B_COLUMN.x, tpos.y, COLUMN_W, COLUMN_H)) {
        return 7;
    }

    return 0;
}

int m_interact(menu_t *m, TSPoint p) {
    if (!m) return -1;

    return MENU_INTERACT[m->it_flag](m, p);
}

int m_interact_default(menu_t *m, TSPoint p) {
  int idx = m_test_touch(p, m);
  if (idx >= 0 && p.z > 70) {
	return idx;
  }

  return -1;
}

int m_interact_set(menu_t *m, TSPoint p) {
    int vbuf[4] = {
        m->options[0].value / 1000 % 10,
        m->options[0].value / 100  % 10,
        m->options[0].value / 10   % 10,
        m->options[0].value % 10
    };
    int idx = m_test_touch_set(p, m);

    if (idx < 0) {  // decrement
        vbuf[abs(idx) - 1] = mod(vbuf[abs(idx) - 1] - 1, 10);
        m->options[0].value = 1000 * vbuf[0] + 100 * vbuf[1] + 10 * vbuf[2] + vbuf[3];
        return S_NEW_VALUE;
    } if (idx > 0 && idx < 6) {    // increment
        vbuf[idx - 1] = mod(vbuf[idx - 1] + 1, 10);
        m->options[0].value = 1000 * vbuf[0] + 100 * vbuf[1] + 10 * vbuf[2] + vbuf[3];
        return S_NEW_VALUE;
    } if (idx == 6) {  // cancel
        m->cb(m, &m->options[idx]);
        return idx;
    } if (idx == 7) {  // confirm
        // format value
        m->options[0].value = 1000 * vbuf[0] + 100 * vbuf[1] + 10 * vbuf[2] + vbuf[3];
        m->cb(m, &m->options[idx]);
        return idx;
    }

    return S_NO_CHANGE;
}



