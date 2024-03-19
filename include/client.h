#ifndef CLIENT_H
#define CLIENT_H

#include <stdlib.h>
#include <curses.h>
#include <ctype.h>
#include <argp.h>
#include <getopt.h>
#include <error.h>
#include <poll.h>

#include "simulator_defines.h"
#include "remote.h"

#define CWIN_WIDTH  30
#define CWIN_HEIGHT 22
#define SWIN_HEIGHT 22
#define SWIN_WIDTH  30
#define GRID_ROWS   2
#define GRID_COLS   3

typedef struct _win_st WINDOW;

// input limits
#define MAX_CMD_LEN 256
#define MAX_VALUES  64
#define MAX_ARGS    64

// client state indices
#define S_EXIT      0
#define S_INPUT     1
#define S_UPCYCLE   2
#define S_EINPUT    7

typedef struct client_t {
  uint8_t s_flags;    // state flags
  uint32_t up_period;
  
  system_t sim;
  circuit_t circuit[C_NUM_CIRCUITS];
  remote_t r;

  int cursor;
  char input[MAX_CMD_LEN];

  WINDOW *scr;
} client_t;

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

int init_client(client_t *c);

int process_key(client_t *c, int key);
int process_packet(client_t *c, packet_t *p);
int process_input(client_t *c, char *cmd);
int process_update(client_t *c, packet_t *p);

int mapstr(char map[][32], int maplen, char *str);
int tokenize_cmd(char *cmd, char **vec); 
size_t store_params(uint8_t *buf, double *data, int len);

int val_seek(valset_t *v, valset_t **vbase, int op, int len);
int val_cmp_flag(const void *a, const void *b);
int val_cmp_op(const void *a, const void *b);
int val_cmp_value(const void *a, const void *b);

size_t construct_packet(packet_t *p, packet_args *pargs);
size_t construct_command(packet_t *p, packet_args *a, int ack, uint8_t timeout);
size_t construct_update(packet_t *p, packet_args *a, uint8_t timeout);

void wclrtorng(WINDOW *w, int y, int bx, int ex);
void center_str(WINDOW *w, int y, const char *str);
void draw_circuits(client_t *c);
void draw_simulator(client_t *c);
void draw_packet(client_t *c);

size_t update_system(client_t *c, uint8_t *bytes);
size_t update_circuits(client_t *c, uint8_t *bytes);

#endif // CLIENT_H