#include "remote.h"

size_t tx_packet(packet_t *p, HardwareSerial *s) {
  size_t w = 0;
  uint8_t *wr = p->bytes;

  s->write(SOTX);
  wrblk(s);
  w += s->write(p->type);
  wrblk(s);
  w += s->write(p->flags);
  wrblk(s);
  w += s->write(p->size);
  wrblk(s);

  while (wr != p->bytes + p->size) {
    w += s->write(*wr++);
    wrblk(s);
  }

  s->write(EOTX);

  return w;
}

size_t rx_packet(HardwareSerial *s, packet_t *p) {
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