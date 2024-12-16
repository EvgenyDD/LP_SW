#include "ui.h"
#include "adc.h"
#include "buttons.h"
#include "error.h"
#include "fram.h"
#include "i2c_display.h"
#include "imu.h"
#include "lsr_ctrl.h"
#include "opt3001.h"
#include "platform.h"
#include "proto_l0.h"
#include "safety.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern bool voltage_enabled;

#define M_M(_item, _next, _prev, _cb, _txt) \
	extern ui_item_t _next;                 \
	extern ui_item_t _prev;                 \
	extern void _cb(bool, ui_item_t *);     \
	ui_item_t _item = {&_next, &_prev, _cb, _txt}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wredundant-decls"
typedef struct ui_item_t ui_item_t;
typedef void (*m_cb)(bool, ui_item_t *);
typedef struct ui_item_t
{
	ui_item_t *next;
	ui_item_t *prev;
	m_cb cb;
	const char *text;
} ui_item_t;

#define UPD_INTERVAL 100 // ms

M_M(m_main, m_info1, m_settings, cb_main, "-AUTO -");
M_M(m_info1, m_info2, m_main, cb_info1, "-Info 1-");
M_M(m_info2, m_light, m_info1, cb_info2, "-Info 2-");
M_M(m_light, m_play, m_info2, cb_light, "-Light-");
M_M(m_play, m_journal, m_light, cb_play, "- Play -");
M_M(m_journal, m_settings, m_play, cb_journal, "Journal");
M_M(m_settings, m_main, m_journal, cb_sett, "Settings");

#pragma GCC diagnostic pop

static ui_item_t *cur = &m_main;
static uint16_t upd_timer = 0;
static char buffer[128];

static void chng_menu(ui_item_t *item)
{
	upd_timer = 0;
	cur = item;
	i2c_display_clear_screen(false);
}

static bool is_err(void)
{
	return (esp_sts.err & (uint32_t) ~(1 << ERR_ESP_SFTY)) || error_get();
}

void cb_main(bool redraw, ui_item_t *current)
{
	static uint8_t sel = 0;

	if(btn_ju.state == BTN_PRESS_SHOT) sel = sel == 0 ? 2 : sel - 1;
	if(btn_jd.state == BTN_PRESS_SHOT) sel = sel >= 3 ? 0 : sel + 1;

	switch(sel)
	{
	default:
	case 0:
		if(btn_jl.state == BTN_PRESS_SHOT) g_fram_data.lp_flags &= (uint32_t) ~(1 << CFG_FLAG_BIT_IMU);
		if(btn_jr.state == BTN_PRESS_SHOT) g_fram_data.lp_flags |= (1 << CFG_FLAG_BIT_IMU);
		break;

	case 1:
		if(btn_jl.state == BTN_PRESS_SHOT) g_fram_data.lp_flags &= (uint32_t) ~(1 << CFG_FLAG_BIT_GAMMA);
		if(btn_jr.state == BTN_PRESS_SHOT) g_fram_data.lp_flags |= (1 << CFG_FLAG_BIT_GAMMA);
		if(btn_jl.state == BTN_PRESS_SHOT || btn_jr.state == BTN_PRESS_SHOT)
		{
			fram_write(0, &g_fram_data, sizeof(g_fram_data));
			proto_send_param(PROTO_CMD_PARAM_LP_COLOR_MODE, &g_fram_data.lp_flags, sizeof(uint32_t));
		}
		break;

	case 2:
		if(btn_jl.state == BTN_PRESS_SHOT) g_fram_data.lp_flags &= (uint32_t) ~(1 << CFG_FLAG_BIT_FULL_POWER);
		if(btn_jr.state == BTN_PRESS_SHOT) g_fram_data.lp_flags |= (1 << CFG_FLAG_BIT_FULL_POWER);
		if(btn_jl.state == BTN_PRESS_SHOT || btn_jr.state == BTN_PRESS_SHOT)
		{
			fram_write(0, &g_fram_data, sizeof(g_fram_data));
			proto_send_param(PROTO_CMD_PARAM_LP_COLOR_MODE, &g_fram_data.lp_flags, sizeof(uint32_t));
		}
		break;
	}

	if(redraw)
	{
		sprintf(buffer, "%s", current->text);
		i2c_display_display_text(0, 0, buffer, strlen(buffer), is_err());

		sprintf(buffer, "I %.1fV  ", adc_val.vin);
		i2c_display_display_text(1, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "  %.2fA  ", adc_val.i24);
		i2c_display_display_text(2, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "P %.1fW  ", adc_val.vp24 * adc_val.i24);
		i2c_display_display_text(3, 0, buffer, strlen(buffer), false);

		float max_temp = adc_val.t[0];
		if(max_temp < adc_val.t[1]) max_temp = adc_val.t[1];
		if(max_temp < adc_val.t_amp) max_temp = adc_val.t_amp;
		sprintf(buffer, "t %.0f %.0f  ", max_temp, adc_val.t_head);
		i2c_display_display_text(4, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "FAN %.0f%%  ", map(fan_get_vel(), FAN_VEL_MIN, FAN_VEL_MAX, 0, 100));
		i2c_display_display_text(5, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "%c IMU: %d ", sel == 0 ? '>' : ' ', (g_fram_data.lp_flags & (1 << CFG_FLAG_BIT_IMU)) ? 1 : 0);
		i2c_display_display_text(7, 0, buffer, strlen(buffer), sel == 0);
		sprintf(buffer, "%c G:   %d ", sel == 1 ? '>' : ' ', (g_fram_data.lp_flags & (1 << CFG_FLAG_BIT_GAMMA)) ? 1 : 0);
		i2c_display_display_text(8, 0, buffer, strlen(buffer), sel == 1);
		sprintf(buffer, "%c PWR: %d ", sel == 2 ? '>' : ' ', (g_fram_data.lp_flags & (1 << CFG_FLAG_BIT_FULL_POWER)) ? 1 : 0);
		i2c_display_display_text(9, 0, buffer, strlen(buffer), sel == 2);

		uint32_t j = 0;
		if(esp_sts.err)
		{
			sprintf(buffer, " x%lx          ", (uint32_t)esp_sts.err);
			i2c_display_display_text(11 + j++, 0, buffer, strlen(buffer), false);
		}
		for(uint32_t i = 0; i < ERR_STM_COUNT; i++)
		{
			if(error_get() & (1 << i))
			{
				sprintf(buffer, "%s          ", error_get_str(i));
				i2c_display_display_text(11 + j++, 0, buffer, strlen(buffer), false);
			}
		}
		for(uint32_t i = 0; i < ERR_STM_COUNT; i++)
		{
			if((error_get_latched() & (1 << i)) &&
			   !(error_get() & (1 << i)))
			{
				sprintf(buffer, "%s          ", error_get_str(i));
				i2c_display_display_text(11 + j++, 0, buffer, strlen(buffer), true);
			}
		}

		for(; j < 6; j++)
		{
			sprintf(buffer, "         ");
			i2c_display_display_text(11 + j, 0, buffer, strlen(buffer), false);
		}
	}

	if(btn_act[0].state & BTN_PRESS && btn_act[2].state & BTN_PRESS)
		safety_reset_lock();

	if(btn_act[1].state == BTN_PRESS_SHOT)
		chng_menu(cur->next);
}

void cb_info1(bool redraw, ui_item_t *current)
{
	if(redraw)
	{
		sprintf(buffer, "%s", current->text);
		i2c_display_display_text(0, 0, buffer, strlen(buffer), is_err());

		sprintf(buffer, "I %.1fV  ", adc_val.vin);
		i2c_display_display_text(1, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "  %.2fA  ", adc_val.i24);
		i2c_display_display_text(2, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "P %.1fW  ", adc_val.vp24 * adc_val.i24);
		i2c_display_display_text(3, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "O %+.1fV  ", adc_val.vp24);
		i2c_display_display_text(5, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "O %+.1fV  ", adc_val.vn24);
		i2c_display_display_text(6, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "t %.0f %.0f  ", adc_val.t[0], adc_val.t[1]);
		i2c_display_display_text(8, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "t %.0f %.0f  ", adc_val.t_amp, adc_val.t_head);
		i2c_display_display_text(9, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "FAN %.0f  ", fan_get_vel());
		i2c_display_display_text(11, 0, buffer, strlen(buffer), false);

		uint32_t j = 0;
		if(esp_sts.err)
		{
			sprintf(buffer, " x%lx          ", (uint32_t)esp_sts.err);
			i2c_display_display_text(12 + j++, 0, buffer, strlen(buffer), false);
		}
		for(uint32_t i = 0; i < ERR_STM_COUNT; i++)
		{
			if(error_get() & (1 << i))
			{
				sprintf(buffer, "%s          ", error_get_str(i));
				i2c_display_display_text(12 + j++, 0, buffer, strlen(buffer), false);
			}
		}
		for(uint32_t i = 0; i < ERR_STM_COUNT; i++)
		{
			if((error_get_latched() & (1 << i)) &&
			   !(error_get() & (1 << i)))
			{
				sprintf(buffer, "%s          ", error_get_str(i));
				i2c_display_display_text(12 + j++, 0, buffer, strlen(buffer), true);
			}
		}

		for(; j < 6; j++)
		{
			sprintf(buffer, "         ");
			i2c_display_display_text(12 + j, 0, buffer, strlen(buffer), false);
		}
	}

	if(btn_act[0].state & BTN_PRESS && btn_act[2].state & BTN_PRESS)
		safety_reset_lock();

	if(btn_act[1].state == BTN_PRESS_SHOT)
		chng_menu(cur->next);
}

void cb_info2(bool redraw, ui_item_t *current)
{
	if(redraw)
	{
		sprintf(buffer, "%s", current->text);
		i2c_display_display_text(0, 0, buffer, strlen(buffer), is_err());

		sprintf(buffer, "%ld    ", g_fram_data.turn_on_cnt);
		i2c_display_display_text(1, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "%ld    ", (uint32_t)(g_fram_data.work_time_ms / 1000));
		i2c_display_display_text(2, 0, buffer, strlen(buffer), false);

		sprintf(buffer, opt3001_lux < 100 ? "%.2f Lx     " : "%.0fLx     ", opt3001_lux);
		i2c_display_display_text(8, 0, buffer, strlen(buffer), false);

		const char axes[3] = "xyz";
		for(uint32_t i = 0; i < 3; i++)
		{
			sprintf(buffer, "%c%+6.1f ", axes[i], imu_val.g[i]);
			i2c_display_display_text(10 + i, 0, buffer, strlen(buffer), false);
		}
		for(uint32_t i = 0; i < 3; i++)
		{
			sprintf(buffer, "%c%+6.3f ", axes[i], imu_val.xl[i]);
			i2c_display_display_text(13 + i, 0, buffer, strlen(buffer), false);
		}
	}

	if(btn_act[1].state == BTN_PRESS_SHOT)
		chng_menu(cur->next);
}

void cb_light(bool redraw, ui_item_t *current)
{
	static uint8_t color_sel = 0, values[3] = {0}, light_cmd = 2;

	if(btn_jl.state == BTN_PRESS_SHOT) values[color_sel] = values[color_sel] <= 10 ? 0 : values[color_sel] - 10;
	if(btn_jr.state == BTN_PRESS_SHOT) values[color_sel] = values[color_sel] >= 90 ? 100 : values[color_sel] + 10;
	if(btn_ju.state == BTN_PRESS_SHOT) color_sel = color_sel == 0 ? 2 : color_sel - 1;
	if(btn_jd.state == BTN_PRESS_SHOT) color_sel = color_sel >= 2 ? 0 : color_sel + 1;

	if(redraw)
	{
		sprintf(buffer, "%s", current->text);
		i2c_display_display_text(0, 0, buffer, strlen(buffer), is_err());

		for(uint32_t i = 0; i < 3; i++)
		{
			sprintf(buffer, " %c %d%%         ", "RGB"[i], values[i]);
			i2c_display_display_text(2 + i, 0, buffer, strlen(buffer), color_sel == i);
		}
		sprintf(buffer, "%s", light_cmd ? " LIGHT  " : "        ");
		i2c_display_display_text(12, 0, buffer, strlen(buffer), light_cmd);
	}

	if(btn_act[0].state & BTN_PRESS && btn_act[2].state & BTN_PRESS)
	{
		if(light_cmd != 1)
		{
			safety_enable_power_and_ctrl();
#if 0
			uint16_t raw[] = {2048, 2048,
							  (int32_t)gamma_256[values[0] * 255 / 100] * 4095 / 256,
							  (int32_t)gamma_256[values[1] * 255 / 100] * 4095 / 256,
							  (int32_t)gamma_256[values[2] * 255 / 100] * 4095 / 256};
#else
			uint16_t raw[] = {2048, 2048,
							  values[0] * 4095 / 100,
							  values[1] * 4095 / 100,
							  values[2] * 4095 / 100};
#endif
			delay_ms(100); // ???
			lsr_ctrl_direct_cmd(raw);
			light_cmd = 1;
		}
	}
	else
	{
		// black cmd
		if(light_cmd != 0)
		{
			light_cmd = 0;
			uint16_t raw[] = {2048, 2048, 0, 0, 0};
			lsr_ctrl_direct_cmd(raw);
			delay_ms(100); // ???
			safety_disable_power_and_ctrl();
		}
	}

	if(btn_act[1].state == BTN_PRESS_SHOT)
	{
		safety_disable_power_and_ctrl();
		light_cmd = 2;
		chng_menu(cur->next);
	}
}

void cb_play(bool redraw, ui_item_t *current)
{
	if(redraw)
	{
		sprintf(buffer, "%s", current->text);
		i2c_display_display_text(0, 0, buffer, strlen(buffer), is_err());
	}
	if(btn_act[1].state == BTN_PRESS_SHOT)
		chng_menu(cur->next);
}

void cb_sett(bool redraw, ui_item_t *current)
{
	if(redraw)
	{
		sprintf(buffer, "%s", current->text);
		i2c_display_display_text(0, 0, buffer, strlen(buffer), is_err());
	}
	if(btn_act[1].state == BTN_PRESS_SHOT)
		chng_menu(cur->next);
}

void cb_journal(bool redraw, ui_item_t *current)
{
	if(redraw)
	{
		sprintf(buffer, "%s", current->text);
		i2c_display_display_text(0, 0, buffer, strlen(buffer), is_err());
	}
	if(btn_act[1].state == BTN_PRESS_SHOT)
		chng_menu(cur->next);
}

void ui_init(void)
{
	i2c_display_init();
	i2c_display_direction(DIRECTION180);
	i2c_display_clear_screen(0);
}

void ui_loop(uint32_t ms_diff)
{
	cur->cb(upd_timer <= ms_diff, cur);
	upd_timer = upd_timer <= ms_diff ? UPD_INTERVAL : upd_timer - ms_diff;
}