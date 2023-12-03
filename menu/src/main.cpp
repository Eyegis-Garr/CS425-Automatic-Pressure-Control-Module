#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <TouchScreen.h>
#include <gfxfont.h>
#include "menu.h"

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 80
#define TS_MINY 80
#define TS_MAXX 900
#define TS_MAXY 900

#define XPOS    A2
#define YPOS    A3
#define YMIN    A1
#define XMIN    A0

TouchScreen ts = TouchScreen(YPOS, XPOS, YMIN, XMIN, 300);
#define TFT_CS 37
#define TFT_DC 36
Adafruit_ILI9341 tft = Adafruit_ILI9341(tft8bitbus, 22, 35, 36, 37, 33, 34);

void init_menus();
void init_menu_options();
void init_input();
int get_input();

struct ui {
	menu_t *active;
} ui;

menu_t *main_menu, *presets, *config, *alarms, *pid;
menu_t *pick_preset, *pick_param, *pick_mode, *pick_pid;
menu_t *set_param;

void init_menus() {
	main_menu = new_menu("MAIN", 8, (vec2){160,120});
	m_set_size(main_menu, 320, 220);
	m_set_draw(main_menu, M_DEFAULT);
	main_menu->cur_color = 0x05A0;

	init_menu_options();
}

void init_menu_options() {
	option_t opts[8];

	// MAIN MENU
	opts[0] = (option_t) {"Presets", 	NULL,    0};
	opts[1] = (option_t) {"Times", 		NULL, 	 0};
	opts[2] = (option_t) {"PID",  		NULL,  	 0};
	opts[3] = (option_t) {"Pressures", 	NULL,    0};
	opts[4] = (option_t) {"Alarms", 	NULL,    0};
	opts[5] = (option_t) {"Mode", 		NULL,    0};
	opts[6] = (option_t) {"BACK", 		NULL,    0};
	opts[7] = (option_t) {"EXIT", 		NULL,    0};
	m_set_options(main_menu, main_menu->nopts, opts);
}

// void interact_menu(menu_t *m) {
//   TSPoint p = ts.getPoint();
//   p.x = map(p.y, TS_MINY, TS_MAXY, 0, tft.width());
//   p.y = map(p.x, TS_MINX, TS_MAXX, 0, tft.height());
//   int idx = m_test_touch(p, main_menu);
//   if (idx > -1) {

// 	ui.active = m->options[idx].target;
// 	m_draw(&tft, m, 1);	// erase menu
// 	m_draw(&tft, ui.active, 0);
//   }
// }

void setup(void)
{
  Serial.begin(9600);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  init_menus();

  m_draw(&tft, main_menu, 0);
}

int pidx = 0;
void loop()
{
  TSPoint p = ts.getPoint();
  p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  p.y = 240 - map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
  int idx = m_test_touch(p, main_menu);
  if (idx >= 0 && p.z > 70 && pidx != idx) {
	Serial.println(main_menu->options[idx].name);
	main_menu->cursor = idx;
	m_draw(&tft, main_menu, 0);
	pidx = idx;
  }
}


