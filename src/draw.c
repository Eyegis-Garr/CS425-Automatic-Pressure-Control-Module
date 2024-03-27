#include "client.h"

void wclrtorng(WINDOW *w, int y, int bx, int ex) {
  wmove(w, y, bx);
  for (; bx < ex; bx += 1) {
    mvwaddch(w, y, bx, ' ');
  }
}

void center_str(WINDOW *w, int y, const char *str) {
  mvwaddstr(w, y, (getmaxx(w) / 2) - (strlen(str) / 2), str);
}

void progress_str(char *str, int len, double val, double max) {
	if (max > 0) {
		double p = val / max;
		int fill = p * len, i;

		str[0] = '|';
		for (i = 1; i < len - 1; i += 1) {
			if (i < fill) str[i] = '+';
			else str[i] = ' ';
		}
		str[i] = '|';
		str[i + 1] = '\0';
	}
}

void scroll_str(WINDOW *w, int y, int x, int scroll, char *str) {
  int l, i;

  l = strlen(str) < getmaxx(w) - x - 1 ? strlen(str) : getmaxx(w) - x - 1;
  for (i = 0; i < l; i += 1) {
    mvwaddch(w, y, x + i, str[(scroll + i) % strlen(str)]);
  }
}

void draw_circuits(client_t *c) {
  static uint32_t r = 0, scroll = 0;
  static double dt;
  char pbuf[256], progress[32], *phead;
  int i, k;
  WINDOW *cw;
  circuit_t *circuit;

  for (i = 0; i < C_NUM_CIRCUITS; i += 1) {
		
		dt = (((double)(clock() - r)) / CLOCKS_PER_SEC) * 1000;

    circuit = &c->circuit[i];
    cw = circuit->w;

    center_str(cw, 1, circuit_map[i]);

    wclrtorng(cw, 3, 1, CWIN_WIDTH - 1);
    progress_str(progress, 30, circuit->params[P_PRESSURE], circuit->params[P_SET_POINT]);
    sprintf(pbuf, "%.02lf %s %.02lf", circuit->params[P_PRESSURE], progress, circuit->params[P_SET_POINT]);
    center_str(cw, 3, pbuf);

    phead = pbuf;
    for (k = 0; k < C_NUM_PARAM; k += 1) {
      phead += sprintf(phead, "| %s = %.02lf%s", param_map[k], circuit->params[k], 
                      (k == C_NUM_PARAM - 1) ? " |" : " ");
    }

		if (dt >= 500) {
      r = clock();
      scroll = (scroll + 1) % (phead - pbuf);
    }

    wclrtorng(cw, 2, 1, CWIN_WIDTH - 2);
    scroll_str(cw, 2, 1, scroll, pbuf);

    for (k = I_PRESSURE_IN; k <= I_LED; k += 1) {
      if (isbset(circuit->io, k)) wattron(cw, A_STANDOUT);
      mvwprintw(cw, 4, (k) * (CWIN_WIDTH / (I_LED + 1)) - (strlen(io_map[k - 1]) / 2), io_map[k - 1]);
      if (isbset(circuit->io, k)) wattroff(cw, A_STANDOUT);
    }

    wrefresh(cw);
  }
}

void draw_input(client_t *c) {
	wborder(c->cmd, 0, 0, 0, 0, 0, 0, 0, 0);
	center_str(c->cmd, 1, "-=  INPUT  =-");
	wclrtorng(c->cmd, 3, 1, CWIN_WIDTH - 3);
  if (c->cursor > CWIN_WIDTH - 6) {
    mvwprintw(c->cmd, 3, 1, "> %s", c->cmd_input + (c->cursor - CWIN_WIDTH + 6));
  }	else {
    mvwprintw(c->cmd, 3, 1, "> %s", c->cmd_input);
  }
	wrefresh(c->cmd);
}

void draw_err(client_t *c) {
	werase(c->cmd);

	center_str(c->cmd, 1, c->err_header);
	center_str(c->cmd, 2, c->err_message);
	center_str(c->cmd, 4, "PRESS ANY KEY TO CONTINUE");

	wrefresh(c->cmd);
}

void draw_help(client_t *c, char *str) {
  print_view(c, "=================================\n");
  if (!str) {
    print_view(c, "Usage: OPERATION [OPTIONS]\n");
    print_view(c, "\nOperations: update, do, ping\n");
    print_view(c, "  update -> Request system/circuit/parameter updates from system\n");
    print_view(c, "  do -> Execute specified command on system\n");
    print_view(c, "  ping -> Request packet echo to verify connection\n");
    print_view(c, "  help [OPERATION|OPTION] -> Prints a similar message to this one :)\n");
  } else {
    if (strcmp(str, "update") == 0) {
      print_view(c, "Update Options\n");
      print_view(c, "  --circuit,-c [NAME,...] -> Circuit to update\n");
      print_view(c, "  --parameter,-p [PARAM,...] -> Parameter to update\n");
      print_view(c, "  --system,-s -> System state update\n");
      print_view(c, "  --refresh,-r -> Signals system to TX configured updates\n");
      print_view(c, "  --timeout,-t [SEC]-> Timeout value in seconds (0-255)\n");
    } else if (strcmp(str, "do") == 0) {
      print_view(c, "Do (command) Options\n");
      print_view(c, "  --circuit,-c [NAME,...] -> Circuit to update\n");
      print_view(c, "  --parameter,-p [PARAM=VALUE,...]-> Parameter to update\n");
      print_view(c, "  --modeset,-m -> Configure system mode\n");
      print_view(c, "  --parset,-v -> Sets specified parameter with given value (e.g. PRESSURE=12.34)\n");
      print_view(c, "  --config,-g -> Requests current configuration/preset\n");
      print_view(c, "  --save,-a -> Triggers system config save\n");
      print_view(c, "  --timeout,-t [SEC] -> Timeout value in seconds (0-255)\n");
    } else if (strcmp(str, "ping") == 0) {
      print_view(c, "Ping Options\n");
      print_view(c, "  --ntimes,-n [PINGS] -> Number of ping packets to issue\n");
      print_view(c, "  --timeout,-t [SEC] -> Timeout value in seconds (0-255)\n");
    }
  }
}

void draw_simulator(client_t *c) {
}

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

void draw_packet(client_t *c, packet_t *p) {
	char pbuf[256], *print;
  int i;

	print = pbuf;
  
  print += sprintf(print, "PACKET TYPE: %s\n", pktype_map[p->type]);
	print += sprintf(print, "PACKET FLAGS (%u):", p->flags);
  print += mapflag(p->flags, 8, flag_map[p->type], print);
  print += sprintf(print, "\nPACKET TIMEOUT: %d\n", p->timeout);
	print += sprintf(print, "PACKET SIZE: %d\n", p->size);

  for (i = 0; i < p->size; i += 1) {
    print += sprintf(print, "PACKET DATA[%d]: %hhu\n", i, p->bytes[i]);
  }

  print_view(c, pbuf);
}
