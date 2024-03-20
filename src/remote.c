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
  printf("Disconnected. Attempting reconnect...\n");

  while (retries--) {

    if (connect(r) < 0) {
      usleep(timeout * 1000);
    } else {
      ret = poll(r->pfds, 1, timeout);
      if (ret == 0) {
        printf("Successfully reconnected!\n");
        r->r_flags &= ~(1 << R_NOTTY);
        break;
      }
    }

    printf("Reconnect failed (retry = %d).\n", retries);
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
  static uint8_t tx_buffer[PS_LENGTH + PS_HEADER];
  size_t wr = 0;

  wr = cobs_encode(&r->tx, r->tx.size + PS_HEADER, tx_buffer);

  tx_buffer[wr++] = 0;

  wr = write(r->fd, tx_buffer, wr);

  return wr;
}

int rx_packet(remote_t *r) {
  static uint8_t rx_buffer[PS_LENGTH + PS_HEADER];
  int ret = 0, rd;

  if (isbclr(r->r_flags, R_RXINP)) {
    r->rx_head = rx_buffer;
  } 
  
  if (r->rx_head != rx_buffer) {
    r->r_flags |= (1 << R_RXINP);
  }

  rd = read(r->fd, r->rx_head, PS_LENGTH);

  if (rd > 0) {
    r->rx_head += rd;
  }

  if (*(r->rx_head) == 0) {
    cobs_decode(rx_buffer, r->rx_head - rx_buffer, &r->rx);
    r->rx_head = rx_buffer;
    r->r_flags &= ~(1 << R_RXINP);
    ret = 1;
  }

  return ret;
}

int poll_resp(remote_t *r, int timeout) {
  int ready, ret = 0;

  while (ret == 0) {
    ready = poll(r->pfds, 1, timeout);

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

int ping(remote_t *r, uint8_t ttl, int ntimes) {
  int reply = 0;
  struct timeval itime;

  // ping header
  r->tx.type = PK_STATUS;
  r->tx.flags = (1 << ST_PING);
  r->tx.size = 0;
  r->tx.timeout = ttl;

  for (int i = 0; i < ntimes; i += 1) {
    gettimeofday(&itime, NULL);

    tx_packet(r);

    if (poll_resp(r, -1) > 0) {
      if (r->rx.type == PK_STATUS) {
        if (isbset(r->rx.flags, ST_PING)) {
          printf("\tPing reply: bytes=%d  TTL=%ds\n", PS_HEADER + r->rx.size, ttl);
          reply += 1;
        } else if (isbset(r->rx.flags, ST_TIMEOUT)) {
          printf("\tPing timed out: TTL=%ds\n", ttl);
        }
      }
    } else {
      printf("\tResponse timed out\n");
    }
  }

  return reply;
}

void print_packet(packet_t *p) {
  printf("===== header =====\n");

  printf("PK TYPE         : %u\n", p->type);
  printf("PK FLAGS        : %u\n", p->flags);
  printf("PK SIZE         : %u\n", p->size);
  printf("PK TIMEOUT      : %u\n", p->timeout);

  printf("\t---- data ----\n");
  for (int i = 0; i < p->size; i += 1) {
    printf("PK DATA[%d]     : %u\n", i, p->bytes[i]);
  }
  printf("\t---- data ----\n");

  printf("===== header =====\n");

  printf("TOTAL BYTES: %d\n", p->size + 3);
}
