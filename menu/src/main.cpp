#include <Arduino.h>
#include <stdlib.h>
#include <inttypes.h>

#include <PID_v1.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <TouchScreen.h>
#include <gfxfont.h>
#include <TM1637Display.h>
#include "menu.h"

#include <avr8-stub.h>

/*
CIRCUIT
outpin
in
analog pin
analog out (?)
enable pin
enable led pin
kp,ki,kd
enable state (??)
prev enable state (??)

max time
delay
check time
purge time
set point
*/

#define C_NUM_IO 5
#define I_PRESSURE_READ 0
#define I_PRESSURE_IN   1
#define I_PRESSURE_OUT  2
#define I_ENABLE        3
#define I_LED           4

#define C_MAX_DEFAULT   	5000
#define C_DELAY_DEFAULT		1000
#define C_PURGE_DEFAULT		120000
#define C_RECLAIM_TIME		30000

// system circuit indices
#define C_NUM_CIRCUITS 1
#define C_MARX        0
#define C_MTG70       1
#define C_MTG         2
#define C_SWITCH      3
#define C_SWTG70      4
#define C_RECLAIMER   5
#define C_BOTTLE      6

// system parameter indices
#define P_SET_POINT   0
#define P_MAX_TIME    1
#define P_CHECK_TIME  2
#define P_PURGE_TIME  3
#define P_DELAY_TIME  4
#define P_KP          5
#define P_KI          6
#define P_KD          7

#define NUM_PRESETS 6
#define SAVE 		0
#define LOAD 		1
#define DEL  		2

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

#define IN_RANGE(v, min, max) ((v >= min && v <= max))

#define S_SHOT    1
#define S_ABORT   (1 << 1)
#define S_PURGE   (1 << 2)
#define S_ALARM   (1 << 3)
#define S_RECLAIM (1 << 4)
#define S_STANDBY (1 << 5)

/*

reclaim start/stop pin (relays)
shotmode purge  alarm  automate reclaimer  abort  pins (buttons)
alarm sound pin (buzzer/speaker)
led pins (alarm, abort, shotmode, purge, start/stop, auto reclaim)
serial sd card pins

* menu stuff *

alarm en (bool)
error state (bool)
prev time
min bottle pressure
standby mode (bool)
auto mode (bool)
reclaim running (bool)
reclaim safety time
prev safety time
min reclaim pressure
max reclaim pressure
is couple (?)

BUTTON STATES
alarm state
auto reclaim state
purge state
shot state
reclaim start/stop state
abort state

* previous ^ state vars *

*/

typedef struct circuit_t {
  bool en, prev_en, set;
  double params[8];
	
  // double set_point;
	
	// uint32_t max_time;
	// uint32_t check_time;
	// uint32_t purge_time;
  // uint32_t delay;

  // double kp, ki, kd;
	
  double pressure;
  double roc;

  uint8_t pins[C_NUM_IO];
} circuit_t;

typedef struct system_t {
  /*
    stores system state/mode
      shot
      abort
      purge
      alarm
      reclaim
      standby
  */
  uint8_t s_flags;

  /*
    stores set-context (active edit circuit) (modified in circuit_select cb)
      marx
      mtg
      switch
      swtg70
      mxtg70
      reclaimer
      bottle
  */
  uint8_t c_flags;
  
  /*
    stores parameter-context (active set param(s)) (modified in pick_param cb)
      set point
      max time
      check time
      purge time
      delay
      kp
      ki
      kd
  */
  uint8_t p_flags;

  circuit_t circuits[C_NUM_CIRCUITS];

  int pid_window_size;
} system_t;

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

struct ui { 
	menu_t *active;
	menu_t *previous;

	int pidx;
	menu_t *path[8];
} ui;

TouchScreen ts = TouchScreen(YPOS, XPOS, YMIN, XMIN, 300);
Adafruit_ILI9341 tft = Adafruit_ILI9341(tft8bitbus, 22, 35, 36, 37, 33, 34);
TM1637Display pdisp(10, 11);

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

	return M_SELECT;
}

int pick_preset_cb(menu_t *m, option_t *o) {
	int code = M_NOP;
	TSPoint p;

	strcat(alert->title, o->name);
	
	// click();
	m_draw(&tft, m, M_CLEAR);		// clear preset selection menu
	// click();
	m_draw(&tft, alert, M_DRAW);	// display alert (confirm/cancel)
	while (code == M_NOP) {			// wait for selection
		p = get_press(&ts);
		if (p.z > 50) code = m_interact(alert, p);
	}
	// click();
	m_draw(&tft, alert, M_CLEAR);	// clear alert
	// click();

	if (code == M_CONFIRM) {
		switch (o->value) {
			case SAVE:		// save preset
				// compute checksum
				// test save success
				sprintf(popup->title, "%s success.", alert->title);
				m_draw(&tft, popup, M_DRAW);
				_delay_ms(M_POPDELAY);
				m_draw(&tft, popup, M_CLEAR);
				break;
			case LOAD:		// load preset
				sprintf(popup->title, "%s failed!", alert->title);
				m_draw(&tft, popup, M_DRAW);
				_delay_ms(M_POPDELAY);
				m_draw(&tft, popup, M_CLEAR);
				break;
			case DEL:		// delete preset
				sprintf(popup->title, "%s is done.", alert->title);
				m_draw(&tft, popup, M_DRAW);
				_delay_ms(M_POPDELAY);
				m_draw(&tft, popup, M_CLEAR);
				break;
			default:
				break;
		}
		popup->title[0] = '\0';

		return M_CONFIRM;
	}

	return M_BACK;
}

int alarms_cb(menu_t *m, option_t *o) {
	if (m->cursor == 0) {		// selected SOUND option
		int code = M_NOP, dir;
		TSPoint p;
		
		// click();
		if (o->value == 0) {  			// off, turn on
			strcpy(alert->title, "Set Sound ON?");
			dir = 1;
		} else if (o->value == 1) { 	// on, turn off
			strcpy(alert->title, "Set Sound OFF?");
			dir = -1;
		}
		
		m_draw(&tft, m, M_CLEAR);		// clear preset selection menu
		// click();
		m_draw(&tft, alert, M_DRAW);	// display alert (confirm/cancel)
		while (code == M_NOP) {			// wait for selection
			p = get_press(&ts);
			if (p.z > 100) code = m_interact(alert, p);
		}
		m_draw(&tft, alert, M_CLEAR);	// clear alert

		if (code == M_CONFIRM) {
			o->value += dir;

			sprintf(popup->title, "Sound is %s.", (o->value) ? "ON" : "OFF");
			m_draw(&tft, popup, M_DRAW);
			_delay_ms(M_POPDELAY);
			m_draw(&tft, popup, M_CLEAR);
			return M_CONFIRM;
		}
	}

	return M_SELECT;
}

int set_param_cb(menu_t *m, option_t *o) {
  if (sys.c_flags && sys.p_flags) {  // if circuit was selected to be modified
    for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {
      if (1 & sys.c_flags) {
        for (int k = 0; k < 8; k += 1) {
          if (1 & sys.p_flags) {
            // need floating point + precision info when storing parameter value
            sys.circuits[i].params[k] = o->value;
            if (m->flags & M_FPARAM) {
              // could use a precision modifier for divisor?
              sys.circuits[i].params[k] /= 100;
            }
          }
          sys.p_flags >>= 1;
        }
      }
      sys.c_flags >>= 1;
    }
  } else {
    // reclaimer and min supply stuff (only if they can't be treated the same as normal circuits?)
  }

  return M_BACK;
}

int pick_pid_cb(menu_t *m, option_t *o) {
  sys.p_flags |= (1 << (P_KP + m->cursor));
  o->target->flags |= M_FPARAM;

  return M_SELECT;
}

int timers_cb(menu_t *m, option_t *o) {
  sys.p_flags |= (1 << (P_PURGE_TIME + m->cursor));
  o->target->flags &= ~M_FPARAM;

  return M_SELECT;
}

int main_cb(menu_t *m, option_t *o) {
  if (m->cursor == 3) {   // pressures
    sys.p_flags |= (1 << P_SET_POINT);
    set_param->flags |= M_FPARAM;
  }

  return M_SELECT;
}

int circuit_select_cb(menu_t *m, option_t *o) {
  sys.c_flags |= (1 << m->cursor);

  return M_SELECT;
}

int mode_cb(menu_t *m, option_t *o) {
  int code;
  TSPoint p;

  sprintf(alert->title, "%s mode?", o->name);

  m_draw(&tft, m, M_CLEAR);


  // this shit is very similar to how we handle sound on/off process
  // maybe push popups into alert callback and modify m_interact_msg ?
  m_draw(&tft, alert, M_DRAW);
  while (code == M_NOP) {
    p = get_press(&ts);
    if (p.z > 100) code = m_interact(alert, p);
  }
  m_draw(&tft, alert, M_CLEAR);

  if (code == M_CONFIRM) {
    sys.s_flags = 1 << m->cursor;

    sprintf(popup->title, "Entered %s mode", o->name);
    m_draw(&tft, popup, M_DRAW);
    _delay_ms(M_POPDELAY);
    m_draw(&tft, popup, M_CLEAR);
    return M_CONFIRM;
  }

  return M_SELECT;
}

void create_menus() {
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

  alarms->cb = alarms_cb;
  presets->cb = preset_cb;
  main_menu->cb = main_cb;
	pick_preset->cb = pick_preset_cb;
  circuit_select->cb = circuit_select_cb;
  set_param->cb = set_param_cb;
  mode->cb = mode_cb;
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
	create_menus();
	init_options();
}

void init_system() {
  sys.s_flags = S_SHOT;

  // load default circuit settings
  sys.circuits[C_MARX] = (circuit_t) {
    true, false, false,       // enable, prev enable, is_set
    {
      13.0,                   // set point
      C_MAX_DEFAULT,          // max time
      0,                      // check time
      C_PURGE_DEFAULT,        // purge time
      C_DELAY_DEFAULT,        // delay time
      50, 0, 25              // PID tuning params
    },
    0,                        // pressure
    0.01,                     // pressure rate-of-change
    { A7, 7, 8, 9, 6 }        // pins
  };

  /* sys.circuits[C_MTG] = (circuit_t) {
    15, 0, 10,              // PID tuning params
    false, false, false,    // enable, prev enable, is_set
    C_MAX_DEFAULT,          // max time
    C_DELAY_DEFAULT,        // delay time
    0,                      // check time
    C_PURGE_DEFAULT,        // purge time
    0,                      // set point
    { A2, 25, 24, 18, 39 }  // pins
  };

  sys.circuits[C_SWITCH] = (circuit_t) {
    50, 0, 25,              // PID tuning params
    false, false, false,    // enable, prev enable, is_set
    C_MAX_DEFAULT,          // max time
    C_DELAY_DEFAULT,        // delay time
    0,                      // check time
    C_PURGE_DEFAULT,        // purge time
    0,                      // set point
    { A1, 27, 26, 19, 40 }  // pins
  };

  sys.circuits[C_MTG70] = (circuit_t) {
    15, 0, 10,              // PID tuning params
    false, false, false,    // enable, prev enable, is_set
    C_MAX_DEFAULT,          // max time
    C_DELAY_DEFAULT,        // delay time
    0,                      // check time
    C_PURGE_DEFAULT,        // purge time
    0,                      // set point
    { A3, 31, 30, 21, 42 }  // pins
  };

  sys.circuits[C_SWTG70] = (circuit_t) {
    15, 0, 10,              // PID tuning params
    false, false, false,    // enable, prev enable, is_set
    C_MAX_DEFAULT,          // max time
    C_DELAY_DEFAULT,        // delay time
    0,                      // check time
    C_PURGE_DEFAULT,        // purge time
    0,                      // set point
    { A4, 29, 28, 20, 41 }  // pins
  }; */
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

void simulate_lines() {
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
      if (ro) {
        sys.circuits[i].pressure -= sys.circuits[i].roc + ((double)(rand() % 10) / 100);
      }
    }

    // if (sys.circuits[i].en) {
    //   digitalWrite(sys.circuits[i].pins[I_ENABLE], HIGH);
    // } else {
    //   digitalWrite(sys.circuits[i].pins[I_ENABLE], LOW);
    // }
  }
  
  pdisp.showNumberDecEx(sys.circuits[0].pressure, 0b01000000, false, 2, 0);
  pdisp.showNumberDecEx((sys.circuits[0].pressure - (int)sys.circuits[0].pressure) * 100, 0b01000000, true, 2, 2);
}

void purge() {
  unsigned long int itime = millis();
  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {    // loop system circuits
    itime = millis();
    if (sys.circuits[i].en) {
      // open exhaust, close intake
      digitalWrite(sys.circuits[i].pins[I_PRESSURE_IN], LOW);
      digitalWrite(sys.circuits[i].pins[I_PRESSURE_OUT], HIGH);
      while (millis() - itime <= sys.circuits[i].params[P_PURGE_TIME] && sys.circuits[i].pressure > 0) { simulate_lines(); }    // loop to purge timer expiration
      // close exhaust
      digitalWrite(sys.circuits[i].pins[I_PRESSURE_OUT], LOW);
    }    
  }

  sys.s_flags = S_STANDBY;
}

void set_pressure(circuit_t *c, double var, int half) {
  unsigned long t;
  double out;
  double set_point = (half) ? c->params[P_SET_POINT] / 2 : c->params[P_SET_POINT];
  int dir;
  PID pressure(&c->pressure, &out, &set_point, c->params[P_KP], c->params[P_KI], c->params[P_KP], 0);

  pressure.SetOutputLimits(0, sys.pid_window_size);
  pressure.SetMode(AUTOMATIC);

  unsigned long wstart = millis();    // base time offset
  while (!IN_RANGE(c->pressure, (set_point - var), (set_point + var))) {
    /*
      can check UI at the top of this loop if we want the UI to be responsive
      while pressures are being set. will require some system state locking
      to ensure safe mode transitions.
    */
    t = millis();

    // pick PID direction (inc or dec)
    dir = (c->pressure > set_point + var) ? REVERSE : DIRECT;
    pressure.SetControllerDirection(dir);
    pressure.Compute();   // compute solenoid PID-window (stored @ out)
    // pread = analogRead(c->pins[I_PRESSURE_READ]);   // get circuit pressure @ inlet

    if (t - wstart >= c->params[P_MAX_TIME]) {   // set pressure timeout (possible leak or obstruction)
      // close intake
      digitalWrite(c->pins[I_PRESSURE_IN], LOW);
      // close exhaust
      digitalWrite(c->pins[I_PRESSURE_OUT], LOW);
      
      // set error state

      break;
    }

    if (out > t - wstart) {    // time remains in PID-window
      if (dir == REVERSE) {         // exhaust
        digitalWrite(c->pins[I_PRESSURE_OUT], HIGH);
        digitalWrite(c->pins[I_PRESSURE_IN], LOW);
      } else if (dir == DIRECT) {   // intake
        digitalWrite(c->pins[I_PRESSURE_OUT], LOW);
        digitalWrite(c->pins[I_PRESSURE_IN], HIGH);
      }
    } else {                   // PID-window expired & close intake/exhaust
      digitalWrite((dir == REVERSE) ? c->pins[I_PRESSURE_OUT] : c->pins[I_PRESSURE_IN], LOW);
      break;
    }

    simulate_lines();
  }

  // close circuit solenoids
  digitalWrite(c->pins[I_PRESSURE_IN], LOW);
  digitalWrite(c->pins[I_PRESSURE_OUT], LOW);
}

void shot_pressure(bool half) {
  double var = 0.05;
  circuit_t *c;

  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {  // loop system circuits
    if (sys.circuits[i].en) {   // if enabled
      c = &sys.circuits[i];
      if (millis() - c->params[P_CHECK_TIME] >= c->params[P_DELAY_TIME]) {   // check time expired
        set_pressure(c, var, half);
        c->params[P_CHECK_TIME] = millis();
      }
    }
  }
}

void init_io() {
  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {
    pinMode(sys.circuits[i].pins[I_PRESSURE_IN], OUTPUT);
    pinMode(sys.circuits[i].pins[I_PRESSURE_OUT], OUTPUT);
    pinMode(sys.circuits[i].pins[I_ENABLE], OUTPUT);
    pinMode(sys.circuits[i].pins[I_PRESSURE_READ], INPUT);
  }
}

void setup() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  init_menus();
  init_system();
  init_io();

  pdisp.setBrightness(3);
  pdisp.clear();

  sys.pid_window_size = 5000;
  sys.s_flags = S_STANDBY;

  ui.active = main_menu;
  ui.previous = NULL;
  ui.path[0] = ui.active;
  ui.pidx = 0;

  m_draw(&tft, ui.active, 0);
}

void loop() {
  update_ui();
  if (sys.s_flags & S_SHOT) {           // continuously checks pressures, keeping within setpoint range
    shot_pressure(false);
  } else if (sys.s_flags & S_PURGE) {   // bleed all circuits to 0 pressure
    purge();
  } else if (sys.s_flags & S_ABORT) {   // reduce all circuits to half-pressure
    shot_pressure(true);
  }

  digitalWrite(sys.circuits[0].pins[I_ENABLE], sys.s_flags & S_STANDBY);
}
