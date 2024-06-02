#include "buttons.h"
#include "debounce.h"
#include "platform.h"

debounce_t btn_act[3], btn_jl, btn_jr, btn_jok, btn_ju, btn_jd, btn_emcy, sd_detect_debounce;

void buttons_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5 | GPIO_Pin_8 /* SD detect */;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOC, &GPIO_InitStruct);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2;
	GPIO_Init(GPIOD, &GPIO_InitStruct);

	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	debounce_init(&btn_act[0], 2000);
	debounce_init(&btn_act[1], 2000);
	debounce_init(&btn_act[2], 2000);
	debounce_init(&btn_jl, 2000);
	debounce_init(&btn_jr, 2000);
	debounce_init(&btn_jok, 2000);
	debounce_init(&btn_ju, 2000);
	debounce_init(&btn_jd, 2000);
	debounce_init(&btn_emcy, 2000);
	debounce_init(&sd_detect_debounce, 2000);
}

void buttons_poll(uint32_t diff_ms)
{
	debounce_update(&btn_act[0], !(GPIOB->IDR & (1 << 5)), diff_ms);
	debounce_update(&btn_act[1], !(GPIOD->IDR & (1 << 2)), diff_ms);
	debounce_update(&btn_act[2], !(GPIOB->IDR & (1 << 3)), diff_ms);
	debounce_update(&btn_jl, !(GPIOC->IDR & (1 << 14)), diff_ms);
	debounce_update(&btn_jr, !(GPIOC->IDR & (1 << 9)), diff_ms);
	debounce_update(&btn_jok, !(GPIOC->IDR & (1 << 15)), diff_ms);
	debounce_update(&btn_ju, !(GPIOC->IDR & (1 << 8)), diff_ms);
	debounce_update(&btn_jd, !(GPIOC->IDR & (1 << 13)), diff_ms);
	debounce_update(&btn_emcy, GPIOA->IDR & (1 << 8), diff_ms);
	debounce_update(&sd_detect_debounce, !(GPIOB->IDR & (1 << 8)), diff_ms);
}