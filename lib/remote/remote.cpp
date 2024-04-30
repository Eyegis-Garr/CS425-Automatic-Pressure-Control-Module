#include "remote.h"
#include <Arduino.h>

int init_remote(remote_t *r, Stream *s) {
  if (!s) return 0;

  r->s = s;
  r->load_state = 0;
  r->rx_head = r->rx.bytes;
  r->state = 0;

  return 0;
}

static size_t cobs_encode(const void *in, size_t length, uint8_t *out) {
	uint8_t *encode = out;            // Encoded byte pointer
	uint8_t *codep = encode++;        // Output code pointer
	uint8_t code = 1;                 // Code value

	for (const uint8_t *byte = (const uint8_t *)in; length--; ++byte) {
		if (*byte) {                     // Byte not zero, write it
			*encode++ = *byte, ++code;
    }

		if (!*byte || code == 0xff) {
			*codep = code, code = 1, codep = encode;
			if (!*byte || length) {
				++encode;
      }
		}
	}

	*codep = code;                      // Write final code value

	return (size_t)(encode - out);
}

static size_t cobs_decode(uint8_t *in, size_t length, void *out) {
  uint8_t *byte = in;       // pointer into encoded input buffer
  uint8_t *decode = (uint8_t *)out;    // pointer into decoded output buffer

  for (uint8_t code = 0xFF, block = 0; byte < in + length; --block) {
    if (block) {              // decodes block byte
      *decode++ = *byte++;    // reads into decoded output
    } else {
      block = *byte++;        // read next block length
      if (block && (code != 0xFF)) {    // encoded zero and not end-of-block
        *decode++ = 0;                  // write zero to output buffer
      }
      code = block;                     // reset block counter
      if (!code)                        // delimiter hit
        break;
    }
  }

  return (size_t)(decode - (uint8_t *)out);
}

static uint8_t chk8(uint8_t *data, size_t len) {
  uint8_t *rd;
  uint32_t s;

  for (s = 0, rd = data; rd < data + len; rd += 1) {
    s += *rd;
  }

  return (uint8_t)s;
}

int rx_packet(remote_t *r) {
  static uint8_t rx_buffer[PS_LENGTH + PS_HEADER + 1];
  uint8_t chk;
  size_t dec;
  int ret = 0, rd;

  if (isbclr(r->state, R_RXINP)) {
    r->rx_head = rx_buffer;
  }

  while (r->s->available()) {
    r->state |= (1 << R_RXINP);

    rd = r->s->read();

    if (rd >= 0) {
      *r->rx_head++ = rd;
    }

    if (rd == 0) {
      dec = cobs_decode(rx_buffer, r->rx_head - rx_buffer, r->rx.bytes);

      chk = chk8(r->rx.bytes, r->rx.packet.size + PS_HEADER - 1);
      
      // Serial.println(chk);
      // for (int i = 0; i < dec; i += 1) {
      //   Serial.println(r->rx.bytes[i]);
      // }
      // Serial.println("===");

      if (r->rx.packet.data[r->rx.packet.size - 1] == chk) {
        r->state |= (1 << R_NDATA);
        r->state &= ~(1 << R_RXINP);
        ret = 1;
        break;
      }

      r->rx_head = rx_buffer;
    }
  }

  return ret;
}

int tx_packet(packet_t *p, Stream *s) {
  static uint8_t tx_buffer[PS_LENGTH + PS_HEADER];
  size_t wr, en;
  uint8_t chk;

  p->packet.size += 1;

  chk = chk8(p->bytes, PS_HEADER + p->packet.size - 1);

  p->packet.data[p->packet.size - 1] = chk;
  
  en = cobs_encode(p->bytes, p->packet.size + PS_HEADER, tx_buffer);

  tx_buffer[en++] = 0;

  wr = s->write(tx_buffer, en);

  return (wr == en) ? 0 : -1;
}

size_t ack_packet(remote_t *r, packet_t *p) {
  size_t wr = 0;

  r->tx.packet.type = PK_ACK;
  r->tx.packet.flags = 0;
  r->tx.packet.size = PS_HEADER;
  r->tx.packet.timeout = 0;

  r->tx.bytes[0] = p->packet.type;
  r->tx.bytes[1] = p->packet.flags;
  r->tx.bytes[2] = p->packet.size;

  wr = tx_packet(&r->tx, r->s);
  
  return wr;
}

