/* Copyright 2024 Grug Huhler.  License SPDX BSD-2-Clause. */
/* 
Top level module of simple SoC based on picorv32

It includes:
     * the picorv32 core
     * An 8192 byte SRAM which is initialzed within the Verilog.
     * A module to read/write LEDs on the Gowin Tang Nano 9K
     * A wrapped version of the UART from picorv32's picosoc.
     * A 32-bit count down timer.
  
Built and tested with the Gowin Eductional tool set on Tang Nano 9K.

The picorv32 core has a very simple memory interface.
See https://github.com/YosysHQ/picorv32

In this SoC, slave (target) device has signals:

   * SLAVE_sel - this is asserted when mem_valid == 1 and mem_addr targets the slave.
     It "tells" the slave that it is active.  It must accept a write for provide data
     for a read.
   * SLAVE_ready - this is asserted by the slave when it is done with the transaction.
     Core signal mem_ready is the OR of all of the SLAVE_ready signals.
   * Core mem_addr, mem_wdata, and mem_wstrb can be passed to all slaves directly.
     The latter is a byte lane enable for writes.
   * Each slave drives SLAVE_data_o.  The core's mem_rdata is formed by selecting the
     correct SLAVE_data_o based on SLAVE_sel.
*/

// Define this for logic analyer connections and enable picorv32_la.cst.
//`define USE_LA

module top (
            input wire        clk_in,
            input wire        reset_button_n,
            input wire        uart_rx,
            output wire       uart_tx,
            output wire       ws2812b_din,

            // PSRAM
            output logic        ps_ce_n,
            output logic        ps_sclk,
            inout  wire  [3:0]  ps_sio,

            // HDMI
            output [2:0] HDMI_TX_P,
            output [2:0] HDMI_TX_N,
            output HDMI_TXC_P,
            output HDMI_TXC_N,

`ifdef USE_LA
            output wire       clk_out,
            output wire       mem_instr, 
            output wire       mem_valid,
            output wire       mem_ready,
            output wire       b25,
            output wire       b24,
            output wire       b17,
            output wire       b16,
            output wire       b09,
            output wire       b08,
            output wire       b01,
            output wire       b00,
            output wire [3:0] mem_wstrb,
`endif
            output wire [3:0] leds
            );

   // This include gets SRAM_ADDR_WIDTH and CLK_FREQ from software build process
   `include "sys_parameters.v"

   parameter BARREL_SHIFTER = 0;
   parameter ENABLE_MUL = 0;
   parameter ENABLE_DIV = 0;
   parameter ENABLE_FAST_MUL = 0;
   parameter ENABLE_COMPRESSED = 0;
   parameter ENABLE_IRQ_QREGS = 0;

   parameter        MEMBYTES = 4*(1 << SRAM_ADDR_WIDTH); 
   parameter [31:0] STACKADDR = (MEMBYTES);         // Grows down.  Software should set it.
   parameter [31:0] PROGADDR_RESET = 32'h0000_0000;
   parameter [31:0] PROGADDR_IRQ = 32'h0000_0000;

   wire                       reset_n; 
   wire                       mem_valid;
   wire                       mem_instr;
   wire [31:0]                mem_addr;
   wire [31:0]                mem_wdata;
   wire [31:0]                mem_rdata;
   wire [3:0]                 mem_wstrb;
   wire                       mem_ready;
   wire [31:0]                psram_rdata;
   reg                       psram_ready;
   wire                       mem_inst;
   wire                       leds_sel;
   wire                       leds_ready;
   wire [31:0]                leds_data_o;
   wire                       sram_sel;
   wire                       sram_ready;
   wire [31:0]                sram_data_o;
   wire                       cdt_sel;
   wire                       cdt_ready;
   wire [31:0]                cdt_data_o;
   wire                       uart_sel;
   wire [31:0]                uart_data_o;
   wire                       uart_ready;
   wire                       ws2812b_sel;
   wire                       ws2812b_ready;
   wire                       dsp_sel;
   wire                       dsp_ready;
   wire [31:0]                dsp_data_o;
   wire                       psram_sel;
   reg                        psram_chip_present;

`ifdef USE_LA
   // Assigns for external logic analyzer connction
   assign clk_out = clk;
   assign b25 = mem_rdata[25];
   assign b24 = mem_rdata[24];
   assign b17 = mem_rdata[17];
   assign b16 = mem_rdata[16];
   assign b09 = mem_rdata[9];
   assign b08 = mem_rdata[8];
   assign b01 = mem_rdata[1];
   assign b00 = mem_rdata[0];
`endif

wire clk;

Gowin_rPLL clk_wiz_0(
   .clkout(clk),  // output 84 MHz
   .clkin(clk_in) // input 27 MHz
);

//-----HDMI------------------------------------------------------------------------

wire vga_vid;

logic [23:0] rgb_screen_color = 24'hFFFFFF;


logic [8:0] audio_cnt;
logic clk_audio;

always @(posedge clk_in) audio_cnt <= (audio_cnt == 9'd280) ? 9'd0 : audio_cnt + 9'd1;
always @(posedge clk_in) if (audio_cnt == 9'd0) clk_audio <= ~clk_audio;

logic [15:0] audio_sample_word [1:0] = '{16'd0, 16'd0};


wire clk_pixel;
wire clk_pixel_x5;

// 125.875 MHz (126 MHz actual)
Gowin_rPLL0 pll0(
  .clkout(clk_pixel_x5), //output
  .clkin(clk_in) //input
);

// 25.175 MHz (25.2 MHz actual)
Gowin_CLKDIV0 clkdiv0(
  .clkout(clk_pixel), //output
  .hclkin(clk_pixel_x5), //input
  .resetn(1'b1) //input
);

reg [23:0] rgb = 24'h0;
wire vga3_vid;

always @(posedge clk_pixel)
begin
  rgb <= vga_vid ? rgb_screen_color : 24'h0;
end

logic [9:0] cx, frame_width, screen_width;
logic [9:0] cy, frame_height, screen_height;
wire [2:0] tmds_x;
wire tmds_clock_x;

// 640x480 @ 60Hz
hdmi #(.VIDEO_ID_CODE(1), .VIDEO_REFRESH_RATE(60), .AUDIO_RATE(48000), .AUDIO_BIT_WIDTH(16)) hdmi(
  .clk_pixel_x5(clk_pixel_x5),
  .clk_pixel(clk_pixel),
  .clk_audio(clk_audio),
  .reset(1'b0),
  .rgb(rgb),
  .audio_sample_word(audio_sample_word),
  .tmds(tmds_x),
  .tmds_clock(tmds_clock_x),
  .cx(cx),
  .cy(cy),
  .frame_width(frame_width),
  .frame_height(frame_height),
  .screen_width(screen_width),
  .screen_height(screen_height)
);

TLVDS_OBUF tmds [2:0] (
  .O(HDMI_TX_P),
  .OB(HDMI_TX_N),
  .I(tmds_x)
);

TLVDS_OBUF tmds_clock(
  .O(HDMI_TXC_P),
  .OB(HDMI_TXC_N),
  .I(tmds_clock_x)
);

reg sync;


always @(posedge clk_pixel)
begin
  sync <= (cx == frame_width - 8) && (cy == frame_height - 1);
end



  wire[31:0] PSRAM_BASE   = 32'h4000_0000;
  wire[31:0] PSRAM_SIZE   = 32'h0080_0000;  // 8MB

  wire[31:0] DSP_BASE     = 32'h5000_0000;
  wire[31:0] DSP_SIZE     = 32'h0000_0800;

   // Establish memory map for all slaves:
   //      SRAM 00000000 - 0001ffff
   //      LED  80000000
   //      UART 80000008 - 8000000f
   //      CDT  80000010 - 80000014
   //   WS2812B 80000020 - 80000024
   //      DSP  50000000 - 500007ff  (2K display BRAM)
   assign sram_sel = mem_valid && (mem_addr < MEMBYTES);
   assign leds_sel = mem_valid && (mem_addr == 32'h80000000);
   assign uart_sel = mem_valid && ((mem_addr & 32'hfffffff8) == 32'h80000008);
   assign cdt_sel = mem_valid && (mem_addr == 32'h80000010);
   assign ws2812b_sel = mem_valid && (mem_addr == 32'h80000020);
   assign dsp_sel = mem_valid && (mem_addr >= DSP_BASE) && (mem_addr < (DSP_BASE + DSP_SIZE));
   assign psram_sel = mem_valid && (mem_addr >= PSRAM_BASE) && (mem_addr < (PSRAM_BASE + PSRAM_SIZE));
    
  // Core can proceed when the selected slave is ready.
  assign mem_ready = mem_valid & (
               (psram_sel   & psram_ready) |
               (sram_sel    & sram_ready)  |
               (leds_sel    & leds_ready)  |
               (uart_sel    & uart_ready)  |
               (cdt_sel     & cdt_ready)   |
               (ws2812b_sel & ws2812b_ready) |
               (dsp_sel     & dsp_ready)
              );


   // Select which slave's output data is to be fed to core.
   assign mem_rdata = psram_sel   ? psram_rdata :
                      sram_sel    ? sram_data_o :
                      dsp_sel     ? dsp_data_o  :
                      leds_sel    ? leds_data_o :
                      uart_sel    ? uart_data_o :
                      cdt_sel     ? cdt_data_o  : 32'h0;

wire status;

assign leds[2] = status;
assign leds[3] = psram_chip_present;

//   assign leds = ~status;//~leds_data_o[5:0]; // Connect to the LEDs off the FPGA

/*
reg psram_accessed = 1'b0;
assign leds[0] = ~psram_accessed;
assign leds[1] = ~status[1];
always @(posedge clk) begin
  if (psram_sel && !psram_ready) begin
    psram_accessed <= ~psram_accessed;
  end
  if (psram_sel) begin
    psram_ready <= 1'b1;
  end
  else psram_ready <= 1'b0;
end
*/

   wire reset_button = ~reset_button_n;

   reset_control reset_controller
     (
      .clk(clk),
      .reset_button(reset_button),
      .reset_n(reset_n)
      );

   display dsp
     (
      .clk(clk),
      .rst(~reset_n),
      .vga_clk(clk_pixel),
      .pixel_data(vga_vid),
      .genlock(sync),
      .mem_valid(dsp_sel),
      .mem_addr(mem_addr - DSP_BASE),  // Address within the display's address space
      .mem_wdata(mem_wdata),
      .mem_wstrb(mem_wstrb),
      .mem_ready(dsp_ready),
      .mem_rdata(dsp_data_o)
      );
  
   uart_wrap uart
     (
      .clk(clk),
      .reset_n(reset_n),
      .uart_tx(uart_tx),
      .uart_rx(uart_rx),
      .uart_sel(uart_sel),
      .addr(mem_addr[3:0]),
      .uart_wstrb(mem_wstrb),
      .uart_di(mem_wdata),
      .uart_do(uart_data_o),
      .uart_ready(uart_ready)
      );

   countdown_timer cdt
     (
      .clk(clk),
      .reset_n(reset_n),
      .cdt_sel(cdt_sel),
      .cdt_data_i(mem_wdata),
      .we(mem_wstrb),
      .cdt_ready(cdt_ready),
      .cdt_data_o(cdt_data_o)
      );

   /* ws2812b_tgt is 32b write only */
   ws2812b_tgt #(.CLK_FREQ(CLK_FREQ)) ws2812b_led
     (
      .clk(clk),
      .reset_n(reset_n),
      .ws2812b_sel(ws2812b_sel),
      .we(&mem_wstrb),
      .wdata({mem_wdata[15:8], mem_wdata[23:16], mem_wdata[7:0]}),
      .ws2812b_ready(ws2812b_ready),
      .to_din(ws2812b_din)
      );

   sram #(.SRAM_ADDR_WIDTH(SRAM_ADDR_WIDTH)) memory
     (
      .clk(clk),
      .reset_n(reset_n),
      .sram_sel(sram_sel),
      .wstrb(mem_wstrb),
      .addr(mem_addr[SRAM_ADDR_WIDTH + 1:0]),
      .sram_data_i(mem_wdata),
      .sram_ready(sram_ready),
      .sram_data_o(sram_data_o)
      );
   
   tang_leds soc_leds
     (
      .clk(clk),
      .reset_n(reset_n),
      .leds_sel(leds_sel),
      .leds_data_i(mem_wdata[5:0]),
      .we(mem_wstrb[0]),
      .leds_ready(leds_ready),
      .leds_data_o(leds_data_o)
      );

    aps6404l_picorv32 psram
    (
        .clk(clk),
        .rst(~reset_n),
        .status(status),
        .psram_chip_present(psram_chip_present),

        // picorv32 memory interface
        .mem_valid(psram_sel),
        .mem_instr(mem_instr),      // unused here, but you may use it for I-cache decisions
        .mem_addr(mem_addr),       // byte address
        .mem_wdata(mem_wdata),
        .mem_wstrb(mem_wstrb),      // byte enables; 0 => read, 1 => write
        .mem_ready(psram_ready),
        .mem_rdata(psram_rdata),

        // QSPI pins
        .ps_ce_n(ps_ce_n),
        .ps_sclk(ps_sclk),
        .ps_sio(ps_sio)
    );

   picorv32
     #(
       .STACKADDR(STACKADDR),
       .PROGADDR_RESET(PROGADDR_RESET),
       .PROGADDR_IRQ(PROGADDR_IRQ),
       .BARREL_SHIFTER(BARREL_SHIFTER),
       .COMPRESSED_ISA(ENABLE_COMPRESSED),
       .ENABLE_MUL(ENABLE_MUL),
       .ENABLE_DIV(ENABLE_DIV),
       .ENABLE_FAST_MUL(ENABLE_FAST_MUL),
       .ENABLE_IRQ(1),
       .ENABLE_IRQ_QREGS(ENABLE_IRQ_QREGS)
       ) cpu
       (
        .clk         (clk),
        .resetn      (reset_n),
        .mem_valid   (mem_valid),
        .mem_instr   (mem_instr),
        .mem_ready   (mem_ready),
        .mem_addr    (mem_addr),
        .mem_wdata   (mem_wdata),
        .mem_wstrb   (mem_wstrb),
        .mem_rdata   (mem_rdata),
        .irq         ('b0)
        );

endmodule // top
