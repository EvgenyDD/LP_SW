#include "sd_card.h"
#include "driver/sdmmc_defs.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "ilda_playground.h"
#include "sdmmc_cmd.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

static bool is_sd_mounted = false;

sdmmc_card_t *card;

bool sd_card_mount(void)
{
	sdmmc_host_t host = SDMMC_HOST_DEFAULT();

	// To use 1-line SD mode, uncomment the following line:
	host.flags = SDMMC_HOST_FLAG_1BIT;
	host.max_freq_khz = SDMMC_FREQ_PROBING;

	// This initializes the slot without card detect (CD) and write protect (WP) signals.
	// Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
	sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

	// Options for mounting the filesystem.
	// If format_if_mount_failed is set to true, SD card will be partitioned and formatted
	// in case when mounting fails.
	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
		.format_if_mount_failed = true,
		.max_files = 5};

	esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
	if(ret != ESP_OK)
	{
		is_sd_mounted = false;
		if(ret == ESP_FAIL)
			ESP_LOGE("SD", "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
		else
			ESP_LOGE("SD", "Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", ret);
		return true;
	}
	ESP_LOGI("SD", "Mount success\n");
	is_sd_mounted = true;

	return false;
}

void sd_card_test(void)
{
	ESP_LOGI("SD", "Check file start");
	int sts = ilda_check_file("/sdcard/TEST_12K.ILD");
	ESP_LOGI("SD", "Check file %d", sts);

	if(sts == 0)
	{
		ilda_file_load("/sdcard/TEST_12K.ILD");
	}
}

void sd_card_init(void)
{
	do
	{
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