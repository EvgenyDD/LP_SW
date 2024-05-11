#ifndef OPT3001_H__
#define OPT3001_H__

#include <stdint.h>

int opt3001_init(void);
int opt3001_read(void);

extern float opt3001_lux;

#endif // OPT3001_H__