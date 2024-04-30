/**
 * @file draw.c
 * @author Bradley Sullivan (bradleysullivan@nevada.unr.edu)
 * @brief Curses UI drawing routines
 * @version 0.1
 * @date 2024-04-24
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "client.h"

/**
 * @brief Clears y-line from bx-col to ex-col
 * 
 * @param w Curses window
 * @param y Line to clear
 * @param bx Start x (column)
 * @param ex End x (column)
 */
void wclrtorng(WINDOW *w, int y, int bx, int ex) {
  wmove(w, y, bx);
  for (; bx < ex; bx += 1) {
    mvwaddch(w, y, bx, ' ');
  }
}

/**
 * @brief Centers provided string at y-line
 * 
 * @param w Curses window
 * @param y Line to put string
 * @param str String to center
 */
void center_str(WINDOW *w, int y, const char *str) {
  mvwaddstr(w, y, (getmaxx(w) / 2) - (strlen(str) / 2), str);
}

/**
 * @brief Formats progress string given total length,
 * progress value, and max value
 * 
 * @param str Progress string to format
 * @param len String length of progress bar
 * @param val Current progress
 * @param max Max progress-value
 */
void progress_str(char *str, int len, double val, double max) {
	if (max > 0) {
		double p = val / max;
		int fill = p * len, i;

		str[0] = '|';
		for (i = 1; i < len - 1; i += 1) {
			if (i < fill) str[i] = '=';
			else str[i] = ' ';
		}
		str[i] = '|';
		str[i + 1] = '\0';
	}
}

/**
 * @brief Draws string at (y,x) from scroll position
 * with wraparound.
 * 
 * @param w Curses window
 * @param y Line to put string
 * @param x Column to put string
 * @param scroll Scroll position
 * @param str String to print/scroll
 */
void scroll_str(WINDOW *w, int y, int x, int scroll, char *str) {
  int l, i, m;

  l = strlen(str);
  m = l < getmaxx(w) ? l : getmaxx(w) - 2;
  for (i = 0; i < m; i += 1) {
    mvwaddch(w, y, x + i, str[MOD(scroll + i, l - 1)]);
  }
}

void draw_system(client_t *c) {
  static char rp[64], sp[64], *phead;
  static struct timeval sc, dt;
  static uint32_t scroll, t;

  gettimeofday(&sc, NULL);
  if (!dt.tv_sec) gettimeofday(&dt, NULL);

  center_str(c->sim_win, 1, "SYSTEM STATE");

  phead = c->print_buf;
  phead += sprintf(c->print_buf, " | %s = %s | %s = %d ms | %s = %.02lf | %s = %.02lf | %s = %.02lf ",
                  "STATE", mode_map[c->sim.state], 
                  "UPTIME", c->sim.uptime, 
                  "REC_ON", c->sim.rec_auto_on,
                  "REC_OFF", c->sim.rec_auto_off,
                  "MIN SUPPLY", c->sim.supply_min);

  t = (sc.tv_sec - dt.tv_sec) * 1000;
  t += (sc.tv_usec - dt.tv_usec) / 1000;

  if (t > 500) {
    gettimeofday(&dt, NULL);
    scroll = MOD(scroll + 1, (phead - c->print_buf));
  }

  wclrtorng(c->sim_win, 2, 1, CWIN_WIDTH - 2);
  scroll_str(c->sim_win, 2, 1, scroll, c->print_buf);

  progress_str(rp, 30, c->sim.reclaimer, S_RECLAIM_MAX);
  progress_str(sp, 30, c->sim.supply, S_SUPPLY_MAX);

  sprintf(c->print_buf, "R %.02f %s %.02lf", c->sim.reclaimer, rp, S_RECLAIM_MAX);
  wclrtorng(c->sim_win, 3, 1, CWIN_WIDTH - 1);
  center_str(c->sim_win, 3, c->print_buf);
  
  sprintf(c->print_buf, "S %.02f %s %.02lf", c->sim.supply, sp, S_SUPPLY_MAX);
  wclrtorng(c->sim_win, 4, 1, CWIN_WIDTH - 1);
  center_str(c->sim_win, 4, c->print_buf);
  
  wrefresh(c->sim_win);
}

/**
 * @brief Draws circuit info to left half of UI
 * 
 * @param c Active client structure
 */
void draw_circuits(client_t *c) {
  static char progress[32], *phead;
  int i, k;
  WINDOW *cw;
  circuit_t *circuit;

  static struct timeval sc, dt;
  static uint32_t scroll, t;

  gettimeofday(&sc, NULL);
  if (!dt.tv_sec) gettimeofday(&dt, NULL);
    
  for (i = 0; i < C_NUM_CIRCUITS; i += 1) {
    circuit = &c->circuit[i];
    cw = circuit->w;

    center_str(cw, 1, circuit_map[i]);

    wclrtorng(cw, 3, 1, CWIN_WIDTH - 1);
    progress_str(progress, 30, circuit->params[P_PRESSURE], circuit->params[P_SET_POINT]);
    sprintf(c->print_buf, "%.02lf %s %.02lf", circuit->params[P_PRESSURE], progress, circuit->params[P_SET_POINT]);
    center_str(cw, 3, c->print_buf);

    phead = c->print_buf;
    for (k = 0; k < C_NUM_PARAM; k += 1) {
      phead += sprintf(phead, "| %s = %.02lf%s", param_map[k], circuit->params[k], 
                      (k == C_NUM_PARAM - 1) ? " |" : " ");
    }

    t = (sc.tv_sec - dt.tv_sec) * 1000;
    t += (sc.tv_usec - dt.tv_usec) / 1000;

    if (t > 500) {
      gettimeofday(&dt, NULL);
      scroll = MOD(scroll + 1, (phead - c->print_buf));
    }

    wclrtorng(cw, 2, 1, CWIN_WIDTH - 2);
    scroll_str(cw, 2, 1, scroll, c->print_buf);

    wclrtorng(cw, 4, 1, CWIN_WIDTH - 2);
    for (k = I_PRESSURE_IN; k <= I_LED; k += 1) {
      if (isbset(circuit->io, k)) wattron(cw, A_STANDOUT);
      mvwprintw(cw, 4, k * (CWIN_WIDTH / (I_LED + 1)) - (strlen(io_map[k - 1]) / 2), io_map[k - 1]);
      wattroff(cw, A_STANDOUT);
    }

    wrefresh(cw);
  }
}

/**
 * @brief Draws and updates input area
 * 
 * @param c Active client structure
 */
void draw_input(client_t *c) {
  char prompt;

	werase(c->cmd); wborder(c->cmd, 0, 0, 0, 0, 0, 0, 0, 0);
	wclrtorng(c->cmd, 3, 1, CWIN_WIDTH - 4);
  center_str(c->cmd, 1, "-= INPUT =-");
  prompt = c->err ? '!' : '>';
  if (c->cursor > CWIN_WIDTH - 7) {
    mvwprintw(c->cmd, 3, 1, "%c %s", prompt, c->cmd_input + (c->cursor - CWIN_WIDTH + 7));
  }	else {
    mvwprintw(c->cmd, 3, 1, "%c %s", prompt, c->cmd_input);
  }

  mvwprintw(c->cmd, 4, ((c->cursor > CWIN_WIDTH - 7) ? CWIN_WIDTH - 4 : c->cursor + 3), "^");

	wrefresh(c->cmd);
}

/**
 * @brief Draws error info to input area.
 * 
 * @param c Active client structure
 */
void draw_err(client_t *c) {
	werase(c->cmd);

	center_str(c->cmd, 1, c->err_header);
	center_str(c->cmd, 2, c->err_message);
	center_str(c->cmd, 4, "PRESS ANY KEY TO CONTINUE");

	wrefresh(c->cmd);
}

/**
 * @brief Prints specified help message to
 * status window
 * 
 * @param c Active client structure
 * @param str Help command specifier
 */
void draw_help(client_t *c, char *str) {
  
  print_view(c, "=================================\n");

  if (str) {
    to_upper(str);
    if (strcmp(str, "UPDATE") == 0) {
      print_view(c, "Update Options\n");
      print_view(c, "  --circuit,-c [NAME,...] -> Circuit to update\n");
      print_view(c, "  --parameter,-p [PARAM,...] -> Parameter to update\n");
      print_view(c, "  --system,-s -> System state update\n");
      print_view(c, "  --refresh,-r -> Signals system to TX configured updates\n");
      print_view(c, "  --timeout,-t [SEC]-> Timeout value in seconds (0-255)\n");
    } else if (strcmp(str, "DO") == 0) {
      print_view(c, "Do (command) Options\n");
      print_view(c, "  --circuit,-c [NAME,...] -> Circuit to update\n");
      print_view(c, "  --parameter,-p [PARAM=VALUE,...]-> Parameter to update\n");
      print_view(c, "  --modeset,-m -> Configure system mode\n");
      print_view(c, "  --parset,-v -> Sets specified parameter with given value (e.g. PRESSURE=12.34)\n");
      print_view(c, "  --config,-g -> Requests current configuration/preset\n");
      print_view(c, "  --save,-a -> Triggers system config save\n");
      print_view(c, "  --timeout,-t [SEC] -> Timeout value in seconds (0-255)\n");
    } else if (strcmp(str, "PING") == 0) {
      print_view(c, "Ping Options\n");
      print_view(c, "  --ntimes,-n [PINGS] -> Number of ping packets to issue\n");
      print_view(c, "  --timeout,-t [SEC] -> Timeout value in seconds (0-255)\n");
    } else {
      print_view(c, "Usage: OPERATION [OPTIONS]\n");
      print_view(c, "\nOperations: update, do, ping\n");
      print_view(c, "  update -> Request system/circuit/parameter updates from system\n");
      print_view(c, "  do -> Execute specified command on system\n");
      print_view(c, "  ping -> Request packet echo to verify connection\n");
      print_view(c, "  help [OPERATION|OPTION] -> Prints a similar message to this one :)\n");
    }
  } else {
    print_view(c, "Usage: OPERATION [OPTIONS]\n");
    print_view(c, "\nOperations: update, do, ping\n");
    print_view(c, "  update -> Request system/circuit/parameter updates from system\n");
    print_view(c, "  do -> Execute specified command on system\n");
    print_view(c, "  ping -> Request packet echo to verify connection\n");
    print_view(c, "  help [OPERATION|OPTION] -> Prints a similar message to this one :)\n");
  }
}

/**
 * @brief Updates status window with text-lines delimited
 * with '\n'. New lines are printed with WA_STANDOUT (i.e.
 * highlighted with inverted background color).
 * 
 * @param c Active client structure
 * @param msg String (line(s)) to print, delimited with '\n'.
 */
void print_view(client_t *c, char *msg) {
  static char input[SWIN_WIDTH - 2];
  int x, y, i;
  char *line;
  
  getmaxyx(c->view, y, x);
  for (i = 0; i < y; i += 1) mvwchgat(c->view, i, 0, x - 1, WA_NORMAL, 0, NULL);

  strcpy(input, msg);
  line = strtok(input, "\n");

  while (line) {
    if (strlen(line) > x) {
      for (i = strlen(line) / (x - 1) + 1; i--; line += x - 1) {
        wattron(c->view, WA_STANDOUT);
        wscrl(c->view, 1);
        mvwaddnstr(c->view, y - 1, 0, line, x - 1);
        wattroff(c->view, WA_STANDOUT);
      }
    } else {
      wattron(c->view, WA_STANDOUT);
      wscrl(c->view, 1);
      mvwaddnstr(c->view, y - 1, 0, line, x - 1);
      wattroff(c->view, WA_STANDOUT);
    }
    
    line = strtok(NULL, "\n");
  }

  wrefresh(c->view);  
}

/**
 * @brief Prints packet info to status win using
 * print_view().
 * 
 * @param c Active client structure
 * @param p Packet to print
 */
void draw_packet(client_t *c, packet_t *p) {
  int i;
  
  sprintf(c->print_buf, "PACKET TYPE: %s\n", pktype_map[p->packet.type]);
  print_view(c, c->print_buf);
	sprintf(c->print_buf, "PACKET FLAGS (%u):", p->packet.flags);
  print_view(c, c->print_buf);

  c->print_buf[0] = '\0';
  mapflag(p->packet.flags, 8, flag_map[p->packet.type], c->print_buf);
  print_view(c, c->print_buf);

  sprintf(c->print_buf, "\nPACKET TIMEOUT: %d\n", p->packet.timeout);
  print_view(c, c->print_buf);
	sprintf(c->print_buf, "PACKET SIZE: %d\n", p->packet.size);
  print_view(c, c->print_buf);

  for (i = 0; i < p->packet.size; i += 1) {
    sprintf(c->print_buf, "PACKET DATA[%d]: %hhu\n", i, p->packet.data[i]);
    print_view(c, c->print_buf);
  }
}

/**
 * @brief Prints latest-received circuit configuration
 * to the status window using print_view()
 * 
 * @param c Active client structure
 * @param idx Circuit index to print
 */
void draw_circuit_view(client_t *c, int idx) {
  int i;
  circuit_t *x;

  if (idx >= 0) {
    x = &c->circuit[idx];
    sprintf(c->print_buf, "  -== %s ==-\n", circuit_map[idx]);
    print_view(c, c->print_buf);
    for (i = 0; i < C_NUM_PARAM; i += 1) {
      sprintf(c->print_buf, "%s : %.02lf\n", param_map[i], x->params[i]);
      print_view(c, c->print_buf);
    }
  }
}

/**
 * @brief Prints latest system configuration parameters
 * to status window using print_view()
 * 
 * @param c Active client structure
 */
void draw_system_view(client_t *c) {
  sprintf(c->print_buf, "-== SYSTEM ==-\n");
  print_view(c, c->print_buf);
  
  sprintf(c->print_buf, "MODE: %s\n", mode_map[c->sim.state]);
  print_view(c, c->print_buf);

  sprintf(c->print_buf, "ENABLED: \n");
  print_view(c, c->print_buf);
  mapflag(c->sim.en_flags, C_NUM_CIRCUITS, circuit_map, c->print_buf);
  print_view(c, c->print_buf);

  sprintf(c->print_buf, "UPTIME: %u ms\n", c->sim.uptime);
  print_view(c, c->print_buf);

  sprintf(c->print_buf, "RECLAIM: %lf ms\n", c->sim.reclaimer);
  print_view(c, c->print_buf);

  sprintf(c->print_buf, "REC_ON: %lf ms\n", c->sim.rec_auto_on);
  print_view(c, c->print_buf);

  sprintf(c->print_buf, "REC_OFF: %lf ms\n", c->sim.rec_auto_off);
  print_view(c, c->print_buf);

  sprintf(c->print_buf, "SUPPLY: %lf ms\n", c->sim.supply);
  print_view(c, c->print_buf);

  sprintf(c->print_buf, "MIN SUPPLY: %lf ms\n", c->sim.supply_min);
  print_view(c, c->print_buf);
}
