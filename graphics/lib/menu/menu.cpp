#include "menu.h"

menu_t *new_menu(const char *title, uint8_t num_options, font_t *font, vec2 center,
                element_t *menu_element, element_t *option_element) {
    menu_t *ret = (menu_t*) malloc(sizeof(menu_t));

    if (title) {
        ret->title = (char*) malloc(sizeof(char) * TITLE_LEN);
        strncpy(ret->title, title, TITLE_LEN);
    }
    
    ret->nopts = num_options;
    ret->options = NULL;
    ret->center = center;
    ret->w = 0;
    ret->h = 0;
    ret->cursor = 0;
    ret->cur_color = 0x1234;

    ret->m_element = menu_element;
    ret->o_element = option_element;

    ret->font = font;
    if (font) ret->opt_sp = font->spacing * font->scale + 3;
    else ret->opt_sp = 10;

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
        m->options[i].name = (char*) malloc(sizeof(char) * OPTION_LEN);
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

    m->opt_offset = (m->h / 2) - (m->opt_sp / 2);
    m->opt_div = (m->h - m->opt_sp) / (MAX_VIS_OPT + 1) + 3;
    m->vis_items = fmin((m->h / m->opt_div) - 1, m->nopts);

    return 0;
}

int m_interact(menu_t *m, int it) {
    static int prev_cursor;
    if (!m) return -1;

    prev_cursor = m->cursor;

    switch (it) {
        case CURS_UP:
            m->cursor = mod(m->cursor - 1, m->nopts);
            if (prev_cursor < m->cursor) {  // page to bottom
                m_draw_options(m, 1);
                m->coffset = m->cursor - (m->cursor % MAX_VIS_OPT);
            } else if (prev_cursor % MAX_VIS_OPT == 0) {  // page to previous
                m_draw_options(m, 1);
                m->coffset -= MAX_VIS_OPT;                
            }
            break;
        case CURS_DOWN:
            m->cursor = mod(m->cursor + 1, m->nopts);
            if (m->cursor < prev_cursor) {  // page to top
                m_draw_options(m, 1);
                m->coffset = 0;
            } else if (m->cursor % MAX_VIS_OPT == 0) {  // page to next
                m_draw_options(m, 1);
                m->coffset += MAX_VIS_OPT;
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

    m_draw_options(m, 0);

    return 0;
}

int m_draw_options(menu_t *m, int clear) {
    if (!m) return -1;

    vec2 tpos = (vec2) { m->center.x, m->center.y + m->opt_offset - m->opt_div };
    int idx, bound;
    uint16_t c;

    m->o_element->pos = tpos;
    bound = min(m->nopts - m->coffset, m->vis_items);
    for (int i = 0; i < bound; i += 1) {
        idx = i + m->coffset;

        if (clear) c = 0;
        else if (idx == m->cursor) c = m->cur_color;
        else c = m->font->color;

        m->o_element->pos.y = tpos.y;
        e_bake_matrix(m->o_element);
        if (clear) e_clear(m->o_element);
        else e_draw(m->o_element);

        f_draw(m->font, m->options[idx].name, strlen(m->options[idx].name), tpos, c);

        tpos.y -= m->opt_div;
    }

    if (m->nopts - m->coffset > m->vis_items) {
        f_draw(m->font, "---", 3, (vec2){ m->center.x, tpos.y + (m->opt_sp / 2) + 5 }, c);
    }

    return 0;
}

int m_draw(menu_t *m, int clear) {
    if (!m) return -1;

    uint16_t fc = (clear) ? 0 : m->font->color;

    // render menu frame
    m->m_element->pos = m->center;
    e_bake_matrix(m->m_element);
    if (clear) e_clear(m->m_element);
    else e_draw(m->m_element);

    // render title
    f_draw(m->font, m->title, strlen(m->title), 
            (vec2){ m->center.x, m->center.y + m->opt_offset }, fc);

    m_draw_options(m, clear);

    return 0;
}


