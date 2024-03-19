#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curses.h>
#include <ctype.h>
#include <argp.h>
#include <error.h>
#include <poll.h>

#include "remote.h"
#include "client.h"

#define isbset(d, b) (((d) & (1 << (b))) != 0)
#define isbclr(d, b) (((d) & (1 << (b))) == 0)

int ping_test(remote_t *r, uint8_t npings);

#define DEBUG 
int main(int argc, char *argv[]) {
  client_t client;
  packet_t p;
  char input[256];

  while (1) {
    fgets(input, 256, stdin);

    printf("\tinput: %s\n", input);

    if (process_input(&client, input) == 0) {
      // construct packet
    }
  }

  // uint8_t types = (1 << UP_CIRCUITS) | (1 << UP_SYSTEM) | (1 << UP_REFRESH);
  // uint8_t cmask = (1 << P_PRESSURE);
  // uint16_t pmask = (1 << C_NUM_PARAM) - 1;

  // packet_args cmd = {
  //   .op_flags = (1 << CMD_PARSET),
  //   .op_type = PK_COMMAND,
  //   .cmask = cmask,
  //   .pmask = pmask,
  //   .next_val = 1
  // };

  // packet_args update = {
  //   .op_flags =(1 << UP_CIRCUITS) | (1 << UP_SYSTEM) | (1 << UP_REFRESH),
  //   .op_type = PK_UPDATE,
  //   .cmask = (1 << C_NUM_CIRCUITS) - 1,
  //   .pmask = (1 << P_PRESSURE) | (1 << P_SET_POINT),    
  // };

  // sys.tx.size = construct_update(&sys.tx, &update, 1);

  // print_packet(&sys.tx);

  // sleep(2);

  // int ct = 0;
  // tx_packet(&sys);
  // while (1) {
  //   if (poll_resp(&sys, 1000) > 0) {
  //     print_packet(&sys.rx);
  //     ct += 1;
  //     printf("ct : %d\n", ct);
  //   } else {
  //     if (isbset(sys.r_flags, R_NOTTY)) {
  //       if (isbset(sys.r_flags, R_EXIT)) {
  //         break;
  //       }
  //     }
  //   }

  //   tx_packet(&sys);
  // }
  
  return 0;
}

int ping_test(remote_t *r, uint8_t npings) {
  printf("This test should ping the remote client with 5 packets.\n");
  printf("A passing test should receive 5 ping replies within timeout intervals\n");

  int replies = ping(r, 1, npings);

  printf("Received replies: %d\n", replies);

  return ((double)replies / npings) > 0.80;
}

