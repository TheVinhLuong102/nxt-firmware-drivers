#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "nxt_lcd.h"
#include "systick.h"
#include "nxt_spi.h"

typedef unsigned int uint;

#define DISPLAY_WIDTH (NXT_LCD_WIDTH)
#define DISPLAY_DEPTH (NXT_LCD_DEPTH)

/* NOTE
 * The following buffer is declared with one extra line (the +1).
 * This is to allow fast dma update of the screen (see nxt_spi.c
 * for details).
 */
static struct {
  uint8_t display[DISPLAY_DEPTH+1][DISPLAY_WIDTH];
} display_array;
static uint8_t (*display_buffer)[DISPLAY_WIDTH] = display_array.display;

/* Font table for a 5x8 font. 1 pixel spacing between chars */
#define N_CHARS 128
#define FONT_WIDTH 5
#define CELL_WIDTH (FONT_WIDTH + 1)
#define DISPLAY_CHAR_WIDTH (DISPLAY_WIDTH/(CELL_WIDTH))
#define DISPLAY_CHAR_DEPTH (DISPLAY_DEPTH)

static const struct {
  uint8_t font[N_CHARS][FONT_WIDTH];
} font_array = {{
/* 0x00 */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x01 */ {0x3E, 0x55, 0x61, 0x55, 0x3E},
/* 0x02 */ {0x3E, 0x6B, 0x5F, 0x6B, 0x3E},
/* 0x03 */ {0x0C, 0x1E, 0x3C, 0x1E, 0x0C},
/* 0x04 */ {0x08, 0x1C, 0x3E, 0x1C, 0x08},
/* 0x05 */ {0x08, 0x7c, 0x0e, 0x7c, 0x08}, /* SHIFT char */
/* 0x06 */ {0x18, 0x5C, 0x7E, 0x5C, 0x18},
/* 0x07 */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x08 */ {0x08, 0x1c, 0x3e, 0x08, 0x08}, /* BACKSPACE char */
/* 0x09 */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x0A */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x0B */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x0C */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x0D */ {0x10, 0x38, 0x7c, 0x10, 0x1e}, /* ENTER char */
/* 0x0E */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x0F */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x10 */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x11 */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x12 */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x13 */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x14 */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x15 */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x16 */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x17 */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x18 */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x19 */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x1A */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x1B */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x1C */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x1D */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x1E */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x1F */ {0x3E, 0x36, 0x2A, 0x36, 0x3E},
/* 0x20 */ {0x00, 0x00, 0x00, 0x00, 0x00},
/* 0x21 */ {0x00, 0x06, 0x5f, 0x06, 0x00},
/* 0x22 */ {0x07, 0x03, 0x00, 0x07, 0x03},
/* 0x23 */ {0x24, 0x7e, 0x24, 0x7e, 0x24},
/* 0x24 */ {0x24, 0x2b, 0x6a, 0x12, 0x00},
/* 0x25 */ {0x63, 0x13, 0x08, 0x64, 0x63},
/* 0x26 */ {0x36, 0x49, 0x56, 0x20, 0x50},
/* 0x27 */ {0x00, 0x07, 0x03, 0x00, 0x00},
/* 0x28 */ {0x00, 0x3e, 0x41, 0x00, 0x00},
/* 0x29 */ {0x00, 0x41, 0x3e, 0x00, 0x00},
/* 0x2A */ {0x08, 0x3e, 0x1c, 0x3e, 0x08},
/* 0x2B */ {0x08, 0x08, 0x3e, 0x08, 0x08},
/* 0x2C */ {0x00, 0xe0, 0x60, 0x00, 0x00},
/* 0x2D */ {0x08, 0x08, 0x08, 0x08, 0x08},
/* 0x2E */ {0x00, 0x60, 0x60, 0x00, 0x00},
/* 0x2F */ {0x20, 0x10, 0x08, 0x04, 0x02},
/* 0x30 */ {0x3e, 0x51, 0x49, 0x45, 0x3e},
/* 0x31 */ {0x00, 0x42, 0x7f, 0x40, 0x00},
/* 0x32 */ {0x62, 0x51, 0x49, 0x49, 0x46},
/* 0x33 */ {0x22, 0x49, 0x49, 0x49, 0x36},
/* 0x34 */ {0x18, 0x14, 0x12, 0x7f, 0x10},
/* 0x35 */ {0x2f, 0x49, 0x49, 0x49, 0x31},
/* 0x36 */ {0x3c, 0x4a, 0x49, 0x49, 0x30},
/* 0x37 */ {0x01, 0x71, 0x09, 0x05, 0x03},
/* 0x38 */ {0x36, 0x49, 0x49, 0x49, 0x36},
/* 0x39 */ {0x06, 0x49, 0x49, 0x29, 0x1e},
/* 0x3A */ {0x00, 0x6c, 0x6c, 0x00, 0x00},
/* 0x3B */ {0x00, 0xec, 0x6c, 0x00, 0x00},
/* 0x3C */ {0x08, 0x14, 0x22, 0x41, 0x00},
/* 0x3D */ {0x24, 0x24, 0x24, 0x24, 0x24},
/* 0x3E */ {0x00, 0x41, 0x22, 0x14, 0x08},
/* 0x3F */ {0x02, 0x01, 0x59, 0x09, 0x06},
/* 0x40 */ {0x3e, 0x41, 0x5d, 0x55, 0x1e},
/* 0x41 */ {0x7e, 0x11, 0x11, 0x11, 0x7e},
/* 0x42 */ {0x7f, 0x49, 0x49, 0x49, 0x36},
/* 0x43 */ {0x3e, 0x41, 0x41, 0x41, 0x22},
/* 0x44 */ {0x7f, 0x41, 0x41, 0x41, 0x3e},
/* 0x45 */ {0x7f, 0x49, 0x49, 0x49, 0x41},
/* 0x46 */ {0x7f, 0x09, 0x09, 0x09, 0x01},
/* 0x47 */ {0x3e, 0x41, 0x49, 0x49, 0x7a},
/* 0x48 */ {0x7f, 0x08, 0x08, 0x08, 0x7f},
/* 0x49 */ {0x00, 0x41, 0x7f, 0x41, 0x00},
/* 0x4A */ {0x30, 0x40, 0x40, 0x40, 0x3f},
/* 0x4B */ {0x7f, 0x08, 0x14, 0x22, 0x41},
/* 0x4C */ {0x7f, 0x40, 0x40, 0x40, 0x40},
/* 0x4D */ {0x7f, 0x02, 0x04, 0x02, 0x7f},
/* 0x4E */ {0x7f, 0x02, 0x04, 0x08, 0x7f},
/* 0x4F */ {0x3e, 0x41, 0x41, 0x41, 0x3e},
/* 0x50 */ {0x7f, 0x09, 0x09, 0x09, 0x06},
/* 0x51 */ {0x3e, 0x41, 0x51, 0x21, 0x5e},
/* 0x52 */ {0x7f, 0x09, 0x09, 0x19, 0x66},
/* 0x53 */ {0x26, 0x49, 0x49, 0x49, 0x32},
/* 0x54 */ {0x01, 0x01, 0x7f, 0x01, 0x01},
/* 0x55 */ {0x3f, 0x40, 0x40, 0x40, 0x3f},
/* 0x56 */ {0x1f, 0x20, 0x40, 0x20, 0x1f},
/* 0x57 */ {0x3f, 0x40, 0x3c, 0x40, 0x3f},
/* 0x58 */ {0x63, 0x14, 0x08, 0x14, 0x63},
/* 0x59 */ {0x07, 0x08, 0x70, 0x08, 0x07},
/* 0x5A */ {0x61, 0x51, 0x49, 0x45, 0x43},
/* 0x5B */ {0x00, 0x7f, 0x41, 0x41, 0x00},
/* 0x5C */ {0x02, 0x04, 0x08, 0x10, 0x20},
/* 0x5D */ {0x00, 0x41, 0x41, 0x7f, 0x00},
/* 0x5E */ {0x04, 0x02, 0x01, 0x02, 0x04},
/* 0x5F */ {0x80, 0x80, 0x80, 0x80, 0x80},
/* 0x60 */ {0x00, 0x03, 0x07, 0x00, 0x00},
/* 0x61 */ {0x20, 0x54, 0x54, 0x54, 0x78},
/* 0x62 */ {0x7f, 0x44, 0x44, 0x44, 0x38},
/* 0x63 */ {0x38, 0x44, 0x44, 0x44, 0x48},
/* 0x64 */ {0x38, 0x44, 0x44, 0x44, 0x7f},
/* 0x65 */ {0x38, 0x54, 0x54, 0x54, 0x08},
/* 0x66 */ {0x04, 0x7e, 0x05, 0x05, 0x00},
/* 0x67 */ {0x18, 0xa4, 0xa4, 0xa4, 0x7c},
/* 0x68 */ {0x7f, 0x04, 0x04, 0x04, 0x78},
/* 0x69 */ {0x00, 0x44, 0x7d, 0x40, 0x00},
/* 0x6A */ {0x40, 0x80, 0x84, 0x7d, 0x00},
/* 0x6B */ {0x7f, 0x10, 0x28, 0x44, 0x40},
/* 0x6C */ {0x00, 0x41, 0x7f, 0x40, 0x00},
/* 0x6D */ {0x7c, 0x04, 0x18, 0x04, 0x78},
/* 0x6E */ {0x7c, 0x08, 0x04, 0x04, 0x78},
/* 0x6F */ {0x38, 0x44, 0x44, 0x44, 0x38},
/* 0x70 */ {0xfc, 0x28, 0x44, 0x44, 0x38},
/* 0x71 */ {0x38, 0x44, 0x44, 0x28, 0xfc},
/* 0x72 */ {0x44, 0x78, 0x44, 0x04, 0x08},
/* 0x73 */ {0x48, 0x54, 0x54, 0x54, 0x20},
/* 0x74 */ {0x04, 0x3e, 0x44, 0x44, 0x00},
/* 0x75 */ {0x3c, 0x40, 0x40, 0x20, 0x7c},
/* 0x76 */ {0x1c, 0x20, 0x40, 0x20, 0x1c},
/* 0x77 */ {0x3c, 0x60, 0x30, 0x60, 0x3c},
/* 0x78 */ {0x44, 0x28, 0x10, 0x28, 0x44},
/* 0x79 */ {0x9c, 0xa0, 0xa0, 0x60, 0x3c},
/* 0x7A */ {0x44, 0x64, 0x54, 0x4c, 0x44},
/* 0x7B */ {0x08, 0x3e, 0x41, 0x41, 0x00},
/* 0x7C */ {0x00, 0x00, 0x77, 0x00, 0x00},
/* 0x7D */ {0x00, 0x41, 0x41, 0x3e, 0x08},
/* 0x7E */ {0x02, 0x01, 0x02, 0x01, 0x00},
#ifdef oldfont
/* 0x21 */ {0x00, 0x00, 0x5F, 0x00, 0x00},
/* 0x22 */ {0x00, 0x07, 0x00, 0x07, 0x00},
/* 0x23 */ {0x14, 0x3E, 0x14, 0x3E, 0x14},
/* 0x24 */ {0x04, 0x2A, 0x7F, 0x2A, 0x10},
/* 0x25 */ {0x26, 0x16, 0x08, 0x34, 0x32},
/* 0x26 */ {0x36, 0x49, 0x59, 0x26, 0x50},
/* 0x27 */ {0x00, 0x00, 0x07, 0x00, 0x00},
/* 0x28 */ {0x00, 0x1C, 0x22, 0x41, 0x00},
/* 0x29 */ {0x00, 0x41, 0x22, 0x1C, 0x00},
/* 0x2A */ {0x2A, 0x1C, 0x7F, 0x1C, 0x2A},
/* 0x2B */ {0x08, 0x08, 0x3E, 0x08, 0x08},
/* 0x2C */ {0x00, 0x50, 0x30, 0x00, 0x00},
/* 0x2D */ {0x08, 0x08, 0x08, 0x08, 0x08},
/* 0x2E */ {0x00, 0x60, 0x60, 0x00, 0x00},
/* 0x2F */ {0x20, 0x10, 0x08, 0x04, 0x02},
/* 0x30 */ {0x3E, 0x51, 0x49, 0x45, 0x3E},
/* 0x31 */ {0x00, 0x42, 0x7F, 0x40, 0x00},
/* 0x32 */ {0x42, 0x61, 0x51, 0x49, 0x46},
/* 0x33 */ {0x21, 0x41, 0x45, 0x4B, 0x31},
/* 0x34 */ {0x18, 0x14, 0x12, 0x7F, 0x10},
/* 0x35 */ {0x27, 0x45, 0x45, 0x45, 0x39},
/* 0x36 */ {0x3C, 0x4A, 0x49, 0x49, 0x30},
/* 0x37 */ {0x01, 0x01, 0x79, 0x05, 0x03},
/* 0x38 */ {0x36, 0x49, 0x49, 0x49, 0x36},
/* 0x39 */ {0x06, 0x49, 0x49, 0x29, 0x1E},
/* 0x3A */ {0x00, 0x36, 0x36, 0x00, 0x00},
/* 0x3B */ {0x00, 0x56, 0x36, 0x00, 0x00},
/* 0x3C */ {0x08, 0x14, 0x22, 0x41, 0x00},
/* 0x3D */ {0x14, 0x14, 0x14, 0x14, 0x14},
/* 0x3E */ {0x41, 0x22, 0x14, 0x08, 0x00},
/* 0x3F */ {0x02, 0x01, 0x59, 0x05, 0x02},
/* 0x40 */ {0x1C, 0x2A, 0x36, 0x3E, 0x0C},
/* 0x41 */ {0x7E, 0x09, 0x09, 0x09, 0x7E},
/* 0x42 */ {0x7F, 0x49, 0x49, 0x49, 0x3E},
/* 0x43 */ {0x3E, 0x41, 0x41, 0x41, 0x22},
/* 0x44 */ {0x7F, 0x41, 0x41, 0x22, 0x1C},
/* 0x45 */ {0x7F, 0x49, 0x49, 0x49, 0x41},
/* 0x46 */ {0x7F, 0x09, 0x09, 0x09, 0x01},
/* 0x47 */ {0x3E, 0x41, 0x41, 0x49, 0x3A},
/* 0x48 */ {0x7F, 0x08, 0x08, 0x08, 0x7F},
/* 0x49 */ {0x00, 0x41, 0x7F, 0x41, 0x00},
/* 0x4A */ {0x20, 0x40, 0x41, 0x3F, 0x01},
/* 0x4B */ {0x7F, 0x08, 0x14, 0x22, 0x41},
/* 0x4C */ {0x7F, 0x40, 0x40, 0x40, 0x40},
/* 0x4D */ {0x7F, 0x02, 0x04, 0x02, 0x7F},
/* 0x4E */ {0x7F, 0x04, 0x08, 0x10, 0x7F},
/* 0x4F */ {0x3E, 0x41, 0x41, 0x41, 0x3E},
/* 0x50 */ {0x7F, 0x09, 0x09, 0x09, 0x06},
/* 0x51 */ {0x3E, 0x41, 0x51, 0x21, 0x5E},
/* 0x52 */ {0x7F, 0x09, 0x19, 0x29, 0x46},
/* 0x53 */ {0x26, 0x49, 0x49, 0x49, 0x32},
/* 0x54 */ {0x01, 0x01, 0x7F, 0x01, 0x01},
/* 0x55 */ {0x3F, 0x40, 0x40, 0x40, 0x3F},
/* 0x56 */ {0x1F, 0x20, 0x40, 0x20, 0x1F},
/* 0x57 */ {0x7F, 0x20, 0x18, 0x20, 0x7F},
/* 0x58 */ {0x63, 0x14, 0x08, 0x14, 0x63},
/* 0x59 */ {0x03, 0x04, 0x78, 0x04, 0x03},
/* 0x5A */ {0x61, 0x51, 0x49, 0x45, 0x43},
/* 0x5B */ {0x00, 0x7F, 0x41, 0x41, 0x00},
/* 0x5C */ {0x02, 0x04, 0x08, 0x10, 0x20},
/* 0x5D */ {0x00, 0x41, 0x41, 0x7F, 0x00},
/* 0x5E */ {0x04, 0x02, 0x01, 0x02, 0x04},
/* 0x5F */ {0x40, 0x40, 0x40, 0x40, 0x40},
/* 0x60 */ {0x00, 0x01, 0x02, 0x04, 0x00},
/* 0x61 */ {0x20, 0x54, 0x54, 0x54, 0x78},
/* 0x62 */ {0x7f, 0x48, 0x44, 0x44, 0x38},
/* 0x63 */ {0x30, 0x48, 0x48, 0x48, 0x20},
/* 0x64 */ {0x38, 0x44, 0x44, 0x48, 0x7f},
/* 0x65 */ {0x38, 0x54, 0x54, 0x54, 0x1c},
/* 0x66 */ {0x00, 0x08, 0x7e, 0x09, 0x09},
/* 0x67 */ {0x0c, 0x52, 0x52, 0x52, 0x3e},
/* 0x68 */ {0x7f, 0x08, 0x04, 0x04, 0x78},
/* 0x69 */ {0x00, 0x44, 0x7d, 0x40, 0x00},
/* 0x6A */ {0x20, 0x40, 0x40, 0x3d, 0x00},
/* 0x6B */ {0x7f, 0x10, 0x28, 0x44, 0x00},
/* 0x6C */ {0x00, 0x41, 0x7f, 0x40, 0x00},
/* 0x6D */ {0x7c, 0x04, 0x18, 0x04, 0x78},
/* 0x6E */ {0x7c, 0x08, 0x04, 0x04, 0x78},
/* 0x6F */ {0x38, 0x44, 0x44, 0x44, 0x38},
/* 0x70 */ {0xfc, 0x14, 0x14, 0x14, 0x08},
/* 0x71 */ {0x08, 0x14, 0x14, 0x18, 0x7c},
/* 0x72 */ {0x7c, 0x08, 0x04, 0x04, 0x08},
/* 0x73 */ {0x48, 0x54, 0x54, 0x54, 0x20},
/* 0x74 */ {0x04, 0x3f, 0x44, 0x40, 0x20},
/* 0x75 */ {0x3c, 0x40, 0x40, 0x20, 0x7c},
/* 0x76 */ {0x1c, 0x20, 0x40, 0x20, 0x1c},
/* 0x77 */ {0x3c, 0x40, 0x38, 0x40, 0x3c},
/* 0x78 */ {0x44, 0x28, 0x10, 0x28, 0x44},
/* 0x79 */ {0x0c, 0x50, 0x50, 0x50, 0x3c},
/* 0x7A */ {0x44, 0x64, 0x54, 0x4c, 0x44},
/* 0x7B */ {0x00, 0x08, 0x36, 0x41, 0x00},
/* 0x7C */ {0x00, 0x00, 0x7F, 0x00, 0x00},
/* 0x7D */ {0x00, 0x41, 0x36, 0x08, 0x00},
/* 0x7E */ {0x08, 0x06, 0x08, 0x30, 0x08},
#endif
/* 0x7F */ {0x55, 0xAA, 0x55, 0xAA, 0x55},
}};
static const uint8_t (*font)[FONT_WIDTH] = font_array.font;

uint32_t display_update_time = 0;
uint32_t display_update_complete_time = 0;
uint32_t display_auto_update_period = DEFAULT_UPDATE_PERIOD;

void
display_update(void)
{
  uint32_t now = get_sys_time();
  if (display_auto_update_period > 0)
    display_update_time = now + display_auto_update_period;
  nxt_lcd_update();
  display_update_complete_time = now + DISPLAY_UPDATE_TIME;
}

int display_set_auto_update_period(int period)
{
  // Set the period for auto updates, 0 disables auto updating
  int prev = (int) display_auto_update_period;
  if (period <= 0)
  {
    display_auto_update_period = 0;
    display_update_time = 0x7fffffff;
  }
  else
  {
    if (period < DISPLAY_UPDATE_TIME) period = DISPLAY_UPDATE_TIME;
    display_auto_update_period = period;
    display_update();
  }
  return prev;
}

int display_get_update_complete_time()
{
  return display_update_complete_time;
}


void display_force_update(void)
{
  // Force a display update even if interrupts are disabled
  nxt_lcd_force_update();
}


void
display_clear(uint32_t updateToo)
{
  //memset(display_buffer, 0, sizeof(display_buffer));
  memset(display_buffer, 0, DISPLAY_WIDTH*DISPLAY_DEPTH);
  if (updateToo)
    display_update();
}


int display_x;
int display_y;

/* text functions, top left is (0,0) */
void
display_goto_xy(int x, int y)
{
  display_x = x;
  display_y = y;
}

void
display_char(int c)
{
  uint8_t *b;
  const uint8_t *f, *fend;

  if ((uint) c < N_CHARS &&
      (uint) display_x < DISPLAY_CHAR_WIDTH &&
      (uint) display_y < DISPLAY_CHAR_DEPTH) {
    b = &display_buffer[display_y][display_x * CELL_WIDTH];
    f = font[c];
    fend = f + FONT_WIDTH;

    do {
      *b++ = *f++;
    } while( f < fend);
    // and now clear the inter char gap
    *b = 0;
  }
}

void
display_string(const char *str)
{
  while (*str) {
    if (*str != '\n') {
      display_char(*str);
      display_x++;
    } else {
      display_x = 0;
      display_y++;
    }
    str++;
  }
}


void
display_hex(uint32_t val, uint32_t places)
{
  char x[9];

  char *p = &x[8];
  int p_count = 0;


  *p = 0;

  if (places > 8)
    places = 8;

  while (val) {
    p--;
    p_count++;
    *p = "0123456789ABCDEF"[val & 0x0f];
    val >>= 4;
  }

  while (p_count < places) {
    p--;
    p_count++;
    *p = '0';
  }

  display_string(p);
}

static void
display_unsigned_worker(uint32_t val, uint32_t places, uint32_t sign)
{
  char x[12];			// enough for 10 digits + sign + NULL
  char *p = &x[11];
  int p_count = 0;
  uint32_t val0;

  *p = 0;

  if (places > 11)
    places = 11;

  while (val) {
    p--;
    p_count++;
    val0 = val;
    val /= 10;
    *p = (val0 - val * 10) + '0';
  }

  if (!p_count) {
    p--;
    p_count++;
    *p = '0';
  }

  if (sign) {
    p--;
    p_count++;
    *p = '-';
  }

  while (p_count < places) {
    p--;
    p_count++;
    *p = ' ';
  }

  display_string(p);
}

void
display_unsigned(uint32_t val, uint32_t places)
{
  display_unsigned_worker(val, places, 0);
}

void
display_int(int val, uint32_t places)
{
  display_unsigned_worker((val < 0) ? -val : val, places, (val < 0));
}

void
display_bitmap_copy(const uint8_t *data, uint32_t width, uint32_t depth, uint32_t x, uint32_t y)
{
  display_bitblt((uint8_t *)data, width, depth*8, 0, 0, (uint8_t *)display_buffer, DISPLAY_WIDTH, DISPLAY_DEPTH*8, x, y*8, width, depth*8, 0x0000ff00);
}

void display_bitblt(uint8_t *src, int sw, int sh, int sx, int sy, uint8_t *dst, int dw, int dh, int dx, int dy, int w, int h, int rop)
{
  /* This is a partial implementation of the BitBlt algorithm. It provides a
   * complete set of raster operations and handles partial and fully aligned
   * images correctly. Overlapping source and destination images is also
   * supported. It does not performing mirroring. The code was converted
   * from an initial Java implementation and has not been optimized for C.
   * The genral mechanism is to perform the block copy with Y as the inner
   * loop (because on the display the bits are packed y-wise into a byte). We
   * perform the various rop cases by reducing the operation to a series of
   * AND and XOR operations. Each step is controlled by a byte in the rop code.
   * This mechanism is based upon that used in the X Windows system server.
   */
  // Clip to source and destination
  int trim;
  if (dx < 0)
  {
    trim = -dx;
    dx = 0;
    sx += trim;
    w -= trim;
  }
  if (dy < 0)
  {
    trim = -dy;
    dy = 0;
    sy += trim;
    h -= trim;
  }
  if (sx < 0 || sy < 0) return;
  if (dx + w > dw) w = dw - dx;
  if (sx + w > sw) w = sw - sx;
  if (w <= 0) return;
  if (dy + h > dh) h = dh - dy;
  if (sy + h > sh) h = sh - sy;
  if (h <= 0) return;
  // Setup initial parameters and check for overlapping copy
  int xinc = 1;
  int yinc = 1;
  uint8_t firstBit = 1;
  if (src == dst)
  {
    // If copy overlaps we use reverse direction
    if (dy > sy)
    {
      sy = sy + h - 1;
      dy = dy + h - 1;
      firstBit = (uint8_t)0x80;
      yinc = -1;
    }
    if (dx > sx)
    {
      xinc = -1;
      sx = sx + w - 1;
      dx = dx + w - 1;
    }
  }
  int inStart = (sy/8)*sw;
  int outStart = (dy/8)*dw;
  uint8_t inStartBit = (uint8_t)(1 << (sy & 0x7));
  uint8_t outStartBit = (uint8_t)(1 << (dy & 0x7));
  dw *= yinc;
  sw *= yinc;
  // Extract rop sub-fields
  uint8_t ca1 = (uint8_t)(rop >> 24);
  uint8_t cx1 = (uint8_t)(rop >> 16);
  uint8_t ca2 = (uint8_t)(rop >> 8);
  uint8_t cx2 = (uint8_t) rop;
  unsigned char noDst = (ca1 == 0 && cx1 == 0);
  int xcnt;
  // Check for byte aligned case and optimise for it
  if (h >= 8 && inStartBit == firstBit && outStartBit == firstBit)
  {
    int ix = sx;
    int ox = dx;
    int byteCnt = h/8;
    xcnt = w;
    while (xcnt-- > 0)
    {
      int inIndex = inStart + ix;
      int outIndex = outStart + ox;
      int cnt = byteCnt;
      while(cnt-- > 0)
      {
        if (noDst)
          dst[outIndex] = (uint8_t)((src[inIndex] & ca2)^cx2);
        else
        {
          uint8_t inVal = src[inIndex];
          dst[outIndex] = (uint8_t)((dst[outIndex] & ((inVal & ca1)^cx1)) ^ ((inVal & ca2)^cx2));
        }
        outIndex += dw;
        inIndex += sw;
      }
      ox += xinc;
      ix += xinc;
    }
    // Do we have a final non byte multiple to do?
    h &= 0x7;
    if (h == 0) return;
    outStart += (byteCnt*dw);
    inStart += (byteCnt*sw);
  }
  // General non byte aligned case
  int ix = sx;
  int ox = dx;
  xcnt = w;
  while(xcnt-- > 0)
  {
    int inIndex = inStart + ix;
    uint8_t inBit = inStartBit;
    uint8_t inVal = src[inIndex];
    uint8_t inAnd = (uint8_t)((inVal & ca1)^cx1);
    uint8_t inXor = (uint8_t)((inVal & ca2)^cx2);
    int outIndex = outStart + ox;
    uint8_t outBit = outStartBit;
    uint8_t outPixels = dst[outIndex];
    int cnt = h;
    while(1)
    {
      if (noDst)
      {
        if ((inXor & inBit) != 0)
          outPixels |= outBit;
        else
          outPixels &= ~outBit;
      }
      else
      {
        uint8_t resBit = (uint8_t)((outPixels & ((inAnd & inBit) != 0 ? outBit : 0))^((inXor & inBit) != 0 ? outBit : 0));
        outPixels = (uint8_t)((outPixels & ~outBit) | resBit);
      }
      if (--cnt <= 0) break;
      if (yinc > 0)
      {
        inBit <<= 1;
        outBit <<= 1;
      }
      else
      {
        inBit >>= 1;
        outBit >>= 1;
      }
      if (inBit == 0)
      {
        inBit = firstBit;
        inIndex += sw;
        inVal = src[inIndex];
        inAnd = (uint8_t)((inVal & ca1)^cx1);
        inXor = (uint8_t)((inVal & ca2)^cx2);
      }
      if (outBit == 0)
      {
        dst[outIndex] = outPixels;
        outBit = firstBit;
        outIndex += dw;
        outPixels = dst[outIndex];
      }
    }
    dst[outIndex] = outPixels;
    ix += xinc;
    ox += xinc;
  }
}

uint8_t *
display_get_buffer(void)
{
  return (uint8_t *)display_buffer;
}

void
display_reset()
{
  nxt_lcd_enable(0);
  display_auto_update_period = DEFAULT_UPDATE_PERIOD;
  display_clear(0);
  nxt_lcd_set_pot(NXT_DEFAULT_CONTRAST);
  nxt_lcd_enable(1);
  display_update();
}

void
display_init(void)
{
  nxt_lcd_init((uint8_t *)display_buffer);
  display_reset();
}

