#ifndef SERIAL_H__
#define SERIAL_H__

#include <stdint.h>

void serial_init(void);

void serial_tx(const uint8_t *data, uint32_t len);

#endif // SERIAL_H__