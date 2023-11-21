#include "graphics.h"

static render_t render_s;

vertex_t *vmalloc(size_t size) {
    vertex_t *t = render_s.nalloc;
    if (t + size >= &render_s.vram[VMAX])
        return NULL;
    render_s.nalloc += size;

    return t;
}

void vfree(vertex_t *vptr) {
    if (vptr) 
        render_s.nalloc = vptr;
}

int r_init() {

    render_s.flags = 0;
    
    render_s.camera = new_camera(ORIGIN, 0, 1.f);
    
    render_s.n_vertices = 0;
    render_s.n_indices = 0;

    render_s.ibuf = NULL;
    render_s.vptr = NULL;

    render_s.nalloc = render_s.vram;

    return 0;
}

int r_set_vbuf(int num_vertices, vertex_t *vertices) {
    if (num_vertices > 0) {
        render_s.vptr = vertices;
        render_s.n_vertices = num_vertices;
    }

    return 0;
}

int r_set_ibuf(int num_indices, int *indices) {
    render_s.n_indices = num_indices;
    if (num_indices > 0)
        render_s.ibuf = indices;

    return 0;
}

int r_render(mat3 matrix, int draw_flag, int clear) {
    static mat3 transform;
    set_draw_clear(clear);
    c_bake_matrix(&render_s.camera);
    if (render_s.n_vertices > 0) {
        multiply_mat3(&render_s.camera.matrix, &matrix, &transform);
        DRAW[draw_flag](render_s.n_indices, render_s.ibuf, render_s.vptr, &transform);
    }

    return 0;
}

int c_bake_matrix(camera_t *c) {
    vec2 v_translate = (vec2) { (ILI_COLS >> 1) - c->pos.x, (ILI_ROWS >> 1) + c->pos.y };
    vec3 v_scale = (vec3) { c->scale, -1 * c->scale, 1 };

    if (!c) return -1;

    c->matrix = IDENT_MAT3;

    rotate_mat3(c->rot, &c->matrix);

    scale_mat3(v_scale, &c->matrix);

    translate_mat3(&v_translate, &c->matrix);

    return 0;
}

camera_t new_camera(vec2 pos, float rot, float scale) {
    camera_t ret = (camera_t) { pos, rot, scale, IDENT_MAT3 };

    c_bake_matrix(&ret);

    return ret;
}
