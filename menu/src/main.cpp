#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <TouchScreen.h>
#include <gfxfont.h>
#include <avr8-stub.h>
#include "menu.h"

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 100
#define TS_MINY 100
#define TS_MAXX 900
#define TS_MAXY 900
#define PRESSURE_THRESH 40

#define XPOS    A2
#define YPOS    A3
#define YMIN    A1
#define XMIN    A0

#define TFT_CS 	37
#define TFT_DC 	36

#define M_SIZE (vec2){320,240}

#define NUM_PRESETS 6
#define SAVE 		0
#define LOAD 		1
#define DEL  		2

TouchScreen ts = TouchScreen(YPOS, XPOS, YMIN, XMIN, 300);
Adafruit_ILI9341 tft = Adafruit_ILI9341(tft8bitbus, 22, 35, 36, 37, 33, 34);

menu_t *set_param;
menu_t *alert;
menu_t *popup;

menu_t *main_menu;
menu_t *timers;
menu_t *purge_timers;
menu_t *delay_timers;
menu_t *alarms;
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

TSPoint get_press(TouchScreen *ts);
int preset_cb(menu_t *m, option_t *o);
int pick_preset_cb(menu_t *m, option_t *o);
int alarms_cb(menu_t *m, option_t *o);
void create_menus();
void init_options();
void init_menus();
int get_input();

void setup(void) {
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

void loop() {
  TSPoint p = get_press(&ts);
  if (p.z > PRESSURE_THRESH) {		// if pressure is above threshold
	int code = m_interact(ui.active, p);	// make interact-call with touch point
	switch (code) {
		case M_UPDATED:
			// triggers redraw/refresh
			m_draw(&tft, ui.active, M_CLEAR);
			m_draw(&tft, ui.active, M_DRAW);
			break;
		case M_SELECT:
			// callbacks invoked
			m_draw(&tft, ui.active, M_CLEAR);		// clear current menu
			ui.active = ui.active->options[ui.active->cursor].target;	// swap active menu
			ui.path[++ui.pidx] = ui.active;	// update path
			m_draw(&tft, ui.active, M_DRAW);		// draw new active menu
			break;
		case M_CONFIRM:
			// grab previous menu
			ui.previous = ui.path[ui.pidx - 1];
			// copy over the value from the active menu, to the value in the selected option of previous
			ui.previous->options[ui.previous->cursor].value = ui.active->options[ui.active->cursor].value;
			// CONFIRM falls through to M_BACK, swapping to previous menu
		case M_BACK:
			if (ui.pidx) {
				m_draw(&tft, ui.active, M_CLEAR);
				ui.active = ui.path[--ui.pidx];
				m_draw(&tft, ui.active, M_DRAW);
			}			
			break;
		case M_EXIT:
			m_draw(&tft, ui.active, M_CLEAR);
			ui.active = ui.path[0];
			ui.pidx = 0;
			m_draw(&tft, ui.active, M_DRAW);
			break;
		case M_NOP:
		default:
			break;
	}
  }
}

TSPoint get_press(TouchScreen *ts) {
  TSPoint p = ts->getPoint();
  while (ts->isTouching()) {
	p = ts->getPoint();
	p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
	p.y = 240 - map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
  }

  return p;
}

int preset_cb(menu_t *m, option_t *o) {
	// int selected_preset = m->options[m->cursor].value;
	int op;
	if (m->cursor == 0) { // save
		// overwrite check
		strcpy(alert->title, "SAVE ");
		op = SAVE; 
	} else if (m->cursor == 1) { // load
		strcpy(alert->title, "LOAD "); 
		op = LOAD;
	} else if (m->cursor == 2) { // delete
		strcpy(alert->title, "DELETE "); 
		op = DEL;
	}

	menu_t *target = o->target;		// o->target is always pick_preset
	for (int i = 0; i < target->nopts; i += 1) {
		target->options[i].value = op;
	}

	return 0;
}

int pick_preset_cb(menu_t *m, option_t *o) {
	int code = M_NOP;
	TSPoint p;

	strcat(alert->title, o->name);
	
	m_draw(&tft, m, M_CLEAR);		// clear preset selection menu
	m_draw(&tft, alert, M_DRAW);	// display alert (confirm/cancel)
	while (code == M_NOP) {			// wait for selection
		p = get_press(&ts);
		if (p.z > 50) {
			code = m_interact(alert, p);
		}
	}
	m_draw(&tft, alert, M_CLEAR);	// clear alert

	if (code == M_CONFIRM) {
		switch (o->value) {
			case SAVE:		// save preset
				// compute checksum
				// test save success
				sprintf(popup->title, "%s success!", alert->title);
				m_draw(&tft, popup, M_REFRESH);
				break;
			case LOAD:		// load preset
				break;
			case DEL:		// delete preset
				break;
			default:
				break;
		}
	}

	return 0;
}

int alarms_cb(menu_t *m, option_t *o) {
	if (m->cursor == 0) {
		int code = M_NOP, dir;
		TSPoint p;

		if (o->value == 0) {  			// off, turn on
			strcpy(alert->title, "Set Sound ON?");
			dir = 1;
		} else if (o->value == 1) { 	// on, turn off
			strcpy(alert->title, "Set Sound OFF?");
			dir = -1;
		}
		
		m_draw(&tft, m, M_CLEAR);		// clear preset selection menu
		m_draw(&tft, alert, M_DRAW);	// display alert (confirm/cancel)
		while (code == M_NOP) {			// wait for selection
			p = get_press(&ts);
			if (p.z > 50) {
				code = m_interact(alert, p);
			}
		}
		m_draw(&tft, alert, M_CLEAR);	// clear alert

		if (code == M_CONFIRM) {
			o->value += dir;
			sprintf(popup->title, "Sound is %s.", (o->value) ? "ON" : "OFF");
			m_draw(&tft, popup, M_REFRESH);
		}
	}

	return 0;
}

void create_menus() {
	alert = new_menu("", 0, CENTER, M_SIZE, M_MESSAGE, 0);
	popup = new_menu("", 0, CENTER, M_SIZE, M_POPUP, 0);
	set_param = new_menu("SET PARAMETER", 1, CENTER, M_SIZE, M_SET, 0);

	main_menu = new_menu("MAIN", 6, CENTER, M_SIZE, M_DEFAULT, M_NOBACK | M_NOEXIT);
	reclaimer_config = new_menu("RECLAIMER CONFIG", 4, CENTER, M_SIZE, M_DEFAULT, 0);
	timers = new_menu("TIMERS", 2, CENTER, M_SIZE, M_DEFAULT, 0);
	pick_pid = new_menu("PICK PID", 3, CENTER, M_SIZE, M_DEFAULT, 0);
	circuit_select = new_menu("PICK CIRCUIT", 7, CENTER, M_SIZE, M_DEFAULT, 0);

	alarms = new_menu("ALARMS", 6, CENTER, M_SIZE, M_DEFAULT, 0);
	alarms->cb = alarms_cb;
	presets = new_menu("PRESETS", 3, CENTER, M_SIZE, M_DEFAULT, 0);
	presets->cb = preset_cb;
	pick_preset = new_menu("PICK PRESET", NUM_PRESETS, CENTER, M_SIZE, M_DEFAULT, 0);
	pick_preset->cb = pick_preset_cb;
}

void init_options() {
	option_t opts[8];

	// SET MENU
	opts[0] = (option_t) {"", 		NULL,    1234};
	m_set_options(set_param, set_param->nopts, opts);
	
	// MAIN MENU
	opts[0] = (option_t) {"Presets", 	presets,   			0};
	opts[1] = (option_t) {"Times", 		timers,    			0};
	opts[2] = (option_t) {"PID",  		pick_pid,  			0};
	opts[3] = (option_t) {"Pressures", 	circuit_select,		0};
	opts[4] = (option_t) {"Alarms", 	alarms,    			0};
	opts[5] = (option_t) {"Reclaimer", 	reclaimer_config,   0};
	m_set_options(main_menu, main_menu->nopts, opts);

	// PRESETS
	opts[0] = (option_t) {"Save",	pick_preset,	0};
	opts[1] = (option_t) {"Load",	pick_preset,	0};
	opts[2] = (option_t) {"Delete",	pick_preset,	0};
	m_set_options(presets, presets->nopts, opts);

	// PICK PRESETS
	for (int i = 0; i < NUM_PRESETS; i += 1) {
		opts[i] = (option_t) { "", presets, 0 };
		sprintf(opts[i].name, "Preset %d", i);
	}
	m_set_options(pick_preset, pick_preset->nopts, opts);

	// CIRCUIT SELECT
	opts[0] = (option_t) {"MARX", 		set_param, 0};
	opts[1] = (option_t) {"MARXTG70", 	set_param, 0};
	opts[2] = (option_t) {"MTG",  		set_param, 0};
	opts[3] = (option_t) {"SWITCH", 	set_param, 0};
	opts[4] = (option_t) {"SWTG70",     set_param, 0};
	opts[5] = (option_t) {"Reclaim",   set_param, 0};
	opts[6] = (option_t) {"Min SUPLY", set_param, 0};
	m_set_options(circuit_select, circuit_select->nopts, opts);

	// RECLAIMER CONFIG
	opts[0] = (option_t) {"REC ON", 		set_param,  0};	// displays popup
	opts[1] = (option_t) {"REC OFF", 		set_param,  0};	// displays popup
	opts[2] = (option_t) {"MIN SPLY", 		set_param,  0};	
	opts[3] = (option_t) {"REC Delay",   	set_param,  0};
	m_set_options(reclaimer_config, reclaimer_config->nopts, opts);

	// ALARMS
	opts[0] = (option_t) {"Sound",		 	alarms,    	0}; // toggle menu or popup
	opts[1] = (option_t) {"MARX", 			set_param,  0};
	opts[2] = (option_t) {"MARXTG70", 		set_param, 	0};
	opts[3] = (option_t) {"MTG",  			set_param,  0};
	opts[4] = (option_t) {"SWITCH", 		set_param,  0};
	opts[5] = (option_t) {"SWTG70", 		set_param,  0};
	m_set_options(alarms, alarms->nopts, opts);

	// TIMERS
	opts[0] = (option_t) {"Purge", circuit_select,   0};
	opts[1] = (option_t) {"Delay", circuit_select,   0};
	m_set_options(timers, timers->nopts, opts);

	// PICK PID
	opts[0] = (option_t) {"KP",  circuit_select,    0};
	opts[1] = (option_t) {"KI",	 circuit_select,    0};
	opts[2] = (option_t) {"KD",  circuit_select,    0};
	m_set_options(pick_pid, pick_pid->nopts, opts);
}

void init_menus() {
	create_menus();
	init_options();
}


