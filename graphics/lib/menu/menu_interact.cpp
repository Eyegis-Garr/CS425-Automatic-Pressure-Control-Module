#include "menu.h"

int m_interact_default(menu_t *m, int it) {
    static int prev_cursor;
    if (!m) return -1;

    prev_cursor = m->cursor;

    switch (it) {
        case CURS_UP:
            m->cursor = mod(m->cursor - 1, m->nopts);
            if (prev_cursor < m->cursor) {  // page to bottom
                m_list_options(m, 1);
                m->coffset = m->cursor - (m->cursor % MAX_VIS_OPT);
                m_list_options(m, 0);
            } else if (prev_cursor % MAX_VIS_OPT == 0) {  // page to previous
                m_list_options(m, 1);
                m->coffset -= MAX_VIS_OPT;
                m_list_options(m, 0);
            }
            break;
        case CURS_DOWN:
            m->cursor = mod(m->cursor + 1, m->nopts);
            if (m->cursor < prev_cursor) {  // page to top
                m_list_options(m, 1);
                m->coffset = 0;
                m_list_options(m, 0);
            } else if (m->cursor % MAX_VIS_OPT == 0) {  // page to next
                m_list_options(m, 1);
                m->coffset += MAX_VIS_OPT;
                m_list_options(m, 0);
            }
            break;
        case CURS_LEFT:
            // move back level
            break;
        case CURS_RIGHT:
            // move to option target
            break;
        case CURS_SELECT:
            break;
        default:
            break;
    }

    return 0;
}



