#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <ctype.h>
#include <argp.h>
#include <error.h>
#include <poll.h>

#include "remote.h"
#include "client.h"

#define isbset(d, b) (((d) & (1 << (b))) != 0)
#define isbclr(d, b) (((d) & (1 << (b))) == 0)

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("\nInvalid usage.\n\n\tPlease provide the device path to the tranceiver.\n");
    printf("\teg. %s /dev/ttyS420\n", argv[0]);
    return 1;
  }

  client_t client;

  if (init_client(&client, argv[1]) < 0) {
    printf("Failed to initialize client.\n\nExiting...\n");
    client.s_flags |= (1 << S_EXIT);
  }

  sleep(1);

  while (isbclr(client.s_flags, S_EXIT)) {

    update_client(&client);

    draw_input(&client);
    draw_circuits(&client);

  }

  endwin();
  
  return 0;
}

