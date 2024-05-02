#include "simulator.h"

Adafruit_ILI9341 disp = Adafruit_ILI9341(tft8bitbus, 22, 35, 36, 37, 33, 34);
system_t sys;

volatile uint8_t *PORT_B  = (volatile uint8_t *) 0x25;
volatile uint8_t *DDR_B   = (volatile uint8_t *) 0x24;
volatile uint8_t *PIN_B   = (volatile uint8_t *) 0x23;
volatile uint8_t *PORT_L  = (volatile uint8_t *) 0x10B;
volatile uint8_t *DDR_L   = (volatile uint8_t *) 0x10A;
volatile uint8_t *PIN_L   = (volatile uint8_t *) 0x109;

static const char *cname_map[] = {
  "MARX",
  "MARXTG70",
  "MTG",
  "SWITCH",
  "SWTG70"
};

/**
 * @brief initializes simulator. should be invoked on startup or reset
 * 
 */
void sim_setup() {
  disp.begin();
  disp.setRotation(1);
  disp.fillScreen(ILI9341_BLACK);

  init_system();
  init_io();

  sys.pid_window_size = 5000;
}

void init_system() {
  uint16_t x, y, tw, th;
  int16_t tx, ty;
  circuit_t *c;
  char pbuf[16];

  sys.disp = &disp;
  sys.state = S_SHOT;
  sys.en_flags = (1 << C_NUM_CIRCUITS) - 1;

  // load default circuit settings
  sys.circuits[C_MARX] = (circuit_t) {
    .params = {
      0,                   // pressure
      15,                 // set point
      C_MAX_DEFAULT,       // max time
      1000,                // check time
      C_PURGE_DEFAULT,     // purge time
      C_DELAY_DEFAULT,     // delay time
      75, 0, 25            // PID tuning params
    },
    .roc = 0.1,
    .supply = NULL,
    .reclaimer = NULL,
    .pins = { A7, 38, 39, 53, 49, 2, 3 }
  };

  sys.circuits[C_MTG70] = (circuit_t) {
    .params = {
      0,                   // pressure
      15,                 // set point
      C_MAX_DEFAULT,       // max time
      1000,                // check time
      C_PURGE_DEFAULT,     // purge time
      C_DELAY_DEFAULT,     // delay time
      75, 0, 25            // PID tuning params
    },
    .roc = 0.5,
    .supply = NULL,
    .reclaimer = NULL,
    .pins = { A7, 40, 41, 52, 48, 4, 5 }
  };

  sys.circuits[C_MTG] = (circuit_t) {
    .params = {
      0,                   // pressure
      15,                 // set point
      C_MAX_DEFAULT,       // max time
      1000,                // check time
      C_PURGE_DEFAULT,     // purge time
      C_DELAY_DEFAULT,     // delay time
      75, 0, 25            // PID tuning params
    },
    .roc = 0.2,
    .supply = NULL,
    .reclaimer = NULL,
    .pins = { A7, 42, 43, 51, 47, 6, 7 }
  };
  
  sys.circuits[C_SWITCH] = (circuit_t) {
    .params = {
      0,                   // pressure
      15,                 // set point
      C_MAX_DEFAULT,       // max time
      1000,                // check time
      C_PURGE_DEFAULT,     // purge time
      C_DELAY_DEFAULT,     // delay time
      75, 0, 25            // PID tuning params
    },
    .roc = 0.45,
    .supply = NULL,
    .reclaimer = NULL,
    .pins = { A7, 44, 17, 50, 46, 8, 9 }
  };

  sys.circuits[C_SWTG70] = (circuit_t) {
    .params = {
      0,                   // pressure
      15,                 // set point
      C_MAX_DEFAULT,       // max time
      1000,                // check time
      C_PURGE_DEFAULT,     // purge time
      C_DELAY_DEFAULT,     // delay time
      75, 0, 25            // PID tuning params
    },
    .roc = 0.75,
    .supply = NULL,
    .reclaimer = NULL,
    .pins = { A7, 8, 21, 10, 45, 11, 12 }
  };
  
  sys.supply = S_SUPPLY_MAX;
  sys.reclaimer = 30;
  sys.rec_auto_on = 8;
  sys.rec_auto_off = 2;
  sys.rec_roc = 1;
  sys.supply_min = 10;

  x = 5;
  y = CWIN_HEIGHT * 2 + 15;
  disp.setTextSize(2);
  disp.setCursor(x + 15, y + 10);
  disp.getTextBounds(F("REC ON"), x + 15, y + 10, &tx, &ty, &tw, &th);
  disp.setCursor(x + (CWIN_WIDTH / 2) - (tw / 2), y + 10 - (th / 2));
  disp.print(F("REC ON"));
  disp.setCursor(x + 18, y + 30);
  disp.fillRect(x + 17,
                y + 29,
                76, 
                17, 
                0);

  sprintf(pbuf, "%02d.%02d", (int)sys.rec_auto_on, (int)(100.0 * (sys.rec_auto_on - (int)sys.rec_auto_on)));
  disp.print(pbuf);
  

  x = CWIN_WIDTH * 2 + 10;
  disp.setTextSize(2);
  disp.setCursor(x + 15, y + 10);
  disp.getTextBounds(F("REC OFF"), x + 15, y + 10, &tx, &ty, &tw, &th);
  disp.setCursor(x + (CWIN_WIDTH / 2) - (tw / 2), y + 10 - (th / 2));
  disp.print(F("REC OFF"));
  disp.setCursor(x + 18, y + 30);
  disp.fillRect(x + 17,
                y + 29,
                76, 
                17, 
                0);

  sprintf(pbuf, "%02d.%02d", (int)sys.rec_auto_off, (int)(100.0 * (sys.rec_auto_off - (int)sys.rec_auto_off)));
  disp.print(pbuf);

  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {
    c = &sys.circuits[i];

    c->params[P_CHECK_TIME] = millis();

    // hook/attach circuit to system
    c->reclaimer = &sys.reclaimer;
    c->supply = &sys.supply;
    c->idx = i;

    // configure circuit PID controller
    pid_set_input(&c->pid, &c->params[P_PRESSURE]);
    pid_set_param(&c->pid, &c->params[P_KP], &c->params[P_KI], &c->params[P_KD]);

    draw_circuit(&sys.circuits[i], 0, 0);
  }
  
  sys.up_types = 0;
  sys.c_flags = (1 << C_NUM_CIRCUITS) - 1;
  sys.p_flags = (1 << C_NUM_PARAM) - 1;

  init_remote(&sys.remote, &Serial1);
}

void init_io() {
  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {
    pinMode(sys.circuits[i].pins[I_PRESSURE_IN], OUTPUT);
    pinMode(sys.circuits[i].pins[I_PRESSURE_OUT], OUTPUT);
    pinMode(sys.circuits[i].pins[I_PRESSURE_READ], INPUT);
  }

  sys.c_button.ddr = DDR_B;
  sys.c_button.pin = PIN_B;
  sys.c_button.port = PORT_B;

  sys.c_led.ddr = DDR_L;
  sys.c_led.pin = PIN_L;
  sys.c_led.port = PORT_L;

  // button port -> input
  *DDR_B &= ~((1 << C_MARX) | (1 << C_MTG70) | (1 << C_MTG) | (1 << C_SWITCH) | (1 << C_SWTG70));

  // led port -> output
  *DDR_L |= (1 << C_MARX) | (1 << C_MTG70) | (1 << C_MTG) | (1 << C_SWITCH) | (1 << C_SWTG70);
  *PORT_L |= (1 << C_MARX) | (1 << C_MTG70) | (1 << C_MTG) | (1 << C_SWITCH) | (1 << C_SWTG70);

  // 1000Hz button input polling timer
  TCCR1A = 0;                             // normal timer operation
  TCCR1B = (1 << CS11) | (1 << CS10);     // clock select -> 64 prescalar
  OCR1A = (F_CPU / (1000UL * 64UL)) - 1;  // top value for timer used for timer count comparisons
  TIMSK1 |= (1 << TOIE1);                 // output-compare interrupt enable for timer 1A
}

void poll_device(remote_t *r) {
  if (rx_packet(r) > 0) {
    process_packet(r);
  } else if (isbclr(r->state, R_RXINP)) {
    issue_updates(r);
  }
}

ISR(TIMER1_OVF_vect) {
  static uint8_t state[C_NUM_CIRCUITS];
  uint8_t i;

  for (i = 0; i < C_NUM_CIRCUITS; i += 1) {
    // shifts inverted input through state variable.
    // 0's shift through state[i] if switch is closed
    state[i] = (state[i] << 1) | ((*sys.c_button.pin & (1 << i)) == 0) | 0xE0;
    // if 12 0's have shifted through state[i] before
    // we see the last open switch-state (a 1 in state[i]) -> accept a button press
    if (state[i] == 0xF0) {
      *sys.c_led.port ^= (1 << i);
    }
  }

  sys.en_flags = *sys.c_led.port;

  poll_device(&sys.remote);
}

/**
 * @brief updates simulation. should be invoked per iteration.
 * 
 */
void sim_tick() {
  uint8_t i;

  sys.uptime = millis();

  switch (sys.state) {
    case S_SHOT:
      shot_pressure(false);
      break;
    case S_PURGE:
      purge();
      break;
    case S_ABORT:
      shot_pressure(true);
      break;
    case S_ERROR:
      break;
  }

  for (i = 0; i < C_NUM_CIRCUITS; i += 1) {
    draw_circuit(&sys.circuits[i], 
                  digitalRead(sys.circuits[i].pins[I_PRESSURE_IN]),
                  digitalRead(sys.circuits[i].pins[I_PRESSURE_OUT]));
  }

  auto_reclaim();
}

void auto_reclaim() {
  static float pdt;
  static uint32_t dt, r;
  uint16_t x, y, tw, th;
  int16_t tx, ty;
  char pbuf[16];
  
  if (!r && sys.reclaimer >= sys.rec_auto_on) {
    r = 1;
  }

  pdt = ((float) millis() - dt) / 1024;

  if (r) {
    if (fabs(sys.reclaimer - sys.rec_auto_off) > 0.1) {
      sys.reclaimer -= sys.rec_roc * pdt;
      sys.supply += sys.rec_roc * pdt;
    } else {
      r = 0;
    }
  }

  x = CWIN_WIDTH + 10;
  y = CWIN_HEIGHT + 10;

  disp.drawCircle(x + (CWIN_WIDTH / 2), y + CWIN_HEIGHT - 15, 5, 0xFFFF);
  disp.fillCircle(x + (CWIN_WIDTH / 2), y + CWIN_HEIGHT - 15, 3, r ? ILI9341_GREEN : 0);

  disp.drawRect(x,
                  y,
                  CWIN_WIDTH, 
                  CWIN_HEIGHT, 
                  0xFFFF);

  disp.setTextSize(2);
  disp.setCursor(x + 15, y + 10);
  disp.getTextBounds(F("RECLAIM"), x + 15, y + 10, &tx, &ty, &tw, &th);
  disp.setCursor(x + (CWIN_WIDTH / 2) - (tw / 2), y + 10 - (th / 2));
  disp.print(F("RECLAIM"));
  disp.setCursor(x + 20, y + 30);
  disp.fillRect(x + 19,
                y + 29,
                61, 
                17, 
                0);

  sprintf(pbuf, "%02d.%02d", (int)sys.reclaimer, (int)(100.0 * (sys.reclaimer - (int)sys.reclaimer)));
  disp.print(pbuf);

  y = CWIN_HEIGHT * 2 + 15;
  disp.setTextSize(2);
  disp.setCursor(x + 15, y + 10);
  disp.getTextBounds(F("SUPPLY"), x + 15, y + 10, &tx, &ty, &tw, &th);
  disp.setCursor(x + (CWIN_WIDTH / 2) - (tw / 2), y + 10 - (th / 2));
  disp.print(F("SUPPLY"));
  disp.setCursor(x + 18, y + 30);
  disp.fillRect(x + 17,
                y + 29,
                76, 
                17, 
                0);

  sprintf(pbuf, "%02d.%02d", (int)sys.supply, (int)(100.0 * (sys.supply - (int)sys.supply)));
  disp.print(pbuf);

  dt = millis();
}

/**
 * @brief modifies circuit pressure according
 * to delta time and solenoid state. updates
 * connected pressure-display if needed.
 * 
 * @param c - circuit to modify
 * @param dt - time elapsed in milliseconds
 * @param in - binary state of intake solenoid
 * @param out - binary state of exhaust solenoid
 */
void modify_circuit(circuit_t *c, uint32_t dt, uint8_t in, uint8_t out) {
  float pdt;

  pdt = ((float)dt) / 128;

  if (in) {         /* intake open */
    if (!out) {     /* exhaust closed */
      // drain supply into circuit
      pdt *= mapf(*c->supply - c->params[P_PRESSURE], -C_PRESSURE_MAX, C_PRESSURE_MAX, -1, 1);
      *c->supply -= c->roc * pdt;
      c->params[P_PRESSURE] += c->roc * pdt;
    } else {        /* exhaust open */
      // drain supply into reclaimer
      pdt *= mapf(*c->reclaimer - *c->supply, -C_PRESSURE_MAX, C_PRESSURE_MAX, -1, 1);
      if (*c->reclaimer > *c->supply)
        c->params[P_PRESSURE] -= 0.5 * c->roc * pdt;
      else 
        c->params[P_PRESSURE] += 0.5 * c->roc * pdt;
      *c->reclaimer -= c->roc * pdt;
      *c->supply += c->roc * pdt;
    }
  } else {        /* intake closed */
    if (out) {    /* exhaust open */
      // drain circuit into reclaimer
      pdt *= mapf(*c->reclaimer - c->params[P_PRESSURE], -C_PRESSURE_MAX, C_PRESSURE_MAX, -1, 1);
      *c->reclaimer -= c->roc * pdt;
      c->params[P_PRESSURE] += c->roc * pdt;
    }
  }

  c->params[P_PRESSURE] = clampf(0, C_PRESSURE_MAX, c->params[P_PRESSURE]);
  *c->supply = clampf(0, S_SUPPLY_MAX, *c->supply);
  *c->reclaimer = clampf(0, S_RECLAIM_MAX, *c->reclaimer);

  auto_reclaim();
  draw_circuit(c, in, out);
}


void draw_circuit(circuit_t *c, int in, int out) {
  static float p[C_NUM_CIRCUITS] = { 420 };
  uint16_t x, y, xoffset, tw, th;
  int16_t tx, ty;
  char pbuf[16];

  xoffset = (c->idx == C_NUM_CIRCUITS - 1) ? CWIN_WIDTH + 5 : 0;
  x = (CWIN_WIDTH + 5)  * (c->idx % 3) + 5 + xoffset;
  y = (CWIN_HEIGHT + 5) * (c->idx / 3) + 5;

  disp.drawCircle(x + 25, y + CWIN_HEIGHT - 15, 5, 0xFFFF);
  disp.fillCircle(x + 25, y + CWIN_HEIGHT - 15, 3, in ? ILI9341_BLUE : 0);
  disp.drawCircle(x + CWIN_WIDTH - 25, y + CWIN_HEIGHT - 15, 5, 0xFFFF);
  disp.fillCircle(x + CWIN_WIDTH - 25, y + CWIN_HEIGHT - 15, 3, out ? ILI9341_RED : 0);
  disp.drawCircle(x + CWIN_WIDTH / 2, y + CWIN_HEIGHT - 15, 5, 0xFFFF);
  disp.fillCircle(x + CWIN_WIDTH / 2, y + CWIN_HEIGHT - 15, 3, isbset(*sys.c_led.pin, c->idx) ? ILI9341_GREEN : 0);

  if (fabs(c->params[P_PRESSURE] - p[c->idx]) < 0.1) {
    return;
  } else {
    p[c->idx] = c->params[P_PRESSURE];
  }

  disp.drawRect(x,
                 y,
                 CWIN_WIDTH, 
                 CWIN_HEIGHT, 
                 0xFFFF);

  disp.setTextSize(2);
  disp.setCursor(x + 15, y + 10);
  disp.getTextBounds(cname_map[c->idx], x + 15, y + 10, &tx, &ty, &tw, &th);
  disp.setCursor(x + (CWIN_WIDTH / 2) - (tw / 2), y + 10 - (th / 2));
  disp.print(cname_map[c->idx]);
  disp.setCursor(x + 20, y + 30);
  disp.fillRect(x + 19,
                y + 29,
                61, 
                17, 
                0);

  sprintf(pbuf, "%02d.%02d", (int)c->params[P_PRESSURE], (int)(100.0 * (c->params[P_PRESSURE] - (int)c->params[P_PRESSURE])));
  disp.print(pbuf);
}

/**
 * @brief opens enabled circuits intake and exhaust for specified
 * purge time-interval
 * 
 */
void purge() {
  static uint32_t itime;
  circuit_t *c;
  int i;


  for (i = 0; i < C_NUM_CIRCUITS; i += 1) {    // loop system circuits
    itime = millis();
    c = &sys.circuits[i];

    if (isbset(sys.en_flags, i)) {
      while (millis() - itime <= c->params[P_PURGE_TIME]) {
        // open exhaust, open intake
        digitalWrite(c->pins[I_PRESSURE_IN], HIGH);
        digitalWrite(c->pins[I_PRESSURE_OUT], HIGH);
        
        // move pressure according to time since last purge-iteration
        modify_circuit(c, millis() - itime, 1, 1);
      }

      // close exhaust, close intake
      digitalWrite(c->pins[I_PRESSURE_OUT], LOW);
      digitalWrite(c->pins[I_PRESSURE_IN], LOW);
    }
  }

  sys.state = S_STANDBY;
}

/**
 * @brief steps circuit pressure towards set-point.
 * 
 * @param c - circuit to set
 * @param var - +/- pressure variance in PSI
 * @param half - true -> set to half of set-point; false -> set to set-point
 */
void set_pressure(circuit_t *c, float var, int half) {
  uint32_t wstart;
  float out, set_point;
  int dir;

  set_point = c->params[P_SET_POINT] * ((half) ? 0.5 : 1);

  pid_set_target(&c->pid, &set_point);
  
  if (!IN_RANGE(c->params[P_PRESSURE], (set_point - var), (set_point + var))) {
    // pick PID direction
    dir = (c->params[P_PRESSURE] > set_point + var) ? REVERSE : DIRECT;
    pid_set_direction(&c->pid, dir);
    wstart = millis();    // base time offset
    out = pid_compute(&c->pid);
    while (millis() - wstart < out + 50) {
      // close intake, open exhaust
      digitalWrite(c->pins[I_PRESSURE_IN], dir ^ 1);
      digitalWrite(c->pins[I_PRESSURE_OUT], dir);
      modify_circuit(c, millis() - wstart + 50, dir ^ 1, dir);
      out = pid_compute(&c->pid);
    }
  }

  // close circuit solenoids
  digitalWrite(c->pins[I_PRESSURE_IN], LOW);
  digitalWrite(c->pins[I_PRESSURE_OUT], LOW);
}

/**
 * @brief invokes set_pressure for all enabled circuits.
 * should be invoked iteratively until set-point is reached for all circuits.
 * 
 * @param half - true -> set to half of set-point; false -> set to set-point
 */
void shot_pressure(bool half) {
  static int i = 0;
  float var = 0.09;
  circuit_t *c;

  if (sys.en_flags & (1 << i)) {   // if enabled
    c = &sys.circuits[i];
    if (millis() - c->params[P_CHECK_TIME] >= c->params[P_DELAY_TIME]) {   // check time expired
      set_pressure(c, var, half);
      c->params[P_CHECK_TIME] = millis();
    }
  }
  i = (i + 1) % C_NUM_CIRCUITS;
}

