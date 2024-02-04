#include "lsr_ctrl.h"
#include "stm32f4xx.h"

#define LDAC_H GPIOC->BSRRL = (1 << 11)
#define LDAC_L GPIOC->BSRRH = (1 << 11)

#define CNT_H GPIOB->BSRRL = (1 << 11)
#define CNT_L GPIOB->BSRRH = (1 << 11)

#define GALV_PWR_H GPIOB->BSRRL = (1 << 10)
#define GALV_PWR_L GPIOB->BSRRH = (1 << 10)

#define LSR_PWR_H GPIOB->BSRRL = (1 << 9)
#define LSR_PWR_L GPIOB->BSRRH = (1 << 9)

void lsr_ctrl_init(void)
{
	lsr_ctrl_init_disable();

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10; // LSR_EN GLV_PWR_EN
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);

	SPI_I2S_DeInit(SPI3);

	SPI_InitTypeDef SPI_InitStructure;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; // 10.5 MHz
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI3, &SPI_InitStructure);

	SPI_CalculateCRC(SPI3, DISABLE);

	SPI_SSOutputCmd(SPI3, DISABLE);
	SPI_Cmd(SPI3, ENABLE);

	GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);
}

void lsr_ctrl_init_disable(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11; // LP_CNT
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_10 | GPIO_Pin_12; // LDAC, SCK, MOSI
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void lsr_ctrl_init_enable(int x)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11; // LP_CNT
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11; // LDAC
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_12; // SCK, MOSI
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	CNT_H;
	LDAC_H;

	GALV_PWR_H; // remove here
	LSR_PWR_L;	// remove here

	uint16_t raw[] = {x ? 2100 : 2000, x ? 2100 : 2000, 0, 0, 0};
	lsr_ctrl_direct_cmd(raw);
}

#define S_WT                                                      \
	while(SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET) \
		;

#define S_DT(x) SPI_I2S_SendData(SPI3, (uint16_t)x);

#define SPI_WAIT_BSY                                     \
	while(SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_BSY)) \
		;

void lsr_ctrl_direct_cmd(uint16_t val[5])
{
	CNT_L;
	CNT_H;
	S_DT((1 << 15) | (1 << 13) | (1 << 12) | val[0]); // X
	S_WT;
	SPI_WAIT_BSY;

	CNT_L;
	CNT_H;
	S_DT((1 << 15) | (1 << 13) | (1 << 12) | val[2]);
	S_WT;
	SPI_WAIT_BSY;

	CNT_L;
	CNT_H;
	S_DT((0 << 15) | (1 << 13) | (1 << 12) | val[1]); // Y
	S_WT;
	SPI_WAIT_BSY;

	CNT_L;
	CNT_H;
	S_DT((0 << 15) | (1 << 13) | (1 << 12) | val[4]);
	S_WT;
	SPI_WAIT_BSY;

	CNT_L;
	CNT_H;
	S_DT((1 << 15) | (1 << 13) | (1 << 12) | val[3]);
	S_WT;
	SPI_WAIT_BSY;

	CNT_L;
	CNT_H;
	LDAC_L;
	LDAC_H;
}