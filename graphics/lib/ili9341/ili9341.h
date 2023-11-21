#ifndef ILI9341_H
#define ILI9341_H

#define __DELAY_BACKWARD_COMPATIBLE__
#include <Arduino.h>
#include <inttypes.h>

/*
TODO:
    implement landscape and portrait switching
        ILI_MAC command
    better color control
    actual rasterization?
*/

// CONTROL & DATA GPIO
extern volatile uint8_t* P_DATA;
extern volatile uint8_t* DDR_DATA;
extern volatile uint8_t* P_CTRL;
extern volatile uint8_t* DDR_CTRL;

// DATA BUS DIRECTONALITY
#define DATA_WR     { DDR_DATA |= 0xFF; }
#define DATA_RD     { DDR_DATA &= 0x00; }

// CONTROL REGISTER BIT POSITIONS
#define CSX         0
#define DCX         1
#define WRX         2
#define RDX         3
#define RESX        4

#define set_b(R,B)  { R |= (1 << B); }
#define clr_b(R,B)  { R &= ~(1 << B); }
#define strobe(R,B) { clr_b(R,B); set_b(R,B); }

// DISPLAY ORIENTATION
// #define PORTRAIT
#define LANDSCAPE

#define ILI_MEMLEN  76800
// DISPLAY DIMENSIONS
#if defined(PORTRAIT)
#define ILI_COLS    240
#define ILI_ROWS    320
#define ILI_ORI     0x4C
#elif defined(LANDSCAPE)
#define ILI_COLS    320
#define ILI_ROWS    240
#define ILI_ORI     0x2C
#endif

// ILI9341 COMMANDS
#define ILI_NOP     0x00    // No Operation
#define ILI_SWRES   0x01    // Software Reset (5ms delay post-res; 120ms delay post-res if in SLEEP OUT)
#define ILI_RDID    0x04    // Read Display ID
#define ILI_RDDST   0x09    // Read Display Status
#define ILI_SLPIN   0x10    // Sleep IN
#define ILI_SLPOUT  0x11    // Sleep OUT (5ms delay post-sleep out)
#define ILI_PARON   0x12    // Partial Area On
#define ILI_NORMON  0x13    // Normal Area On
#define ILI_RDMODE  0x0A    // Read Display Power Mode
#define ILI_RDMADCT 0x0B    // Read Display MADCTL
#define ILI_RDPIXFT 0x0C    // Read Display Pixel Format
#define ILI_RDIMGFT 0x0D    // Read Display Image Format
#define ILI_RDSELFD 0x0F    // Read Display Self-Diagnostic report
#define ILI_INVOFF  0x20    // Display Inversion OFF
#define ILI_INVON   0x21    // Display Inversion ON
#define ILI_GAMSET  0x26    // Gamma Set
#define ILI_OFF     0x28    // Display OFF
#define ILI_ON      0x29    // Display ON
#define ILI_CAS     0x2A    // Column Address Set
#define ILI_PAS     0x2B    // Page Address Set
#define ILI_RAMWR   0x2C    // Memory Write
#define ILI_RAMRD   0x2E    // Memory Read
#define ILI_CSET    0x2D    // Color Set
#define ILI_PAR     0x30    // Partial Area
#define ILI_VSD     0x33    // Vertical Scrolling Direction
#define ILI_TELOFF  0x34    // Tearing Effect Line OFF
#define ILI_TELON   0x35    // Tearing Effect Line ON
#define ILI_MAC     0x36    // Memory Access Control
#define ILI_VSSA    0x37    // Vertical Scrolling Start Address
#define ILI_IDLOFF  0x38    // Idle Mode OFF
#define ILI_IDLON   0x39    // Idle Mode ON
#define ILI_PIXFMT  0x3A    // Pixel Format Set
#define ILI_WRMEMC  0x3C    // Write Memory Continue
#define ILI_RDMEMC  0x3E    // Read Memory Continue
#define ILI_STS     0x44    // Set Tear Scanline
#define ILI_GSC     0x45    // Get Scanline
#define ILI_WRBRT   0x51    // Write Display Brightness
#define ILI_RDBRT   0x52    // Read Display Brightness
#define ILI_WRCTD   0x53    // Write CTRL Display
#define ILI_RDCTD   0x54    // Read CTRL Display
#define ILI_WRCABC  0x55    // Write Content Adaptive Brightness Control (CABC)
#define ILI_RDCABC  0x56    // Read CABC
#define ILI_WCABCMN 0x5E    // Write CABC Minimum Brightness
#define ILI_RCABCMN 0x5F    // Read CABC Minimum Brightness
// LEVEL 2 COMMANDS
#define ILI_RGBISC  0xB0    // RGB Interface Signal Control
#define ILI_FRCNOR  0xB1    // Frame Rate Control (Normal Mode/Full Colors)
#define ILI_FRCIDL  0xB2    // Frame Rate Control (Idle Mode/8 Colors)
#define ILI_FRCPAR  0xB3    // Frame Rate Control (Partial Mode/Full Colors)
#define ILI_DINVCT  0xB4    // Display Inversion Control
#define ILI_BLNKPC  0xB5    // Blanking Porch Control
#define ILI_DFC     0xB6    // Display Function Control
#define ILI_EMS     0xB7    // Entry Mode Set
#define ILI_BLCT1   0xB8    // Backlight Control 1
#define ILI_BLCT2   0xB9    // Backlight Control 2 ...
#define ILI_BLCT3   0xBA
#define ILI_BLCT4   0xBB
#define ILI_BLCT5   0xBC
#define ILI_BLCT7   0xBD
#define ILI_BLCT8   0xBF
#define ILI_PWRCT1  0xC0    // Power Control 1
#define ILI_PWRCT2  0xC1    // Power Control 2
#define ILI_VCOMCT1 0xC5    // VCOM Control 1
#define ILI_VCOMCT2 0xC7    // VCOM Control 2
#define ILI_NVMEMWR 0xD0    // NV Memory Write
#define ILI_NVMEMPR 0xD1    // NV Memory Protection Key
#define ILI_NVMEMST 0xD2    // NV Memory Status Read
#define ILI_READID4 0xD3    // Read ID4
#define ILI_POSGAMC 0xE0    // Positive Gamma Correction
#define ILI_NEGGAMC 0xE1    // Negative Gamma Correction
#define ILI_DIGGAM1 0xE2    // Digital Gamma Control 1
#define ILI_DIGGAM2 0xE3    // Digital Gamma Control 2
#define ILI_IFCTRL  0xF6    // Interface Control
#define ILI_PWRCTA  0xCB    // Power Control A
#define ILI_PWRCTB  0xCF    // Power Control B
#define ILI_DTCA    0xE8    // Driver Timing Control A
#define ILI_DTCB    0xEA    // Driver Timing Control B
#define ILI_PWSEQCT 0xED    // Power ON Sequence Control
#define ILI_EN3G    0xF2    // Enable 3G
#define ILI_PMPRAT  0xF7    // Pump Ration Control

void init_display();
void hw_reset();
void init_io();

void set_command(uint8_t cmd);

void set_data_8(uint8_t data);
void set_data_16(uint16_t word);
void set_data_32(uint32_t data);

void set_active_area(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void set_pixel(uint16_t x, uint16_t y, uint16_t c);
void set_color565(uint16_t c, uint32_t ct);

void clear_screen(uint16_t color);

void begin_write();
void end_write();

#endif // ILI9341_H