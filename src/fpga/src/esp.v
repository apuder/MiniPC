
module esp(
    input              clk,          // System clock
    input              reset_n,      // Active low reset
    output reg [2:0]   esp_s,        // ESP 3-bit request code
    output             req,          // High when a request is made to the ESP
    input              done,         // High when the ESP has completed the request

    // picorv32 memory interface
    input  wire        mem_valid,
    input  wire [31:0] mem_wdata,
    input  wire [3:0]  mem_wstrb,    // byte enables; 0 => read

    output wire        mem_ready,
    output wire [31:0] mem_rdata
);

reg       done_sync_ff1;
reg       done_sync_ff2;
reg       done_sync_ff2_prev;
reg       done_seen;

reg       req_start_pending;
reg [3:0] req_count;

wire      is_write;
wire      done_rise;

assign is_write = mem_valid && (|mem_wstrb);
assign done_rise = done_sync_ff2 && !done_sync_ff2_prev;

assign req = (req_count != 4'd0);
assign mem_ready = mem_valid;
assign mem_rdata = {31'b0, done_seen};

always @(posedge clk or negedge reset_n) begin
    if (!reset_n) begin
        esp_s              <= 3'b000;
        done_sync_ff1      <= 1'b0;
        done_sync_ff2      <= 1'b0;
        done_sync_ff2_prev <= 1'b0;
        done_seen          <= 1'b0;
        req_start_pending  <= 1'b0;
        req_count          <= 4'd0;
    end else begin
        // Synchronize done into clk domain before edge detection.
        done_sync_ff1 <= done;
        done_sync_ff2 <= done_sync_ff1;
        done_sync_ff2_prev <= done_sync_ff2;

        // New write captures command and arms request pulse for next cycle.
        if (is_write) begin
            esp_s <= mem_wdata[2:0];
            req_start_pending <= 1'b1;
            done_seen <= 1'b0;
        end

        // Latch completion event from synchronized done rising edge.
        if (done_rise)
            done_seen <= 1'b1;

        // Start request exactly one cycle after write, then hold for 10 clocks.
        if (req_start_pending) begin
            req_start_pending <= 1'b0;
            req_count <= 4'd10;
        end else if (req_count != 4'd0) begin
            req_count <= req_count - 4'd1;
        end
    end
end

endmodule
