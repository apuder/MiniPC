
#pragma once

#include <cstdint>

uint8_t* get_spi_in_buffer();
uint8_t* get_spi_out_buffer();
void spi_transmit();
void init_spi();
 