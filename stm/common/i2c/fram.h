#ifndef FRAM_H__
#define FRAM_H__

#include <stdint.h>

typedef struct __attribute__((packed))
{
	uint32_t turn_on_cnt;
	uint64_t work_time_ms;
	uint32_t lp_flags; // see <proto.h>
} fram_data_t;

int fram_read(uint32_t addr, void *buf, uint32_t sz);
int fram_write(uint32_t addr, const void *buf, uint32_t sz);

extern fram_data_t g_fram_data;

#endif // FRAM_H__