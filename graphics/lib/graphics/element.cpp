#include "graphics.h"

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

int e_draw_auto(element_t *e) {
    if (!e || e->n_vertices <= 0) {
        return -1;
    }

    r_set_vbuf(e->n_vertices, e->vbuf);
    r_set_ibuf(e->n_indices, e->ibuf);

    r_render(e->matrix, e->draw_flag, 1);
    e_bake_matrix(e);
    r_render(e->matrix, e->draw_flag, 0);

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

int e_pip(element_t *e, vec2 p) {
    static vec3 a, b;
    uint8_t in = 0;

    if (!e || e->n_vertices < 3) return 0;

    int minx = 0, maxx = 0, miny = 0, maxy = 0;
    for (int i = 0; i < e->n_indices; i += 1) {
        a = e->vbuf[e->ibuf[i]].pos;
        minx = min(a.x, minx);
        maxx = max(a.x, maxx);
        miny = min(a.y, miny);
        maxy = max(a.y, maxy);
    }

    if (p.x < minx || p.x > maxx || p.y < miny || p.y > maxy) return 0;

    int i, k = e->n_indices - 1;
    for (i = 0; i < e->n_indices; i += 1) {
        transform_vec3(&e->matrix, &e->vbuf[e->ibuf[i]].pos, &a);
        transform_vec3(&e->matrix, &e->vbuf[e->ibuf[k]].pos, &b);

        if ((a.y > p.y) != (b.y > p.y) && 
            p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x) {
            in = ~in;
        }

        k = i;
    }

    return in;
}