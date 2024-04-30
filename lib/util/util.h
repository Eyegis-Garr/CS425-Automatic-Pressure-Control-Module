#ifndef UTIL_H
#define UTIL_H

#include <inttypes.h>

int lerp(int x, int x0, int y0, int x1, int y1);
int mod(int a, int m);
int in_range(int a, int b, int v);
int clamp(int a, int b, int v);
float clampf(float a, float b, float v);
float mapf(float v, float vmin, float vmax, float tmin, float tmax);

#endif // UTIL_H