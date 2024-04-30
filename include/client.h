/**
 * @file client.h
 * @author Bradley Sullivan (bradleysullivan@nevada.unr.edu)
 * @brief Main client functionality
 * @version 0.1
 * @date 2024-04-24
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef CLIENT_H
#define CLIENT_H

#include <stdlib.h>
#include <curses.h>
#include <ctype.h>
#include <getopt.h>
#include <error.h>
#include <poll.h>
#include <sys/time.h>
#include <pthread.h>

#include "simulator_defines.h"
#include "remote.h"

/**
 * @brief Curses window dimensions
 * 
 */
#define CWIN_WIDTH  50  /// Curses circuit window width
#define CWIN_HEIGHT 6   /// Curses circuit window height
#define SWIN_HEIGHT 30  /// Curses status window height
#define SWIN_WIDTH  50  /// Curses status window width

typedef struct _win_st WINDOW;

/**
 * @brief Input buffer limits
 * 
 */
#define MAX_CMD_LEN 256 /// Max. command buffer length
#define MAX_VALUES  64  /// Max. command value-length
#define MAX_ARGS    64  /// Max. command argument-length


/**
 * @brief Client-state bit indexes
 * 
 */
#define S_EXIT      0   /// Client exit state bit position
#define S_INPUT     1   /// Client flag for new input to process
#define S_RECON     2   /// Client flag to initiate serial device reconnect process
#define S_PING      3   /// Client flag to process/initiate ping requests


/**
 * @brief Client-error codes
 * 
 */
#define E_INPUT     1   /// Input processing error
#define E_TOKEN     2   /// Input tokenization failure
#define E_OPTION    3   /// Unknown command option
#define E_SUBOPT    4   /// Unknown option sub-argument
#define E_CONNECT   5   /// Serial device connection failure
#define E_NOTTY     6   /// Serial device disconnect detected
#define E_PCREATE   7   /// Packet construction failure
#define E_ARGUMENT  8   /// Missing command option argument
#define E_RETRY     9   /// Packet transmission failure/timeout


/**
 * @brief 
 * 
 * @var valset_t::op Operation associated with value-set-context
 * @var valset_t::flag Operation flags for value-set-context
 * @var valset_t::value Value to set
 * 
 */
typedef struct valset_t {
  int op;
  int flag;  
  double value;
} valset_t;

/**
 * @brief Configuration structure for building packets
 * 
 */
typedef struct packet_args {
  /// @brief Packet type code (PK_UPDATE, PK_COMMAND, PK_STATUS)
  uint8_t op_type;
  /// @brief Packet subtype/flags (UP_SYSTEM, CMD_PARSET, ST_PING, etc.)
  uint8_t op_flags;

  /// @brief Set bit positions denote circuit context
  uint8_t cmask;
  /// @brief Set bit positions denote parameter context
  uint16_t pmask;
  /// @brief Time interval in seconds for timeout
  uint8_t timeout;
  /// @brief If non-zero, sender requests notification of reception
  uint8_t req_ack;

  /// @brief Index of next valset context
  uint8_t next_val;

  /// @brief Value context buffer for parameter value configuration
  valset_t values[MAX_VALUES];
} packet_args;

/**
 * @brief Main client datastructure and state.
 * 
 */
typedef struct client_t {
  /// @brief Client state variable. Uses bit-indexes S_EXIT, S_INPUT, S_...
  uint8_t state;

  /// @brief Client error state. Takes error codes of E_INPUT, E_CONNECT, E_...
  uint8_t err;
  
  /// @brief System uptime in milliseconds
  uint32_t up_period;

  /// @brief Serial device polling interval/timeout
  uint32_t poll_time;
  
  /// @brief System state datastructure
  system_t sim;

  /// @brief Circuit state/configuration buffer
  circuit_t circuit[C_NUM_CIRCUITS];
  
  /// @brief Connected remote structure for receiving and transmitting data
  remote_t *remote;
  
  /// @brief Pointer to newly received packets
  packet_t *receive;

  /// @brief User-input keypress as fetched from curses with wgetch(client_t::scr)
  int key;

  /// @brief Next input-buffer location
  int cursor;

  /// @brief Various input, print, and device-path buffers
  char cmd_input[MAX_CMD_LEN];
  char err_header[MAX_CMD_LEN];
  char err_message[MAX_CMD_LEN];
  char print_buf[MAX_CMD_LEN];
  char dev_buf[32];

  /// @brief Main packet argument structure for constructing packets with parsed commands
  packet_args pargs;

  /// @brief Curses UI windows
  WINDOW *scr;
  WINDOW *view_win;
  WINDOW *view;
  WINDOW *cmd;
  WINDOW *sim_win;
} client_t;

// string maps
extern char *const circuit_map[];
extern char *const param_map[];
extern char *const io_map[];
extern char *const mode_map[];
extern char *const flag_map[][7];
extern char *const status_map[];
extern char *const pktype_map[];

client_t *c_get(client_t *c, char *dev_buf);

void update_client(client_t *c);
size_t update_system(client_t *c, uint8_t *bytes);
size_t update_circuits(client_t *c, uint8_t *bytes);

int process_key(client_t *c, int key);
int process_packet(client_t *c, packet_t *p);
int process_input(client_t *c, char *cmd);
int process_update(client_t *c, packet_t *p);
int process_error(client_t *c);

int parse_update(packet_args *pargs, int ac, char *av[]);
int parse_command(packet_args *pargs, int ac, char *av[]);
int parse_ping(packet_args *pargs, int ac, char *av[]);
int mapstr(char *const map[], int maplen, char *str);
int mapflag(int flag, int width, char *const map[], char *str);
int tokenize_cmd(char *cmd, char **vec);
void to_lower(char *str);
void to_upper(char *str);
size_t store_params(uint8_t *buf, double *data, int len);

int val_seek(valset_t *v, valset_t **vbase, int op, int len);
int val_cmp_flag(const void *a, const void *b);
int val_cmp_op(const void *a, const void *b);
int val_cmp_value(const void *a, const void *b);

int construct_packet(client_t *c, packet_t *p, packet_args *pargs);

void wclrtorng(WINDOW *w, int y, int bx, int ex);
void center_str(WINDOW *w, int y, const char *str);
void draw_system(client_t *c);
void draw_circuits(client_t *c);
void draw_input(client_t *c);
void draw_packet(client_t *c, packet_t *p);
void draw_err(client_t *c);
void draw_help(client_t *c, char *msg);
void print_view(client_t *c, char *msg);
void draw_circuit_view(client_t *c, int idx);
void draw_system_view(client_t *c);

#endif // CLIENT_H