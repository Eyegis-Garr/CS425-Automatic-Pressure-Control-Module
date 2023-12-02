#include "menu.h"

menu_t *new_menu(const char *title, uint8_t num_options, vec2 center) {
    menu_t *ret = (menu_t*) malloc(sizeof(menu_t));

    if (title) {
        strncpy(ret->title, title, TITLE_LEN);
    }
    
    ret->nopts = num_options;
    ret->options = NULL;
    ret->center = center;
    ret->w = 0;
    ret->h = 0;
    ret->cursor = 0;
    ret->cur_color = 0x1234;

    ret->opt_sp = 10;

    ret->coffset = 0;

    ret->cb = NULL;

    return ret;
}

int m_set_options(menu_t *m, int num_options, option_t *options) {
    if (!m || !options) return -1;

    if (!m->options) {
        m->options = (option_t*) malloc(sizeof(option_t) * num_options);
    } else {
        option_t *s = (option_t*) realloc(m->options, num_options);
        if (!s) return -1;
        m->options = s;
    }

    for (int i = 0; i < num_options; i += 1) {
        strncpy(m->options[i].name, options[i].name, OPTION_LEN);
        m->options[i].target = options[i].target;
        m->options[i].value = options[i].value;
    }

    return 0;
}

int m_set_size(menu_t *m, int width, int height) {
    if (!m) return -1;

    m->w = width;
    m->h = height;

    m->opt_offset = (m->h - m->opt_sp) / 2;
    m->opt_div = (m->h - m->opt_sp) / 6;
    m->vis_items = fmin((m->h / m->opt_div) - 1, m->nopts);

    return 0;
}

int m_set_draw(menu_t *m, int draw_flag) {
    if (!m || draw_flag < 0 || draw_flag >= 4) return -1;

    m->draw_flag = draw_flag;

    return 0;
}

int m_set_interact(menu_t *m, int it_flag) {
    if (!m || it_flag < 0 || it_flag >= 4) return -1;

    m->it_flag = it_flag;

    return 0;
}

int m_test_touch(TSPoint t, menu_t *m) {
    if (!m) return -1;
    vec2 a = A_COLUMN, b = B_COLUMN;
    vec2 tpos = (vec2) { m->center.x, m->center.y - m->opt_offset - m->opt_div + 80 };

    for (int i = 0; i < m->nopts - 1; i += 2) {
        if (TEST_POINT(t.x, t.y, a.x, tpos.y, COLUMN_W, COLUMN_H)) {   // CURRENT A OPTION
            return i;
        } else if (TEST_POINT(t.x, t.y, b.x, tpos.y, COLUMN_W, COLUMN_H)) {    // CURRENT B OPTION
            return i + 1;
        }

        tpos.y += m->opt_div;
    }

    return -1;
}


