#ifndef IMU_H__
#define IMU_H__

typedef struct
{
	float xl[3];
	float g[3];
} imu_val_t;

int imu_init(void);
int imu_read(void);

extern imu_val_t imu_val;

#endif // IMU_H__