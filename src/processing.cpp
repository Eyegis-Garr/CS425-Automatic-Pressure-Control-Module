#include "processing.h"

int process_packet(remote_t *r) {
  uint8_t *rd = r->rx.packet.data;
  int ret = 0;

  switch (r->rx.packet.type) {
    case PK_UPDATE:
      if (isbset(r->rx.packet.flags, UP_REMOVE)) {
        // mask UP_REMOVE and bit-clear rest of packet flag-bits (i.e. rest of update types)
        sys.up_types &= ~(r->rx.packet.flags & ~(1 << UP_REMOVE));
      } else {
        sys.up_types = r->rx.packet.flags;   // configures system for updating specified system state
        if (isbset(r->rx.packet.flags, UP_CIRCUITS)) {
          // store circuits to send updates for
          sys.c_flags = *rd++;
          // store parameters to package
          sys.p_flags = (uint16_t) *rd++;
          sys.p_flags <<= 8;
          sys.p_flags |= (uint16_t) *rd++;
        }
      }
      break;
    case PK_COMMAND:
      ret = process_command(r->rx.packet.flags, rd);
      if (isbset(r->rx.packet.flags, CMD_RESPND) && ret) {
        // if response requested and command successfully executed, send ACK
        ret = tx_packet(&r->rx, r->s);
      } else {
        ret = ack_packet(r, &r->rx);
      }
      break;
    case PK_STATUS:
      if (isbset(r->rx.packet.flags, ST_PING)) {
        r->rx.packet.timeout = 0;
        ret = tx_packet(&r->rx, r->s); // echo ping back
      }
      break;
    default:
      ret = -1;
      break;
  }

  if (ret > 0) {
    r->r_flags &= ~(1 << R_NDATA);
  }

  return ret;
}

/**
 * @brief WIP command packet processing
 * 
 * @param flags - command packet flags
 * @param bytes - command packet payload
 * @return size_t - number of bytes processed
 */
size_t process_command(uint8_t flags, uint8_t *bytes) {
  uint8_t *cbytes = bytes;
  uint8_t cmask = 0;
  uint16_t pmask = 0;

  for (int i = 1; i < 8; i += 1) {
    if (isbset(flags, i)) {
      switch (i) {
        case CMD_MODESET:
          sys.s_flags = *cbytes++;
          break;
        case CMD_PARSET:
          cmask = *cbytes++;
          pmask = (uint16_t) *cbytes++;
          pmask <<= 8;
          pmask |= (uint16_t) *cbytes++;
          bytes += packetize_circuits(cbytes, cmask, pmask, 0);
          break;
        case CMD_SAVE:
          // perform save
          break;
        case CMD_GETLOG:
          // retreive and send log
          break;
        case CMD_DMPCFG:
          // retreive and send config
          break;
        default:
          break;
      }
    }
  }

  return (size_t)(cbytes - bytes);
}

/**
 * @brief transmits update packet based on 
 * system update flags.
 * 
 * @param r - system remote device
 * @return int - number of bytes sent
 */
size_t issue_updates(remote_t *r) {
  uint8_t *pdata = r->tx.packet.data;

  if (sys.up_types) {
    r->tx.packet.type = PK_UPDATE;
    r->tx.packet.flags = sys.up_types;
    r->tx.packet.size = 0;
    r->tx.packet.timeout = 0;
    
    if (isbset(sys.up_types, UP_SYSTEM)) {
      pdata += packetize_system(pdata);
    } if (isbset(sys.up_types, UP_CIRCUITS)) {
      pdata += packetize_circuits(pdata, sys.c_flags, sys.p_flags, 1);
    } if (isbset(sys.up_types, UP_REMOTE)) {
      pdata += packetize_remote(pdata);
    }
  }

  r->tx.packet.size = pdata - r->tx.packet.data;

  // send packet over configured remote device
  tx_packet(&r->tx, r->s);

  return pdata - r->tx.packet.data;
}

/**
 * @brief loads system-related state into packet buffer
 * 
 * @param bytes - destination packet buffer
 * @return size_t - bytes packed
 */
size_t packetize_system(uint8_t *bytes) {
  uint8_t *sbytes = bytes;

  // store system flags
  *sbytes++ = sys.s_flags;
  *sbytes++ = sys.c_flags;
  *sbytes++ = sys.p_flags >> 8;
  *sbytes++ = sys.p_flags;
  *sbytes++ = sys.en_flags;
  // store last recieved client update flags
  *sbytes++ = sys.up_types;
  // store system uptime
  *sbytes++ = sys.uptime >> 24;
  *sbytes++ = sys.uptime >> 16;
  *sbytes++ = sys.uptime >> 8;
  *sbytes++ = sys.uptime; 

  return sbytes - bytes;
}

/**
 * @brief circuit packeting & depacketing.
 * -=== circuit packet body structure ===-
 * PK_BYTE[0]   ->  circuit mask
 * PK_BYTE[1]   ->  circuit parameter mask (high byte)
 * PK_BYTE[2]   ->  circuit parameter mask (low byte)
 * ...          ->  circuit parameter data
 * PK_BYTE[N]   ->  circuit IO data (outgoing packets only)
 * 
 * maximum packet size (all circuits and all parameters)
 *    C_NUM_CIRCUITS * ((2 * C_NUM_PARAM) + 1)  +  3     bytes
 *    |----------- parameter data -----------| |-masks-|
 * 
 * @param bytes - source or destination packet-buffer
 * @param cmask - masks affect circuits
 * @param pmask - masks affect parameters
 * @param dir - 1 -> loads packet with circuit data
 *              0 -> loads circuits with packet data
 * @return size_t - bytes packed
 */
size_t packetize_circuits(uint8_t *bytes, uint8_t cmask, uint16_t pmask, uint8_t dir) {
  uint8_t *pdata = bytes;
  double *cdata;
  circuit_t *c;

  if (dir) {
    *pdata++ = cmask;
    *pdata++ = pmask >> 8;
    *pdata++ = pmask;
  }

  for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {
    if (isbset(cmask, i)) {
      c = &sys.circuits[i];
      cdata = c->params;
      for (int k = 0; k < C_NUM_PARAM; k += 1) {
        if (isbset(pmask, k)) {
          if (dir) {
            *pdata++ = (uint8_t) c->params[k];
            *pdata++ = (uint8_t) ((c->params[k] - (uint8_t)c->params[k]) * 100);
          } else {
            *cdata++ = *pdata + ((double)(*(pdata + 1)) / 100);
            pdata += 2;
          }
        }
      }

      if (dir) {
        // set circuit binary-state (solenoid io, en button, en LED)
        *pdata++ = (digitalRead(c->pins[I_PRESSURE_IN]) << I_PRESSURE_IN)    |
                   (digitalRead(c->pins[I_PRESSURE_OUT]) << I_PRESSURE_OUT);
      }
    }
  }
  
  return pdata - bytes;
}

size_t packetize_remote(uint8_t *bytes) {
  uint8_t *rbytes = bytes + 1;

  *rbytes++ = sys.remote.r_flags;

  *rbytes++ = sys.remote.rx.packet.type;
  *rbytes++ = sys.remote.rx.packet.flags;
  *rbytes++ = sys.remote.rx.packet.size;

  *rbytes++ = sys.remote.tx.packet.type;
  *rbytes++ = sys.remote.tx.packet.flags;
  *rbytes++ = sys.remote.tx.packet.size;

  bytes[0] = rbytes - bytes;

  return rbytes - bytes;
}