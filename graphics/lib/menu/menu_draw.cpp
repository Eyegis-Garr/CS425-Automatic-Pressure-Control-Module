#include "menu.h"

MenuDrawCallback MENU_DRAW[4] = {
    m_list_options,
    m_set_value,
    m_toggle_value,
    m_print_value
};

int m_draw(menu_t *m, int clear) {
    if (!m) return -1;

    uint16_t fc = (clear) ? 0 : m->font->color;

    // render menu frame
    m->m_element->pos = m->center;
    e_bake_matrix(m->m_element);
    e_draw(m->m_element, clear);

    // render title
    f_draw(m->font, m->title, strlen(m->title), 
            (vec2){ m->center.x, m->center.y + m->opt_offset }, fc);

    MENU_DRAW[m->draw_flag](m, clear);

    return 0;
}

int m_list_options(menu_t *m, int clear) {
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
        e_draw(m->o_element, clear);

        f_draw(m->font, m->options[idx].name, strlen(m->options[idx].name), tpos, c);

        tpos.y -= m->opt_div;
    }

    if (m->nopts - m->coffset > m->vis_items) {
        f_draw(m->font, "---", 3, (vec2){ m->center.x, tpos.y + (m->opt_sp / 2) + 5 }, c);
    }

    return 0;
}

int m_set_value(menu_t *m, int clear) {
    if (!m) return -1;

    vec2 tpos = (vec2) { m->center.x, m->center.y + m->opt_offset - m->opt_div };
    uint16_t c = (clear) ? 0 : m->font->color;
    int q, r, s, t, x;

    f_draw(m->font, "+", 1, tpos, c);
    tpos.y -= m->opt_div;

    m->o_element->pos = tpos;
    e_bake_matrix(m->o_element);
    e_draw(m->o_element, clear);

    if (m->options) {
        x = tpos.x - m->font->spacing;
        q = m->options[0].value / 1000 % 10;
        r = m->options[0].value / 100  % 10;
        s = m->options[0].value / 10   % 10;
        t = m->options[0].value % 10;
        sprintf(m->options[0].name, "%d%d%d%d", q, r, s, t);
        f_draw(m->font, m->options[0].name, 4, tpos, c);
        f_draw(m->font, ".", 1, (vec2){tpos.x, tpos.y - 10}, c);
        f_draw(m->font, "^", 1, (vec2){x + m->cursor * m->font->spacing, tpos.y - m->font->spacing}, c);
    } else {
        f_draw(m->font, "NULL", 4, tpos, c);
    }

    tpos.y -= m->opt_div;

    f_draw(m->font, "-", 1, tpos, c);
    tpos.y -= m->opt_div;

    m->o_element->pos = tpos;
    e_bake_matrix(m->o_element);
    e_draw(m->o_element, clear);
    f_draw(m->font, "OK", 2, tpos, (m->cursor < 0) ? 0x05A0 : c);

    return 0;
}

int m_toggle_value(menu_t *m, int clear) {
    if (!m) return -1;



    return 0;
}
int m_print_value(menu_t *m, int clear) {
    if (!m) return -1;



    return 0;
}
