#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "simulator.h"

int preset_cb(menu_t *m, option_t *o);
int pick_preset_cb(menu_t *m, option_t *o);
int alarms_cb(menu_t *m, option_t *o);
int set_param_cb(menu_t *m, option_t *o);
int pick_pid_cb(menu_t *m, option_t *o);
int timers_cb(menu_t *m, option_t *o);
int main_cb(menu_t *m, option_t *o);
int circuit_select_cb(menu_t *m, option_t *o);
int mode_cb(menu_t *m, option_t *o);

void setup_callbacks();

#endif // CALLBACKS_H
