#ifndef ELEMENT_H
#define ELEMENT_H

#include "graphics.h"
#include "matrix.h"
#include "draw.h"

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

int e_draw(element_t *e, int clear);
int e_bake_matrix(element_t *e);
element_t new_element(vec2 pos, int num_vertices, vertex_t *vertex_list, uint8_t draw_flag);

#endif // ELEMENT_H