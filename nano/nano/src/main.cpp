#include <AltSoftSerial.h>
#include "remote.h"

#define D_NOACK 0   // waiting for ACK

typedef struct device_t device_t;
typedef struct device_t {
  remote_t remote;

  uint8_t ack_mask;   // masks packet responses to clear timeout
  uint32_t itx;   // initial TX time
  uint32_t tom;   // RX timeout max

  uint8_t d_flags;

  device_t *dest;
} device_t;

AltSoftSerial xbee;

device_t client;
device_t rsys;

void setup() {
  Serial.begin(9600);
  xbee.begin(9600);

  init_remote(&client.remote, &Serial);
  init_remote(&rsys.remote, &xbee);

  client.dest = &rsys;
  rsys.dest = &client;
}

void configure_timeout(device_t *d, uint8_t resp_mask, uint8_t sec) {
  if (!sec) return;               // 0 second timeout -> timeout disabled.
  d->itx = millis();
  d->tom = sec * 1000;
  d->d_flags |= (1 << D_NOACK);   // set on TX, cleared on sufficient RX
  d->ack_mask = resp_mask;
}

int process_packet(device_t *src, device_t *dest, packet_t *p) {
  int ret = 0;

  if (isbset(dest->d_flags, D_NOACK)) {     // dest device waiting for ack
    if (isbset(dest->ack_mask, p->type)) {  // if packet matches valid ack packet types
      dest->d_flags &= ~(1 << D_NOACK);     // clear missing ACK
      ret = tx_packet(p, dest->remote.s);
    }
  } else if (isbclr(src->d_flags, D_NOACK)) {
    // processes timeouts and fowards from src -> dest
    switch (p->type) {
      case PK_STATUS:
        if (isbset(p->flags, ST_PING)) {
          configure_timeout(src, (1 << PK_STATUS), p->bytes[p->size - 1]);
        } 
        break;
      case PK_UPDATE:
        // set timeout, valid responses are ACK and UPDATE packets
        configure_timeout(src, (1 << PK_UPDATE) | (1 << PK_ACK), p->bytes[p->size - 1]);
        break;
      case PK_COMMAND:
        if (isbset(p->flags, CMD_RESPND)) {
          // set timeout, valid responses are ACK packets
          configure_timeout(src, (1 << PK_ACK), p->bytes[p->size - 1]);
        }
        break;
      default:
        break;
    }

    // forwards packet
    ret = tx_packet(p, dest->remote.s);
  }

  return ret;
}

void poll_device(device_t *d) {
  remote_t *r = &d->remote;

  if (load_packet(r)) {
    process_packet(d, d->dest, &r->pkt);
  }

  if (isbset(d->d_flags, D_NOACK)) {
    if (millis() - d->itx >= d->tom) {    // if expired, issue timeout
      if (timeout(r)) {
        d->d_flags &= ~(1 << D_NOACK);      // clear missing ACK
      }
    }
  }
}

void loop() {
  // tx_packet(&rsys.remote.rx, rsys.remote.s);
  poll_device(&rsys);
  poll_device(&client);
}

