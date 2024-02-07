#include <Arduino.h>
#include <stdlib.h>
#include <inttypes.h>

#include "simulator.h"

#define SOTX        0xAA
#define EOTX        0x55
#define PK_UPDATE   1
#define PK_COMMAND  (1 << 1)
#define PK_STATUS   (1 << 2)

// system -> client
#define UP_SYSTEM   1
#define UP_CIRCUIT  (1 << 1)
#define UP_IFACE    (1 << 2)
#define UP_UPTIME   (1 << 3)

// client -> system
#define CMD_MODESET
#define CMD_CSELECT
#define CMD_PSELECT
#define CMD_VALSET
#define CMD_SAVE

typedef struct packet_t {
  uint8_t type;   // update, command, keepalive, ack (?)
  uint8_t flags;
  uint8_t size;

  uint8_t *bytes;
} packet_t;

uint8_t p_sys_bytes[sizeof(system_t)], p_circuit_bytes[sizeof(circuit_t)];
packet_t p_sys, p_command, p_status;

size_t packetize_circuit(circuit_t *c, uint8_t *bytes) {
  uint8_t *base = bytes;

  // set circuit double-type parameter byte-length @ first index
  // use two bytes for every double type (integer + fractional parts)
  // (+ 1 parameter (+ 2 bytes) for pressure reading)
  *bytes++ = 2 * (C_NUM_PARAM + 1);

  *bytes++ = (uint8_t) c->pressure;
  *bytes++ = (uint8_t) ((c->pressure - ((uint8_t) c->pressure)) * 100);
  for (uint8_t i = 0; i < C_NUM_PARAM; i += 1) {
    *bytes++ = (uint8_t) c->params[i];
    *bytes++ = (uint8_t) ((c->params[i] - ((uint8_t) c->params[i])) * 100);
  }

  // set circuit binary-state (solenoid io, en button, en LED)
  *bytes++ = (digitalRead(c->pins[I_PRESSURE_IN]) << I_PRESSURE_IN)    |
             (digitalRead(c->pins[I_PRESSURE_OUT]) << I_PRESSURE_OUT)  |
             (digitalRead(c->pins[I_ENABLE_BTN]) << I_ENABLE_BTN)      |
             (digitalRead(c->pins[I_LED]) << I_LED);
  
  return bytes - base;
}

void packetize_system(packet_t *p, system_t *s) {
  /*
    SYSTEM UPDATE DATA
      system flags
        state/mode, param/edit circuit, circuit enable, alarms/sound
        current preset, time since last save, unwritten changes (custom preset)
      circuit state
        pressure, solenoid state, params, pid (?)
        extra-circuit state
          supply pressure, reclaimer state
      ui state (maybe only for logging purposes?)
        path trace
      system runtime (millis() or RTC reading)
      
  */
  p->type = PK_UPDATE;
  p->flags = UP_SYSTEM | UP_CIRCUIT;
  p->size = 4;
  p->bytes = p_sys_bytes;

  // store system flags
  p->bytes[0] = sys.s_flags;
  p->bytes[1] = sys.c_flags;
  p->bytes[2] = sys.p_flags;
  p->bytes[3] = sys.en_flags;

  // store circuit state
  uint8_t *cbytes = &p->bytes[4];
  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {
    p->size += packetize_circuit(&sys.circuits[i], cbytes);
  }
  
  // store other state
}

size_t tx_packet(packet_t *p, HardwareSerial *s) {
  size_t w = 0;
  uint8_t *wr;

  if (s->availableForWrite()) {
    w += s->write(p->type);
    while (!s->availableForWrite()) { }
    w += s->write(p->flags);
    while (!s->availableForWrite()) { }
    w += s->write(p->size);

    wr = p->bytes;
    while (wr != p->bytes + p->size) {
      w += s->write(*wr++);
      while (!s->availableForWrite()) { }
    }

    w += s->write(EOTX);
  }

  return w;
}

size_t rx_packet(HardwareSerial *s, packet_t *p) {
  size_t r = 0;
  if (s->available()) {
    p->type = s->read(); r += 1;
    while (!s->available()) { }
    p->size = s->read(); r += 1;

    r += s->readBytes(p->bytes, p->size);
  }

  return r;
}

void setup() {
  sim_setup();
  Serial1.begin(9600);
  Serial.begin(9600);

  packetize_system(&p_sys, &sys);
  size_t b = tx_packet(&p_sys, &Serial1);
  Serial.println(b);
}

void loop() {
  // sim_tick();
}
