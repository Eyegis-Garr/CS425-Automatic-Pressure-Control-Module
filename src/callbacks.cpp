#include "callbacks.h"
#include "simulator.h"

int preset_cb(menu_t *m, option_t *o) {
	// int selected_preset = m->options[m->cursor].value;
	int op;
	if (m->cursor == 0) { // save
		// overwrite check
		strcpy(alert->title, "SAVE ");
		op = SAVE; 
	} else if (m->cursor == 1) { // load
		strcpy(alert->title, "LOAD "); 
		op = LOAD;
	} else if (m->cursor == 2) { // delete
		strcpy(alert->title, "DELETE "); 
		op = DEL;
	}

	for (int i = 0; i < o->target->nopts; i += 1) {
		o->target->options[i].value = op;
	}

	return M_SELECT;
}

int pick_preset_cb(menu_t *m, option_t *o) {
	int code = M_NOP;
	TSPoint p;

	strcat(alert->title, o->name);
	
	m_draw(&tft, m, M_CLEAR);		// clear preset selection menu
	m_draw(&tft, alert, M_DRAW);	// display alert (confirm/cancel)
	while (code == M_NOP) {			// wait for selection
		p = get_press(&ts);
		if (p.z > 50) code = m_interact(alert, p);
	}
	m_draw(&tft, alert, M_CLEAR);	// clear alert

	if (code == M_CONFIRM) {
		switch (o->value) {
			case SAVE:		// save preset
				// compute checksum
				// test save success
				sprintf(popup->title, "%s success.", alert->title);
				break;
			case LOAD:		// load preset
				sprintf(popup->title, "%s failed!", alert->title);
				break;
			case DEL:		// delete preset
				sprintf(popup->title, "%s is done.", alert->title);
				break;
			default:
				break;
		}
    m_draw(&tft, popup, M_DRAW);
    _delay_ms(M_POPDELAY);
    m_draw(&tft, popup, M_CLEAR);
		popup->title[0] = '\0';

		return M_CONFIRM;
	}

	return M_BACK;
}

int alarms_cb(menu_t *m, option_t *o) {
	if (m->cursor == 0) {		// selected SOUND option
		int code = M_NOP, dir;
		TSPoint p;
		
		if (o->value == 0) {  			// off, turn on
			strcpy(alert->title, "Set Sound ON?");
			dir = 1;
		} else if (o->value == 1) { 	// on, turn off
			strcpy(alert->title, "Set Sound OFF?");
			dir = -1;
		}
		
		m_draw(&tft, m, M_CLEAR);		// clear preset selection menu
		m_draw(&tft, alert, M_DRAW);	// display alert (confirm/cancel)
		while (code == M_NOP) {			// wait for selection
			p = get_press(&ts);
			if (p.z > 100) code = m_interact(alert, p);
		}
		m_draw(&tft, alert, M_CLEAR);	// clear alert

		if (code == M_CONFIRM) {
			o->value += dir;

			sprintf(popup->title, "Sound is %s.", (o->value) ? "ON" : "OFF");
			m_draw(&tft, popup, M_DRAW);
			_delay_ms(M_POPDELAY);
			m_draw(&tft, popup, M_CLEAR);
			return M_CONFIRM;
		}
	}

	return M_SELECT;
}

int set_param_cb(menu_t *m, option_t *o) {
  if (sys.ui.cmask && sys.ui.pmask) {  // if circuit was selected to be modified
    for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {
      if (sys.ui.cmask & (1 << i)) {
        for (int k = 0; k < C_NUM_PARAM; k += 1) {
          if (sys.ui.pmask & (1 << k)) {
            // need floating point + precision info when storing parameter value
            sys.circuits[i].params[k] = o->value;
            if (m->flags & M_FPARAM) {
              // could use a precision modifier for divisor?
              sys.circuits[i].params[k] /= 100;
            }
          }
        }
      }
    }
  } else {
    // reclaimer and min supply stuff (only if they can't be treated the same as normal circuits?)
  }

  return M_BACK;
}

int pick_pid_cb(menu_t *m, option_t *o) {
  sys.ui.pmask |= (1 << (P_KP + m->cursor));
  o->target->flags |= M_FPARAM;

  return M_SELECT;
}

int timers_cb(menu_t *m, option_t *o) {
  sys.ui.pmask |= (1 << (P_PURGE_TIME + m->cursor));
  o->target->flags &= ~(M_FPARAM);

  return M_SELECT;
}

int main_cb(menu_t *m, option_t *o) {
  if (m->cursor == 3) {   // pressures
    sys.ui.pmask = (1 << P_SET_POINT);
    set_param->flags |= M_FPARAM;
  }

  return M_SELECT;
}

int circuit_select_cb(menu_t *m, option_t *o) {
  sys.ui.cmask = (1 << m->cursor);

  return M_SELECT;
}

int mode_cb(menu_t *m, option_t *o) {
  int code = M_NOP;
  TSPoint p;

  sprintf(alert->title, "%s mode?", o->name);

  m_draw(&tft, m, M_CLEAR);

  m_draw(&tft, alert, M_DRAW);
  while (code == M_NOP) {
    p = get_press(&ts);
    if (p.z > 100) code = m_interact(alert, p);
  }
  m_draw(&tft, alert, M_CLEAR);

  if (code == M_CONFIRM) {
    sys.s_flags = 1 << m->cursor;

    sprintf(popup->title, "Entered %s mode", o->name);
    m_draw(&tft, popup, M_DRAW);
    _delay_ms(M_POPDELAY);
    m_draw(&tft, popup, M_CLEAR);
    return M_CONFIRM;
  }

  return M_SELECT;
}
