#ifndef ACCEL_H__
#define ACCEL_H__

#include <stdint.h>

void accel_reset_fixture(void);
void accel_get_data(int8_t acc[3]);

void accel_task(void *pvParameter);

#endif // ACCEL_H__