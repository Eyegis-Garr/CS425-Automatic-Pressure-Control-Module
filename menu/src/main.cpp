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

#define CENTER (vec2){160,120}

void init_menus();
void init_input();
int get_input();

menu_t *main_menu, *presets, *config, *alarms, *pressures, *timers, *purge_timers, *delay_timers, *safety_timers;
menu_t *pick_preset, *pick_param, *pick_mode, *pick_pid, *pick_circuit;
menu_t *set_param, *reclaimer_config;

struct ui { 
	menu_t *active;
	menu_t *previous;

	int pidx;
	menu_t *path[8];
} ui;

void create_menus() {
	set_param = new_menu("SET PARAMETER", 8, CENTER);
	m_set_size(set_param, 320, 220);
	m_set_draw(set_param, M_SET);
	m_set_interact(set_param, M_SET);
	set_param->cur_color = 0x05A0;

	main_menu = new_menu("MAIN", 8, CENTER);
	m_set_size(main_menu, 320, 220);
	m_set_draw(main_menu, M_DEFAULT);
	main_menu->cur_color = 0x05A0;

	timers = new_menu("TIMERS CONFIG", 8, CENTER);
	m_set_size(timers, 320, 220);
	m_set_draw(timers, M_DEFAULT);
	timers->cur_color = 0x05A0;

	presets = new_menu("PRESETS", 8, CENTER);
	m_set_size(presets, 320, 220);
	m_set_draw(presets, M_DEFAULT);
	presets->cur_color = 0x05A0;

	delay_timers = new_menu("SET CIRCUIT DELAY", 8, CENTER);
	m_set_size(delay_timers, 320, 220);
	m_set_draw(delay_timers, M_DEFAULT);
	delay_timers->cur_color = 0x05A0;

	purge_timers = new_menu("SET PURGE TIMES", 8, CENTER);
	m_set_size(purge_timers, 320, 220);
	m_set_draw(purge_timers, M_DEFAULT);
	purge_timers->cur_color = 0x05A0;

	alarms = new_menu("ALARM CONFIG", 8, CENTER);
	m_set_size(alarms, 320, 220);
	m_set_draw(alarms, M_DEFAULT);
	alarms->cur_color = 0x05A0;

	reclaimer_config = new_menu("RECLAIMER CONFIG", 8, CENTER);
	m_set_size(reclaimer_config, 320, 220);
	m_set_draw(reclaimer_config, M_DEFAULT);
	reclaimer_config->cur_color = 0x05A0;

	pressures = new_menu("SET PRESSURES", 8, CENTER);
	m_set_size(pressures, 320, 220);
	m_set_draw(pressures, M_DEFAULT);
	pressures->cur_color = 0x05A0;

	pick_pid = new_menu("PID CONFIG", 8, CENTER);
	m_set_size(pick_pid, 320, 220);
	m_set_draw(pick_pid, M_DEFAULT);
	pick_pid->cur_color = 0x05A0;
}

void init_options() {
	option_t opts[8];

	opts[0] = (option_t) {"0000", 		NULL,    1234};	// SUBMENU (PRESETS MENU)
	opts[1] = (option_t) {"", 			NULL, 	 0};	// SUBMENU (TIMERS MENU)
	opts[2] = (option_t) {"",  			NULL,  	 0};	// SUBMENU (PID MENU)
	opts[3] = (option_t) {"", 			NULL,    0};	// SUBMENU (PRESSURES MENU)
	opts[4] = (option_t) {"", 			NULL,    0};	// SUBMENU (ALARM CONFIG)
	opts[5] = (option_t) {"", 			NULL,    0};	// SUBMENU (RECLAIMER MENU)
	opts[6] = (option_t) {"CANCEL", 	NULL,    0};	// Not necessary on MAIN
	opts[7] = (option_t) {"CONFIRM", 	NULL,    0};	// Not necessary on MAIN
	m_set_options(set_param, set_param->nopts, opts);
	
	opts[0] = (option_t) {"Presets", 	presets,    0};	//SUBMENU (PRESETS MENU)
	opts[1] = (option_t) {"Times", 		timers, 	 0};	//SUBMENU (TIMERS MENU)
	opts[2] = (option_t) {"PID",  		pick_pid,  	 0};	//SUBMENU (PID MENU)
	opts[3] = (option_t) {"Pressures", 	pressures,    0};	//SUBMENU (PRESSURES MENU)
	opts[4] = (option_t) {"Alarms", 	alarms,    0};	//SUBMENU (ALARM CONFIG)
	opts[5] = (option_t) {"Reclaimer", 	reclaimer_config,    0};	//SUBMENU (RECLAIMER MENU)
	opts[6] = (option_t) {"", 		NULL,    0};	//Not necessary on MAIN
	opts[7] = (option_t) {"", 		NULL,    0};	//Not necessary on MAIN
	m_set_options(main_menu, main_menu->nopts, opts);

	opts[0] = (option_t) {"Save", 	NULL,    0};	//SUBMENU, FUNCTION
	opts[1] = (option_t) {"Load", 	NULL, 	 0};	//SUBMENU, FUNCTION
	opts[2] = (option_t) {"Delete",  NULL,  	 0};	//SUBMENU, FUNCTION
	opts[3] = (option_t) {"", 				NULL,    0};
	opts[4] = (option_t) {"", 				NULL,    0};
	opts[5] = (option_t) {"", 				NULL,    0};
	opts[6] = (option_t) {"BACK", 		main_menu,    0};	//To MAIN
	opts[7] = (option_t) {"EXIT", 		main_menu,    0};	//To MAIN
	
	m_set_options(presets, presets->nopts, opts);	

	opts[0] = (option_t) {"MARX", 		NULL,    0};	//FUNCTION
	opts[1] = (option_t) {"MARXTG70", 	NULL, 	 0};	//FUNCTION
	opts[2] = (option_t) {"MTG",  		NULL,  	 0};	//FUNCTION
	opts[3] = (option_t) {"SWITCH", 	NULL,    0};	//FUNCTION
	opts[4] = (option_t) {"SWTG70",     NULL,    0};	//FUNCTION
	opts[5] = (option_t) {"", 		    NULL,    0};
	opts[7] = (option_t) {"BACK", 		main_menu,    0};	//To MAIN
	opts[7] = (option_t) {"EXIT", 		main_menu,    0};	//To MAIN
	m_set_options(pressures, pressures->nopts, opts);

	opts[0] = (option_t) {"REC ON", 	NULL,    0};	//FUNCTION
	opts[1] = (option_t) {"REC OFF", 	NULL,    0};	//FUNCTION
	opts[2] = (option_t) {"MIN SPLY", 	NULL,    0};	//FUNCTION
	opts[3] = (option_t) {"Safe Delay",   NULL,    0};	//FUNCTION
	opts[4] = (option_t) {"", 				NULL,    0};
	opts[7] = (option_t) {"BACK", 		main_menu,    0};	//To MAIN
	opts[7] = (option_t) {"EXIT", 		main_menu,    0};	//To MAIN
	m_set_options(reclaimer_config, reclaimer_config->nopts, opts);

	opts[0] = (option_t) {"Sound ON/OFF", 	NULL,    0};	//FUNCTION
	opts[1] = (option_t) {"MARX", 	NULL,    0};	//FUNCTION
	opts[2] = (option_t) {"MARXTG70", NULL, 	 0};	//FUNCTION
	opts[3] = (option_t) {"MTG",  	NULL,  	 0};	//FUNCTION
	opts[4] = (option_t) {"SWITCH", 	NULL,    0};	//FUNCTION
	opts[5] = (option_t) {"SWTG70", NULL,    0};	//FUNCTION
	opts[6] = (option_t) {"BACK", 		main_menu,    0};	//To MAIN
	opts[7] = (option_t) {"EXIT", 		main_menu,    0};	//To MAIN
	
	m_set_options(alarms, alarms->nopts, opts);

	opts[0] = (option_t) {"Purge", purge_timers,   0};	//SUBMENU (TIMERS MENU)
	opts[1] = (option_t) {"Delay", delay_timers, 0};	//SUBMENU (TIMERS MENU)
	opts[2] = (option_t) {"", 		NULL,  	 0};
	opts[3] = (option_t) {"", 		NULL,    0};
	opts[4] = (option_t) {"", 		NULL,    0};
	opts[5] = (option_t) {"", 		NULL,    0};
	opts[6] = (option_t) {"BACK", 		main_menu,    0};	//To MAIN
	opts[7] = (option_t) {"EXIT", 		main_menu,    0};	//To MAIN
	m_set_options(timers, timers->nopts, opts);

	opts[0] = (option_t) {"MARX", 	set_param,    0};	//FUNCTION
	opts[1] = (option_t) {"MARXTG70", NULL, 	 0};	//FUNCTION
	opts[2] = (option_t) {"MTG",  	NULL,  	 0};	//FUNCTION
	opts[3] = (option_t) {"SWITCH", 	NULL,    0};	//FUNCTION
	opts[4] = (option_t) {"SWTG70", NULL,    0};	//FUNCTION
	opts[5] = (option_t) {"",		NULL,    0};
	opts[6] = (option_t) {"BACK", 		timers,    0};	//To timers
	opts[7] = (option_t) {"EXIT", 		main_menu,    0};	//To MAIN
	m_set_options(purge_timers, purge_timers->nopts, opts);	

	opts[0] = (option_t) {"MARX", 	NULL,    0};	//FUNCTION
	opts[1] = (option_t) {"MARXTG70", NULL, 	 0};	//FUNCTION
	opts[2] = (option_t) {"MTG",  	NULL,  	 0};	//FUNCTION
	opts[3] = (option_t) {"SWITCH", 	NULL,    0};	//FUNCTION
	opts[4] = (option_t) {"SWTG70", NULL,    0};	//FUNCTION
	opts[5] = (option_t) {"",		NULL,    0};
	opts[6] = (option_t) {"BACK", 		timers,    0};	//To TIMERS 
	opts[7] = (option_t) {"EXIT", 		main_menu,    0};	//To MAIN
	m_set_options(delay_timers, delay_timers->nopts, opts);

	opts[0] = (option_t) {"MARX", 		 NULL,    0};	//FUNCTION
	opts[1] = (option_t) {"MARXTG70",	 NULL, 	 0};	//FUNCTION
	opts[2] = (option_t) {"MTG",  		 NULL,  	 0};	//FUNCTION
	opts[3] = (option_t) {"SWITCH", 	 NULL,    0};	//FUNCTION
	opts[4] = (option_t) {"SWTG70", NULL,    0};	//FUNCTION
	opts[5] = (option_t) {"", 				 NULL,    0};
	opts[6] = (option_t) {"BACK", 			 main_menu,    0};	//To MAIN
	opts[7] = (option_t) {"EXIT", 			 main_menu,    0};	//To MAIN
	m_set_options(pick_pid, pick_pid->nopts, opts);	
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

