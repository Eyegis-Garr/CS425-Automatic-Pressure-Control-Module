#ifndef CLIENT_H
#define CLIENT_H

#include <stdlib.h>
#include <curses.h>
#include <ctype.h>
#include <argp.h>
#include <getopt.h>
#include <error.h>
#include <poll.h>
#include <time.h>

#include "simulator_defines.h"
#include "remote.h"

#define CWIN_WIDTH  50
#define CWIN_HEIGHT 6
#define SWIN_HEIGHT 24
#define SWIN_WIDTH  50

typedef struct _win_st WINDOW;

// input limits
#define MAX_CMD_LEN 256
#define MAX_VALUES  64
#define MAX_ARGS    64

// client state indices
#define S_EXIT      0
#define S_INPUT     1
#define S_UPCYCLE   2
#define S_PING      3
#define S_RETRY     4

#define E_INPUT     1
#define E_TIMEOUT   2
#define E_TOKEN     3
#define E_OPTION    4
#define E_SUBOPT    5
#define E_CONNECT   6
#define E_PCREATE   7
#define E_ARGUMENT  8
#define E_RETRY     9

typedef struct valset_t {
  int op;
  int flag;  
  double value;
} valset_t;

typedef struct packet_args {
  uint8_t op_type;
  uint8_t op_flags;

  uint8_t cmask;
  uint16_t pmask;
  uint8_t timeout;
  uint8_t req_ack;

  uint8_t next_val;
  double pdata[MAX_VALUES];

  valset_t values[MAX_VALUES];
} packet_args;

typedef struct client_t {
  uint8_t s_flags;    // state flags
  uint8_t err;    // error flags
  uint32_t up_period;
  
  system_t sim;
  circuit_t circuit[C_NUM_CIRCUITS];

  remote_t r;
  packet_args pargs;

  int cursor;
  char cmd_input[MAX_CMD_LEN];
  char err_header[MAX_CMD_LEN];
  char err_message[MAX_CMD_LEN];
  char print_buf[MAX_CMD_LEN];

  WINDOW *scr;
  WINDOW *view_win;
  WINDOW *view;
  WINDOW *cmd;
} client_t;

// string maps
extern char *const circuit_map[];
extern char *const param_map[];
extern char *const io_map[];
extern char *const mode_map[];
extern char *const flag_map[][7];
extern char *const status_map[];
extern char *const pktype_map[];

// command optstrings
extern const char update_opts[];
extern const char command_opts[];
extern const char ping_opts[];

int init_client(client_t *c, char *dev_path);

int process_key(client_t *c, int key);
int process_packet(client_t *c, packet_t *p);
int process_input(client_t *c, char *cmd);
int process_update(client_t *c, packet_t *p);
int process_error(client_t *c, int key);

int parse_update(packet_args *pargs, int ac, char *av[]);
int parse_command(packet_args *pargs, int ac, char *av[]);
int parse_ping(packet_args *pargs, int ac, char *av[]);
int mapstr(char *const map[], int maplen, char *str);
int mapflag(int flag, int width, char *const map[], char *str);
int tokenize_cmd(char *cmd, char **vec); 
size_t store_params(uint8_t *buf, double *data, int len);

int val_seek(valset_t *v, valset_t **vbase, int op, int len);
int val_cmp_flag(const void *a, const void *b);
int val_cmp_op(const void *a, const void *b);
int val_cmp_value(const void *a, const void *b);

int construct_packet(client_t *c, packet_t *p, packet_args *pargs);
int ping(client_t *c, packet_t *p);

void wclrtorng(WINDOW *w, int y, int bx, int ex);
void center_str(WINDOW *w, int y, const char *str);

void draw_circuits(client_t *c);
void draw_input(client_t *c);
void draw_simulator(client_t *c);
void draw_packet(client_t *c, packet_t *p);
void draw_err(client_t *c);
void draw_help(client_t *c, char *msg);
void print_view(client_t *c, char *msg);

void update_client(client_t *c);

size_t update_system(client_t *c, uint8_t *bytes);
size_t update_circuits(client_t *c, uint8_t *bytes);

#endif // CLIENT_H