#ifndef __ILDA_H__
#define __ILDA_H__

#include <stdbool.h>
#include <stdint.h>

#define ILDA_FILE_CHUNK 256

#define ILDA_FMT_3D_INDEXED 0
#define ILDA_FMT_2D_INDEXED 1
#define ILDA_FMT_PALETTE 2
#define ILDA_FMT_3D_TRUE 4
#define ILDA_FMT_2D_TRUE 5

enum
{
	ILDA_ERR_CHUNK_OVF = -10,
	ILDA_ERR_NO_ILDA_HDR,
	ILDA_ERR_UNKNWN_FMT,
	ILDA_ERR_EXTRA_DATA,
} ILDA_ERR_t;

typedef struct __attribute__((packed))
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

typedef struct __attribute__((packed))
{
	int16_t x;
	int16_t y;
	int16_t z;
	uint8_t color[3];
	bool blanked;
	bool last_point;
} ilda_point_t;

typedef struct __attribute__((packed))
{
	ilda_point_t *points;
	uint16_t point_count;
} ilda_frame_t;

typedef struct /*__attribute__((packed))*/
{
	ilda_frame_t *frames;
	uint32_t frame_count;
	uint32_t point_count;

	uint8_t *pallete;
	uint16_t pallete_size;
	bool pallete_present;

	uint32_t max_point_per_frame; // for debug
	int fpos;					  // debug

	ilda_header_t header;
	uint8_t chunk[ILDA_FILE_CHUNK];
	uint32_t chunk_pos;
	uint32_t processed_points;
	bool phase_frames;
} ilda_t;

// Video is made of frames. Each frame is a frame header + N*points

typedef struct __attribute__((packed))
{
	void *p_frame_next;		// pointer to the next lp_frame_h_t structure (w/ points)
	uint32_t repeat_cnt;	// frame repeat counter, 0 - eternal repeat
	uint32_t point_cnt;		// count of the points in the frame
	uint32_t point_cur;		// current processing point of the frame
	uint32_t frame_counter; // frame index number in the video
} lp_frame_h_t;

typedef struct __attribute__((packed))
{
	uint16_t time_us;
	uint16_t x;
	uint16_t y;
	uint16_t r;
	uint16_t g;
	uint16_t b;
} lp_point_t;

void ilda_file_init(ilda_t *ilda);
void ilda_file_free(ilda_t *ilda);
int ilda_file_parse_chunk(ilda_t *ilda, const uint8_t *buf, uint32_t size, bool use_64_color_table);
int ilda_file_parse_file(ilda_t *ilda, uint8_t *buf, uint32_t size, bool use_64_color_table);

#endif // __ILDA_H__