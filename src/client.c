#include "client.h"

static char *const circuit_map[] = {
  "MARX",
  "MTG70",
  "MTG",
  "SWITCH",
  "SWTG70",
  "ALL",
  NULL
};

static char *const param_map[] = {
  "PRESSURE",
  "SET_POINT",
  "MAX_TIME",
  "CHECK_TIME",
  "PURGE_TIME",
  "DELAY_TIME",
  "PID_KP",
  "PID_KI",
  "PID_KD",
  "ALL",
  NULL
};

static char io_map[3][16] = {
  "INTAKE",
  "EXHAUST",
  "ENABLED"
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
  "SYSTEM",
  "CIRCUITS",
  "REMOTE"
};

static char uptype_map[5][32] = {
  "UP_SYSTEM",
  "UP_CIRCUITS",
  "UP_REMOTE",
  "UP_REFRESH",
  "UP_REMOVE"
};

static char cmdtype_map[6][32] = {
  "CMD_RESPND", 
  "CMD_MODESET",
  "CMD_PARSET", 
  "CMD_SAVE",   
  "CMD_GETLOG", 
  "CMD_DMPCF " 
};

static char pktype_map[4][32] = {
  "PK_UPDATE", 
  "PK_COMMAND",
  "PK_STATUS", 
  "PK_ACK"    
};

static struct option loptions[] = {
  {"circuit",   required_argument,  NULL, 'c'},
  {"system",    no_argument,        NULL, 's'},
  {"parameter", required_argument,  NULL, 'p'},
  {"refresh",   no_argument,        NULL, 'r'},
  {"modeset",   required_argument,  NULL, 'm'},
  {"parset",    required_argument,  NULL, 'v'},
  {"save",      no_argument,        NULL, 'a'},
  {"config",    no_argument,        NULL, 'g'},
  {"timeout",   required_argument,  NULL, 't'},
  {"ack",       no_argument,        NULL, 'k'},
  {"nping",     required_argument,  NULL, 'n'},
  {"ttl",       required_argument,  NULL, 'l'},
  { 0 }
};

static const char update_opts[] = "c:p:t:rsk";
static const char command_opts[] = "c:p:t:m:v:agk";
static const char ping_opts[] = "n:l:";

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
        c->circuit[idx].w = subwin(c->scr, CWIN_HEIGHT, CWIN_WIDTH, i * CWIN_HEIGHT, k * CWIN_WIDTH + 1);
      }
    }
  }

  return ret;
}

int process_key(client_t *c, int key) {
  int ret = 0;

  if (key != ERR) {
    // buffer user input
    if (isprint(key) && c->cursor < MAX_CMD_LEN - 1) {
      c->input[c->cursor++] = (char) key;
      c->input[c->cursor] = '\0';
    }

    switch (key) {
      case 13:
      case KEY_ENTER:
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

int parse_input(packet_args *pargs, int ac, char *av[], char *optstring) {
  int opidx = 0, opt, ret = 0;
  int cidx, pidx, midx, tidx;
  char *subopts, *sopt, *value;

  // reset optind for reinitializing getopt state
  optind = 0;

  while ((opt = getopt_long(ac, av, optstring, loptions, &opidx)) != -1) {
    switch (opt) {
      case 's':   /* --system, -s */
        pargs->op_flags |= (1 << UP_SYSTEM); break;
      case 'c':   /* --circuit, -c */
        pargs->op_flags |= (1 << UP_CIRCUITS);

        subopts = optarg;
        while (*subopts != '\0') {
          tidx = getsubopt(&subopts, circuit_map, &value);

          if (tidx != -1) {
            if (tidx == C_NUM_CIRCUITS)   // 'ALL' argument
              pargs->cmask = (1 << C_NUM_CIRCUITS) - 1;
            else
              pargs->cmask |= (1 << tidx); 
          } else {
            ret = -1;
          }
        }
        
        break;
      case 'p':   /* --parameter, -p */
        subopts = optarg;
        while (*subopts != '\0') {
          tidx = getsubopt(&subopts, param_map, &value);

          if (tidx != -1) {
            if (tidx == C_NUM_PARAM) 
              pargs->pmask = (1 << C_NUM_PARAM) - 1;
            else
              pargs->pmask |= (1 << tidx); 
          } else {
            ret = -1;
          }
        }
        break;
      case 'r':   /* --refresh, -r */
        pargs->op_flags |= (1 << UP_REFRESH); break;          
      case 'm':   /* --modeset, -m */
        pargs->op_flags |= (1 << CMD_MODESET);
        if ((midx = mapstr(mode_map, S_NUM_MODES, optarg)) != -1) {
          // stores modeset operation/flag and value info for later processing
          pargs->values[pargs->next_val].op = CMD_MODESET;
          pargs->values[pargs->next_val].flag = CMD_MODESET;
          pargs->values[pargs->next_val].value = midx;
          pargs->next_val += 1;
        }
        else
          ret = -1;
        break;
      case 'v':   /* --parset, -v */
        pargs->op_flags |= (1 << CMD_PARSET);

        subopts = optarg;
        while (*subopts != '\0') {
          tidx = getsubopt(&subopts, param_map, &value);

          if (tidx != -1 && value) {
            if (tidx == C_NUM_PARAM) {
              pargs->pmask = (1 << C_NUM_PARAM) - 1;
            } else {
              pargs->pmask |= (1 << tidx); 
            }

            if (sscanf(value, "%lf", &pargs->values[pargs->next_val].value) != -1) {
              // if successfully scanned value, store parset operation and flag for later processing
              pargs->values[pargs->next_val].op = CMD_PARSET;
              pargs->values[pargs->next_val].flag = tidx;
              pargs->next_val += 1;
            }
          } else {
            ret = -1;
          }
        }
        break;
      case 'a':   /* --save, -s */
        pargs->op_flags |= (1 << CMD_SAVE); break;
      case 'f':   /* --config, -f */
        pargs->op_flags |= (1 << CMD_DMPCFG); break;
      case 'l':   /* --ttl, -l  */
      case 't':   /* --timeout, -t */
        if (sscanf(optarg, "%u", &pargs->timeout) == -1) {
          pargs->timeout = 0;
          ret = -1;
        }
        break;
      case 'k':   /* --ack, -k */
        pargs->req_ack = 1; break;
      case 'n':   /* --nping, -n */
        pargs->op_flags |= (1 << ST_PING);
        if (sscanf(optarg, "%lf", &pargs->values[pargs->next_val].value) != -1) {
          pargs->values[pargs->next_val].op = ST_PING;
          pargs->values[pargs->next_val].flag = ST_PING;
          pargs->next_val += 1;
        } else {
          ret = -1;
        }
        break;
      case '?':   /* unknown/invalid option */
        ret = -1;
        break;
    }
  }

  return ret;
}

int process_input(client_t *c, char *cmd) {
  packet_args pargs = { 0 };
  char *av[MAX_ARGS], *optstring;
  int ret = 0, ac;
  
  if (!cmd) return -1;

  ac = tokenize_cmd(cmd, av);
  
  if (ac < 0) {
    printf("failed to tokenize command\n");
    ret = -1;
  } else {
    if (strcmp(av[0], "update") == 0) {
      optstring = update_opts;
      pargs.op_type = PK_UPDATE;
    } else if (strcmp(av[0], "do") == 0) {
      optstring = command_opts;
      pargs.op_type = PK_COMMAND;
    } else if (strcmp(av[0], "ping") == 0) {
      optstring = ping_opts;
      pargs.op_type = PK_STATUS;
    } else {
      return -1;
    }

    if (parse_input(&pargs, ac, av, optstring) == 0) {
      // construct packet
      construct_packet(&c->r.tx, &pargs);
    } else {
      // invalid input
      c->s_flags |= (1 << S_EINPUT);
      printf("fuck\n");
    }

    for (int i = 0; i < ac; i += 1) {
      free(av[i]);
    }
  }

  return ret;
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
    if (strcmp(map[i], str) == 0) return i;
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

int val_seek(valset_t *base, valset_t **vhead, int op, int len) {
  int i = 0;

  // searches value array @ vhead for first valset w/ equal op
  while (*vhead < base + len && (*vhead)->op != op) {
    *vhead += 1;
  }

  // counts length of contiguous valset's from base w/ equal op
  while (*vhead + i < base + len && (*vhead + i)->op == op) {
    i += 1;
  }

  return i;
}

int val_cmp_flag(const void *a, const void *b) {
  const valset_t *av = *(const valset_t **)a;
  const valset_t *bv = *(const valset_t **)b;
  
  return av->flag - bv->flag;
}

int val_cmp_op(const void *a, const void *b) {
  const valset_t *av = (const valset_t *)a;
  const valset_t *bv = (const valset_t *)b;
  
  return av->flag - bv->flag;
}

int val_cmp_value(const void *a, const void *b) {
  const valset_t *av = (const valset_t *)a;
  const valset_t *bv = (const valset_t *)b;
  
  return av->flag - bv->flag;
}

size_t construct_packet(packet_t *p, packet_args *pargs) {
  int i, k, l, s;
  uint8_t *pdata = p->bytes;
  valset_t *seek, *sort[MAX_VALUES];

  p->type = pargs->op_type;
  p->flags = pargs->op_flags;
  p->timeout = pargs->timeout;

  switch (pargs->op_type) {
    case PK_UPDATE:
      if (isbset(p->flags, UP_CIRCUITS)) {
        // store update masks
        *pdata++ = pargs->cmask;
        *pdata++ = pargs->pmask >> 8;
        *pdata++ = pargs->pmask;
      }
      break;
    case PK_COMMAND:
      if (pargs->req_ack) p->flags |= (1 << CMD_RESPND);

      for (i = 0; i < 8; i += 1) {
        if (isbset(pargs->op_flags, i)) {
          switch (i) {
            case CMD_MODESET:
              seek = pargs->values;
              // seek to first CMD_MODESET option
              l = val_seek(pargs->values, &seek, CMD_MODESET, pargs->next_val);
              // load into packet
              *pdata++ = (uint8_t) seek->value;
              break;
            case CMD_PARSET:
              s = 0;
              seek = pargs->values;

              // seek to next element with CMD_PARSET op, repeat while parset-runs remain
              while ((l = val_seek(pargs->values, &seek, CMD_PARSET, pargs->next_val))) {
                // store pointers into array for sorting
                while (l--) sort[s++] = seek++;
              }

              // sort valset pointer array keyed on flag/parameter
              qsort(sort, s, sizeof(valset_t *), val_cmp_flag);
              for (int m = 0; m < s; m += 1) printf("sorted[%d] -> %d %d %lf\n", m, sort[m]->op, sort[m]->flag, sort[m]->value);
              // iterate and load values into packet
              for (l = 0; l < s; l += 1) {
                *pdata++ = (uint8_t) sort[l]->value;
                *pdata++ = (uint8_t) ((sort[l]->value - (uint8_t)sort[l]->value) * 100);
              }
              break;
            case CMD_SAVE:
            case CMD_GETLOG:
            case CMD_DMPCFG:
              break;
          }
        }
      }
    case PK_STATUS:
      break;
  }

  p->size = pdata - p->bytes;

  return p->size;
}

size_t construct_command(packet_t *p, packet_args *a, int ack, uint8_t timeout) {
  double *data = a->pdata;
  uint8_t *pdata = p->bytes;
  
  p->type = PK_COMMAND;
  p->flags = a->op_flags;

  if (ack) {
    p->flags |= (1 << CMD_RESPND);
  }

  for (int i = 0; i < 8; i += 1) {
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
        center_str(c->circuit[idx].w, 1, circuit_map[idx]);

        x = 0;
        for (j = I_PRESSURE_IN; j <= I_ENABLE_BTN; j += 1) {
          if (isbset(c->circuit[idx].io, j)) attron(COLOR_PAIR(j));
          mvwprintw(c->circuit[idx].w, 2, x + 4, io_map[j - 1]);
          if (isbset(c->circuit[idx].io, j)) attroff(COLOR_PAIR(j));

          x += strlen(io_map[j - 1]);
        }

        for (j = 0; j < C_NUM_PARAM; j += 1) {
          pbuf[0] = '\0';
          sprintf(pbuf, "%s:  %.02lf", param_map[j], c->circuit[idx].params[j]);
          wclrtorng(c->circuit[idx].w, 2 * j + 3, 1, CWIN_WIDTH - 1);
          center_str(c->circuit[idx].w, 2 * j + 3, pbuf);
          center_str(c->circuit[idx].w, 2 * j + 4, "-----------");
          wrefresh(c->circuit[idx].w);
        }

        wborder(c->circuit[idx].w, '|', '|', '-', '-', '+', '+', '+', '+');
        wrefresh(c->circuit[idx].w);
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

  // center_str(c->simwin, l++, "SIMULATOR");
  // center_str(c->simwin, l++, "-----------");

  // pbuf[0] = '\0';

  // center_str(c->simwin, l++, "MODE");
  // for (i = 0; i < 8; i += 1) {
  //   if (isbset(c->sim.s_flags, i))
  //     strcat(pbuf, mode_map[i]);
  // }
  // center_str(c->simwin, l++, pbuf);
  // center_str(c->simwin, l++, "-----------");

  // pbuf[0] = '\0';

  // center_str(c->simwin, l++, "UPDATE TYPES");
  // for (i = 0; i < 3; i += 1) {
  //   if (isbset(c->sim.up_types, i)) 
  //     strcat(pbuf, update_map[i]);
  // }
  // center_str(c->simwin, l++, pbuf);
  // center_str(c->simwin, l++, "-----------");

  // center_str(c->simwin, l++, "UPTIME");
  // sprintf(pbuf, "%lf s", ((double) c->sim.uptime) / 1000);
  // center_str(c->simwin, l++, pbuf);
  // center_str(c->simwin, l++, "-----------");

  // wborder(c->simwin, '|', '|', '-', '-', '+', '+', '+', '+');
  // wrefresh(c->simwin);

  curs_set(1);
}

void draw_packet(client_t *c) {
  char pbuf[64];
  int i, l = 1;

  curs_set(0);

  // center_str(c->stwin, l++, "RX PACKET");
  // center_str(c->stwin, l++, "-----------");

  // pbuf[0] = '\0';

  // center_str(c->stwin, l++, "TYPE");
  // strcpy(pbuf, pktype_map[c->r->rx.type]);
  // wclrtorng(c->stwin, l, 1, SWIN_WIDTH - 1);
  // center_str(c->stwin, l++, pbuf);
  // center_str(c->stwin, l++, "-----------");

  // pbuf[0] = '\0';

  // center_str(c->stwin, l++, "FLAGS");
  // for (i = 0; i < 6; i += 1) {
  //   if (isbset(c->r->rx.flags, i)) {
  //     switch (c->r->rx.type) {
  //       case PK_UPDATE:
  //         strcat(pbuf, update_map[i]);
  //         break;
  //       case PK_COMMAND:
  //         strcat(pbuf, cmdtype_map[i]);
  //         break;
  //       case PK_STATUS:
  //         if (i == 0) {
  //           strcpy(pbuf, "ST_TIMEOUT");
  //         } else {
  //           strcpy(pbuf, "ST_PING");
  //         }
  //         break;
  //       default:
  //         break;
  //     }
  //   }
  // }
  // wclrtorng(c->stwin, l, 1, SWIN_WIDTH - 1);
  // center_str(c->stwin, l++, pbuf);
  // center_str(c->stwin, l++, "-----------");

  // center_str(c->stwin, l++, "SIZE");
  // sprintf(pbuf, "%d bytes", c->r->rx.size);
  // wclrtorng(c->stwin, l, 1, SWIN_WIDTH - 1);
  // center_str(c->stwin, l++, pbuf);
  // center_str(c->stwin, l++, "-----------");

  // wborder(c->stwin, '|', '|', '-', '-', '+', '+', '+', '+');
  // wrefresh(c->stwin);

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
      circuit = &c->circuit[i];
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


