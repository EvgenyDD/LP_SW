#include "display.h"
#include "dev_drivers.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>

// https://www.displayfuture.com/Display/datasheet/controller/SH1107.pdf
// https://www.waveshare.com/w/upload/1/1b/Pico-OLED-1.3_SchDoc.pdf
// https://www.waveshare.com/pico-oled-1.3.htm

#define I2C_SH1107_ADDR 0x3C

/* Control byte for I2C

Co : bit 8 : Continuation Bit
 * 1 = no-continuation (only one byte to follow)
 * 0 = the controller should expect a stream of bytes.

D/C# : bit 7 : Data/Command Select bit
 * 1 = the next byte or byte stream will be Data.
 * 0 = a Command byte or byte stream will be coming up next.
 Bits 6-0 will be all zeros.

Usage:
0x80 : Single Command byte
0x00 : Command Stream
0xC0 : Single Data byte
0x40 : Data Stream
*/

#define I2C_CONTROL_BYTE_CMD_SINGLE 0x80
#define I2C_CONTROL_BYTE_CMD_STREAM 0x00
#define I2C_CONTROL_BYTE_DATA_SINGLE 0xC0
#define I2C_CONTROL_BYTE_DATA_STREAM 0x40

#define HEIGHT 128
#define WIDTH 64

typedef enum
{
	SCROLL_RIGHT = 1,
	SCROLL_LEFT = 2,
	SCROLL_DOWN = 3,
	SCROLL_UP = 4,
	SCROLL_STOP = 5
} sh1107_scroll_type_t;

static int _pages = HEIGHT / 8;
static int _direction = DIRECTION0;
static uint8_t disp_buf[16][64];

uint8_t font8x8_basic_tr[128][8] = {
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0000 (nul)
	{0x00, 0x04, 0x02, 0xFF, 0x02, 0x04, 0x00, 0x00}, // U+0001 (Up Allow)
	{0x00, 0x20, 0x40, 0xFF, 0x40, 0x20, 0x00, 0x00}, // U+0002 (Down Allow)
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0003
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0004
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0005
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0006
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0007
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0008
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0009
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000A
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000B
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000C
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000D
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000E
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000F
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0010
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0011
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0012
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0013
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0014
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0015
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0016
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0017
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0018
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0019
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001A
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001B
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001C
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001D
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001E
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001F
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0020 (space)
	{0x00, 0x00, 0x06, 0x5F, 0x5F, 0x06, 0x00, 0x00}, // U+0021 (!)
	{0x00, 0x03, 0x03, 0x00, 0x03, 0x03, 0x00, 0x00}, // U+0022 (")
	{0x14, 0x7F, 0x7F, 0x14, 0x7F, 0x7F, 0x14, 0x00}, // U+0023 (#)
	{0x24, 0x2E, 0x6B, 0x6B, 0x3A, 0x12, 0x00, 0x00}, // U+0024 ($)
	{0x46, 0x66, 0x30, 0x18, 0x0C, 0x66, 0x62, 0x00}, // U+0025 (%)
	{0x30, 0x7A, 0x4F, 0x5D, 0x37, 0x7A, 0x48, 0x00}, // U+0026 (&)
	{0x04, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0027 (')
	{0x00, 0x1C, 0x3E, 0x63, 0x41, 0x00, 0x00, 0x00}, // U+0028 (()
	{0x00, 0x41, 0x63, 0x3E, 0x1C, 0x00, 0x00, 0x00}, // U+0029 ())
	{0x08, 0x2A, 0x3E, 0x1C, 0x1C, 0x3E, 0x2A, 0x08}, // U+002A (*)
	{0x08, 0x08, 0x3E, 0x3E, 0x08, 0x08, 0x00, 0x00}, // U+002B (+)
	{0x00, 0x80, 0xE0, 0x60, 0x00, 0x00, 0x00, 0x00}, // U+002C (,)
	{0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00}, // U+002D (-)
	{0x00, 0x00, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00}, // U+002E (.)
	{0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00}, // U+002F (/)
	{0x3E, 0x7F, 0x71, 0x59, 0x4D, 0x7F, 0x3E, 0x00}, // U+0030 (0)
	{0x40, 0x42, 0x7F, 0x7F, 0x40, 0x40, 0x00, 0x00}, // U+0031 (1)
	{0x62, 0x73, 0x59, 0x49, 0x6F, 0x66, 0x00, 0x00}, // U+0032 (2)
	{0x22, 0x63, 0x49, 0x49, 0x7F, 0x36, 0x00, 0x00}, // U+0033 (3)
	{0x18, 0x1C, 0x16, 0x53, 0x7F, 0x7F, 0x50, 0x00}, // U+0034 (4)
	{0x27, 0x67, 0x45, 0x45, 0x7D, 0x39, 0x00, 0x00}, // U+0035 (5)
	{0x3C, 0x7E, 0x4B, 0x49, 0x79, 0x30, 0x00, 0x00}, // U+0036 (6)
	{0x03, 0x03, 0x71, 0x79, 0x0F, 0x07, 0x00, 0x00}, // U+0037 (7)
	{0x36, 0x7F, 0x49, 0x49, 0x7F, 0x36, 0x00, 0x00}, // U+0038 (8)
	{0x06, 0x4F, 0x49, 0x69, 0x3F, 0x1E, 0x00, 0x00}, // U+0039 (9)
	{0x00, 0x00, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00}, // U+003A (:)
	{0x00, 0x80, 0xE6, 0x66, 0x00, 0x00, 0x00, 0x00}, // U+003B (;)
	{0x08, 0x1C, 0x36, 0x63, 0x41, 0x00, 0x00, 0x00}, // U+003C (<)
	{0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x00, 0x00}, // U+003D (=)
	{0x00, 0x41, 0x63, 0x36, 0x1C, 0x08, 0x00, 0x00}, // U+003E (>)
	{0x02, 0x03, 0x51, 0x59, 0x0F, 0x06, 0x00, 0x00}, // U+003F (?)
	{0x3E, 0x7F, 0x41, 0x5D, 0x5D, 0x1F, 0x1E, 0x00}, // U+0040 (@)
	{0x7C, 0x7E, 0x13, 0x13, 0x7E, 0x7C, 0x00, 0x00}, // U+0041 (A)
	{0x41, 0x7F, 0x7F, 0x49, 0x49, 0x7F, 0x36, 0x00}, // U+0042 (B)
	{0x1C, 0x3E, 0x63, 0x41, 0x41, 0x63, 0x22, 0x00}, // U+0043 (C)
	{0x41, 0x7F, 0x7F, 0x41, 0x63, 0x3E, 0x1C, 0x00}, // U+0044 (D)
	{0x41, 0x7F, 0x7F, 0x49, 0x5D, 0x41, 0x63, 0x00}, // U+0045 (E)
	{0x41, 0x7F, 0x7F, 0x49, 0x1D, 0x01, 0x03, 0x00}, // U+0046 (F)
	{0x1C, 0x3E, 0x63, 0x41, 0x51, 0x73, 0x72, 0x00}, // U+0047 (G)
	{0x7F, 0x7F, 0x08, 0x08, 0x7F, 0x7F, 0x00, 0x00}, // U+0048 (H)
	{0x00, 0x41, 0x7F, 0x7F, 0x41, 0x00, 0x00, 0x00}, // U+0049 (I)
	{0x30, 0x70, 0x40, 0x41, 0x7F, 0x3F, 0x01, 0x00}, // U+004A (J)
	{0x41, 0x7F, 0x7F, 0x08, 0x1C, 0x77, 0x63, 0x00}, // U+004B (K)
	{0x41, 0x7F, 0x7F, 0x41, 0x40, 0x60, 0x70, 0x00}, // U+004C (L)
	{0x7F, 0x7F, 0x0E, 0x1C, 0x0E, 0x7F, 0x7F, 0x00}, // U+004D (M)
	{0x7F, 0x7F, 0x06, 0x0C, 0x18, 0x7F, 0x7F, 0x00}, // U+004E (N)
	{0x1C, 0x3E, 0x63, 0x41, 0x63, 0x3E, 0x1C, 0x00}, // U+004F (O)
	{0x41, 0x7F, 0x7F, 0x49, 0x09, 0x0F, 0x06, 0x00}, // U+0050 (P)
	{0x1E, 0x3F, 0x21, 0x71, 0x7F, 0x5E, 0x00, 0x00}, // U+0051 (Q)
	{0x41, 0x7F, 0x7F, 0x09, 0x19, 0x7F, 0x66, 0x00}, // U+0052 (R)
	{0x26, 0x6F, 0x4D, 0x59, 0x73, 0x32, 0x00, 0x00}, // U+0053 (S)
	{0x03, 0x41, 0x7F, 0x7F, 0x41, 0x03, 0x00, 0x00}, // U+0054 (T)
	{0x7F, 0x7F, 0x40, 0x40, 0x7F, 0x7F, 0x00, 0x00}, // U+0055 (U)
	{0x1F, 0x3F, 0x60, 0x60, 0x3F, 0x1F, 0x00, 0x00}, // U+0056 (V)
	{0x7F, 0x7F, 0x30, 0x18, 0x30, 0x7F, 0x7F, 0x00}, // U+0057 (W)
	{0x43, 0x67, 0x3C, 0x18, 0x3C, 0x67, 0x43, 0x00}, // U+0058 (X)
	{0x07, 0x4F, 0x78, 0x78, 0x4F, 0x07, 0x00, 0x00}, // U+0059 (Y)
	{0x47, 0x63, 0x71, 0x59, 0x4D, 0x67, 0x73, 0x00}, // U+005A (Z)
	{0x00, 0x7F, 0x7F, 0x41, 0x41, 0x00, 0x00, 0x00}, // U+005B ([)
	{0x01, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x00}, // U+005C (\)
	{0x00, 0x41, 0x41, 0x7F, 0x7F, 0x00, 0x00, 0x00}, // U+005D (])
	{0x08, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x08, 0x00}, // U+005E (^)
	{0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, // U+005F (_)
	{0x00, 0x00, 0x03, 0x07, 0x04, 0x00, 0x00, 0x00}, // U+0060 (`)
	{0x20, 0x74, 0x54, 0x54, 0x3C, 0x78, 0x40, 0x00}, // U+0061 (a)
	{0x41, 0x7F, 0x3F, 0x48, 0x48, 0x78, 0x30, 0x00}, // U+0062 (b)
	{0x38, 0x7C, 0x44, 0x44, 0x6C, 0x28, 0x00, 0x00}, // U+0063 (c)
	{0x30, 0x78, 0x48, 0x49, 0x3F, 0x7F, 0x40, 0x00}, // U+0064 (d)
	{0x38, 0x7C, 0x54, 0x54, 0x5C, 0x18, 0x00, 0x00}, // U+0065 (e)
	{0x48, 0x7E, 0x7F, 0x49, 0x03, 0x02, 0x00, 0x00}, // U+0066 (f)
	{0x98, 0xBC, 0xA4, 0xA4, 0xF8, 0x7C, 0x04, 0x00}, // U+0067 (g)
	{0x41, 0x7F, 0x7F, 0x08, 0x04, 0x7C, 0x78, 0x00}, // U+0068 (h)
	{0x00, 0x44, 0x7D, 0x7D, 0x40, 0x00, 0x00, 0x00}, // U+0069 (i)
	{0x60, 0xE0, 0x80, 0x80, 0xFD, 0x7D, 0x00, 0x00}, // U+006A (j)
	{0x41, 0x7F, 0x7F, 0x10, 0x38, 0x6C, 0x44, 0x00}, // U+006B (k)
	{0x00, 0x41, 0x7F, 0x7F, 0x40, 0x00, 0x00, 0x00}, // U+006C (l)
	{0x7C, 0x7C, 0x18, 0x38, 0x1C, 0x7C, 0x78, 0x00}, // U+006D (m)
	{0x7C, 0x7C, 0x04, 0x04, 0x7C, 0x78, 0x00, 0x00}, // U+006E (n)
	{0x38, 0x7C, 0x44, 0x44, 0x7C, 0x38, 0x00, 0x00}, // U+006F (o)
	{0x84, 0xFC, 0xF8, 0xA4, 0x24, 0x3C, 0x18, 0x00}, // U+0070 (p)
	{0x18, 0x3C, 0x24, 0xA4, 0xF8, 0xFC, 0x84, 0x00}, // U+0071 (q)
	{0x44, 0x7C, 0x78, 0x4C, 0x04, 0x1C, 0x18, 0x00}, // U+0072 (r)
	{0x48, 0x5C, 0x54, 0x54, 0x74, 0x24, 0x00, 0x00}, // U+0073 (s)
	{0x00, 0x04, 0x3E, 0x7F, 0x44, 0x24, 0x00, 0x00}, // U+0074 (t)
	{0x3C, 0x7C, 0x40, 0x40, 0x3C, 0x7C, 0x40, 0x00}, // U+0075 (u)
	{0x1C, 0x3C, 0x60, 0x60, 0x3C, 0x1C, 0x00, 0x00}, // U+0076 (v)
	{0x3C, 0x7C, 0x70, 0x38, 0x70, 0x7C, 0x3C, 0x00}, // U+0077 (w)
	{0x44, 0x6C, 0x38, 0x10, 0x38, 0x6C, 0x44, 0x00}, // U+0078 (x)
	{0x9C, 0xBC, 0xA0, 0xA0, 0xFC, 0x7C, 0x00, 0x00}, // U+0079 (y)
	{0x4C, 0x64, 0x74, 0x5C, 0x4C, 0x64, 0x00, 0x00}, // U+007A (z)
	{0x08, 0x08, 0x3E, 0x77, 0x41, 0x41, 0x00, 0x00}, // U+007B ({)
	{0x00, 0x00, 0x00, 0x77, 0x77, 0x00, 0x00, 0x00}, // U+007C (|)
	{0x41, 0x41, 0x77, 0x3E, 0x08, 0x08, 0x00, 0x00}, // U+007D (})
	{0x02, 0x03, 0x01, 0x03, 0x02, 0x03, 0x01, 0x00}, // U+007E (~)
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // U+007F
};

void display_init(void)
{
	if(xSemaphoreTake(g_i2c_mutex, portMAX_DELAY))
	{
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();

		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (I2C_SH1107_ADDR << 1) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte(cmd, I2C_CONTROL_BYTE_CMD_STREAM, true);
		i2c_master_write_byte(cmd, 0xAE, true); // Turn display off
		i2c_master_write_byte(cmd, 0xDC, true); // Set display start line
		i2c_master_write_byte(cmd, 0x00, true); // ...value
		i2c_master_write_byte(cmd, 0x81, true); // Set display contrast
		i2c_master_write_byte(cmd, 0x2F, true); // ...value
		i2c_master_write_byte(cmd, 0x20, true); // Set memory mode
		i2c_master_write_byte(cmd, 0xA0, true); // Non-rotated display
		i2c_master_write_byte(cmd, 0xC0, true); // Non-flipped vertical
		i2c_master_write_byte(cmd, 0xA8, true); // Set multiplex ratio
		i2c_master_write_byte(cmd, 0x7F, true); // ...value
		i2c_master_write_byte(cmd, 0xD3, true); // Set display offset to zero
		i2c_master_write_byte(cmd, 0x60, true); // ...value
		i2c_master_write_byte(cmd, 0xD5, true); // Set display clock divider
		i2c_master_write_byte(cmd, 0x51, true); // ...value
		i2c_master_write_byte(cmd, 0xD9, true); // Set pre-charge
		i2c_master_write_byte(cmd, 0x22, true); // ...value
		i2c_master_write_byte(cmd, 0xDB, true); // Set com detect
		i2c_master_write_byte(cmd, 0x35, true); // ...value
		i2c_master_write_byte(cmd, 0xB0, true); // Set page address
		i2c_master_write_byte(cmd, 0xDA, true); // Set com pins
		i2c_master_write_byte(cmd, 0x12, true); // ...value
		i2c_master_write_byte(cmd, 0xA4, true); // output ram to display
		i2c_master_write_byte(cmd, 0xA6, true); // Non-inverted display
		// i2c_master_write_byte(cmd, 0xA7, true);	// Inverted display
		i2c_master_write_byte(cmd, 0xAF, true); // Turn display on

		i2c_master_stop(cmd);

		esp_err_t espRc = i2c_master_cmd_begin(0, cmd, 10 / portTICK_PERIOD_MS);
		if(espRc == ESP_OK)
		{
			ESP_LOGI("", "OLED configured successfully");
		}
		else
		{
			ESP_LOGE("", "OLED configuration failed. code: 0x%.2X", espRc);
		}
		i2c_cmd_link_delete(cmd);
		xSemaphoreGive(g_i2c_mutex);
	}
}

void display_image(int page, int seg, uint8_t *images, int width)
{
	i2c_cmd_handle_t cmd;

	if(page >= HEIGHT / 8) return;
	if(seg >= WIDTH) return;

	if(xSemaphoreTake(g_i2c_mutex, portMAX_DELAY))
	{
		uint8_t columLow = seg & 0x0F;
		uint8_t columHigh = (seg >> 4) & 0x0F;

		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (I2C_SH1107_ADDR << 1) | I2C_MASTER_WRITE, true);

		i2c_master_write_byte(cmd, I2C_CONTROL_BYTE_CMD_STREAM, true);
		// Set Higher Column Start Address for Page Addressing Mode
		i2c_master_write_byte(cmd, (0x10 + columHigh), true);
		// Set Lower Column Start Address for Page Addressing Mode
		i2c_master_write_byte(cmd, (0x00 + columLow), true);
		// Set Page Start Address for Page Addressing Mode
		i2c_master_write_byte(cmd, 0xB0 | page, true);

		i2c_master_stop(cmd);
		i2c_master_cmd_begin(0, cmd, 10 / portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);

		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (I2C_SH1107_ADDR << 1) | I2C_MASTER_WRITE, true);

		i2c_master_write_byte(cmd, I2C_CONTROL_BYTE_DATA_STREAM, true);
		i2c_master_write(cmd, images, width, true);

		i2c_master_stop(cmd);
		i2c_master_cmd_begin(0, cmd, 10 / portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
		xSemaphoreGive(g_i2c_mutex);
	}
}

void display_contrast(uint8_t contrast)
{
	if(xSemaphoreTake(g_i2c_mutex, portMAX_DELAY))
	{
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (I2C_SH1107_ADDR << 1) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte(cmd, I2C_CONTROL_BYTE_CMD_STREAM, true);
		i2c_master_write_byte(cmd, 0x81, true); // Set Contrast Control Register
		i2c_master_write_byte(cmd, contrast, true);
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(0, cmd, 10 / portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
		xSemaphoreGive(g_i2c_mutex);
	}
}

void display_show_buffer(void)
{
	for(int page = 0; page < _pages; page++)
	{
		display_image(page, 0, disp_buf[page], WIDTH);
	}
}

void display_set_buffer(uint8_t *buffer)
{
	int index = 0;
	for(int page = 0; page < _pages; page++)
	{
		memcpy(disp_buf[page], &buffer[index], 64);
		index = index + 64;
	}
}

void display_get_buffer(uint8_t *buffer)
{
	int index = 0;
	for(int page = 0; page < _pages; page++)
	{
		memcpy(&buffer[index], disp_buf[page], 64);
		index = index + 64;
	}
}

void display_display_image(int page, int seg, uint8_t *images, int width)
{
	display_image(page, seg, images, width);
	memcpy(&disp_buf[page][seg], images, width);
}

void display_display_text(int row, int col, char *text, int text_len, bool invert)
{
	int _length = text_len;
	int w = 0;
	int h = 0;
	int _row = 0;
	int _col = 0;
	int _rowadd = 0;
	int _coladd = 0;
	if(_direction == DIRECTION0)
	{
		w = WIDTH / 8;
		h = HEIGHT / 8;
		_row = row;
		_col = col * 8;
		_rowadd = 0;
		_coladd = 8;
	}
	else if(_direction == DIRECTION90)
	{
		w = HEIGHT / 8;
		h = WIDTH / 8;
		_row = col;
		_col = (WIDTH - 8) - (row * 8);
		_rowadd = 1;
		_coladd = 0;
	}
	else if(_direction == DIRECTION180)
	{
		w = WIDTH / 8;
		h = HEIGHT / 8;
		_row = h - row - 1;
		_col = (WIDTH - 8) - (col * 8);
		_rowadd = 0;
		_coladd = -8;
	}
	else if(_direction == DIRECTION270)
	{
		w = HEIGHT / 8;
		h = WIDTH / 8;
		_row = (_pages - 1) - col;
		_col = row * 8;
		_rowadd = -1;
		_coladd = 0;
	}
	if(row >= h) return;
	if(col >= w) return;
	if(col + text_len > w) _length = w - col;
	ESP_LOGD("", "_direction=%d WIDTH=%d HEIGHT=%d _length=%d _row=%d _col=%d", _direction, w, h, _length, _row, _col);

	uint8_t image[8];
	for(int i = 0; i < _length; i++)
	{
		memcpy(image, font8x8_basic_tr[(uint8_t)text[i]], 8);
		if(invert) display_invert(image, 8);
		display_rotate_image(image, _direction);
		display_display_image(_row, _col, image, 8);
		_row = _row + _rowadd;
		_col = _col + _coladd;
	}
}

void display_clear_screen(bool invert)
{
	uint8_t zero[64];
	memset(zero, invert ? 0xff : 0, sizeof(zero));
	for(int page = 0; page < _pages; page++)
	{
		memcpy(&disp_buf[page][0], zero, 64);
		display_image(page, 0, zero, WIDTH);
	}
}

void display_clear_line(int row, bool invert)
{
	char space[1];
	space[0] = 0x20;
	if(_direction == DIRECTION0 || _direction == DIRECTION90)
	{
		for(int col = 0; col < WIDTH / 8; col++)
		{
			display_display_text(row, col, space, 1, invert);
		}
	}
	else
	{
		for(int col = 0; col < HEIGHT / 8; col++)
		{
			display_display_text(row, col, space, 1, invert);
		}
	}
}

// delay = 0 : display with no wait
// delay > 0 : display with wait
// delay < 0 : no display
void display_wrap_arround(sh1107_scroll_type_t scroll, int start, int end, int8_t delay)
{
	if(scroll == SCROLL_RIGHT)
	{
		int _start = start; // 0 to 15
		int _end = end;		// 0 to 15
		if(_end >= _pages) _end = _pages - 1;
		uint8_t wk;
		// for (int page=0;page<_pages;page++) {
		for(int page = _start; page <= _end; page++)
		{
			wk = disp_buf[page][63];
			for(int seg = 63; seg > 0; seg--)
			{
				disp_buf[page][seg] = disp_buf[page][seg - 1];
			}
			disp_buf[page][0] = wk;
		}
	}
	else if(scroll == SCROLL_LEFT)
	{
		int _start = start; // 0 to 15
		int _end = end;		// 0 to 15
		if(_end >= _pages) _end = _pages - 1;
		uint8_t wk;
		// for (int page=0;page<_pages;page++) {
		for(int page = _start; page <= _end; page++)
		{
			wk = disp_buf[page][0];
			for(int seg = 0; seg < 63; seg++)
			{
				disp_buf[page][seg] = disp_buf[page][seg + 1];
			}
			disp_buf[page][63] = wk;
		}
	}
	else if(scroll == SCROLL_UP)
	{
		int _start = start; // 0 to {width-1}
		int _end = end;		// 0 to {width-1}
		if(_end >= WIDTH) _end = WIDTH - 1;
		uint8_t wk0;
		uint8_t wk1;
		uint8_t wk2;
		uint8_t save[64];
		// Save pages 0
		for(int seg = 0; seg < 64; seg++)
		{
			save[seg] = disp_buf[0][seg];
		}
		// Page0 to Page15
		for(int page = 0; page < _pages - 1; page++)
		{
			// for (int seg=0;seg<64;seg++) {
			for(int seg = _start; seg <= _end; seg++)
			{
				wk0 = disp_buf[page][seg];
				wk1 = disp_buf[page + 1][seg];
				if(seg == 0)
				{
					ESP_LOGD("", "b page=%d wk0=%02x wk1=%02x", page, wk0, wk1);
				}
				wk0 = wk0 >> 1;
				wk1 = wk1 & 0x01;
				wk1 = wk1 << 7;
				wk2 = wk0 | wk1;
				if(seg == 0)
				{
					ESP_LOGD("", "a page=%d wk0=%02x wk1=%02x wk2=%02x", page, wk0, wk1, wk2);
				}
				disp_buf[page][seg] = wk2;
			}
		}
		// Page15
		int pages = _pages - 1;
		// for (int seg=0;seg<64;seg++) {
		for(int seg = _start; seg <= _end; seg++)
		{
			wk0 = disp_buf[pages][seg];
			wk1 = save[seg];
			wk0 = wk0 >> 1;
			wk1 = wk1 & 0x01;
			wk1 = wk1 << 7;
			wk2 = wk0 | wk1;
			disp_buf[pages][seg] = wk2;
		}
	}
	else if(scroll == SCROLL_DOWN)
	{
		int _start = start; // 0 to {width-1}
		int _end = end;		// 0 to {width-1}
		if(_end >= WIDTH) _end = WIDTH - 1;
		uint8_t wk0;
		uint8_t wk1;
		uint8_t wk2;
		uint8_t save[64];
		// Save pages 15
		int pages = _pages - 1;
		for(int seg = 0; seg < 64; seg++)
		{
			save[seg] = disp_buf[pages][seg];
		}
		// Page15 to Page1
		for(int page = pages; page > 0; page--)
		{
			// for (int seg=0;seg<64;seg++) {
			for(int seg = _start; seg <= _end; seg++)
			{
				wk0 = disp_buf[page][seg];
				wk1 = disp_buf[page - 1][seg];
				if(seg == 0)
				{
					ESP_LOGD("", "b page=%d wk0=%02x wk1=%02x", page, wk0, wk1);
				}
				wk0 = wk0 << 1;
				wk1 = wk1 & 0x80;
				wk1 = wk1 >> 7;
				wk2 = wk0 | wk1;
				if(seg == 0)
				{
					ESP_LOGD("", "a page=%d wk0=%02x wk1=%02x wk2=%02x", page, wk0, wk1, wk2);
				}
				disp_buf[page][seg] = wk2;
			}
		}
		// Page0
		for(int seg = _start; seg <= _end; seg++)
		{
			wk0 = disp_buf[0][seg];
			wk1 = save[seg];
			wk0 = wk0 << 1;
			wk1 = wk1 & 0x80;
			wk1 = wk1 >> 7;
			wk2 = wk0 | wk1;
			disp_buf[0][seg] = wk2;
		}
	}

	if(delay >= 0)
	{
		for(int page = 0; page < _pages; page++)
		{
			display_image(page, 0, disp_buf[page], 64);
			if(delay) vTaskDelay(delay);
		}
	}
}

void display_bitmaps(int xpos, int ypos, uint8_t *bitmap, int width, int height, bool invert)
{
	if((width % 8) != 0)
	{
		ESP_LOGE("", "width must be a multiple of 8");
		return;
	}
	if((xpos % 8) != 0)
	{
		ESP_LOGE("", "xpos must be a multiple of 8");
		return;
	}
	int w = width / 8;
	// uint8_t wk0;
	uint8_t wk1;
	int _seg = 63 - ypos;
	int offset = 0;
	for(int h = 0; h < height; h++)
	{
		int page = (xpos / 8);
		for(int index = 0; index < w; index++)
		{
			// wk0 = disp_buf[page][_seg];
			wk1 = bitmap[index + offset];
			wk1 = display_rotate_byte(wk1);
			if(invert) wk1 = ~wk1;
			disp_buf[page][_seg] = wk1;
			page++;
		}
		vTaskDelay(1);
		offset = offset + w;
		_seg--;
	}

	display_show_buffer();
}

void display_invert(uint8_t *buf, size_t blen)
{
	uint8_t wk;
	for(int i = 0; i < blen; i++)
	{
		wk = buf[i];
		buf[i] = ~wk;
	}
}

uint8_t display_copy_bit(uint8_t src, int srcBits, uint8_t dst, int dstBits)
{
	ESP_LOGD("", "src=%02x srcBits=%d dst=%02x dstBits=%d", src, srcBits, dst, dstBits);
	uint8_t smask = 0x01 << srcBits;
	uint8_t dmask = 0x01 << dstBits;
	uint8_t _src = src & smask;
#if 0
	if (_src != 0) _src = 1;
	uint8_t _wk = _src << dstBits;
	uint8_t _dst = dst | _wk;
#endif
	uint8_t _dst;
	if(_src != 0)
	{
		_dst = dst | dmask; // set bit
	}
	else
	{
		_dst = dst & ~(dmask); // clear bit
	}
	return _dst;
}

// Rotate 8-bit data
// 0x12-->0x48
uint8_t display_rotate_byte(uint8_t ch1)
{
	uint8_t ch2 = 0;
	for(int j = 0; j < 8; j++)
	{
		ch2 = (ch2 << 1) + (ch1 & 0x01);
		ch1 = ch1 >> 1;
	}
	return ch2;
}

void display_rotate_image(uint8_t *buf, int dir)
{
	uint8_t wk[8];
	if(dir == DIRECTION0) return;
	for(int i = 0; i < 8; i++)
	{
		wk[i] = buf[i];
		buf[i] = 0;
	}
	if(dir == DIRECTION90 || dir == DIRECTION270)
	{
		uint8_t wk2[8];
		uint8_t mask1 = 0x80;
		for(int i = 0; i < 8; i++)
		{
			wk2[i] = 0;
			ESP_LOGD("", "wk[%d]=%x", i, wk[i]);
			uint8_t mask2 = 0x01;
			for(int j = 0; j < 8; j++)
			{
				if((wk[j] & mask1) == mask1) wk2[i] = wk2[i] + mask2;
				mask2 = mask2 << 1;
			}
			mask1 = mask1 >> 1;
		}
		for(int i = 0; i < 8; i++)
		{
			ESP_LOGD("", "wk2[%d]=%x", i, wk2[i]);
		}

		for(int i = 0; i < 8; i++)
		{
			if(dir == DIRECTION90)
			{
				buf[i] = wk2[i];
			}
			else
			{
				buf[i] = display_rotate_byte(wk2[7 - i]);
			}
		}
	}
	else if(dir == DIRECTION180)
	{
		for(int i = 0; i < 8; i++)
		{
			buf[i] = display_rotate_byte(wk[7 - i]);
		}
	}
	return;
}

void display_fadeout(void)
{
	uint8_t image[1];
	for(int page = 0; page < _pages; page++)
	{
		image[0] = 0xFF;
		for(int line = 0; line < 8; line++)
		{
			image[0] = image[0] << 1;
			for(int seg = 0; seg < WIDTH; seg++)
			{
				display_image(page, seg, image, 1);
			}
		}
	}
}

void display_direction(int _d) { _direction = _d; }
