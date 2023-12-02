#ifndef DRAW_H
#define DRAW_H

#include <Arduino.h>
#include "ili9341.h"
#include "matrix.h"

#define DRAW_COLS       ILI_COLS
#define DRAW_ROWS       ILI_ROWS
#define DRAW_MEMLEN     ILI_MEMLEN

#define DRAW_POINTS             0
#define DRAW_LINES              1
#define DRAW_LINE_STRIP         2
#define DRAW_LINE_LOOP          3
#define DRAW_TRIANGLES          4
#define DRAW_TRIANGLE_STRIP     5
#define DRAW_TRIANGLE_FAN       6

#define LINE_WIDTH_0     1
#define LINE_WIDTH_2     2

typedef struct {
    vec3 pos;
    uint16_t color;
} vertex_t;

typedef void (*DrawCallback)(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform);

void set_line_width(int width);
void set_draw_clear(int clear);
void set_line_draw(int x0, int y0, int x1, int y1, uint16_t c);

void draw_points(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform);
void draw_lines(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform);
void draw_line_strip(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform);
void draw_line_loop(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform);
void draw_triangles(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform);
void draw_triangle_strip(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform);
void draw_triangle_fan(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform);
void draw_polygon_fill(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform);

extern uint8_t _LINE_WIDTH;
extern uint8_t _CLEAR;
extern uint16_t color;
extern vec3 A, B, C;


static const DrawCallback DRAW[8] = {
    draw_points,
    draw_lines,
    draw_line_strip,
    draw_line_loop,
    draw_triangles,
    draw_triangle_strip,
    draw_triangle_fan,
    draw_polygon_fill
};

#endif // DRAW_H
