
#include <cstdio>
#include "driver/gpio.h"
#include "driver/sdspi_host.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "fat.h"

#define CS_SD GPIO_NUM_21

namespace {

constexpr char kMountPoint[] = "/sdcard";
const char* TAG = "FAT";

sdmmc_card_t* s_card = nullptr;
bool s_is_mounted = false;

}  // namespace

void init_fat() {
    if (s_is_mounted) {
        ESP_LOGI(TAG, "FAT filesystem already mounted at %s", kMountPoint);
        return;
    }

    // The SPI2 bus is already initialized by init_spi(); skip bus init here
    // and attach the SD card as an additional device on the shared bus.
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;
    host.max_freq_khz = 10000;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = CS_SD;
    slot_config.host_id = static_cast<spi_host_device_t>(host.slot);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 5;
    mount_config.allocation_unit_size = 16 * 1024;

    esp_err_t ret = esp_vfs_fat_sdspi_mount(kMountPoint, &host, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount FAT filesystem on SD card");
        } else {
            ESP_LOGE(TAG, "SD card initialization failed: %s", esp_err_to_name(ret));
        }
        return;
    }

    s_is_mounted = true;
    sdmmc_card_print_info(stdout, s_card);
    ESP_LOGI(TAG, "Mounted FAT filesystem at %s", kMountPoint);
}