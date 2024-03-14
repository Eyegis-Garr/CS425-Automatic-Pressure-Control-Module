#include "client.h"

static char cname_map[C_NUM_CIRCUITS][32] = {
  "MARX",
  "MTG70",
  "MTG",
  "SWITCH",
  "SWTG70"
};

static char param_map[C_NUM_PARAM][32] = {
  "PRESSURE",
  "SET_POINT",
  "MAX_TIME",
  "CHECK_TIME",
  "PURGE_TIME",
  "DELAY_TIME",
  "PID_KP",
  "PID_KI",
  "PID_KD"
};

static char io_map[3][16] = {
  "INTAKE  ",
  "EXHAUST ",
  "ENABLED "
};

static char mode_map[8][32] = {
  "SHOT",
  "ABORT",
  "PURGE",
  "ALARM",
  "RECLAIM",
  "STANDBY",
  "ERROR",
  "REMOTE "
};

static char update_map[3][32] = {
  "SYSTEM  ",
  "CIRCUITS  ",
  "REMOTE  "
};

static char uptype_map[5][32] = {
  "UP_SYSTEM ",
  "UP_CIRCUITS ",
  "UP_REMOTE ",
  "UP_REFRESH ",
  "UP_REMOVE"
};

static char cmdtype_map[6][32] = {
  "CMD_RESPND ", 
  "CMD_MODESET ",
  "CMD_PARSET ", 
  "CMD_SAVE ",   
  "CMD_GETLOG ", 
  "CMD_DMPCFG " 
};

static char pktype_map[4][32] = {
  "PK_UPDATE", 
  "PK_COMMAND",
  "PK_STATUS", 
  "PK_ACK"    
};

static char update_args_doc[] = "update [--circuit(-c)=index --parameter(-p)=index | --system(-s)]";
static char command_args_doc[] = "idfk";

static struct argp_option update_options[] = {
  {"circuit",   'c',  0, 0, "Apply command to specified circuit. Can be an index or circuit name."},
  {"system",    's',  0, 0, "Apply command to system state."},
  {"parameter", 'p',  0, 0, "Apply command to specified parameter."},
  {"refresh",   'r',  0, 0, "Request immediate response."},
  { 0 }
};

static struct argp_option command_options[] = {
  {"circuit",         'c',  0,          0, "Apply command to specified circuit. Can be an index or circuit name."},
  {"parameter",       'p',  0,          0, "Apply command to specified parameter."},
  {"mode-set",        'm', "mode",      0, "Modifies system mode with provided mode."},
  {"parameter-set",   'a', 0,           0, "Modifies specified circuit parameter with provided value."},
  {"save",            'e', 0,           0, "Saves system configuration to system storage."},
  {"gen-logs",        'g', 0,           0, "Generates and dumps system log to remote client."},
  {"dump-config",     'd', 0,           0, "Collects and dumps system configuration to remote client."},
  { 0 }
};

static struct argp update_argp = { update_options, update_parser, update_args_doc, 0 };
static struct argp command_argp = { command_options, command_parser, command_args_doc, 0 };

int init_client(client_t *c) {
  int ret = 0;

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
        c->circw[idx] = subwin(c->scr, CWIN_HEIGHT, CWIN_WIDTH, i * CWIN_HEIGHT, k * CWIN_WIDTH + 1);
      }
    }
  }

  c->stwin = subwin(c->scr, SWIN_HEIGHT, SWIN_WIDTH, (GRID_ROWS - 1) * CWIN_HEIGHT, (GRID_COLS - 1) * CWIN_WIDTH + 1);
  // c->stwin = subwin(c->scr, SWIN_HEIGHT, SWIN_WIDTH,  GRID_ROWS * CWIN_HEIGHT, GRID_COLS * CWIN_WIDTH + 1);

  return ret;
}

int process_input(client_t *c, int key) {
  int ret = 0;

  if (key != ERR) {
    // buffer user input
    if (isprint(key) && c->cursor < MAX_CMD_LEN - 1) {
      c->input[c->cursor++] = (char) key;
      c->input[c->cursor] = '\0';
    }

    switch (key) {
      case '\n':
        ret = 1;
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
        break;
      default:
        break;
    }
  }

  return ret;
}

int process_command(char *cmd, packet_t *p) {
  packet_args update_args = { 0 }, command_args = { 0 };
  char *av[MAX_ARGS];
  int ret = 0, ac;
  
  ac = tokenize_cmd(cmd, av);
  if (ac < 0) {
    printf("failed to tokenize command\n");
    ret = -1;
  } else {
    if (strcmp(av[0], "update") == 0) {
      update_args.op_type = PK_UPDATE;
      ret = argp_parse(&update_argp, ac, av, 0, 0, &update_args);
      if (ret == 0)
        p->size = construct_update(p, &update_args, 3);
    } else if (strcmp(av[0], "do") == 0) {
      ret = argp_parse(&command_argp, ac, av, 0, 0, &command_args);
      if (ret == 0)
        p->size = construct_command(p, &command_args, 1, 1);
    } else {
      ret = -1;
    }

    for (int i = 0; i < ac; i += 1) {
      free(av[i]);
    }
  }

  return (ret < 0) ? ret : p->size;  
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
        ret = -1;
      }
      break;
    default:
      break;
  }

  return ret;
}

int mapstr(char map[][32], int maplen, char *str) {
  for (int i = 0; i < maplen; i += 1) {
    if (strcmp(map[i], str) == 0) {
      return i;
    }
  }

  return -1;
}

int tokenize_cmd(char *cmd, char **vec) {
  int ct = 0;

  char *tok = strtok(cmd, " \n\t");     // split by spaces, tabs, and new lines
  while (tok && ct < MAX_ARGS) {        // while tokens remain
    vec[ct] = malloc(strlen(tok) + 1);  // malloc space for new token in vector
    if (!vec[ct]) return -1;            // error if malloc fails

    strcpy(vec[ct], tok);               // copy token into argument vector

    tok = strtok(NULL, " \t\n");        // move to next token
    ct += 1;                            // increment counter
  }

  vec[ct] = NULL;

  return ct;
}

error_t update_parser(int key, char *arg, struct argp_state *state) {
  packet_args *up_args = (packet_args *)state->input;
  char *next_arg;
  int cidx = 0, pidx;

  switch (key) {
    case 'c':
      up_args->op_flags |= (1 << UP_CIRCUITS);

      // parses specified circuits
      next_arg = state->argv[state->next];
      while (next_arg && next_arg[0] != '-' && state->next < state->argc) {
        cidx = mapstr(cname_map, C_NUM_CIRCUITS, next_arg);
        if (cidx < 0) {
          return 0;
        }

        up_args->cmask |= (1 << cidx);

        next_arg = state->argv[++state->next];
      }
      break;
    case 's':
      up_args->op_flags |= (1 << UP_SYSTEM);
      break;
    case 'p':
      // parses specified parameters
      next_arg = state->argv[state->next];
      while (next_arg && next_arg[0] != '-' && state->next < state->argc) {
        if (strcmp("ALL", next_arg) == 0) {
          up_args->pmask = (1 << C_NUM_PARAM) - 1;
        } else {
          pidx = mapstr(param_map, C_NUM_PARAM, next_arg);
          if (pidx < 0) {
            return 0;
          }

          up_args->pmask |= (1 << pidx);

          next_arg = state->argv[++state->next];
        }
      }
      break;
    case 'r':
      up_args->op_flags |= (1 << UP_REFRESH);
      break;
    default:
      return 0;
  }

  return 0;
}

error_t command_parser(int key, char *arg, struct argp_state *state) {
  packet_args *do_args = (packet_args *)state->input;
  char *next_arg;
  int cidx, pidx, midx;

  switch (key) {
    case 'c':
      // parses specified circuits
      next_arg = state->argv[state->next];
      while (next_arg && next_arg[0] != '-' && state->next < state->argc) {
        cidx = mapstr(cname_map, C_NUM_CIRCUITS, next_arg);
        if (cidx < 0) {
          return 0;
        }

        do_args->cmask |= (1 << cidx);

        next_arg = state->argv[++state->next];
      }
      break;
    case 'p':
      // parses specified parameters
      next_arg = state->argv[state->next];
      while (next_arg && next_arg[0] != '-' && state->next < state->argc) {
        if (strcmp("ALL", next_arg) == 0) {
          do_args->pmask = (1 << C_NUM_PARAM) - 1;
        } else {
          pidx = mapstr(param_map, C_NUM_PARAM, next_arg);
          if (pidx < 0) {
            return 0;
          }

          do_args->pmask |= (1 << pidx);

          next_arg = state->argv[++state->next];
        }
      }
      break;
    case 'm':
      do_args->op_flags |= (1 << CMD_MODESET);

      midx = mapstr(mode_map, S_NUM_MODES, arg);
      if (midx < 0) {
        return 0;
      }

      if (do_args->next_val < MAX_VALUES) {
        do_args->data[do_args->next_val++] = midx;
      } else {
        return 0;
      }
      break;
    case 'a':
      do_args->op_flags |= (1 << CMD_PARSET);

      next_arg = state->argv[state->next];
      while (next_arg && next_arg[0] != '-' && state->next < state->argc) {
        if (do_args->next_val < MAX_VALUES) {
          sscanf(next_arg, "%lf", &do_args->data[do_args->next_val++]);
        } else {
          return 0;
        }

        next_arg = state->argv[++state->next];
      }
      break;
    case 'e':
      do_args->op_flags |= (1 << CMD_SAVE);
      break;
    case 'g':
      do_args->op_flags |= (1 << CMD_GETLOG);
      break;
    case 'd':
      do_args->op_flags |= (1 << CMD_DMPCFG);
      break;
    default:
      return 0;
  }

  return 0;
}

size_t store_params(uint8_t *buf, double *data, int len) {
  uint8_t *bytes = buf;
  double *dbuf = data;

  while (bytes - buf < 2 * len) {
    *bytes++ = (uint8_t) (*dbuf);
    *bytes++ = (uint8_t) ((*dbuf - (uint8_t)*dbuf) * 100);

    dbuf += 1;
  }

  return 2 * len;
}

size_t construct_command(packet_t *p, packet_args *a, int ack, uint8_t timeout) {
  double *data = a->data;
  uint8_t *pdata = p->bytes;
  
  p->type = PK_COMMAND;
  p->flags = a->op_flags;

  if (ack) {
    p->flags |= (1 << CMD_RESPND);
  }

  for (int i = 1; i < 8; i += 1) {
    if (isbset(a->op_flags, i)) {
      switch (i) {
        case CMD_MODESET:
          *pdata++ = (uint8_t) (*data++);
          break;
        case CMD_PARSET:
          // store cmask
          *pdata++ = a->cmask;
          // store pmask
          *pdata++ = a->pmask >> 8;
          *pdata++ = a->pmask;
          pdata += store_params(pdata, data, a->next_val);
          break;
        case CMD_SAVE:
        case CMD_GETLOG:
        case CMD_DMPCFG:
        default:
          break;
      }
    }
  }

  *pdata++ = timeout;

  return (size_t)(pdata - p->bytes);
}

size_t construct_update(packet_t *p, packet_args *a, uint8_t timeout) {
  uint8_t *pdata = p->bytes;

  // configure headear
  p->type = PK_UPDATE;
  p->flags = a->op_flags;

  if (isbset(a->op_flags, UP_CIRCUITS)) {
    // store update masks
    *pdata++ = a->cmask;
    *pdata++ = a->pmask >> 8;
    *pdata++ = a->pmask;
  }

  *pdata++ = timeout;

  return pdata - p->bytes;
}

void wclrtorng(WINDOW *w, int y, int bx, int ex) {
  wmove(w, y, bx);
  for (; bx < ex; bx += 1) {
    mvwaddch(w, y, bx, ' ');
  }
}

void center_str(WINDOW *w, int y, const char *str) {
  mvwaddstr(w, y, (getmaxx(w) / 2) - (strlen(str) / 2), str);
  wrefresh(w);
}

void draw_circuits(client_t *c) {
  char pbuf[64];
  int x = 0, i, k, j, idx;

  curs_set(0);

  for (i = 0; i < GRID_ROWS; i += 1) {
    for (k = 0; k < GRID_COLS; k += 1) {
      idx = i * GRID_COLS + k;
      if (idx < C_NUM_CIRCUITS) {        
        center_str(c->circw[idx], 1, cname_map[idx]);

        x = 0;
        for (j = I_PRESSURE_IN; j <= I_ENABLE_BTN; j += 1) {
          if (isbset(c->circuits[idx].io, j)) attron(COLOR_PAIR(j));
          mvwprintw(c->circw[idx], 2, x + 4, io_map[j - 1]);
          if (isbset(c->circuits[idx].io, j)) attroff(COLOR_PAIR(j));

          x += strlen(io_map[j - 1]);
        }

        for (j = 0; j < C_NUM_PARAM; j += 1) {
          pbuf[0] = '\0';
          sprintf(pbuf, "%s:  %.02lf", param_map[j], c->circuits[idx].params[j]);
          wclrtorng(c->circw[idx], 2 * j + 3, 1, CWIN_WIDTH - 1);
          center_str(c->circw[idx], 2 * j + 3, pbuf);
          center_str(c->circw[idx], 2 * j + 4, "-----------");
          wrefresh(c->circw[idx]);
        }

        wborder(c->circw[idx], '|', '|', '-', '-', '+', '+', '+', '+');
        wrefresh(c->circw[idx]);
        refresh();
      }
    }
  }

  curs_set(1);
}

void draw_simulator(client_t *c) {
  char pbuf[64];
  int i, l = 1;

  curs_set(0);

  center_str(c->simwin, l++, "SIMULATOR");
  center_str(c->simwin, l++, "-----------");

  pbuf[0] = '\0';

  center_str(c->simwin, l++, "MODE");
  for (i = 0; i < 8; i += 1) {
    if (isbset(c->sim.s_flags, i))
      strcat(pbuf, mode_map[i]);
  }
  center_str(c->simwin, l++, pbuf);
  center_str(c->simwin, l++, "-----------");

  pbuf[0] = '\0';

  center_str(c->simwin, l++, "UPDATE TYPES");
  for (i = 0; i < 3; i += 1) {
    if (isbset(c->sim.up_types, i)) 
      strcat(pbuf, update_map[i]);
  }
  center_str(c->simwin, l++, pbuf);
  center_str(c->simwin, l++, "-----------");

  center_str(c->simwin, l++, "UPTIME");
  sprintf(pbuf, "%lf s", ((double) c->sim.uptime) / 1000);
  center_str(c->simwin, l++, pbuf);
  center_str(c->simwin, l++, "-----------");

  wborder(c->simwin, '|', '|', '-', '-', '+', '+', '+', '+');
  wrefresh(c->simwin);

  curs_set(1);
}

void draw_packet(client_t *c) {
  char pbuf[64];
  int i, l = 1;

  curs_set(0);

  center_str(c->stwin, l++, "RX PACKET");
  center_str(c->stwin, l++, "-----------");

  pbuf[0] = '\0';

  center_str(c->stwin, l++, "TYPE");
  strcpy(pbuf, pktype_map[c->r->rx.type]);
  wclrtorng(c->stwin, l, 1, SWIN_WIDTH - 1);
  center_str(c->stwin, l++, pbuf);
  center_str(c->stwin, l++, "-----------");

  pbuf[0] = '\0';

  center_str(c->stwin, l++, "FLAGS");
  for (i = 0; i < 6; i += 1) {
    if (isbset(c->r->rx.flags, i)) {
      switch (c->r->rx.type) {
        case PK_UPDATE:
          strcat(pbuf, update_map[i]);
          break;
        case PK_COMMAND:
          strcat(pbuf, cmdtype_map[i]);
          break;
        case PK_STATUS:
          if (i == 0) {
            strcpy(pbuf, "ST_TIMEOUT");
          } else {
            strcpy(pbuf, "ST_PING");
          }
          break;
        default:
          break;
      }
    }
  }
  wclrtorng(c->stwin, l, 1, SWIN_WIDTH - 1);
  center_str(c->stwin, l++, pbuf);
  center_str(c->stwin, l++, "-----------");

  center_str(c->stwin, l++, "SIZE");
  sprintf(pbuf, "%d bytes", c->r->rx.size);
  wclrtorng(c->stwin, l, 1, SWIN_WIDTH - 1);
  center_str(c->stwin, l++, pbuf);
  center_str(c->stwin, l++, "-----------");

  wborder(c->stwin, '|', '|', '-', '-', '+', '+', '+', '+');
  wrefresh(c->stwin);

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


