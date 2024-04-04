#include "client.h"

char *const circuit_map[] = {
  "MARX",
  "MTG70",
  "MTG",
  "SWITCH",
  "SWTG70",
  "ALL",
  NULL
};

char *const param_map[] = {
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

char *const io_map[] = {
  "INTAKE",
  "EXHAUST",
  "ENABLED"
};

char *const mode_map[] = {
  "SHOT",
  "ABORT",
  "PURGE",
  "ALARM",
  "RECLAIM",
  "STANDBY",
  "ERROR",
  "REMOTE",
  NULL
};

char *const flag_map[][7] = {
  { /* update flags */
    "UP_SYSTEM",
    "UP_CIRCUITS",
    "UP_REMOTE",
    "UP_REFRESH",
    "UP_REMOVE",
    NULL
  },

  { /* command flags */
    "CMD_RESPND", 
    "CMD_MODESET",
    "CMD_PARSET", 
    "CMD_SAVE",   
    "CMD_GETLOG", 
    "CMD_DMPCFG",
    NULL
  },

  { /* status flags */
    "ST_TIMEOUT",
    "ST_PING",
    NULL
  },

  {
    NULL
  }
};

char *const status_map[] = {
  "ST_TIMEOUT",
  "ST_PING",
  NULL
};

char *const pktype_map[] = {
  "PK_UPDATE", 
  "PK_COMMAND",
  "PK_STATUS", 
  "PK_ACK",
  NULL
};

int init_client(client_t *c, char *dev_path) {
  int ret = 0;

  c->s_flags = 0;
  c->cursor = 0;

  // init curses
  c->scr = initscr();
  noecho();
  cbreak();
  keypad(c->scr, TRUE);
  nodelay(c->scr, TRUE);
  curs_set(0);
  start_color();

  int i;
  for (i = 0; i < C_NUM_CIRCUITS; i += 1) {
    c->circuit[i].w = subwin(c->scr, CWIN_HEIGHT, CWIN_WIDTH, i * CWIN_HEIGHT, 0);
    wborder(c->circuit[i].w, 0, 0, 0, 0, 0, 0, 0, 0);
  }

  c->view_win = subwin(c->scr, SWIN_HEIGHT, SWIN_WIDTH - 2, CWIN_HEIGHT, CWIN_WIDTH + 1);
  c->view = subwin(c->scr, SWIN_HEIGHT - 2, SWIN_WIDTH - 4, CWIN_HEIGHT + 1, CWIN_WIDTH + 2);
  wborder(c->view_win, 0, 0, 0, 0, 0, 0, 0, 0);
  scrollok(c->view, TRUE);

  c->cmd = subwin(c->scr, CWIN_HEIGHT, CWIN_WIDTH - 2, 0, CWIN_WIDTH + 1);

  ret = init_remote(&c->r, dev_path, B9600);

  return ret;
}

int construct_packet(client_t *c, packet_t *p, packet_args *pargs) {
  int i, l, s, ret;
  uint8_t *pdata = p->packet.data;
  valset_t *seek, *sort[MAX_VALUES];

  p->packet.type = pargs->op_type;
  p->packet.flags = pargs->op_flags;
  p->packet.timeout = pargs->timeout;
  ret = 0;

  switch (pargs->op_type) {
    case PK_UPDATE:
      if (isbset(p->packet.flags, UP_CIRCUITS)) {
        // store update masks
        *pdata++ = pargs->cmask;
        *pdata++ = pargs->pmask >> 8;
        *pdata++ = pargs->pmask;
      }
      break;
    case PK_COMMAND:
      if (pargs->req_ack) p->packet.flags |= (1 << CMD_RESPND);

      for (i = 0; i < 8; i += 1) {
        if (isbset(pargs->op_flags, i)) {
          switch (i) {
            case CMD_MODESET:
              seek = pargs->values;
              // seek to first CMD_MODESET option
              if (val_seek(pargs->values, &seek, CMD_MODESET, pargs->next_val))
                // load into packet
                *pdata++ = (uint8_t) seek->value;
              break;
            case CMD_PARSET:
              s = 0;
              seek = pargs->values;
              *pdata++ = pargs->cmask;
              *pdata++ = pargs->pmask >> 8;
              *pdata++ = pargs->pmask;

              // seek to next element with CMD_PARSET op, repeat while parset-runs remain
              while ((l = val_seek(pargs->values, &seek, CMD_PARSET, pargs->next_val))) {
                // store pointers into array for sorting
                while (l--) sort[s++] = seek++;
              }

              // sort valset pointer array keyed on flag/parameter
              qsort(sort, s, sizeof(valset_t *), val_cmp_flag);
              // for (int m = 0; m < s; m += 1) printf("sorted[%d] -> %d %d %lf\n", m, sort[m]->op, sort[m]->flag, sort[m]->value);
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
            default:
              ret = E_PCREATE;
              break;
          }
        }
      }
      break;
    case PK_STATUS:
      if (isbset(pargs->op_flags, ST_PING)) {
        seek = pargs->values;
        if (val_seek(pargs->values, &seek, ST_PING, pargs->next_val))
          *pdata++ = (uint8_t) seek->value;
      }
      break;
    default:
      ret = E_PCREATE;
      break;
  }

  p->packet.size = pdata - p->packet.data;

  return ret;
}

int ping(client_t *c, packet_t *p) {
  int replies = 0;
  while (p->packet.data[0]--) {
    tx_packet(&c->r);

    if (poll_resp(&c->r, p->packet.timeout * 1000) > 0) {
      if (isbset(c->r.rx.packet.flags, ST_PING)) {
        sprintf(c->print_buf, "Ping reply: bytes=%d  TTL=%ds\n", PS_HEADER + p->packet.size, p->packet.timeout);
        print_view(c, c->print_buf);
        replies += 1;
      } else if (isbset(c->r.rx.packet.flags, ST_TIMEOUT)) {
        sprintf(c->print_buf, "Ping timed out (TTL=%ds)\n", p->packet.timeout);
        print_view(c, c->print_buf);
      } 
    } else {
      print_view(c, "Device poll timed out\n");
    }
  }

  return replies;
}

void update_client(client_t *c) {
  int i, r, p;

  c->key = wgetch(c->scr);

  // processes client state updates
  for (i = 0; i < 8; i += 1) {
    if (isbset(c->s_flags, i)) {
      switch (i) {
        case S_EXIT:
          // dealloc client stuff
          break;
        case S_POLL:
          if (isbclr(c->r.r_flags, R_EXIT)) {
            if (poll_resp(&c->r, 1) > 0) {
              process_packet(c, &c->r.rx);
            }
          }
          break;
        case S_INPUT:
          c->err = process_input(c, c->cmd_input);
          if (!c->err)  {
            // construct and transmit packet(s)
            c->err = construct_packet(c, &c->r.tx, &c->pargs);
            if (!c->err) {
              if (isbclr(c->r.r_flags, R_RXINP)) {
                // draw_packet(c, &c->r.tx);
                tx_packet(&c->r);
              } else {
                c->s_flags |= (1 << S_RETRY);
              }
            }
          }
          c->s_flags &= ~(1 << S_INPUT);
          c->cmd_input[0] = '\0';
          c->cursor = 0;
          break;
        case S_UPCYCLE:
        case S_PING:
          // ping system for nping packets
          p = c->r.tx.packet.data[0];
          sprintf(c->print_buf, "\nPINGING SYSTEM: n=%d  TTL=%ds\n", p, c->r.tx.packet.timeout);
          print_view(c, c->print_buf);
          r = ping(c, &c->r.tx);
          sprintf(c->print_buf, "\nRECEIVED: %d of %d replies (%.02lf%%)\n", r, p, 100 * (double)r / p);
          print_view(c, c->print_buf);
          c->s_flags &= ~(1 << S_PING);
          break;
        case S_RETRY:
          if (isbclr(c->r.r_flags, R_RXINP)) {
            tx_packet(&c->r);
            c->s_flags &= ~(1 << S_RETRY);
          }
          break;
        default:
          break;
      }
    }
  }

  
  if (c->err) {
    process_error(c);
  } else {
    if (process_key(c, c->key)) {                    // returns 1 on key ENTER
      c->s_flags |= (1 << S_INPUT);
    }
  }
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


