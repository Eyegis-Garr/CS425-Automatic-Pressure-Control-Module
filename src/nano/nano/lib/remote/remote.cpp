#include "remote.h"

int init_remote(remote_t *r, Stream *s) {
  if (!s) return 0;

  r->s = s;
  r->load_state = 0;
  r->pkt_head = r->pkt.bytes;
  r->r_flags = 0;

  return 0;
}

static size_t cobs_encode(const void *data, size_t length, uint8_t *buffer) {
	uint8_t *encode = buffer; // Encoded byte pointer
	uint8_t *codep = encode++; // Output code pointer
	uint8_t code = 1; // Code value

	for (const uint8_t *byte = (const uint8_t *)data; length--; ++byte) {
		if (*byte) // Byte not zero, write it
			*encode++ = *byte, ++code;

		if (!*byte || code == 0xff) {
			*codep = code, code = 1, codep = encode;
			if (!*byte || length)
				++encode;
		}
	}
	*codep = code; // Write final code value

	return (size_t)(encode - buffer);
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

  return (size_t)(decode - (uint8_t)out);
}

int load_packet(remote_t *r) {
  static uint8_t rx_buffer[PS_LENGTH + PS_HEADER];
  size_t rd;
  int ret = 0;

  if (isbclr(r->r_flags, R_RXINP)) {
    r->pkt_head = rx_buffer;
  }

  if (r->s->available()) {
    rd = r->s->readBytesUntil(0, rx_buffer, PS_LENGTH + PS_HEADER);

    r->pkt_head += rd;

    r->r_flags |= (1 << R_RXINP);

    if (*(r->pkt_head) == 0) {
      cobs_decode(rx_buffer, r->pkt_head - rx_buffer + 1, &r->pkt);
      r->pkt_head = rx_buffer;
      r->r_flags &= ~(1 << R_RXINP);
      ret = 1;
    }
  }

  return ret;
}

size_t tx_packet(packet_t *p, Stream *s) {
  static uint8_t tx_buffer[PS_LENGTH + PS_HEADER];
  size_t wr = 0;

  wr = cobs_encode(p, p->size + PS_HEADER, tx_buffer);

  tx_buffer[wr++] = 0;

  wr = s->write(tx_buffer, wr);

  return wr;
}

size_t ack_packet(remote_t *r, packet_t *p) {
  size_t wr = 0;

  r->pkt.type = PK_ACK;
  r->pkt.flags = 0;
  r->pkt.size = PS_HEADER;

  r->pkt.bytes[0] = p->type;
  r->pkt.bytes[1] = p->flags;
  r->pkt.bytes[2] = p->size;

  wr = tx_packet(&r->pkt, r->s);
  
  return wr;
}

size_t timeout(remote_t *r) {
  size_t wr = 0;

  r->pkt.type = PK_STATUS;
  r->pkt.flags = (1 << ST_TIMEOUT);
  r->pkt.size = 0;

  wr = tx_packet(&r->pkt, r->s);

  return wr;  
}

