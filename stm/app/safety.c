#include "safety.h"
#include "adc.h"
#include "buttons.h"
#include "error.h"
#include "imu.h"
#include "lsr_ctrl.h"
#include "platform.h"
#include "proto.h"
#include "proto_l0.h"
#include <math.h>

#define GALV_PWR_EN GPIOB->BSRRL = (1 << 10)
#define GALV_PWR_DIS GPIOB->BSRRH = (1 << 10)

#define LSR_PWR_EN GPIOB->BSRRH = (1 << 9)
#define LSR_PWR_DIS GPIOB->BSRRL = (1 << 9)

#define TEMP_THRS 25
#define TEMP_THRS_CRITICAL 60
#define TEMP_THRS_FAN_HEAD 38
#define TEMP_THRS_CRITICAL_HEAD 40
#define TIMER_VAL_RESET_ERR 500

#define TO_SER_THRS 500

#define V_PS_THRSH 1.2f
#define V_PS_NOM 24.0f

#define FAN_STARTUP_DLY 3500

#if 0
#define IMU_XL_THRS 0.05f
#define IMU_G_THRS 10.0f
#else
#define IMU_XL_THRS 999.0f
#define IMU_G_THRS 99999.0f
#endif

static bool safety_lock = true;
static uint32_t reset_timer = 0;

static uint32_t fan_prev_capt = 0;
static float fan_vel = 0;
static uint32_t fan_capt_to = 0;

static uint32_t to_ser = TO_SER_THRS;

static uint32_t startup_cnt_fan = 0;

static imu_val_t imu_lock_fixture = {0};

static uint8_t lsr_ctrl_used_by_esp = false;
static uint8_t lsr_ctrl_used_by_stm = false;

static uint8_t is_pwr_enabled = false;

static int check_imu_fix(void)
{
	for(uint32_t i = 0; i < 3; i++)
	{
		if(fabsf(imu_lock_fixture.xl[i] - imu_val.xl[i]) > IMU_XL_THRS) return 1;
		if(fabsf(imu_lock_fixture.g[i] - imu_val.g[i]) > IMU_G_THRS) return 2;
	}
	return 0;
}

static void fan_ctrl(uint32_t diff_ms)
{
	float maxt = -373.15f;

	if(adc_val.t[0] > maxt) maxt = adc_val.t[0];
	if(adc_val.t[1] > maxt) maxt = adc_val.t[1];
	if(adc_val.t_amp > maxt) maxt = adc_val.t_amp;

	uint32_t value0 = map(maxt, TEMP_THRS, TEMP_THRS_CRITICAL, 0, 255);
	uint32_t value1 = map(adc_val.t_head, TEMP_THRS, TEMP_THRS_FAN_HEAD, 0, 255);
	if(startup_cnt_fan < FAN_STARTUP_DLY)
	{
		startup_cnt_fan += diff_ms;
		fan_update(map(startup_cnt_fan, 0, FAN_STARTUP_DLY, 255, value0 > value1 ? value0 : value1));
	}
	else
	{
		fan_update(value0 > value1 ? value0 : value1);
	}
}

void fan_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource15, GPIO_AF_TIM2);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource1, GPIO_AF_TIM3);

	DAC_InitTypeDef DAC_InitStructure;
	DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;
	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
	DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = 0;
	DAC_Init(DAC_Channel_1, &DAC_InitStructure);

	DAC_Cmd(DAC_Channel_1, ENABLE);

	DAC_SetChannel1Data(DAC_Align_12b_R, 4095);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_ICInitTypeDef TIM_ICInitStructure;

	TIM_TimeBaseStructure.TIM_Period = 65535;
	TIM_TimeBaseStructure.TIM_Prescaler = 8400 - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure); // TIM2 is used to capture fan speed
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); // TIM3 is used to capture fan timeout

	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 15;
	TIM_ICInit(TIM2, &TIM_ICInitStructure);
	TIM_SelectSlaveMode(TIM2, TIM_SlaveMode_Reset);
	TIM_SelectInputTrigger(TIM2, TIM_TS_TI1FP1);

	TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
	TIM_ICInit(TIM3, &TIM_ICInitStructure);

	TIM_Cmd(TIM2, ENABLE);
	TIM_CCxCmd(TIM2, TIM_Channel_1, TIM_CCx_Enable);

	TIM_Cmd(TIM3, ENABLE);
	TIM_CCxCmd(TIM3, TIM_Channel_4, TIM_CCx_Enable);
}

void fan_update(uint8_t value)
{
	DAC_SetChannel1Data(DAC_Align_12b_R, 4096 - (((int)value) + 1) * 16);
}

void fan_track(uint32_t diff_ms)
{
	if(TIM3->CCR4 != fan_prev_capt)
	{
		fan_vel = (float)(84000000UL / (TIM2->PSC + 1) / TIM2->CCR1);
		fan_capt_to = 200;
		fan_prev_capt = TIM3->CCR4;
	}
	else
	{
		fan_capt_to = fan_capt_to >= diff_ms ? fan_capt_to - diff_ms : 0;
		if(fan_capt_to == 0) fan_vel = 0;
	}
}

float fan_get_vel(void) { return fan_vel; }

void safety_reset_ser_to(void) { to_ser = TO_SER_THRS; }

void safety_loop(uint32_t diff_ms)
{
	fan_ctrl(diff_ms);

	to_ser = to_ser >= diff_ms ? to_ser - diff_ms : 0;

	error_set(ERR_STM_KEY, GPIOC->IDR & (1 << 7));
	error_set(ERR_STM_FAN, fan_get_vel() == 0);
	error_set(ERR_STM_RB, GPIOA->IDR & (1 << 8));
	error_set(ERR_STM_TO, to_ser == 0);

	// ERR_STM_CFG,

	const bool error_p24 = adc_val.vp24 > V_PS_NOM + V_PS_THRSH ||
						   adc_val.vp24 < V_PS_NOM - V_PS_THRSH;
	const bool error_n24 = adc_val.vn24 > -V_PS_NOM + V_PS_THRSH ||
						   adc_val.vn24 < -V_PS_NOM - V_PS_THRSH;
	error_set(ERR_STM_PE, error_p24 || error_n24);
	error_set(ERR_STM_PS, !(GPIOC->IDR & (1 << 3)));

	const bool error_temp = adc_val.t[0] > TEMP_THRS_CRITICAL ||
							adc_val.t[1] > TEMP_THRS_CRITICAL ||
							adc_val.t_amp > TEMP_THRS_CRITICAL;
	const bool error_temp_head = adc_val.t_head > TEMP_THRS_CRITICAL_HEAD;
	error_set(ERR_STM_OT, error_temp || error_temp_head);

	error_set(ERR_STM_IMU_MOVE, check_imu_fix());

	safety_lsr_set_used_by_esp(esp_sts.lpc_on);

	if(error_get() == 0 && esp_sts.err == 0)
	{
		if(esp_sts.pwr_on && lsr_ctrl_used_by_esp && is_pwr_enabled == false)
		{
			safety_enable_power();
		}
		else if(esp_sts.pwr_on == false && lsr_ctrl_used_by_esp == false && is_pwr_enabled && lsr_ctrl_used_by_stm == false)
		{
			safety_disable_power_and_ctrl();
		}
	}

	if(reset_timer)
	{
		reset_timer = reset_timer >= diff_ms ? reset_timer - diff_ms : 0;

		if(error_get() == 0) reset_timer = 0;
		safety_lock = false;
		error_reset();
		imu_read();
		imu_lock_fixture = imu_val;
	}
	else
	{
		if(error_get()) safety_lock = true;

		if(safety_lock)
		{
			GALV_PWR_DIS;
			LSR_PWR_DIS;
		}
		error_set(ERR_STM_SFTY, safety_lock);
	}
}

void safety_reset_lock(void)
{
	if(esp_sts.err & (uint32_t) ~(1 << ERR_ESP_SFTY)) return;

	if(error_get() & (uint32_t) ~((1 << ERR_STM_SFTY) |
								  (1 << ERR_STM_IMU_MOVE))) return;

	reset_timer = TIMER_VAL_RESET_ERR;
	GALV_PWR_DIS;
	LSR_PWR_DIS;
}

void safety_enable_power_and_ctrl(void)
{
	if(error_get()) return;
	if(lsr_ctrl_used_by_esp) return;

	lsr_ctrl_used_by_stm = true;
	lsr_ctrl_init_enable();

	GALV_PWR_EN;
	LSR_PWR_EN;
	is_pwr_enabled = true;
}

void safety_enable_power(void)
{
	if(error_get()) return;

	GALV_PWR_EN;
	LSR_PWR_EN;
	is_pwr_enabled = true;
}

void safety_disable_power_and_ctrl(void)
{
	GALV_PWR_DIS;
	LSR_PWR_DIS;
	lsr_ctrl_init_disable();
	lsr_ctrl_used_by_stm = false;
	is_pwr_enabled = false;
}

void safety_disable(void)
{
	safety_lock = true;
	GALV_PWR_DIS;
	LSR_PWR_DIS;
	lsr_ctrl_init_disable();
	lsr_ctrl_used_by_stm = false;
	is_pwr_enabled = false;
}

void safety_lsr_set_used_by_esp(bool state) { lsr_ctrl_used_by_esp = state; }
uint8_t safety_lsr_is_used_by_stm(void) { return lsr_ctrl_used_by_stm; }
uint8_t safety_is_pwr_enabled(void) { return is_pwr_enabled; }
