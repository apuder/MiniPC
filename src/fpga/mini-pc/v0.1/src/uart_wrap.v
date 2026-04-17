module uart_wrap #(parameter integer DEFAULT_STOP_BITS = 1)
  (
   input wire         clk,
   input wire         reset_n,
   input wire         uart_rx,
   output wire        uart_tx,
   input wire         uart_sel,
   input wire [3:0]   addr, // Choose div or dat
   input wire [3:0]   uart_wstrb,
   input wire [31:0]  uart_di,
   output wire [31:0] uart_do,
   output wire        uart_ready,
   output wire        uart_rx_ready_pulse
   );

   wire               div_sel;
   wire               dat_sel;
   wire               stop_sel;
   wire               dat_word_sel;
   wire               stop_lane3_write;
   wire [31:0]        div_do;
   wire [31:0]        dat_do;
   wire [31:0]        stop_do;
   wire               dat_wait;
            
  // Support both absolute-style offsets (8/c/f) and region-relative
  // offsets (0/4/f). Because PicoRV32 aligns mem_addr for data accesses,
  // writes to byte address ...f arrive at word address ...c with wstrb[3].
   assign div_sel = uart_sel && ((addr == 4'h8) || (addr == 4'h0));
   assign dat_word_sel = uart_sel && ((addr == 4'hc) || (addr == 4'h4));
   assign stop_lane3_write = dat_word_sel && (uart_wstrb == 4'b1000);
   assign dat_sel = dat_word_sel && !stop_lane3_write;
   assign stop_sel = (uart_sel && (addr == 4'hf)) || stop_lane3_write;
   assign uart_do = div_sel ? div_do :
              dat_sel ? dat_do :
              stop_sel ? stop_do : 32'h0;
  assign uart_ready = div_sel | stop_sel | (dat_sel && !dat_wait);
   
   simpleuart
     #(
      .DEFAULT_STOP_BITS(DEFAULT_STOP_BITS)
      )
     uart
     (
      .clk(clk),
      .resetn(reset_n),
      .ser_tx(uart_tx),
      .ser_rx(uart_rx),
      .reg_div_we(div_sel ? uart_wstrb : 4'b0000),
      .reg_div_di(uart_di),
      .reg_div_do(div_do),
      .reg_stop_we(stop_sel ? uart_wstrb : 4'b0000),
      .reg_stop_di(uart_di),
      .reg_stop_do(stop_do),
      .reg_dat_we(dat_sel ? uart_wstrb[0] : 1'b0),
      .reg_dat_re(dat_sel && !uart_wstrb),
      .reg_dat_di(uart_di),
      .reg_dat_do(dat_do),
      .reg_dat_wait(dat_wait),
      .rx_ready_pulse(uart_rx_ready_pulse)
      );

endmodule
