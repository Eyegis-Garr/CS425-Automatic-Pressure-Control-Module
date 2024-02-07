#include "simulator.h"

system_t sys;

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

TouchScreen ts(YPOS, XPOS, YMIN, XMIN, 300);
Adafruit_ILI9341 tft(tft8bitbus, 22, 35, 36, 37, 33, 34);
TM1637Display pdisp(10, 11);

struct ui { 
	menu_t *active;
	menu_t *previous;

	int pidx;
	menu_t *path[8];
} ui;

void sim_setup() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  init_menus();
  init_system();
  init_io();

  pdisp.setBrightness(3);
  pdisp.clear();

  sys.pid_window_size = 5000;

  ui.active = main_menu;
  ui.previous = NULL;
  ui.path[0] = ui.active;
  ui.pidx = 0;

  m_draw(&tft, ui.active, 0);
}

void sim_tick() {
  update_ui();
  if (sys.s_flags & S_SHOT) {           // continuously checks pressures, keeping within setpoint range
    shot_pressure(false);
  } else if (sys.s_flags & S_PURGE) {   // bleed all circuits to 0 pressure
    purge();
  } else if (sys.s_flags & S_ABORT) {   // reduce all circuits to half-pressure
    shot_pressure(true);
  }

  digitalWrite(sys.circuits[0].pins[I_ENABLE_BTN], sys.s_flags & S_STANDBY);
}

void init_options() {
	option_t opts[8];

	// SET MENU
	opts[0] = (option_t) {"", 		NULL,    1234};
	m_set_options(set_param, set_param->nopts, opts);
	
	// MAIN MENU
	opts[0] = (option_t) {"Presets", 	  presets,          0};
	opts[1] = (option_t) {"Times", 		  timers,           0};
	opts[2] = (option_t) {"PID",  		  pick_pid,         0};
	opts[3] = (option_t) {"Pressures", 	circuit_select,   0};
	opts[4] = (option_t) {"Alarms", 	  alarms,           0};
	opts[5] = (option_t) {"Reclaimer", 	reclaimer_config, 0};
  opts[6] = (option_t) {"Mode", 	mode, 0};
	m_set_options(main_menu, main_menu->nopts, opts);

  opts[0] = (option_t) {"SHOT",     main_menu, 0};
	opts[1] = (option_t) {"ABORT", 		main_menu, 0};
	opts[2] = (option_t) {"PURGE",  	main_menu, 0};
	opts[3] = (option_t) {"ALARM", 	  main_menu, 0};
	opts[4] = (option_t) {"RECLAIM", 	main_menu, 0};
  opts[5] = (option_t) {"STANDBY", 	main_menu, 0};
	m_set_options(mode, mode->nopts, opts);

	// PRESETS
	opts[0] = (option_t) {"Save",	  pick_preset,	0};
	opts[1] = (option_t) {"Load",	  pick_preset,	0};
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
	alert = new_menu("", 0, CENTER, M_SIZE, M_MESSAGE, 0);
	popup = new_menu("", 0, CENTER, M_SIZE, M_POPUP, M_NOTITLE);
	set_param = new_menu("SET PARAMETER", 1, CENTER, M_SIZE, M_SET, 0);

	main_menu = new_menu("MAIN", 7, CENTER, M_SIZE, M_DEFAULT, M_NOBACK | M_NOEXIT);
  mode = new_menu("MODE", 6, CENTER, M_SIZE, M_DEFAULT, 0);
	reclaimer_config = new_menu("RECLAIMER CONFIG", 4, CENTER, M_SIZE, M_DEFAULT, 0);
	timers = new_menu("TIMERS", 2, CENTER, M_SIZE, M_DEFAULT, 0);
	pick_pid = new_menu("PICK PID", 3, CENTER, M_SIZE, M_DEFAULT, 0);
	circuit_select = new_menu("PICK CIRCUIT", 7, CENTER, M_SIZE, M_DEFAULT, 0);
	alarms = new_menu("ALARMS", 6, CENTER, M_SIZE, M_DEFAULT, 0);
	presets = new_menu("PRESETS", 3, CENTER, M_SIZE, M_DEFAULT, 0);
	pick_preset = new_menu("PICK PRESET", NUM_PRESETS, CENTER, M_SIZE, M_DEFAULT, 0);

  setup_callbacks();

	init_options();
}

void init_system() {
  sys.s_flags = S_SHOT;
  sys.c_flags = 0;
  sys.p_flags = 0;
  sys.en_flags = (1 << C_MARX);

  // load default circuit settings
  sys.circuits[C_MARX] = (circuit_t) {
    {
      25.75,                   // set point
      C_MAX_DEFAULT,          // max time
      0,                      // check time
      C_PURGE_DEFAULT,        // purge time
      C_DELAY_DEFAULT,        // delay time
      50, 0, 25              // PID tuning params
    },
    50,                        // pressure
    0.01,                     // pressure rate-of-change
    { A7, 7, 8, 9, 6 }        // pins
  };

  pid_t *p;
  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {
    p = &sys.circuits[i].pid;
    pid_set_input(p, &sys.circuits[i].pressure);
    pid_set_param(p, sys.circuits[i].params[P_KP], sys.circuits[i].params[P_KI], sys.circuits[i].params[P_KD]);
  }
}

void init_io() {
  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {
    pinMode(sys.circuits[i].pins[I_PRESSURE_IN], OUTPUT);
    pinMode(sys.circuits[i].pins[I_PRESSURE_OUT], OUTPUT);
    pinMode(sys.circuits[i].pins[I_ENABLE_BTN], OUTPUT);
    pinMode(sys.circuits[i].pins[I_PRESSURE_READ], INPUT);
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

void update_ui() {
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
        m_draw(&tft, ui.active, M_CLEAR);		// clear current menu
        ui.path[ui.pidx++] = ui.active;			// update path
        ui.active = ui.active->options[ui.active->cursor].target;	// swap active menu
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

void update_circuits() {
  int ri, ro;
  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {
    ri = digitalRead(sys.circuits[i].pins[I_PRESSURE_IN]);
    ro = digitalRead(sys.circuits[i].pins[I_PRESSURE_OUT]);

    // pressurized pipes are hardly as random when it comes to flow (probably)
    // can write a flow rate sampler to more accurately model flow
    randomSeed(millis());
    if (ri) {
      if (!ro) {
        sys.circuits[i].pressure += sys.circuits[i].roc + ((double)(rand() % 10) / 100);
      } else {
        sys.circuits[i].pressure -= sys.circuits[i].roc + ((double)(rand() % 10) / 100);
      }
    } else {
      if (ro && sys.circuits[i].pressure >= 0) {
        sys.circuits[i].pressure -= sys.circuits[i].roc + ((double)(rand() % 10) / 100);
      }
    }
  }
  
  pdisp.showNumberDecEx(sys.circuits[0].pressure, 0b01000000, false, 2, 0);
  pdisp.showNumberDecEx((sys.circuits[0].pressure - (int)sys.circuits[0].pressure) * 100, 0b01000000, true, 2, 2);
}

void purge() {
  uint32_t itime;
  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {    // loop system circuits
    itime = millis();
    if (sys.en_flags & (1 << i)) {
      // open exhaust, close intake
      digitalWrite(sys.circuits[i].pins[I_PRESSURE_IN], LOW);
      digitalWrite(sys.circuits[i].pins[I_PRESSURE_OUT], HIGH);
      while (millis() - itime <= sys.circuits[i].params[P_PURGE_TIME] && sys.circuits[i].pressure > 0) { update_circuits(); }    // loop to purge timer expiration
      // close exhaust
      digitalWrite(sys.circuits[i].pins[I_PRESSURE_OUT], LOW);
    }    
  }

  sys.s_flags = S_STANDBY;
}

void set_pressure(circuit_t *c, double var, int half) {
  uint32_t wstart;
  double out, set_point = (half) ? c->params[P_SET_POINT] / 2 : c->params[P_SET_POINT];
  int dir;

  pid_set_target(&c->pid, &set_point);
  
  if (!IN_RANGE(c->pressure, (set_point - var), (set_point + var))) {
    // pick PID direction
    dir = (c->pressure > set_point + var) ? REVERSE : DIRECT;
    pid_set_direction(&c->pid, dir);
    out = pid_compute(&c->pid);
    wstart = millis();    // base time offset
    while (millis() - wstart < out * 5) {
      

      digitalWrite(c->pins[I_PRESSURE_OUT], dir);
      digitalWrite(c->pins[I_PRESSURE_IN], dir ^ 1);

      update_circuits();
    }
  }

  // close circuit solenoids
  digitalWrite(c->pins[I_PRESSURE_IN], LOW);
  digitalWrite(c->pins[I_PRESSURE_OUT], LOW);
}

void shot_pressure(bool half) {
  double var = 0.05;
  circuit_t *c;

  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {  // loop system circuits
    if (sys.en_flags & (1 << i)) {   // if enabled
      c = &sys.circuits[i];
      if (millis() - c->params[P_CHECK_TIME] >= c->params[P_DELAY_TIME]) {   // check time expired
        set_pressure(c, var, half);
        c->params[P_CHECK_TIME] = millis();
      }
      // set_pressure(c, var, half);
    }
  }
}

