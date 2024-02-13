#include <AltSoftSerial.h>

#define SOTX        0xAA
#define EOTX        0x55

#define PK_UPDATE   1
#define PK_COMMAND  (1 << 1)
#define PK_STATUS   (1 << 2)

// system -> client
#define UP_SYSTEM   1
#define UP_CIRCUITS  (1 << 1)
#define UP_IFACE    (1 << 2)
#define UP_UPTIME   (1 << 3)

// client -> system
#define CMD_MODESET 1
#define CMD_CSELECT (1 << 1)
#define CMD_PSELECT (1 << 2)
#define CMD_VALSET  (1 << 3)
#define CMD_SAVE    (1 << 4)

AltSoftSerial xbee;

typedef struct packet_t {
  uint8_t type;   // update, command, keepalive, ack (?)
  uint8_t flags;
  uint8_t size;

  uint8_t *bytes;
} packet_t;

uint8_t pbuf[64];
packet_t p_rx, p_tx;

void gen_modeset(packet_t *p, uint8_t mode) {
  p->type = PK_COMMAND;
  p->flags = CMD_MODESET;
  p->size = 1;

  p->bytes = pbuf;
  p->bytes[0] = mode;
}

size_t tx_packet(packet_t *p, AltSoftSerial *s) {
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

  w += s->write(EOTX);
}

size_t rx_packet(AltSoftSerial *s, packet_t *p) {
  size_t r = 0;

  while (!s->available()) { }
  p->type = s->read(); r += 1;
  while (!s->available()) { }
  p->flags = s->read(); r += 1;
  while (!s->available()) { }
  p->size = s->read(); r += 1;

  r += s->readBytes(p->bytes, p->size);

  return r;
}

void setup() {
  xbee.begin(9600);
  Serial.begin(9600);

  p_tx.bytes = pbuf;
  p_rx.bytes = pbuf;
  gen_modeset(&p_tx, 0xAA);
  tx_packet(&p_tx, &xbee);
}

void loop() {
  if (xbee.available()) {
    if (xbee.read() == SOTX) {
      int r = rx_packet(&xbee, &p_rx);
      if (r) {
        Serial.print("RX BYTES: "); Serial.println(r);
        Serial.print("PK TYPE: "); Serial.println(p_rx.type);
        Serial.print("PK FLAGS:"); Serial.println(p_rx.flags);
        Serial.print("PK SIZE: "); Serial.println(p_rx.size);
        for (int i = 0; i < p_rx.size; i += 1) {
          Serial.println(p_rx.bytes[i]);
        }
      }
    }
  }
}
