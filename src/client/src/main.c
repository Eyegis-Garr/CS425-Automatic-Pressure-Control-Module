#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curses.h>
#include <ctype.h>
#include <argp.h>

#include "remote.h"
#include "simulator_defines.h"

#define MAX_CMD_LEN 64
typedef struct _win_st WINDOW;
typedef struct client_t {
  uint8_t s_flags;    // state flags
  
  simulator_t sim;
  circuit_t circuits[C_NUM_CIRCUITS];

  WINDOW *sw;
  WINDOW *cw[C_NUM_CIRCUITS];

  int cursor;
  char input[MAX_CMD_LEN];

  remote_t *r;

  WINDOW *scr;
} client_t;

static const char cname_map[C_NUM_CIRCUITS][32] = {
  "MARX",
  "MTG 70",
  "MTG",
  "SWITCH",
  "SWITCH TG70"
};

static const char param_map[C_NUM_PARAM][32] = {
  "PRESSURE    ",
  "SET POINT   ",
  "MAX. TIME   ",
  "CHECK TIME  ",
  "PURGE TIME  ",
  "DELAY TIME  ",
  "PID KP      ",
  "PID KI      ",
  "PID KD      "
};

static const char io_map[3][16] = {
  "INTAKE  ",
  "EXHAUST ",
  "ENABLED "
};

static const char mode_map[8][16] = {
  "SHOT ",
  "ABORT ",
  "PURGE ",
  "ALARM ",
  "RECLAIM ",
  "STANDBY ",
  "ERROR ",
  "REMOTE "
};

static const char update_map[3][16] = {
  "SYSTEM  ",
  "CIRCUITS  ",
  "REMOTE  "
};

#define isbset(d, b) (((d) & (1 << (b))) != 0)
#define isbclr(d, b) (((d) & (1 << (b))) == 0)

#define CWIN_WIDTH  30
#define CWIN_HEIGHT 22
#define SWIN_HEIGHT 22
#define SWIN_WIDTH  30
#define GRID_ROWS   2
#define GRID_COLS   3

void *remote_listen(void *r);
void *client_listen(void *c);
void print_packet(packet_t *p);
size_t construct_update(packet_t *p, uint8_t up_types, uint8_t cmask, uint16_t pmask, uint8_t timeout);
int ping_test(remote_t *r, uint8_t npings);

void center_str(WINDOW *w, int y, const char *str);
void draw_circuits(client_t *c);
void init_client(client_t *c);
int process_command(client_t *c);
int process_key(client_t *c, int key);

void draw_simulator(client_t *c) {
  char pbuf[64];
  int i, l = 1;

  curs_set(0);

  center_str(c->sw, l++, "SIMULATOR");
  center_str(c->sw, l++, "-----------");

  pbuf[0] = '\0';

  center_str(c->sw, l++, "MODE");
  for (i = 0; i < 8; i += 1) {
    if (isbset(c->sim.s_flags, i))
      strcat(pbuf, mode_map[i]);
  }
  center_str(c->sw, l++, pbuf);
  center_str(c->sw, l++, "-----------");

  pbuf[0] = '\0';

  center_str(c->sw, l++, "UPDATE TYPES");
  for (i = 0; i < 3; i += 1) {
    if (isbset(c->sim.up_types, i)) 
      strcat(pbuf, update_map[i]);
  }
  center_str(c->sw, l++, pbuf);
  center_str(c->sw, l++, "-----------");

  center_str(c->sw, l++, "UPTIME");
  sprintf(pbuf, "%lf s", (double) c->sim.uptime / 1000);
  center_str(c->sw, l++, pbuf);
  center_str(c->sw, l++, "-----------");

  wborder(c->sw, '|', '|', '-', '-', '+', '+', '+', '+');
  wrefresh(c->sw);

  curs_set(1);
}

size_t update_circuits(client_t *c, uint8_t *bytes) {
  uint8_t *pdata = bytes;
  double *cdata;  
  uint8_t cmask;
  uint16_t pmask;

  // read packet circuit mask
  cmask = *pdata++;
  // read packet parameter mask
  pmask = *pdata++;
  pmask <<= 8;
  pmask |= *pdata++;

  circuit_t *circuit;
  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {
    if (isbset(cmask, i)) {
      circuit = &c->circuits[i];
      cdata = circuit->params;
      for (uint8_t k = 0; k < C_NUM_PARAM; k += 1) {
        if (isbset(pmask, k)) {
          // load parameter into enabled circuit
          *cdata++ = *pdata + ((double)(*(pdata + 1)) / 100);
          pdata += 2;
        }
      }

      // read io state
      circuit->io = *pdata++;
    }
  }

  return pdata - bytes;
}

size_t update_system(client_t *c, uint8_t *bytes) {
  uint8_t *sbytes = bytes;

  // read system flags
  c->sim.s_flags = *sbytes++;
  c->sim.c_flags = *sbytes++;
  c->sim.p_flags = *sbytes++;
  c->sim.p_flags <<= 8;
  c->sim.p_flags = *sbytes++;
  c->sim.en_flags = *sbytes++;
  // read last recieved client update flags
  c->sim.up_types = *sbytes++;
  // read system uptime
  c->sim.uptime = 0;
  for (int i = 0; i < 4; i += 1) {
    c->sim.uptime |= *sbytes++;
    c->sim.uptime <<= 8;
  }

  return sbytes - bytes;
}

int process_update(client_t *c, packet_t *p) {
  uint8_t *bytes = p->bytes;

  if (isbset(p->flags, UP_SYSTEM)) {
    // load system state variables
    bytes += update_system(c, bytes);
  } if (isbset(p->flags, UP_CIRCUITS)) {
    bytes += update_circuits(c, bytes);
  } if (isbset(p->flags, UP_REMOTE)) {
    // load system remote/packet state (not a priority)
  }

  return bytes != p->bytes;
}

int process_packet(client_t *c, packet_t *p) {
  int ret = 0;

  switch (p->type) {
    case PK_UPDATE:
      // filter and push update
      ret = process_update(c, p);
      break;
    case PK_STATUS:
      // if timeout, flag for retransmit?
      if (isbset(p->flags, ST_TIMEOUT)) {
        mvwprintw(c->scr, LINES - 2, 1, "RECEIVED TIMEOUT");
        wrefresh(c->scr);
      }
      break;
    default:
      break;
  }

  return ret;
}

#define DEBUG 
int main(void) {
  remote_t rsys;
  client_t client;

  client.r = &rsys;

#ifndef DEBUG
  pthread_t rthread;
  pthread_t cthread;

  if (pthread_create(&rthread, NULL, remote_listen, (void *)&rsys)) {
    printf("error creating remote listening thread.\n");
    exit(1);
  } if (pthread_create(&cthread, NULL, client_listen, (void *)&client)) {
    printf("error creating client input thread.\n");
  }

  pthread_join(rthread, NULL);
  pthread_join(cthread, NULL);
#endif
#ifdef DEBUG

  init_remote(&rsys, "/dev/ttyS13", B9600);

  sleep(2);

  uint8_t types = (1 << UP_CIRCUITS) | (1 << UP_REFRESH);
  uint8_t cmask = (1 << 1) - 1;
  uint16_t pmask = (1 << 9) - 1;
  construct_update(&rsys.tx, types, cmask, pmask, 3);

  int ct = 0;
  while (1) {
    tx_packet(&rsys.tx, rsys.fd);
    if (block_resp(&rsys, 50) > 0) {
      print_packet(&rsys.rx);
      ct += 1;
      printf("ct : %d\n", ct);
    } 
  }
#endif
  
  return 0;
}

void *remote_listen(void *r) {
  remote_t *rsys = (remote_t *)r;

  init_remote(rsys, "/dev/ttyS13", B9600);

  sleep(2);

  uint8_t types = (1 << UP_CIRCUITS) | (1 << UP_SYSTEM) | (1 << UP_REFRESH);
  uint8_t cmask = (1 << C_MARX);
  uint16_t pmask = (1 << C_NUM_PARAM) - 1;
  construct_update(&rsys->tx, types, cmask, pmask, 2);

  int ready;
  while (isbclr(rsys->r_flags, R_EXIT)) {
    tx_packet(&rsys->tx, rsys->fd);
    if (block_resp(rsys, 80) > 0) {
      rsys->r_flags |= (1 << R_NDATA);
      // print_packet(&rsys->rx);
      // ct += 1;
      // printf("ct : %d\n", ct);
    } 
  }

  printf("exiting...\n");

  return r;
}

void *client_listen(void *c) {
  client_t *client = (client_t *)c;

  init_client(client);

  mvwaddch(client->scr, LINES - 1, 0, '>');

  int key, exit = 0;
  while (!exit) {

    key = getch();

    if (key == 27) {
      exit = 1;
    } else if (key != ERR) {
      // buffer user input
      process_key(client, key);
      // update screen with new input
      move(LINES - 1, 1); clrtoeol();
      mvwprintw(client->scr, LINES - 1, 1, client->input);
    } 
    
    if (isbset(client->r->r_flags, R_NDATA)) {      
      // update circuits with new data
      process_packet(client, &client->r->rx);

      client->r->r_flags &= ~(1 << R_NDATA);
    }

    draw_circuits(client);
    draw_simulator(client);
    refresh();
  }

  client->r->r_flags |= (1 << R_EXIT);

	endwin();			/* End curses mode */

  return c;
}

void print_packet(packet_t *p) {
  printf("===== header =====\n");

  printf("PK TYPE         : %u\n", p->type);
  printf("PK FLAGS        : %u\n", p->flags);
  printf("PK SIZE         : %u\n", p->size);

  printf("\t---- data ----\n");
  for (int i = 0; i < p->size; i += 1) {
    printf("PK DATA[%d]     : %u\n", i, p->bytes[i]);
  }
  printf("\t---- data ----\n");

  printf("===== header =====\n");

  printf("TOTAL BYTES: %d\n", p->size + 3);
}

size_t construct_update(packet_t *p, uint8_t up_types, uint8_t cmask, uint16_t pmask, uint8_t timeout) {
  uint8_t *pdata = p->bytes;

  // configure headear
  p->type = PK_UPDATE;
  p->flags = up_types;

  if (isbset(up_types, UP_CIRCUITS)) {
    // store update masks
    *pdata++ = cmask;
    *pdata++ = pmask >> 8;
    *pdata++ = pmask;
  }

  p->size = pdata - p->bytes;
  
  p->bytes[p->size++] = timeout;

  return pdata - p->bytes;
}

int ping_test(remote_t *r, uint8_t npings) {
  printf("This test should ping the remote client with 5 packets.\n");
  printf("A passing test should receive 5 ping replies within timeout intervals\n");

  int replies = ping(r, 1, npings);

  printf("Received replies: %d\n", replies);

  return replies == npings;
}

void center_str(WINDOW *w, int y, const char *str) {
  mvwaddstr(w, y, (getmaxx(w) / 2) - (strlen(str) / 2), str);
  wrefresh(w);
}

void wclrtorng(WINDOW *w, int y, int bx, int ex) {
  wmove(w, y, bx);
  for (; bx < ex; bx += 1) {
    mvwaddch(w, y, bx, ' ');
  }
}

void draw_circuits(client_t *c) {
  char pbuf[64];
  int x = 0, i, k, j, idx;

  curs_set(0);

  for (i = 0; i < GRID_ROWS; i += 1) {
    for (k = 0; k < GRID_COLS; k += 1) {
      idx = i * GRID_COLS + k;
      if (idx < C_NUM_CIRCUITS) {        
        center_str(c->cw[idx], 1, cname_map[idx]);

        x = 0;
        for (j = I_PRESSURE_IN; j <= I_ENABLE_BTN; j += 1) {
          if (isbset(c->circuits[idx].io, j)) wattron(c->cw[idx], COLOR_PAIR(j));
          mvwprintw(c->cw[idx], 2, x + 4, io_map[j - 1]);
          if (isbset(c->circuits[idx].io, j)) wattroff(c->cw[idx], COLOR_PAIR(j));

          x += strlen(io_map[j - 1]);
        }

        for (j = 0; j < C_NUM_PARAM; j += 1) {
          pbuf[0] = '\0';
          sprintf(pbuf, "%s:  %.02lf", param_map[j], c->circuits[idx].params[j]);
          wclrtorng(c->cw[idx], 2 * j + 3, 1, CWIN_WIDTH - 1);
          center_str(c->cw[idx], 2 * j + 3, pbuf);
          center_str(c->cw[idx], 2 * j + 4, "-----------");
          wrefresh(c->cw[idx]);
        }

        wborder(c->cw[idx], '|', '|', '-', '-', '+', '+', '+', '+');
        wrefresh(c->cw[idx]);
        refresh();
      }
    }
  }

  curs_set(1);
}

void init_client(client_t *c) {
  c->s_flags = 0;
  c->cursor = 0;

  // init curses
  c->scr = initscr();
  noecho();
  cbreak();
  keypad(c->scr, TRUE);
  nodelay(c->scr, TRUE);
  start_color();

  init_pair(1, COLOR_GREEN, COLOR_BLACK);
  init_pair(2, COLOR_RED, COLOR_BLACK);
  init_pair(3, COLOR_BLUE, COLOR_BLACK);

  int i, k, idx;
  for (i = 0; i < GRID_ROWS; i += 1) {
    for (k = 0; k < GRID_COLS; k += 1) {
      idx = i * GRID_COLS + k;
      if (idx < C_NUM_CIRCUITS) {
        c->cw[idx] = subwin(c->scr, CWIN_HEIGHT, CWIN_WIDTH, i * CWIN_HEIGHT, k * CWIN_WIDTH + 1);
      }
    }
  }

  c->sw = subwin(c->scr, SWIN_HEIGHT, SWIN_WIDTH, (GRID_ROWS - 1) * CWIN_HEIGHT, (GRID_COLS - 1) * CWIN_WIDTH + 1);
  
  draw_simulator(c);
  draw_circuits(c);
}

int process_command(client_t *c) {
  // parse string with argp
  // construct packet with parsing output?
  // check remote for open transmit
    // no packets flagged for RX-in-progress
    // no need to reflush TX buffer
}

int process_key(client_t *c, int key) {
  int ret = 0;
  if (isprint(key) && c->cursor < MAX_CMD_LEN - 1) {
    c->input[c->cursor++] = (char) key;
    c->input[c->cursor] = '\0';
  } else {
    switch (key) {
      case KEY_ENTER:
        // process command
        ret = process_command(c);
        break;
      case KEY_BACKSPACE:
      case KEY_DC:
      case 127:
        if (c->cursor > 0) c->input[--c->cursor] = '\0';
        break;
      case KEY_LEFT:
        if (c->cursor > 0) c->cursor -= 1;
        break;
      case KEY_RIGHT:
        if (c->cursor < MAX_CMD_LEN - 1) c->cursor += 1;
      default:
        break;
    }
  }

  return ret;
}
