#ifndef ERROR_H__
#define ERROR_H__

#include <stdbool.h>
#include <stdint.h>

#define ERR_DESCR \
	_F(CFG),      \
		_F(FRAM), \
		_F(RB),   \
		_F(OT),   \
		_F(PS),   \
		_F(FAN),  \
		_F(KEY),  \
		_F(SFTY), \
		_F(COUNT)
enum
{
#define _F(x) ERROR_##x
	ERR_DESCR
#undef _F
};

void error_set(uint32_t error, bool value);
uint32_t error_get(void);
uint32_t error_get_latched(void);
const char *error_get_str(uint32_t error);

#endif // ERROR_H__