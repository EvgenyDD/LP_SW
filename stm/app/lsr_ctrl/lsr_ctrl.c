#include "lsr_ctrl.h"
#include "stm32f4xx.h"

#define LDAC_H GPIOC->BSRRL = (1 << 11)
#define LDAC_L GPIOC->BSRRH = (1 << 11)

#define CNT_H GPIOB->BSRRL = (1 << 11)
#define CNT_L GPIOB->BSRRH = (1 << 11)

static const uint16_t default_cmd[] = {2048, 2048, 0, 0, 0};

const uint8_t gamma_256[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
								1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
								2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
								5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
								10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
								17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
								25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
								37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
								51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
								69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
								90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
								115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
								144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
								177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
								215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255};

#define DELAY ;

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
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
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
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11; // LDAC
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_12; // SCK, MOSI
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);

	CNT_H;
	LDAC_H;

	lsr_ctrl_direct_cmd(default_cmd);
}

#define S_WT                                                      \
	while(SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET) \
		;

#define S_DT(x) SPI_I2S_SendData(SPI3, (uint16_t)x);

#define SPI_WAIT_BSY                                     \
	while(SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_BSY)) \
		;

void lsr_ctrl_direct_cmd(const uint16_t val[5])
{
	CNT_L;
	DELAY;
	CNT_H;
	S_DT((1 << 15) | (1 << 13) | (1 << 12) | val[0]); // X
	S_WT;
	SPI_WAIT_BSY;

	CNT_L;
	DELAY;
	CNT_H;
	S_DT((1 << 15) | (1 << 13) | (1 << 12) | val[2]);
	S_WT;
	SPI_WAIT_BSY;

	CNT_L;
	DELAY;
	CNT_H;
	S_DT((0 << 15) | (1 << 13) | (1 << 12) | val[1]); // Y
	S_WT;
	SPI_WAIT_BSY;

	CNT_L;
	DELAY;
	CNT_H;
	S_DT((0 << 15) | (1 << 13) | (1 << 12) | val[4]);
	S_WT;
	SPI_WAIT_BSY;

	CNT_L;
	DELAY;
	CNT_H;
	S_DT((1 << 15) | (1 << 13) | (1 << 12) | val[3]);
	S_WT;
	SPI_WAIT_BSY;

	CNT_L;
	DELAY;
	CNT_H;
	DELAY;
	LDAC_L;
	DELAY;
	LDAC_H;
	DELAY;
}