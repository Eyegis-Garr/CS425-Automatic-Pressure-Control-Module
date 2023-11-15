#include "util.h"

void msb_radixsort_index(uint16_t *data, uint16_t *idx, int zbin, int obin, uint16_t mask) {
    static uint16_t sw = 0;

    if (mask == 0 || zbin >= obin) return;

    uint16_t zh = zbin, oh = obin;

    while (zh <= oh) {
        if (data[idx[zh]] & mask) {
            // sw = data[zh]; data[zh] = data[oh]; data[oh] = sw;
            sw = idx[zh]; idx[zh] = idx[oh]; idx[oh] = sw;
            oh -= 1;
        } else {
            zh += 1;
        }
    }

    mask >>= 1;

    msb_radixsort_index(data, idx, zbin, oh, mask);     // recursively sorts zero bin
    msb_radixsort_index(data, idx, zh, obin, mask);     // recursively sorts one bin
}
// sorts data array in-place
void msb_radixsort(uint16_t *data, int zbin, int obin, uint16_t mask) {
    static uint16_t sw = 0;

    if (mask == 0 || zbin >= obin) return;

    int zh = zbin, oh = obin;

    do {
        if (data[zh] & mask) {
            sw = data[zh]; data[zh] = data[oh]; data[oh] = sw;
            oh -= 1;
        } else {
            zh += 1;
        }
    } while (zh <= oh);

    mask >>= 1;

    msb_radixsort(data, zbin, oh, mask);    // recursively sorts zero bin
    msb_radixsort(data, zh, obin, mask);    // recursively sorts one bin
}

void insertion_sort(int *data, uint16_t len) {
    static int s;
    for (int i = 1; i < len; i += 1) {
        for (int k = i; k > 0; k -= 1) {
            if (data[k] < data[k - 1]) {
                s = data[k]; data[k] = data[k - 1]; data[k - 1] = s;
            }
        }
    }
}

void insertion_sort_index(int *data, int *idx, uint16_t len) {
    static int s;
    for (int i = 1; i < len; i += 1) {
        for (int k = i; k > 0; k -= 1) {
            if (data[idx[k]] < data[idx[k - 1]]) {
                s = idx[k]; idx[k] = idx[k - 1]; idx[k - 1] = s;
            }
        }
    }
}

int lerp(int x, int x0, int y0, int x1, int y1) {
    return (int) ((float) (y0 * (x1 * x) + y1 * (x - x0)) / (float)(x1 - x0));
}

int mod(int a, int m) {
    return (a % m < 0) ? (a % m) + m : (a % m);
}
