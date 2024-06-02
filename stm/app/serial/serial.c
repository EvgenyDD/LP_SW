#include "serial.h"
#include "cobs.h"
#include "platform.h"
#include "proto.h"
#include "proto_l0.h"
#include "serial.h"

static volatile uint8_t usart_rx_dma_buffer[PROTO_MAX_PKT_SIZE];
static volatile uint32_t pos_rx_old = 0;
static volatile bool uart_is_transmit = 0;
static uint8_t buf_cobs_encode[PROTO_MAX_PKT_SIZE + 2];

void serial_init(void)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

	USART_DeInit(USART1);
	USART_InitTypeDef USART_InitStruct;
	USART_InitStruct.USART_BaudRate = 2000000;
	USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART1, &USART_InitStruct);
	USART_Cmd(USART1, ENABLE);
	USART_DMACmd(USART1, USART_DMAReq_Tx | USART_DMAReq_Rx, ENABLE);

	// RX
	DMA_InitTypeDef DMA_InitStructure;
	DMA_DeInit(DMA2_Stream5);
	DMA_InitStructure.DMA_Channel = DMA_Channel_4;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)usart_rx_dma_buffer;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = PROTO_MAX_PKT_SIZE;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA2_Stream5, &DMA_InitStructure);
	DMA_Cmd(DMA2_Stream5, ENABLE);

	// TX
	DMA_DeInit(DMA2_Stream7);
	DMA_InitStructure.DMA_Channel = DMA_Channel_4;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)buf_cobs_encode;
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_InitStructure.DMA_BufferSize = 1;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA2_Stream7, &DMA_InitStructure);

	DMA_ITConfig(DMA2_Stream5, DMA_IT_HT, ENABLE);
	DMA_ITConfig(DMA2_Stream5, DMA_IT_TC, ENABLE);
	DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream5_IRQn;
	NVIC_Init(&NVIC_InitStructure);
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream7_IRQn;
	NVIC_Init(&NVIC_InitStructure);
}

void serial_rx_check(void)
{
	uint32_t pos_rx = PROTO_MAX_PKT_SIZE - DMA_GetCurrDataCounter(DMA2_Stream5);
	if(pos_rx != pos_rx_old)
	{
		if(pos_rx > pos_rx_old)
		{
			proto_append((const volatile uint8_t *)&usart_rx_dma_buffer[pos_rx_old], pos_rx - pos_rx_old);
		}
		else
		{
			proto_append((const volatile uint8_t *)&usart_rx_dma_buffer[pos_rx_old], PROTO_MAX_PKT_SIZE - pos_rx_old);
			if(pos_rx > 0) proto_append((const volatile uint8_t *)&usart_rx_dma_buffer[0], pos_rx);
		}
		pos_rx_old = pos_rx;
	}
}

void DMA2_Stream5_IRQHandler(void) // RX
{
	if(DMA_GetITStatus(DMA2_Stream5, DMA_IT_TCIF5) == SET)
	{
		DMA_ClearITPendingBit(DMA2_Stream5, DMA_IT_TCIF5);
		serial_rx_check();
	}
	if(DMA_GetITStatus(DMA2_Stream5, DMA_IT_HTIF5) == SET)
	{
		DMA_ClearITPendingBit(DMA2_Stream5, DMA_IT_HTIF5);
		serial_rx_check();
	}
}

void DMA2_Stream7_IRQHandler(void) // TX
{
	if(DMA_GetITStatus(DMA2_Stream7, DMA_IT_TCIF7) == SET)
	{
		DMA_ClearITPendingBit(DMA2_Stream7, DMA_IT_TCIF7);
		uart_is_transmit = 0;
	}
}

void serial_tx(const uint8_t *data, uint32_t len)
{
	if(uart_is_transmit > 0 || len + 2 > PROTO_MAX_PKT_SIZE + 2) return;

	volatile uint32_t old_primask = __get_PRIMASK();
	__disable_irq();
	{
		DMA_Cmd(DMA2_Stream7, DISABLE);
		while(DMA_GetCmdStatus(DMA2_Stream7) != DISABLE)
			;

		DMA_ClearFlag(DMA2_Stream7, DMA_FLAG_TCIF7);
		DMA_ClearFlag(DMA2_Stream7, DMA_FLAG_HTIF7);
		DMA_ClearFlag(DMA2_Stream7, DMA_FLAG_TEIF7);
		DMA_ClearFlag(DMA2_Stream7, DMA_FLAG_FEIF7);

		uint32_t sz = 1 + cobs_encode(data, len, &buf_cobs_encode[1]);
		DMA_SetCurrDataCounter(DMA2_Stream7, sz);
		DMA_MemoryTargetConfig(DMA2_Stream7, (uint32_t)buf_cobs_encode, DMA_Memory_0);
		DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, ENABLE);

		DMA_Cmd(DMA2_Stream7, ENABLE);
	}
	__set_PRIMASK(old_primask);
}