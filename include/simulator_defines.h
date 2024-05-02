#ifndef SIMULATOR_DEFINES_H
#define SIMULATOR_DEFINES_H

// system state numbers
#define S_SHOT    0
#define S_ABORT   1
#define S_PURGE   2
#define S_ALARM   3
#define S_AUTOREC 4
#define S_STANDBY 5
#define S_ERROR   6
#define S_REMOTE  7

#define E_CMODIFY     1
#define E_LOWSUPPLY   2

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

// system circuit defaults (time values in milliseconds)
#define C_MAX_DEFAULT   	5000
#define C_DELAY_DEFAULT		1000
#define C_PURGE_DEFAULT		5000
#define C_RECLAIM_TIME		5000
#define C_PRESSURE_MAX    80.0
#define S_SUPPLY_MAX      150.0
#define S_RECLAIM_MAX     32.0

#define CWIN_WIDTH  100
#define CWIN_HEIGHT 75

#define IN_RANGE(v, min, max) ((v >= min && v <= max))

#endif // SIMULATOR_DEFINES_H