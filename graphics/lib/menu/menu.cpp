#include "menu.h"

menu_t *new_menu(const char *title, uint8_t num_options, const char *options[], font_t *font, uint8_t flags) {
    menu_t *ret = (menu_t*) malloc(sizeof(menu_t));

    if (title) 
        ret->title = (char*) malloc(sizeof(char) * TITLE_LEN);
    strncpy(ret->title, title, TITLE_LEN);
    ret->nopts = num_options;
    ret->opts = options;
    ret->center = ORIGIN;
    ret->w = 0;
    ret->h = 0;
    ret->opt_sp = 0;
    ret->cursor = 0;
    ret->cur_color = 0x1234;
    ret->font = font;

    ret->cb = NULL;

    return ret;
}

int m_set_aabb(menu_t *m, vec2 tl, vec2 br) {
    if (!m) return -1;

    m->w = br.x - tl.x;
    m->h = tl.y - br.y;
    m->center.x = tl.x + (m->w / 2);
    m->center.y = br.y + (m->h / 2);

    if (m->font)
        m->opt_sp = m->font->spacing * m->font->scale + 3;

    return 0;
}

int m_interact(menu_t *m, int it) {
    if (!m) return -1;

    switch (it) {
        case CURS_UP:
            m->cursor = mod(m->cursor - 1, m->nopts);
            m_opt_redraw(m, mod(m->cursor + 1, m->nopts));
            break;
        case CURS_DOWN:
            m->cursor = mod(m->cursor + 1, m->nopts);
            m_opt_redraw(m, mod(m->cursor - 1, m->nopts));
            break;
        case CURS_LEFT:
            // move up level
            break;
        case CURS_RIGHT:
            // same as select?
            break;
        case CURS_SELECT:
            m->cb(m->cursor);
            break;
        default:
            break;
    }

    return 0;
}

int m_opt_redraw(menu_t *m, int opt) {
    if (!m) return -1;

    uint16_t fcolor = m->font->color;
    int div = (m->h - m->opt_sp) / (m->nopts + 1);
    int ymax = m->center.y + (m->h / 2) - m->opt_sp;
    vec2 pos = (vec2) { m->center.x, ymax };

    m->font->color = m->cur_color;
    pos.y = ymax - ((m->cursor + 1) * div);
    f_draw(m->font, m->opts[m->cursor], strlen(m->opts[m->cursor]), pos);
    m->font->color = fcolor;
    pos.y = ymax - ((opt + 1) * div);
    f_draw(m->font, m->opts[opt], strlen(m->opts[opt]), pos);

    return 0;
}

int m_draw(menu_t *m) {
    if (!m) return -1;

    uint16_t c;
    int div = (m->h - m->opt_sp) / (m->nopts + 1);
    int ymax = m->center.y + (m->h / 2) - m->opt_sp;
    vec2 fpos = (vec2) { m->center.x, ymax };

    // render menu frame
    e_bake_matrix(&m->m_element);
    e_draw(&m->m_element);

    // render title
    f_draw(m->font, m->title, strlen(m->title), fpos);

    for (int i = 0; i < m->nopts + 1; i += 1) {
        m->o_element.pos.y = ymax - (i * div);
        e_bake_matrix(&m->o_element);
        e_draw(&m->o_element);

        fpos.y -= div;
        if (i == m->cursor) {
            c = m->font->color;
            m->font->color = m->cur_color;
            f_draw(m->font, m->opts[i], strlen(m->opts[i]), fpos);
            m->font->color = c;

        } else {
            f_draw(m->font, m->opts[i], strlen(m->opts[i]), fpos);
        }
    }

    return 0;
}
