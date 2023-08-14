#include "ilda.h"
#include <arpa/inet.h>
// #include "winsock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__attribute__((weak)) void cb_ilda_point(ilda_t *ilda, const ilda_point_t *p) {}
__attribute__((weak)) void cb_ilda_frame(ilda_t *ilda, uint32_t count_points) {}

static const char ilda_format_chunk_sz[] = {
	3 * sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t),	  // 3D INDEXED
	2 * sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t),	  // 2D INDEXED
	3 * sizeof(uint8_t),										  // PALETTE
	3,															  // WTF is that format? no specs....
	3 * sizeof(uint16_t) + sizeof(uint8_t) + 3 * sizeof(uint8_t), // 3D TRUE
	2 * sizeof(uint16_t) + sizeof(uint8_t) + 3 * sizeof(uint8_t), // 2D TRUE
};

static const uint8_t ilda_pallete_64_default[][3] = {
	{255, 0, 0}, // Red
	{255, 16, 0},
	{255, 32, 0},
	{255, 48, 0},
	{255, 64, 0},
	{255, 80, 0},
	{255, 96, 0},
	{255, 112, 0},
	{255, 128, 0},
	{255, 144, 0},
	{255, 160, 0},
	{255, 176, 0},
	{255, 192, 0},
	{255, 208, 0},
	{255, 224, 0},
	{255, 240, 0},
	{255, 255, 0}, // Yellow
	{224, 255, 0},
	{192, 255, 0},
	{160, 255, 0},
	{128, 255, 0},
	{96, 255, 0},
	{64, 255, 0},
	{32, 255, 0},
	{0, 255, 0}, // Green
	{0, 255, 36},
	{0, 255, 73},
	{0, 255, 109},
	{0, 255, 146},
	{0, 255, 182},
	{0, 255, 219},
	{0, 255, 255}, // Cyan
	{0, 227, 255},
	{0, 198, 255},
	{0, 170, 255},
	{0, 142, 255},
	{0, 113, 255},
	{0, 85, 255},
	{0, 56, 255},
	{0, 28, 255},
	{0, 0, 255}, // Blue
	{32, 0, 255},
	{64, 0, 255},
	{96, 0, 255},
	{128, 0, 255},
	{160, 0, 255},
	{192, 0, 255},
	{224, 0, 255},
	{255, 0, 255}, // Magenta
	{255, 32, 255},
	{255, 64, 255},
	{255, 96, 255},
	{255, 128, 255},
	{255, 160, 255},
	{255, 192, 255},
	{255, 224, 255},
	{255, 255, 255}, // White
	{255, 224, 224},
	{255, 192, 192},
	{255, 160, 160},
	{255, 128, 128},
	{255, 96, 96},
	{255, 64, 64},
	{255, 32, 32},
};

static const uint8_t ilda_pallete_256_default[][3] = {
	{0, 0, 0},		 // Black/blanked (fixed)
	{255, 255, 255}, // White (fixed)
	{255, 0, 0},	 // Red (fixed)
	{255, 255, 0},	 // Yellow (fixed)
	{0, 255, 0},	 // Green (fixed)
	{0, 255, 255},	 // Cyan (fixed)
	{0, 0, 255},	 // Blue (fixed)
	{255, 0, 255},	 // Magenta (fixed)
	{255, 128, 128}, // Light red
	{255, 140, 128},
	{255, 151, 128},
	{255, 163, 128},
	{255, 174, 128},
	{255, 186, 128},
	{255, 197, 128},
	{255, 209, 128},
	{255, 220, 128},
	{255, 232, 128},
	{255, 243, 128},
	{255, 255, 128}, // Light yellow
	{243, 255, 128},
	{232, 255, 128},
	{220, 255, 128},
	{209, 255, 128},
	{197, 255, 128},
	{186, 255, 128},
	{174, 255, 128},
	{163, 255, 128},
	{151, 255, 128},
	{140, 255, 128},
	{128, 255, 128}, // Light green
	{128, 255, 140},
	{128, 255, 151},
	{128, 255, 163},
	{128, 255, 174},
	{128, 255, 186},
	{128, 255, 197},
	{128, 255, 209},
	{128, 255, 220},
	{128, 255, 232},
	{128, 255, 243},
	{128, 255, 255}, // Light cyan
	{128, 243, 255},
	{128, 232, 255},
	{128, 220, 255},
	{128, 209, 255},
	{128, 197, 255},
	{128, 186, 255},
	{128, 174, 255},
	{128, 163, 255},
	{128, 151, 255},
	{128, 140, 255},
	{128, 128, 255}, // Light blue
	{140, 128, 255},
	{151, 128, 255},
	{163, 128, 255},
	{174, 128, 255},
	{186, 128, 255},
	{197, 128, 255},
	{209, 128, 255},
	{220, 128, 255},
	{232, 128, 255},
	{243, 128, 255},
	{255, 128, 255}, // Light magenta
	{255, 128, 243},
	{255, 128, 232},
	{255, 128, 220},
	{255, 128, 209},
	{255, 128, 197},
	{255, 128, 186},
	{255, 128, 174},
	{255, 128, 163},
	{255, 128, 151},
	{255, 128, 140},
	{255, 0, 0}, // Red (cycleable)
	{255, 23, 0},
	{255, 46, 0},
	{255, 70, 0},
	{255, 93, 0},
	{255, 116, 0},
	{255, 139, 0},
	{255, 162, 0},
	{255, 185, 0},
	{255, 209, 0},
	{255, 232, 0},
	{255, 255, 0}, // Yellow (cycleable)
	{232, 255, 0},
	{209, 255, 0},
	{185, 255, 0},
	{162, 255, 0},
	{139, 255, 0},
	{116, 255, 0},
	{93, 255, 0},
	{70, 255, 0},
	{46, 255, 0},
	{23, 255, 0},
	{0, 255, 0}, // Green (cycleable)
	{0, 255, 23},
	{0, 255, 46},
	{0, 255, 70},
	{0, 255, 93},
	{0, 255, 116},
	{0, 255, 139},
	{0, 255, 162},
	{0, 255, 185},
	{0, 255, 209},
	{0, 255, 232},
	{0, 255, 255}, // Cyan (cycleable)
	{0, 232, 255},
	{0, 209, 255},
	{0, 185, 255},
	{0, 162, 255},
	{0, 139, 255},
	{0, 116, 255},
	{0, 93, 255},
	{0, 70, 255},
	{0, 46, 255},
	{0, 23, 255},
	{0, 0, 255}, // Blue (cycleable)
	{23, 0, 255},
	{46, 0, 255},
	{70, 0, 255},
	{93, 0, 255},
	{116, 0, 255},
	{139, 0, 255},
	{162, 0, 255},
	{185, 0, 255},
	{209, 0, 255},
	{232, 0, 255},
	{255, 0, 255}, // Magenta (cycleable)
	{255, 0, 232},
	{255, 0, 209},
	{255, 0, 185},
	{255, 0, 162},
	{255, 0, 139},
	{255, 0, 116},
	{255, 0, 93},
	{255, 0, 70},
	{255, 0, 46},
	{255, 0, 23},
	{128, 0, 0}, // Dark red
	{128, 12, 0},
	{128, 23, 0},
	{128, 35, 0},
	{128, 47, 0},
	{128, 58, 0},
	{128, 70, 0},
	{128, 81, 0},
	{128, 93, 0},
	{128, 105, 0},
	{128, 116, 0},
	{128, 128, 0}, // Dark yellow
	{116, 128, 0},
	{105, 128, 0},
	{93, 128, 0},
	{81, 128, 0},
	{70, 128, 0},
	{58, 128, 0},
	{47, 128, 0},
	{35, 128, 0},
	{23, 128, 0},
	{12, 128, 0},
	{0, 128, 0}, // Dark green
	{0, 128, 12},
	{0, 128, 23},
	{0, 128, 35},
	{0, 128, 47},
	{0, 128, 58},
	{0, 128, 70},
	{0, 128, 81},
	{0, 128, 93},
	{0, 128, 105},
	{0, 128, 116},
	{0, 128, 128}, // Dark cyan
	{0, 116, 128},
	{0, 105, 128},
	{0, 93, 128},
	{0, 81, 128},
	{0, 70, 128},
	{0, 58, 128},
	{0, 47, 128},
	{0, 35, 128},
	{0, 23, 128},
	{0, 12, 128},
	{0, 0, 128}, // Dark blue
	{12, 0, 128},
	{23, 0, 128},
	{35, 0, 128},
	{47, 0, 128},
	{58, 0, 128},
	{70, 0, 128},
	{81, 0, 128},
	{93, 0, 128},
	{105, 0, 128},
	{116, 0, 128},
	{128, 0, 128}, // Dark magenta
	{128, 0, 116},
	{128, 0, 105},
	{128, 0, 93},
	{128, 0, 81},
	{128, 0, 70},
	{128, 0, 58},
	{128, 0, 47},
	{128, 0, 35},
	{128, 0, 23},
	{128, 0, 12},
	{255, 192, 192}, // Very light red
	{255, 64, 64},	 // Light-medium red
	{192, 0, 0},	 // Medium-dark red
	{64, 0, 0},		 // Very dark red
	{255, 255, 192}, // Very light yellow
	{255, 255, 64},	 // Light-medium yellow
	{192, 192, 0},	 // Medium-dark yellow
	{64, 64, 0},	 // Very dark yellow
	{192, 255, 192}, // Very light green
	{64, 255, 64},	 // Light-medium green
	{0, 192, 0},	 // Medium-dark green
	{0, 64, 0},		 // Very dark green
	{192, 255, 255}, // Very light cyan
	{64, 255, 255},	 // Light-medium cyan
	{0, 192, 192},	 // Medium-dark cyan
	{0, 64, 64},	 // Very dark cyan
	{192, 192, 255}, // Very light blue
	{64, 64, 255},	 // Light-medium blue
	{0, 0, 192},	 // Medium-dark blue
	{0, 0, 64},		 // Very dark blue
	{255, 192, 255}, // Very light magenta
	{255, 64, 255},	 // Light-medium magenta
	{192, 0, 192},	 // Medium-dark magenta
	{64, 0, 64},	 // Very dark magenta
	{255, 96, 96},	 // Medium skin tone
	{255, 255, 255}, // White (cycleable)
	{245, 245, 245},
	{235, 235, 235},
	{224, 224, 224}, // Very light gray (7/8 intensity)
	{213, 213, 213},
	{203, 203, 203},
	{192, 192, 192}, // Light gray (3/4 intensity)
	{181, 181, 181},
	{171, 171, 171},
	{160, 160, 160}, // Medium-light gray (5/8 int.)
	{149, 149, 149},
	{139, 139, 139},
	{128, 128, 128}, // Medium gray (1/2 intensity)
	{117, 117, 117},
	{107, 107, 107},
	{96, 96, 96}, // Medium-dark gray (3/8 int.)
	{85, 85, 85},
	{75, 75, 75},
	{64, 64, 64}, // Dark gray (1/4 intensity)
	{53, 53, 53},
	{43, 43, 43},
	{32, 32, 32}, // Very dark gray (1/8 intensity)
	{21, 21, 21},
	{11, 11, 11},
	{0, 0, 0}, // Black
};

static int lookup_color(uint8_t *c, uint8_t idx, ilda_t *ilda, bool use_64_color_table)
{
	if(ilda->pallete_present)
	{
		if(idx >= ilda->pallete_size) return -1;
		memcpy(c, &ilda->pallete[3 * idx], 3);
	}
	else
	{
		if(use_64_color_table)
		{
			if(idx >= sizeof(ilda_pallete_64_default) / sizeof(ilda_pallete_64_default[0])) return -2;
			memcpy(c, &ilda_pallete_64_default[idx], 3);
		}
		else
		{
			memcpy(c, &ilda_pallete_256_default[idx], 3);
		}
	}
	return 0;
}

static void parse_point(const uint8_t *data, uint32_t format, ilda_t *ilda, bool use_64_color_table)
{
	ilda_point_t p = {0};

	switch(format)
	{
	case ILDA_FMT_3D_INDEXED:
		p.x = (data[0] << 8) | data[1];
		p.y = (data[2] << 8) | data[3];
		p.z = (data[4] << 8) | data[5];
		p.blanked = data[6] & (1 << 6);
		p.last_point = data[6] & (1 << 7);
		lookup_color(p.color, data[7], ilda, use_64_color_table);
		if(p.blanked) p.color[0] = p.color[1] = p.color[2] = 0;
		cb_ilda_point(ilda, &p);
		break;

	case ILDA_FMT_2D_INDEXED:
		p.x = (data[0] << 8) | data[1];
		p.y = (data[2] << 8) | data[3];
		p.blanked = data[4] & (1 << 6);
		p.last_point = data[4] & (1 << 7);
		lookup_color(p.color, data[5], ilda, use_64_color_table);
		if(p.blanked) p.color[0] = p.color[1] = p.color[2] = 0;
		cb_ilda_point(ilda, &p);
		break;

	case ILDA_FMT_3D_TRUE:
		p.x = (data[0] << 8) | data[1];
		p.y = (data[2] << 8) | data[3];
		p.z = (data[4] << 8) | data[5];
		p.blanked = data[6] & (1 << 6);
		p.last_point = data[6] & (1 << 7);
		p.color[2] = data[7]; // b
		p.color[1] = data[8]; // g
		p.color[0] = data[9]; // r
		if(p.blanked) p.color[0] = p.color[1] = p.color[2] = 0;
		cb_ilda_point(ilda, &p);
		break;

	case ILDA_FMT_2D_TRUE:
		p.x = (data[0] << 8) | data[1];
		p.y = (data[2] << 8) | data[3];
		p.blanked = data[4] & (1 << 6);
		p.last_point = data[4] & (1 << 7);
		p.color[2] = data[5]; // b
		p.color[1] = data[6]; // g
		p.color[0] = data[7]; // r
		if(p.blanked) p.color[0] = p.color[1] = p.color[2] = 0;
		cb_ilda_point(ilda, &p);
		break;

	case ILDA_FMT_PALETTE:
		memcpy(&ilda->pallete[ilda_format_chunk_sz[ILDA_FMT_PALETTE] * ilda->pallete_size++], data, ilda_format_chunk_sz[ILDA_FMT_PALETTE]);
		break;

	default: break;
	}
}

static int parse(ilda_t *ilda, uint8_t *buf, uint32_t *buf_size, bool use_64_color_table)
{
ONCE_AGAIN:
	if(ilda->phase_frames == false)
	{
		if(*buf_size >= sizeof(ilda_header_t))
		{
			memcpy((uint8_t *)&ilda->header, buf, sizeof(ilda_header_t));

			if(strcmp("ILDA", ilda->header.ilda_id_str) != 0) return ILDA_ERR_NO_ILDA_HDR;
			if(ilda->header.format >= sizeof(ilda_format_chunk_sz) / sizeof(ilda_format_chunk_sz[0])) return ILDA_ERR_UNKNWN_FMT;

			ilda->processed_points = 0;
			ilda->header.frame_count = ntohs(ilda->header.frame_count);
			ilda->header.point_count = ntohs(ilda->header.point_count);

			if(ilda->header.format == ILDA_FMT_PALETTE && ilda->header.point_count > 0)
			{
				ilda->pallete_size = 0;
				if(ilda->pallete) free(ilda->pallete);
				ilda->pallete = malloc(ilda_format_chunk_sz[ilda->header.format] * ilda->header.point_count);
			}

			if(ilda->header.point_count == 0)
			{
				memcpy(buf, &buf[sizeof(ilda_header_t)], *buf_size - sizeof(ilda_header_t));
				*buf_size -= sizeof(ilda_header_t);

				if(*buf_size >= ilda_format_chunk_sz[ilda->header.format]) goto ONCE_AGAIN; // can process one more header
				return 0;
			}

			if(ilda->header.format != ILDA_FMT_PALETTE && ilda->header.format != 3)
			{
				if(ilda->header.point_count) cb_ilda_frame(ilda, ilda->header.point_count);
				ilda->frame_count++;
				ilda->point_count += ilda->header.point_count;
				if(ilda->max_point_per_frame < ilda->header.point_count) ilda->max_point_per_frame = ilda->header.point_count;
			}

			memcpy(buf, &buf[sizeof(ilda_header_t)], *buf_size - sizeof(ilda_header_t));
			*buf_size -= sizeof(ilda_header_t);

			ilda->phase_frames = true;

			if(*buf_size >= ilda_format_chunk_sz[ilda->header.format]) goto ONCE_AGAIN;
		}
	}
	else
	{
		for(uint32_t rel_pos = 0;;)
		{
			if(*buf_size - rel_pos < ilda_format_chunk_sz[ilda->header.format])
			{
				if(rel_pos > 0) memcpy(buf, &buf[sizeof(ilda_header_t)], *buf_size - rel_pos);
				*buf_size -= rel_pos;
				return 0; // wait next chunk
			}
			parse_point(&buf[rel_pos], ilda->header.format, ilda, use_64_color_table);
			rel_pos += ilda_format_chunk_sz[ilda->header.format];
			ilda->processed_points++;

			if(ilda->processed_points >= ilda->header.point_count)
			{
				ilda->phase_frames = false;
				if(*buf_size - rel_pos > 0) memcpy(buf, &buf[rel_pos], *buf_size - rel_pos);
				*buf_size -= rel_pos;
				if(*buf_size >= sizeof(ilda_header_t)) goto ONCE_AGAIN; // can process one more header
				break;
			}
		}
	}

	return 0;
}

void ilda_file_init(ilda_t *ilda)
{
	memset(ilda, 0, sizeof(ilda_t));
}

void ilda_file_free(ilda_t *ilda)
{
	if(ilda->frames) free(ilda->frames);
	if(ilda->pallete) free(ilda->pallete);
}

int ilda_file_parse_chunk(ilda_t *ilda, const uint8_t *buf, uint32_t size, bool use_64_color_table)
{
	if(size == 0) return ilda->chunk_pos ? ILDA_ERR_EXTRA_DATA : 0;

	if(ilda->chunk_pos + size > ILDA_FILE_CHUNK) return ILDA_ERR_CHUNK_OVF;

	memcpy(&ilda->chunk[ilda->chunk_pos], buf, size);
	ilda->chunk_pos += size;

	return parse(ilda, ilda->chunk, &ilda->chunk_pos, use_64_color_table);
}

int ilda_file_parse_file(ilda_t *ilda, uint8_t *buf, uint32_t size, bool use_64_color_table)
{
	memset(ilda, 0, sizeof(ilda_t));

	for(uint32_t of = 0;;)
	{
#define CHNK (ILDA_FILE_CHUNK / 2)
		int sts = ilda_file_parse_chunk(ilda, (uint8_t *)&buf[of], size - of >= CHNK ? CHNK : size - of, use_64_color_table);
		if(sts != 0) return sts;

		of += size - of >= CHNK ? CHNK : size - of;
		if(of >= size)
		{
			sts = ilda_file_parse_chunk(ilda, NULL, 0, use_64_color_table);
			if(sts != 0) return sts;
			return 0;
		}
	}
}
