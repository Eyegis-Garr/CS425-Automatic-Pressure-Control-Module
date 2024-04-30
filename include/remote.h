/**
 * @file remote.h
 * @author Bradley Sullivan (bradleysullivan@nevada.unr.edu)
 * @brief Serial remote communication module
 * @version 0.1
 * @date 2024-04-24
 * 
 * @copyright Copyright (c) 2024
 * 
 */
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

#define PS_DATA    255    /// Packet max. data-payload length
#define PS_HEADER  4      /// Packet header byte-length
#define PS_LENGTH  (PS_DATA + PS_HEADER)  /// Max. total packet length
#define RX_LENGTH  128    /// RX ring-buffer packet max
#define TX_LENGTH  128    /// TX ring-buffer packet max
// packet type
#define PK_UPDATE   0     /// state update packet (required response)
#define PK_COMMAND  1     /// state config packet (specified response)
#define PK_STATUS   2     /// comm status packet  (required response)

// system <-> client
#define ST_PING     0     /// ping status packet
#define ST_ACK      1     /// packet ACK

// system <-> client
#define UP_SYSTEM   0     /// add system updates to update config
#define UP_CIRCUITS 1     /// add circuit updates to update config
#define UP_RESET    2     /// clears system update flags

// client -> system
#define CMD_RESPND  0     /// response/ack required
#define CMD_MODESET 1     /// configure mode
#define CMD_PARSET  2     /// parameter set
#define CMD_SAVE    3     /// initiates config save to disk
#define CMD_GETLOG  4     /// dumps system log
#define CMD_DMPCFG  5     /// dumps loaded config

// remote flags
#define R_RXINP 0         /// receive in progress
#define R_NDATA 1         /// new data to process
#define R_FLUSH 2         /// data is ready to transmit
#define R_NOTTY 3         /// terminal is disconnected
#define R_EXIT  4         /// signal to exit

#define isbset(d, b) (((d) & (1 << (b))) != 0)
#define isbclr(d, b) (((d) & (1 << (b))) == 0)
#define MOD(a,b) (((a)%(b))<0?((a)%(b))+(b):((a)%(b)))
#define RNEXT(a,b) MOD((a)-(b)+1,RX_LENGTH)+(b)
#define RPREV(a,b) MOD((a)-(b)-1,RX_LENGTH)+(b)
#define RLEFT(a,b) ((a)-(b))

/**
 * @brief Basic packet structure. Contains
 * a 4-byte header with a 255-byte max. payload
 * 
 */
struct packet_s {
  /// @brief Packet type code. Takes value of (PK_UPDATE, PK_COMMAND, or PK_STATUS)
  uint8_t type;

  /// @brief Packet sub-type flags. Uses bit indices corresponding to specific packet type (UP_CIRCUITS, CMD_PARSET, ST_PING, etc.)
  uint8_t flags;

  /// @brief Length of packet data payload in bytes
  uint8_t size;

  /// @brief Timeout interval in seconds
  uint8_t timeout;

  /// @brief Packet data payload buffer
  uint8_t data[PS_DATA];
};

/**
 * @brief Packet union for directly accessing individual 
 * header fields or for using entire packet as a byte buffer
 * 
 */
typedef union packet_t {
  struct packet_s packet;
  uint8_t bytes[PS_LENGTH];
} packet_t;

/**
 * @brief Serial device datastructure. Stores
 * state regarding serial device, RX/TX packet data,
 * and communication state.
 * 
 */
typedef struct remote_t {
  /// @brief Serial device path
  char dev_buf[32];

  /// @brief File descriptor for current serial terminal
  int fd;

  /// @brief Terminal baud rate as defined in 'termios.h' (i.e. B9600, B19200, etc.)
  int speed;

  /// @brief poll() datastructure for storing terminal state-changes
  struct pollfd *pfds;

  /// @brief RX ring-buffer
  packet_t *rx_head;
  packet_t *wr_head;
  packet_t *rd_tail;
  packet_t data[RX_LENGTH];    // ring buffer

  /// @brief TX ring-buffer
  packet_t *tx_head;
  packet_t out[TX_LENGTH];
  packet_t rx;
  packet_t tx;

  /// @brief Remote state flags. Uses bit-indices R_EXIT, R_NDATA, R_RXINP, R_...
  uint8_t state;
} remote_t;

remote_t *r_get(remote_t *r, char *path, int speed);
void r_free(remote_t *r);
int connect(remote_t *r);
int reconnect(remote_t *r, char *path, int retries, int timeout);
packet_t *r_read(remote_t *r);
int tx_packet(remote_t *r);
int rx_packet(remote_t *r);
int poll_resp(remote_t *r, int timeout);

#endif // REMOTE_H