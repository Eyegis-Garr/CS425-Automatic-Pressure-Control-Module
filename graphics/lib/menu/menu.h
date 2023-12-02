#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include "graphics.h"
#include "font.h"
#include "util.h"

#define OPTION_LEN  16
#define TITLE_LEN   16
#define MAX_VIS_OPT 4

// MENU INTERACTIONS
#define CURS_UP     0
#define CURS_DOWN   1
#define CURS_RIGHT  2
#define CURS_LEFT   3
#define CURS_SELECT 4
#define CURS_BACK   5

// MENU DRAW FUNCTIONS
#define M_DEFAULT   0
#define M_SET       1
#define M_TOGGLE    2
#define M_PRINT     3

// MENU INTERACT FUNCTIONS
#define I_DEFAULT   0
#define I_SET       1
#define I_TOGGLE    2
#define I_PRINT     3

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
*/

typedef struct menu_t menu_t;

typedef struct option_t {
    char name[OPTION_LEN];
    menu_t *target;
    int value;
} option_t;

typedef int (*MenuCallback)(menu_t *m, option_t *o);
typedef int (*MenuDrawCallback)(menu_t *m, int clear);
typedef int (*MenuInteractCallback)(menu_t *m, int it);

typedef struct menu_t {
    char title[TITLE_LEN];
    uint8_t nopts;
    option_t *options;

    vec2 center;
    int w;
    int h;

    uint8_t opt_offset;
    uint8_t opt_sp;
    uint8_t opt_div;
    uint8_t vis_items;

    uint8_t coffset;
    int cursor;
    uint16_t cur_color;

    uint8_t draw_flag;
    uint8_t it_flag;

    element_t *m_element;
    element_t *o_element;
    font_t *font;
    MenuCallback cb;
} menu_t;

menu_t *new_menu(const char *title, uint8_t num_options, font_t *font, vec2 center,
                element_t *menu_element, element_t *option_element);
int m_set_options(menu_t *m, int num_options, option_t *options);
int m_set_size(menu_t *m, int width, int height);
int m_set_draw(menu_t *m, int draw_flag);
int m_set_interact(menu_t *m, int it_flag);

int m_interact(menu_t *m, int it);

int m_interact_default(menu_t *m, int it);
int m_interact_set(menu_t *m, int it);
int m_interact_toggle(menu_t *m, int it);

int m_draw(menu_t *m, int clear);

int m_list_options(menu_t *m, int clear);
int m_set_value(menu_t *m, int clear);
int m_toggle_value(menu_t *m, int clear);
int m_print_value(menu_t *m, int clear);

extern MenuDrawCallback MENU_DRAW[4];
extern MenuInteractCallback MENU_INTERACT[4];

#endif // MENU_H