#ifndef __I2C_DISPLAY_H__
#define __I2C_DISPLAY_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DIRECTION0 0
#define DIRECTION90 1
#define DIRECTION180 2
#define DIRECTION270 3

void i2c_display_init(void);
void i2c_display_contrast(uint8_t contrast);
void i2c_display_display_text(int row, int col, char *text, int text_len, bool invert);
void i2c_display_direction(int _d);
void i2c_display_clear(void);
void i2c_display_clear_screen(bool invert);
void i2c_display_clear_line(int row, bool invert);
void i2c_display_fadeout(void);

uint8_t i2c_display_rotate_byte(uint8_t ch1);
void i2c_display_invert(uint8_t *buf, size_t blen);
void i2c_display_rotate_image(uint8_t *buf, int dir);

#endif // __I2C_DISPLAY_H__