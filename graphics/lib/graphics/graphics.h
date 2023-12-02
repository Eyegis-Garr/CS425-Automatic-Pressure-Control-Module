#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <Arduino.h>
#include "matrix.h"
#include "draw.h"

#define VMAX        32

typedef struct {
    vec2 pos;
    float rot;
    float scale;
    mat3 matrix;
} camera_t;

typedef struct render_t {
    uint8_t flags;
    
    camera_t camera;

    int n_vertices;
    int n_indices;

    int *ibuf;
    vertex_t *vptr;
    vertex_t vram[VMAX];

    vertex_t *nalloc;
} render_t;

typedef struct {
    vec2 pos;
    float rot;
    vec3 scale;

    int n_vertices;
    int n_indices;

    int *ibuf;
    vertex_t *vbuf;
    mat3 matrix;

    uint8_t draw_flag;
} element_t;

vertex_t *vmalloc(size_t size);
void vfree(vertex_t *vptr);

int r_init();
int r_set_vbuf(int num_vertices, vertex_t *vertices);
int r_set_ibuf(int num_indices, int *indices);
int r_render(mat3 matrix, int draw_flag, int clear);

element_t new_element(vec2 pos, int num_vertices, vertex_t *vertex_list, uint8_t draw_flag);
int e_draw(element_t *e, int clear);
int e_draw_auto(element_t *e);
int e_bake_matrix(element_t *e);
int e_pip(element_t *e, vec2 p);
int c_bake_matrix(camera_t *c);

camera_t new_camera(vec2 pos, float rot, float scale);

#endif // GRAPHICS_H