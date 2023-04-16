#ifndef __DEV_DRIVERS_H__
#define __DEV_DRIVERS_H__

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

void dev_drivers_init(void);

extern SemaphoreHandle_t g_i2c_mutex;

#endif // __DEV_DRIVERS_H__