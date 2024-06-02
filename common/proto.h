#ifndef PROTO_H__
#define PROTO_H__

/**
 * UART protocol
 *
 * Format:
 *  L CMD PL CRC16
 *      L - length of the whole packet, uint16
 *      CMD - command code, from ::PROTO_CMD_t
 *      PL - command payload (data)
 *      CRC16 - CRC-CCITT
 */

#include <stdint.h>

#define PROTO_MAX_PKT_SIZE (256 - 2 /* end/start markers*/ - 1)

typedef enum
{
	PROTO_CMD_STS = 0,
	PROTO_CMD_RESET_ERRORS = 0,
} PROTO_CMD_t;

/**
 * PROTO_CMD_STS format:
 *  set of key+value pairs
 *      key: uint8_t, from ::PROTO_STS_t
 *      value: optional
 */

typedef enum
{
	PROTO_STS_SW_TYPE_BOOT = 0, // 0
	PROTO_STS_SW_TYPE_FW,		// 0
	PROTO_STS_ERR,				// u32
	PROTO_STS_ERR_LATCHED,		// u32
	PROTO_STS_U_IN,				// float
	PROTO_STS_U_P24,			// float
	PROTO_STS_U_N24,			// float
	PROTO_STS_I_24,				// float
	PROTO_STS_T_INV0,			// float
	PROTO_STS_T_INV1,			// float
	PROTO_STS_T_AMP,			// float
	PROTO_STS_T_HEAD,			// float
	PROTO_STS_T_MCU,			// float
	PROTO_STS_VEL_FAN,			// float
	PROTO_STS_LUM,				// float
	PROTO_STS_XL_X,				// float
	PROTO_STS_XL_Y,				// float
	PROTO_STS_XL_Z,				// float
	PROTO_STS_G_X,				// float
	PROTO_STS_G_Y,				// float
	PROTO_STS_G_Z,				// float
	PROTO_STS_INCR_CNT,			// u32
	PROTO_STS_LPC_ACT,			// u8, laser projector control active (GPIO)
	PROTO_STS_GALV_LSR_PWR_EN,	// u8
	PROTO_STS_SD_CARD_INSRT,	// u8
	PROTO_STS_LSR_EN,			// u8

} PROTO_STS_t;

typedef enum
{
	ERR_STM_SFTY = 0,
	ERR_STM_CFG,
	ERR_STM_FRAM,
	ERR_STM_RB,
	ERR_STM_TO,
	ERR_STM_OT,
	ERR_STM_PS,
	ERR_STM_PE, // power error
	ERR_STM_FAN,
	ERR_STM_KEY,
	ERR_STM_IMU_MOVE,
	ERR_STM_PROTO,

	ERR_STM_COUNT
} ERR_STM_t;

typedef enum
{
	ERR_ESP_SFTY = 0,
	ERR_ESP_TO,
	ERR_ESP_PROTO,

	ERR_ESP_COUNT
} ERR_ESP_t;

typedef union
{
	uint8_t byte;
	struct
	{
		uint8_t shrt_press_upd : 2;
		uint8_t shrt_release_upd : 2;
		uint8_t long_press_upd : 2;
		uint8_t long_release_upd : 2;
	} cnt;
} button_upd_cnt_t;

#endif // PROTO_H__