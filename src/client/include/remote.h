#ifndef REMOTE_H
#define REMOTE_H

#include <stdio.h>
#include <inttypes.h>
#include <poll.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/termios.h>
#include <sys/time.h>

// packet head/tail bytes
#define SOTX        0xAA
#define EOTX        0x55

#define PS_LENGTH  255
#define PS_HEADER  3
// packet type
#define PK_UPDATE   0  // state update packet (required response)
#define PK_COMMAND  1  // state config packet (specified response)
#define PK_STATUS   2  // comm status packet  (required response)
#define PK_ACK      3  // ACK response packet

// system <-> client
#define ST_TIMEOUT  0  // broken packet (?)
#define ST_PING     1  // if time permits (lol)

// system <-> client
#define UP_SYSTEM   0  // add system updates to update config
#define UP_CIRCUITS 1  // add circuit updates to update config
#define UP_REMOTE   2  // add remote updates to update config
#define UP_REFRESH  3  // system update trigger
#define UP_REMOVE   4  // updates specified are removed from update config

// client -> system
#define CMD_RESPND  0  // response/ack required
#define CMD_MODESET 1  // configure mode
#define CMD_PSET    2  // parameter set
#define CMD_SAVE    3  // initiates config save to disk

// remote flags
#define R_RXINP 0   // receive in progress
#define R_NDATA 1   // new data to process
#define R_TIME  2   // timeout counter active

#define isbset(d, b) (((d) & (1 << (b))) != 0)
#define isbclr(d, b) (((d) & (1 << (b))) == 0)

typedef struct packet_t {
  uint8_t type;
  uint8_t flags;
  uint8_t size;

  uint8_t bytes[PS_LENGTH];
} packet_t;

typedef struct remote_t {
  char dev_path[32];
  int fd;

  struct pollfd *pfds;

  int load_state;
  uint8_t *rx_head;
  packet_t rx;
  packet_t tx;

  uint8_t r_flags;
  uint8_t e_flags;
} remote_t;

int init_remote(remote_t *r, char *path, int speed);

size_t rx_packet(int fd, packet_t *p);
size_t tx_packet(packet_t *p, int fd);

int load_packet(remote_t *r);
int block_resp(remote_t *r, int timeout);
int ping(remote_t *r, uint8_t ttl, int ntimes);

uint32_t cpu_time_dt(struct timeval *itime);

#endif // REMOTE_H