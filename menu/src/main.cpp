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

#define XPOS    A0
#define YPOS    A3
#define YMIN    A2
#define XMIN    A1

TouchScreen ts = TouchScreen(XPOS, YPOS, XMIN, YMIN, 300);
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

menu_t *main_menu, *presets, *config, *alarms, *pressures, *timers, *delay_timers, *safety_timers;
menu_t *pick_preset, *pick_param, *pick_mode, *pick_pid, *pick_circuit;
menu_t *set_param;

void init_menus() {
	init_menu_options();
}

//MAIN MENU
void init_menu_options() {
	main_menu = new_menu("MAIN", 8, (vec2){160,120});
	m_set_size(main_menu, 320, 220);
	m_set_draw(main_menu, M_DEFAULT);
	main_menu->cur_color = 0x05A0;
	option_t opts[8];

	opts[0] = (option_t) {"Presets", 	NULL,    0};	//SUBMENU (PRESETS MENU)
	opts[1] = (option_t) {"Times", 		NULL, 	 0};	//SUBMENU (TIMERS MENU)
	opts[2] = (option_t) {"PID",  		NULL,  	 0};	//SUBMENU (PID MENU)
	opts[3] = (option_t) {"Pressures", 	NULL,    0};	//SUBMENU (PRESSURES MENU)
	opts[4] = (option_t) {"Alarms", 	NULL,    0};	//SUBMENU (ALARM CONFIG)
	opts[5] = (option_t) {"Reclaimer", 	NULL,    0};	//SUBMENU (RECLAIMER MENU)
	opts[6] = (option_t) {"BACK", 		NULL,    0};	//Not necessary on MAIN
	opts[7] = (option_t) {"EXIT", 		NULL,    0};	//Not necessary on MAIN
	
	//m_set_options(main_menu, main_menu->nopts, opts);
}

//PRESETS MENU
void init_presets_options() {
	presets = new_menu("PRESETS", 8, (vec2){160,120});
	m_set_size(presets, 320, 220);
	m_set_draw(presets, M_DEFAULT);
	presets->cur_color = 0x05A0;
	option_t opts[8];

	opts[0] = (option_t) {"Save Preset", 	NULL,    0};	//SUBMENU, FUNCTION
	opts[1] = (option_t) {"Load Preset", 	NULL, 	 0};	//SUBMENU, FUNCTION
	opts[2] = (option_t) {"Delete Preset",  NULL,  	 0};	//SUBMENU, FUNCTION
	opts[3] = (option_t) {"", 		NULL,    0};
	opts[4] = (option_t) {"", 		NULL,    0};
	opts[5] = (option_t) {"", 		NULL,    0};
	opts[6] = (option_t) {"BACK", 		NULL,    0};	//To MAIN
	opts[7] = (option_t) {"EXIT", 		NULL,    0};	//To MAIN
	
	//m_set_options(presets, presets->nopts, opts);
}

//PRESETS SUBMENU
void init_pick_presets_options() {
	pick_preset = new_menu("SELECT PRESET", 8, (vec2){160,120});
	m_set_size(pick_preset, 320, 220);
	m_set_draw(pick_preset, M_DEFAULT);
	pick_preset->cur_color = 0x05A0;
	option_t opts[8];

	opts[0] = (option_t) {"Preset 1", 	NULL,    0};	//FUNCTION
	opts[1] = (option_t) {"Preset 2", 	NULL, 	 0};	//FUNCTION
	opts[2] = (option_t) {"Preset 3",  	NULL,  	 0};	//FUNCTION
	opts[3] = (option_t) {"Preset 4", 	NULL,    0};	//FUNCTION
	opts[4] = (option_t) {"Preset 5", 	NULL,    0};	//FUNCTION
	opts[5] = (option_t) {"Preset 6", 	NULL,    0};	//FUNCTION
	opts[6] = (option_t) {"BACK", 		NULL,    0};	//To PRESETS
	opts[7] = (option_t) {"EXIT", 		NULL,    0};	//To MAIN
	
	//m_set_options(pick_preset, pick_preset->nopts, opts);
}

//PRESSURES MENU
void init_pressures_options() {
	pressures = new_menu("SET PRESSURES", 8, (vec2){160,120});
	m_set_size(pressures, 320, 220);
	m_set_draw(pressures, M_DEFAULT);
	pressures->cur_color = 0x05A0;
	option_t opts[8];

	opts[0] = (option_t) {"Set MARX", 	NULL,    0};	//FUNCTION
	opts[1] = (option_t) {"Set MARX TG70", 	NULL, 	 0};	//FUNCTION
	opts[2] = (option_t) {"Set MTG",  	NULL,  	 0};	//FUNCTION
	opts[3] = (option_t) {"Set SWITCH", 	NULL,    0};	//FUNCTION
	opts[4] = (option_t) {"Set SWITCH TG70", NULL,    0};	//FUNCTION
	opts[5] = (option_t) {"", 		NULL,    0};
	opts[6] = (option_t) {"BACK", 		NULL,    0};	//To MAIN
	opts[7] = (option_t) {"EXIT", 		NULL,    0};	//To MAIN
	
	//m_set_options(pressures, pressures->nopts, opts);
}

//RECLAIMER MENU
void init_reclaimer_options() {
	reclaimer_config = new_menu("RECLAIMER CONFIG", 8, (vec2){160,120});
	m_set_size(reclaimer_config, 320, 220);
	m_set_draw(reclaimer_config, M_DEFAULT);
	reclaimer_config->cur_color = 0x05A0;
	option_t opts[8];

	opts[0] = (option_t) {"Set RECL. ON", 	NULL,    0};	//FUNCTION
	opts[1] = (option_t) {"Set RECL. OFF", 	NULL,    0};	//FUNCTION
	opts[2] = (option_t) {"Set MIN SUPPLY", NULL,    0};	//FUNCTION
	opts[3] = (option_t) {"Set Safety Delay", NULL,    0};	//FUNCTION
	opts[4] = (option_t) {"", 		NULL,    0};
	opts[5] = (option_t) {"BACK", 		NULL,    0};	//To MAIN
	opts[6] = (option_t) {"EXIT", 		NULL,    0};	//To MAIN
	
	//m_set_options(reclaimer_config, reclaimer_config->nopts, opts);
}


//ALARM MENU
void init_alarms_options() {
	alarms = new_menu("ALARM CONFIG", 8, (vec2){160,120});
	m_set_size(alarms, 320, 220);
	m_set_draw(alarms, M_DEFAULT);
	alarms->cur_color = 0x05A0;
	option_t opts[8];

	opts[0] = (option_t) {"Sound ON/OFF", 	NULL,    0};	//FUNCTION
	opts[1] = (option_t) {"MARX Alarm", 	NULL,    0};	//FUNCTION
	opts[2] = (option_t) {"MARX TG70 Alarm", NULL, 	 0};	//FUNCTION
	opts[3] = (option_t) {"MTG Alarm",  	NULL,  	 0};	//FUNCTION
	opts[4] = (option_t) {"SWITCH Alarm", 	NULL,    0};	//FUNCTION
	opts[5] = (option_t) {"SWITCH TG70 Alarm", NULL,    0};	//FUNCTION
	opts[6] = (option_t) {"BACK", 		NULL,    0};	//To MAIN
	opts[7] = (option_t) {"EXIT", 		NULL,    0};	//To MAIN
	
	//m_set_options(alarms, alarms->nopts, opts);
}

//TIMERS MENU
void init_timers_options() {
	timers = new_menu("TIMERS CONFIG", 8, (vec2){160,120});
	m_set_size(timers, 320, 220);
	m_set_draw(timers, M_DEFAULT);
	timers->cur_color = 0x05A0;
	option_t opts[8];

	opts[0] = (option_t) {"Set Purge Times", NULL,   0};	//SUBMENU (TIMERS MENU)
	opts[1] = (option_t) {"Set Circuit Delay", NULL, 0};	//SUBMENU (TIMERS MENU)
	opts[2] = (option_t) {"", 		NULL,  	 0};
	opts[3] = (option_t) {"", 		NULL,    0};
	opts[4] = (option_t) {"", 		NULL,    0};
	opts[5] = (option_t) {"", 		NULL,    0};
	opts[6] = (option_t) {"BACK", 		NULL,    0};	//To MAIN
	opts[7] = (option_t) {"EXIT", 		NULL,    0};	//To MAIN
	
	//m_set_options(timers, timers->nopts, opts);
}

//PURGE TIMERS SUBMENU
void init_purge_timers_options() {
	timers = new_menu("SET PURGE TIMES", 8, (vec2){160,120});
	m_set_size(timers, 320, 220);
	m_set_draw(timers, M_DEFAULT);
	timers->cur_color = 0x05A0;
	option_t opts[8];

	opts[0] = (option_t) {"MARX Purge", 	NULL,    0};	//FUNCTION
	opts[1] = (option_t) {"MARX TG70 Purge", NULL, 	 0};	//FUNCTION
	opts[2] = (option_t) {"MTG Purge",  	NULL,  	 0};	//FUNCTION
	opts[3] = (option_t) {"SWITCH Purge", 	NULL,    0};	//FUNCTION
	opts[4] = (option_t) {"SWITCH TG70 Purge", NULL,    0};	//FUNCTION
	opts[5] = (option_t) {"",		NULL,    0};
	opts[6] = (option_t) {"BACK", 		NULL,    0};	//To TIMERS 
	opts[7] = (option_t) {"EXIT", 		NULL,    0};	//To MAIN
	
	//m_set_options(timers, timers->nopts, opts);
}

//CIRCUIT DELAY TIMERS SUBMENU
void init_delay_timers_options() {
	delay_timers = new_menu("SET CIRCUIT DELAY", 8, (vec2){160,120});
	m_set_size(delay_timers, 320, 220);
	m_set_draw(delay_timers, M_DEFAULT);
	delay_timers->cur_color = 0x05A0;
	option_t opts[8];

	opts[0] = (option_t) {"MARX Delay", 	NULL,    0};	//FUNCTION
	opts[1] = (option_t) {"MARX TG70 Delay", NULL, 	 0};	//FUNCTION
	opts[2] = (option_t) {"MTG Delay",  	NULL,  	 0};	//FUNCTION
	opts[3] = (option_t) {"SWITCH Delay", 	NULL,    0};	//FUNCTION
	opts[4] = (option_t) {"SWITCH TG70 Delay", NULL,    0};	//FUNCTION
	opts[5] = (option_t) {"",		NULL,    0};
	opts[6] = (option_t) {"BACK", 		NULL,    0};	//To TIMERS 
	opts[7] = (option_t) {"EXIT", 		NULL,    0};	//To MAIN
	
	//m_set_options(delay_timers, delay_timers->nopts, opts);
}

//PID MENU
void init_pid_options() {
	pick_pid = new_menu("PID CONFIG", 8, (vec2){160,120});
	m_set_size(pick_pid, 320, 220);
	m_set_draw(pick_pid, M_DEFAULT);
	pick_pid->cur_color = 0x05A0;
	option_t opts[8];

	opts[0] = (option_t) {"MARX PID", 	NULL,    0};	//FUNCTION
	opts[1] = (option_t) {"MARX TG70 PID", NULL, 	 0};	//FUNCTION
	opts[2] = (option_t) {"MTG PID",  	NULL,  	 0};	//FUNCTION
	opts[3] = (option_t) {"SWITCH PID", 	NULL,    0};	//FUNCTION
	opts[4] = (option_t) {"SWITCH TG70 PID", NULL,    0};	//FUNCTION
	opts[5] = (option_t) {"", 		NULL,    0};
	opts[6] = (option_t) {"BACK", 		NULL,    0};	//To MAIN
	opts[7] = (option_t) {"EXIT", 		NULL,    0};	//To MAIN
	
	//m_set_options(pick_pid, pick_pid->nopts, opts);
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

void loop()
{
  TSPoint p = ts.getPoint();
  p.x = map(p.y, TS_MINY, TS_MAXY, 0, tft.width());
  p.y = map(p.x, TS_MINX, TS_MAXX, 0, tft.height());
  int idx = m_test_touch(p, main_menu);
  Serial.print("( "); Serial.print(p.x); Serial.print(", "); 
  Serial.print(p.y); Serial.print(" )  \n");
  if (idx >= 0) {
	Serial.println(main_menu->options[idx].name);
  }
}


