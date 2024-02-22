#include "remote.h"

size_t tx_packet(packet_t *p, usart_t *u) {
  size_t w = 0;
  uint8_t *wr = p->bytes;

  cli();
  *u->udr = SOTX;
  wrblk(u->ucsra);
  *u->udr = p->type; w += 1;
  wrblk(u->ucsra);
  *u->udr = p->flags; w += 1;
  wrblk(u->ucsra);
  *u->udr = p->size; w += 1;
  wrblk(u->ucsra);

  while (wr != p->bytes + p->size) {
    *u->udr = *wr++; w += 1;
    wrblk(u->ucsra);
  }

  *u->udr = EOTX;

  sei();
  return w;
}

size_t rx_packet(usart_t *u, packet_t *p) {
  size_t r = 0, i;

  rdblk(u->ucsra);
  p->type = *u->udr; r += 1;
  rdblk(u->ucsra);
  p->flags = *u->udr; r += 1;
  rdblk(u->ucsra);
  p->size = *u->udr; r += 1;

  for (i = 0; i < p->size; i += 1) {
    rdblk(u->ucsra);
    p->bytes[i] = *u->udr;
  }

  return r + i;
}

void usart_init(usart_t *u, uint32_t baud) {
  uint16_t ubrr = F_CPU / (16 * baud) - 1;

  *u->ubrrh = (uint8_t)(ubrr >> 8);
  *u->ubrrl = (uint8_t)(ubrr);

  // enable receiver
  *u->ucsrb = (1 << RX_ENABLE) | (1 << TX_ENABLE);

  // 8 data-bits, 1 stop-bit, no parity
  *u->ucsrc = (1 << CH_SZ1) | (1 << CH_SZ0);
}

void usart_enable_rx_interrupt(usart_t *u) {
  *u->ucsrb |= (1 << RXC_IENABLE);
}
