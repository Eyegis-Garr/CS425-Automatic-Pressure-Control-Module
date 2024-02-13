#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <PID_v1.h>
#include <Adafruit_ILI9341.h>
#include <TouchScreen.h>
#include <TM1637Display.h>
#include <avr8-stub.h>

#include "menu.h"
#include "callbacks.h"
#include "pid.h"
#include "remote.h"

// system state bit positions
#define S_SHOT    1
#define S_ABORT   (1 << 1)
#define S_PURGE   (1 << 2)
#define S_ALARM   (1 << 3)
#define S_RECLAIM (1 << 4)
#define S_STANDBY (1 << 5)
#define S_ERROR   (1 << 6)

// system circuit array/bit indices
#define C_NUM_CIRCUITS 1
#define C_MARX        0
#define C_MTG70       1
#define C_MTG         2
#define C_SWITCH      3
#define C_SWTG70      4
#define C_RECLAIMER   5
#define C_BOTTLE      6

// system circuit IO indices
#define C_NUM_IO 5
#define I_PRESSURE_READ 0
#define I_PRESSURE_IN   1
#define I_PRESSURE_OUT  2
#define I_ENABLE_BTN    3
#define I_LED           4

// circuit parameter array/bit indices
#define C_NUM_PARAM   9
#define P_PRESSURE    0
#define P_SET_POINT   1
#define P_MAX_TIME    2
#define P_CHECK_TIME  3
#define P_PURGE_TIME  4
#define P_DELAY_TIME  5
#define P_KP          6
#define P_KI          7
#define P_KD          8

// system circuit defaults (time values in seconds)
#define C_MAX_DEFAULT   	5
#define C_DELAY_DEFAULT		1
#define C_PURGE_DEFAULT		120
#define C_RECLAIM_TIME		30

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

/**
 * @brief circuit model
 * 
 * @params: storage for per-circuit state/configuration.
 * uses P_<PARAMETER> for array-indexing
 * 
 * @pressure: pressure reading in PSI for circuit
 * 
 * @roc: pressure rate-of-change or step value for 
 * modifying pressure changes.
 * 
 * @pins: circuit digital pin IO assignments. uses
 * I_<CONNECTION> (pressure in/out/read, enable, LED)
 * for array-indexing
 * 
 * @pid: PID controller state for approaching set-point.
 * 
 */
typedef struct circuit_t {
  double params[C_NUM_PARAM];
  double roc;

  uint8_t pins[C_NUM_IO];
  pid_t pid;
} circuit_t;

typedef int (*TimerCallback)(void);
typedef struct timer_t {
  uint32_t itime;
  uint32_t etime;

  TimerCallback cb;
} timer_t;

/**
 * @brief system model
 * 
 * @s_flags: set bit-positions encode current state/mode
 * macros S_<STATE/MODE> denote specific mode bit positions
 * 
 * @c_flags: set bit-positions encode selected circuit
 * context used for assigning parameters. uses C_<CIRCUIT>
 * macro for bit-indexing.
 * 
 * @p_flags: set bit-positions encode active parameters
 * for modification. uses P_<PARAMETER> for bit-indexing.
 * 
 * @en_flags: encodes enabled/disabled circuits. uses 
 * C_<CIRCUIT> macros for bit-indexing
 * 
 * @circuits: storage for individual circuit state. uses
 * C_<CIRCUIT> for array-indexing.
 * 
 */
typedef struct system_t {
  uint8_t s_flags;    // state
  uint8_t c_flags;    // circuit select
  uint16_t p_flags;   // parameter select
  uint8_t en_flags;   // circuit enable/disable

  int up_cycle;       // update period in ms
  uint8_t up_types;   // updates to issue
  timer_t up_timer;   // software update timer

  uint8_t pbuf[128];   // shared packet data buffer
  packet_t p_tx, p_rx;  // transmit and receive packets

  circuit_t circuits[C_NUM_CIRCUITS];

  uint32_t uptime;
  int pid_window_size;
} system_t;

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

/**
 * @brief initializes simulator. should be invoked on startup or reset
 * 
 */
void sim_setup();

/**
 * @brief updates simulation. should be invoked per iteration.
 * 
 */
void sim_tick();

void init_system();
void init_io();
void init_menus();
void init_options();

TSPoint get_press(TouchScreen *ts);
void update_ui();
void update_circuits();

size_t packetize_circuits(circuit_t *c, uint8_t *bytes);
size_t packetize_system(system_t *s, uint8_t *bytes);
int issue_updates(HardwareSerial *s);

/**
 * @brief purges all enabled system circuits. currently lowers pressure to 0.
 * can/should purge for each circuit's purge time.
 * 
 */
void purge();

/**
 * @brief steps circuit pressure towards set-point.
 * 
 * @param c - circuit to set
 * @param var - +/- pressure variance in PSI
 * @param half - true -> set to half of set-point; false -> set to set-point
 */
void set_pressure(circuit_t *c, double var, int half);

/**
 * @brief invokes set_pressure for all enabled circuits.
 * should be invoked iteratively until set-point is reached for all circuits.
 * 
 * @param half - true -> set to half of set-point; false -> set to set-point
 */
void shot_pressure(bool half);

#endif // SIMULATOR_H