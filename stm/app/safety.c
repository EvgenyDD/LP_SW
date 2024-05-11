#include "safety.h"
#include "adc.h"
#include "buttons.h"
#include "error.h"
#include "lsr_ctrl.h"
#include "platform.h"

#define GALV_PWR_EN GPIOB->BSRRL = (1 << 10)
#define GALV_PWR_DIS GPIOB->BSRRH = (1 << 10)

#define LSR_PWR_EN GPIOB->BSRRH = (1 << 9)
#define LSR_PWR_DIS GPIOB->BSRRL = (1 << 9)

#define TEMP_THRS 32
#define TEMP_THRS_CRITICAL 60
#define TIMER_VAL_RESET_ERR 500

static bool safety_lock = true;
static uint32_t reset_timer = 0;

static uint32_t fan_prev_capt = 0;
static float fan_vel = 0;
static uint32_t fan_capt_to = 0;

void fan_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

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
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource15, GPIO_AF_TIM2);

	DAC_InitTypeDef DAC_InitStructure;
	DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;
	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
	DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = 0;
	DAC_Init(DAC_Channel_1, &DAC_InitStructure);

	DAC_Cmd(DAC_Channel_1, ENABLE);

	DAC_SetChannel1Data(DAC_Align_12b_R, 4095);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_ICInitTypeDef TIM2_ICInitStructure;

	TIM_TimeBaseStructure.TIM_Period = 65535;
	TIM_TimeBaseStructure.TIM_Prescaler = 8400 - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	TIM2_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM2_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM2_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM2_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM2_ICInitStructure.TIM_ICFilter = 15;
	TIM_ICInit(TIM2, &TIM2_ICInitStructure);

	TIM_Cmd(TIM2, ENABLE);
	TIM_CCxCmd(TIM2, TIM_Channel_1, TIM_CCx_Enable);
}

void fan_update(uint8_t value)
{
	DAC_SetChannel1Data(DAC_Align_12b_R, 4096 - (((int)value) + 1) * 16);
}

void fan_track(uint32_t diff_ms)
{
	if(TIM2->CCR1 != fan_prev_capt)
	{
		float period = TIM2->CCR1 >= fan_prev_capt ? TIM2->CCR1 - fan_prev_capt : (TIM2->ARR + 1) - fan_prev_capt + TIM2->CCR1;
		fan_vel = 84000000.0f / (float)(TIM2->PSC + 1) / period;
		fan_capt_to = 200;
		fan_prev_capt = TIM2->CCR1;
	}
	else
	{
		fan_capt_to = fan_capt_to >= diff_ms
						  ? fan_capt_to - diff_ms
						  : 0;
		if(fan_capt_to == 0) fan_vel = 0;
	}
}

float fan_get_vel(void) { return fan_vel; }

void safety_loop(uint32_t diff_ms)
{
	float maxt = -373.15f;
	for(uint32_t i = 0; i < 4; i++)
	{
		if(adc_val.t[i] / 10 > maxt) maxt = adc_val.t[i] / 10;
	}

	fan_update(map(maxt, TEMP_THRS, TEMP_THRS_CRITICAL, 0, 255));

	if(reset_timer)
	{
		reset_timer = reset_timer >= diff_ms ? reset_timer - diff_ms : 0;
		if(!btn_emcy.state)
		{
			safety_lock = false;
		}
	}
	else
	{
		if(btn_emcy.state == BTN_PRESS)
		{
			safety_lock = true;
		}

		if(safety_lock)
		{
			// GALV_PWR_DIS;
			// LSR_PWR_DIS;
		}
	}

	error_set(ERROR_SFTY, safety_lock);
}

void safety_reset_lock(void)
{
	if(adc_val.t[0] / 10 > TEMP_THRS_CRITICAL ||
	   adc_val.t[1] / 10 > TEMP_THRS_CRITICAL ||
	   adc_val.t[2] / 10 > TEMP_THRS_CRITICAL ||
	   adc_val.t[3] / 10 > TEMP_THRS_CRITICAL) return;

	reset_timer = TIMER_VAL_RESET_ERR;

	// GALV_PWR_EN;
	// LSR_PWR_EN;
}

void safety_enable(void)
{
	lsr_ctrl_init_enable(0);
	GALV_PWR_EN;
	LSR_PWR_EN;
}

void safety_disable(void)
{
	GALV_PWR_DIS;
	LSR_PWR_DIS;
	lsr_ctrl_init_disable();
}