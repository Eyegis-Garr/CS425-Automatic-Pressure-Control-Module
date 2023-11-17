#include "ili9341.h"

static const uint8_t PROGMEM init_commands[] = {
    13,
    0, 200, ILI_SWRES,                      // SW Reset
    0, 0, ILI_OFF,                          // Display OFF
    1, 0, ILI_PWRCT1,     0x23,             // GVDD of 4.60v
    1, 0, ILI_PWRCT2,     0x10,             // idk
    2, 0, ILI_VCOMCT1,    0x2B,    0x2B,    // VCOMH of 3.775v, VCOML -1.425
    1, 0, ILI_VCOMCT2,    0xC0,             // nVM -> 1, VCOM offset of VCOMH + 0 and VCOML + 0
    1, 0, ILI_MAC,        ILI_ORI,   // MX (column addr. order), BGR (color filter panel)
    1, 0, ILI_PIXFMT,     0x55,             // 16-bit/pix RGB interface, 16-bit/pix MCU interface
    2, 0, ILI_FRCNOR,     0x00,    0x17,    // Division ratio of fosc, 83Hz refresh (default)
    1, 0, ILI_EMS,        0x06,             // Low voltage detection ctr. enabled, gate driver g1-g320 normal display
    0, 0, ILI_TELOFF,                       // Set Tearing Effect Line OFF    
    0, 150, ILI_SLPOUT,                     // Sleep out
    0, 200, ILI_ON,                         // Display ON    
};

void init_display() {
    init_io();

    hw_reset();
    
    const uint8_t *icommand = init_commands;

    uint8_t num_commands = pgm_read_byte(icommand++);
    uint8_t args, cmd, delay;

    while (num_commands--) {
        args = pgm_read_byte(icommand++);
        delay = pgm_read_byte(icommand++);
        cmd = pgm_read_byte(icommand++);
        
        begin_write();
            set_command(cmd);
            while (args--) set_data_8(pgm_read_byte(icommand++));
        end_write();

        if (delay) _delay_ms(delay);
    }

    clear_screen(0);
}

void hw_reset() {
    *DDR_CTRL |= (1 << RESX);

    clr_b(*P_CTRL, RESX);
    _delay_ms(20);
    set_b(*P_CTRL, RESX);
    _delay_ms(200);
}

void init_io() {
    // setup GPIO to interface with display

    // configure control lines as output
    *DDR_CTRL |= (1 << CSX) | (1 << DCX) | (1 << WRX) | (1 << RDX) | (1 << RESX);

    // default output data direction for data bus
    *DDR_DATA = 0xFF;

    // set all control pins high (idle mode)
    *P_CTRL |= (1 << CSX) | (1 << DCX) | (1 << WRX) | (1 << RDX) | (1 << RESX);
}

void begin_write() {
    clr_b(*P_CTRL, CSX);
}

void end_write() {
    set_b(*P_CTRL, CSX);
}

void set_command(uint8_t cmd) {
    // pull data/command low for command set
    clr_b(*P_CTRL, DCX);

    // set data on data bus
    *P_DATA = cmd;
    // strobe write
    strobe(*P_CTRL, WRX);

    // set data/command high
    set_b(*P_CTRL, DCX);
}

void set_data_8(uint8_t data) {
    // set data/command high for data write
    set_b(*P_CTRL, DCX);

    // set data on data bus
    *P_DATA = data;
    // strobe write/read
    strobe(*P_CTRL, WRX);
}

void set_data_16(uint16_t word) {
    // set data/command high for data write
    set_b(*P_CTRL, DCX);

    // set high byte on data bus
    *P_DATA = word >> 8;
    // strobe write/read
    strobe(*P_CTRL, WRX);

    // set low byte on data bus
    *P_DATA = word;
    // strobe write/read
    strobe(*P_CTRL, WRX);
}

void set_data_32(uint32_t data) {
    // set data/command high for data write
    set_b(*P_CTRL, DCX);

    // set high byte on data bus
    *P_DATA = data >> 24;
    // strobe write/read
    strobe(*P_CTRL, WRX);

    // set high byte on data bus
    *P_DATA = data >> 16;
    // strobe write/read
    strobe(*P_CTRL, WRX);

    // set high byte on data bus
    *P_DATA = data >> 8;
    // strobe write/read
    strobe(*P_CTRL, WRX);

    // set low byte on data bus
    *P_DATA = data;
    // strobe write/read
    strobe(*P_CTRL, WRX);
}

void set_active_area(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    set_command(ILI_CAS);
    set_data_16(x0);
    set_data_16(x1);

    set_command(ILI_PAS);
    set_data_16(y0);
    set_data_16(y1);
}

void set_pixel(uint16_t x, uint16_t y, uint16_t c) {
    // write cycle must be initialized prior
    set_command(ILI_CAS);
    set_data_16(x); set_data_16(x + 1);

    set_command(ILI_PAS);
    set_data_16(y); set_data_16(y + 1);

    set_command(ILI_RAMWR);
    set_data_16(c);
}

void set_color565(uint16_t c, uint32_t ct) {
    set_command(ILI_RAMWR);
    while (ct--) { set_data_16(c); }
}

void clear_screen(uint16_t color) {
    begin_write();
        for (uint32_t i = 0; i < ILI_ROWS; i += 1) {
            for (uint32_t k = 0; k < ILI_COLS; k += 1) {
                set_pixel(k, i, color);
            }
        }
    end_write();
}

