#ifndef REMOTE_H
#define REMOTE_H

#include <Arduino.h>
#include <inttypes.h>

#define SOTX        0xAA
#define EOTX        0x55

// packet type
#define PK_UPDATE   1         // state update packet
#define PK_COMMAND  (1 << 1)  // state config packet

// system -> client
#define UP_SYSTEM   1         // full system update
#define UP_CIRCUITS  (1 << 1)  // individual circuit update
#define UP_IFACE    (1 << 2)  // UI state update
#define UP_UPTIME   (1 << 3)  // system runtime update
#define UP_LASTSAVE (1 << 4)  // time since last save

// client -> system
#define CMD_MODESET 1         // config mode
#define CMD_SAVE    (1 << 1)  // config save
#define CMD_UPCYCLE (1 << 2)  // config update cycle
#define CMD_UPTYPE  (1 << 3)  // config update types
#define CMD_CSEL    (1 << 4)  // circuit select
#define CMD_PSET    (1 << 5)  // parameter set
#define CMD_TSYNC   (1 << 6)  // synchronizes system time/date

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

  uint8_t *bytes;
} packet_t;

static inline void wrblk(HardwareSerial *s) { while (!s->availableForWrite()) { } }
size_t tx_packet(packet_t *p, HardwareSerial *s);
size_t rx_packet(HardwareSerial *s, packet_t *p);

#endif // REMOTE_H