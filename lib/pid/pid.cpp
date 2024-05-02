#include "pid.h"

float pid_compute(pid_t *p) {
	float err = *p->s - *p->i;
	float prop = *p->kp * err;

	p->integral += err;
	float integral = *p->ki * p->integral;

	float derivative = *p->kd * (err - p->prev_err);
	p->prev_err = err;

	float out = prop + integral + derivative;

	return out;
}

void pid_set_param(pid_t *p, float *kp, float *ki, float *kd) {
	p->kp = kp;
	p->ki = ki;
	p->kd = kd;
}

void pid_set_input(pid_t *p, float *input) {
	p->i = input;
}

void pid_set_output(pid_t *p, float *output) {
	p->o = output;
}

void pid_set_target(pid_t *p, float *target) {
	p->s = target;
}

void pid_set_direction(pid_t *p, int direction) {
	if (p->dir != direction) {
		*p->kp *= -1;
		*p->ki *= -1;
		*p->kd *= -1;
	}
	p->dir = direction;
}