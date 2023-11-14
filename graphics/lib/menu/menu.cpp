#include "menu.h"

menu_t *new_menu(uint8_t num_options, char **options, font_t *font, uint8_t flags) {
    menu_t *ret = (menu_t*) malloc(sizeof(menu_t));

    ret->nopts = num_options;
    ret->opts = options;
    ret->cursor = 0;
    ret->font = font;
    ret->flags = flags;

    ret->m_element = NULL;
    ret->o_element = NULL;
    ret->font = NULL;
    ret->cb = NULL;

    ret->flags |= 1 << INIT;

    return ret;
}

int m_set_aabb(menu_t *m, vec2 tl, vec2 br) {
    if (!m) return -1;
    if (!(m->flags & (1 << INIT))) return -1;

    m->w = br.x - tl.x;
    m->h = tl.y - br.y;
    m->opt_sp = m->font->spacing * m->font->scale;
    m->topleft.x = tl.x;
    m->topleft.y = tl.y;

    m->m_element->pos.x = m->topleft.x + (m->w >> 1);
    m->m_element->pos.y = m->topleft.y + (m->h >> 1);

    return 0;
}

int m_draw(menu_t *m) {
    if (!m) return -1;

    int vitems = m->h / m->opt_sp;
    m->m_element->pos.x = m->topleft.x + (m->w >> 1);
    m->m_element->pos.y = m->topleft.y + (m->h >> 1);
    m->o_element->pos.x = m->topleft.x + (m->w >> 1);
    e_bake_matrix(m->m_element);
    e_draw(m->m_element);
    for (int i = 0; i < vitems; i += 1) {
        m->o_element->pos.y = m->topleft.y + (i * m->opt_sp);
        if (m->o_element->pos.y < m->topleft.y + m->h) {
            e_bake_matrix(m->o_element);
            e_draw(m->o_element);
        }
    }

    return 0;
}
