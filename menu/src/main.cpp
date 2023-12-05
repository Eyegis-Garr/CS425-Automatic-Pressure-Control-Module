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

#define TFT_CS 37
#define TFT_DC 36

#define M_SIZE (vec2){320,240}

TouchScreen ts = TouchScreen(YPOS, XPOS, YMIN, XMIN, 300);
Adafruit_ILI9341 tft = Adafruit_ILI9341(tft8bitbus, 22, 35, 36, 37, 33, 34);

void init_menus();
void init_input();
int get_input();

menu_t *set_param;
menu_t *main_menu;
menu_t *mode;
menu_t *presets;
menu_t *circuit_select;
menu_t *reclaimer_config;
menu_t *pick_param;
menu_t *pick_pid;
menu_t *pick_preset;

struct ui { 
	menu_t *active;
	menu_t *previous;

	int pidx;
	menu_t *path[8];
} ui;

void create_menus() {
	set_param = new_menu("SET PARAMETER", 1, CENTER, M_SIZE, M_SET);
	main_menu = new_menu("MAIN", 3, CENTER, M_SIZE, M_DEFAULT);
	mode = new_menu("MODE SELECT", 3, CENTER, M_SIZE, M_DEFAULT);
	presets = new_menu("PRESETS", 3, CENTER, M_SIZE, M_DEFAULT);
	circuit_select = new_menu("PICK CIRCUIT", 7, CENTER, M_SIZE, M_DEFAULT);
	reclaimer_config = new_menu("RECLAIMER CONFIG", 4, CENTER, M_SIZE, M_DEFAULT);
	pick_param = new_menu("PICK PARAM.", 5, CENTER, M_SIZE, DEFAULT);
	pick_pid = new_menu("PID CONFIG", 3, CENTER, M_SIZE, M_DEFAULT);
	pick_preset = new_menu("PICK PRESET", 4, CENTER, M_SIZE, M_PRINT);
}

void init_options() {
	option_t opts[8];

	opts[0] = (option_t) {"", 		NULL,    1234};
	m_set_options(set_param, set_param->nopts, opts);
	
	opts[0] = (option_t) {"Mode", 	mode,     0};
	opts[1] = (option_t) {"Preset", presets,  0};
	opts[2] = (option_t) {"Config", pick_pid, 0};
	m_set_options(main_menu, main_menu->nopts, opts);

	opts[0] = (option_t) {"Auto", 	 main_menu,    0};
	opts[1] = (option_t) {"Manual",	 main_menu,    0};
	opts[2] = (option_t) {"Standby", main_menu,    0};
	m_set_options(mode, mode->nopts, opts);

	opts[0] = (option_t) {"Save", 	pick_preset, 0};
	opts[1] = (option_t) {"Load", 	pick_preset, 0};
	opts[2] = (option_t) {"Delete", pick_preset, 0};
	m_set_options(presets, presets->nopts, opts);

	// can create a circuit -> value map for each circuit
	opts[0] = (option_t) {"MARX", 		pick_param, 0};
	opts[1] = (option_t) {"MX-TG70", 	pick_param, 0};
	opts[2] = (option_t) {"MTG", 		pick_param, 0};
	opts[3] = (option_t) {"SWITCH", 	pick_param, 0};
	opts[4] = (option_t) {"SW-TG70", 	pick_param, 0};
	opts[5] = (option_t) {"RECLAIM", 	set_param, 	0};		// only has configurable pressure
	opts[6] = (option_t) {"MIN SUPL", 	set_param, 	0};		// only has configurable pressure
	m_set_options(circuit_select, circuit_select->nopts, opts);

	// could potentially swap these two in the hierarchy
	// depending on whether it's more common to modify more params for a single circuit
	// or the same param for multiple circuits
	
	opts[0] = (option_t) {"Pressure", 	set_param, 0};
	opts[1] = (option_t) {"Purge", 		set_param, 0};
	opts[2] = (option_t) {"Delay", 		set_param, 0};
	opts[3] = (option_t) {"Alarm", 		set_param, 0};
	opts[4] = (option_t) {"PID", 		pick_pid,  0};
	m_set_options(pick_param, pick_param->nopts, opts);

	opts[0] = (option_t) {"KP", set_param, 		0};
	opts[1] = (option_t) {"KI", set_param, 		0};
	opts[2] = (option_t) {"KD",	set_param, 		0};
	m_set_options(pick_pid, pick_pid->nopts, opts);

	opts[0] = (option_t) {"REC ON", 	NULL,    0};	//FUNCTION
	opts[1] = (option_t) {"REC OFF", 	NULL,    0};	//FUNCTION
	opts[2] = (option_t) {"MIN SPLY", 	NULL,    0};	//FUNCTION
	opts[3] = (option_t) {"Safe Delay", NULL,    0};	//FUNCTION
	m_set_options(reclaimer_config, reclaimer_config->nopts, opts);
}

void init_menus() {
	create_menus();
	init_options();
}

void setup(void)
{
  Serial.begin(9600);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  init_menus();

  ui.active = main_menu;
  ui.previous = NULL;
  ui.path[0] = ui.active;
  ui.pidx = 0;

  m_draw(&tft, ui.active, 0);
}

void loop()
{
  TSPoint p;
  while (ts.isTouching()) {
	p = ts.getPoint();
	p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
	p.y = 240 - map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
  }

  if (p.z > 80) {		// if pressure is above threshold
	int code = m_interact(ui.active, p);	// make interact-call with touch point
	switch (code) {
		case M_UPDATED:
			// triggers redraw/refresh
			m_draw(&tft, ui.active, 1);
			m_draw(&tft, ui.active, 0);
			break;
		case M_SELECT:
			m_draw(&tft, ui.active, 1);		// clear current menu
			ui.pidx += 1;					// increment stack
			ui.active = ui.active->options[ui.active->cursor].target;	// swap active menu
			ui.path[ui.pidx] = ui.active;	// update path
			m_draw(&tft, ui.active, 0);		// draw new active menu
			break;
		case M_CONFIRM:
			// grab previous menu
			ui.previous = ui.path[ui.pidx - 1];
			// copy over the value from the active menu, to the value in the selected option of previous
			ui.previous->options[ui.previous->cursor].value = ui.active->options[ui.active->cursor].value;
			// CONFIRM falls through to M_BACK, swapping to previous menu
		case M_BACK:
			m_draw(&tft, ui.active, 1);
			ui.active = ui.path[ui.pidx - 1];
			ui.pidx -= 1;
			m_draw(&tft, ui.active, 0);
			break;
		case M_EXIT:
			m_draw(&tft, ui.active, 1);
			ui.active = ui.path[0];
			ui.pidx = 0;
			m_draw(&tft, ui.active, 0);
			break;
		case M_NOP:
		default:
			break;
	}
  }
}


