#ifndef REMOTE_H
#define REMOTE_H

#include <Stream.h>
#include <inttypes.h>

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
// #define UP_PACKETS 4 // if time permits

// client -> system
#define CMD_RESPND  0  // response/ack required
#define CMD_MODESET 1  // configure mode
#define CMD_PSET    2  // parameter set
#define CMD_SAVE    3  // initiates config save to disk

#define R_RXINP 0   // receive in progress
#define R_NDATA 1   // new data to process
#define R_TIME  2   // timeout counter active

#define isbset(d, b) (((d) & (1 << (b))) != 0)
#define isbclr(d, b) (((d) & (1 << (b))) == 0)

/**
 * @brief simple packet model
 * 
 * @type: broad packet scope. uses PK_<TYPE> define
 * 
 * @flags: per-type packet attributes/subtype
 * 
 * @size: packet body size in bytes
 * 
 * @bytes: packet payload buffer
 * 
 */
typedef struct packet_t {
  uint8_t type;
  uint8_t flags;
  uint8_t size;

  uint8_t bytes[PS_LENGTH];
} packet_t;

typedef struct remote_t {
  Stream *s;
  
  int load_state;
  uint8_t *pkt_head;
  packet_t pkt;    // device input packets

  uint8_t ack_timeout;    // response timeout in seconds

  uint8_t r_flags;
} remote_t;

int init_remote(remote_t *r, Stream *s);

int load_packet(remote_t *r);
size_t tx_packet(packet_t *p, Stream *s);
size_t rx_packet(Stream *s, packet_t *p);

size_t ack_packet(remote_t *r, packet_t *p);
size_t timeout(remote_t *r);

#endif // REMOTE_H