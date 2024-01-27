#ifndef MENU_H
#define MENU_H

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <TouchScreen.h>
#include <gfxfont.h>
#include "util.h"

#define OPTION_LEN  16
#define TITLE_LEN   32
#define MAX_VIS_OPT 4

// MENU DRAW CODES
#define M_DRAW      0
#define M_CLEAR     1
#define M_REFRESH   2

// MENU INTERACTIONS
#define CURS_UP     0
#define CURS_DOWN   1
#define CURS_RIGHT  2
#define CURS_LEFT   3
#define CURS_SELECT 4
#define CURS_BACK   5

// MENU TYPE FLAGS
#define M_DEFAULT   0
#define M_SET       1
#define M_TOGGLE    2
#define M_POPUP     3
#define M_COMBO     4
#define M_MESSAGE   5

// MENU FLAGS
#define M_NOTITLE   1
#define M_NOBACK    (1 << 1)
#define M_NOEXIT    (1 << 2)
#define M_FPARAM    (1 << 3)

// SET MENU SPACING/FORMATTING
#define S_VERT_SPACING  100
#define S_BTN_WIDTH     35
#define S_BTN_SPACING   10

#define NUM_MTYPES      6
// MENU INTERACT CODES
#define M_NOP           0
#define M_SELECT        1
#define M_BACK          2
#define M_EXIT          3
#define M_CONFIRM       4
#define M_UPDATED       5

#define ORIGIN (vec2){0,0}
#define CENTER (vec2){160,120}

#define COLUMN_W 115
#define COLUMN_H 30
#define A_COLUMN (vec2){20, 80}
#define B_COLUMN (vec2){20 + COLUMN_W, 80}
#define C_COLUMN (vec2){300, 80}

#define A_BTN    (vec2){A_COLUMN.x, 230 - COLUMN_H}
#define B_BTN    (vec2){B_COLUMN.x + 10, 230 - COLUMN_H}

#define M_POPDELAY 2500

#define TEST_RECT(x,y,x1,y1,w,h) (((x > x1) && (x < w + x1) && (y > y1) && (y < h + y1)))

typedef struct menu_t menu_t;
typedef struct option_t option_t;
typedef int (*MenuCallback)(menu_t *m, option_t *o);
typedef int (*MenuDraw)(Adafruit_ILI9341 *disp, menu_t *m, int clear);
typedef int (*MenuInteract)(menu_t *m, TSPoint p);

typedef struct vec2 {
    int x,y;
} vec2;

typedef struct option_t {
    char name[OPTION_LEN];
    menu_t *target;
    int value;
} option_t;

typedef struct menu_t {
    uint8_t flags;
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

    uint8_t m_type;

    MenuCallback cb;
} menu_t;

menu_t *new_menu(const char *title, uint8_t num_options, vec2 center, vec2 size, int type, uint8_t flags);
int m_set_options(menu_t *m, int num_options, option_t *options);
int m_set_size(menu_t *m, int width, int height);

int m_interact(menu_t *m, TSPoint p);
int m_interact(menu_t *m, int command);

int m_interact_default(menu_t *m, TSPoint p);
int m_interact_set(menu_t *m, TSPoint p);
int m_interact_toggle(menu_t *m, TSPoint p);
int m_interact_msg(menu_t *m, TSPoint p);

int m_draw(Adafruit_ILI9341 *disp, menu_t *m, int clear);

int m_draw_default(Adafruit_ILI9341 *disp, menu_t *m, int clear);
int m_draw_set(Adafruit_ILI9341 *disp, menu_t *m, int clear);
int m_toggle_value(Adafruit_ILI9341 *disp, menu_t *m, int clear);
int m_draw_popup(Adafruit_ILI9341 *disp, menu_t *m, int clear);
int m_draw_alert(Adafruit_ILI9341 *disp, menu_t *m, int clear);

int m_test_touch(TSPoint t, menu_t *m);

extern MenuDraw MENU_DRAW[NUM_MTYPES];
extern MenuInteract MENU_INTERACT[NUM_MTYPES];


#endif // MENU_H