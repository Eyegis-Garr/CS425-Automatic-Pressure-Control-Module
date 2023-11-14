#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include "element.h"
#include "font.h"

#define OPTION_LEN  32

// FLAGS
#define INIT        0
#define SCROLLABLE  1

typedef int (*MenuCallback)(int selection);

typedef struct menu_t {
    uint8_t flags;
    uint8_t nopts;
    char **opts;

    vec2 topleft;
    uint8_t w;
    uint8_t h;
    uint8_t opt_sp;

    uint8_t cursor;

    element_t *m_element;
    element_t *o_element;
    font_t *font;
    MenuCallback cb;
};

menu_t *new_menu(uint8_t num_options, char *options[OPTION_LEN], font_t *font, uint8_t flags);
int m_set_aabb(menu_t *m, vec2 tl, vec2 br);
int m_draw(menu_t *m);

#endif // MENU_H