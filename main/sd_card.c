#include "sd_card.h"
#include "driver/sdmmc_defs.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

void sd_card_init(void)
{
	do
	{
		sdmmc_host_t host = SDMMC_HOST_DEFAULT();
		host.flags = SDMMC_HOST_FLAG_1BIT; // SDMMC_HOST_FLAG_DDR or SDMMC_HOST_FLAG_1BIT
		host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
		sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
		slot_config.width = 1;
		esp_vfs_fat_sdmmc_mount_config_t mount_config = {
			.format_if_mount_failed = false,
			.max_files = 5};
		sdmmc_card_t *card;
		esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
		if(ret != ESP_OK)
		{
			if(ret == ESP_FAIL)
				ESP_LOGE("SD", "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
			else
				ESP_LOGE("SD", "Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", ret);
			break;
		}

		sdmmc_card_print_info(stdout, card);

		ESP_LOGI("SD", "Opening file");
		FILE *f = fopen("/sdcard/hello.txt", "w");
		if(f == NULL)
		{
			ESP_LOGE("SD", "Failed to open file for writing");
			break;
		}
		fprintf(f, "Hello %s!\n", card->cid.name);
		fclose(f);
		ESP_LOGI("SD", "File written");

		struct stat st;
		if(stat("/sdcard/foo.txt", &st) == 0)
		{
			unlink("/sdcard/foo.txt"); // Delete it if it exists
		}

		ESP_LOGI("SD", "Renaming file");
		if(rename("/sdcard/hello.txt", "/sdcard/foo.txt") != 0)
		{
			ESP_LOGE("SD", "Rename failed");
			break;
		}

		ESP_LOGI("SD", "Reading file");
		f = fopen("/sdcard/foo.txt", "r");
		if(f == NULL)
		{
			ESP_LOGE("SD", "Failed to open file for reading");
			break;
		}

		char line[64];
		fgets(line, sizeof(line), f);
		fclose(f);

		char *pos = strchr(line, '\n'); // strip newline
		if(pos) *pos = '\0';
		ESP_LOGI("SD", "Read from file: '%s'", line);

		esp_vfs_fat_sdmmc_unmount(); // unmount partition and disable SDMMC host peripheral
		ESP_LOGI("SD", "Card unmounted");
	} while(0);
}