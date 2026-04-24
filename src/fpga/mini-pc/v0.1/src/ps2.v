// PS/2 Keyboard Interface Module
// Provides a read-only bus interface similar to uart_wrap's data register path.
module ps2
   (
    input                         clk,               // System clock
    input                         reset_n,           // Active low reset
    inout                         ps2_clk,           // PS/2 clock (open-drain)
    inout                         ps2_data,          // PS/2 data (open-drain)
    input                         ps2_sel,           // Read strobe/select from bus fabric
    output [31:0]                 ps2_do,            // Read data (ASCII in low byte, 0xff when empty)
    output                        ps2_ready,         // Transaction ready
   output                        ps_rx_ready_pulse  // One-cycle pulse on newly received ASCII keystroke
    );

   localparam [3:0] INIT_POWERON_WAIT          = 4'd0;
   localparam [3:0] INIT_PREP_RESET            = 4'd1;
   localparam [3:0] INIT_CLK_INHIBIT           = 4'd2;
   localparam [3:0] INIT_START_BIT             = 4'd3;
   localparam [3:0] INIT_SEND_BYTE             = 4'd4;
   localparam [3:0] INIT_WAIT_ACK              = 4'd5;
   localparam [3:0] INIT_WAIT_AA               = 4'd6;
   localparam [3:0] INIT_RETRY_WAIT            = 4'd7;
   localparam [3:0] INIT_PREP_SET_SCANCODE_CMD = 4'd8;
   localparam [3:0] INIT_PREP_SET_SCANCODE_VAL = 4'd9;
   localparam [3:0] INIT_SETTLE_WAIT           = 4'd10;
   localparam [3:0] INIT_PREP_ENABLE_SCAN      = 4'd11;
   localparam [3:0] INIT_READY                 = 4'd12;

   localparam integer INIT_POWERON_CYCLES      = 32'd29400000;  // ~350ms at 84MHz
   localparam integer INIT_RETRY_WAIT_CYCLES   = 32'd29400000;  // ~350ms at 84MHz
   localparam integer INIT_SETTLE_CYCLES       = 32'd16800000;  // ~200ms at 84MHz
   localparam integer INIT_CLK_INHIBIT_CYCLES  = 32'd16800;    // ~200us at 84MHz
   localparam integer INIT_START_SETUP_CYCLES  = 32'd840;      // ~10us at 84MHz
   localparam integer INIT_ACK_TIMEOUT_CYCLES  = 32'd8400000;   // ~100ms at 84MHz
   localparam integer INIT_BAT_TIMEOUT_CYCLES  = 32'd168000000; // ~2s at 84MHz (covers ~616ms BAT)
   localparam [1:0]   CMD_RESET                = 2'd0;
   localparam [1:0]   CMD_SET_SCANCODE_F0      = 2'd1;
   localparam [1:0]   CMD_SET_SCANCODE_02      = 2'd2;
   localparam [1:0]   CMD_ENABLE_SCAN          = 2'd3;
   localparam [1:0]   MAX_RESET_RETRIES         = 2'd2;         // 0,1,2 => three attempts
   localparam integer PS2_MIN_EDGE_CYCLES      = 16'd1000;     // ~11.9us at 84MHz

   reg                            ps2_clk_drive_low;
   reg                            ps2_data_drive_low;
   wire                           ps2_clk_in;
   wire                           ps2_data_in;

   assign ps2_clk = ps2_clk_drive_low ? 1'b0 : 1'bz;
   assign ps2_data = ps2_data_drive_low ? 1'b0 : 1'bz;
   assign ps2_clk_in = ps2_clk;
   assign ps2_data_in = ps2_data;

   // PS/2 protocol synchronization
   reg                            ps2_clk_r1, ps2_clk_r2, ps2_clk_r3;
   reg                            ps2_data_r1, ps2_data_r2, ps2_data_r3;

   // Synchronize inputs to system clock (metastability protection)
   always @(posedge clk or negedge reset_n) begin
      if (!reset_n) begin
         ps2_clk_r1 <= 1'b1;
         ps2_clk_r2 <= 1'b1;
         ps2_clk_r3 <= 1'b1;
         ps2_data_r1 <= 1'b1;
         ps2_data_r2 <= 1'b1;
         ps2_data_r3 <= 1'b1;
      end
      else begin
         ps2_clk_r1 <= ps2_clk_in;
         ps2_clk_r2 <= ps2_clk_r1;
         ps2_clk_r3 <= ps2_clk_r2;
         ps2_data_r1 <= ps2_data_in;
         ps2_data_r2 <= ps2_data_r1;
         ps2_data_r3 <= ps2_data_r2;
      end
   end

   // PS/2 receivers sample on clock falling edge.
   // Host transmit updates data on rising edge so it is stable before the next fall.
   wire ps2_clk_rise_raw = ps2_clk_r2 & ~ps2_clk_r3;
   wire ps2_clk_fall_raw = ~ps2_clk_r2 & ps2_clk_r3;
   reg [15:0]                     ps2_edge_ctr;
   wire                           ps2_edge_ok = (ps2_edge_ctr >= PS2_MIN_EDGE_CYCLES);
   wire                           ps2_clk_sample_edge = ps2_clk_fall_raw & ps2_edge_ok;
   wire                           ps2_clk_drive_edge  = ps2_clk_rise_raw & ps2_edge_ok;

   always @(posedge clk or negedge reset_n) begin
      if (!reset_n) begin
         ps2_edge_ctr <= PS2_MIN_EDGE_CYCLES;
      end
      else if (ps2_clk_rise_raw || ps2_clk_fall_raw) begin
         if (ps2_edge_ok)
            ps2_edge_ctr <= 16'd0;
      end
      else if (ps2_edge_ctr < 16'hFFFF) begin
         ps2_edge_ctr <= ps2_edge_ctr + 16'd1;
      end
   end

   // PS/2 frame receiver
   reg [3:0]                      bit_count;
   reg [8:0]                      shift_reg;
   reg                            parity_bit;
   reg                            frame_complete;
   reg [7:0]                      scancode;

   always @(posedge clk or negedge reset_n) begin
      if (!reset_n) begin
         bit_count <= 4'b0;
         shift_reg <= 9'b0;
         parity_bit <= 1'b0;
         frame_complete <= 1'b0;
         scancode <= 8'b0;
      end
      else if (ps2_clk_sample_edge) begin
         if (bit_count == 4'b0) begin
            // Start bit (should be 0)
            if (ps2_data_r3 == 1'b0) begin
               bit_count <= 4'd1;
               shift_reg <= 9'b0;
               parity_bit <= 1'b0;
               frame_complete <= 1'b0;
            end
         end
         else if (bit_count >= 4'd1 && bit_count <= 4'd8) begin
            // Data bits (LSB first)
            shift_reg[bit_count - 4'd1] <= ps2_data_r3;
            parity_bit <= parity_bit ^ ps2_data_r3;
            bit_count <= bit_count + 4'd1;
         end
         else if (bit_count == 4'd9) begin
            // Odd parity bit
            if (parity_bit == ps2_data_r3) begin
               // Parity error - discard frame
               bit_count <= 4'b0;
               frame_complete <= 1'b0;
            end
            else begin
               bit_count <= 4'd10;
            end
         end
         else if (bit_count == 4'd10) begin
            // Stop bit (should be 1)
            if (ps2_data_r3 == 1'b1) begin
               scancode <= shift_reg[7:0];
               frame_complete <= 1'b1;
            end
            bit_count <= 4'b0;
         end
      end
      else begin
         frame_complete <= 1'b0;
      end
   end

   // ASCII decode helpers (US layout)
   function [7:0] scancode_to_ascii_unshifted;
      input [7:0] code;
      begin
         case (code)
            8'h1C: scancode_to_ascii_unshifted = 8'h61; // a
            8'h32: scancode_to_ascii_unshifted = 8'h62; // b
            8'h21: scancode_to_ascii_unshifted = 8'h63; // c
            8'h23: scancode_to_ascii_unshifted = 8'h64; // d
            8'h24: scancode_to_ascii_unshifted = 8'h65; // e
            8'h2B: scancode_to_ascii_unshifted = 8'h66; // f
            8'h34: scancode_to_ascii_unshifted = 8'h67; // g
            8'h33: scancode_to_ascii_unshifted = 8'h68; // h
            8'h43: scancode_to_ascii_unshifted = 8'h69; // i
            8'h3B: scancode_to_ascii_unshifted = 8'h6A; // j
            8'h42: scancode_to_ascii_unshifted = 8'h6B; // k
            8'h4B: scancode_to_ascii_unshifted = 8'h6C; // l
            8'h3A: scancode_to_ascii_unshifted = 8'h6D; // m
            8'h31: scancode_to_ascii_unshifted = 8'h6E; // n
            8'h44: scancode_to_ascii_unshifted = 8'h6F; // o
            8'h4D: scancode_to_ascii_unshifted = 8'h70; // p
            8'h15: scancode_to_ascii_unshifted = 8'h71; // q
            8'h2D: scancode_to_ascii_unshifted = 8'h72; // r
            8'h1B: scancode_to_ascii_unshifted = 8'h73; // s
            8'h2C: scancode_to_ascii_unshifted = 8'h74; // t
            8'h3C: scancode_to_ascii_unshifted = 8'h75; // u
            8'h2A: scancode_to_ascii_unshifted = 8'h76; // v
            8'h1D: scancode_to_ascii_unshifted = 8'h77; // w
            8'h22: scancode_to_ascii_unshifted = 8'h78; // x
            8'h35: scancode_to_ascii_unshifted = 8'h79; // y
            8'h1A: scancode_to_ascii_unshifted = 8'h7A; // z
            8'h45: scancode_to_ascii_unshifted = 8'h30; // 0
            8'h16: scancode_to_ascii_unshifted = 8'h31; // 1
            8'h1E: scancode_to_ascii_unshifted = 8'h32; // 2
            8'h26: scancode_to_ascii_unshifted = 8'h33; // 3
            8'h25: scancode_to_ascii_unshifted = 8'h34; // 4
            8'h2E: scancode_to_ascii_unshifted = 8'h35; // 5
            8'h36: scancode_to_ascii_unshifted = 8'h36; // 6
            8'h3D: scancode_to_ascii_unshifted = 8'h37; // 7
            8'h3E: scancode_to_ascii_unshifted = 8'h38; // 8
            8'h46: scancode_to_ascii_unshifted = 8'h39; // 9
            8'h0E: scancode_to_ascii_unshifted = 8'h60; // `
            8'h4E: scancode_to_ascii_unshifted = 8'h2D; // -
            8'h55: scancode_to_ascii_unshifted = 8'h3D; // =
            8'h66: scancode_to_ascii_unshifted = 8'h08; // backspace
            8'h0D: scancode_to_ascii_unshifted = 8'h09; // tab
            8'h5D: scancode_to_ascii_unshifted = 8'h5C; // \
            8'h54: scancode_to_ascii_unshifted = 8'h5B; // [
            8'h5B: scancode_to_ascii_unshifted = 8'h5D; // ]
            8'h4C: scancode_to_ascii_unshifted = 8'h3B; // ;
            8'h52: scancode_to_ascii_unshifted = 8'h27; // '
            8'h5A: scancode_to_ascii_unshifted = 8'h0D; // enter
            8'h29: scancode_to_ascii_unshifted = 8'h20; // space
            8'h41: scancode_to_ascii_unshifted = 8'h2C; // ,
            8'h49: scancode_to_ascii_unshifted = 8'h2E; // .
            8'h4A: scancode_to_ascii_unshifted = 8'h2F; // /
            default: scancode_to_ascii_unshifted = 8'h00;
         endcase
      end
   endfunction

   function [7:0] apply_shift_us;
      input [7:0] code;
      input [7:0] ascii_in;
      begin
         if (ascii_in >= 8'h61 && ascii_in <= 8'h7A) begin
            apply_shift_us = ascii_in - 8'h20; // a-z -> A-Z
         end else begin
            case (code)
               8'h16: apply_shift_us = 8'h21; // 1 -> !
               8'h1E: apply_shift_us = 8'h40; // 2 -> @
               8'h26: apply_shift_us = 8'h23; // 3 -> #
               8'h25: apply_shift_us = 8'h24; // 4 -> $
               8'h2E: apply_shift_us = 8'h25; // 5 -> %
               8'h36: apply_shift_us = 8'h5E; // 6 -> ^
               8'h3D: apply_shift_us = 8'h26; // 7 -> &
               8'h3E: apply_shift_us = 8'h2A; // 8 -> *
               8'h46: apply_shift_us = 8'h28; // 9 -> (
               8'h45: apply_shift_us = 8'h29; // 0 -> )
               8'h0E: apply_shift_us = 8'h7E; // ` -> ~
               8'h4E: apply_shift_us = 8'h5F; // - -> _
               8'h55: apply_shift_us = 8'h2B; // = -> +
               8'h5D: apply_shift_us = 8'h7C; // \ -> |
               8'h54: apply_shift_us = 8'h7B; // [ -> {
               8'h5B: apply_shift_us = 8'h7D; // ] -> }
               8'h4C: apply_shift_us = 8'h3A; // ; -> :
               8'h52: apply_shift_us = 8'h22; // ' -> "
               8'h41: apply_shift_us = 8'h3C; // , -> <
               8'h49: apply_shift_us = 8'h3E; // . -> >
               8'h4A: apply_shift_us = 8'h3F; // / -> ?
               default: apply_shift_us = ascii_in;
            endcase
         end
      end
   endfunction

   function [7:0] scancode_to_ascii;
      input [7:0] code;
      input       shift_on;
      reg [7:0]   ascii_unshifted;
      begin
         ascii_unshifted = scancode_to_ascii_unshifted(code);
         if (shift_on)
            scancode_to_ascii = apply_shift_us(code, ascii_unshifted);
         else
            scancode_to_ascii = ascii_unshifted;
      end
   endfunction

   reg [3:0]                      init_state;
   reg [31:0]                     init_ctr;
   reg [3:0]                      tx_sample_count;
   reg [9:0]                      tx_shift;
   reg                            tx_keyboard_started;
   reg [7:0]                      tx_cmd;
   reg [1:0]                      init_cmd_kind;
   reg [1:0]                      init_retry_count;
   reg                            break_pending;
   reg                            extended_pending;
   reg                            shift_pressed;
   reg [7:0]                      recv_buf_data;
   reg                            recv_buf_valid;
   reg                            rx_ready_pulse_q;

   wire [7:0] ascii_now = scancode_to_ascii(scancode, shift_pressed);
   wire       is_shift_scancode = (scancode == 8'h12) || (scancode == 8'h59);

   assign ps2_do = recv_buf_valid ? {24'h000000, recv_buf_data} : 32'h000000FF;
   assign ps2_ready = ps2_sel;
   assign ps_rx_ready_pulse = rx_ready_pulse_q;

   // Startup sequence aligned to the ESP32 reference:
   // 1) Wait 350ms after power-up.
   // 2) Try reset (0xFF) up to three times.
   // 3) For reset expect 0xFA ACK, then 0xAA BAT complete (500ms timeout).
   // 4) Wait 200ms settle, then set scancode set 2 (0xF0 then 0x02, each with 0xFA ACK).
   // 5) Enter READY and never trigger another reset on incoming 0xAA.
   always @(posedge clk or negedge reset_n) begin
      if (!reset_n) begin
         init_state <= INIT_POWERON_WAIT;
         init_ctr <= 32'd0;
         tx_sample_count <= 4'd0;
         tx_shift <= 10'd0;
         tx_keyboard_started <= 1'b0;
         tx_cmd <= 8'd0;
         init_cmd_kind <= CMD_RESET;
         init_retry_count <= 2'd0;
         ps2_clk_drive_low <= 1'b0;
         ps2_data_drive_low <= 1'b0;
      end
      else begin
         case (init_state)
           INIT_POWERON_WAIT: begin
              ps2_clk_drive_low <= 1'b0;
              ps2_data_drive_low <= 1'b0;
              tx_sample_count <= 4'd0;

              if (init_ctr >= INIT_POWERON_CYCLES) begin
                 init_ctr <= 32'd0;
                 init_retry_count <= 2'd0;
                 init_state <= INIT_PREP_RESET;
              end
              else begin
                 init_ctr <= init_ctr + 32'd1;
              end
           end

           INIT_PREP_RESET: begin
              tx_cmd <= 8'hFF;
              init_cmd_kind <= CMD_RESET;
              init_ctr <= 32'd0;
              tx_sample_count <= 4'd0;
              tx_keyboard_started <= 1'b0;
              init_state <= INIT_CLK_INHIBIT;
           end

           INIT_PREP_SET_SCANCODE_CMD: begin
              tx_cmd <= 8'hF0;
              init_cmd_kind <= CMD_SET_SCANCODE_F0;
              init_ctr <= 32'd0;
              tx_sample_count <= 4'd0;
              tx_keyboard_started <= 1'b0;
              init_state <= INIT_CLK_INHIBIT;
           end

           INIT_PREP_SET_SCANCODE_VAL: begin
              tx_cmd <= 8'h02;
              init_cmd_kind <= CMD_SET_SCANCODE_02;
              init_ctr <= 32'd0;
              tx_sample_count <= 4'd0;
              tx_keyboard_started <= 1'b0;
              init_state <= INIT_CLK_INHIBIT;
           end

           INIT_PREP_ENABLE_SCAN: begin
              tx_cmd <= 8'hF4;
              init_cmd_kind <= CMD_ENABLE_SCAN;
              init_ctr <= 32'd0;
              tx_sample_count <= 4'd0;
              tx_keyboard_started <= 1'b0;
              init_state <= INIT_CLK_INHIBIT;
           end

           INIT_RETRY_WAIT: begin
              ps2_clk_drive_low <= 1'b0;
              ps2_data_drive_low <= 1'b0;
              if (init_ctr >= INIT_RETRY_WAIT_CYCLES) begin
                 init_ctr <= 32'd0;
                 init_state <= INIT_PREP_RESET;
              end
              else begin
                 init_ctr <= init_ctr + 32'd1;
              end
           end

           INIT_CLK_INHIBIT: begin
              ps2_clk_drive_low <= 1'b1;
              ps2_data_drive_low <= 1'b0;
              if (init_ctr >= INIT_CLK_INHIBIT_CYCLES) begin
                 init_ctr <= 32'd0;
                 init_state <= INIT_START_BIT;
              end
              else begin
                 init_ctr <= init_ctr + 32'd1;
              end
           end

           INIT_START_BIT: begin
              ps2_clk_drive_low <= 1'b1;
              ps2_data_drive_low <= 1'b1;
              if (init_ctr >= INIT_START_SETUP_CYCLES) begin
                 ps2_clk_drive_low <= 1'b0;
                 init_ctr <= 32'd0;
                 tx_shift <= {1'b1, ~(^tx_cmd), tx_cmd};
                 tx_sample_count <= 4'd0;
                 tx_keyboard_started <= 1'b0;
                 init_state <= INIT_SEND_BYTE;
              end
              else begin
                 init_ctr <= init_ctr + 32'd1;
              end
           end

           INIT_SEND_BYTE: begin
              ps2_clk_drive_low <= 1'b0;

              // After releasing CLK, ignore the host-generated release edge.
              // Wait for the keyboard to start clocking and advance bits only on
              // keyboard falling edges (receiver sample edges).
              if (!tx_keyboard_started && ps2_clk_sample_edge) begin
                 // First falling edge samples the start bit we are already driving low.
                 // Now place data bit 0 for the next bit cell.
                 tx_keyboard_started <= 1'b1;
                 ps2_data_drive_low <= ~tx_shift[0];
                 tx_shift <= {1'b1, tx_shift[9:1]};
                 tx_sample_count <= 4'd0;
                 init_ctr <= 32'd0;
              end
              else if (tx_keyboard_started && ps2_clk_sample_edge) begin
                 init_ctr <= 32'd0;
                 if (tx_sample_count == 4'd9) begin
                    // Stop bit sampled. Release DAT so keyboard can send ACK.
                    ps2_data_drive_low <= 1'b0;
                    init_state <= INIT_WAIT_ACK;
                 end
                 else begin
                    tx_sample_count <= tx_sample_count + 4'd1;
                    ps2_data_drive_low <= ~tx_shift[0];
                    tx_shift <= {1'b1, tx_shift[9:1]};
                 end
              end
              else if (init_ctr < INIT_ACK_TIMEOUT_CYCLES) begin
                 init_ctr <= init_ctr + 32'd1;
              end
              else begin
                 // Could not clock out command, fail open.
                 init_state <= INIT_READY;
                 init_ctr <= 32'd0;
                 ps2_clk_drive_low <= 1'b0;
                 ps2_data_drive_low <= 1'b0;
                 tx_sample_count <= 4'd0;
                 tx_keyboard_started <= 1'b0;
              end
           end

           INIT_WAIT_ACK: begin
              ps2_clk_drive_low <= 1'b0;
              ps2_data_drive_low <= 1'b0;

              if (frame_complete && scancode == 8'hFA) begin
                 init_ctr <= 32'd0;
                 if (init_cmd_kind == CMD_RESET) begin
                    init_state <= INIT_WAIT_AA;
                 end
                 else if (init_cmd_kind == CMD_SET_SCANCODE_F0) begin
                    init_state <= INIT_PREP_SET_SCANCODE_VAL;
                 end
                 else if (init_cmd_kind == CMD_ENABLE_SCAN) begin
                    init_state <= INIT_READY;
                 end
                 else begin
                    init_state <= INIT_SETTLE_WAIT;
                 end
              end
              else if (init_ctr < INIT_ACK_TIMEOUT_CYCLES) begin
                 init_ctr <= init_ctr + 32'd1;
              end
              else begin
                 init_ctr <= 32'd0;
                 if (init_cmd_kind == CMD_RESET && init_retry_count < MAX_RESET_RETRIES) begin
                    init_retry_count <= init_retry_count + 2'd1;
                    init_state <= INIT_RETRY_WAIT;
                 end
                 else begin
                    init_state <= INIT_READY;
                 end
              end
           end

           INIT_WAIT_AA: begin
              ps2_clk_drive_low <= 1'b0;
              ps2_data_drive_low <= 1'b0;

              if (frame_complete && scancode == 8'hAA) begin
                 init_ctr <= 32'd0;
                 init_state <= INIT_PREP_SET_SCANCODE_CMD;
              end
              else if (init_ctr < INIT_BAT_TIMEOUT_CYCLES) begin
                 init_ctr <= init_ctr + 32'd1;
              end
              else begin
                 init_ctr <= 32'd0;
                 if (init_retry_count < MAX_RESET_RETRIES) begin
                    init_retry_count <= init_retry_count + 2'd1;
                    init_state <= INIT_RETRY_WAIT;
                 end
                 else begin
                    init_state <= INIT_READY;
                 end
              end
           end

           INIT_SETTLE_WAIT: begin
              ps2_clk_drive_low <= 1'b0;
              ps2_data_drive_low <= 1'b0;
              if (init_ctr >= INIT_SETTLE_CYCLES) begin
                 init_ctr <= 32'd0;
                 init_state <= INIT_READY;
              end
              else begin
                 init_ctr <= init_ctr + 32'd1;
              end
           end

           default: begin
              init_state <= INIT_READY;
              ps2_clk_drive_low <= 1'b0;
              ps2_data_drive_low <= 1'b0;
           end
         endcase

         // ESP32-style runtime recovery: if a live keyboard suddenly emits BAT
         // completion (0xAA) while we are already in READY, treat it as a hot-plug
         // event and send Enable Scanning (0xF4) once.
         if (init_state == INIT_READY && frame_complete && scancode == 8'hAA) begin
            init_ctr <= 32'd0;
            tx_sample_count <= 4'd0;
            tx_keyboard_started <= 1'b0;
            init_state <= INIT_PREP_ENABLE_SCAN;
         end
      end
   end

   always @(posedge clk or negedge reset_n) begin
      if (!reset_n) begin
         break_pending <= 1'b0;
         extended_pending <= 1'b0;
         shift_pressed <= 1'b0;
         recv_buf_data <= 8'h00;
         recv_buf_valid <= 1'b0;
         rx_ready_pulse_q <= 1'b0;
      end
      else begin
         rx_ready_pulse_q <= 1'b0;

         // Read consumes the buffered character.
         if (ps2_sel)
            recv_buf_valid <= 1'b0;

         if (frame_complete && (init_state == INIT_READY)) begin
            if (scancode == 8'hE0) begin
               extended_pending <= 1'b1;
            end
            else if (scancode == 8'hF0) begin
               break_pending <= 1'b1;
            end
            else begin
               if (!extended_pending && is_shift_scancode) begin
                  shift_pressed <= !break_pending;
               end
               else if (!extended_pending && !break_pending && (ascii_now != 8'h00)) begin
                  recv_buf_data <= ascii_now;
                  recv_buf_valid <= 1'b1;
                  rx_ready_pulse_q <= 1'b1;
               end
               else if (extended_pending && !break_pending) begin
                  // Arrow keys (E0-prefixed): emit single-byte control codes
                  case (scancode)
                     8'h75: begin recv_buf_data <= 8'h11; recv_buf_valid <= 1'b1; rx_ready_pulse_q <= 1'b1; end // Up
                     8'h72: begin recv_buf_data <= 8'h12; recv_buf_valid <= 1'b1; rx_ready_pulse_q <= 1'b1; end // Down
                     8'h6B: begin recv_buf_data <= 8'h13; recv_buf_valid <= 1'b1; rx_ready_pulse_q <= 1'b1; end // Left
                     8'h74: begin recv_buf_data <= 8'h14; recv_buf_valid <= 1'b1; rx_ready_pulse_q <= 1'b1; end // Right
                     default: ;
                  endcase
               end

               break_pending <= 1'b0;
               extended_pending <= 1'b0;
            end
         end
      end
   end

endmodule
