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
		lp_fb_append(p->x, p->y, p->color[0], p->color[1], p->color[2], 0.2f, 0.2f, 1000000 / fr);
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

	ESP_LOGI("FA", "COORD: X: %d %d | Y: %d %d | Count: %d", inst.min_c[0], inst.max_c[0], inst.min_c[1], inst.max_c[1], pnt_count);

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
	ESP_LOGI("", "File load size: %d | sts: %d", pnt_count, sts);
	fclose(fd);
	return sts;
}