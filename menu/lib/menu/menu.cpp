#include "menu.h"

menu_t *new_menu(const char *title, uint8_t num_options, vec2 center, vec2 size, int type) {
    menu_t *ret = (menu_t*) malloc(sizeof(menu_t));

    if (title) {
        strncpy(ret->title, title, TITLE_LEN);
    }
    
    ret->nopts = num_options;
    ret->options = NULL;
    ret->center = center;
    ret->w = size.x;
    ret->h = size.y;

    ret->opt_offset = (ret->h - ret->opt_sp) / 2;
    ret->opt_div = (ret->h - ret->opt_sp) / 6;
    ret->vis_items = fmin((ret->h / ret->opt_div) - 1, ret->nopts);

    ret->cursor = 0;
    ret->cur_color = 0x1234;

    ret->opt_sp = 10;

    ret->coffset = 0;

    ret->cb = NULL;
    ret->m_type = (in_range(0, 3, type)) ? type : M_DEFAULT;

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
