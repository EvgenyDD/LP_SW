#include "ilda.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ILDA_FMT_3D_INDEXED 0
#define ILDA_FMT_2D_INDEXED 1
#define ILDA_FMT_PALETTE 2
#define ILDA_FMT_3D_TRUE 4
#define ILDA_FMT_2D_TRUE 5

static const char format_chunk_sz[] = {
	3 * sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t),	  // 3D INDEXED
	2 * sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t),	  // 2D INDEXED
	3 * sizeof(uint8_t),										  // PALETTE
	3,															  // WTF is that format? no specs....
	3 * sizeof(uint16_t) + sizeof(uint8_t) + 3 * sizeof(uint8_t), // 3D TRUE
	2 * sizeof(uint16_t) + sizeof(uint8_t) + 3 * sizeof(uint8_t), // 2D TRUE
};

const uint8_t ilda_pallete_64_default[][3] = {
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

const uint8_t ilda_pallete_256_default[][3] = {
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

int ilda_file_read2(const char *data, uint32_t fsize, ilda_t *ilda, bool use_64_color_table)
{
	if(strcmp("ILDA", data) != 0)
	{
		printf("Section header did not contain \"ILDA\"\n");
		return -1;
	}

	memset(ilda, 0, sizeof(ilda_t));

	int fpos = 0;

	ilda_header_t header;

	// read in the header
	memcpy((uint8_t *)&header, data + fpos, sizeof(ilda_header_t));
	fpos += sizeof(ilda_header_t);
	header.point_count = ntohs(header.point_count);
	header.frame_count = ntohs(header.frame_count);
	uint32_t frame_count = header.frame_count;

	// allocate space for the frames
	ilda->frame_count = header.frame_count;
	ilda->frames = (ilda_frame_t *)calloc(ilda->frame_count, sizeof(ilda_frame_t));

	// read in each frame/pallete
	for(uint32_t i = 0; i < ilda->frame_count; i++)
	{
		if(header.format >= sizeof(format_chunk_sz) / sizeof(format_chunk_sz[0]))
		{
			printf("Unknown format %d at frame %d, file pos: x%X\n", header.format, i, fpos);
			return -2;
		}
		const int chunk = format_chunk_sz[header.format];
		if(format_chunk_sz[header.format] == 0)
		{
			printf("Unknown format %d at frame %d, file pos: x%X\n", header.format, i, fpos);
			// return -3;
		}

		if(header.format == 3) printf("3 fmt count: %d\n", header.point_count);

		if(header.format == ILDA_FMT_PALETTE)
		{
			ilda->pallete_present = true;
			ilda->pallete_size = header.point_count;
			printf("\t\tPallete found %d\n", ilda->pallete_size);
			if(ilda->pallete) free(ilda->pallete);
			ilda->pallete = (uint8_t *)malloc(ilda->pallete_size * 3);
			memcpy(ilda->pallete, data + fpos, ilda->pallete_size * 3);
			fpos += ilda->pallete_size * 3;
		}
		else
		{
			ilda->frames[i].point_count = header.point_count;
			ilda->frames[i].points = (ilda_point_t *)calloc(header.point_count, sizeof(ilda_point_t));
			for(uint32_t j = 0; j < header.point_count; j++)
			{
				uint8_t ch[chunk];
				memcpy(ch, data + fpos, chunk);
				fpos += chunk;

				switch(header.format)
				{
				case ILDA_FMT_3D_INDEXED:
					ilda->frames[i].points[j].x = (ch[0] << 8) | ch[1];
					ilda->frames[i].points[j].y = (ch[2] << 8) | ch[3];
					ilda->frames[i].points[j].z = (ch[4] << 8) | ch[5];
					ilda->frames[i].points[j].blanked = ch[6] & (1 << 6);
					ilda->frames[i].points[j].last_point = ch[6] & (1 << 7);
					if(lookup_color(ilda->frames[i].points[j].color, ch[7], ilda, use_64_color_table) != 0) return -4;
					break;

				case ILDA_FMT_2D_INDEXED:
					ilda->frames[i].points[j].x = (ch[0] << 8) | ch[1];
					ilda->frames[i].points[j].y = (ch[2] << 8) | ch[3];
					ilda->frames[i].points[j].blanked = ch[4] & (1 << 6);
					ilda->frames[i].points[j].last_point = ch[4] & (1 << 7);
					if(lookup_color(ilda->frames[i].points[j].color, ch[5], ilda, use_64_color_table) != 0) return -4;
					break;

				case ILDA_FMT_3D_TRUE:
					ilda->frames[i].points[j].x = (ch[0] << 8) | ch[1];
					ilda->frames[i].points[j].y = (ch[2] << 8) | ch[3];
					ilda->frames[i].points[j].z = (ch[4] << 8) | ch[5];
					ilda->frames[i].points[j].blanked = ch[6] & (1 << 6);
					ilda->frames[i].points[j].last_point = ch[6] & (1 << 7);
					ilda->frames[i].points[j].color[2] = ch[7]; // b
					ilda->frames[i].points[j].color[1] = ch[8]; // g
					ilda->frames[i].points[j].color[0] = ch[9]; // r
					break;

				case ILDA_FMT_2D_TRUE:
					ilda->frames[i].points[j].x = (ch[0] << 8) | ch[1];
					ilda->frames[i].points[j].y = (ch[2] << 8) | ch[3];
					ilda->frames[i].points[j].blanked = ch[4] & (1 << 6);
					ilda->frames[i].points[j].last_point = ch[4] & (1 << 7);
					ilda->frames[i].points[j].color[2] = ch[5]; // b
					ilda->frames[i].points[j].color[1] = ch[6]; // g
					ilda->frames[i].points[j].color[0] = ch[7]; // r
					break;

					// default:
					// printf("Unknown format: %d at frame %d\n", header.format, i);
					// return -3;
				}
			}
		}

		// read the next header
		if(i != ilda->frame_count - 1)
		{
			memcpy((uint8_t *)&header, data + fpos, sizeof(ilda_header_t));
			fpos += sizeof(ilda_header_t);
			header.point_count = ntohs(header.point_count);
			header.frame_count = ntohs(header.frame_count);
		}

		if(frame_count != header.frame_count)
		{
			// printf("WTFFFFFFFFFFFFFFFFFFFFFFFff frame_count != header.frame_count, %d %d\n", frame_count, header.frame_count);
			frame_count = header.frame_count;
		}
	}

	if(fpos != fsize)
	{
		printf("Length mismatch: %d and file %d =====> %d\n", fpos, fsize, fpos - fsize);
	}

	return 0;
}

int ilda_file_read(const char *data, uint32_t fsize, ilda_t *ilda, bool use_64_color_table)
{
	memset(ilda, 0, sizeof(ilda_t));
	int fpos = 0;
	ilda_header_t header;

	while(1)
	{
		// read in the header
		memcpy((uint8_t *)&header, data + fpos, sizeof(ilda_header_t));
		fpos += sizeof(ilda_header_t);

		if(strcmp("ILDA", header.ilda_id_str) != 0)
		{
			printf("Section header did not contain \"ILDA\" @offset x%X\n", fpos);
			return -1;
		}

		if(header.format >= sizeof(format_chunk_sz) / sizeof(format_chunk_sz[0]))
		{
			printf("Unknown format %d, file pos: x%X\n", header.format, fpos);
			return -2;
		}

		header.frame_count = ntohs(header.frame_count);

		header.point_count = ntohs(header.point_count);
		if(header.point_count == 0) return 0;

		if(header.format != 2 && header.format != 3)
		{
			ilda->frame_count++;
			ilda->point_count += header.point_count;
			if(ilda->max_point_per_frame < header.point_count) ilda->max_point_per_frame = header.point_count;
		}

		fpos += format_chunk_sz[header.format] * header.point_count;

		if(fpos == fsize) return 0;
		if(fpos > fsize)
		{
			printf("Length mismatch: %d and file %d =====> %d\n", fpos, fsize, fpos - fsize);
			return -10;
		}
	}
	// // allocate space for the frames
	// ilda->frame_count = header.frame_count;
	// ilda->frames = (ilda_frame_t *)calloc(ilda->frame_count, sizeof(ilda_frame_t));

	// // read in each frame/pallete
	// for(uint32_t i = 0; i < ilda->frame_count; i++)
	// {
	// 	if(header.format >= sizeof(format_chunk_sz) / sizeof(format_chunk_sz[0]))
	// 	{
	// 		printf("Unknown format %d at frame %d, file pos: x%X\n", header.format, i, fpos);
	// 		return -2;
	// 	}
	// 	const int chunk = format_chunk_sz[header.format];
	// 	if(format_chunk_sz[header.format] == 0)
	// 	{
	// 		printf("Unknown format %d at frame %d, file pos: x%X\n", header.format, i, fpos);
	// 		// return -3;
	// 	}

	// 	if(header.format == 3) printf("3 fmt count: %d\n", header.point_count);

	// 	if(header.format == ILDA_FMT_PALETTE)
	// 	{
	// 		ilda->pallete_present = true;
	// 		ilda->pallete_size = header.point_count;
	// 		printf("\t\tPallete found %d\n", ilda->pallete_size);
	// 		if(ilda->pallete) free(ilda->pallete);
	// 		ilda->pallete = (uint8_t *)malloc(ilda->pallete_size * 3);
	// 		memcpy(ilda->pallete, data + fpos, ilda->pallete_size * 3);
	// 		fpos += ilda->pallete_size * 3;
	// 	}
	// 	else
	// 	{
	// 		ilda->frames[i].point_count = header.point_count;
	// 		ilda->frames[i].points = (ilda_point_t *)calloc(header.point_count, sizeof(ilda_point_t));
	// 		for(uint32_t j = 0; j < header.point_count; j++)
	// 		{
	// 			uint8_t ch[chunk];
	// 			memcpy(ch, data + fpos, chunk);
	// 			fpos += chunk;

	// 			switch(header.format)
	// 			{
	// 			case ILDA_FMT_3D_INDEXED:
	// 				ilda->frames[i].points[j].x = (ch[0] << 8) | ch[1];
	// 				ilda->frames[i].points[j].y = (ch[2] << 8) | ch[3];
	// 				ilda->frames[i].points[j].z = (ch[4] << 8) | ch[5];
	// 				ilda->frames[i].points[j].blanked = ch[6] & (1 << 6);
	// 				ilda->frames[i].points[j].last_point = ch[6] & (1 << 7);
	// 				if(lookup_color(ilda->frames[i].points[j].color, ch[7], ilda, use_64_color_table) != 0) return -4;
	// 				break;

	// 			case ILDA_FMT_2D_INDEXED:
	// 				ilda->frames[i].points[j].x = (ch[0] << 8) | ch[1];
	// 				ilda->frames[i].points[j].y = (ch[2] << 8) | ch[3];
	// 				ilda->frames[i].points[j].blanked = ch[4] & (1 << 6);
	// 				ilda->frames[i].points[j].last_point = ch[4] & (1 << 7);
	// 				if(lookup_color(ilda->frames[i].points[j].color, ch[5], ilda, use_64_color_table) != 0) return -4;
	// 				break;

	// 			case ILDA_FMT_3D_TRUE:
	// 				ilda->frames[i].points[j].x = (ch[0] << 8) | ch[1];
	// 				ilda->frames[i].points[j].y = (ch[2] << 8) | ch[3];
	// 				ilda->frames[i].points[j].z = (ch[4] << 8) | ch[5];
	// 				ilda->frames[i].points[j].blanked = ch[6] & (1 << 6);
	// 				ilda->frames[i].points[j].last_point = ch[6] & (1 << 7);
	// 				ilda->frames[i].points[j].color[2] = ch[7]; // b
	// 				ilda->frames[i].points[j].color[1] = ch[8]; // g
	// 				ilda->frames[i].points[j].color[0] = ch[9]; // r
	// 				break;

	// 			case ILDA_FMT_2D_TRUE:
	// 				ilda->frames[i].points[j].x = (ch[0] << 8) | ch[1];
	// 				ilda->frames[i].points[j].y = (ch[2] << 8) | ch[3];
	// 				ilda->frames[i].points[j].blanked = ch[4] & (1 << 6);
	// 				ilda->frames[i].points[j].last_point = ch[4] & (1 << 7);
	// 				ilda->frames[i].points[j].color[2] = ch[5]; // b
	// 				ilda->frames[i].points[j].color[1] = ch[6]; // g
	// 				ilda->frames[i].points[j].color[0] = ch[7]; // r
	// 				break;

	// 				// default:
	// 				// printf("Unknown format: %d at frame %d\n", header.format, i);
	// 				// return -3;
	// 			}
	// 		}
	// 	}

	// 	// read the next header
	// 	if(i != ilda->frame_count - 1)
	// 	{
	// 		memcpy((uint8_t *)&header, data + fpos, sizeof(ilda_header_t));
	// 		fpos += sizeof(ilda_header_t);
	// 		header.point_count = ntohs(header.point_count);
	// 		header.frame_count = ntohs(header.frame_count);
	// 	}

	// 	if(frame_count != header.frame_count)
	// 	{
	// 		// printf("WTFFFFFFFFFFFFFFFFFFFFFFFff frame_count != header.frame_count, %d %d\n", frame_count, header.frame_count);
	// 		frame_count = header.frame_count;
	// 	}
	// }

	// if(fpos != fsize)
	// {
	// 	printf("Length mismatch: %d and file %d =====> %d\n", fpos, fsize, fpos - fsize);
	// }
}

void ilda_file_free(ilda_t *ilda)
{
	if(ilda->frames) free(ilda->frames);
	if(ilda->pallete) free(ilda->pallete);
}