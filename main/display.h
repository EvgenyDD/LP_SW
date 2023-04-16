#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DIRECTION0 0
#define DIRECTION90 1
#define DIRECTION180 2
#define DIRECTION270 3

void display_init(void);
void display_contrast(uint8_t contrast);
void display_display_text(int row, int col, char *text, int text_len, bool invert);
void display_direction(int _d);
void display_clear_screen(bool invert);
void display_clear_line(int row, bool invert);
void display_fadeout(void);

uint8_t display_rotate_byte(uint8_t ch1);
void display_invert(uint8_t *buf, size_t blen);
void display_rotate_image(uint8_t *buf, int dir);

#endif // __DISPLAY_H__