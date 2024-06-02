// single include file !!!

extern void periph_module_enable(periph_module_t periph);

static int IRAM_ATTR spi_freq_for_pre_n(int fapb, int pre, int n) { return (fapb / (pre * n)); }
static int IRAM_ATTR spi_set_clock(spi_dev_t *hw, int fapb, int hz, int duty_cycle)
{
	int pre, n, h, l, eff_clk;
	if(hz > ((fapb / 4) * 3))
	{
		hw->clock.clkcnt_l = 0;
		hw->clock.clkcnt_h = 0;
		hw->clock.clkcnt_n = 0;
		hw->clock.clkdiv_pre = 0;
		hw->clock.clk_equ_sysclk = 1;
		eff_clk = fapb;
	}
	else
	{
		int bestn = -1;
		int bestpre = -1;
		int besterr = 0;
		int errval;
		for(n = 1; n <= 64; n++)
		{
			pre = ((fapb / n) + (hz / 2)) / hz;
			if(pre <= 0) pre = 1;
			if(pre > 8192) pre = 8192;
			errval = abs(spi_freq_for_pre_n(fapb, pre, n) - hz);
			if(bestn == -1 || errval <= besterr)
			{
				besterr = errval;
				bestn = n;
				bestpre = pre;
			}
		}

		n = bestn;
		pre = bestpre;
		l = n;
		h = (duty_cycle * n + 127) / 256;
		if(h <= 0) h = 1;

		hw->clock.clk_equ_sysclk = 0;
		hw->clock.clkcnt_n = n - 1;
		hw->clock.clkdiv_pre = pre - 1;
		hw->clock.clkcnt_h = h - 1;
		hw->clock.clkcnt_l = l - 1;
		eff_clk = spi_freq_for_pre_n(fapb, pre, n);
	}
	return eff_clk;
}

static inline void spi_config(void)
{
	periph_module_enable(DAC_SPI_BUS_MODULE);
	esp_intr_alloc(ETS_SPI2_INTR_SOURCE, ESP_INTR_FLAG_INTRDISABLED, NULL, NULL, NULL);
	DAC_SPI.dma_conf.val |= SPI_OUT_RST | SPI_AHBM_RST | SPI_AHBM_FIFO_RST;
	DAC_SPI.dma_out_link.start = 0;
	DAC_SPI.dma_in_link.start = 0;
	DAC_SPI.dma_conf.val &= ~(SPI_OUT_RST | SPI_AHBM_RST | SPI_AHBM_FIFO_RST);
	DAC_SPI.ctrl2.val = 0;
	DAC_SPI.slave.rd_buf_done = 0;
	DAC_SPI.slave.wr_buf_done = 0;
	DAC_SPI.slave.rd_sta_done = 0;
	DAC_SPI.slave.wr_sta_done = 0;
	DAC_SPI.slave.rd_buf_inten = 0;
	DAC_SPI.slave.wr_buf_inten = 0;
	DAC_SPI.slave.rd_sta_inten = 0;
	DAC_SPI.slave.wr_sta_inten = 0;
	DAC_SPI.slave.trans_inten = 0;
	DAC_SPI.slave.trans_done = 0;
	DAC_SPI.pin.master_ck_sel &= (1 << 0);
	DAC_SPI.pin.master_cs_pol &= (1 << 0);
	int effclk = spi_set_clock(&DAC_SPI, APB_CLK_FREQ, 16000000, /*cfg.duty_cycle_pos*/ 128);
	DAC_SPI.ctrl.rd_bit_order = 0;
	DAC_SPI.ctrl.wr_bit_order = 0;

	int nodelay = 0;
	int extra_dummy = 0;
	if(effclk >= APB_CLK_FREQ / 2)
	{
		nodelay = 1;
		extra_dummy = 1;
	}
	else if(effclk >= APB_CLK_FREQ / 4)
	{
		nodelay = 1;
	}

	DAC_SPI.pin.ck_idle_edge = 0;
	DAC_SPI.user.ck_out_edge = 0;
	DAC_SPI.ctrl2.miso_delay_mode = nodelay ? 0 : 2;
	DAC_SPI.user.usr_dummy = (extra_dummy) ? 1 : 0;
	DAC_SPI.user.usr_addr = 0;
	DAC_SPI.user.usr_command = 0;
	DAC_SPI.user1.usr_addr_bitlen = 0 - 1;
	DAC_SPI.user1.usr_dummy_cyclelen = extra_dummy - 1;
	DAC_SPI.user2.usr_command_bitlen = 0 - 1;
	DAC_SPI.user.doutdin = /*(handle->cfg.flags & SPI_DEVICE_HALFDUPLEX) ?*/ 0 /*: 1*/;
	DAC_SPI.user.sio = 0;
	DAC_SPI.ctrl2.setup_time = 0 - 1;
	DAC_SPI.user.cs_setup = 0;
	DAC_SPI.ctrl2.hold_time = 0 - 1;
	DAC_SPI.user.cs_hold = 0;
	DAC_SPI.pin.cs0_dis = 0;
	DAC_SPI.pin.cs1_dis = 0;
	DAC_SPI.pin.cs2_dis = 0;
	DAC_SPI.user.usr_mosi_highpart = 0;
	DAC_SPI.mosi_dlen.usr_mosi_dbitlen = 16 - 1;
	DAC_SPI.user.usr_mosi = 1;
	DAC_SPI.miso_dlen.usr_miso_dbitlen = 0;
	DAC_SPI.user.usr_miso = 0;
}

static inline void timer_config(void)
{
	periph_module_enable(PERIPH_TIMG0_MODULE);
	timer_ll_set_reload_value(&TIMERG0, 0, 0);
	timer_ll_trigger_soft_reload(&TIMERG0, 0);
	timer_ll_set_clock_source(&TIMERG0, 0, SOC_MOD_CLK_APB);
	timer_ll_set_clock_prescale(&TIMERG0, 0, 80 /* 1 us per tick */);
	timer_ll_set_count_direction(&TIMERG0, 0, GPTIMER_COUNT_UP);
	timer_ll_enable_intr(&TIMERG0, TIMER_LL_EVENT_ALARM(0), false);
	timer_ll_clear_intr_status(&TIMERG0, TIMER_LL_EVENT_ALARM(0));
	timer_ll_enable_alarm(&TIMERG0, 0, true);
	timer_ll_enable_auto_reload(&TIMERG0, 0, true);
	timer_ll_enable_counter(&TIMERG0, 0, false);
	timer_ll_set_alarm_value(&TIMERG0, 0, 22);
	timer_ll_enable_intr(&TIMERG0, TIMER_LL_EVENT_ALARM(0), true);
	ESP_ERROR_CHECK(esp_intr_alloc_intrstatus(timer_group_periph_signals.groups[0].timer_irq_id[0], ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL5,
											  (uint32_t)timer_ll_get_intr_status_reg(&TIMERG0), TIMER_LL_EVENT_ALARM(0), NULL, NULL, NULL));
}