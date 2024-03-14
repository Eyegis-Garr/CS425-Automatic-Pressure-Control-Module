#ifndef CLIENT_H
#define CLIENT_H

#include <curses.h>
#include <ctype.h>
#include <argp.h>
#include <error.h>
#include <poll.h>

#include "simulator_defines.h"
#include "remote.h"

#define S_TXRDY     0

#define CWIN_WIDTH  30
#define CWIN_HEIGHT 22
#define SWIN_HEIGHT 22
#define SWIN_WIDTH  30
#define GRID_ROWS   2
#define GRID_COLS   3

typedef struct _win_st WINDOW;

#define MAX_CMD_LEN 64
#define MAX_VALUES  32
#define MAX_ARGS    16
typedef struct client_t {
  uint8_t s_flags;    // state flags
  
  simulator_t sim;
  circuit_t circuits[C_NUM_CIRCUITS];
  remote_t *r;

  int cursor;
  char input[MAX_CMD_LEN];

  WINDOW *stwin;
  WINDOW *simwin;
  WINDOW *circw[C_NUM_CIRCUITS];
  WINDOW *scr;
} client_t;


typedef struct packet_args {
  uint8_t op_type;
  uint8_t op_flags;

  uint8_t cmask;
  uint16_t pmask;

  uint8_t next_val;
  double data[MAX_VALUES];
} packet_args;

int init_client(client_t *c);

int process_input(client_t *c, int key);
int process_packet(client_t *c, packet_t *p);
int process_command(char *cmd, packet_t *p);
int process_update(client_t *c, packet_t *p);

int mapstr(char map[][32], int maplen, char *str);
int tokenize_cmd(char *cmd, char **vec);
error_t update_parser(int key, char *arg, struct argp_state *state);
error_t command_parser(int key, char *arg, struct argp_state *state);
size_t store_params(uint8_t *buf, double *data, int len);
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