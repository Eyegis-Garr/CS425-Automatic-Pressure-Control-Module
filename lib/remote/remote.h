#ifndef REMOTE_H
#define REMOTE_H

#include <Stream.h>
#include <inttypes.h>

// packet head/tail bytes
#define SOTX        0xAA
#define EOTX        0x55

// packet sizing
#define PS_LENGTH  255
#define PS_HEADER  4

// packet header byte-indices
#define PH_TYPE     0
#define PH_FLAGS    1
#define PH_SIZE     2
#define PH_TIMEOUT  3

// packet type
#define PK_UPDATE   0  // state update packet (required response)
#define PK_COMMAND  1  // state config packet (specified response)
#define PK_STATUS   2  // comm status packet  (required response)
#define PK_ACK      3  // ACK response packet

// system <-> client
#define ST_TIMEOUT  0  // broken packet (?)
#define ST_PING     1  // if time permits (lol)

// update packet flags
#define UP_SYSTEM   0  // add system updates to update config
#define UP_CIRCUITS 1  // add circuit updates to update config
#define UP_REMOTE   2  // add remote updates to update config
#define UP_REFRESH  3  // system update trigger
#define UP_REMOVE   4  // specified updates are removed from update config

// command packet flags
#define CMD_RESPND  0  // response/ack required
#define CMD_MODESET 1  // configure mode
#define CMD_PARSET  2  // parameter set
#define CMD_SAVE    3  // initiates config save to disk
#define CMD_GETLOG  4  // dumps system log
#define CMD_DMPCFG  5  // dumps loaded config

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
struct packet_s {
  uint8_t type;
  uint8_t flags;
  uint8_t size;
  uint8_t timeout;

  uint8_t data[PS_LENGTH];
};

typedef union packet_t {
  struct packet_s packet;
  uint8_t bytes[PS_HEADER + PS_LENGTH];
} packet_t;

typedef struct remote_t {
  Stream *s;
  
  int load_state;
  uint8_t *rx_head;
  packet_t rx;    // device input packet
  packet_t tx;

  uint8_t ack_timeout;    // response timeout in seconds

  uint8_t r_flags;
} remote_t;

int init_remote(remote_t *r, Stream *s);

int rx_packet(remote_t *r);
size_t tx_packet(packet_t *p, Stream *s);

size_t ack_packet(remote_t *r, packet_t *p);

#endif // REMOTE_H