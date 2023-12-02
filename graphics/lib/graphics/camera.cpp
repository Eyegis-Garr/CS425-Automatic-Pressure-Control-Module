#include "graphics.h"

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
