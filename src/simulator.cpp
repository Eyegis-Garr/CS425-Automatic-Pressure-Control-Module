#include "simulator.h"

system_t sys;

volatile uint8_t *PORT_B  = (volatile uint8_t *) 0x25;
volatile uint8_t *DDR_B   = (volatile uint8_t *) 0x24;
volatile uint8_t *PIN_B   = (volatile uint8_t *) 0x23;
volatile uint8_t *PORT_L  = (volatile uint8_t *) 0x10B;
volatile uint8_t *DDR_L   = (volatile uint8_t *) 0x10A;
volatile uint8_t *PIN_L   = (volatile uint8_t *) 0x109;

void sim_setup() {
  init_ui(&sys.ui);
  init_system();
  init_io();

  sys.pid_window_size = 5000;
}

void init_system() {
  sys.s_flags = S_SHOT;
  sys.c_flags = 0;
  sys.p_flags = 0;
  sys.en_flags = (1 << C_MARX);

  // load default circuit settings
  sys.circuits[C_MARX] = (circuit_t) {
    {
      0,                  // pressure
      254,               // set point
      C_MAX_DEFAULT,       // max time
      1000,                // check time
      C_PURGE_DEFAULT,     // purge time
      C_DELAY_DEFAULT,     // delay time
      50, 0, 25            // PID tuning params
    },
    1,                  // pressure rate-of-change
    { A7, 13, 13, 53, 49, 2, 3 },    // pins
    NULL
  };

  sys.circuits[C_MTG70] = (circuit_t) {
    {
      0,                  // pressure
      254,               // set point
      C_MAX_DEFAULT,       // max time
      1000,                // check time
      C_PURGE_DEFAULT,     // purge time
      C_DELAY_DEFAULT,     // delay time
      50, 0, 25            // PID tuning params
    },
    1,                  // pressure rate-of-change
    { A7, 43, 39, 53, 49, 4, 5 },    // pins
    NULL
  };

  sys.circuits[C_MTG] = (circuit_t) {
    {
      0,                  // pressure
      254,               // set point
      C_MAX_DEFAULT,       // max time
      1000,                // check time
      C_PURGE_DEFAULT,     // purge time
      C_DELAY_DEFAULT,     // delay time
      50, 0, 25            // PID tuning params
    },
    0.5,                  // pressure rate-of-change
    { A7, 40, 13, 53, 49, 6, 7 },    // pins
    NULL
  };
  
  sys.circuits[C_SWITCH] = (circuit_t) {
    {
      0,                  // pressure
      254,               // set point
      C_MAX_DEFAULT,       // max time
      1000,                // check time
      C_PURGE_DEFAULT,     // purge time
      C_DELAY_DEFAULT,     // delay time
      50, 0, 25            // PID tuning params
    },
    2.5,                  // pressure rate-of-change
    { A7, 16, 17, 53, 49, 8, 9 },    // pins
    NULL
  };

  sys.circuits[C_SWTG70] = (circuit_t) {
    {
      0,                  // pressure
      254,               // set point
      C_MAX_DEFAULT,       // max time
      1000,                // check time
      C_PURGE_DEFAULT,     // purge time
      C_DELAY_DEFAULT,     // delay time
      50, 0, 25            // PID tuning params
    },
    0.5,                  // pressure rate-of-change
    { A7, 20, 21, 53, 49, 11, 12 },    // pins
    NULL
  };
  
  circuit_t *c;
  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {
    c = &sys.circuits[i];

    // configure circuit PID controller
    pid_set_input(&c->pid, &c->params[P_PRESSURE]);
    pid_set_param(&c->pid, c->params[P_KP], c->params[P_KI], c->params[P_KD]);
    // configure circuit pressure display
    c->disp = new TM1637Display(c->pins[I_DISP_CLK], c->pins[I_DISP_DIO]);
    c->disp->setBrightness(3);
    c->disp->clear();
  }
  
  sys.up_types = (1 << UP_CIRCUITS);
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

  // 100Hz button input polling timer
  TCCR1A = 0;                           // normal timer operation
  TCCR1B = (1 << CS11) | (1 << CS10);                 // clock select -> 8 prescalar
  OCR1A = (F_CPU / (100 * 8)) - 1;      // top value for timer used for comparisons
  TIMSK1 |= (1 << TOIE1);               // timer overflow interrupt enable for timer 1A
}

void poll_device(remote_t *r) {
  if (rx_packet(r) > 0) {
    process_packet(r);
  } else {
    issue_updates(r);
  }
}

ISR(TIMER1_OVF_vect) {
  static uint16_t state[C_NUM_CIRCUITS];
  uint8_t i;

  for (i = 0; i < C_NUM_CIRCUITS; i += 1) {
    // shifts inverted input through state variable.
    // 0's shift through state[i] if switch is closed
    state[i] = (state[i] << 1) | ((*sys.c_button.pin & (1 << i)) == 0) | 0xE000;
    // if 12 0's have shifted through state[i] before
    // we see the last open switch-state (a 1 in state[i]) -> accept a button press
    if (state[i] == 0xF000) {
      *sys.c_led.port ^= (1 << i);
    }
  }

  sys.en_flags = *sys.c_led.port;

  poll_device(&sys.remote);
}

void sim_tick() {
  sys.uptime = millis();

  // update_ui(&sys.ui);
  if (sys.s_flags == S_SHOT) {           // continuously checks pressures, keeping within setpoint range
    shot_pressure(false);
  } else if (sys.s_flags == S_PURGE) {   // bleed all circuits to 0 pressure
    purge();
  } else if (sys.s_flags == S_ABORT) {   // reduce all circuits to half-pressure
    shot_pressure(true);
  } if (sys.s_flags == S_ERROR) {
    // handle and log error
  }
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
  if (in) {
    if (!out) {
      c->params[P_PRESSURE] += c->roc * ((double)dt / 1000);
    } else {
      c->params[P_PRESSURE] -= c->roc * ((double)dt / 1000);
    }
  } else {
    if (out && c->params[P_PRESSURE] >= 0) {
      c->params[P_PRESSURE] -= c->roc * ((double)dt / 1000);
    }
  }

  c->params[P_PRESSURE] = clamp(0, 255, c->params[P_PRESSURE]);

  if (in || out) {
    c->disp->showNumberDecEx(c->params[P_PRESSURE], 0x40, false, 2, 0);
    c->disp->showNumberDecEx(100 * (c->params[P_PRESSURE] - (int)c->params[P_PRESSURE]), 0x40, true, 2, 2);
  }
}


/**
 * @brief purges all circuits to zero pressure (full purge)
 * 
 */
void purge() {
  uint32_t itime;
  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {    // loop system circuits
    itime = millis();
    if (sys.en_flags & (1 << i)) {
      // open exhaust, close intake
      digitalWrite(sys.circuits[i].pins[I_PRESSURE_IN], LOW);
      digitalWrite(sys.circuits[i].pins[I_PRESSURE_OUT], HIGH);
      while (sys.circuits[i].params[P_PRESSURE] > 0) {
        modify_circuit(&sys.circuits[i], millis() - itime, 0, 1);
      }
      // close exhaust
      digitalWrite(sys.circuits[i].pins[I_PRESSURE_OUT], LOW);
    }    
  }

  sys.s_flags = S_STANDBY;
}

/**
 * @brief modifies circuit pressure to approach set-point.
 * modifies pressure for a single PID window. should be called
 * iteratively.
 * 
 * @param c - circuit to modify
 * @param var - acceptable pressure variance
 * @param half - half-pressure set-point modifier
 */
void set_pressure(circuit_t *c, double var, int half) {
  uint32_t wstart;
  double out, set_point = (half) ? c->params[P_SET_POINT] / 2 : c->params[P_SET_POINT];
  int dir;

  pid_set_target(&c->pid, &set_point);
  
  if (!IN_RANGE(c->params[P_PRESSURE], (set_point - var), (set_point + var))) {
    // pick PID direction
    dir = (c->params[P_PRESSURE] > set_point + var) ? REVERSE : DIRECT;
    pid_set_direction(&c->pid, dir);
    wstart = millis();    // base time offset
    do {
      digitalWrite(c->pins[I_PRESSURE_OUT], dir);
      digitalWrite(c->pins[I_PRESSURE_IN], dir ^ 1);
      modify_circuit(c, millis() - wstart, dir ^ 1, dir);
      out = pid_compute(&c->pid);
      // poll_device(&sys.remote);
    } while (millis() - wstart < out);
  }

  // close circuit solenoids
  digitalWrite(c->pins[I_PRESSURE_IN], LOW);
  digitalWrite(c->pins[I_PRESSURE_OUT], LOW);
}

/**
 * @brief steps all enabled circuits towards desired set-point if
 * circuit check time has expired.
 * 
 * @param half - half-pressure set-point modifier
 */
void shot_pressure(bool half) {
  static int i = 0;
  double var = 0.05;
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

