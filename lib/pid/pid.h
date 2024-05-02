#ifndef PID_H
#define PID_H

#define DIRECT  0
#define REVERSE 1

typedef struct pid_t {
  float *i;
  float *o;
  float *s;

  float *kp;
  float *ki;
  float *kd;

  float prev_err;
  float integral;
  float deriv;

  int dir;
} pid_t;

float pid_compute(pid_t *p);
void pid_set_param(pid_t *p, float *kp, float *ki, float *kd);
void pid_set_input(pid_t *p, float *input);
void pid_set_output(pid_t *p, float *output);
void pid_set_target(pid_t *p, float *target);
void pid_set_direction(pid_t *p, int dir);

#endif // PID_H