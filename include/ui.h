#ifndef UI_H
#define UI_H

#include <Adafruit_ILI9341.h>
#include <TouchScreen.h>
#include "menu.h"
#include "callbacks.h"

#define M_SIZE (vec2){320,240}
#define TS_MINX 100
#define TS_MINY 100
#define TS_MAXX 900
#define TS_MAXY 900
#define PRESSURE_THRESH 40
#define XPOS    A2
#define YPOS    A3
#define YMIN    A1
#define XMIN    A0

#define NUM_PRESETS 6
#define SAVE 		0
#define LOAD 		1
#define DEL  		2

typedef struct ui_t {
	menu_t *active;
	menu_t *previous;

	int pidx;
	menu_t *path[8];

  uint8_t cmask;
  uint16_t pmask;
  int set_value;
  int op_value;
} ui_t;

extern Adafruit_ILI9341 tft;
extern TouchScreen ts;

extern menu_t *set_param;
extern menu_t *alert;
extern menu_t *popup;
extern menu_t *main_menu;
extern menu_t *timers;
extern menu_t *purge_timers;
extern menu_t *delay_timers;
extern menu_t *alarms;
extern menu_t *mode;
extern menu_t *presets;
extern menu_t *circuit_select;
extern menu_t *reclaimer_config;
extern menu_t *pick_param;
extern menu_t *pick_pid;
extern menu_t *pick_preset;

void init_ui(ui_t *ui);
void init_menus();
void init_options();

TSPoint get_press(TouchScreen *ts);
void update_ui(ui_t *ui);
void setup_callbacks();

#endif // UI_H