#include "menu.h"

MenuInteractCallback MENU_INTERACT[4] = {
    m_interact_default,
    m_interact_set,
    NULL,
    NULL
};

int m_interact(menu_t *m, int it) {
    if (!m) return -1;

    return MENU_INTERACT[m->it_flag](m, it);
}

int m_interact_default(menu_t *m, int it) {
    static int prev_cursor;
    if (!m) return -1;

    prev_cursor = m->cursor;

    switch (it) {
        case CURS_UP:
            m->cursor = mod(m->cursor - 1, m->nopts);
            // if (prev_cursor < m->cursor) {  // page to bottom
            //     m_list_options(m, 1);
            //     m->coffset = m->cursor - (m->cursor % MAX_VIS_OPT);
            //     m_list_options(m, 0);
            // } else if (prev_cursor % MAX_VIS_OPT == 0) {  // page to previous
            //     m_list_options(m, 1);
            //     m->coffset -= MAX_VIS_OPT;
            //     m_list_options(m, 0);
            // }
            break;
        case CURS_DOWN:
            m->cursor = mod(m->cursor + 1, m->nopts);
            // if (m->cursor < prev_cursor) {  // page to top
            //     m_list_options(m, 1);
            //     m->coffset = 0;
            //     m_list_options(m, 0);
            // } else if (m->cursor % MAX_VIS_OPT == 0) {  // page to next
            //     m_list_options(m, 1);
            //     m->coffset += MAX_VIS_OPT;
            //     m_list_options(m, 0);
            // }
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
            return 1;
    }

    return 0;
}

int m_interact_set(menu_t *m, int it) {
    static int prev_cursor;
    if (!m) return -1;

    prev_cursor = m->cursor;

    switch (it) {
        case CURS_UP:
            if (m->cursor < 0) m->cursor = 0;
            m->options[0].value += pow(10, m->cursor);
            break;
        case CURS_DOWN:
            if (m->cursor < 0) m->cursor = 0;
            m->options[0].value -= pow(10, m->cursor);
            break;
        case CURS_LEFT:
            if (m->cursor < 0) m->cursor = 0;
            m->cursor = mod(m->cursor + 1, 4);
            break;
        case CURS_RIGHT:
            if (m->cursor < 0) m->cursor = 0;
            m->cursor = mod(m->cursor - 1, 4);
            break;
        case CURS_SELECT:
            m->cursor = -1;
            break;
        default:
            return 1;
    }

    return 0;
}



