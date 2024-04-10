#include "remote.h"

int init_remote(remote_t *r, char *path, int speed) {
  int ret = 0;

  strncpy(r->dev_path, path, sizeof(r->dev_path));

  r->rx_head = r->rx.bytes;
  r->load_state = 0;
  r->r_flags = 0;

  r->speed = speed;
  ret = connect(r);

  r->pfds = calloc(1, sizeof(struct pollfd));
  r->pfds->fd = r->fd;
  r->pfds->events = POLLIN | POLLERR | POLLHUP;

  return ret;
}

int connect(remote_t *r) {
  int ret = 0;

  r->fd = open(r->dev_path, O_RDWR | O_NOCTTY, S_IRUSR | S_IWUSR);
  if (r->fd < 0) {
    printf("ERROR: Could not open port '%s'.\n", r->dev_path);
    ret = -1;
  } else {
    struct termios op;
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
  }  

  return ret;
}

int reconnect(remote_t *r, int retries, int timeout) {
  int ret = 0;

  close(r->fd);
  
  while (retries--) {

    if (connect(r) < 0) {
      usleep(timeout * 1000);
    } else {
      ret = poll(r->pfds, 1, timeout);
      if (ret == 0) {
        r->r_flags &= ~(1 << R_NOTTY);
        break;
      }
    }
  }

  if (!retries && isbset(r->r_flags, R_NOTTY)) {
    // reconnect failed. signal to exit
    r->r_flags |= (1 << R_EXIT);
    ret = -1;
  }

  return ret;
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

  return (size_t)(decode - (uint8_t *)out);
}

size_t tx_packet(remote_t *r) {
  static uint8_t tx_buffer[PS_DATA + PS_HEADER];
  size_t wr = 0;

  wr = cobs_encode(&r->tx, r->tx.packet.size + PS_HEADER, tx_buffer);

  tx_buffer[wr++] = 0;

  wr = write(r->fd, tx_buffer, wr);

  return wr;
}

int rx_packet(remote_t *r) {
  static uint8_t rx_buffer[PS_LENGTH];
  uint8_t *delim;
  int ret, rx_len, ex;

  ret = 0;
  
  if (isbclr(r->r_flags, R_RXINP)) {
    r->rx_head = rx_buffer;
  }

  rx_len = read(r->fd, r->rx_head, PS_LENGTH - (r->rx_head - rx_buffer) - 1);

  if (rx_len > 0) {
    r->r_flags |= (1 << R_RXINP);
    // search for delimiter in new bytes
    delim = memchr(r->rx_head, 0, rx_len);
    if (delim) {
      r->r_flags &= ~(1 << R_RXINP);
      // decode the contents of the receive buffer into packet
      cobs_decode(rx_buffer, delim - rx_buffer + 1, r->rx.bytes);

      // rx_head points to current read position (rx_len offset from rx_buffer for prev read)
      // if delimiter is found, bytes from next packet may have been loaded into rx_buffer
      // len of extra bytes is -> length of packet bytes - total length of loaded bytes - delimiter byte
      //                       -> (delim - rx_buffer) - ((r->rx_head + rx_len) - rx_buffer) - 1
      ex = (r->rx_head + rx_len) - delim - 1;
      
      if (ex > 0) {
        // rx still in progress
        r->r_flags |= (1 << R_RXINP);
        // move extra bytes read from next packet to beginning of receive buffer
        r->rx_head = memmove(rx_buffer, delim + 1, ex) + ex;
        rx_len = 0;
      }

      ret = 1;
    }

    // advance receive head
    r->rx_head += rx_len; 
  }

  return ret;
}

int poll_resp(remote_t *r, int timeout) {
  int ready, ret = 0;

  while (ret == 0) {
    ready = poll(r->pfds, 1, timeout * 2);

    if (ready) {
      if (r->pfds->revents & POLLIN) {
        ret = rx_packet(r);
      } else if (r->pfds->events & ENOTTY) {
        r->r_flags |= (1 << R_NOTTY);
        ret = reconnect(r, 10, 1000);
      }
    } else {    // timeout
      ret = -1;
    }
  } 

  return ret;
}

void print_packet(packet_t *p) {
  printf("===== header =====\n");

  printf("PK TYPE         : %u\n", p->packet.type);
  printf("PK FLAGS        : %u\n", p->packet.flags);
  printf("PK SIZE         : %u\n", p->packet.size);
  printf("PK TIMEOUT      : %u\n", p->packet.timeout);

  printf("\t---- data ----\n");
  for (int i = 0; i < p->packet.size; i += 1) {
    printf("PK DATA[%d]     : %u\n", i, p->packet.data[i]);
  }
  printf("\t---- data ----\n");

  printf("===== header =====\n");

  printf("TOTAL BYTES: %d\n", p->packet.size + 3);
}
