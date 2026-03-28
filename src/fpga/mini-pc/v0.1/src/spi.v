
module spi(
    input              clk_in,       // 27 MHz reference clock for SPI engine PLL
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
    output wire [31:0] mem_rdata,
    output wire [3:0]  leds
);

//------------------------------------------------------------------------------------
//-----SPI Interface------------------------------------------------------------------
//------------------------------------------------------------------------------------

wire spi_fast_clk;

Gowin_rPLL1 spi_fast_rpll(
    .clkout(spi_fast_clk), // 81 MHz
    .clkin(clk_in)         // 27 MHz
);


reg[2:0] clk_raw = 3'b000;
reg[1:0] cs_raw = 2'b00;
reg[1:0] mosi_raw = 2'b00;

always @(posedge spi_fast_clk) begin
    clk_raw <= {clk_raw[1:0], spi_clk};
    cs_raw <= {cs_raw[0], spi_cs};
    mosi_raw <= {mosi_raw[0], spi_mosi};
end

assign cs_active = (cs_raw[1:0] == 2'b00);
assign clk_falling_edge = (clk_raw[2:1] == 2'b10);
assign clk_rising_edge = (clk_raw[2:1] == 2'b01);

reg[7:0]  tx_byte;
wire[7:0] tx_byte_buffer;
reg[7:0]  tx_byte_index;
reg[2:0]  tx_bit_index;
reg       tx_trigger_read;

reg[7:0] rx_byte;
reg[7:0] rx_byte_buffer;
reg      rx_trigger_write;
reg[7:0] rx_byte_index;
reg[2:0] rx_bit_index;

assign spi_miso = cs_active ? tx_byte[7] : 1'bz; // Tri-state when not active, otherwise output MSB of tx_byte

always @(posedge spi_fast_clk) begin
    if (!cs_active) begin
        tx_bit_index <= 3'b000;
        tx_byte_index <= 8'b0;
        tx_trigger_read <= 1'b0;
        rx_bit_index <= 3'b000;
        rx_byte_index <= 8'hff;  // Store first dummy byte at address 0xff so that it gets effectively ignored
        rx_trigger_write <= 1'b0;
    end
    else begin
        if (clk_falling_edge) begin
          // Shift out the next bit on MOSI
          tx_byte <= {tx_byte[6:0], 1'b0};
          tx_trigger_read <= (tx_bit_index == 3'b101) ? 1'b1 : 1'b0;
          if (tx_bit_index == 3'b111) begin
              tx_byte <= tx_byte_buffer; // Load the next byte to shift out
              tx_byte_index <= tx_byte_index + 8'b1; // Increment byte index for next read
          end
          tx_bit_index <= tx_bit_index + 3'b001;
        end
        if (clk_rising_edge) begin
          // Shift in the next bit from MISO
          rx_byte <= {rx_byte[6:0], mosi_raw[1]};
          if (rx_bit_index == 3'b111) begin
              rx_byte_buffer <= {rx_byte[6:0], mosi_raw[1]};
              rx_trigger_write <= 1'b1;
          end
          rx_bit_index <= rx_bit_index + 3'b001;
        end
    end
    if (rx_trigger_write) begin
        rx_byte_index <= rx_byte_index + 8'b1;
        rx_trigger_write <= 1'b0;
    end
end

//------------------------------------------------------------------------------------
//-----RISV Interface-----------------------------------------------------------------
//------------------------------------------------------------------------------------
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

assign mem_ready = mem_ready_lat;
assign mem_rdata = rdata_lat;

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

// BRAM Port A: clk domain (CPU bus).  Port B: tied off (no SPI engine).
// spi_in captures data from MOSI
SPI_in spi_in(
    .clka(clk),
    .ocea(1'b1),
    .cea(spi_in_cea),
    .reseta(1'b0),
    .wrea(spi_in_wrea),
    .ada(spi_in_ada),
    .dina(spi_in_dina),
    .douta(spi_in_douta),

    .clkb(spi_fast_clk),
    .oceb(1'b1),
    .ceb(rx_trigger_write),
    .resetb(1'b0),
    .wreb(1'b1),
    .adb(rx_byte_index),
    .dinb(rx_byte_buffer),
    .doutb()
);

// spi_out holds data to be sent out on MISO
SPI_out spi_out(
    .clka(clk),
    .ocea(1'b1),
    .cea(spi_out_cea),
    .reseta(1'b0),
    .wrea(spi_out_wrea),
    .ada(spi_out_ada),
    .dina(spi_out_dina),
    .douta(spi_out_douta),

    .clkb(spi_fast_clk),
    .oceb(1'b1),
    .ceb(tx_trigger_read),
    .resetb(1'b0),
    .wreb(1'b0),
    .adb(tx_byte_index),
    .dinb(8'b0),
    .doutb(tx_byte_buffer)
 );


endmodule
