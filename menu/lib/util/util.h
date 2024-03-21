#ifndef UTIL_H
#define UTIL_H

#include <inttypes.h>

void msb_radixsort_index(uint16_t *data, uint16_t *idx, int zbin, int obin, uint16_t mask);
void msb_radixsort(uint16_t *data, int zbin, int obin, uint16_t mask);
void insertion_sort_index(int *data, int *idx, uint16_t len);
void insertion_sort(int *data, uint16_t len);

int lerp(int x, int x0, int y0, int x1, int y1);
int mod(int a, int m);
int in_range(int a, int b, int v);
int clamp(int a, int b, int v);
float clampf(float a, float b, float v);

#endif // UTIL_H