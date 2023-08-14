#include "ui.h"
#include "components/i2c_adc.h"
#include "components/i2c_display.h"
#include "components/i2c_exp.h"
#include "lp.h"
#include "safety.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern void sd_card_test(void);
extern bool voltage_enabled;
extern uint32_t fr;

#define M_M(_item, _next, _prev, _cb, _txt)   \
	extern ui_item_t _next;                   \
	extern ui_item_t _prev;                   \
	extern void _cb(bool, bool, ui_item_t *); \
	ui_item_t _item = {&_next, &_prev, _cb, _txt}

typedef struct ui_item_t ui_item_t;
typedef void (*m_cb)(bool, bool, ui_item_t *);
typedef struct ui_item_t
{
	ui_item_t *next;
	ui_item_t *prev;
	m_cb cb;
	const char *text;
} ui_item_t;

static ui_item_t null_menu = {0};

#define UPD_INTERVAL 30 // ms

M_M(m_common, m_play, m_settings, cb_common, "Common");
M_M(m_play, m_journal, m_common, cb_play, "Play");
M_M(m_journal, m_settings, m_play, cb_journal, "Journal");
M_M(m_settings, m_common, m_journal, cb_sett, "Settings");

static ui_item_t *cur = &m_common;
static uint16_t upd_timer = 0;
static char buffer[128];

static void chng_menu(ui_item_t *item)
{
	upd_timer = 0;
	cur = item;
	i2c_display_clear_screen(false);
}

void cb_common(bool error, bool redraw, ui_item_t *current)
{
	if(redraw)
	{
		sprintf(buffer, "%d.%02dA ", adc_val.i_p / 1000, (adc_val.i_p % 1000) / 10);
		i2c_display_display_text(0, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "%d.%01dV ", adc_val.v_p / 1000, (adc_val.v_p % 1000) / 100);
		i2c_display_display_text(1, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "%-d.%01dV ", adc_val.v_n / 1000, (adc_val.v_n % 1000) / 100);
		i2c_display_display_text(2, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "%d.%01dV ", adc_val.v_i / 1000, (adc_val.v_i % 1000) / 100);
		i2c_display_display_text(3, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "%d %d ", adc_val.t_drv / 10, adc_val.t_lsr_head / 10);
		i2c_display_display_text(4, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "%d %d ", adc_val.t_inv_p / 10, adc_val.t_inv_n / 10);
		i2c_display_display_text(5, 0, buffer, strlen(buffer), false);

		int W = adc_val.v_p * adc_val.i_p / 1000;
		sprintf(buffer, "%d.%01dW ", W / 1000, (W % 1000) / 100);
		i2c_display_display_text(7, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "b %d s%d ", btn_emcy.pressed, (safety_is_locked() ? 1 : 0) * 10 + (voltage_enabled ? 1 : 0));
		i2c_display_display_text(8, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "%d ", fr);
		i2c_display_display_text(9, 0, buffer, strlen(buffer), false);
	}

	static uint32_t sss0 = 0;
	if(btn_j_press.pressed_shot)
	{
		sss0++;
		if(sss0 & 1)
		{
			lp_on();
		}
		else
		{
			static uint32_t d = 0;
			d++;
			if(d >= 4) d = 0;
			uint32_t frr[] = {12000, 20000, 30000, 40000};
			// fr = frr[d];
			lp_off();
			// sd_card_test();
		}
	}

	if(btn_l.pressed && btn_r.pressed)
		safety_reset_lock();

	if(btn_j_l.pressed_shot)
		chng_menu(cur->prev);
	if(btn_j_r.pressed_shot)
		chng_menu(cur->next);
}

void cb_play(bool error, bool redraw, ui_item_t *current)
{
	if(redraw)
	{
		sprintf(buffer, current->text);
		i2c_display_display_text(0, 0, buffer, strlen(buffer), false);
	}
	if(btn_j_l.pressed_shot)
		chng_menu(cur->prev);
	if(btn_j_r.pressed_shot)
		chng_menu(cur->next);
}

void cb_sett(bool error, bool redraw, ui_item_t *current)
{
	if(redraw)
	{
		sprintf(buffer, current->text);
		i2c_display_display_text(0, 0, buffer, strlen(buffer), false);
	}
	if(btn_j_l.pressed_shot)
		chng_menu(cur->prev);
	if(btn_j_r.pressed_shot)
		chng_menu(cur->next);
}

void cb_journal(bool error, bool redraw, ui_item_t *current)
{
	if(redraw)
	{
		sprintf(buffer, current->text);
		i2c_display_display_text(0, 0, buffer, strlen(buffer), false);
	}
	if(btn_j_l.pressed_shot)
		chng_menu(cur->prev);
	if(btn_j_r.pressed_shot)
		chng_menu(cur->next);
}

void ui_init(void)
{
	i2c_display_init();
	i2c_display_direction(DIRECTION180);
	i2c_display_clear_screen(0);
}

void ui_loop(bool error, uint32_t ms_diff)
{
	cur->cb(error, upd_timer <= ms_diff, cur);
	upd_timer = upd_timer <= ms_diff ? UPD_INTERVAL : upd_timer - ms_diff;
}