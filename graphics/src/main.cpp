#include <Arduino.h>
#include <avr8-stub.h>
#include <string.h>

#include "ili9341.h"
#include "graphics.h"
#include "element.h"
#include "Amalgama.h"
#include "font.h"

font_t amal;

void setup() {
	init_display();

	f_setup(&amal, Amalgama, 16, 3);
	
}

void loop() {
	
}

