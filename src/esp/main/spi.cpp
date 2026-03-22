
#include <cstring>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "spi.h"

#define CS_FPGA GPIO_NUM_22
#define SPI_CLK GPIO_NUM_26
#define SPI_MOSI GPIO_NUM_25
#define SPI_MISO GPIO_NUM_19

namespace {

constexpr size_t SPI_BUFFER_SIZE = 256;
// One dummy byte keeps the ESP transfer byte-aligned while giving the FPGA
// 8 SCK periods to fetch byte 0 before payload sampling begins.
constexpr int SPI_DUMMY_BITS = 8;
constexpr size_t SPI_DUMMY_BYTES = SPI_DUMMY_BITS / 8;
constexpr size_t SPI_TRANSFER_SIZE = SPI_BUFFER_SIZE + SPI_DUMMY_BYTES;
constexpr int SPI_TRANSFER_BITS = SPI_TRANSFER_SIZE * 8;
constexpr int SPI_CLOCK_HZ = 20 * 1000 * 1000;

const char* TAG = "SPI";

spi_device_handle_t spi_handle = nullptr;
uint8_t* spi_transfer_in_buffer = nullptr;
uint8_t* spi_transfer_out_buffer = nullptr;

}  // namespace

uint8_t* get_spi_in_buffer() {
    return (spi_transfer_in_buffer == nullptr) ? nullptr : (spi_transfer_in_buffer + SPI_DUMMY_BYTES);
}

uint8_t* get_spi_out_buffer() {
    return (spi_transfer_out_buffer == nullptr) ? nullptr : (spi_transfer_out_buffer + SPI_DUMMY_BYTES);
}

void spi_transmit() {
    if (spi_handle == nullptr || spi_transfer_in_buffer == nullptr || spi_transfer_out_buffer == nullptr) {
        ESP_LOGE(TAG, "SPI master is not initialized");
        return;
    }

    std::memset(spi_transfer_in_buffer, 0, SPI_DUMMY_BYTES);
    std::memset(spi_transfer_out_buffer, 0, SPI_DUMMY_BYTES);

    spi_transaction_t transaction = {};
    transaction.length = SPI_TRANSFER_BITS;
    transaction.rxlength = SPI_TRANSFER_BITS;
    transaction.tx_buffer = spi_transfer_out_buffer;
    transaction.rx_buffer = spi_transfer_in_buffer;

    esp_err_t ret = spi_device_transmit(spi_handle, &transaction);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_device_transmit failed: %s", esp_err_to_name(ret));
    }
}

void init_spi() {
    if (spi_handle != nullptr) {
        return;
    }

    if (spi_transfer_in_buffer == nullptr) {
        spi_transfer_in_buffer = static_cast<uint8_t*>(heap_caps_malloc(SPI_TRANSFER_SIZE, MALLOC_CAP_DMA));
    }
    if (spi_transfer_out_buffer == nullptr) {
        spi_transfer_out_buffer = static_cast<uint8_t*>(heap_caps_malloc(SPI_TRANSFER_SIZE, MALLOC_CAP_DMA));
    }

    if (spi_transfer_in_buffer == nullptr || spi_transfer_out_buffer == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate DMA-capable SPI buffers");
        return;
    }

    std::memset(spi_transfer_in_buffer, 0, SPI_TRANSFER_SIZE);
    std::memset(spi_transfer_out_buffer, 0, SPI_TRANSFER_SIZE);

    spi_bus_config_t bus_config = {};
    bus_config.mosi_io_num = SPI_MOSI;
    bus_config.miso_io_num = SPI_MISO;
    bus_config.sclk_io_num = SPI_CLK;
    bus_config.quadwp_io_num = -1;
    bus_config.quadhd_io_num = -1;
    bus_config.max_transfer_sz = SPI_TRANSFER_SIZE;

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        return;
    }

    spi_device_interface_config_t device_config = {};
    device_config.mode = 0;
    device_config.clock_speed_hz = SPI_CLOCK_HZ;
    device_config.spics_io_num = CS_FPGA;
    device_config.queue_size = 1;
    device_config.cs_ena_pretrans = 0;

    ret = spi_bus_add_device(SPI2_HOST, &device_config, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(ret));
        spi_bus_free(SPI2_HOST);
        return;
    }
}