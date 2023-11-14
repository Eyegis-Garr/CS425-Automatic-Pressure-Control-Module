#include "draw.h"

// pass vec2 of 0-color,1-color for lerp
void set_line_draw(int x0, int y0, int x1, int y1, uint16_t c) {
    // int cx0 = x0, cy0 = y0, cx1 = x1, cy1 = y1;
    int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1; 
    int err = dx+dy, e2; /* error value e_xy */

    for(;;) {  /* loop */
        // int c = lerp(x0, cx0, cy0, cx1, cy1);
        set_pixel(x0, y0, c);
        if (x0==x1 && y0==y1) break;
        e2 = 2*err;
        if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
        if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
    }
}

void draw_points(int n_indices, int *v_indices, vertex_t *vertices) {
    static vertex_t d_vertex;

    begin_write();
        for (int i = 0; i < n_indices; i += 1) {
            d_vertex = vertices[v_indices[i]];

            set_pixel(d_vertex.pos.x, d_vertex.pos.y, d_vertex.color);
        }
    end_write();
}

void draw_lines(int n_indices, int *v_indices, vertex_t *vertices) {
    static vertex_t a, b;

    begin_write();
        for (int i = 0; i < n_indices; i += 2) {
            a = vertices[v_indices[i]];
            b = vertices[v_indices[i + 1]];

            // can use pixel distance from either point to interpolate color
            // rasterization!

            set_line_draw(a.pos.x, a.pos.y, b.pos.x, b.pos.y, a.color); 
        }
    end_write();
}

void draw_line_strip(int n_indices, int *v_indices, vertex_t *vertices) {
    static vertex_t a, b;

    begin_write();
        for (int i = 0; i < n_indices - 1; i += 1) {
            a = vertices[v_indices[i]];
            b = vertices[v_indices[i + 1]];

            set_line_draw(a.pos.x, a.pos.y, b.pos.x, b.pos.y, a.color); 
        }
    end_write();
}

void draw_line_loop(int n_indices, int *v_indices, vertex_t *vertices) {
    static vertex_t a, b;

    begin_write();
        for (int i = 0; i < n_indices; i += 1) {
            a = vertices[v_indices[i]];

            if (i == n_indices - 1) {
                b = vertices[v_indices[0]];
            } else {
                b = vertices[v_indices[i + 1]];
            }

            set_line_draw(a.pos.x, a.pos.y, b.pos.x, b.pos.y, a.color); 
        }
    end_write();
}

void draw_triangles(int n_indices, int *v_indices, vertex_t *vertices) {
    static vertex_t a, b, c;

    begin_write();
        for (int i = 0; i < n_indices - 2; i += 3) {
            a = vertices[v_indices[i]];
            b = vertices[v_indices[i + 1]];
            c = vertices[v_indices[i + 2]];

            // can use barycentric coords to interpolate color values for pixels within triangle?
            // requires further step of processing before drawing (rasterization!)

            set_line_draw(a.pos.x, a.pos.y, b.pos.x, b.pos.y, a.color);
            set_line_draw(a.pos.x, a.pos.y, c.pos.x, c.pos.y, a.color);
            set_line_draw(c.pos.x, c.pos.y, b.pos.x, b.pos.y, a.color);
        }
    end_write();
}

void draw_triangle_strip(int n_indices, int *v_indices, vertex_t *vertices) {
    static vertex_t a, b, c;

    begin_write();
        for (int i = 0; i < n_indices - 2; i += 1) {
            a = vertices[v_indices[i]];
            b = vertices[v_indices[i + 1]];
            c = vertices[v_indices[i + 2]];

            // can use barycentric coords to interpolate color values for pixels within triangle?
            // requires further step of processing before drawing (rasterization!)

            set_line_draw(a.pos.x, a.pos.y, b.pos.x, b.pos.y, a.color);
            set_line_draw(a.pos.x, a.pos.y, c.pos.x, c.pos.y, a.color);
            set_line_draw(c.pos.x, c.pos.y, b.pos.x, b.pos.y, a.color); 
        }
    end_write();
}

void draw_triangle_fan(int n_indices, int *v_indices, vertex_t *vertices) {
    static vertex_t a, b, c;

    begin_write();
        a = vertices[v_indices[0]];
        for (int i = 1; i < n_indices - 1; i += 1) {
            b = vertices[v_indices[i]];
            c = vertices[v_indices[i + 1]];

            // can use barycentric coords to interpolate color values for pixels within triangle?
            // requires further step of processing before drawing (rasterization!)

            set_line_draw(a.pos.x, a.pos.y, b.pos.x, b.pos.y, a.color);
            set_line_draw(a.pos.x, a.pos.y, c.pos.x, c.pos.y, a.color);
            set_line_draw(c.pos.x, c.pos.y, b.pos.x, b.pos.y, a.color); 
        }
    end_write();
}

void draw_polygon_fill(int n_indices, int *v_indices, vertex_t *vertices) {
    begin_write();
               
    end_write();
}