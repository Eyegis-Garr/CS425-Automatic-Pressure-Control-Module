#include "remote.h"

int init_remote(remote_t *r, char *path, int speed) {
  strncpy(r->dev_path, path, sizeof(r->dev_path));
  r->rx_head = r->rx.bytes;
  r->load_state = 0;
  r->r_flags = 0;

  r->fd = open(path, O_RDWR | O_NOCTTY | O_NDELAY);
  if (r->fd < 0) {
    printf("ERR: Could not open port '%s'.\n", path);
    return 1;
  }

  struct termios op;
  tcgetattr(r->fd, &op);

  cfsetispeed(&op, speed);
  cfsetospeed(&op, speed);

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
  op.c_cc[VTIME] = 0;

  op.c_oflag &= ~OPOST;
  
  if (tcsetattr(r->fd, TCSANOW, &op)) {
    printf("ERR: Error %d from tcsetattr.", errno);
    return 1;
  }

  r->pfds = calloc(1, sizeof(struct pollfd));
  r->pfds->fd = r->fd;
  r->pfds->events = POLLIN;

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

  return (size_t)(decode - (uint8_t *)out);
}

static size_t wrblk(int fd, uint8_t *data, int len) {
  size_t t;
  while ((t = write(fd, data, len)) == -1) { }
  return t;
}

size_t tx_packet(packet_t *p, int fd) {
  static uint8_t tx_buffer[PS_LENGTH + PS_HEADER];
  size_t wr = 0;

  wr = cobs_encode((uint8_t *)p, p->size + PS_HEADER + 1, (uint8_t *)&tx_buffer);

  tx_buffer[wr++] = 0;

  wr = wrblk(fd, (uint8_t *)&tx_buffer, wr);

  return wr;
}

int load_packet(remote_t *r) {
  static uint8_t rx_buffer[PS_LENGTH + PS_HEADER];
  int ret = 0;

  if (isbclr(r->r_flags, R_RXINP)) {
    r->rx_head = rx_buffer;
  }

  if (r->pfds->revents & POLLIN) {
    r->rx_head += read(r->fd, r->rx_head, PS_LENGTH);
    r->r_flags |= (1 << R_RXINP);
    if (*(r->rx_head) == 0) {
      size_t len = cobs_decode(rx_buffer, r->rx_head - rx_buffer + 1, &r->rx);
      r->rx_head = rx_buffer;
      r->r_flags &= ~(1 << R_RXINP);
      ret = 1;
    }
  }

  return ret;
}

int block_resp(remote_t *r, int timeout) {
  int ready, ret = 0;

  while (ret == 0) {
    ready = poll(r->pfds, 1, timeout);

    if (ready > 0) {
      if (r->pfds->revents & POLLIN) {
        ret = load_packet(r);         // load packet until complete
        // printf("POLLIN\n");
      }
    } else if (ready == 0) {
      r->r_flags &= ~(1 << R_RXINP);
      // printf("POLLERR\n");
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
  r->tx.size = 1;

  // ping time-to-live in seconds
  r->tx.bytes[0] = ttl;

  for (int i = 0; i < ntimes; i += 1) {
    sleep(1);

    gettimeofday(&itime, NULL);

    tx_packet(&r->tx, r->fd);

    if (block_resp(r, ttl * 1000)) {
      if (r->rx.type == PK_STATUS) {
        if (isbset(r->rx.flags, ST_PING)) {
          printf("\tPing reply: bytes=%d time=%ums TTL=%ds\n", PS_HEADER + r->rx.size, cpu_time_dt(&itime), ttl);
          reply += 1;
        } else if (isbset(r->rx.flags, ST_TIMEOUT)) {
          printf("\tPing timed out: time=%ums TTL=%ds\n", cpu_time_dt(&itime), ttl);
        }
      }
    } else {
      printf("\tResponse timed out: time=%ums TTL=%ds\n", cpu_time_dt(&itime), ttl);
    }
  }

  return reply;
}

uint32_t cpu_time_dt(struct timeval *itime) {
  struct timeval etime;

  gettimeofday(&etime, NULL);

  return  (etime.tv_sec - itime->tv_sec) * 1000;
}
