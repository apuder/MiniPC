
module spi(
    input              clk,          // System clock
    input              reset_n,      // Active low reset
    input              spi_cs,       // SPI chip select (active low)
    input              spi_clk,      // SPI clock
    input              spi_mosi,     // SPI Master Out Slave In
    output             spi_miso,     // SPI Master In Slave Out

    // picorv32 memory interface
    input  wire        mem_valid_spi_in,
    input  wire        mem_valid_spi_out,
    input  wire [31:0] mem_addr,       // byte address
    input  wire [31:0] mem_wdata,
    input  wire [3:0]  mem_wstrb,      // byte enables; 0 => read

    output wire        mem_ready,
    output wire [31:0] mem_rdata
);

// Port A is attached to the picorv32 memory bus.
// PicoRV32 sends byte addresses. We sequence through 4 bytes starting from mem_addr.
// step timeline with READ_MODE0=1 (2-cycle read latency):
//   Writes: step 0-3 = present+write byte 0-3
//   Reads:  prefetch in !busy at addr+0
//           step 1: present addr+1
//           step 2: capture byte0, present addr+2
//           step 3: capture byte1, present addr+3
//           step 4: capture byte2
//           step 5: capture byte3, done
reg        busy;
reg        op_write;
reg [2:0]  step;       // 0..5
reg [7:0]  base_addr;
reg [31:0] wdata_lat;
reg [3:0]  wstrb_lat;
reg        target_in_lat;
reg [31:0] rdata_lat;
reg        mem_ready_lat;

wire req_in  = mem_valid_spi_in;
wire req_out = mem_valid_spi_out;
wire req_any = req_in | req_out;

// During busy:
//   Writes: step 0-3, present and write base_addr+step
//   Reads:  step 1-3 present base_addr+step, step 4-5 are capture-only
wire [7:0] bram_addr_busy = base_addr + {5'b0, step[2:0]};
wire [7:0] phase_wbyte = (step == 3'b000) ? wdata_lat[7:0] :
                         (step == 3'b001) ? wdata_lat[15:8] :
                         (step == 3'b010) ? wdata_lat[23:16] :
                                            wdata_lat[31:24];
wire phase_strb = wstrb_lat[step[1:0]];

reg        spi_in_cea;
reg        spi_in_wrea;
reg [7:0]  spi_in_ada;
reg [7:0]  spi_in_dina;
wire [7:0] spi_in_douta;
reg        spi_in_ceb;
reg        spi_in_wreb;
reg [7:0]  spi_in_adb;
reg [7:0]  spi_in_dinb;

reg        spi_out_cea;
reg        spi_out_wrea;
reg [7:0]  spi_out_ada;
reg [7:0]  spi_out_dina;
wire [7:0] spi_out_douta;
reg        spi_out_ceb;
reg [7:0]  spi_out_adb;

wire [7:0] spi_in_doutb;
wire [7:0] spi_out_doutb;

// SPI slave (mode 0): sample MOSI on rising edge, update MISO on falling edge.
// Use 3-stage synchronizers for CDC robustness:
//   stage0 catches metastability, stage1 is synchronized signal,
//   stage2 is previous stage1 value for edge detection.
reg [2:0] spi_clk_sync;
reg [2:0] spi_cs_sync;

reg       spi_active;
reg [2:0] spi_bit_cnt;
reg [7:0] spi_byte_cnt;
reg [2:0] rx_bit_cnt;
reg [7:0] rx_byte_cnt;
reg [7:0] rx_shift;
reg [7:0] tx_shift;
reg       spi_miso_reg;
reg [7:0] tx_first_byte;

reg       rx_write_pending;
reg [7:0] rx_write_addr;
reg [7:0] rx_write_data;

reg [2:0] spi_mosi_sync;

reg       tx_read_pending;
reg       tx_read_wait;
reg       tx_read_wait2;
reg [7:0] tx_read_addr;
reg [7:0] tx_next_byte;
reg       tx_next_valid;
reg       tx_load_first;

assign mem_ready = mem_ready_lat;
assign mem_rdata = rdata_lat;
assign spi_miso  = spi_miso_reg;

always @(*) begin
    spi_in_cea   = 1'b0;
    spi_in_wrea  = 1'b0;
    spi_in_ada   = busy ? bram_addr_busy : mem_addr[7:0];
    spi_in_dina  = phase_wbyte;

    spi_out_cea  = 1'b0;
    spi_out_wrea = 1'b0;
    spi_out_ada  = busy ? bram_addr_busy : mem_addr[7:0];
    spi_out_dina = phase_wbyte;

    spi_in_ceb   = 1'b0;
    spi_in_wreb  = 1'b0;
    spi_in_adb   = 8'b0;
    spi_in_dinb  = 8'b0;

    spi_out_ceb  = 1'b0;
    spi_out_adb  = 8'b0;

    // Memory-side Port A access
    if (busy) begin
        // For reads (2-cycle latency): enable BRAM only for steps 1-3.
        // Steps 4-5 are capture-only cycles.
        // For writes: enable BRAM for steps 0-3
        if (!op_write && (step == 3'd4 || step == 3'd5)) begin
            // Capture-only read steps: don't present new address
        end else begin
            if (target_in_lat) begin
                spi_in_cea  = 1'b1;
                spi_in_wrea = op_write && phase_strb;
            end else begin
                spi_out_cea  = 1'b1;
                spi_out_wrea = op_write && phase_strb;
            end
        end
    end else if (req_any && !(|mem_wstrb)) begin
        // For reads: pre-fetch byte 0 while still in !busy state
        if (req_in) begin
            spi_in_cea  = 1'b1;
        end else begin
            spi_out_cea = 1'b1;
        end
    end

    // SPI-side BRAM Port B access in clk domain.
    if (tx_read_pending && spi_active) begin
        spi_out_ceb = 1'b1;
        spi_out_adb = tx_read_addr;
    end

    // Do not gate RX writes with spi_active: byte 0 can arrive before the
    // clk-domain CS synchronizer asserts spi_active.
    if (rx_write_pending) begin
        spi_in_ceb  = 1'b1;
        spi_in_wreb = 1'b1;
        spi_in_adb  = rx_write_addr;
        spi_in_dinb = rx_write_data;
    end
end

always @(posedge clk) begin
    if (!reset_n) begin
        busy           <= 1'b0;
        op_write       <= 1'b0;
        step           <= 3'b000;
        base_addr      <= 8'b0;
        wdata_lat      <= 32'b0;
        wstrb_lat      <= 4'b0;
        target_in_lat  <= 1'b0;
        rdata_lat      <= 32'b0;
        mem_ready_lat  <= 1'b0;
        spi_clk_sync   <= 3'b000;
        spi_cs_sync    <= 3'b111;
        spi_mosi_sync  <= 3'b000;
        spi_active     <= 1'b0;
        spi_bit_cnt    <= 3'b000;
        spi_byte_cnt   <= 8'b0;
        rx_bit_cnt     <= 3'b000;
        rx_byte_cnt    <= 8'b0;
        rx_shift       <= 8'b0;
        tx_shift       <= 8'b0;
        spi_miso_reg   <= 1'b0;
        tx_first_byte  <= 8'b0;
        rx_write_pending <= 1'b0;
        rx_write_addr    <= 8'b0;
        rx_write_data    <= 8'b0;
        tx_read_pending <= 1'b0;
        tx_read_wait    <= 1'b0;
        tx_read_wait2   <= 1'b0;
        tx_read_addr    <= 8'b0;
        tx_next_byte    <= 8'b0;
        tx_next_valid   <= 1'b0;
        tx_load_first   <= 1'b0;
    end else begin
        mem_ready_lat <= 1'b0;

        // BRAM Port B write pulse is one clk wide.
        if (rx_write_pending)
            rx_write_pending <= 1'b0;

        // Keep RX state aligned to synchronized CS and ignore SCLK activity
        // while CS is inactive-high.
        if (spi_cs_sync[2:1] == 2'b10 || spi_cs_sync[2:1] == 2'b01) begin
            rx_bit_cnt  <= 3'b000;
            rx_byte_cnt <= 8'b0;
            rx_shift    <= 8'b0;
        end else if (!spi_cs_sync[1] && spi_clk_sync[2:1] == 2'b01) begin
            rx_shift <= {rx_shift[6:0], spi_mosi_sync[1]};
            if (rx_bit_cnt == 3'b111) begin
                rx_bit_cnt       <= 3'b000;
                rx_write_pending <= 1'b1;
                rx_write_addr    <= rx_byte_cnt;
                rx_write_data    <= {rx_shift[6:0], spi_mosi_sync[1]};
                if (rx_byte_cnt != 8'hFF)
                    rx_byte_cnt <= rx_byte_cnt + 8'h01;
            end else begin
                rx_bit_cnt <= rx_bit_cnt + 3'b001;
            end
        end

        // Memory interface state machine
        if (!busy) begin
            if (req_any) begin
                busy          <= 1'b1;
                op_write      <= |mem_wstrb;
                // Writes: step starts at 0 (present+write byte 0 this cycle)
                // Reads:  step starts at 1 (prefetch at !busy already presented addr+0)
                step          <= (|mem_wstrb) ? 3'd0 : 3'd1;
                base_addr     <= mem_addr[7:0];
                wdata_lat     <= mem_wdata;
                wstrb_lat     <= mem_wstrb;
                target_in_lat <= req_in;
            end
        end else begin
            step <= step + 3'd1;

            // Keep a local copy of SPI_out byte 0 so a new SPI transaction can
            // start transmitting immediately without waiting for a BRAM read.
            if (op_write && !target_in_lat && phase_strb && (bram_addr_busy == 8'h00)) begin
                tx_first_byte <= phase_wbyte;
            end

            // Capture read data from PREVIOUS address (READ_MODE0=1 => 2-cycle latency)
            // prefetch addr+0 -> capture at step 2
            // step 1 addr+1    -> capture at step 3
            // step 2 addr+2    -> capture at step 4
            // step 3 addr+3    -> capture at step 5
            if (!op_write) begin
                case (step)
                    3'd2: rdata_lat[7:0]   <= target_in_lat ? spi_in_douta : spi_out_douta;
                    3'd3: rdata_lat[15:8]  <= target_in_lat ? spi_in_douta : spi_out_douta;
                    3'd4: rdata_lat[23:16] <= target_in_lat ? spi_in_douta : spi_out_douta;
                    3'd5: rdata_lat[31:24] <= target_in_lat ? spi_in_douta : spi_out_douta;
                endcase
            end

            // Complete: writes at step 3, reads at step 5 (all 4 bytes captured)
            if ((op_write && step == 3'd3) || (!op_write && step == 3'd5)) begin
                busy          <= 1'b0;
                mem_ready_lat <= 1'b1;
                step          <= 3'd0;
            end
        end

        // Synchronize SPI pins into clk domain.
        spi_clk_sync  <= {spi_clk_sync[1:0], spi_clk};
        spi_cs_sync   <= {spi_cs_sync[1:0], spi_cs};
        spi_mosi_sync <= {spi_mosi_sync[1:0], spi_mosi};

        // SPI_out Port B uses registered output, so capture data two cycles
        // after the read strobe has been issued.
        if (tx_read_wait2) begin
            tx_next_byte  <= spi_out_doutb;
            tx_next_valid <= 1'b1;
        end

        if (tx_read_pending) begin
            tx_read_pending <= 1'b0;
            tx_read_wait <= 1'b1;
            tx_read_wait2 <= 1'b0;
        end else if (tx_read_wait) begin
            tx_read_wait <= 1'b0;
            tx_read_wait2 <= 1'b1;
        end else begin
            tx_read_wait <= 1'b0;
            tx_read_wait2 <= 1'b0;
        end

        // Start a new 256-byte transaction on CS falling edge.
        if (spi_cs_sync[2:1] == 2'b10) begin
            spi_active      <= 1'b1;
            spi_bit_cnt     <= 3'b000;
            spi_byte_cnt    <= 8'b0;
            tx_shift        <= tx_first_byte;
            spi_miso_reg    <= tx_first_byte[7];
            tx_read_addr    <= 8'h01;
            tx_read_pending <= 1'b1;
            tx_read_wait    <= 1'b0;
            tx_read_wait2   <= 1'b0;
            tx_next_valid   <= 1'b0;
            tx_load_first   <= 1'b0;
        end

        // End transaction on CS rising edge.
        if (spi_cs_sync[2:1] == 2'b01) begin
            spi_active      <= 1'b0;
            tx_read_pending <= 1'b0;
            tx_read_wait    <= 1'b0;
            tx_read_wait2   <= 1'b0;
            tx_next_valid   <= 1'b0;
            tx_load_first   <= 1'b0;
            spi_bit_cnt     <= 3'b000;
            spi_byte_cnt    <= 8'b0;
        end

        if (spi_active) begin
            // Arm the first transmit byte as soon as the first BRAM read returns.
            if (tx_load_first && tx_next_valid) begin
                tx_shift      <= tx_next_byte;
                spi_miso_reg  <= tx_next_byte[7];
                tx_next_valid <= 1'b0;
                tx_load_first <= 1'b0;
                tx_read_addr  <= 8'h01;
                tx_read_pending <= 1'b1;
            end

            // Detect SPI clock edges after synchronization.
            if (spi_clk_sync[2:1] == 2'b01) begin
                if (spi_bit_cnt == 3'b111) begin
                    spi_bit_cnt <= 3'b000;

                    if (spi_byte_cnt != 8'hFF) begin
                        spi_byte_cnt <= spi_byte_cnt + 8'h01;

                        // Load byte for next transfer chunk.
                        if (tx_next_valid) begin
                            tx_shift      <= tx_next_byte;
                            spi_miso_reg  <= tx_next_byte[7];
                            tx_next_valid <= 1'b0;
                        end

                        // Request following byte from SPI_out memory.
                        if (spi_byte_cnt < 8'hFE) begin
                            tx_read_addr    <= spi_byte_cnt + 8'h02;
                            tx_read_pending <= 1'b1;
                        end
                    end
                end else begin
                    spi_bit_cnt <= spi_bit_cnt + 3'b001;
                end
            end else if (spi_clk_sync[2:1] == 2'b10) begin
                // Falling edge: shift data to present next MISO bit.
                if (!tx_load_first) begin
                    tx_shift <= {tx_shift[6:0], 1'b0};
                    spi_miso_reg <= tx_shift[6];
                end
            end
        end
    end
end

SPI_in spi_in(
    .douta(spi_in_douta),
    .doutb(spi_in_doutb),
    .clka(clk),
    .ocea(1'b1),
    .cea(spi_in_cea),
    .reseta(1'b0),
    .wrea(spi_in_wrea),
    .clkb(clk),
    .oceb(1'b1),
    .ceb(spi_in_ceb),
    .resetb(1'b0),
    .wreb(spi_in_wreb),
    .ada(spi_in_ada),
    .dina(spi_in_dina),
    .adb(spi_in_adb),
    .dinb(spi_in_dinb)
);

SPI_out spi_out(
    .douta(spi_out_douta),
    .doutb(spi_out_doutb),
    .clka(clk),
    .ocea(1'b1),
    .cea(spi_out_cea),
    .reseta(1'b0),
    .wrea(spi_out_wrea),
    .clkb(clk),
    .oceb(1'b1),
    .ceb(spi_out_ceb),
    .resetb(1'b0),
    .wreb(1'b0),
    .ada(spi_out_ada),
    .dina(spi_out_dina),
    .adb(spi_out_adb),
    .dinb(8'b0)
);


endmodule
