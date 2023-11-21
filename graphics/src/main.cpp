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

#define A_MENU		(vec2){ -80, 0 }
#define B_MENU		(vec2){ 80, 0 }

// SYSTEM COMPONENT IDs
#define SC_MARX		0
#define SC_MARXTG70	1
#define SC_MTG		2
#define SC_SWITCH	3
#define SC_SWTG70	4
#define SC_RECLAIM	5
#define SC_MINSUPL	6

// SYSTEM PARAMETER IDs
#define SP_PRESSURE	0
#define SP_TIME		1
#define SP_PID		2
#define SP_KP		3
#define SP_KI		4
#define SP_KD		5

typedef struct state_t {
	menu_t *a_menu;
	menu_t *b_menu;

	int pidx;
	menu_t *path[10];
} state_t;

void init_menus();
void init_menu_options();
void init_input();
int get_input();
int build_polygon(int n_vertices, vertex_t *vbuf, int **ibuf, int radius, uint16_t color);
int build_rectangle(int width, int height, vertex_t *vbuf, int **ibuf, uint16_t color);


font_t amal;
menu_t *main_menu, *set;
element_t menu_box, option_box;
state_t sys;

menu_t *primary = NULL;
menu_t *secondary = NULL;
menu_t *hold;

void init_menus() {
	menu_box = new_element(ORIGIN, 4, NULL, DRAW_LINE_LOOP);
	option_box = new_element(ORIGIN, 4, NULL, DRAW_LINE_LOOP);
	build_rectangle(150, 210, menu_box.vbuf, &menu_box.ibuf, 0xFFFF);
	build_rectangle(127, 30, option_box.vbuf, &option_box.ibuf, 0x7BCF);

	f_setup(&amal, Amalgama, 0xFFFF, 20, 1);

	main_menu = new_menu("MAIN", 4, &amal, A_MENU, &menu_box, &option_box);
	m_set_size(main_menu, 120, 210);
    m_set_draw(main_menu, M_LIST);
	main_menu->cur_color = 0x05A0;

	set = new_menu("SET", 4, &amal, A_MENU, &menu_box, &option_box);
    m_set_size(set, 120, 210);
    m_set_draw(set, M_SET);

	init_menu_options();
}

void init_menu_options() {
	option_t opts[8];

	// MAIN MENU
	opts[0] = (option_t) {"Mode", 	set, 0};
	opts[1] = (option_t) {"Preset", set, 0};
	opts[2] = (option_t) {"Config", set, 0};
	opts[3] = (option_t) {"Sleep", 	set, 0};
	m_set_options(main_menu, main_menu->nopts, opts);

    opts[0] = (option_t) {"00.00", NULL, 12.34};
    m_set_options(set, set->nopts, opts);	
}

void setup() {
	// debug_init();
	// Serial.begin(9600);
	init_display();
	r_init();
	set_line_width(LINE_WIDTH_0);
	init_menus();
	init_input();

	m_draw(main_menu, 0);
    set->center = B_MENU;
    m_draw(set, 0);

	sys.a_menu = main_menu;
	sys.b_menu = NULL;
	sys.pidx = 0;

	primary = main_menu;
	// breakpoint();
}

int prev_cursor = -1;
void loop() {
	// int input = get_input();

	// if (input & U_BTN) {
	// 	m_interact_default(primary, CURS_UP);
	// } if (input & D_BTN) {
	// 	m_interact_default(primary, CURS_DOWN);
	// } if (input & R_BTN) {
	// 	if (secondary) {
	// 		m_draw(secondary, 1);
	// 		secondary->center = A_MENU;
	// 	}
	// 	secondary = primary->options[primary->cursor].target;
	// 	if (secondary) {
	// 		secondary->center = B_MENU;
	// 		m_draw(secondary, 0);
			
	// 		hold = primary;
	// 		primary = secondary;
	// 	}
	// } if (input & L_BTN) {
	// 	if (secondary) {
	// 		m_draw(secondary, 1);
	// 		secondary->center = A_MENU;
			
	// 		primary = hold;
	// 		secondary = NULL;
	// 	}
	// } if (input & A_BTN) {
	// 	m_interact_default(primary, CURS_SELECT);
	// } if (input & B_BTN) {
	// 	m_interact_default(primary, CURS_BACK);
	// }

	// prev_cursor = primary->cursor;
}

void init_input() {
	*DDR_B &= ~(U_BTN | D_BTN | R_BTN | L_BTN | A_BTN | B_BTN);
}

int get_input() {
	return *PIN_B;
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
