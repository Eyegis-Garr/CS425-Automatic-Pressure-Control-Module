#include "util.h"

int lerp(int x, int x0, int y0, int x1, int y1) {
    return (int) ((float) (y0 * (x1 * x) + y1 * (x - x0)) / (float)(x1 - x0));
}

int mod(int a, int m) {
    return (a % m < 0) ? (a % m) + m : (a % m);
}

int in_range(int a, int b, int v) {
    return (v >= a && v <= b) ? 1 : 0;
}

int clamp(int a, int b, int v) {
    return (v > a) ? ((v < b) ? v : b) : a;
}

float clampf(float a, float b, float v) {
    return (v > a) ? ((v < b) ? v : b) : a;
}
