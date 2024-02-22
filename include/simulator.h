#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <Arduino.h>
#include <TM1637Display.h>
#include <avr8-stub.h>

#include "ui.h"
#include "pid.h"
#include "remote.h"

// system state bit positions
#define S_SHOT    0
#define S_ABORT   1
#define S_PURGE   2
#define S_ALARM   3
#define S_RECLAIM 4
#define S_STANDBY 5
#define S_ERROR   6
#define S_REMOTE  7

// system circuit array/bit indices
#define C_NUM_CIRCUITS 5
#define C_MARX        0
#define C_MTG70       1
#define C_MTG         2
#define C_SWITCH      3
#define C_SWTG70      4
#define C_RECLAIMER   5
#define C_BOTTLE      6

// system circuit IO indices
#define C_NUM_IO 7
#define I_PRESSURE_READ 0
#define I_PRESSURE_IN   1
#define I_PRESSURE_OUT  2
#define I_ENABLE_BTN    3
#define I_LED           4
#define I_DISP_CLK      5
#define I_DISP_DIO      6

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

#define IN_RANGE(v, min, max) ((v >= min && v <= max))

typedef struct io_t {
  volatile uint8_t *pin;
  volatile uint8_t *port;
  volatile uint8_t *ddr;
} io_t;

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

  TM1637Display *disp;
} circuit_t;

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
  uint8_t c_flags;    // circuit mask
  uint8_t en_flags;   // circuit enable/disable
  uint16_t p_flags;   // parameter mask
  uint8_t r_flags;    // remote flags

  uint8_t up_types;   // updates to issue
  uint32_t uptime;    // system uptime in ms

  uint8_t pbuf[256];   // shared packet data buffer
  
  circuit_t circuits[C_NUM_CIRCUITS];
  io_t c_button;
  io_t c_led;
  usart_t remote;
  packet_t pkt;

  int pid_window_size;

  ui_t ui;
} system_t;


extern TM1637Display pdisp;
extern system_t sys;

extern volatile uint8_t *PORT_B;
extern volatile uint8_t *DDR_B;
extern volatile uint8_t *PIN_B;
extern volatile uint8_t *PORT_L;
extern volatile uint8_t *DDR_L;
extern volatile uint8_t *PIN_L;

/**
 * @brief initializes simulator. should be invoked on startup or reset
 * 
 */
void sim_setup();
void init_system();
void init_io();

/**
 * @brief updates simulation. should be invoked per iteration.
 * 
 */
void sim_tick();

void modify_circuit(circuit_t *c, uint32_t dt);

int process_pkt();
size_t parse_command(uint8_t flags, uint8_t *bytes);
size_t packetize_circuits(uint8_t *bytes, uint8_t cmask, uint16_t pmask, uint8_t dir);
size_t packetize_system(uint8_t *bytes);
int issue_updates(usart_t *u);

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