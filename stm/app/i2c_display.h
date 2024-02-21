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
void i2c_display_display_text(uint32_t row, uint32_t col, char *text, uint32_t text_len, bool invert);
void i2c_display_direction(uint32_t _d);
void i2c_display_clear(void);
void i2c_display_clear_screen(bool invert);
void i2c_display_clear_line(uint32_t row, bool invert);
void i2c_display_fadeout(void);

uint8_t i2c_display_rotate_byte(uint8_t ch1);
void i2c_display_invert(uint8_t *buf, size_t blen);
void i2c_display_rotate_image(uint8_t *buf, uint32_t dir);

void i2c_display_get_buffer(uint8_t *buffer);
void i2c_display_display_image(uint32_t page, uint32_t seg, uint8_t *images, uint32_t width);
void i2c_display_image(uint32_t page, uint32_t seg, uint8_t *images, uint32_t width);
void i2c_display_show_buffer(void);
void i2c_display_set_buffer(uint8_t *buffer);

#endif // __I2C_DISPLAY_H__