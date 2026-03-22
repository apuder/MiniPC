
#include "fpga.h"
#include "spi.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

#include <cstdint>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define REQ GPIO_NUM_34
#define DONE GPIO_NUM_18

namespace {

const char* TAG = "FPGA";
QueueHandle_t fpga_req_queue = nullptr;

void IRAM_ATTR req_isr_handler(void* arg) {
	const uint32_t event = 1;
	BaseType_t task_woken = pdFALSE;
	xQueueSendFromISR(fpga_req_queue, &event, &task_woken);
	if (task_woken == pdTRUE) {
		portYIELD_FROM_ISR();
	}
}

void fpga_event_task(void* arg) {
	uint32_t event = 0;
	FILE* f = nullptr;

	while (true) {
		if (xQueueReceive(fpga_req_queue, &event, portMAX_DELAY) == pdTRUE) {
            // Got a request from the FPGA
            uint8_t* out = get_spi_out_buffer();
            uint8_t* in = get_spi_in_buffer();
			if (out == nullptr || in == nullptr) {
				ESP_LOGE(TAG, "SPI buffers not initialized");
				continue;
			}
            spi_transmit();
			ESP_LOGI(TAG, "Received request: %d", *in);
			switch(*in) {
				case 0:
				  // NOP
				  break;
				case 1:
				  // Open kernel.img from SD card and prepare it for transfer to FPGA
				  f = fopen("/sdcard/kernel.bin", "rb");
				  *out = (f != nullptr) ? 0 : 1; // Return 0 on success, 1 on failure
				  if (f == nullptr) {
					ESP_LOGE(TAG, "Failed to open kernel.bin");
				  } else {
	    			  if (fseek(f, 0, SEEK_END) != 0) {
						ESP_LOGE(TAG, "fseek(SEEK_END) failed");
						fclose(f);
						f = nullptr;
						*out = 1;
				      } else {
						long size = ftell(f);
						if (size < 0 || fseek(f, 0, SEEK_SET) != 0) {
							ESP_LOGE(TAG, "ftell/fseek(SEEK_SET) failed");
							fclose(f);
							f = nullptr;
							*out = 1;
						} else {
							ESP_LOGI(TAG, "kernel.bin size: %u bytes", (unsigned)size);
							uint32_t size32 = static_cast<uint32_t>(size);
							std::memcpy(out + 1, &size32, sizeof(size32));
						}
				      }
				  }
				  break;
				case 2:
				  // Read up to 256 bytes from kernel.bin and prepare it for transfer to FPGA
				  if (f == nullptr) {
					ESP_LOGE(TAG, "kernel.bin not opened");
					memset(out, 0, 256);
				  } else {
					size_t bytesRead = fread(out, 1, 256, f);
					ESP_LOGW(TAG, "Read %d bytes from kernel.bin", bytesRead);
					if (bytesRead < 256) {
						ESP_LOGI(TAG, "Reached end of kernel.bin, closing file");
						memset(out + bytesRead, 0, 256 - bytesRead); // Pad remaining bytes with zeros
						fclose(f);
						f = nullptr;
					}
				  }
				  break;
				default:
				  ESP_LOGW(TAG, "Unknown request received from FPGA: %d", *in);
				  memset(out, 0, 256);
				  break;
			}
            // Signal to FPGA that data is ready
			gpio_set_level(DONE, 1);
			ets_delay_us(1);
			gpio_set_level(DONE, 0);
		}
	}
}

}  // namespace

void init_fpga() {
	if (fpga_req_queue != nullptr) {
		return;
	}

	fpga_req_queue = xQueueCreate(8, sizeof(uint32_t));
	if (fpga_req_queue == nullptr) {
		ESP_LOGE(TAG, "Failed to create FPGA request queue");
		return;
	}

	gpio_config_t req_config = {};
	req_config.pin_bit_mask = (1ULL << REQ);
	req_config.mode = GPIO_MODE_INPUT;
	req_config.pull_up_en = GPIO_PULLUP_DISABLE;
	req_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
	req_config.intr_type = GPIO_INTR_POSEDGE;
	gpio_config(&req_config);

	gpio_config_t done_config = {};
	done_config.pin_bit_mask = (1ULL << DONE);
	done_config.mode = GPIO_MODE_OUTPUT;
	done_config.pull_up_en = GPIO_PULLUP_DISABLE;
	done_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
	done_config.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&done_config);
	gpio_set_level(DONE, 0);

	esp_err_t ret;
#if 0
    // Already initialized in init_button()
	ret = gpio_install_isr_service(0);
	if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
		ESP_LOGE(TAG, "gpio_install_isr_service failed: %s", esp_err_to_name(ret));
		return;
	}
#endif

	ret = gpio_isr_handler_add(REQ, req_isr_handler, nullptr);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "gpio_isr_handler_add failed: %s", esp_err_to_name(ret));
		return;
	}

	xTaskCreatePinnedToCore(fpga_event_task, "fpga_event", 4096, nullptr, 5, nullptr, tskNO_AFFINITY);
}