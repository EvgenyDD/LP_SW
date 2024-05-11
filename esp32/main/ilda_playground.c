#include "ilda_playground.h"
#include "esp_log.h"
#include "ilda.h"
#include "lp.h"
#include <stdint.h>
#include <stdio.h>

uint32_t fr = 12000;
static bool use_64b_color = true;

static ilda_t ilda = {0};
static uint32_t pnt_count = 0;

const uint8_t gamma8[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
	2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
	5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
	10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
	17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
	25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
	37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
	51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
	69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
	90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
	115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
	144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
	177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
	215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255};

struct
{
	bool fill_fb; // frame buffer
	int32_t min_c[2], max_c[2];
	float max_br;
} inst = {0};

void cb_ilda_point(ilda_t *ilda, const ilda_point_t *p)
{
	pnt_count++;
	if(inst.min_c[0] > p->x) inst.min_c[0] = p->x;
	if(inst.min_c[1] > p->y) inst.min_c[1] = p->y;

	if(inst.max_c[0] < p->x) inst.max_c[0] = p->x;
	if(inst.max_c[1] < p->y) inst.max_c[1] = p->y;

	if(inst.fill_fb)
	{
		lp_fb_append(p->x, p->y, gamma8[p->color[0]], gamma8[p->color[1]], gamma8[p->color[2]], 0.3f, 1.0f, 1000000 / fr);
	}
}

void cb_ilda_frame(ilda_t *ilda, uint32_t count_points)
{
	if(inst.fill_fb)
	{
	}
}

int ilda_check_file(const char *path)
{
	pnt_count = 0;
	lp_fb_reinit();

	inst.min_c[0] = inst.min_c[1] = inst.max_c[0] = inst.max_c[1] = 0;

	inst.fill_fb = false;

	ilda_file_init(&ilda);

	FILE *fd = fopen(path, "r");
	if(fd == NULL) return ERR_OPEN;

	uint8_t buf[128];
	uint32_t sz = 0;
	int chunksize, sts;
	do
	{
		chunksize = fread(buf, 1, sizeof(buf), fd);
		if(chunksize > 0)
		{
			sts = ilda_file_parse_chunk(&ilda, buf, chunksize, use_64b_color);
			if(sts)
			{
				fclose(fd);
				return sts;
			}
			sz += chunksize;
		}
	} while(chunksize != 0);

	sts = ilda_file_parse_chunk(&ilda, NULL, 0, use_64b_color);

	ESP_LOGI("FA", "COORD: X: %ld %ld | Y: %ld %ld | Count: %ld", inst.min_c[0], inst.max_c[0], inst.min_c[1], inst.max_c[1], pnt_count);

	fclose(fd);
	return sts;
}

int ilda_file_load(const char *path)
{
	pnt_count = 0;

	inst.max_br = 0.3;
	inst.fill_fb = true;
	ilda_file_init(&ilda);

	FILE *fd = fopen(path, "r");
	if(fd == NULL) return ERR_OPEN;

	uint8_t buf[128];
	uint32_t sz = 0;
	int chunksize, sts;
	do
	{
		chunksize = fread(buf, 1, sizeof(buf), fd);
		if(chunksize > 0)
		{
			sts = ilda_file_parse_chunk(&ilda, buf, chunksize, use_64b_color);
			if(sts)
			{
				fclose(fd);
				return sts;
			}
			sz += chunksize;
		}
	} while(chunksize != 0);

	sts = ilda_file_parse_chunk(&ilda, NULL, 0, use_64b_color);
	ESP_LOGI("", "File load size: %ld | sts: %d", pnt_count, sts);
	fclose(fd);
	return sts;
}