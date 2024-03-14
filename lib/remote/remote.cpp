#include "remote.h"
#include <Arduino.h>

int init_remote(remote_t *r, Stream *s) {
  if (!s) return 0;

  r->s = s;
  r->load_state = 0;
  r->rx_head = r->rx.bytes;
  r->r_flags = 0;

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

int rx_packet(remote_t *r) {
  static uint8_t rx_buffer[PS_LENGTH + PS_HEADER + 1];
  int ret = 0, rd;

  if (isbclr(r->r_flags, R_RXINP)) {
    r->rx_head = rx_buffer;
  }

  while (r->s->available()) {
    r->r_flags |= (1 << R_RXINP);

    rd = r->s->read();

    if (rd == 0) {
      cobs_decode(rx_buffer, r->rx_head - rx_buffer, &r->rx);

      // Serial.println("============================");
      // Serial.print("type: "); Serial.println(r->rx.type);
      // Serial.print("flags: "); Serial.println(r->rx.flags);
      // Serial.print("size: "); Serial.println(r->rx.size);
      // for (int i = 0; i < r->rx.size; i += 1) {
      //   Serial.print("byte: "); Serial.println(r->rx.bytes[i]);
      // }

      r->rx_head = rx_buffer;
      r->r_flags &= ~(1 << R_RXINP);
      r->r_flags |= (1 << R_NDATA);
      ret = 1;
    } else if (rd > 0) {
      *r->rx_head++ = rd;
    }
  }

  return ret;
}

size_t tx_packet(packet_t *p, Stream *s) {
  static uint8_t tx_buffer[PS_LENGTH + PS_HEADER];
  size_t wr = 0;

  // Serial.println("============================");
  // Serial.print("type: "); Serial.println(p->type);
  // Serial.print("flags: "); Serial.println(p->flags);
  // Serial.print("size: "); Serial.println(p->size);
  // for (int i = 0; i < p->size; i += 1) {
  //   Serial.print("byte: "); Serial.println(p->bytes[i]);
  // }

  wr = cobs_encode(p, p->size + PS_HEADER, tx_buffer);

  tx_buffer[wr++] = 0;

  wr = s->write(tx_buffer, wr);

  return wr;
}

size_t ack_packet(remote_t *r, packet_t *p) {
  size_t wr = 0;

  r->tx.type = PK_ACK;
  r->tx.flags = 0;
  r->tx.size = PS_HEADER;

  r->tx.bytes[0] = p->type;
  r->tx.bytes[1] = p->flags;
  r->tx.bytes[2] = p->size;

  wr = tx_packet(&r->tx, r->s);
  
  return wr;
}

