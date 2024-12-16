#include "error.h"
#include "proto.h"

static volatile uint32_t errors = 0, errors_latched = 0;

void error_set(uint32_t error, bool value)
{
	if(value)
	{
		errors |= 1 << error;
		errors_latched |= 1 << error;
	}
	else
	{
		errors &= (uint32_t) ~(1 << error);
	}
}

uint32_t error_get(void) { return errors; }
uint32_t error_get_latched(void) { return errors_latched; }

void error_reset(void)
{
	errors_latched = 0;
	errors = 0;
}

const char *error_get_str(uint32_t error)
{
	switch(error)
	{
	case ERR_STM_SFTY: return "S_SFTY";
	case ERR_STM_CFG: return "S_CFG";
	case ERR_STM_FRAM: return "S_FRAM";
	case ERR_STM_RB: return "S_RB";
	case ERR_STM_TO: return "S_TO";
	case ERR_STM_OT: return "S_OT";
	case ERR_STM_PS: return "S_PS";
	case ERR_STM_PE: return "S_PE";
	case ERR_STM_FAN: return "S_FAN";
	case ERR_STM_KEY: return "S_KEY";
	case ERR_STM_IMU_MOVE: return "S_IMU";
	case ERR_STM_PROTO: return "S_PROT";
	default: return "---";
	}
}