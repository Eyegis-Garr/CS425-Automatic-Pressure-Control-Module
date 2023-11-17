#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include "element.h"
#include "font.h"
#include "util.h"

#define OPTION_LEN  32
#define TITLE_LEN   16
#define MAX_VIS_OPT 4

// MENU INTERACTIONS
#define CURS_UP     0
#define CURS_DOWN   1
#define CURS_RIGHT  2
#define CURS_LEFT   3
#define CURS_SELECT 4
#define CURS_BACK   5

// BASIC MENU TYPES
#define M_SIMPLE    0
#define M_TOGGLE    1
#define M_INPUT     2

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

// master menu LUT

typedef struct menu_t menu_t;

typedef struct option_t {
    char *name;
    menu_t *target;
    int value;
} option_t;

typedef int (*MenuCallback)(menu_t *m, int selection);
typedef struct menu_t {
    char *title;
    uint8_t nopts;
    option_t *options;

    vec2 center;
    int w;
    int h;

    int opt_offset;
    int opt_sp;
    int opt_div;
    int vis_items;

    int coffset;
    int cursor;
    uint16_t cur_color;

    element_t *m_element;
    element_t *o_element;
    font_t *font;
    MenuCallback cb;
} menu_t;

menu_t *new_menu(const char *title, uint8_t num_options, font_t *font, vec2 center,
                element_t *menu_element, element_t *option_element);
int m_set_options(menu_t *m, int num_options, option_t *options);
int m_set_size(menu_t *m, int width, int height);

int m_interact(menu_t *m, int it);

int m_draw(menu_t *m, int clear);
int m_draw_options(menu_t *m, int clear);

#endif // MENU_H