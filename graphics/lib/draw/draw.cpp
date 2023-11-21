#include "draw.h"

uint8_t _LINE_WIDTH = LINE_WIDTH_0;
uint8_t _CLEAR = 0;
vec3 A, B, C;
uint16_t color;

void set_line_width(int width) {
    _LINE_WIDTH = width;
}

void set_draw_clear(int clear) {
    _CLEAR = clear;
}

void set_line_draw(int x0, int y0, int x1, int y1, uint16_t c) {
    int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1; 
    int err = dx+dy, e2; /* error value e_xy */

    for(;;) {  /* loop */
        for (int i = _LINE_WIDTH / -2; i <= _LINE_WIDTH / 2; i += 1) {
            set_pixel(x0 + i, y0, c);
            set_pixel(x0, y0 + i, c);
        }
        if (x0==x1 && y0==y1) break;
        e2 = 2*err;
        if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
        if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
    }
}

void draw_points(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform) {
    begin_write();
        for (int i = 0; i < n_indices; i += 1) {
            color = vertices[v_indices[i]].color;
            transform_vec3(transform, &vertices[v_indices[i]].pos, &A);
            set_pixel(A.x, A.y, (_CLEAR) ?  0 : color);
        }
    end_write();
}

void draw_lines(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform) {
    begin_write();
        for (int i = 0; i < n_indices; i += 2) {      
            color = vertices[v_indices[i]].color;      
            transform_vec3(transform, &vertices[v_indices[i]].pos, &A);
            transform_vec3(transform, &vertices[v_indices[i+1]].pos, &B);
            set_line_draw(A.x, A.y, B.x, B.y, (_CLEAR) ?  0 : color); 
        }
    end_write();
}

void draw_line_strip(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform) {
    begin_write();
        for (int i = 0; i < n_indices - 1; i += 1) {
            color = vertices[v_indices[i]].color;
            
            transform_vec3(transform, &vertices[v_indices[i]].pos, &A);
            transform_vec3(transform, &vertices[v_indices[i+1]].pos, &B);

            set_line_draw(A.x, A.y, B.x, B.y, (_CLEAR) ?  0 : color); 
        }
    end_write();
}

void draw_line_loop(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform) {
    begin_write();
        for (int i = 0; i < n_indices; i += 1) {
            color = vertices[v_indices[i]].color;

            if (i == n_indices - 1) {
                transform_vec3(transform, &vertices[v_indices[0]].pos, &B);
            } else {
                transform_vec3(transform, &vertices[v_indices[i+1]].pos, &B);
            }

            transform_vec3(transform, &vertices[v_indices[i]].pos, &A);

            set_line_draw(A.x, A.y, B.x, B.y, (_CLEAR) ?  0 : color); 
        }
    end_write();
}

void draw_triangles(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform) {

    begin_write();
        for (int i = 0; i < n_indices - 2; i += 3) {
            color = vertices[v_indices[i]].color;

            transform_vec3(transform, &vertices[v_indices[i]].pos, &A);
            transform_vec3(transform, &vertices[v_indices[i+1]].pos, &B);
            transform_vec3(transform, &vertices[v_indices[i+2]].pos, &C);

            set_line_draw(A.x, A.y, B.x, B.y, (_CLEAR) ?  0 : color);
            set_line_draw(A.x, A.y, C.x, C.y, (_CLEAR) ?  0 : color);
            set_line_draw(C.x, C.y, B.x, B.y, (_CLEAR) ?  0 : color);
        }
    end_write();
}

void draw_triangle_strip(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform) {

    begin_write();
        for (int i = 0; i < n_indices - 2; i += 1) {
            color = vertices[v_indices[i]].color;

            transform_vec3(transform, &vertices[v_indices[i]].pos, &A);
            transform_vec3(transform, &vertices[v_indices[i+1]].pos, &B);
            transform_vec3(transform, &vertices[v_indices[i+2]].pos, &C);

            set_line_draw(A.x, A.y, B.x, B.y, (_CLEAR) ?  0 : color);
            set_line_draw(A.x, A.y, C.x, C.y, (_CLEAR) ?  0 : color);
            set_line_draw(C.x, C.y, B.x, B.y, (_CLEAR) ?  0 : color); 
        }
    end_write();
}

void draw_triangle_fan(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform) {

    begin_write();
        A = vertices[v_indices[0]].pos;
        transform_vec3(transform, &vertices[v_indices[0]].pos, &A);
        for (int i = 1; i < n_indices - 1; i += 1) {
            color = vertices[v_indices[i]].color;

            transform_vec3(transform, &vertices[v_indices[i]].pos, &B);
            transform_vec3(transform, &vertices[v_indices[i+1]].pos, &C);

            set_line_draw(A.x, A.y, B.x, B.y, (_CLEAR) ?  0 : color);
            set_line_draw(A.x, A.y, C.x, C.y, (_CLEAR) ?  0 : color);
            set_line_draw(C.x, C.y, B.x, B.y, (_CLEAR) ?  0 : color); 
        }
    end_write();
}

void draw_polygon_fill(int n_indices, int *v_indices, vertex_t *vertices, mat3 *transform) {
    begin_write();
               
    end_write();
}