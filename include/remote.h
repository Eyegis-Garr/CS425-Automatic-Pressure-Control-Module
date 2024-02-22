#ifndef REMOTE_H
#define REMOTE_H

#include <Arduino.h>
#include <inttypes.h>

#define SOTX        0xAA
#define EOTX        0x55

// packet type
#define PK_UPDATE   0  // state update packet
#define PK_COMMAND  1  // state config packet

// system <-> client
#define UP_SYSTEM   0  // full system update
#define UP_CIRCUITS 1  // circuit update
#define UP_REFRESH  3  // system update trigger

// client -> system
#define CMD_MODESET 0  // config mode
#define CMD_PSET    1  // parameter set

// UCSRA FLAGS
#define RX_COMPLETE 7
#define TX_COMPLETE 6
#define TX_EMPTY    5
#define FRAME_ERROR 4
#define DATA_OVRUN  3
#define PARITY_ERR  2
#define DOUBLE_TX   1
#define MULTI_MODE  0

// UCSRB FLAGS
#define RXC_IENABLE 7
#define TXC_IENABLE 6
#define RXE_IENABLE 5
#define RX_ENABLE   4
#define TX_ENABLE   3
#define CH_SZ2      2
#define RX_DATABIT  1
#define TX_DATABIT  0

// UCSRC FLAGS
#define ASYNC       (0 << 7)
#define SYNC        (1 << 7)
#define MASTER_SPI  (3 << 7)
#define NO_PARITY   (0 << 5)
#define EVEN_PARITY (2 << 5)
#define ODD_PARITY  (3 << 5)
#define STOP_BIT    3
#define CH_SZ1      2
#define CH_SZ0      1

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

typedef struct usart_t {
  volatile uint8_t *udr;
  volatile uint8_t *ucsra;
  volatile uint8_t *ucsrb;
  volatile uint8_t *ucsrc;
  volatile uint8_t *ubrrl;
  volatile uint8_t *ubrrh;
} usart_t;

static inline void wrblk(volatile uint8_t *ctrl) { while (!(*ctrl & (1 << TX_EMPTY))) { } }
static inline void rdblk(volatile uint8_t *ctrl) { while (!(*ctrl & (1 << RX_COMPLETE))) { } }
size_t tx_packet(packet_t *p, usart_t *u);
size_t rx_packet(usart_t *u, packet_t *p);

void usart_init(usart_t *u, uint32_t baud);
void usart_enable_rx_interrupt(usart_t *u);

#endif // REMOTE_H