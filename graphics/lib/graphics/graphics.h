#ifndef GRAPHICS_H
#define GRAPHICS_H

#define __DELAY_BACKWARD_COMPATIBLE__
#include <Arduino.h>
#include <inttypes.h>
#include <avr8-stub.h>

#include "matrix.h"
#include "draw.h"

#define RW_TOGGLE   0
#define VMAX        64

typedef struct {
    vec2 pos;
    float rot;
    float scale;
    mat3 matrix;
} camera_t;

static struct render_s {
    uint8_t flags;
    
    camera_t camera;

    int n_vertices;
    int n_indices;

    int *ibuf;
    vertex_t *vptr;
    vertex_t vbuf[VMAX];
    vertex_t vram[VMAX];

    vertex_t *nalloc;
} render_s;

vertex_t *vmalloc(size_t size);
void vfree(vertex_t *vptr);

int r_init();

int r_set_vbuf(int num_vertices, vertex_t *vertices);
int r_set_ibuf(int num_indices, int *indices);

int r_render(mat3 matrix, int draw_flag, int draw_layer);

int c_bake_matrix(camera_t *c);
camera_t new_camera(vec2 pos, float rot, float scale);

#endif // GRAPHICS_H