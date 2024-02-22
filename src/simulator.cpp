#include "simulator.h"

system_t sys;

volatile uint8_t *PORT_B  = (volatile uint8_t *) 0x25;
volatile uint8_t *DDR_B   = (volatile uint8_t *) 0x24;
volatile uint8_t *PIN_B   = (volatile uint8_t *) 0x23;
volatile uint8_t *PORT_L  = (volatile uint8_t *) 0x10B;
volatile uint8_t *DDR_L   = (volatile uint8_t *) 0x10A;
volatile uint8_t *PIN_L   = (volatile uint8_t *) 0x109;

ISR(TIMER1_COMPA_vect) {
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
}

ISR(USART1_RX_vect) {
  uint8_t r = UDR1;

  cli();
  if (r == SOTX) {
    int rd = rx_packet(&sys.remote, &sys.pkt);
    if (rd) {
      sys.s_flags |= (1 << S_REMOTE);
    }
  }
  sei();
}

void sim_setup() {
  init_ui(&sys.ui);
  init_system();
  init_io();

  sys.pid_window_size = 5000;
}

void init_system() {
  sys.s_flags = (1 << S_SHOT);
  sys.c_flags = 0;
  sys.p_flags = 0;
  sys.en_flags = (1 << C_MARX);

  // load default circuit settings
  sys.circuits[C_MARX] = (circuit_t) {
    {
      50,                  // pressure
      25.75,               // set point
      C_MAX_DEFAULT,       // max time
      1000,                // check time
      C_PURGE_DEFAULT,     // purge time
      C_DELAY_DEFAULT,     // delay time
      50, 0, 25            // PID tuning params
    },
    3,                  // pressure rate-of-change
    { A7, 13, 13, 53, 49, 2, 3 },    // pins
    NULL
  };

  sys.circuits[C_MTG70] = (circuit_t) {
    {
      50,                  // pressure
      25.75,               // set point
      C_MAX_DEFAULT,       // max time
      1000,                // check time
      C_PURGE_DEFAULT,     // purge time
      C_DELAY_DEFAULT,     // delay time
      50, 0, 25            // PID tuning params
    },
    5,                  // pressure rate-of-change
    { A7, 43, 39, 53, 49, 4, 5 },    // pins
    NULL
  };

  sys.circuits[C_MTG] = (circuit_t) {
    {
      50,                  // pressure
      25.75,               // set point
      C_MAX_DEFAULT,       // max time
      1000,                // check time
      C_PURGE_DEFAULT,     // purge time
      C_DELAY_DEFAULT,     // delay time
      50, 0, 25            // PID tuning params
    },
    6,                  // pressure rate-of-change
    { A7, 40, 13, 53, 49, 6, 7 },    // pins
    NULL
  };
  
  
  sys.circuits[C_SWITCH] = (circuit_t) {
    {
      50,                  // pressure
      25.75,               // set point
      C_MAX_DEFAULT,       // max time
      1000,                // check time
      C_PURGE_DEFAULT,     // purge time
      C_DELAY_DEFAULT,     // delay time
      50, 0, 25            // PID tuning params
    },
    4.5,                  // pressure rate-of-change
    { A7, 16, 17, 53, 49, 8, 9 },    // pins
    NULL
  };

  
  sys.circuits[C_SWTG70] = (circuit_t) {
    {
      50,                  // pressure
      25.75,               // set point
      C_MAX_DEFAULT,       // max time
      1000,                // check time
      C_PURGE_DEFAULT,     // purge time
      C_DELAY_DEFAULT,     // delay time
      50, 0, 25            // PID tuning params
    },
    5,                  // pressure rate-of-change
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

  // sys.up_types =  (1 << UP_SYSTEM) | (1 << UP_CIRCUITS);
  sys.up_types = 0;
  sys.uptime = 0;

  sys.pkt.bytes = sys.pbuf;
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

  // 1KHz button input polling timer (prescalar -> 64, output comare -> 2000 - 1)
  TCCR1A = 0; // normal timer operation
  TCCR1B |= (1 << WGM12); // clear timer on compare
  TCCR1B |= (1 << CS11) | (1 << CS10); // clock select -> 64 prescalar
  OCR1A = 1999; // top value for timer used for comparisons
  TIMSK1 |= (1 << OCIE1A);  // compare match interrupt enable for timer 1A

  // enable recieve complete interrupts for USART1 (Serial1)
  sys.remote = (usart_t) {
    &UDR1,
    &UCSR1A,
    &UCSR1B,
    &UCSR1C,
    &UBRR1L,
    &UBRR1H
  };

  usart_init(&sys.remote, 9600);
  usart_enable_rx_interrupt(&sys.remote);
}

void sim_tick() {
  update_ui(&sys.ui);
  if (sys.s_flags & (1 << S_SHOT)) {           // continuously checks pressures, keeping within setpoint range
    shot_pressure(false);
  } else if (sys.s_flags & (1 << S_PURGE)) {   // bleed all circuits to 0 pressure
    purge();
  } else if (sys.s_flags & (1 << S_ABORT)) {   // reduce all circuits to half-pressure
    shot_pressure(true);
  }

  sys.uptime = millis(); 

  if (sys.s_flags & (1 << S_REMOTE)) {
    sys.s_flags &= ~(1 << S_REMOTE);
    int p = process_pkt();
  }
}

/**
 * @brief WIP processes received packets
 * 
 * @return int - 0 on success, 1 on error
 */
int process_pkt() {
  packet_t *p = &sys.pkt;
  uint8_t *rd = sys.pkt.bytes;

  switch (p->type) {
    case PK_UPDATE:
      sys.up_types = p->flags;
      if (p->flags & (1 << UP_CIRCUITS)) {
        sys.c_flags = *rd++;  // store circuits to send updates for
        sys.p_flags = *rd++;  // store parameters to package
      } if (p->flags & (1 << UP_REFRESH)) {
        issue_updates(&sys.remote);
      }
      break;
    case PK_COMMAND:
      rd += parse_command(p->flags, rd);
    default:
      return 1;
  }

  return 0;
}

/**
 * @brief WIP command packet processing
 * 
 * @param flags - command packet flags
 * @param bytes - command packet payload
 * @return size_t - number of bytes processed
 */
size_t parse_command(uint8_t flags, uint8_t *bytes) {
  uint8_t cmask = 0, *cbytes = bytes;
  uint16_t pmask = 0;

  if (flags & (1 << CMD_MODESET)) {
    sys.s_flags = *bytes++;
  } if (flags & (1 << CMD_PSET)) {
    cmask = *cbytes++;
    pmask = *cbytes++;
    pmask <<= 8;
    pmask |= *cbytes++;
    bytes += packetize_circuits(cbytes, cmask, pmask, 0);
  }

  return (cbytes - bytes);
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

  if (in || out) {
    c->disp->showNumberDecEx(c->params[P_PRESSURE], 0x40, false, 2, 0);
    c->disp->showNumberDecEx(100 * (c->params[P_PRESSURE] - (int)c->params[P_PRESSURE]), 0x40, true, 2, 2);
  }
}

/**
 * @brief transmits update packet based on 
 * system update flags.
 * 
 * @param u - serial transmitter/remote connection
 * @return int - number of bytes sent
 */
int issue_updates(usart_t *u) {
  packet_t *p = &sys.pkt;
  if (sys.up_types) {
    p->type = (1 << PK_UPDATE);
    p->flags = sys.up_types;
    p->size = 0;
    if (sys.up_types & (1 << UP_SYSTEM)) {
      p->size += packetize_system(p->bytes);
    } if (sys.up_types & (1 << UP_CIRCUITS)) {
      p->size += packetize_circuits(p->bytes + p->size, sys.c_flags, sys.p_flags, 1);
    }
  }

  // send packet over configured serial device
  return tx_packet(p, u);
}

/**
 * @brief circuit packeting & depacketing
 * 
 * @param bytes - source or destination packet-buffer
 * @param cmask - masks affect circuits
 * @param pmask - masks affect parameters
 * @param dir - 1 -> loads packet : 0 -> unloads packet
 * @return size_t - bytes packed
 */
size_t packetize_circuits(uint8_t *bytes, uint8_t cmask, uint16_t pmask, uint8_t dir) {
  uint8_t *pdata = bytes;
  double *cdata;
  circuit_t *c;

  if (dir) {
    pdata += 1; // use bytes+1 to leave space for size @ bytes[0]
    *pdata++ = cmask;
    *pdata++ = (uint8_t)(pmask >> 8);
    *pdata++ = (uint8_t) pmask;
  }

  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {
    if ((1 << i) & cmask) {
      c = &sys.circuits[i];
      cdata = c->params;
      for (uint8_t k = 0; k < C_NUM_PARAM; k += 1) {
        if ((1 << k) & pmask) {
          if (dir) {
            *pdata++ = (uint8_t) c->params[k];
            *pdata++ = (uint8_t) ((c->params[k] - ((uint8_t) c->params[k])) * 100);
          } else {
            *cdata++ = *pdata + ((double)(*(pdata + 1)) / 100);
            pdata += 2;
          }
        }
      }

      if (dir) {
        // set circuit binary-state (solenoid io, en button, en LED)
        *pdata++ = (digitalRead(c->pins[I_PRESSURE_IN]) << I_PRESSURE_IN)   |
                  (digitalRead(c->pins[I_PRESSURE_OUT]) << I_PRESSURE_OUT)  |
                  (digitalRead(c->pins[I_ENABLE_BTN]) << I_ENABLE_BTN)      |
                  (digitalRead(c->pins[I_LED]) << I_LED);
      }
    }
  }

  // byte length of circuit data
  if (dir) bytes[0] = pdata - bytes;
  
  return pdata - bytes;
}

/**
 * @brief loads system-related state into packet buffer
 * 
 * @param bytes - destination packet buffer
 * @return size_t - bytes packed
 */
size_t packetize_system(uint8_t *bytes) {
  uint8_t *sbytes = bytes + 1;

  // store system flags
  *sbytes++ = sys.s_flags;
  *sbytes++ = sys.c_flags;
  *sbytes++ = (uint8_t) sys.p_flags >> 8;
  *sbytes++ = (uint8_t) sys.p_flags;
  *sbytes++ = sys.en_flags;
  // store last recieved client update flags
  *sbytes++ = sys.up_types;
  // store system uptime
  *sbytes++ = (uint8_t) sys.uptime;
  *sbytes++ = (uint8_t) (sys.uptime >> 8);
  *sbytes++ = (uint8_t) (sys.uptime >> 16);
  *sbytes++ = (uint8_t) (sys.uptime >> 24);

  // byte-length of system state
  bytes[0] = sbytes - bytes;

  return sbytes - bytes;
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

  sys.s_flags = (1 << S_STANDBY);
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
    while (millis() - wstart < out + 50) {
      digitalWrite(c->pins[I_PRESSURE_OUT], dir);
      digitalWrite(c->pins[I_PRESSURE_IN], dir ^ 1);
      modify_circuit(c, millis() - wstart, dir ^ 1, dir);
      out = pid_compute(&c->pid);
    }
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
  double var = 0.05;
  circuit_t *c;

  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {  // loop system circuits
    if (sys.en_flags & (1 << i)) {   // if enabled
      c = &sys.circuits[i];
      if (millis() - c->params[P_CHECK_TIME] >= c->params[P_DELAY_TIME]) {   // check time expired
        set_pressure(c, var, half);
        c->params[P_CHECK_TIME] = millis();
      }
    }
  }
}

