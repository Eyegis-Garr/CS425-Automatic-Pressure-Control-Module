#include <AltSoftSerial.h>

// packet head/tail bytes
#define SOTX        0xAA
#define EOTX        0x55

#define PS_LENGTH  255
#define PS_HEADER  3
// packet type
#define PK_UPDATE   0  // state update packet (required response)
#define PK_COMMAND  1  // state config packet (specified response)
#define PK_STATUS   2  // comm status packet  (required response)

#define ST_CTS      0  // clear to send
#define ST_RTS      1  // request to send
#define ST_TIMEOUT  2  // broken packet (?)

// system <-> client
#define UP_SYSTEM   0  // full system update
#define UP_CIRCUITS 1  // circuit update
#define UP_REFRESH  2  // system update trigger

// client -> system
#define CMD_RESPND  0  // response timeout flag
#define CMD_MODESET 1  // config mode
#define CMD_PARSET    2  // parameter set

#define R_RXINP 0   // receive in progress
#define R_NDATA 1   // new data to process
#define R_TIME  2   // timeout counter active

typedef struct packet_t {
  uint8_t type;
  uint8_t flags;
  uint8_t size;

  uint8_t bytes[PS_LENGTH];
} packet_t;

typedef struct remote_t remote_t;
typedef struct remote_t {
  Stream *s;

  uint8_t *rx_head;

  packet_t rx;    // device packets

  uint32_t irx;   // initial rx time
  uint32_t itx;   // initial tx time
  uint32_t tom;   // time-out max

  uint8_t r_flags;    // remote state

  remote_t *dest;
} remote_t;

size_t tx_packet(packet_t *p, Stream *s);
int load_packet(remote_t *r);

remote_t client;
remote_t rsys;

void setup() {
  Serial.begin(9600);

  client.rx_head = client.rx.bytes;
}

#define isbset(d, b) (((d) & (1 << (b))) != 0)
#define isbclr(d, b) (((d) & (1 << (b))) == 0)
int process_packet(remote_t *src, remote_t *dest, packet_t *p) {
  int ret = 0;

  if (isbclr(dest->r_flags, R_RXINP)) {     // is data coming in from dest?
    if (isbclr(src->r_flags, R_RXINP)) {    // is data coming in from source?
      switch (p->type) {
        case PK_STATUS:
        case PK_UPDATE:
          src->itx = millis();
          src->tom = p->bytes[p->size--];   // read and strip time-out max
          src->r_flags |= (1 << R_TIME);
          break;
        case PK_COMMAND:
          if (isbset(p->flags, CMD_RESPND)) {
            src->itx = millis();
            src->tom = p->bytes[p->size--];   // read and strip time-out max
            src->r_flags |= (1 << R_TIME);
          } else {
            src->r_flags &= ~(1 << R_TIME);
          }
          break;
        default:
          break;
      }
      ret = tx_packet(p, dest->s);
    }
  }

  return ret;
}

AltSoftSerial xbee;

void poll_device(remote_t *r) {
  if (load_packet(r)) {
    r->r_flags &= ~(1 << R_RXINP);
    r->irx = millis();
    r->r_flags |= (1 << R_NDATA);
  }

  if (isbset(r->r_flags, R_NDATA)) {
    if (process_packet(r, r->dest, &r->rx) == 0) {
      r->r_flags &= ~(1 << R_NDATA);
    }
  }

  if (isbset(r->r_flags, R_TIME)) {
    if (millis() - r->itx >= r->tom) {
      // issue packet timeout to client
      if (timeout(r)) { r->r_flags &= ~(1 << R_TIME); }
    }
  }
}

void loop() {
  poll_device(&client);
  poll_device(&rsys);
}

int load_packet(remote_t *r) {
  static int ld_st = 0;
  int ret = 0;

  if (r->s->available()) {
    *r->rx_head++ = r->s->read();
    // set RX in progress
    r->r_flags |= (1 << R_RXINP);
  }

  if (!ld_st && r->rx_head - r->rx.bytes >= PS_HEADER + 1) {
    r->rx_head = r->rx.bytes + 1;

    r->rx.type = *r->rx_head++;
    r->rx.flags = *r->rx_head++;
    r->rx.size = *r->rx_head++;

    r->rx_head = r->rx.bytes;
    ld_st = 1;
  } else if (ld_st && (r->rx_head - r->rx.bytes) >= r->rx.size + 1) {
    r->rx_head = r->rx.bytes;
    ret = 1;
    ld_st = 0;
  }

  return ret;
}

size_t tx_packet(packet_t *p, Stream *s) {
  size_t w = 0;
  uint8_t *wr;

  s->write(SOTX);

  w += s->write(p->type);
  w += s->write(p->flags);
  w += s->write(p->size);

  wr = p->bytes;
  while (wr != p->bytes + p->size) {
    w += s->write(*wr++);
  }

  s->write(EOTX);

  return w;
}
