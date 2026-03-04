
module display (
    input  wire        clk,
    input  wire        rst,

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

blk_mem_gen_2 trs_dsp (
   .clka(clk),
   .cea(bram_cea),
   .ada(bram_ada),
   .wrea(bram_wrea),
   .dina(bram_dina),
   .douta(bram_dout),
   .ocea(1'b1),
   .reseta(1'b0),
   .clkb(1'b0),
   .ceb(1'b0),
   .adb(11'b0),
   .wreb(1'b0),
   .dinb(8'h00),
   .doutb(),
   .oceb(1'b0),
   .resetb(1'b0)
);

endmodule
