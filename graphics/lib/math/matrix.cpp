#include "matrix.h"

extern inline mat3 mat3_copy(mat3 *m)  { return (mat3){m->a, m->b, m->c, m->d, m->e, m->f, m->g, m->h, m->i}; }
extern inline mat2 mat2_copy(mat2 *m)  { return (mat2){m->a, m->b, m->c, m->d}; }

void multiply_mat2(mat2 *a, mat2 *b, mat2 *p) {
    p->a = a->a * b->a + a->b * b->c;
    p->b = a->a * b->b + a->b * b->d;
    p->c = a->c * b->a + a->d * b->c;
    p->d = a->c * b->b + a->d * b->d;
}

void multiply_mat3(mat3 *a, mat3 *b, mat3 *p) {
    p->a = a->a * b->a + a->b * b->d + a->c * b->g;
    p->b = a->a * b->b + a->b * b->e + a->c * b->h;
    p->c = a->a * b->c + a->b * b->f + a->c * b->i;
    
    p->d = a->d * b->a + a->e * b->d + a->f * b->g;
    p->e = a->d * b->b + a->e * b->e + a->f * b->h;
    p->f = a->d * b->c + a->e * b->f + a->f * b->i;
    
    p->g = a->g * b->a + a->h * b->d + a->i * b->g;
    p->h = a->g * b->b + a->h * b->e + a->i * b->h;
    p->i = a->g * b->c + a->h * b->f + a->i * b->i;
}

void transform_vec2(mat2 *t, vec2 *v, vec2 *w) {
    w->x = t->a * v->x + t->b * v->y;
    w->y = t->c * v->x + t->d * v->y;
}

void transform_vec3(mat3 *t, vec3 *v, vec3 *w) {
    w->x = t->a * v->x + t->b * v->y + t->c * v->w;
    w->y = t->d * v->x + t->e * v->y + t->f * v->w;
    w->w = t->g * v->x + t->h * v->y + t->i * v->w;
}

void translate_mat3(vec2 *t, mat3 *m) {
    mat3 translate = { 1, 0, (float)t->x, 
                       0, 1, (float)t->y, 
                       0, 0, 1 };
    mat3 mcopy = mat3_copy(m);

    multiply_mat3(&translate, &mcopy, m);
}

void rotate_mat3(float theta, mat3 *m) {
    float c_theta = cos(theta), s_theta = sin(theta);
    mat3 rotate = { c_theta, -s_theta, 0,
                    s_theta, c_theta, 0, 
                    0,       0,       1};
    mat3 mcopy = mat3_copy(m);
    multiply_mat3(&rotate, &mcopy, m);
}

void scale_mat3(vec3 s, mat3 *m) {
    mat3 scale = { (float)s.x, 0, 0, 
                   0, (float)s.y, 0,
                   0, 0, (float)s.w };
    mat3 mcopy = mat3_copy(m);
    multiply_mat3(&scale, &mcopy, m);
}

void rotate_mat2(float theta, mat2 *m) {
    float c_theta = cos(theta), s_theta = sin(theta);
    mat2 rotate = { c_theta, -s_theta,
                    s_theta, c_theta};
    mat2 mcopy = mat2_copy(m);
    multiply_mat2(&rotate, &mcopy, m);
}

void scale_mat2(vec3 s, mat2 *m) {
    mat2 scale = { (float)s.x, 0, 
                   0, (float)s.y };
    mat2 mcopy = mat2_copy(m);
    multiply_mat2(&scale, &mcopy, m);
}
