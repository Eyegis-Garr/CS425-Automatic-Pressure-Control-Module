#include "client.h"

int process_key(client_t *c, int key) {
  int ret = 0;

  if (key != ERR) {
    // buffer user input
    if ((isprint(key) || key == 10) && c->cursor < MAX_CMD_LEN - 1) {
      c->cmd_input[c->cursor++] = (char) key;
      c->cmd_input[c->cursor] = '\0';
    }

		// process control keys
    switch (key) {
      case 27:
        c->s_flags |= (1 << S_EXIT);
        break;
			case 10:
      case 13:
      case KEY_ENTER:
        ret = 1;
        break;
      case KEY_BACKSPACE:
      case KEY_DC:
      case 127:
        if (c->cursor > 0) c->cmd_input[--c->cursor] = '\0';
        break;
      case 9:
        c->s_flags ^= (1 << S_POLL);
      default:
        break;
    }
  }

  return ret;
}

int process_input(client_t *c, char *cmd) {
  char *av[MAX_ARGS];
  int ret = 0, ac;
  
  if (!cmd) return E_INPUT;

  ac = tokenize_cmd(cmd, av);

  if (ac < 0) {
    ret = E_TOKEN;
  } else {
    memset(&c->pargs, 0, sizeof(packet_args));
    if (strcmp(av[0], "update") == 0) {
			// parse update input command
      c->pargs.op_type = PK_UPDATE;

      ret = parse_update(&c->pargs, ac, av);
			// ret = parse_input(&c->pargs, ac, av, update_opts);
    } else if (strcmp(av[0], "do") == 0) {
			// parse command input command
      c->pargs.op_type = PK_COMMAND;

			ret = parse_command(&c->pargs, ac, av);
    } else if (strcmp(av[0], "ping") == 0) {
			// parse ping input command
      c->pargs.op_type = PK_STATUS;
      c->s_flags |= (1 << S_PING);

			ret = parse_ping(&c->pargs, ac, av);
    } else if (strcmp(av[0], "exit") == 0) {
      c->s_flags |= (1 << S_EXIT);
    } else if (strcmp(av[0], "help") == 0) {
      draw_help(c, (ac > 0) ? av[1] : NULL);
      ret = -1;
    } else {
      ret = E_INPUT;
    }

    for (int i = 0; i < ac; i += 1) {
      free(av[i]);
    }
  }

  return ret;
}

int process_update(client_t *c, packet_t *p) {
  uint8_t *bytes = p->packet.data;

  if (isbset(p->packet.flags, UP_SYSTEM)) {
    // load system state variables
    bytes += update_system(c, bytes);
  } if (isbset(p->packet.flags, UP_CIRCUITS)) {
    bytes += update_circuits(c, bytes);
  } if (isbset(p->packet.flags, UP_REMOTE)) {
    // load system remote/packet state (not a priority)
  }

  return bytes != p->packet.data;
}

int process_packet(client_t *c, packet_t *p) {
  int ret = 0;

  switch (p->packet.type) {
    case PK_UPDATE:
      // filter and push update
      ret = process_update(c, p);
      break;
    case PK_STATUS:
      // if timeout, flag for retransmit?
      if (isbset(p->packet.flags, ST_TIMEOUT)) {
        c->err = E_RETRY;
        ret = E_RETRY;
      }
      break;
    default:
      break;
  }

  return ret;
}

int process_error(client_t *c) {
	switch (c->err) {
		case E_INPUT:
			sprintf(c->err_header, "INPUT ERROR (%d)", c->err);
			sprintf(c->err_message, "Failed to process input.");
			break;
		case E_TIMEOUT:
			sprintf(c->err_header, "PACKET TIMED OUT (%d)", c->err);
			sprintf(c->err_message, "%s packet timed out. TTL -> %d", pktype_map[c->r.tx.packet.type], c->r.tx.packet.timeout);
			break;
		case E_TOKEN:
			sprintf(c->err_header, "TOKENIZING ERROR (%d)", c->err);
			sprintf(c->err_message, "Failed to tokenize input.");
			break;
		case E_OPTION:	
			sprintf(c->err_header, "UNKNOWN OPTION (%d)", c->err);
			sprintf(c->err_message, "Invalid usage. See --help,-h for valid options.");
			break;
		case E_SUBOPT:
			sprintf(c->err_header, "UNKNOWN SUBOPTION (%d)", c->err);
			sprintf(c->err_message, "Unrecognized suboption. See --help,-h for valid usage.");
			break;
    case E_CONNECT:
      sprintf(c->err_header, "DISCONNECTED (%d)", c->err);
      sprintf(c->err_message, "Couldn't connect to serial device %s.", c->r.dev_path);
      break;
    case E_PCREATE:
      sprintf(c->err_header, "PACKET CONSTRUCT (%d)", c->err);
      sprintf(c->err_message, "Failed to create %s-type packet.", pktype_map[c->r.tx.packet.type]);
      break;
    case E_ARGUMENT:
      sprintf(c->err_header, "MISSING ARGUMENT (%d)", c->err);
      sprintf(c->err_message, "Option is missing argument.");
      break;
    case E_RETRY:
      sprintf(c->err_header, "PACKET TIMEOUT (%d)", c->err);
      sprintf(c->err_message, "Packet timed out. Resend? (Y/N)");
      if (c->key == 'Y' || c->key == 'y') {
        c->s_flags |= (1 << S_RETRY);
      } else {
        c->s_flags &= ~(1 << S_RETRY);
      }
      break;
    default:
      *c->err_header = '\0';
      *c->err_message = '\0';
      break;
	}

  if (c->key != ERR) {
    werase(c->cmd); wrefresh(c->cmd);
    sprintf(c->err_header, "INPUT");
    c->cmd_input[0] = '\0';
    c->cursor = 0;
    c->err = 0;
  } else {
    draw_err(c);
  }

	return c->err;	
}
