#ifndef LP_H
#define LP_H

#include <stdbool.h>
#include <stdint.h>

typedef struct __attribute__((packed))
{
	void *p_frame_next;
	uint32_t repeat_counter;
	uint32_t point_count;
	uint32_t point_current;
} lframe_header_t;

typedef struct __attribute__((packed))
{
	uint16_t time_us;
	uint16_t x_flipped;
	uint16_t y_flipped;
	uint16_t r_flipped;
	uint16_t g_flipped;
	uint16_t b_flipped;
} s;

#endif // LP_H