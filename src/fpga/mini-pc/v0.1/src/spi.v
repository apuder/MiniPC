
module spi(
    input              clk_in,       // 27 MHz reference clock for SPI engine PLL
    input              clk,         // System clock
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
    output wire [31:0] mem_rdata,
    output wire [3:0]  leds
);

wire clk_spi_fast;
Gowin_rPLL1 spi_fast_pll(
    .clkout(clk_spi_fast),
    .clkin(clk_in)
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

reg        spi_out_cea;
reg        spi_out_wrea;
reg [7:0]  spi_out_ada;
reg [7:0]  spi_out_dina;
wire [7:0] spi_out_douta;

wire [7:0] spi_in_doutb;
wire [7:0] spi_out_doutb;

// ---------------------------------------------------------------------------
// SPI engine runs entirely in the clk_spi_fast domain.
// 3-stage synchronizers bring the raw pins into that domain.
// Edge detectors look at stages [2:1].
// ---------------------------------------------------------------------------
reg [2:0] sck_sync;    // clk_spi_fast domain SCK synchronizer
reg [2:0] cs_sync;     // clk_spi_fast domain CS  synchronizer
reg [2:0] mosi_sync;   // clk_spi_fast domain MOSI synchronizer

wire sck_rise = (sck_sync[2:1] == 2'b01);
wire sck_fall = (sck_sync[2:1] == 2'b10);
wire cs_fall  = (cs_sync[2:1]  == 2'b10);  // CS active-low: 1->0 = start
wire cs_rise  = (cs_sync[2:1]  == 2'b01);  // CS active-low: 0->1 = end
wire cs_active = !cs_sync[1];

// SPI FSM state
reg       spi_dummy;           // 1 while receiving the dummy byte
reg       dummy_done_pending;  // set at rise 8 (end of dummy); triggers MISO reload at fall 8
reg [2:0] spi_bit_cnt;         // bit counter within current byte (0-7)
reg [7:0] spi_byte_cnt;        // byte counter within payload (0-255)
reg [7:0] rx_shift;            // MOSI shift register
reg [7:0] tx_shift;            // MISO shift register
reg       spi_miso_reg;        // registered MISO output

// RX byte-write handshake to BRAM Port B (clk_spi_fast drives BRAM-B directly)
reg       rx_write_en;     // one-cycle write strobe into spi_in BRAM
reg [7:0] rx_write_addr;
reg [7:0] rx_write_data;

// TX pre-fetch: clk_spi_fast reads spi_out BRAM Port B directly.
// tx_read_req is a one-cycle strobe; the BRAM has registered outputs so
// the data appears 2 clk_spi_fast cycles later (READ_MODE1 => registered).
reg       tx_read_req;     // strobe: present tx_read_addr to BRAM-B
reg [7:0] tx_read_addr;
reg [1:0] tx_read_dly;    // 2-cycle pipeline to capture registered BRAM output
reg [7:0] tx_next_byte;    // buffered next TX byte
reg       tx_next_valid;   // tx_next_byte is valid

assign mem_ready = mem_ready_lat;
assign mem_rdata = rdata_lat;
assign spi_miso  = cs_sync[1] ? 1'bz : spi_miso_reg;  // tri-state when CS inactive

always @(*) begin
    spi_in_cea   = 1'b0;
    spi_in_wrea  = 1'b0;
    spi_in_ada   = busy ? bram_addr_busy : mem_addr[7:0];
    spi_in_dina  = phase_wbyte;

    spi_out_cea  = 1'b0;
    spi_out_wrea = 1'b0;
    spi_out_ada  = busy ? bram_addr_busy : mem_addr[7:0];
    spi_out_dina = phase_wbyte;

    // Memory-side Port A access (clk domain only)
    if (busy) begin
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
        if (req_in) begin
            spi_in_cea  = 1'b1;
        end else begin
            spi_out_cea = 1'b1;
        end
    end

    // NOTE: BRAM Port B (spi_in_ceb/wreb, spi_out_ceb) is driven from the
    // clk_spi_fast always block below via the registered signals
    // rx_write_en / tx_read_req.  The combinational block above only drives
    // Port A and must leave Port B signals at their defaults (0) here so
    // that the clk_spi_fast domain has exclusive control over Port B.
end

// ---------------------------------------------------------------------------
// clk domain: CPU memory-bus state machine only.
// ---------------------------------------------------------------------------
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
    end else begin
        mem_ready_lat <= 1'b0;

        if (!busy) begin
            if (req_any) begin
                busy          <= 1'b1;
                op_write      <= |mem_wstrb;
                step          <= (|mem_wstrb) ? 3'd0 : 3'd1;
                base_addr     <= mem_addr[7:0];
                wdata_lat     <= mem_wdata;
                wstrb_lat     <= mem_wstrb;
                target_in_lat <= req_in;
            end
        end else begin
            step <= step + 3'd1;

            if (!op_write) begin
                case (step)
                    3'd2: rdata_lat[7:0]   <= target_in_lat ? spi_in_douta : spi_out_douta;
                    3'd3: rdata_lat[15:8]  <= target_in_lat ? spi_in_douta : spi_out_douta;
                    3'd4: rdata_lat[23:16] <= target_in_lat ? spi_in_douta : spi_out_douta;
                    3'd5: rdata_lat[31:24] <= target_in_lat ? spi_in_douta : spi_out_douta;
                endcase
            end

            if ((op_write && step == 3'd3) || (!op_write && step == 3'd5)) begin
                busy          <= 1'b0;
                mem_ready_lat <= 1'b1;
                step          <= 3'd0;
            end
        end
    end
end

// ---------------------------------------------------------------------------
// clk_spi_fast domain: synchronizers, SPI FSM, MISO/MOSI logic.
//
// Protocol:
//   - CS falls  -> enter DUMMY phase (spi_dummy=1) and begin a BRAM read of
//                  spi_out[0].
//   - DUMMY phase lasts exactly 8 SCK cycles (one byte).  MOSI is ignored
//     and rx counters are not advanced.
//   - After dummy byte completes -> on the next falling edge, load the BRAM
//     result for spi_out[0] into tx_shift so the first payload sample sees
//     the correct MSB. At that same moment, request spi_out[1].
//   - From there MOSI is captured to spi_in and MISO shifts spi_out bytes.
//   - CS rises  -> end transaction, tri-state MISO.
//
// BRAM Port B clock is clk_spi_fast; registered output mode means data
// appears 2 clk_spi_fast cycles after the read strobe.
// ---------------------------------------------------------------------------
always @(posedge clk_spi_fast or negedge reset_n) begin
    if (!reset_n) begin
        sck_sync      <= 3'b000;
        cs_sync       <= 3'b111;  // CS inactive-high at reset
        mosi_sync     <= 3'b000;
        spi_dummy          <= 1'b0;
        dummy_done_pending <= 1'b0;
        spi_bit_cnt        <= 3'b000;
        spi_byte_cnt       <= 8'b0;
        rx_shift           <= 8'b0;
        tx_shift           <= 8'b0;
        spi_miso_reg       <= 1'b0;
        rx_write_en        <= 1'b0;
        rx_write_addr <= 8'b0;
        rx_write_data <= 8'b0;
        tx_read_req   <= 1'b0;
        tx_read_addr  <= 8'b0;
        tx_read_dly   <= 2'b00;
        tx_next_byte  <= 8'b0;
        tx_next_valid <= 1'b0;
    end else begin
        // Advance synchronizers
        sck_sync  <= {sck_sync[1:0],  spi_clk};
        cs_sync   <= {cs_sync[1:0],   spi_cs};
        mosi_sync <= {mosi_sync[1:0], spi_mosi};

        // Clear one-cycle strobes
        rx_write_en <= 1'b0;
        tx_read_req <= 1'b0;

        // BRAM registered-output pipeline: data valid 2 cycles after read strobe
        tx_read_dly <= {tx_read_dly[0], tx_read_req};
        if (tx_read_dly[1]) begin
            tx_next_byte  <= spi_out_doutb;
            tx_next_valid <= 1'b1;
        end

        // ---- CS FALL: start of new transaction --------------------------------
        if (cs_fall) begin
            spi_dummy          <= 1'b1;    // enter dummy-byte phase
            dummy_done_pending <= 1'b0;
            spi_bit_cnt        <= 3'b000;
            spi_byte_cnt       <= 8'b0;
            rx_shift           <= 8'b0;
            tx_shift           <= 8'b0;
            spi_miso_reg       <= 1'b0;
            tx_next_valid      <= 1'b0;
            // Pre-fetch spi_out[0] during dummy phase so that the first real
            // payload byte comes from BRAM, not from a clk-domain shadow copy.
            tx_read_addr  <= 8'h00;
            tx_read_req   <= 1'b1;
        end

        // ---- CS RISE: end of transaction --------------------------------------
        if (cs_rise) begin
            spi_dummy          <= 1'b0;
            dummy_done_pending <= 1'b0;
            spi_bit_cnt        <= 3'b000;
            spi_byte_cnt       <= 8'b0;
            tx_next_valid      <= 1'b0;
        end

        // ---- SCK RISING EDGE: sample MOSI, advance RX -------------------------
        if (sck_rise && cs_active) begin
            rx_shift <= {rx_shift[6:0], mosi_sync[1]};

            if (spi_bit_cnt == 3'b111) begin
                // Byte complete
                spi_bit_cnt <= 3'b000;

                if (spi_dummy) begin
                    // Dummy byte done -> switch to payload.
                    // Set flag so the very next SCK fall reloads tx_shift
                    // for the first real payload byte instead of shifting.
                    spi_dummy          <= 1'b0;
                    dummy_done_pending <= 1'b1;
                    spi_byte_cnt       <= 8'b0;
                    rx_shift           <= 8'b0;
                end else begin
                    // Payload byte complete: write to spi_in BRAM
                    rx_write_en   <= 1'b1;
                    rx_write_addr <= spi_byte_cnt;
                    rx_write_data <= {rx_shift[6:0], mosi_sync[1]};
                    if (spi_byte_cnt != 8'hFF)
                        spi_byte_cnt <= spi_byte_cnt + 8'h01;

                    // Load next TX byte if available
                    if (tx_next_valid) begin
                        tx_shift      <= tx_next_byte;
                        spi_miso_reg  <= tx_next_byte[7];
                        tx_next_valid <= 1'b0;
                    end

                    // Pre-fetch the byte after next from spi_out
                    // (spi_byte_cnt still holds current byte index here)
                    if (spi_byte_cnt < 8'hFE) begin
                        tx_read_addr <= spi_byte_cnt + 8'h02;
                        tx_read_req  <= 1'b1;
                    end
                end
            end else begin
                spi_bit_cnt <= spi_bit_cnt + 3'b001;
            end
        end

        // ---- SCK FALLING EDGE: update MISO -----------------------------------
        if (sck_fall && cs_active) begin
            if (dummy_done_pending) begin
                // Fall 8: the transition fall between dummy and payload.
                // Load spi_out[0] that was fetched from BRAM during the dummy
                // byte so rise 9 sees the correct MSB of byte 0.
                tx_shift           <= tx_next_byte;
                spi_miso_reg       <= tx_next_byte[7];
                tx_next_valid      <= 1'b0;
                dummy_done_pending <= 1'b0;

                // Start fetching spi_out[1] now that byte 0 has been loaded.
                tx_read_addr       <= 8'h01;
                tx_read_req        <= 1'b1;
            end else begin
                tx_shift     <= {tx_shift[6:0], 1'b0};
                spi_miso_reg <= tx_shift[6];
            end
        end
    end
end

// BRAM Port A: clk domain (CPU bus).  Port B: clk_spi_fast domain (SPI engine).
SPI_in spi_in(
    .douta(spi_in_douta),
    .doutb(spi_in_doutb),
    .clka(clk),
    .ocea(1'b1),
    .cea(spi_in_cea),
    .reseta(1'b0),
    .wrea(spi_in_wrea),
    .clkb(clk_spi_fast),
    .oceb(1'b1),
    .ceb(rx_write_en),
    .resetb(1'b0),
    .wreb(rx_write_en),
    .ada(spi_in_ada),
    .dina(spi_in_dina),
    .adb(rx_write_addr),
    .dinb(rx_write_data)
);

SPI_out spi_out(
    .douta(spi_out_douta),
    .doutb(spi_out_doutb),
    .clka(clk),
    .ocea(1'b1),
    .cea(spi_out_cea),
    .reseta(1'b0),
    .wrea(spi_out_wrea),
    .clkb(clk_spi_fast),
    .oceb(1'b1),
    .ceb(tx_read_req),
    .resetb(1'b0),
    .wreb(1'b0),
    .ada(spi_out_ada),
    .dina(spi_out_dina),
    .adb(tx_read_addr),
    .dinb(8'b0)
);


endmodule
