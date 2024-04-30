/**
 * @file remote.c
 * @author Bradley Sullivan (bradleysullivan@nevada.unr.edu)
 * @brief Serial remote communication implementations
 * @version 0.1
 * @date 2024-04-24
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "remote.h"

/**
 * @brief Allocates and/or initializes remote comm.
 * structure. If 'r' is NULL, a structure is allocated
 * and returned
 * 
 * @param r Pointer to remote structure to initialize
 * @param path Path to serial device
 * @param speed Terminal baud rate defined in 'termios.h'
 * (i.e. B9600, B19200, etc.)
 * @return remote_t* Pointer to allocated/initialized structure
 */
remote_t *r_get(remote_t *r, char *path, int speed) {
  remote_t *out;
  
  if (!r)
    out = malloc(sizeof(remote_t));
  else
    out = r;

  strncpy(out->dev_buf, path, 32);
  out->rd_tail = out->data;
  out->rx_head = out->data;
  out->wr_head = RNEXT(out->rx_head, out->data);
  out->tx_head = out->out;
  out->state = 0;

  out->speed = speed;

  connect(out);

  out->pfds = calloc(1, sizeof(struct pollfd));
  out->pfds->fd = out->fd;
  out->pfds->events = POLLIN | POLLERR | POLLHUP;

  return out;
}

/**
 * @brief Deallocation routine for remote structure
 * 
 * @param r Pointer to structure to deallocate
 */
void r_free(remote_t *r) {
  if (!r) return;

  if (r->fd > 0) close(r->fd);

  free(r->pfds);
  free(r);  
}

/**
 * @brief Remote ring-buffer packet read routine.
 * Tests for new data if head != tail, returns pointer
 * to next packet. Advances read-tail if data is
 * available.
 * 
 * @param r Remote structure to read from
 * @return packet_t* Pointer to unread packet
 */
packet_t *r_read(remote_t *r) {
  packet_t *p;
  
  p = NULL;

  if (r->rd_tail != r->rx_head) {
    p = r->rd_tail;
    r->rd_tail = RNEXT(r->rd_tail, r->data);
  }

  return p;
}

/**
 * @brief Initializes remote structure terminal.
 * Utilizes provided serial device path to setup
 * 'termios.h' serial terminal. Opens with options
 * O_RDWR | O_NOCTTY | O_NDELAY and mode S_IRUSR | S_IWUSR
 * 
 * @param r Remote structure to connect
 * @return int Returns 0 on success, -1 on failure
 */
int connect(remote_t *r) {
  int ret = 0;
  struct termios op;

  r->fd = open(r->dev_buf, O_RDWR | O_NOCTTY | O_NDELAY, S_IRUSR | S_IWUSR);
  if (r->fd >= 0) {
    tcgetattr(r->fd, &op);

    cfsetispeed(&op, r->speed);
    cfsetospeed(&op, r->speed);

    // enable read
    op.c_cflag |= (CLOCAL | CREAD);

    // 8N1 (no parity)
    op.c_cflag &= ~PARENB;
    op.c_cflag &= ~CSTOPB;
    op.c_cflag &= ~CSIZE;
    op.c_cflag |= CS8;

    // no hw/sw flow ctrl, raw input
    op.c_cflag &= ~CRTSCTS;
    op.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    op.c_iflag &= ~(IXON | IXOFF | IXANY);

    op.c_cc[VMIN] = 1;
    op.c_cc[VTIME] = 1;

    op.c_oflag &= ~OPOST;

    ret = tcsetattr(r->fd, TCSANOW, &op);
    
    if (ret) {
      printf("ERROR: Error %d from tcsetattr.\n", errno);
      ret = -1;
    }
  } else {
    ret = -1;
  }

  return ret;
}

/**
 * @brief Attempts to reconnect to serial device
 * specified by 'path', 'retries' number of times,
 * waiting 'timeout' milliseconds before re-attempting.
 * 
 * @param r Remote structure to reconnect
 * @param path Path to serial device
 * @param retries Number of times to attempt reconnect
 * @param timeout Time in milliseconds between attempts
 * @return int Returns 0 on success, -1 on failure. Sets
 * R_EXIT flag after failing all retry attempts.
 */
int reconnect(remote_t *r, char *path, int retries, int timeout) {
  int ret = 0;

  strncpy(r->dev_buf, path, 32);

  close(r->fd);
  
  while (retries--) {
    ret = connect(r);
    if (ret == 0) {
      break;
    }

    usleep(timeout * 1000);
  }

  if (!retries && ret) {
    // reconnect failed. signal to exit
    r->state |= (1 << R_EXIT);
    ret = -1;
  } else {
    r->state &= ~(1 << R_EXIT);
  }

  return ret;
}

/**
 * @brief Consistent-Overhead-Byte-Stuffing (COBS)
 * packet encoding routine. Appends single byte
 * to beginning of encode-buffer for the distance
 * to the first zero-byte. Every successive zero
 * is replaced by the distance to the next ...
 * 
 * Does NOT append a packet-delimitting zero-byte.
 * 
 * Blocks of 254 non-zero bytes use group delimiters.
 * 
 * Source -> Wikipedia (c) 2024 https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing
 * 
 * @param data Byte-buffer to encode
 * @param length Length of data buffer in bytes
 * @param buffer Encoded output-buffer
 * @return size_t Number of bytes successfully encoded
 */
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

/**
 * @brief Consistent-Overhead-Byte-Stuffing (COBS)
 * packet decoding routine. Decodes provided encoded 
 * input buffer into output buffer.
 * 
 * Uses zero-byte as a delimiter.
 * 
 * Source -> Wikipedia (c) 2024 https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing
 * 
 * @param in COBS encoded data buffer 
 * @param length Length of input buffer in bytes
 * @param out Decoded data buffer
 * @return size_t Number of bytes sucessfully decoded
 */
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

/**
 * @brief Computes an 8bit checksum over the input
 * buffer. 
 * 
 * @param data Data buffer to compute checksum for
 * @param len Number of bytes present in buffer
 * @return uint8_t Computed 8bit checksum
 */
static uint8_t chk8(uint8_t *data, size_t len) {
  uint8_t *rd, s;

  for (s = 0, rd = data; rd < data + len; rd += 1) {
    s += *rd;
  }

  return (uint8_t)s;
}

/**
 * @brief Packet transmission routine. Outputs
 * next remote-output packet pointed to by 'tx_head'.
 * 
 * Sets data on configured/connected serial terminal.
 * 
 * Advances 'tx_head' on success.
 * 
 * @param r Remote structure/device to transmit to
 * @return int Returns 0 on success, -1 on failure.
 */
int tx_packet(remote_t *r) {
  static uint8_t tx_buffer[PS_DATA + PS_HEADER + 1];
  size_t wr, en;
  uint8_t chk;

  r->tx_head->packet.size += 1;

  chk = chk8(r->tx_head->bytes, PS_HEADER + r->tx_head->packet.size - 1);

  r->tx_head->packet.data[r->tx_head->packet.size - 1] = chk;

  en = cobs_encode(r->tx_head->bytes, r->tx_head->packet.size + PS_HEADER, tx_buffer);

  tx_buffer[en++] = 0;

  wr = write(r->fd, tx_buffer, en);
  if (wr == en) {
    r->tx_head = RNEXT(r->tx_head, r->out);
  }

  return (wr == en) ? 0 : -1;
}

/**
 * @brief Packet reception routine. Receives, 
 * decodes, and buffers new data/packets into
 * remote-structure RX-ring buffer.
 * 
 * Reads from connected serial terminal until
 * zero-byte is received. Data read up until
 * delimiter is decoded into ring-buffer position
 * pointed to by 'rx_head'. 
 * 
 * An 8bit checksum is computed over decoded
 * contents and compared to last byte received.
 * If packet cannot be validated, reception restarts.
 * 
 * Any extra bytes read after the delimiter are
 * moved to the beginning of the next packet
 * receive-buffer pointed to by 'wr_head'.
 * 
 * Upon successful reception, 'rx_head' and 
 * 'wr_head' are advanced. 
 * 
 * R_RXINP is set for incomplete packet receptions
 * R_NDATA is set upon complete packet reception
 * 
 * @param r Remote structure to read from and buffer to
 * @return int Returns 1 on complete RX, 0 otherwise
 */
int rx_packet(remote_t *r) {
  static uint8_t *wr, *buf;
  uint8_t *delim, chk;
  size_t dec;
  int ret, rx_len, ex;

  ret = 0;
  buf = r->wr_head->bytes;

  // check for new read
  if (isbclr(r->state, R_RXINP)) {
    r->state &= ~(1 << R_NDATA);
    wr = buf;
  }

  // read max PS_LENGTH bytes
  rx_len = read(r->fd, wr, PS_LENGTH - (wr - buf) - 1);

  if (rx_len > 0) {
    // receive-in-progress
    r->state |= (1 << R_RXINP);
    
    // search for delimiter in new bytes
    delim = memchr(wr, 0, rx_len);
    if (delim) {
      // decode the contents of the receive buffer into packet
      dec = cobs_decode(buf, delim - buf, r->rx_head->bytes);

      // compute checksum over decoded buffer (excluding appended 1st byte and last chksum byte)
      chk = chk8(r->rx_head->bytes, PS_HEADER + r->rx_head->packet.size - 1);
      if (r->rx_head->bytes[dec - 1] != chk) {
        // broken packet, disregard
        r->state &= ~(1 << R_RXINP);
      } else {
        // wr points to current write position (rx_len offset from buf)
        // if delimiter is found, bytes from next packet may have been loaded into buf
        // len of extra bytes is -> receive len offset - delim - 1
        ex = (wr + rx_len) - delim - 1;
        
        // advance receive and write heads
        r->rx_head = RNEXT(r->rx_head, r->data);
        r->wr_head = RNEXT(r->rx_head, r->data);

        if (ex > 0) {
          // move extra bytes read from next packet to beginning of buffer
          wr = memmove(r->wr_head->bytes, delim + 1, ex);
          rx_len = ex;
        } else {
          r->state &= ~(1 << R_RXINP);
        }

        r->state |= (1 << R_NDATA); 
      }

      ret = 1;
    }

    // advance receive head
    wr += rx_len; 
  }

  return ret;
}

/**
 * @brief Polls connected serial device for
 * data status. If new data is available,
 * 'rx_packet' is invoked to buffer new input.
 * 
 * R_EXIT is set if poll() sets ENOTTY in events
 * 
 * @param r Remote structure to poll for changes
 * @param timeout poll() timeout in milliseconds
 * @return int Returns 1 on new data, 0 on no change, 
 * -1 on connection error (ENOTTY)
 */
int poll_resp(remote_t *r, int timeout) {
  int ready, ret;

  ret = 0;

  ready = poll(r->pfds, 1, timeout * 2);

  if (ready) {    
    if (r->pfds->revents & POLLIN) {
      ret = rx_packet(r);
    } else if (r->pfds->events & ENOTTY) {
      r->state |= (1 << R_EXIT);
      ret = -1;
    }
  }

  return ret;
}
