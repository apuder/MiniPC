module debug (
  input wire        clk,
  input wire        clk_in,
  input wire        reset_n,
  input wire        dbg_enable,
  input wire [31:0] dbg_starting_addr,
  input wire        mem_valid,
  input wire        mem_instr,
  input wire [31:0] mem_addr,
  input wire        mem_ready_raw,
  input wire [31:0] mem_rdata_raw,
  output wire       mem_ready,
  output wire [31:0] mem_rdata,
  output wire       dbg_clk,
  output wire       dbg_data
);

  reg        dbg_trace_started;
  reg        dbg_trace_pending;
  reg [31:0] dbg_trace_addr_clk;
  reg        dbg_trace_req_toggle;
  reg [31:0] dbg_trace_rdata_latched;
  reg        dbg_trace_response_latched;
  reg        dbg_trace_done;
  reg        dbg_trace_ack_sync_0;
  reg        dbg_trace_ack_sync_1;
  reg        dbg_trace_ack_sync_1_q;

  reg        dbg_trace_req_sync_0;
  reg        dbg_trace_req_sync_1;
  reg        dbg_trace_req_sync_1_q;
  reg [31:0] dbg_trace_addr_sync_0;
  reg [31:0] dbg_trace_addr_sync_1;
  reg [31:0] dbg_shift_addr;
  reg [5:0]  dbg_shift_bits_remaining;
  reg        dbg_shift_active;
  reg        dbg_clk_q;
  reg        dbg_ack_toggle;
  reg        dbg_data_q;

  wire dbg_trace_start_hit = mem_valid && mem_instr && (mem_addr == dbg_starting_addr);
  wire dbg_trace_fetch = dbg_enable && mem_valid && mem_instr && (dbg_trace_started || dbg_trace_start_hit || dbg_trace_pending);
  wire dbg_trace_release = dbg_trace_pending && dbg_trace_done && (dbg_trace_response_latched || mem_ready_raw);

  assign mem_ready = dbg_trace_fetch ? dbg_trace_release : mem_ready_raw;
  assign mem_rdata = dbg_trace_response_latched ? dbg_trace_rdata_latched : mem_rdata_raw;
  // SPI Mode 0: idle clock low, data stable before each rising edge.
  assign dbg_clk = dbg_clk_q;
  assign dbg_data = dbg_data_q;

  always @(posedge clk or negedge reset_n) begin
    if (!reset_n) begin
      dbg_trace_started <= 1'b0;
      dbg_trace_pending <= 1'b0;
      dbg_trace_addr_clk <= 32'b0;
      dbg_trace_req_toggle <= 1'b0;
      dbg_trace_rdata_latched <= 32'b0;
      dbg_trace_response_latched <= 1'b0;
      dbg_trace_done <= 1'b0;
      dbg_trace_ack_sync_0 <= 1'b0;
      dbg_trace_ack_sync_1 <= 1'b0;
      dbg_trace_ack_sync_1_q <= 1'b0;
    end else begin
      dbg_trace_ack_sync_0 <= dbg_ack_toggle;
      dbg_trace_ack_sync_1 <= dbg_trace_ack_sync_0;
      dbg_trace_ack_sync_1_q <= dbg_trace_ack_sync_1;

      if (!dbg_enable) begin
        dbg_trace_started <= 1'b0;
        dbg_trace_pending <= 1'b0;
        dbg_trace_response_latched <= 1'b0;
        dbg_trace_done <= 1'b0;
      end else begin
        if (dbg_trace_ack_sync_1 != dbg_trace_ack_sync_1_q)
          dbg_trace_done <= 1'b1;

        if (dbg_trace_pending && mem_ready_raw && !dbg_trace_release) begin
          dbg_trace_response_latched <= 1'b1;
          dbg_trace_rdata_latched <= mem_rdata_raw;
        end

        if (!dbg_trace_pending && dbg_trace_fetch) begin
          dbg_trace_pending <= 1'b1;
          dbg_trace_done <= 1'b0;
          dbg_trace_response_latched <= 1'b0;
          dbg_trace_addr_clk <= mem_addr;
          dbg_trace_req_toggle <= ~dbg_trace_req_toggle;
          if (!dbg_trace_started)
            dbg_trace_started <= dbg_trace_start_hit;
        end else if (dbg_trace_release) begin
          dbg_trace_pending <= 1'b0;
          dbg_trace_done <= 1'b0;
          dbg_trace_response_latched <= 1'b0;
        end
      end
    end
  end

  always @(posedge clk_in or negedge reset_n) begin
    if (!reset_n) begin
      dbg_trace_req_sync_0 <= 1'b0;
      dbg_trace_req_sync_1 <= 1'b0;
      dbg_trace_req_sync_1_q <= 1'b0;
      dbg_trace_addr_sync_0 <= 32'b0;
      dbg_trace_addr_sync_1 <= 32'b0;
      dbg_shift_addr <= 32'b0;
      dbg_shift_bits_remaining <= 6'b0;
      dbg_shift_active <= 1'b0;
      dbg_clk_q <= 1'b0;
      dbg_ack_toggle <= 1'b0;
      dbg_data_q <= 1'b0;
    end else begin
      dbg_trace_req_sync_0 <= dbg_trace_req_toggle;
      dbg_trace_req_sync_1 <= dbg_trace_req_sync_0;
      dbg_trace_req_sync_1_q <= dbg_trace_req_sync_1;
      dbg_trace_addr_sync_0 <= dbg_trace_addr_clk;
      dbg_trace_addr_sync_1 <= dbg_trace_addr_sync_0;

      if (!dbg_enable) begin
        dbg_shift_active <= 1'b0;
        dbg_shift_bits_remaining <= 6'b0;
        dbg_clk_q <= 1'b0;
        dbg_data_q <= 1'b0;
      end else if (dbg_shift_active) begin
        if (!dbg_clk_q) begin
          dbg_clk_q <= 1'b1;
        end else begin
          dbg_clk_q <= 1'b0;
          if (dbg_shift_bits_remaining == 6'd1) begin
            dbg_shift_active <= 1'b0;
            dbg_shift_bits_remaining <= 6'b0;
            dbg_ack_toggle <= ~dbg_ack_toggle;
            dbg_data_q <= 1'b0;
          end else begin
            dbg_shift_addr <= {dbg_shift_addr[30:0], 1'b0};
            dbg_shift_bits_remaining <= dbg_shift_bits_remaining - 1'b1;
            dbg_data_q <= dbg_shift_addr[30];
          end
        end
      end else if (dbg_trace_req_sync_1 != dbg_trace_req_sync_1_q) begin
        // Load MOSI while SCLK is low so the next rising edge is the sample edge.
        dbg_shift_active <= 1'b1;
        dbg_shift_addr <= dbg_trace_addr_sync_1;
        dbg_shift_bits_remaining <= 6'd32;
        dbg_clk_q <= 1'b0;
        dbg_data_q <= dbg_trace_addr_sync_1[31];
      end else begin
        dbg_clk_q <= 1'b0;
        dbg_data_q <= 1'b0;
      end
    end
  end

endmodule
