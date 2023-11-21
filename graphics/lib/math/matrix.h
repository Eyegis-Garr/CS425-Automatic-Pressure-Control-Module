#ifndef MATH_H
#define MATH_H

#define __DELAY_BACKWARD_COMPATIBLE__

#define UNIT_X2 (vec2){ 1, 0 }
#define UNIT_Y2 (vec2){ 0, 1 }
#define UNIT_X3 (vec3){ 1, 0, 0 }
#define UNIT_Y3 (vec3){ 0, 1, 0 }
#define UNIT_W3 (vec3){ 0, 0, 1 }
#define ORIGIN (vec2){ 0, 0 }
#define IDENT_MAT2 (mat2){ 1, 0, 1, 0 }
#define IDENT_MAT3 (mat3){ 1, 0, 0, 0, 1, 0, 0, 0, 1 }

#include <Arduino.h>

typedef struct vec2 {
    float x, y;
} vec2;

typedef struct {
    float x, y, w;
} vec3;

typedef struct {
    float a, b;
    float c, d;
} mat2;

typedef struct {
    float a, b, c;
    float d, e, f;
    float g, h, i;
} mat3;

extern inline mat3 mat3_copy(mat3 *m);
extern inline mat2 mat2_copy(mat2 *m);

void multiply_mat2(mat2 *a, mat2 *b, mat2 *p);
void multiply_mat3(mat3 *a, mat3 *b, mat3 *p);

void transform_vec2(mat2 *t, vec2 *v, vec2 *w);
void transform_vec3(mat3 *t, vec3 *v, vec3 *w);

void translate_vec3(vec3 *v, vec3 *t);
void rotate_vec3(float theta, vec3* v);
void scale_vec3(vec3 *sc, vec3 *v);

void translate_vec2(vec2 *v, vec2 *t);
void rotate_vec2(float theta, vec2* v);
void scale_vec2(vec2 *sc, vec2 *v);

void translate_mat3(vec2 *t, mat3 *m);
void rotate_mat3(float theta, mat3 *m);
void scale_mat3(vec3 s, mat3 *m);

void rotate_mat2(float theta, mat2 *m);
void scale_mat2(vec3 s, mat2 *m);

void norm_vec2(vec2 *v);

#endif // MATH_H