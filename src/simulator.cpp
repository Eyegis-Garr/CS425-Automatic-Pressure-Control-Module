#include "simulator.h"

system_t sys;

volatile uint8_t *PORT_B  = (volatile uint8_t *) 0x25;
volatile uint8_t *DDR_B   = (volatile uint8_t *) 0x24;
volatile uint8_t *PIN_B   = (volatile uint8_t *) 0x23;
volatile uint8_t *PORT_L  = (volatile uint8_t *) 0x10B;
volatile uint8_t *DDR_L   = (volatile uint8_t *) 0x10A;
volatile uint8_t *PIN_L   = (volatile uint8_t *) 0x109;

/**
 * @brief initializes simulator. should be invoked on startup or reset
 * 
 */
void sim_setup() {
  init_ui(&sys.ui);
  init_system();
  init_io();

  sys.pid_window_size = 5000;
}

void init_system() {
  circuit_t *c;

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
      50, 0, 25            // PID tuning params
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
      50, 0, 25            // PID tuning params
    },
    .roc = 0.05,
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
      50, 0, 25            // PID tuning params
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
      50, 0, 25            // PID tuning params
    },
    .roc = 0.15,
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
      50, 0, 25            // PID tuning params
    },
    .roc = 0.75,
    .supply = NULL,
    .reclaimer = NULL,
    .pins = { A7, 8, 21, 10, 45, 11, 12 }
  };
  
  sys.supply = S_SUPPLY_MAX;
  sys.reclaimer = 15;
  sys.rec_auto_on = 8;
  sys.rec_auto_off = 2;
  sys.supply_min = 10;

  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {
    c = &sys.circuits[i];

    // hook/attach circuit to system
    c->reclaimer = &sys.reclaimer;
    c->supply = &sys.supply;

    // configure circuit PID controller
    pid_set_input(&c->pid, &c->params[P_PRESSURE]);
    pid_set_param(&c->pid, c->params[P_KP], c->params[P_KI], c->params[P_KD]);
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

  // 100Hz button input polling timer
  TCCR1A = 0;                           // normal timer operation
  TCCR1B = (1 << CS11) | (1 << CS10);                 // clock select -> 8 prescalar
  OCR1A = (F_CPU / (100 * 8)) - 1;      // top value for timer used for comparisons
  TIMSK1 |= (1 << TOIE1);               // timer overflow interrupt enable for timer 1A
}

void poll_device(remote_t *r) {
  if (rx_packet(r) > 0) {
    process_packet(r);
  } else if (isbclr(r->state, R_RXINP)) {
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

/**
 * @brief updates simulation. should be invoked per iteration.
 * 
 */
void sim_tick() {
  sys.uptime = millis();

  // update_ui(&sys.ui);
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
}

void auto_reclaim() {
  static int r;
  
  if (!r && sys.reclaimer >= sys.rec_auto_on) {
    r = 1;
  }

  if (r) {
    if (sys.reclaimer >= sys.rec_auto_off) {
      sys.reclaimer -= 0.05;
      sys.supply += 0.05;
    } else {
      r = 0;
    }
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
  float pdt;

  pdt = ((float)dt) / 1024.0;

  if (in) {         /* intake open */
    if (!out) {     /* exhaust closed */
      // drain supply into circuit
      pdt *= mapf(*c->supply - c->params[P_PRESSURE], -C_PRESSURE_MAX, C_PRESSURE_MAX, -1, 1);
      *c->supply -= c->roc * pdt;
      c->params[P_PRESSURE] += c->roc * pdt;
    } else {        /* exhaust open */
      // drain supply into reclaimer
      pdt *= mapf(*c->reclaimer - *c->supply, -C_PRESSURE_MAX, C_PRESSURE_MAX, -1, 1);
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
      while (millis() - itime <= c->params[P_PURGE_TIME] * 1000) {
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
  float out, set_point = (half) ? c->params[P_SET_POINT] / 2 : c->params[P_SET_POINT];
  int dir;

  pid_set_target(&c->pid, &set_point);
  
  if (!IN_RANGE(c->params[P_PRESSURE], (set_point - var), (set_point + var))) {
    // pick PID direction
    dir = (c->params[P_PRESSURE] > set_point + var) ? REVERSE : DIRECT;
    pid_set_direction(&c->pid, dir);
    wstart = millis();    // base time offset
    do {
      // close intake, open exhaust
      digitalWrite(c->pins[I_PRESSURE_IN], dir ^ 1);
      digitalWrite(c->pins[I_PRESSURE_OUT], dir);
      modify_circuit(c, millis() - wstart, dir ^ 1, dir);
      out = pid_compute(&c->pid);
    } while (millis() - wstart < out);
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
  float var = 0.1;
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

