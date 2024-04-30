/**
 * @file main.c
 * @author Bradley Sullivan (bradleysullivan@nevada.unr.edu)
 * @brief Main program entry point
 * @version 0.1
 * @date 2024-04-24
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <ctype.h>
#include <error.h>
#include <poll.h>
#include <pthread.h>

#include "remote.h"
#include "client.h"

#define isbset(d, b) (((d) & (1 << (b))) != 0)
#define isbclr(d, b) (((d) & (1 << (b))) == 0)

int main(int argc, char *argv[]) {
  client_t *c;

  if (argc < 2) {
    printf("\nInvalid usage.\n\n\tPlease provide the device path to the tranceiver.\n");
    printf("\teg. %s /dev/ttyS420\n", argv[0]);
    return 1;
  }

  c = c_get(NULL, argv[1]);
  if (!c) {
    printf("Failed to initialize client.\n\nExiting...\n");
    c->state |= (1 << S_EXIT);
    if (c->scr) {
      endwin();
    }

    return 1;
  }

  sleep(1);

  while (isbclr(c->state, S_EXIT)) {

    draw_circuits(c);
    draw_system(c);

    if (isbclr(c->remote->state, R_EXIT)) {
      if (poll_resp(c->remote, c->poll_time) < 0) {
        c->err = E_NOTTY;
        print_view(c, "Serial device unavailable...\n");
      } else if (isbset(c->remote->state, R_FLUSH)) {
        if (tx_packet(c->remote) == 0) {
          c->remote->state &= ~(1 << R_FLUSH);
        }
      }
    }

    update_client(c);

  }

  endwin();
  
  return 0;
}


