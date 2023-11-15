#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include "element.h"
#include "font.h"
#include "util.h"

#define OPTION_LEN  32
#define TITLE_LEN   16

// MENU INTERACTIONS
#define CURS_UP     0
#define CURS_DOWN   1
#define CURS_RIGHT  2
#define CURS_LEFT   3
#define CURS_SELECT 4
#define CURS_BACK   5

typedef int (*MenuCallback)(int selection);

/*
TODO:
    option abstraction
    menu hierarchy abstraction
    menu hierarchy configuration
    menu hierarchy traversal
    2D menu option traversal?
        array of option lists/pairs
    better color formatting?
    menu draw clearing
    menu freeing/pooling?
*/

typedef struct menu_t {
    char *title;
    uint8_t nopts;
    const char **opts;

    vec2 center;
    int w;
    int h;
    int opt_sp;

    int cursor;
    uint16_t cur_color;

    element_t m_element;
    element_t o_element;
    font_t *font;
    MenuCallback cb;
};

menu_t *new_menu(const char *title, uint8_t num_options, const char *options[], font_t *font, uint8_t flags);
int m_set_aabb(menu_t *m, vec2 tl, vec2 br);
int m_interact(menu_t *m, int it);
int m_opt_redraw(menu_t *m, int opt);
int m_draw(menu_t *m);

#endif // MENU_H