#include "pid.h"

double pid_compute(pid_t *p) {
	double err = *p->s - *p->i;
	double prop = p->kp * err;

	p->integral += err;
	double integral = p->ki * p->integral;

	double derivative = p->kd * (err - p->prev_err);
	p->prev_err = err;

	double out = prop + integral + derivative;

	return out;
}

void pid_set_param(pid_t *p, double kp, double ki, double kd) {
	p->kp = kp;
	p->ki = ki;
	p->kd = kd;
}

void pid_set_input(pid_t *p, double *input) {
	p->i = input;
}

void pid_set_output(pid_t *p, double *output) {
	p->o = output;
}

void pid_set_target(pid_t *p, double *target) {
	p->s = target;
}

void pid_set_direction(pid_t *p, int direction) {
	if (p->dir != direction) {
		p->kp *= -1;
		p->ki *= -1;
		p->kd *= -1;
	}
	p->dir = direction;
}