#include <Arduino.h>
#include <stdlib.h>
#include <inttypes.h>

#include <PID_v1.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <TouchScreen.h>
#include <gfxfont.h>
#include "menu.h"

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

#define CIRCUIT_IO 5
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
#define C_NUM_CIRCUITS 7
#define C_MARX        0
#define C_MTG         1
#define C_SWITCH      2
#define C_MTG70       3
#define C_SWTG70      4
#define C_RECLAIMER   5
#define C_BOTTLE      6

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
#define S_AUTOREC (1 << 4)
#define S_RECLAIM (1 << 5)

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

typedef struct io_t {
  const uint8_t flags;
  const uint8_t *port;
  const uint8_t *ddir;
  const uint8_t *pinv;
} io_t;

typedef struct circuit_t {
	double kp, ki, kd;
	bool en, prev_en, set;
	uint32_t max_time;
	uint32_t delay;
	uint32_t check_time;
	uint32_t purge_time;

	double set_point;

  uint8_t pins[CIRCUIT_IO];
} circuit_t;

typedef struct system_t {
  /*
    stores system state/mode
      alarm state
      auto reclaim state
      purge state
      shot mode
      reclaimer state (start/stop)
      abort state

      bool alarmState = false;
      bool automatereclaimerState = true;
      bool purgeState = false;
      bool shotmodeState = false;
      bool startreclaimerState = false;
      bool stopreclaimerState = false;
      bool abortState = false;
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
      pressure set
      pressure min
      pressure max
      timeout
      delay time
      check time
      purge time
  */
  uint8_t p_flags;

  /*
    button io state & pins
      alarm
      auto reclaim
      purge
      shot mode
      abort
      reclaimer start/stop
  */
  int buttons[6];

  /*
    solenoids/relays (in/out ?)
      marx
      mtg
      switch
      swtg70
      mxtg70
      reclaim start
      reclaim stop
    in-pressure data (analog) (+ out?)
      marx
      mtg
      switch
      swtg70
      mxtg70
      reclaimer
      bottle
  */
  // io_t leds;

  int ncirc;
  circuit_t *circuits;

  int pid_window_size;
} system_t;

system_t sys;
circuit_t marx, mtg, sw, swtg70, marxtg70;
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
		// // click();
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
			m_draw(&tft, popup, M_DRAW);
			_delay_ms(M_POPDELAY);
			m_draw(&tft, popup, M_CLEAR);
			return M_CONFIRM;
		}
	}

	return M_SELECT;
}

void create_menus() {
	alert = new_menu("", 0, CENTER, M_SIZE, M_MESSAGE, 0);
	popup = new_menu("", 0, CENTER, M_SIZE, M_POPUP, M_NOTITLE);
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

void init_menus() {
	create_menus();
	init_options();
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

void init_circuits() {
  sys.circuits = (circuit_t*) malloc(sizeof(circuit_t) * C_NUM_CIRCUITS);

  // load default circuit settings
  sys.circuits[C_MARX] = (circuit_t) {
    50, 0, 25,              // PID tuning params
    false, false, false,    // enable, prev enable, is_set
    C_MAX_DEFAULT,          // max time
    C_DELAY_DEFAULT,        // delay time
    0,                      // check time
    C_PURGE_DEFAULT,        // purge time
    0,                      // set point
    { A0, 23, 22, 17, 38 }  // pins
  };

  sys.circuits[C_MTG] = (circuit_t) {
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
  };
}

void setup() {
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
  update_ui();
  if (sys.s_flags & S_SHOT) {   // continuously checks pressures, keeping within setpoint range
    shot_pressure(false);
  } else if (sys.s_flags & S_PURGE) {
    purge();
  } else if (sys.s_flags & S_ABORT) {
    shot_pressure(true);
  }
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

void purge() {
  unsigned long int itime;
  for (int i = 0; i < sys.ncirc; i += 1) {    // loop system circuits
    itime = millis();
    if (sys.circuits[i].en) {
      // open intake and exhaust
      digitalWrite(sys.circuits[i].pins[I_PRESSURE_IN], HIGH);
      digitalWrite(sys.circuits[i].pins[I_PRESSURE_OUT], HIGH);
      while (millis() - itime >= sys.circuits[i].purge_time) { }    // loop to purge timer expiration
      // close intake and exhaust
      digitalWrite(sys.circuits[i].pins[I_PRESSURE_IN], LOW);
      digitalWrite(sys.circuits[i].pins[I_PRESSURE_OUT], LOW);
    }    
  }
}

void set_pressure(circuit_t *c, int var, int half) {
  unsigned long int t;
  double pread = analogRead(c->pins[I_PRESSURE_READ]), out;
  double set_point = (half) ? c->set_point / 2 : c->set_point;
  int dir = (pread > set_point + var) ? REVERSE : DIRECT;
  PID pressure(&pread, &out, &set_point, c->kp, c->ki, c->kd, dir);
  pressure.SetOutputLimits(0, sys.pid_window_size);

  unsigned long wstart = millis();    // base time offset
  while (!IN_RANGE(pread, set_point - var, set_point + var)) {
    /*
      can check UI at the top of this loop if we want the UI to be responsive
      while pressures are being set. will require some system state locking
      to ensure safe mode transitions.
    */
    t = millis();
    pread = analogRead(c->pins[I_PRESSURE_READ]);   // get circuit pressure @ inlet
    pressure.Compute();   // compute solenoid PID-window (stored @ out)

    // adjust initial window offset (?)
    if (t - wstart > sys.pid_window_size) wstart += sys.pid_window_size;

    if (t - wstart >= c->max_time) {   // set pressure timeout (possible leak or obstruction)
      // close gas intake
      digitalWrite(c->pins[I_PRESSURE_IN], LOW);
      // close gas exhaust (?)
      digitalWrite(c->pins[I_PRESSURE_OUT], LOW);
      
      // set error state

      break;
    }

    if (out > t - wstart) {    // time remains in PID-window & open intake/exhaust
      digitalWrite((dir == DIRECT) ? c->pins[I_PRESSURE_OUT] : c->pins[I_PRESSURE_IN], HIGH);
    } else {                          // PID-window expired & close intake/exhaust
      digitalWrite((dir == DIRECT) ? c->pins[I_PRESSURE_OUT] : c->pins[I_PRESSURE_IN], LOW);
      break;
    }
  }

  // close circuit solenoids
  digitalWrite(c->pins[I_PRESSURE_IN], LOW);
  digitalWrite(c->pins[I_PRESSURE_OUT], LOW);
}

void shot_pressure(bool half) {
  int var = 5;
  circuit_t *c;

  for (int i = 0; i < sys.ncirc; i += 1) {  // loop system circuits
    if (sys.circuits[i].en) {   // if enabled
      c = &sys.circuits[i];
      if (millis() - c->check_time >= c->delay) {   // check time expired
        set_pressure(c, var, half);
        c->check_time = millis();
      }
    }
  }
}
