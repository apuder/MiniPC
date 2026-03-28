
module display (
    input  wire        clk,
    input  wire        rst,

    input              vga_clk,
    input              genlock,
    output wire        pixel_data,

    // picorv32 memory interface
    input  wire        mem_valid,
    input  wire [31:0] mem_addr,       // byte address
    input  wire [31:0] mem_wdata,
    input  wire [3:0]  mem_wstrb,      // byte enables; 0 => read

    output wire        mem_ready,
    output wire [31:0] mem_rdata
);

// picorv32 memory protocol is word-oriented:
// - mem_addr is word-aligned
// - mem_wstrb selects which bytes in that word are written
// This BRAM is 8-bit wide, so each transaction is expanded to 4 byte steps.

reg        busy;
reg        op_write;
reg [1:0]  phase;
reg [10:0] base_addr;
reg [31:0] wdata_lat;
reg [3:0]  wstrb_lat;
reg [31:0] rdata_lat;
reg        mem_ready_lat;

reg        bram_cea;
reg        bram_wrea;
reg [10:0] bram_ada;
reg [7:0]  bram_dina;
wire [7:0] bram_dout;

assign mem_ready = mem_ready_lat;
assign mem_rdata = rdata_lat;

wire [10:0] phase_addr = base_addr + {9'b0, phase};
wire [7:0] phase_wbyte = (phase == 2'b00) ? wdata_lat[7:0] :
                         (phase == 2'b01) ? wdata_lat[15:8] :
                         (phase == 2'b10) ? wdata_lat[23:16] :
                                           wdata_lat[31:24];

always @(*) begin
  bram_cea  = busy || mem_valid;
  bram_wrea = 1'b0;
  bram_ada  = busy ? phase_addr : mem_addr[10:0];
  bram_dina = phase_wbyte;

  if (busy && op_write)
    bram_wrea = wstrb_lat[phase];
end

always @(posedge clk) begin
  if (rst) begin
    busy         <= 1'b0;
    op_write     <= 1'b0;
    phase        <= 2'b00;
    base_addr    <= 11'b0;
    wdata_lat    <= 32'b0;
    wstrb_lat    <= 4'b0;
    rdata_lat    <= 32'b0;
    mem_ready_lat <= 1'b0;
  end else begin
    mem_ready_lat <= 1'b0;

    if (!busy) begin
      if (mem_valid) begin
        busy      <= 1'b1;
        op_write  <= |mem_wstrb;
        phase     <= |mem_wstrb ? 2'b00 : 2'b01;  // Writes start at 0, reads at 1
        base_addr <= mem_addr[10:0];
        wdata_lat <= mem_wdata;
        wstrb_lat <= mem_wstrb;
      end
    end else begin
      // Capture read data from PREVIOUS address (BRAM has 1-cycle latency for registered addr)
      if (!op_write) begin
        case (phase)
          2'b01: rdata_lat[7:0]   <= bram_dout;  // Data from addr+0
          2'b10: rdata_lat[15:8]  <= bram_dout;  // Data from addr+1
          2'b11: rdata_lat[23:16] <= bram_dout;  // Data from addr+2
          2'b00: rdata_lat[31:24] <= bram_dout;  // Data from addr+3
        endcase
      end

      // Complete transaction after all 4 phases
      if ((op_write && phase == 2'b11) || (!op_write && phase == 2'b00)) begin
        busy          <= 1'b0;
        mem_ready_lat <= 1'b1;
        phase         <= 2'b00;
      end else begin
        phase <= phase + 2'b01;
      end
    end
  end
end


wire   opreg_80_64_n  = 1'b1;
wire   opreg_invvide  = 1'b0;
wire   mod_modsel = 1'b0; // forward reference
wire   mod_enaltset = 1'b0; // forward reference

wire [7:0] char_rom_data;
wire [7:0] chr_dout;

assign pixel_data = dsp_pixel_shift_reg[7];

// The VGA display is 640x480.
// The pixel clock is divided by two and each row of the TRS-80 display is repeated two times
// for an effective resolution of 640x240 which is slightly larger than the 512x192 native
// resolution of the M3 display resulting in a small border around the M3 display.
// In 64x16 mode the characters are 8x12 or 8x24 when rows are repeated.
// In 80x24 mode the characters are 8x10 or 8x20 when rows are repeated.
// For convenience the VGA X and Y counters are partitioned into high and low parts which
// count the character position and the position within the character respectively.
reg vga_80_64_n;
reg [2:0] vga_xxx;     // 0-7
reg [6:0] vga_XXXXXXX; // 0-79 active, -99 total
reg [4:0] vga_yyyyy;   // 0-23 in 64x16 mode, 0-19 in 80x24 mode
reg [4:0] vga_YYYYY;   // 0-19 active, -21-20/24 total in 64x16 mode, 0-23 active, -26-4/20 total in 80x24 mode
reg vga_Z;
// VGA in active area.
wire vga_act = vga_80_64_n ?
               ((vga_XXXXXXX < 7'd80) & (vga_YYYYY < 5'd24)) :
               ((vga_XXXXXXX < 7'd80) & (vga_YYYYY < 5'd20));

// Center the 64x16 text display in the 640x480 VGA display.
wire [6:0] dsp_XXXXXXX = vga_XXXXXXX - 7'd8;
wire [4:0] dsp_YYYYY   = vga_YYYYY   - 5'd2;
// Display in active area.
wire dsp_act = vga_80_64_n ?
               ((vga_XXXXXXX < 7'd80) & (vga_YYYYY < 5'd24)) :
               ((dsp_XXXXXXX < 7'd64) & (dsp_YYYYY < 5'd16));
// 64/32 or 80/40 column display mode.
// If modsel=1 then in 32/40 column mode.
// in 32/40 column mode only the even columns are active.
wire col_act = (mod_modsel ? ~dsp_XXXXXXX[0] : 1'b1);

wire [7:0] trs_dsp_data_b;

wire [7:0] dummy_out;

// Pre-accumulated row base address for 80x24 mode: row80_base = 80 * vga_YYYYY.
// Updated once per text row to replace the double carry-chain adder
// ({Y,6'b0} + {Y,4'b0,2'b0} + X) in the BRAM address path with a single
// addition (row80_base + X), halving carry-chain resource consumption and
// allowing the P&R tool to place SPI logic in better locations.
reg [10:0] row80_base;

always @(posedge vga_clk) begin
    if (genlock) begin
        row80_base <= 11'd0;
    end else if (vga_xxx == 3'b111 && vga_XXXXXXX == 7'd99) begin
        if ({vga_YYYYY, vga_yyyyy} == {5'd26, 5'd4})
            row80_base <= 11'd0;           // frame wrap
        else if (vga_yyyyy == 5'd19)
            row80_base <= row80_base + 11'd80; // next text row
    end
end

blk_mem_gen_2 trs_dsp (
   .clka(clk),
   .cea(bram_cea),
   .ada(bram_ada),
   .wrea(bram_wrea),
   .dina(bram_dina),
   .douta(bram_dout),
   .ocea(1'b1),
   .reseta(1'b0),

   .clkb(vga_clk), // input
   .ceb(dsp_act & col_act & (vga_xxx == 3'b000)), // input
   .adb(vga_80_64_n ?
        (row80_base + {4'b0, vga_XXXXXXX}) : // pre-accumulated 80*Y + X, single adder
        {1'b0, dsp_YYYYY[3:0], dsp_XXXXXXX[5:0]}), // input [10:0]
   .wreb(1'b0), // input
   .dinb(8'h00), // input [7:0]
   .doutb(trs_dsp_data_b), // output [7:0]
   .oceb(dsp_act & col_act & (vga_xxx == 3'b001)), // input
   .resetb(1'b0)
);

blk_mem_gen_3 char_rom (
   .clka(vga_clk), // input
   .cea(dsp_act & col_act & (vga_yyyyy[4] == 1'b0) & (vga_xxx == 3'b010)), // input
   .ada({trs_dsp_data_b[7] & ~opreg_invvide,
         trs_dsp_data_b[6] & ~(trs_dsp_data_b[7] & ~opreg_invvide & mod_enaltset),
         trs_dsp_data_b[5:0], vga_yyyyy[3:1]}), // input [11:0]
   .wrea(1'b0),
   .dina(8'b00000000), // input [7:0]
   .douta(char_rom_data), // output [7:0]
   .ocea(dsp_act & col_act & (vga_yyyyy[4] == 1'b0) & (vga_xxx == 3'b011)),
   .reseta(1'b0),

   .clkb(0), // input
   .ceb(0), // input
   .adb(0), // input [11:0]
   .wreb(0),
   .dinb(0), // input [7:0]
   .doutb(dummy_out), // output [7:0]
   .oceb(1'b1),
   .resetb(1'b0)
);

// Latch the character rom address with the same latency as the rom.
// This is the block graphic.
reg [11:0] char_rom_addr, _char_rom_addr;

always @ (posedge vga_clk)
begin
   if(dsp_act & col_act & (vga_xxx == 3'b010))
      _char_rom_addr <= {trs_dsp_data_b, vga_yyyyy[4:1]};
   if(dsp_act & col_act & (vga_xxx == 3'b011))
      char_rom_addr <= _char_rom_addr;
end


// Bump the VGA counters.
always @ (posedge vga_clk)
begin
   if(genlock)
   begin
      vga_xxx <= 3'b000;
      vga_XXXXXXX <= 7'd0;
      vga_yyyyy <= 5'd0;
      vga_YYYYY <= 5'd0;
      vga_Z <= ~vga_Z;
      vga_80_64_n <= opreg_80_64_n;
   end
   else
   begin
      if(vga_xxx == 3'b111)
      begin
         if(vga_XXXXXXX == 7'd99)
         begin
            vga_XXXXXXX <= 7'd0;

            if({vga_YYYYY, vga_yyyyy} == (vga_80_64_n ? {5'd26, 5'd4} : {5'd21, 5'd20}))
            begin
               vga_yyyyy <= 5'd0;
               vga_YYYYY <= 5'd0;
               vga_Z <= ~vga_Z;
               vga_80_64_n <= opreg_80_64_n;
            end
            else if(vga_yyyyy == (vga_80_64_n ? 5'd19 : 5'd23))
            begin
               vga_yyyyy <= 5'd0;
               vga_YYYYY <= vga_YYYYY + 5'd1;
            end
            else
               vga_yyyyy <= vga_yyyyy + 5'd1;
         end
         else
            vga_XXXXXXX <= vga_XXXXXXX + 7'd1;
      end
      vga_xxx <= vga_xxx + 3'b1;
   end
end


// Load the display pixel data into the pixel shift register, or shift current contents.
reg [7:0] dsp_pixel_shift_reg;

always @ (posedge vga_clk)
begin
   // If the msb's are 10 and not inverse video then it's block graphic.
   // Otherwise it's character data from the character rom.
   if(dsp_act & col_act & (vga_xxx == 3'b100))
   begin
      if(~((char_rom_addr[11:10] == 2'b10) & ~opreg_invvide))
         dsp_pixel_shift_reg <= (char_rom_addr[3] ? 8'h00 : char_rom_data) ^ {8{char_rom_addr[11] & opreg_invvide}};
      else
      begin
         // The character is 12 rows.
         case(char_rom_addr[3:2])
            2'b00: dsp_pixel_shift_reg <= {{4{char_rom_addr[4]}}, {4{char_rom_addr[5]}}};
            2'b01: dsp_pixel_shift_reg <= {{4{char_rom_addr[6]}}, {4{char_rom_addr[7]}}};
            2'b10: dsp_pixel_shift_reg <= {{4{char_rom_addr[8]}}, {4{char_rom_addr[9]}}};
            2'b11: dsp_pixel_shift_reg <= 8'h00; // should never happen
         endcase
      end
   end
   else
   begin
      // If 32 column mode then shift only every other clock.
      // Note the vga_xxx[0] value here (0 or 1) must be the same as the lsb used above
      // so that the load cycle would also be a shift cycle.
      if(mod_modsel ? (vga_xxx[0] == 1'b0) : 1'b1)
         dsp_pixel_shift_reg <= {dsp_pixel_shift_reg[6:0], 1'b0};
   end
end

endmodule
