#ifndef PID_H
#define PID_H

#define DIRECT  0
#define REVERSE 1

typedef struct pid_t {
  double *i;
  double *o;
  double *s;

  double kp;
  double ki;
  double kd;

  double prev_err;
  double integral;
  double deriv;

  int dir;
} pid_t;

double pid_compute(pid_t *p);
void pid_set_param(pid_t *p, double kp, double ki, double kd);
void pid_set_input(pid_t *p, double *input);
void pid_set_output(pid_t *p, double *output);
void pid_set_target(pid_t *p, double *target);
void pid_set_direction(pid_t *p, int dir);

#endif // PID_H