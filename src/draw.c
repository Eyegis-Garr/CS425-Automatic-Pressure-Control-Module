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

  l = strlen(str) < getmaxx(w) - x - 2 ? strlen(str) : getmaxx(w) - x - 2;
  for (i = 0; i < l; i += 1) {
    mvwaddch(w, y, x + i, str[(scroll + i) % strlen(str)]);
  }
}

void draw_circuits(client_t *c) {
  static uint32_t r = 0;
  static double dt;
  char pbuf[256], progress[32], *phead;
  int i, k, scroll;
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
      scroll = (scroll + 1) % strlen(pbuf);
    }

    wclrtorng(cw, 2, 1, CWIN_WIDTH - 2);
    scroll_str(cw, 2, 1, scroll, pbuf);

    for (k = I_PRESSURE_IN; k <= I_LED; k += 1) {
      if (isbset(circuit->io, k)) wattron(cw, COLOR_PAIR(k));
      mvwprintw(cw, 4, k * 8 + 6, io_map[k - 1]);
      if (isbset(circuit->io, k)) wattroff(cw, COLOR_PAIR(k));
    }

    wrefresh(cw);
  }
}

void draw_input(client_t *c) {
	wborder(c->cmd, 0, 0, 0, 0, 0, 0, 0, 0);
	center_str(c->cmd, 1, "-=  INPUT  =-");
	wclrtorng(c->cmd, 3, 4, CWIN_WIDTH - 5);
	mvwprintw(c->cmd, 3, 1, "> %s", c->cmd_input);
	wrefresh(c->cmd);
}

void draw_err(client_t *c) {
	werase(c->cmd);
	center_str(c->cmd, 1, c->err_header);
	center_str(c->cmd, 2, c->err_message);

	center_str(c->cmd, 4, "PRESS ANY KEY TO CONTINUE");

	wrefresh(c->cmd);
}

void draw_simulator(client_t *c) {
}

void draw_packet(client_t *c, packet_t *p) {
	int l;
	char pbuf[64], *print;
	werase(c->view); wborder(c->view, 0, 0, 0, 0, 0, 0, 0, 0);
	
	l = 1;
	sprintf(pbuf, "PACKET TYPE: %s", pktype_map[p->type]);
	center_str(c->view, l++, pbuf);

	print = pbuf;
	print += sprintf(pbuf, "PACKET FLAGS: %hhu", p->flags);
	center_str(c->view, l++, pbuf);

	sprintf(pbuf, "PACKET TIMEOUT: %d", p->timeout);
	center_str(c->view, l++, pbuf);

	sprintf(pbuf, "PACKET SIZE: %d", p->size);
	center_str(c->view, l++, pbuf);

	wrefresh(c->view);
}
