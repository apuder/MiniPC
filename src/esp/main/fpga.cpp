
#include "fpga.h"
#include "spi.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

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
	while (true) {
		if (xQueueReceive(fpga_req_queue, &event, portMAX_DELAY) == pdTRUE) {
            // Got a request from the FPGA
            uint8_t* out = get_spi_out_buffer();
            uint8_t* in = get_spi_in_buffer();
            // Fill out buffer with some data
            for (int i = 0; i < 256; i++) {
                out[i] = 10 + i % 10;
            }
            // Transmit data over SPI
            spi_transmit();
            // Signal to FPGA that data is ready
			gpio_set_level(DONE, 1);
			ets_delay_us(1);
			gpio_set_level(DONE, 0);
            // Log the received data for debugging
            ESP_LOGI(TAG, "Received request from FPGA, sent response:");
            for (int i = 0; i < 256; i++) {
                ESP_LOGI(TAG, "Response[%d]: %02X", i, in[i]);
            }
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

	esp_err_t ret = gpio_install_isr_service(0);
	if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
		ESP_LOGE(TAG, "gpio_install_isr_service failed: %s", esp_err_to_name(ret));
		return;
	}

	ret = gpio_isr_handler_add(REQ, req_isr_handler, nullptr);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "gpio_isr_handler_add failed: %s", esp_err_to_name(ret));
		return;
	}

	xTaskCreatePinnedToCore(fpga_event_task, "fpga_event", 2048, nullptr, 5, nullptr, tskNO_AFFINITY);
}