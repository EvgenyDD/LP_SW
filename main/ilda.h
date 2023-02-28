#ifndef __ILDA_H__
#define __ILDA_H__

#include <stdbool.h>
#include <stdint.h>

#pragma pack(1)
typedef struct
{
	char ilda_id_str[4];
	uint8_t res0[3];
	uint8_t format;
	char name_frame[8];
	char name_company[8];
	uint16_t point_count;
	uint16_t frame_num;
	uint16_t frame_count;
	uint8_t projector_num;
	uint8_t reserved2;
} ilda_header_t;
#pragma pack()

#pragma pack(1)
typedef struct
{
	int16_t x;
	int16_t y;
	int16_t z;
	uint8_t color[3];
	bool blanked;
	bool last_point;
} ilda_point_t;
#pragma pack()

typedef struct
{
	ilda_point_t *points;
	uint16_t point_count;
} ilda_frame_t;

typedef struct
{
	ilda_frame_t *frames;
	uint32_t frame_count;

	uint8_t *pallete;
	uint16_t pallete_size;
	bool pallete_present;

    uint8_t last_format;
} ilda_t;

int ilda_file_read(const char *data, uint32_t fsize, ilda_t *ilda, bool use_64_color_table);
void ilda_file_free(ilda_t *ilda);

#endif // __ILDA_H__