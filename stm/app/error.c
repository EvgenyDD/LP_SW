#include "error.h"

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

const char *error_get_str(uint32_t error)
{
	const char *s[] = {
#define _F(x) #x
		ERR_DESCR
#undef _F
	};
	return s[error];
}