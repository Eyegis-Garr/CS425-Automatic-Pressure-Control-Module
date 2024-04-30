/**
 * @file processing.c
 * @author Bradley Sullivan (bradleysullivan@nevada.unr.edu)
 * @brief Input and error processing routine implementations
 * @version 0.1
 * @date 2024-04-24
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "client.h"

/**
 * @brief Client's main user key-processing
 * routine. Buffers printable user-keypresses
 * into client 'cmd_input' buffer @ client 'cursor'
 * position. 
 * 
 * Sets S_EXIT on ESC keypress
 * 
 * @param c Active client structure
 * @param key User-keypress curses keycode
 * @return int Returns 1 on ENTER, 0 otherwise
 */
int process_key(client_t *c, int key) {
  int ret = 0;

  if (key != ERR) {
    // buffer user input
    if (isprint(key) && c->cursor < MAX_CMD_LEN - 1) {
      c->cmd_input[c->cursor++] = (char) key;
      c->cmd_input[c->cursor] = '\0';
    }

		// process control keys
    switch (key) {
      case 27:
        c->state |= (1 << S_EXIT);
        break;
			case 10:
        c->cmd_input[c->cursor++] = '\n';
        c->cmd_input[c->cursor] = '\0';
        ret = 1;
        break;
      case KEY_BACKSPACE:
      case KEY_DC:
      case 127:
        if (c->cursor > 0) c->cmd_input[--c->cursor] = '\0';
        break;
      default:
        break;
    }
  }

  return ret;
}

/**
 * @brief Processing routine for full user-input string.
 * Tokenizes, parses, and processes user commands.
 * 
 * Input strings are converted to uppercase with 'to_upper'
 * 
 * Sets client 'err' on error.
 * 
 * @param c Active client structure
 * @param cmd User input-string
 * @return int Returns 0 on parsing success, client E_<ERROR> on error,
 * -1 for no further processing
 */
int process_input(client_t *c, char *cmd) {
  char *av[MAX_ARGS];
  int ret = 0, ac, i;

  if (!cmd || strlen(cmd) <= 1) return E_INPUT;

  ac = tokenize_cmd(cmd, av);

  to_upper(av[0]);

  if (ac < 0) {
    ret = E_TOKEN;
  } else {
    memset(&c->pargs, 0, sizeof(packet_args));
    if (strcmp(av[0], "UPDATE") == 0) {         /* update command */
      for (i = 0; i < ac; i += 1) to_upper(av[i]);
			// parse update input command
      c->pargs.op_type = PK_UPDATE;
      ret = parse_update(&c->pargs, ac, av);
    } else if (strcmp(av[0], "DO") == 0) {      /* do command */
      for (i = 0; i < ac; i += 1) to_upper(av[i]);
			// parse command input command
      c->pargs.op_type = PK_COMMAND;
			ret = parse_command(&c->pargs, ac, av);
    } else if (strcmp(av[0], "PING") == 0) {    /* ping command */
      for (i = 0; i < ac; i += 1) to_upper(av[i]);
			// parse ping input command
      c->pargs.op_type = PK_STATUS;
      c->state |= (1 << S_PING);

			ret = parse_ping(&c->pargs, ac, av);
    } else if (strcmp(av[0], "VIEW") == 0 && ac > 1) {    /* view command */
      for (i = 0; i < ac; i += 1) to_upper(av[i]);
      if (strcmp(av[1], "SYSTEM") == 0) {
        draw_system_view(c);
      } else {
        for (i = 0; i < ac; i += 1) {
          to_upper(av[i]);
          draw_circuit_view(c, mapstr(circuit_map, C_NUM_CIRCUITS, av[i]));
        }
      }
      ret = -1;
    } else if (strcmp(av[0], "EXIT") == 0) {    /* exit command */
      c->state |= (1 << S_EXIT);
      ret = -1;
    } else if (strcmp(av[0], "HELP") == 0) {    /* help command */
      draw_help(c, (ac > 1) ? av[1] : NULL);
      ret = -1;
    } else if (strcmp(av[0], "CONNECT") == 0) { /* connect command */
      if (ac > 1) strncpy(c->dev_buf, av[1], MAX_CMD_LEN);
      print_view(c, "Attempting reconnect...\n");
      if (reconnect(c->remote, c->dev_buf, 5, 1000) < 0) {
        ret = E_CONNECT;
        sprintf(c->print_buf, "Failed to connect to '%s'.\n", c->dev_buf);
        print_view(c, c->print_buf);
      } else {
        print_view(c, "Serial device reconnected!\n");
        ret = -1;
      }
    } else {
      ret = E_INPUT;
    }

    for (int i = 0; i < ac; i += 1) {
      free(av[i]);
    }
  }

  c->err = (ret > 0) ? ret : c->err;

  return ret;
}

/**
 * @brief Populates client system view with update
 * data in provided packet.
 * 
 * @param c Active client structure
 * @param p Packet to process
 * @return int Returns 0 for processed data, -1 otherwise
 */
int process_update(client_t *c, packet_t *p) {
  uint8_t *bytes = p->packet.data;

  if (isbset(p->packet.flags, UP_SYSTEM)) {
    // load system state variables
    bytes += update_system(c, bytes);
  } if (isbset(p->packet.flags, UP_CIRCUITS)) {
    bytes += update_circuits(c, bytes);
  }

  return bytes != p->packet.data ? 0 : -1;
}

/**
 * @brief Filters packet for processing. Processes
 * PK_UPDATE and PK_STATUS packets.
 * 
 * @param c Active client structure
 * @param p Packet to process
 * @return int Returns 0 on success, -1 on failure
 */
int process_packet(client_t *c, packet_t *p) {
  int ret = 0;

  switch (p->packet.type) {
    case PK_UPDATE:
      // filter and push update
      ret = process_update(c, p);
      break;
    case PK_STATUS:
      if (isbset(p->packet.flags, ST_PING)) {
        draw_packet(c, p);
        sprintf(c->print_buf, "Ping (%d) received: bytes=%d  TTL=%ds\n", p->packet.data[0], PS_HEADER + p->packet.size, p->packet.timeout);
        print_view(c, c->print_buf);
        if (p->packet.data[0]) {
          if (isbset(c->state, S_PING)) {
            // decrement ping seq. number
            p->packet.data[0] -= 1;
          }

          // copy ping packet, signal to echo
          memcpy(c->remote->tx_head->bytes, p->bytes, p->packet.size + PS_HEADER);
          c->remote->state |= (1 << R_FLUSH);
        }        
      }
      break;
    default:
      break;
  }

  return ret;
}

/**
 * @brief Processes displaying error messages to 
 * input area. Blocks until user-keypress/acknowledgement.
 * 
 * Clears client 'err' to 0 on acknowledgement.
 * 
 * @param c Active client structure
 * @return int Returns client 'err'
 */
int process_error(client_t *c) {
	switch (c->err) {
		case E_INPUT:
			sprintf(c->err_header, "INPUT ERROR (%d)", c->err);
			sprintf(c->err_message, "Failed to process input.");
			break;
		case E_TOKEN:
			sprintf(c->err_header, "TOKENIZING ERROR (%d)", c->err);
			sprintf(c->err_message, "Failed to tokenize input.");
			break;
		case E_OPTION:	
			sprintf(c->err_header, "UNKNOWN OPTION (%d)", c->err);
			sprintf(c->err_message, "Invalid usage. See help for valid options.");
			break;
		case E_SUBOPT:
			sprintf(c->err_header, "UNKNOWN SUBOPTION (%d)", c->err);
			sprintf(c->err_message, "Unrecognized suboption. See help for valid usage.");
			break;
    case E_NOTTY:
      sprintf(c->err_header, "NO SERIAL TERMINAL (%d)", c->err);
      sprintf(c->err_message, "No connection. Reconnect '%s' (Y/N)?", c->dev_buf);
      if (toupper(c->key) == 'Y') c->state |= (1 << S_RECON);
      break;
    case E_CONNECT:
      sprintf(c->err_header, "DISCONNECTED (%d)", c->err);
      sprintf(c->err_message, "Couldn't connect to serial device '%s'.", c->dev_buf);
      break;
    case E_PCREATE:
      sprintf(c->err_header, "PACKET CONSTRUCT (%d)", c->err);
      sprintf(c->err_message, "Failed to create %s-type packet.", pktype_map[c->pargs.op_type]);
      break;
    case E_ARGUMENT:
      sprintf(c->err_header, "MISSING ARGUMENT (%d)", c->err);
      sprintf(c->err_message, "Option is missing argument.");
      break;
    default:
      *c->err_header = '\0';
      *c->err_message = '\0';
      c->err = 0;
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
