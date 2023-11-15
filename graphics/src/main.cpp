#include <Arduino.h>
#include <avr8-stub.h>
#include <string.h>

#include "ili9341.h"
#include "graphics.h"
#include "element.h"
#include "font.h"
#include "menu.h"

// FONT HEADERS
#include "Amalgama.h"

#define U_BTN       0x40  // digital pin 12 (Up)
#define D_BTN       0x02  // digital pin 52 (Down)
#define R_BTN       0x04  // digital pin 51 (Right)
#define L_BTN       0x08  // digital pin 50 (Left)
#define A_BTN       0x01  // digital pin 53 (A)
#define B_BTN       0x10  // digital pin 10 (B)

static volatile uint8_t *PORT_B = (uint8_t*) 0x25;
static volatile uint8_t *DDR_B  = (uint8_t*) 0x24;
static volatile uint8_t *PIN_B  = (uint8_t*) 0x23;

void init_menus();
void init_input();
int get_input();
int build_polygon(int n_vertices, vertex_t *vbuf, int **ibuf, int radius, uint16_t color);
int build_rectangle(int width, int height, vertex_t *vbuf, int **ibuf, uint16_t color);

font_t amal;
menu_t *test;
const char *test_opts[OPTION_LEN] = {
	"Presets",
	"Set Points",
	"Set Times",
	"Alarms",
	"PID",
	"EXIT"
};

void init_menus() {
	f_setup(&amal, Amalgama, 0xFFFF, 20, 1);
	test = new_menu("MAIN MENU", 6, test_opts, &amal, 0);
	m_set_aabb(test, (vec2){-100,150}, (vec2){100,-150});
	test->cur_color = 0x05A0;

	test->m_element = new_element(test->center, 4, NULL, DRAW_LINE_LOOP, 0);
	build_rectangle(test->w, test->h, test->m_element.vbuf, &test->m_element.ibuf, 0xFFFF);

	test->o_element = new_element(ORIGIN, 4, NULL, DRAW_LINE_LOOP, 0);
	build_rectangle(test->w * 0.85, 30, test->o_element.vbuf, &test->o_element.ibuf, 0x7BCF);
	
	test->cursor = 4;

	m_draw(test);
}

void setup() {
	// debug_init();
	Serial.begin(9600);
	init_display();
	r_init();
	set_line_width(LINE_WIDTH_0);
	init_menus();
	init_input();
}

void loop() {
	int input = get_input();
	char buf[8]; sprintf(buf, "%02x", input);

	if (input & U_BTN) {
		m_interact(test, CURS_UP);
	} else if (input & D_BTN) {
		m_interact(test, CURS_DOWN);
	} else if (input & R_BTN) {
		m_interact(test, CURS_LEFT);
	} else if (input & L_BTN) {
		m_interact(test, CURS_RIGHT);
	} else if (input & A_BTN) {
		m_interact(test, CURS_SELECT);
	} else if (input & B_BTN) {
		m_interact(test, CURS_BACK);
	}

	Serial.println(buf);

	_delay_ms(10);
}

void init_input() {
	*DDR_B &= ~(U_BTN | D_BTN | R_BTN | L_BTN | A_BTN | B_BTN);
}

int get_input() {
	return *PIN_B;
}

int build_polygon(int n_vertices, vertex_t *vbuf, int **ibuf, int radius, uint16_t color) {
	if (!vbuf || !ibuf || n_vertices <= 0) {
		return -1;
	}

	(*ibuf) = (int*) malloc(sizeof(int) * n_vertices);
	float r = DEG_TO_RAD * (360 / n_vertices);
	mat3 o = IDENT_MAT3;
	vec3 v = { 1, 1, 1 };

	scale_mat3((vec3){radius, radius, 1}, &o);
	rotate_mat3(r, &o);

	for (int i = 0; i < n_vertices; i += 1) {
		// compute color
		vbuf[i].color = color;
		transform_vec3(&o, &v, &vbuf[i].pos);
		(*ibuf)[i] = i;
		rotate_mat3(r, &o);
	}

	return 0;
}

int build_rectangle(int width, int height, vertex_t *vbuf, int **ibuf, uint16_t color) {
	if (!vbuf || !ibuf) {
		return -1;
	}

	int w2 = width / 2, h2 = height / 2;
	(*ibuf) = (int*) malloc(sizeof(int) * 4);
	for (int i = 0; i < 4; i += 1) {
		(*ibuf)[i] = i;
		vbuf[i].color = color;
	}

	vbuf[0].pos = (vec3) {w2, h2, 1};
	vbuf[1].pos = (vec3) {-w2, h2, 1};
	vbuf[2].pos = (vec3) {-w2, -h2, 1};
	vbuf[3].pos = (vec3) {w2, -h2, 1};

	return 0;
}
