#include "element.h"

element_t new_element(vec2 pos, int num_vertices, vertex_t *vertex_list, uint8_t draw_flag) {
    element_t ret;

    ret.pos = pos;
    ret.rot = 0.f;
    ret.scale = (vec3) { 1, 1, 1 };

    ret.n_vertices = num_vertices;
    ret.n_indices = num_vertices;
    ret.ibuf = NULL;
    ret.vbuf = vmalloc(num_vertices);
    if (ret.vbuf && vertex_list) {
        memcpy(ret.vbuf, vertex_list, sizeof(vertex_t) * num_vertices);
    }

    ret.matrix = IDENT_MAT3;
    ret.draw_flag = draw_flag;

    e_bake_matrix(&ret);

    return ret;
} 

int e_draw(element_t *e, int clear) {
    if (!e || e->n_vertices <= 0) {
        return -1;
    }

    r_set_vbuf(e->n_vertices, e->vbuf);
    r_set_ibuf(e->n_indices, e->ibuf);

    r_render(e->matrix, e->draw_flag, clear);

    return 0;
}

int e_bake_matrix(element_t *e) {
    if (!e) {
        return -1;
    }

    e->matrix = IDENT_MAT3;
    
    rotate_mat3(e->rot, &e->matrix);

    scale_mat3(e->scale, &e->matrix);

    translate_mat3(&e->pos, &e->matrix);

    return 0;
}