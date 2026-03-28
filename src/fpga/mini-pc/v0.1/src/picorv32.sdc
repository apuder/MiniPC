//Copyright (C)2014-2024 GOWIN Semiconductor Corporation.
//All rights reserved.
//File Title: Timing Constraints file
//GOWIN Version: 1.9.9 Beta-4 Education
//Created Time: 2024-06-01 16:16:50

create_clock -name clk_in -period 37 [get_ports {clk_in}]

// Clocks generated from the 27 MHz input.
create_generated_clock -name clk          -source [get_ports {clk_in}] -multiply_by 28 -divide_by 9 [get_nets {clk}]
create_generated_clock -name clk_pixel_x5 -source [get_ports {clk_in}] -multiply_by 14 -divide_by 3 [get_nets {clk_pixel_x5}]
create_generated_clock -name clk_pixel    -source [get_nets {clk_pixel_x5}] -divide_by 5 [get_nets {clk_pixel}]
create_generated_clock -name spi_fast_clk -source [get_ports {clk_in}] -multiply_by 3 [get_nets {spi/spi_fast_clk}]

// SPI engine is intentionally isolated from system/video domains via async boundaries.
set_clock_groups -asynchronous -group [get_clocks {spi_fast_clk}] -group [get_clocks {clk clk_pixel_x5 clk_pixel}]