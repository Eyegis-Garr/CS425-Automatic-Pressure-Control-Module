/**
 * @file simulator_defines.h
 * @author Bradley Sullivan (bradleysullivan@nevada.unr.edu)
 * @brief Main defines, macros and datastructures used by system
 * @version 0.1
 * @date 2024-04-24
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef SIMULATOR_DEFINES_H
#define SIMULATOR_DEFINES_H

#include <inttypes.h>
#include <curses.h>

/**
 * @brief Main system mode codes
 * 
 */
#define S_NUM_MODES 8
#define S_SHOT    0
#define S_ABORT   1
#define S_PURGE   2
#define S_ALARM   3
#define S_RECLAIM 4
#define S_STANDBY 5
#define S_ERROR   6
#define S_REMOTE  7

/**
 * @brief Main system circuit indices. Circuit
 * masks should use these bit-positions for each circuit
 * 
 */
#define C_NUM_CIRCUITS 5
#define C_MARX        0
#define C_MTG70       1
#define C_MTG         2
#define C_SWITCH      3
#define C_SWTG70      4

/**
 * @brief Circuit I/O indices/bit-positions
 * 
 */
#define C_NUM_IO 7
#define I_PRESSURE_READ 0
#define I_PRESSURE_IN   1
#define I_PRESSURE_OUT  2
#define I_LED           3
#define I_ENABLE_BTN    4
#define I_DISP_CLK      5
#define I_DISP_DIO      6

/**
 * @brief Main circuit parameter indices. Circuit
 * parameter masks should use these bit-positions.
 * 
 */
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

/**
 * @brief Circuit, reclaimer and supply pressure max values
 * 
 */
#define C_PRESSURE_MAX    25.0
#define S_SUPPLY_MAX      150.0
#define S_RECLAIM_MAX     32.0

/**
 * @brief Basic circuit-info datastructure
 * 
 */
typedef struct circuit_t {
  /// @brief Stores circuit parameter values
  double params[C_NUM_PARAM];

  /// @brief Bit-positions indicate HIGH/LOW digital I/O state
  uint8_t io;

  /// @brief Curses circuit UI panel
  WINDOW *w;
} circuit_t;

/**
 * @brief Core system state/info
 * 
 */
typedef struct system_t {
  /// @brief Current system mode. Takes values of S_SHOT, S_ABORT, S_ERROR, S_...
  uint8_t state;

  /// @brief System error number
  uint8_t err;

  /// @brief Masks outgoing circuit-updates
  uint8_t c_flags;

  /// @brief Masks parameters for each circuit-update
  uint16_t p_flags;

  /// @brief Bit-positions denote enabled/disabled circuits
  uint8_t en_flags;
  
  /// @brief System's subscribed update types. Uses bit-positions UP_SYSTEM, UP_CIRCUITS, and UP_REMOTE (see remote.h)
  uint8_t up_types;

  /// @brief System uptime in milliseconds. Rolls over every ~55 days
  uint32_t uptime;

  /// @brief System reclaimer pressure
  float reclaimer;

  /// @brief System bottle/supply pressure
  float supply;

  /// @brief System reclaimer on pressure threshold
  float rec_auto_on;

  /// @brief System reclaimer off pressure threshold
  float rec_auto_off;

  /// @brief System minimum supply pressure alarm threshold
  float supply_min;
} system_t;

#endif // SIMULATOR_DEFINES_H