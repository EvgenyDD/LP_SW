#ifndef ERROR_H__
#define ERROR_H__

#include "proto.h"
#include <stdbool.h>
#include <stdint.h>

void error_set(uint32_t error, bool value);
uint32_t error_get(void);
uint32_t error_get_latched(void);
const char *error_get_str(uint32_t error);

#endif // ERROR_H__