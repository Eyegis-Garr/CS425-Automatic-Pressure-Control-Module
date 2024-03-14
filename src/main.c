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

void *remote_listen(void *r);
void *client_listen(void *c);
int ping_test(remote_t *r, uint8_t npings);

static pthread_spinlock_t spin;

#define DEBUG 
int main(int argc, char *argv[]) {
  remote_t sys;
  client_t client;

  client.r = &sys;

  if (argc < 2) {
    printf("please provide path to transceiver as argument\n");
    exit(1);
  }

  if (init_remote(&sys, argv[1], B9600) < 0) {
    printf("Error: Failed to initialize remote structrue.\n");
    exit(1);
  }

  sleep(2);

  if (!ping_test(&sys, 10)) {
    printf("Error: Unstable connection to system. Exiting...\n");
    // exit(1);
  }

#ifndef DEBUG
  pthread_t rthread;
  pthread_t cthread;

  pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);

  if (pthread_create(&rthread, NULL, remote_listen, (void *)&sys)) {
    printf("error creating remote listening thread.\n");
    exit(1);
  } 
  
  if (pthread_create(&cthread, NULL, client_listen, (void *)&client)) {
    printf("error creating client input thread.\n");
  }

  pthread_join(rthread, NULL);
  pthread_join(cthread, NULL);

  pthread_spin_destroy(&spin);
#endif
#ifdef DEBUG

  uint8_t types = (1 << UP_CIRCUITS) | (1 << UP_SYSTEM) | (1 << UP_REFRESH);
  uint8_t cmask = (1 << P_PRESSURE);
  uint16_t pmask = (1 << C_NUM_PARAM) - 1;

  packet_args cmd = {
    .op_flags = (1 << CMD_PARSET),
    .op_type = PK_COMMAND,
    .cmask = cmask,
    .pmask = pmask,
    .next_val = 1
  };

  packet_args update = {
    .op_flags =(1 << UP_CIRCUITS) | (1 << UP_SYSTEM) | (1 << UP_REFRESH),
    .op_type = PK_UPDATE,
    .cmask = (1 << C_NUM_CIRCUITS) - 1,
    .pmask = (1 << P_PRESSURE) | (1 << P_SET_POINT),    
  };

  sys.tx.size = construct_update(&sys.tx, &update, 1);

  print_packet(&sys.tx);

  sleep(2);

  int ct = 0;
  tx_packet(&sys);
  while (1) {
    if (poll_resp(&sys, 1000) > 0) {
      print_packet(&sys.rx);
      ct += 1;
      printf("ct : %d\n", ct);
    } else {
      if (isbset(sys.r_flags, R_NOTTY)) {
        if (isbset(sys.r_flags, R_EXIT)) {
          break;
        }
      }
    }

    tx_packet(&sys);
  }

#endif
  
  return 0;
}

void *remote_listen(void *r) {
  remote_t *sys = (remote_t *)r;

  while (isbclr(sys->r_flags, R_EXIT)) {
    // check for new TX data from client
    if (isbset(sys->r_flags, R_FLUSH)) {
      if (isbclr(sys->r_flags, R_RXINP)) {
        // send new data
        tx_packet(sys);
      }
    }

    if (poll_resp(sys, 1000) > 0) {
      // new data received
      sys->r_flags |= (1 << R_NDATA);
    } else {
      // error or timeout
      if (isbset(sys->r_flags, R_NOTTY)) {
        if (isbset(sys->r_flags, R_EXIT)) {
          // reconnect failed, prompt for retry
        }
      }
    }
  }

  printf("Exiting...\n");

  return r;
}

void *client_listen(void *c) {
  client_t *client = (client_t *)c;

  init_client(client);

  mvwaddch(client->scr, LINES - 1, 0, '>');

  int key, exit = 0;
  while (!exit) {

    // check for any remote errors

    // get key press
    key = wgetch(client->scr);

    // ESC -> exit
    if (key == 27) break;

    // update textbox, returns 1 on enter
    if (process_input(client, key)) {
      // block until RX finishes
      while (isbset(client->r->r_flags, R_RXINP)) { }

      // process command, returns cmd packet size if valid
      if (process_command(client->input, &client->r->tx) > 0) {
        // lock to set packet flush flag
        pthread_spin_lock(&spin);
        client->r->r_flags |= (1 << R_FLUSH);
        pthread_spin_unlock(&spin);
      }
    }
    
    // if remote thread has new data
    if (isbset(client->r->r_flags, R_NDATA)) {
      // update circuits with new data
      if (process_packet(client, &client->r->rx) > 0) {
        // lock to clear new data flag
        pthread_spin_lock(&spin);
        client->r->r_flags &= ~(1 << R_NDATA);
        pthread_spin_unlock(&spin);
      }
    }

    // update UI
    draw_circuits(client);
    draw_simulator(client);
    draw_packet(client);

    // update screen with new input
    wmove(client->scr, LINES - 1, 1); clrtoeol();
    mvwprintw(client->scr, LINES - 1, 1, client->input);

    refresh();
  }

  pthread_spin_lock(&spin);
  client->r->r_flags |= (1 << R_EXIT);
  pthread_spin_unlock(&spin);

	endwin();

  return c;
}

int ping_test(remote_t *r, uint8_t npings) {
  printf("This test should ping the remote client with 5 packets.\n");
  printf("A passing test should receive 5 ping replies within timeout intervals\n");

  int replies = ping(r, 1, npings);

  printf("Received replies: %d\n", replies);

  return ((double)replies / npings) > 0.80;
}

