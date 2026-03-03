
// PS/2 Keyboard Interface Module
// Translates PS/2 scan codes to ASCII key codes (US layout)
module ps2
   (
    input                         clk,          // System clock, 27Mhz
    input                         reset_n,      // Active low reset
    input                         ps2_clk,      // PS/2 clock
    input                         ps2_data,     // PS/2 data
    output                        key_event,    // High when a new key event occurs
    output [7:0]                  key,          // ASCII key code
    output                        key_released  // High if the key was released
    );

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
         ps2_clk_r1 <= ps2_clk;
         ps2_clk_r2 <= ps2_clk_r1;
         ps2_clk_r3 <= ps2_clk_r2;
         ps2_data_r1 <= ps2_data;
         ps2_data_r2 <= ps2_data_r1;
         ps2_data_r3 <= ps2_data_r2;
      end
   end
   
   // Detect falling edge on PS/2 clock
   wire ps2_clk_fall = ps2_clk_r2 & ~ps2_clk_r3;
   
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
         frame_complete <= 1'b0;
         scancode <= 8'b0;
      end
      else if (ps2_clk_fall) begin
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
            // Parity bit
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
   
   // Key code state machine
   reg [1:0]                      state;
   reg [7:0]                      last_scancode;
   reg                            key_released_reg;
   reg                            key_event_reg;
   reg [7:0]                      ascii_code;
   
   localparam IDLE = 2'b00;
   localparam BREAK = 2'b01;
   localparam EXTENDED = 2'b10;
   localparam EXTENDED_BREAK = 2'b11;
   
   // PS/2 scan code to ASCII mapping (US layout, unshifted)
   function [7:0] scancode_to_ascii;
      input [7:0] code;
      begin
         case (code)
            8'h1C: scancode_to_ascii = 8'h61; // A
            8'h32: scancode_to_ascii = 8'h62; // B
            8'h21: scancode_to_ascii = 8'h63; // C
            8'h23: scancode_to_ascii = 8'h64; // D
            8'h24: scancode_to_ascii = 8'h65; // E
            8'h2B: scancode_to_ascii = 8'h66; // F
            8'h34: scancode_to_ascii = 8'h67; // G
            8'h33: scancode_to_ascii = 8'h68; // H
            8'h43: scancode_to_ascii = 8'h69; // I
            8'h3B: scancode_to_ascii = 8'h6A; // J
            8'h42: scancode_to_ascii = 8'h6B; // K
            8'h4B: scancode_to_ascii = 8'h6C; // L
            8'h3A: scancode_to_ascii = 8'h6D; // M
            8'h31: scancode_to_ascii = 8'h6E; // N
            8'h44: scancode_to_ascii = 8'h6F; // O
            8'h4D: scancode_to_ascii = 8'h70; // P
            8'h15: scancode_to_ascii = 8'h71; // Q
            8'h2D: scancode_to_ascii = 8'h72; // R
            8'h1B: scancode_to_ascii = 8'h73; // S
            8'h2C: scancode_to_ascii = 8'h74; // T
            8'h3C: scancode_to_ascii = 8'h75; // U
            8'h2A: scancode_to_ascii = 8'h76; // V
            8'h1D: scancode_to_ascii = 8'h77; // W
            8'h22: scancode_to_ascii = 8'h78; // X
            8'h35: scancode_to_ascii = 8'h79; // Y
            8'h1A: scancode_to_ascii = 8'h7A; // Z
            8'h45: scancode_to_ascii = 8'h30; // 0
            8'h16: scancode_to_ascii = 8'h31; // 1
            8'h1E: scancode_to_ascii = 8'h32; // 2
            8'h26: scancode_to_ascii = 8'h33; // 3
            8'h25: scancode_to_ascii = 8'h34; // 4
            8'h2E: scancode_to_ascii = 8'h35; // 5
            8'h36: scancode_to_ascii = 8'h36; // 6
            8'h3D: scancode_to_ascii = 8'h37; // 7
            8'h3E: scancode_to_ascii = 8'h38; // 8
            8'h46: scancode_to_ascii = 8'h39; // 9
            8'h0E: scancode_to_ascii = 8'h60; // `
            8'h4E: scancode_to_ascii = 8'h2D; // -
            8'h55: scancode_to_ascii = 8'h3D; // =
            8'h66: scancode_to_ascii = 8'h08; // Backspace
            8'h0D: scancode_to_ascii = 8'h09; // Tab
            8'h5D: scancode_to_ascii = 8'h5C; // \
            8'h54: scancode_to_ascii = 8'h5B; // [
            8'h5B: scancode_to_ascii = 8'h5D; // ]
            8'h3F: scancode_to_ascii = 8'h3B; // ;
            8'h52: scancode_to_ascii = 8'h27; // '
            8'h5A: scancode_to_ascii = 8'h0D; // Enter
            8'h29: scancode_to_ascii = 8'h20; // Space
            8'h41: scancode_to_ascii = 8'h2C; // ,
            8'h49: scancode_to_ascii = 8'h2E; // .
            8'h4A: scancode_to_ascii = 8'h2F; // /
            default: scancode_to_ascii = 8'h00; // Unknown
         endcase
      end
   endfunction
   
   always @(posedge clk or negedge reset_n) begin
      if (!reset_n) begin
         state <= IDLE;
         last_scancode <= 8'b0;
         key_released_reg <= 1'b0;
         key_event_reg <= 1'b0;
         ascii_code <= 8'b0;
      end
      else if (frame_complete) begin
         case (state)
            IDLE: begin
               if (scancode == 8'hF0) begin
                  // Break code (key release)
                  state <= BREAK;
                  key_event_reg <= 1'b0;
               end
               else if (scancode == 8'hE0) begin
                  // Extended key
                  state <= EXTENDED;
                  key_event_reg <= 1'b0;
               end
               else begin
                  // Regular key press
                  last_scancode <= scancode;
                  ascii_code <= scancode_to_ascii(scancode);
                  key_released_reg <= 1'b0;
                  key_event_reg <= 1'b1;
                  state <= IDLE;
               end
            end
            BREAK: begin
               // After F0, next byte is the released key
               last_scancode <= scancode;
               ascii_code <= scancode_to_ascii(scancode);
               key_released_reg <= 1'b1;
               key_event_reg <= 1'b1;
               state <= IDLE;
            end
            EXTENDED: begin
               if (scancode == 8'hF0) begin
                  // Extended key release
                  state <= EXTENDED_BREAK;
                  key_event_reg <= 1'b0;
               end
               else begin
                  // Extended key press (not typically used for ASCII)
                  last_scancode <= scancode;
                  ascii_code <= 8'h00;
                  key_released_reg <= 1'b0;
                  key_event_reg <= 1'b1;
                  state <= IDLE;
               end
            end
            EXTENDED_BREAK: begin
               // Extended key release
               last_scancode <= scancode;
               ascii_code <= 8'h00;
               key_released_reg <= 1'b1;
               key_event_reg <= 1'b1;
               state <= IDLE;
            end
         endcase
      end
      else begin
         key_event_reg <= 1'b0;
      end
   end
   
   // Assign outputs
   assign key_event = key_event_reg;
   assign key = ascii_code;
   assign key_released = key_released_reg;

endmodule