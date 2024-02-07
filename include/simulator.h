#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <PID_v1.h>
#include <Adafruit_ILI9341.h>
#include <TouchScreen.h>
#include <TM1637Display.h>
#include <avr8-stub.h>

#include "menu.h"
#include "callbacks.h"


// system circuit IO indices
#define C_NUM_IO 5
#define I_PRESSURE_READ 0
#define I_PRESSURE_IN   1
#define I_PRESSURE_OUT  2
#define I_ENABLE_BTN    3
#define I_LED           4

// system circuit defaults (time values in seconds)
#define C_MAX_DEFAULT   	5
#define C_DELAY_DEFAULT		1
#define C_PURGE_DEFAULT		120
#define C_RECLAIM_TIME		30

// system circuit indices
#define C_NUM_CIRCUITS 1
#define C_MARX        0
#define C_MTG70       1
#define C_MTG         2
#define C_SWITCH      3
#define C_SWTG70      4
#define C_RECLAIMER   5
#define C_BOTTLE      6

// circuit parameter indices
#define C_NUM_PARAM   8
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

// system state bit positions
#define S_SHOT    1
#define S_ABORT   (1 << 1)
#define S_PURGE   (1 << 2)
#define S_ALARM   (1 << 3)
#define S_RECLAIM (1 << 4)
#define S_STANDBY (1 << 5)
#define S_ERROR   (1 << 6)
#define S_UPDATE  (1 << 7)

#define IN_RANGE(v, min, max) ((v >= min && v <= max))

#include "pid.h"
typedef struct circuit_t {
  double params[8];
	
  double pressure;
  double roc;

  uint8_t pins[C_NUM_IO];
  pid_t pid;
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

  /*
    bit positions store circuit enable state
    bitshifts follow sys.circuits indexing
    i.e. MARX -> 0-bit, MTG_70 -> 1-bit, ...
  */
  uint8_t en_flags;

  circuit_t circuits[C_NUM_CIRCUITS];

  uint32_t uptime;
  int pid_window_size;
} system_t;

/*
  event types
    errors
      error type
      error message
      error severity (?)
    state transitions
      previous state
      destination state
    ui transitions
      current menu
      pathtrace (list of menu indices)
    parameter modifications
      system or circuit parameter
      previous value
      new value
    circuit enable/disable
      circuit index
      prev state
      current state
    state save/load (preset stuff)
      save/load (or delete for presets)
      success/fail
      data written in bytes?
*/
typedef struct event_t {
  uint8_t type;
  uint32_t time;    // pulls from system-start or RTC module
  uint8_t msg[32];
  
  circuit_t *c;
  uint8_t s_flags;
  uint8_t c_flags;
  uint8_t p_flags;
} event_t;

extern Adafruit_ILI9341 tft;
extern TouchScreen ts;
extern TM1637Display pdisp;

extern system_t sys;
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

void sim_setup();
void sim_tick();

void init_system();
void init_io();
void init_menus();
void init_options();

TSPoint get_press(TouchScreen *ts);
void update_ui();
void update_circuits();

void purge();
void set_pressure(circuit_t *c, double var, int half);
void shot_pressure(bool half);

#endif // SIMULATOR_H