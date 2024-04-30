/**
 * @file client.c
 * @author Bradley Sullivan (bradleysullivan@nevada.unr.edu)
 * @brief Core client functionalities
 * @version 0.1
 * @date 2024-04-24
 * 
 * @copyright Copyright (c) 2024
 * 
 */

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

/**
 * @brief Allocates and/or initializes client structures.
 * If 'c' is provided NULL, a structure is allocated and
 * returned.
 * 
 * @param c Pointer to client structure to initialize
 * @param dev_buf Path to serial device
 * @return client_t* Pointer to allocated/initialized structure
 */
client_t *c_get(client_t *c, char *dev_buf) {
  client_t *ret;
  int i;

  if (!c)
    ret = malloc(sizeof(client_t));
  else
    ret = c;

  if (!ret) {
    printf("Failed to allocate client structure...\n");
    return NULL;
  }

  ret->state = 0;
  ret->cursor = 0;
  ret->cmd_input[0] = '\0';
  ret->poll_time = 10;
  ret->err = 0;
  strncpy(ret->dev_buf, dev_buf, 32);

  ret->remote = r_get(NULL, dev_buf, B9600);

  // init curses
  ret->scr = initscr();
  noecho();
  cbreak();
  keypad(ret->scr, TRUE);
  nodelay(ret->scr, TRUE);
  curs_set(0);
  start_color();

  for (i = 0; i < C_NUM_CIRCUITS; i += 1) {
    ret->circuit[i] = (circuit_t) { .params[0] = 0, .params[1] = C_PRESSURE_MAX };
    ret->circuit[i].w = subwin(ret->scr, CWIN_HEIGHT, CWIN_WIDTH, i * CWIN_HEIGHT, 0);
    wborder(ret->circuit[i].w, 0, 0, 0, 0, 0, 0, 0, 0);
  }

  ret->sim_win = subwin(ret->scr, CWIN_HEIGHT, CWIN_WIDTH, C_NUM_CIRCUITS * CWIN_HEIGHT, 0);
  wborder(ret->sim_win, 0, 0, 0, 0, 0, 0, 0, 0);

  ret->view_win = subwin(ret->scr, SWIN_HEIGHT, SWIN_WIDTH - 2, CWIN_HEIGHT, CWIN_WIDTH + 1);
  ret->view = subwin(ret->scr, SWIN_HEIGHT - 2, SWIN_WIDTH - 4, CWIN_HEIGHT + 1, CWIN_WIDTH + 2);
  wborder(ret->view_win, 0, 0, 0, 0, 0, 0, 0, 0);
  scrollok(ret->view, TRUE);

  ret->cmd = subwin(ret->scr, CWIN_HEIGHT, CWIN_WIDTH - 2, 0, CWIN_WIDTH + 1);

  return ret;
}

/**
 * @brief Configures provided packet with packet 
 * arguments. Packet arguments are created by parsing
 * user-input commands.
 * 
 * @param c Active client structure
 * @param p Packet-refreence to construct/configure
 * @param pargs Arguments detailing configuration
 * @return int Returns 0 on success, -1 on error. Sets
 * client 'err'.
 */
int construct_packet(client_t *c, packet_t *p, packet_args *pargs) {
  int i, l, s, ret;
  uint8_t *pdata;
  valset_t *seek, *sort[MAX_VALUES];

  ret = c->err;

  p->packet.type = pargs->op_type;
  p->packet.flags = pargs->op_flags;
  p->packet.timeout = pargs->timeout;

  pdata = p->packet.data;

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
              c->err = E_PCREATE;
              break;
          }
        }
      }
      break;
    case PK_STATUS:
      if (isbset(pargs->op_flags, ST_PING)) {
        seek = pargs->values;
        if (val_seek(pargs->values, &seek, ST_PING, pargs->next_val)) {
          *pdata++ = (uint8_t) seek->value;
        } else {
          *pdata++ = 1;
        }
      }
      break;
    default:
      c->err = E_PCREATE;
      break;
  }

  p->packet.size = pdata - p->packet.data;

  return ret == c->err ? 0 : -1;
}

/**
 * @brief Client tick routine. Processes new
 * remote data, user-input, errors, and device
 * connection.
 * 
 * @param c Active client
 */
void update_client(client_t *c) {
  int i;
  
  c->key = wgetch(c->scr);

  if (isbset(c->remote->state, R_NDATA)) {
    c->receive = r_read(c->remote);
    if (c->receive) {
      process_packet(c, c->receive);
      c->remote->state &= ~(1 << R_NDATA);
    }
  }

  // processes client state updates
  for (i = 0; i < 8; i += 1) {
    if (isbset(c->state, i)) {
      switch (i) {
        case S_EXIT:
          c->remote->state |= (1 << R_EXIT);
          break;
        case S_INPUT:   /* user input/command available to process */
          print_view(c, c->cmd_input);
          if (process_input(c, c->cmd_input) == 0)  {
            // construct and transmit packet(s)
            if (construct_packet(c, c->remote->tx_head, &c->pargs) == 0) {
              c->remote->state |= (1 << R_FLUSH);
            }
          }

          // reset user-input state
          c->state &= ~(1 << S_INPUT);
          c->cmd_input[0] = '\0';
          c->cursor = 0;
          break;
        case S_RECON:
          print_view(c, "Attempting reconnect...\n");
          if (reconnect(c->remote, c->dev_buf, 5, 1000) < 0) {
            c->err = E_CONNECT;
            print_view(c, "Failed to reconnect to serial device.\n");
          } else {
            print_view(c, "Serial device reconnected!\n");
          }

          c->state &= ~(1 << S_RECON);
        default:
          break;
      }
    }
  }

  if (c->err) {
    process_error(c);
  } else {
    if (process_key(c, c->key)) {                    // returns 1 on key ENTER
      c->state |= (1 << S_INPUT);
    }
    draw_input(c);    
  }
}

size_t update_circuits(client_t *c, uint8_t *bytes) {
  uint8_t *pdata = bytes;
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
      for (uint8_t k = 0; k < C_NUM_PARAM; k += 1) {
        if (isbset(pmask, k)) {
          // load parameter into enabled circuit
          circuit->params[k] = *pdata * 100;
          circuit->params[k] += *(pdata + 1);
          circuit->params[k] /= 100;
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
  int i;

  // read system flags
  c->sim.state = *sbytes++;
  c->sim.err = *sbytes++;
  c->sim.c_flags = *sbytes++;
  c->sim.p_flags = *sbytes++;
  c->sim.p_flags <<= 8;
  c->sim.p_flags = *sbytes++;
  c->sim.en_flags = *sbytes++;
  // read last recieved client update flags
  c->sim.up_types = *sbytes++;
  // read system uptime
  c->sim.uptime = 0;
  for (i = 0; i < 4; i += 1) {
    c->sim.uptime |= *sbytes++;
    c->sim.uptime <<= 8;
  }
  // read system reclaimer/supply state
  c->sim.reclaimer = (float)*sbytes + ((float)(*(sbytes + 1)) / 100);
  sbytes += 2;
  c->sim.supply = (float)*sbytes + ((float)(*(sbytes + 1)) / 100);
  sbytes += 2;
  c->sim.rec_auto_on = (float)*sbytes + ((float)(*(sbytes + 1)) / 100);
  sbytes += 2;
  c->sim.rec_auto_off = (float)*sbytes + ((float)(*(sbytes + 1)) / 100);
  sbytes += 2;
  c->sim.supply_min = (float)*sbytes + ((float)(*(sbytes + 1)) / 100);
  sbytes += 2;

  return sbytes - bytes;
}


