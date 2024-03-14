#ifndef PROCESSING_H
#define PROCESSING_H

#include "remote.h"
#include "simulator_defines.h"
#include "simulator.h"

int process_packet(remote_t *r);
size_t process_command(uint8_t flags, uint8_t *bytes);
size_t issue_updates(remote_t *r);
size_t packetize_system(uint8_t *bytes);
size_t packetize_circuits(uint8_t *bytes, uint8_t cmask, uint16_t pmask, uint8_t dir);
size_t packetize_remote(uint8_t *bytes);

#endif // PROCESSING_H