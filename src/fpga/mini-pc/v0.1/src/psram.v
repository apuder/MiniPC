// APS6404L QSPI PSRAM controller adapted for picorv32 "mem" interface.
// - One outstanding request at a time (picorv32 stalls via mem_ready).
// - Reads return 32-bit little-endian word assembled from 4 sequential bytes.
// - Writes honor mem_wstrb. For partial writes, performs per-byte write transactions
//   (simple + correct, slower). For full-word writes, does a single 4-byte write.
// - Performs POR delay, RSTEN(0x66)+RST(0x99), then Enter Quad Mode (0x35).
//
// Notes:
// - APS6404L is 8MB => valid address bits A[22:0] (byte addressing).
// - This module expects you to hook it to a 3.3V I/O bank and route CE#, SCLK, SIO[3:0].
// - Start with a low SCLK (e.g., 5 MHz) for bring-up, then increase.
// - RD_WAIT_CLKS may need tuning depending on your mode/speed.
//
// Integration with picorv32:
//   wire mem_valid, mem_instr;
//   wire [31:0] mem_addr, mem_wdata;
//   wire [3:0]  mem_wstrb;
//   wire mem_ready;
//   wire [31:0] mem_rdata;
// Connect those directly.
//
// Address mapping:
// - If you want PSRAM at base 0x4000_0000, set PSRAM_BASE and decode inside.
// - Or set PSRAM_BASE=0 and PSRAM_SIZE=0x0080_0000 and feed already-decoded addr.

module aps6404l_picorv32 #
(
    parameter int unsigned SYS_HZ       = 84_000_000,
    parameter int unsigned SCLK_INIT_HZ = 4_500_000,
    parameter int unsigned SCLK_RUN_HZ  = 9_000_000,
    parameter int unsigned RD_WAIT_CLKS = 6,              // dummy "quad-nibble" cycles after addr for 0xEB
    parameter logic [31:0] PSRAM_BASE   = 32'h4000_0000,
    parameter logic [31:0] PSRAM_SIZE   = 32'h0080_0000   // 8MB
)
(
    input  wire        clk,
    input  wire        rst,
    output wire        status,
    output reg         psram_chip_present,

    // picorv32 memory interface
    input  wire        mem_valid,
    input  wire        mem_instr,      // unused here, but you may use it for I-cache decisions
    input  wire [31:0] mem_addr,       // byte address
    input  wire [31:0] mem_wdata,
    input  wire [3:0]  mem_wstrb,      // byte enables; 0 => read

    output reg        mem_ready,
    output reg [31:0] mem_rdata,

    // QSPI pins
    output reg        ps_ce_n,
    output wire        ps_sclk,
    inout  wire [3:0]  ps_sio
);

    // --------------------------
    // Simple address decode
    // --------------------------
    wire psram_sel = mem_valid;
    wire [22:0] psram_addr = (mem_addr - PSRAM_BASE); // byte address within 8MB

reg led = 1'b0;
assign status = led;

/*
always @(posedge clk) begin
  if (psram_sel && !mem_ready) begin
    led <= ~led; // indicate PSRAM access
  end
end
*/

/*
always @(posedge clk) begin
    if (psram_sel && mem_valid) begin
        led <= ~led; // indicate PSRAM access
    end
end
*/

    // --------------------------
    // SCLK generator (SPI mode 0: CPOL=0, CPHA=0)
    // - Use low speed during reset/ID/quad-enable sequencing.
    // - Switch to run speed only after successful init and QPI entry.
    // --------------------------
    localparam int unsigned DIV_INIT_RAW = (SCLK_INIT_HZ == 0) ? 1 : (SYS_HZ/(2*SCLK_INIT_HZ));
    localparam int unsigned DIV_RUN_RAW  = (SCLK_RUN_HZ  == 0) ? 1 : (SYS_HZ/(2*SCLK_RUN_HZ));
    localparam int unsigned DIV_INIT     = (DIV_INIT_RAW < 1) ? 1 : DIV_INIT_RAW;
    localparam int unsigned DIV_RUN      = (DIV_RUN_RAW  < 1) ? 1 : DIV_RUN_RAW;
    localparam int unsigned DIV_MAX      = (DIV_INIT > DIV_RUN) ? DIV_INIT : DIV_RUN;
    localparam int unsigned DIV_CNT_W    = $clog2((DIV_MAX < 2) ? 2 : DIV_MAX);

    logic [DIV_CNT_W-1:0] divcnt;
    logic [DIV_CNT_W-1:0] div_target;
    logic sclk_en;
    logic sclk_int;
    logic sclk_fast_mode;

    always_comb begin
        div_target = sclk_fast_mode ? DIV_RUN[DIV_CNT_W-1:0] : DIV_INIT[DIV_CNT_W-1:0];
    end

    always_ff @(posedge clk) begin
        if (rst) begin
            divcnt   <= '0;
            sclk_int <= 1'b0;
        end else if (sclk_en && !sh_tx_req && !sh_rx_req) begin
            if (divcnt == (div_target - 1'b1)) begin
                divcnt   <= '0;
                sclk_int <= ~sclk_int;
            end else begin
                divcnt <= divcnt + 1;
            end
        end else begin
            divcnt   <= '0;
            sclk_int <= 1'b0;
        end
    end
    assign ps_sclk = sclk_int;

    // Edge-detect SCLK in clk domain
    logic sclk_d;
    always_ff @(posedge clk) sclk_d <= ps_sclk;
    wire sclk_fall  = ( sclk_d && !ps_sclk);
    wire sclk_rise  = (!sclk_d &&  ps_sclk);

    // --------------------------
    // Bidirectional IO handling
    // --------------------------
    // Shifter-driven and FSM-driven IO controls are kept separate and then muxed
    // to avoid multiple procedural drivers on the same net.
    logic [3:0] sio_out_sh, sio_oe_sh;
    logic [3:0] sio_out_fsm, sio_oe_fsm;
    logic [3:0] sio_out,    sio_oe;
    wire  [3:0] sio_in = ps_sio;

    // --------------------------
    // Byte shifter for x1 and x4
    //   - For x1: drive SIO0 on falling edges, sample on rising edges
    //   - For x4: drive 4 bits per cycle on falling, sample 4 bits per rising
    // --------------------------
    typedef enum logic {W_X1, W_X4} width_t;

    width_t width;
    logic [7:0] sh_byte;
    logic [7:0] sh_byte_next;  // pre-computed next byte value for pipelined timing
    logic [3:0] rem;        // remaining bits (x1) or nibbles*4 (x4) encoded as 8 or 2 steps
    logic       sh_active;
    logic       sh_done_pulse;

    // Kick pulses from FSM to the shifter (one-cycle pulses, latched here)
    logic       sh_tx_req, sh_rx_req;
    logic [7:0] sh_req_byte;
    width_t     sh_req_width;

    // For x4 receive helper
    logic [7:0] rx_byte;
    logic       rx_active;
    logic       rx_done_pulse;
    logic       rx_done_pending; // Stage rx_done_pulse by one cycle to let rx_byte update
    logic       rx_nib_hi; // 0 => expecting high nibble next, 1 => expecting low nibble next

    // Shifter activity takes priority over FSM defaults.
    assign sio_out = (sh_active || rx_active) ? sio_out_sh : sio_out_fsm;
    assign sio_oe  = (sh_active || rx_active) ? sio_oe_sh  : sio_oe_fsm;
    genvar i;
    generate
      for (i=0; i<4; i=i+1) begin : SIO_TRI
        assign ps_sio[i] = sio_oe[i] ? sio_out[i] : 1'bz;
      end
    endgenerate
    //assign ps_sio  = (sio_oe != 4'b0000) ? sio_out : 4'bz;

    task automatic sh_start_tx(input logic [7:0] b, input width_t w);
        begin
            sh_req_byte  <= b;
            sh_req_width <= w;
            sh_tx_req    <= 1'b1;
        end
    endtask

    task automatic sh_start_rx(input width_t w);
        begin
            sh_req_width  <= w;
            sh_rx_req     <= 1'b1;
        end
    endtask

    always_ff @(posedge clk) begin

        sh_done_pulse <= 1'b0;
        rx_done_pulse <= rx_done_pending;  // Stage the pulse
        rx_done_pending <= 1'b0;

        if (rst) begin
            sio_out_sh<= 4'h0;
            sio_oe_sh <= 4'h0;
            sh_active <= 1'b0;
            rx_active <= 1'b0;
            sh_byte   <= 8'h00;
            sh_byte_next <= 8'h00;
            rx_byte   <= 8'h00;
            rem       <= 4'd0;
            width     <= W_X1;
            rx_nib_hi <= 1'b0;
            rx_done_pending <= 1'b0;
        end else begin
            // Latch new TX/RX start requests from FSM
            if (sh_tx_req) begin
                sh_active     <= 1'b1;
                sh_byte       <= sh_req_byte;
                width         <= sh_req_width;
                rem           <= (sh_req_width == W_X1) ? 4'd8 : 4'd2;
                sh_done_pulse <= 1'b0;

                // CPOL=0, CPHA=0: first bit/nibble must be valid before the first SCLK rising edge.
                // Preload the first output immediately when the TX request is latched.
                if (sh_req_width == W_X1) begin
                    sio_oe_sh  <= 4'b0001;
                    sio_out_sh <= {3'b000, sh_req_byte[7]};
                    // Pre-compute next byte value for pipelined timing
                    sh_byte_next <= {sh_req_byte[6:0], 1'b0};
                end else begin
                    sio_oe_sh  <= 4'b1111;
                    sio_out_sh <= sh_req_byte[7:4];
                    // Pre-compute next byte value for pipelined timing
                    sh_byte_next <= {sh_req_byte[3:0], 4'b0000};
                end
            end else if (sh_rx_req) begin
                rx_active     <= 1'b1;
                width         <= sh_req_width;
                rx_done_pulse <= 1'b0;
                rx_nib_hi     <= 1'b0;
                rx_byte       <= 8'h00;
                rem           <= (sh_req_width == W_X1) ? 4'd8 : 4'd2;
            end
            // Transmit (mode 0): Pipeline the data updates for better timing
            if (sh_active) begin
                // Pre-compute next byte on rising edge (while SCLK is low, max time for settling)
                // This gives the output combinatorial logic plenty of time to stabilize before the next rising edge
                if (sclk_rise && rem > 1) begin
                    // Prepare the NEXT byte/nibble from the current sh_byte.
                    // Shifting from sh_byte_next would skip symbols and corrupt transfers.
                    if (width == W_X1) begin
                        sh_byte_next <= {sh_byte[6:0], 1'b0};
                    end else begin
                        sh_byte_next <= {sh_byte[3:0], 4'b0000};
                    end
                end

                // Decrement remaining count on rising edge
                if (sclk_rise && rem != 0) begin
                    rem <= rem - 1;
                end

                // After the last bit is sampled (rem == 0), wait for falling edge to ensure hold time,
                // then signal completion.
                if (sclk_fall && rem == 0) begin
                    sh_active     <= 1'b0;
                    sh_done_pulse <= 1'b1;
                end

                // Advance output data on falling edge using pre-computed value.
                // Skip the first falling edge since the first bit/nibble is already preloaded.
                // The data was computed during the previous rising edge, giving maximum settling time.
                if (sclk_fall && rem > 0 && rem < ((width == W_X1) ? 8 : 2)) begin
                    if (width == W_X1) begin
                        sio_oe_sh  <= 4'b0001;
                        sh_byte    <= sh_byte_next;
                        sio_out_sh <= {3'b000, sh_byte_next[7]};
                    end else begin
                        sio_oe_sh  <= 4'b1111;
                        sh_byte    <= sh_byte_next;
                        sio_out_sh <= sh_byte_next[7:4];
                    end
                end
            end

            // Receive (only meaningful in x4 here; but kept generic)
            if (rx_active) begin
                sio_oe_sh <= 4'b0000; // ensure input
                if (sclk_rise && rem != 0) begin
                    if (width == W_X1) begin
                        // SPI mode 0, MSB-first: shift left, append at LSB
                        rx_byte <= {rx_byte[6:0], sio_in[1]};
                    end else begin
                        if (!rx_nib_hi) begin
                            rx_byte[7:4] <= sio_in;
                            rx_nib_hi    <= 1'b1;
                        end else begin
                            rx_byte[3:0] <= sio_in;
                            rx_nib_hi    <= 1'b0;
                        end
                    end
                    rem <= rem - 1;
                    if (rem == 1) begin
                        rx_active     <= 1'b0;
                        rx_done_pending <= 1'b1;  // Stage the pulse; actual pulse goes out next cycle
                    end
                end
            end
        end
    end

    // --------------------------
    // Main FSM
    // --------------------------
    typedef enum logic [5:0] {
        ST_POR,
        ST_SPI_RSTEN_CMD, ST_SPI_RSTEN_WAIT, ST_RSTEN_GAP,
        ST_SPI_RST_CMD, ST_SPI_RST_WAIT,
        ST_QPI_RSTEN_CMD, ST_QPI_RSTEN_WAIT,
        ST_QPI_RST_CMD, ST_QPI_RST_WAIT,
        ST_QPI_POST_RST_WAIT,
        ST_QPI_EXIT_WAIT,
        ST_POST_RST_GAP,
        ST_ENTER_QUAD_CMD, ST_ENTER_QUAD_WAIT, ST_ENTER_QUAD,
        ST_READID_CMD, ST_READID_WAIT, ST_READID_A2, ST_READID_A1, ST_READID_A0, ST_READID_RX_DELAY, ST_READID_MFG, ST_READID_KGD,
        ST_IDLE,
        ST_LATCH_REQ,

        // Common header: cmd + addr[23:0]
        ST_CMD, ST_CMD_WAIT, ST_A2, ST_A1, ST_A0,

        // Read path
        ST_RD_DUMMY,
        ST_RD_B0, ST_RD_B1, ST_RD_B2, ST_RD_B3, ST_RD_TERM,

        // Write path (byte or word)
        ST_WR_BYTE,       // single-byte write
        ST_WR_W0, ST_WR_W1, ST_WR_W2, ST_WR_W3,

        ST_NEXT_BYTE,
        ST_PARTIAL_GAP,
        ST_FINISH
    } state_t;

    state_t st;

    // power-on delay (~200us default)
    localparam int unsigned POR_TICKS = (SYS_HZ/5000);
    logic [$clog2(POR_TICKS+1)-1:0] por_cnt;

    // reset recovery delay after RST command sequence (~100us default)
    localparam int unsigned RESET_WAIT_TICKS = (SYS_HZ/10000);
    logic [$clog2(RESET_WAIT_TICKS+1)-1:0] reset_wait_cnt;

    // latched CPU request
    logic [22:0] addr_l;
    logic [22:0] base_addr; // preserve base address for partial writes
    logic [31:0] wdata_l;
    logic [3:0]  wstrb_l;
    logic [3:0]  wstrb_orig; // preserve original strobes for partial writes
    logic        we_l;

    // read assembly
    logic [31:0] rdata_l;

    // for partial writes: iterate lane 0..3
    logic [1:0] lane;

    // dummy counter for 0xEB
    logic [$clog2(RD_WAIT_CLKS+1)-1:0] dummy_cnt;
    // Read termination helper: after the last received data nibble/byte, keep CE# low
    // for at least one additional full SCLK cycle before deasserting CE#.
    logic rd_term_seen_rise;

    // command byte
    logic [7:0] cmd;

    // Read-ID timeout watchdog
    logic [23:0] id_timeout;
    // Small CS# high gap counter between commands
    logic [3:0] gap_cnt;

    // chip ID detection
    logic [7:0] mfg_id;    // manufacturer ID
    logic [7:0] kgd_id;    // known good die ID

    // QPI mode tracking (set after Enter Quad Mode 0x35)
    logic       qpi_mode;

    // helpers
    function automatic logic [7:0] wbyte(input logic [31:0] wd, input logic [1:0] ln);
        case (ln)
            2'd0: wbyte = wd[7:0];
            2'd1: wbyte = wd[15:8];
            2'd2: wbyte = wd[23:16];
            default: wbyte = wd[31:24];
        endcase
    endfunction

    // Address for a given lane
    function automatic logic [22:0] addr_plus_lane(input logic [22:0] a, input logic [1:0] ln);
        addr_plus_lane = a + {{21{1'b0}}, ln};
    endfunction

    // --------------------------
    // FSM sequencing
    // --------------------------
    always_ff @(posedge clk) begin

        if (rst) begin
            st        <= ST_POR;
            por_cnt   <= '0;

            sh_tx_req    <= 1'b0;
            sh_rx_req    <= 1'b0;
            sh_req_byte  <= 8'h00;
            sh_req_width <= W_X1;

            ps_ce_n    <= 1'b1;
            sclk_en    <= 1'b0;
            sio_oe_fsm <= 4'b0000;
            sio_out_fsm<= 4'h0;

            mem_ready <= 1'b0;
            mem_rdata <= 32'h0000_0000;

            addr_l    <= '0;
            base_addr <= '0;
            wdata_l   <= '0;
            wstrb_l   <= 4'h0;
            wstrb_orig <= 4'h0;
            we_l      <= 1'b0;
            lane      <= 2'd0;

            rdata_l   <= 32'h0;
            dummy_cnt <= '0;
            rd_term_seen_rise <= 1'b0;
            cmd       <= 8'h00;
               mfg_id    <= 8'h00;
               kgd_id    <= 8'h00;
                    qpi_mode  <= 1'b0;
               sclk_fast_mode <= 1'b0;
               psram_chip_present <= 1'b0;
                id_timeout <= 24'h000000;
                gap_cnt    <= 4'd0;
            reset_wait_cnt <= '0;
        end else begin

            // Default: clear shifter kick pulses each cycle; states will raise them when needed
            sh_tx_req <= 1'b0;
            sh_rx_req <= 1'b0;

            mem_ready <= 1'b0;

            unique case (st)

                // --- POR: hold CE high, SCLK low, IO low-ish
                ST_POR: begin
                    //status[5] <= 1'b1; // indicate POR
                    ps_ce_n    <= 1'b1;
                    sclk_en    <= 1'b0;
                    // Keep IOs tri-stated during POR
                    sio_oe_fsm <= 4'b0000;
                    sio_out_fsm<= 4'h0;

                    if (por_cnt == POR_TICKS) begin
                        por_cnt <= '0;
                        // Start QPI-form reset-enable first for warm-reset recovery.
                        ps_ce_n <= 1'b0;
                        sh_start_tx(8'h66, W_X4); // RSTEN (QPI-form)
                        st <= ST_QPI_RSTEN_CMD;
                    end else begin
                        por_cnt <= por_cnt + 1;
                    end
                end

                ST_QPI_RSTEN_CMD: begin
                    st <= ST_QPI_RSTEN_WAIT;
                end

                ST_QPI_RSTEN_WAIT: begin
                    sclk_en <= 1'b1;
                    if (sh_done_pulse) begin
                        sh_start_tx(8'h99, W_X4);
                        st <= ST_QPI_RST_CMD;
                    end
                end

                ST_QPI_RST_CMD: begin
                    st <= ST_QPI_RST_WAIT;
                end

                ST_QPI_RST_WAIT: begin
                    sclk_en <= 1'b1;
                    if (sh_done_pulse) begin
                        // End contiguous QPI RSTEN/RST transaction and wait tRST.
                        ps_ce_n        <= 1'b1;
                        sclk_en        <= 1'b0;
                        reset_wait_cnt <= RESET_WAIT_TICKS[$clog2(RESET_WAIT_TICKS+1)-1:0];
                        st <= ST_QPI_POST_RST_WAIT;
                    end
                end

                ST_QPI_POST_RST_WAIT: begin
                    if (reset_wait_cnt != 0) begin
                        reset_wait_cnt <= reset_wait_cnt - 1;
                    end else begin
                        // After reset wait, send explicit QPI exit as its own transaction.
                        ps_ce_n <= 1'b0;
                        sh_start_tx(8'hF5, W_X4);
                        st <= ST_QPI_EXIT_WAIT;
                    end
                end

                ST_QPI_EXIT_WAIT: begin
                    sclk_en <= 1'b1;
                    if (sh_done_pulse) begin
                        // Insert CS# high gap before SPI-form reset sequence.
                        ps_ce_n    <= 1'b1;
                        sclk_en    <= 1'b0;
                        gap_cnt    <= 4'd8; // small CS# high gap (~8 clk cycles)
                        st <= ST_RSTEN_GAP;
                    end
                end

                ST_RSTEN_GAP: begin
                    if (gap_cnt != 0) begin
                        gap_cnt <= gap_cnt - 1;
                    end else begin
                        // Always end recovery with SPI-form reset so READ ID starts from SPI.
                        ps_ce_n <= 1'b0;
                        sh_start_tx(8'h66, W_X1);
                        st <= ST_SPI_RSTEN_CMD;
                    end
                end

                ST_SPI_RSTEN_CMD: begin
                    // Wait one cycle for shifter to load data
                    st <= ST_SPI_RSTEN_WAIT;
                end

                ST_SPI_RSTEN_WAIT: begin
                    // Now shifter is active with data loaded, enable SCLK
                    sclk_en <= 1'b1;
                    if (sh_done_pulse) begin
                        // Immediately follow with 0x99 while CS# stays low (SPI)
                        sh_start_tx(8'h99, W_X1); // RST
                        st <= ST_SPI_RST_CMD;
                    end
                end

                ST_SPI_RST_CMD: begin
                    // One-cycle launch/load state for 0x99 (RST)
                    st <= ST_SPI_RST_WAIT;
                end

                ST_SPI_RST_WAIT: begin
                    // Run the 0x99 byte, then wait tRST before probing ID.
                    sclk_en <= 1'b1;
                    if (sh_done_pulse) begin
                        ps_ce_n         <= 1'b1;
                        sclk_en         <= 1'b0;
                        reset_wait_cnt  <= RESET_WAIT_TICKS[$clog2(RESET_WAIT_TICKS+1)-1:0];
                        st <= ST_POST_RST_GAP;
                    end
                end

                ST_POST_RST_GAP: begin
                    if (reset_wait_cnt != 0) begin
                        reset_wait_cnt <= reset_wait_cnt - 1;
                    end else begin
                        st <= ST_READID_CMD;
                    end
                end

                ST_ENTER_QUAD_CMD: begin
                    // Issue Enter Quad Mode (0x35) in x1
                    ps_ce_n <= 1'b0;
                    sh_start_tx(8'h35, W_X1);
                    st <= ST_ENTER_QUAD_WAIT;
                end

                ST_ENTER_QUAD_WAIT: begin
                    // Now shifter is active with data loaded, enable SCLK
                    sclk_en <= 1'b1;
                    if (sh_done_pulse) begin
                        st <= ST_ENTER_QUAD;
                    end
                end

                ST_ENTER_QUAD: begin
                    // Wrap up quad-enter
                    ps_ce_n    <= 1'b1;
                    sclk_en    <= 1'b0;
                    sio_oe_fsm <= 4'b0000;
                    qpi_mode   <= 1'b1;
                    sclk_fast_mode <= 1'b1;
                    st         <= ST_IDLE;
                end

                // --- Read ID: issue 0x9F command in SPI mode (x1) to read manufacturer ID
                ST_READID_CMD: begin
                    // Do not preempt init by mem_valid: complete detection first.
                    ps_ce_n     <= 1'b0;
                    id_timeout  <= 24'h000000; // reset timeout
                    qpi_mode    <= 1'b0;  // still in SPI during ID probe
                    sclk_fast_mode <= 1'b0;
                    sh_start_tx(8'h9F, W_X1); // Read ID opcode (SPI)
                    st <= ST_READID_WAIT;
                end

                ST_READID_WAIT: begin
                    // Now shifter is active with data loaded, enable SCLK
                    sclk_en <= 1'b1;
                    if (sh_done_pulse) begin
                        // Datasheet describes an address phase (similar to Fast Read, without waits).
                        // Send 24-bit address = 0x000000, then read ID bytes.
                        sh_start_tx(8'h00, W_X1);
                        st <= ST_READID_A2;
                    end
                end

                ST_READID_A2: begin
                    if (sh_done_pulse) begin
                        sh_start_tx(8'h00, W_X1);
                        st <= ST_READID_A1;
                    end
                end

                ST_READID_A1: begin
                    if (sh_done_pulse) begin
                        sh_start_tx(8'h00, W_X1);
                        st <= ST_READID_A0;
                    end
                end

                ST_READID_A0: begin
                    if (sh_done_pulse) begin
                        // After last address byte, enable SCLK and start RX for response
                        sio_oe_fsm <= 4'b0000;   // release IOs
                        sclk_en    <= 1'b1;      // KEEP SCLK RUNNING for response bytes
                        sh_start_rx(W_X1);       // start capturing vendor ID
                        st <= ST_READID_MFG;
                    end
                end

                ST_READID_RX_DELAY: begin // XXX not used, but retained for clarity
                    // Wait for one SCLK cycle to let PSRAM drive the first data bit
                    sio_oe_fsm <= 4'b0000; // release IOs
                    sclk_en    <= 1'b1;    // keep SCLK running
                    if (sclk_fall) begin
                        // PSRAM data is now stable; start RX sampling
                        sh_start_rx(W_X1); // manufacturer ID
                        st <= ST_READID_MFG;
                    end
                end

                // Optional dummy state retained but bypassed
                ST_READID_MFG: begin
                    sclk_en <= 1'b1;  // KEEP SCLK RUNNING while waiting for response
                    if (rx_done_pulse) begin
                        mfg_id <= rx_byte;
                        sh_start_rx(W_X1); // KGD (Known Good Die) or continuation byte
                        st <= ST_READID_KGD;
                    end else begin
                        id_timeout <= id_timeout + 1;
                        if (id_timeout == 24'hFFFFFF) begin
                            // Timeout: assume no chip, clean up and proceed
                            ps_ce_n    <= 1'b1;
                            sclk_en    <= 1'b0;
                            sio_oe_fsm <= 4'b0000;
                            psram_chip_present <= 1'b0;
                            st <= ST_IDLE;
                        end
                    end
                end

                ST_READID_KGD: begin
                    sclk_en <= 1'b1;  // KEEP SCLK RUNNING while waiting for response
                    if (rx_done_pulse) begin
                        kgd_id <= rx_byte;
                        ps_ce_n    <= 1'b1;
                        sclk_en    <= 1'b0;
                        sio_oe_fsm <= 4'b0000;

                        // Consider present if IDs are neither 0x00 nor 0xFF
                        if ((mfg_id != 8'h00) && (mfg_id != 8'hFF) && (rx_byte != 8'h00) && (rx_byte != 8'hFF)) begin
                            psram_chip_present <= 1'b1;
                            st <= ST_ENTER_QUAD_CMD;
                        end else begin
                            psram_chip_present <= 1'b0;
                            st <= ST_IDLE;
                        end
                    end else begin
                        id_timeout <= id_timeout + 1;
                        if (id_timeout == 24'hFFFFFF) begin
                            ps_ce_n    <= 1'b1;
                            sclk_en    <= 1'b0;
                            sio_oe_fsm <= 4'b0000;
                            psram_chip_present <= 1'b0;
                            st <= ST_IDLE;
                        end
                    end
                end

                // --- Idle: wait for CPU request that hits PSRAM range
                ST_IDLE: begin

                    //status[0] <= 1'b1; // indicate idle
                    if (psram_sel && !mem_ready) begin
                        if (!psram_chip_present) begin
                            // No chip present: respond immediately with default data
                            mem_rdata <= 32'hFFFF_FFFF;
                            mem_ready <= 1'b1;
                            // remain in IDLE
                        end else begin
                            led <= ~led; // indicate PSRAM access
                            st <= ST_LATCH_REQ;
                        end
//                        led <= 1'b1; // indicate PSRAM access

                        //status[1] <= ~status[1]; // indicate PSRAM access
                    end
                end

                ST_LATCH_REQ: begin
//                        led <= ~led; // indicate PSRAM access
                    // latch request; picorv32 holds mem_valid stable until mem_ready
                    addr_l  <= psram_addr;
                    base_addr <= psram_addr; // save base address for partial writes
                    wdata_l <= mem_wdata;
                    wstrb_l <= mem_wstrb;
                    wstrb_orig <= mem_wstrb; // preserve original for partial write iteration
                    we_l    <= (mem_wstrb != 4'b0000);

                    // For partial writes, apply byte-lane offset to the very first transaction.
                    // picorv32 may provide a word-aligned address with lane encoded in mem_wstrb.
                    if ((mem_wstrb != 4'b0000) && (mem_wstrb != 4'b1111)) begin
                        logic [1:0] first_lane;
                        if      (mem_wstrb[0]) first_lane = 2'd0;
                        else if (mem_wstrb[1]) first_lane = 2'd1;
                        else if (mem_wstrb[2]) first_lane = 2'd2;
                        else                   first_lane = 2'd3;

                        lane    <= first_lane;
                        addr_l  <= addr_plus_lane(psram_addr, first_lane);
                        wstrb_l <= (4'b0001 << first_lane);
                    end else begin
                        lane    <= 2'd0;
                    end

                    // begin transaction
                    ps_ce_n <= 1'b0;

                    // command: read 0xEB, write 0x38
                    cmd <= (mem_wstrb != 0) ? 8'h38 : 8'hEB;
                    sh_start_tx((mem_wstrb != 0) ? 8'h38 : 8'hEB, width_t'(qpi_mode ? W_X4 : W_X1));
                    st <= ST_CMD;
                end

                // --- Send 24-bit address (quad)
                ST_CMD: begin
                    // Wait one cycle for shifter to load data
                    st <= ST_CMD_WAIT;
                end

                ST_CMD_WAIT: begin
                    // Now shifter is active with data loaded, enable SCLK
                    sclk_en <= 1'b1;
                    if (sh_done_pulse) begin
                        sh_start_tx({1'b0, addr_l[22:16]}, W_X4); // A23=0
                        st <= ST_A2;
                    end
                end

                ST_A2: if (sh_done_pulse) begin sh_start_tx(addr_l[15:8], W_X4); st <= ST_A1; end
                ST_A1: if (sh_done_pulse) begin sh_start_tx(addr_l[7:0],  W_X4); st <= ST_A0; end

                ST_A0: begin
                    if (sh_done_pulse) begin
                        if (!we_l) begin
                            // Read: dummy cycles then receive 4 bytes
                            dummy_cnt <= RD_WAIT_CLKS[$clog2(RD_WAIT_CLKS+1)-1:0];
                            st <= ST_RD_DUMMY;
                        end else begin
                            // Write:
                            // If full word write, send 4 bytes in one transaction.
                            // Else do per-byte writes (one transaction per lane) for correctness.
                            if (wstrb_l == 4'b1111) begin
                                sh_start_tx(wdata_l[7:0], W_X4);
                                st <= ST_WR_W0;
                            end else begin
                                // Start with lane 0; find first enabled lane in ST_NEXT_BYTE
                                st <= ST_NEXT_BYTE;
                            end
                        end
                    end
                end

                // --- Read dummy: just toggle clocks with IO released
                ST_RD_DUMMY: begin
                    sio_oe_fsm <= 4'b0000;
                    if (sclk_rise && dummy_cnt != 0) begin
                        dummy_cnt <= dummy_cnt - 1;
                        if (dummy_cnt == 1) begin
                            // Start receiving 4 bytes in quad
                            rdata_l <= 32'h0;
                            sh_start_rx(W_X4);
                            st <= ST_RD_B0;
                        end
                    end
                end

                // Receive 4 bytes (little-endian: byte0 at addr -> bits[7:0])
                ST_RD_B0: begin
                    if (rx_done_pulse) begin
                        rdata_l[7:0] <= rx_byte;
                        sh_start_rx(W_X4);
                        st <= ST_RD_B1;
                    end
                end
                ST_RD_B1: begin
                    if (rx_done_pulse) begin
                        rdata_l[15:8] <= rx_byte;
                        sh_start_rx(W_X4);
                        st <= ST_RD_B2;
                    end
                end
                ST_RD_B2: begin
                    if (rx_done_pulse) begin
                        rdata_l[23:16] <= rx_byte;
                        sh_start_rx(W_X4);
                        st <= ST_RD_B3;
                    end
                end
                ST_RD_B3: begin
                    if (rx_done_pulse) begin
                        rdata_l[31:24] <= rx_byte;
                        // Datasheet §8.6 recommends a longer CE# hold on read termination:
                        // tCHD > tACLK + tCLK. Insert a post-read hold state that waits for one
                        // more full SCLK cycle (rise+fall) before deasserting CE#.
                        rd_term_seen_rise <= 1'b0;
                        st <= ST_RD_TERM;
                    end
                end

                // Post-read termination hold: keep clocks running and CE# asserted.
                // Wait for one additional rising edge and the subsequent falling edge,
                // then finish (which deasserts CE# with SCLK low).
                ST_RD_TERM: begin
                    sio_oe_fsm <= 4'b0000;
                    sclk_en    <= 1'b1;
                    if (!rd_term_seen_rise) begin
                        if (sclk_rise) rd_term_seen_rise <= 1'b1;
                    end else begin
                        if (sclk_fall) st <= ST_FINISH;
                    end
                end

                // --- Full-word write (single transaction, 4 sequential bytes)
                ST_WR_W0: if (sh_done_pulse) begin sh_start_tx(wdata_l[15:8],  W_X4); st <= ST_WR_W1; end
                ST_WR_W1: if (sh_done_pulse) begin sh_start_tx(wdata_l[23:16], W_X4); st <= ST_WR_W2; end
                ST_WR_W2: if (sh_done_pulse) begin sh_start_tx(wdata_l[31:24], W_X4); st <= ST_WR_W3; end
                ST_WR_W3: if (sh_done_pulse) begin st <= ST_FINISH; end

                // --- Partial write: do one-byte transactions per enabled lane
                ST_NEXT_BYTE: begin
                    // Find next lane with wstrb=1 using wstrb_orig (not the mangled wstrb_l)
                    if (lane == 2'd0 && !wstrb_orig[0]) lane <= 2'd1;
                    else if (lane == 2'd1 && !wstrb_orig[1]) lane <= 2'd2;
                    else if (lane == 2'd2 && !wstrb_orig[2]) lane <= 2'd3;
                    else if (lane == 2'd3 && !wstrb_orig[3]) begin
                        // no more bytes to write
                        st <= ST_FINISH;
                    end else begin
                        // Start a single-byte write transaction for this lane:
                        // End current transaction (if any), then wait briefly with CS# high.
                        // The next state starts a clean new command+address+data transaction.
                        ps_ce_n    <= 1'b1;
                        sclk_en    <= 1'b0;
                        sio_oe_fsm <= 4'b0000;

                        // Update address for this lane using saved base address
                        addr_l <= addr_plus_lane(base_addr, lane);

                        // To guarantee a real CS# high period, restart via dedicated gap state.
                        gap_cnt <= 4'd2;
                        st <= ST_PARTIAL_GAP;

                        // To make the branch correct, we use a sentinel: set wstrb_l to one-hot of current lane
                        // then in ST_A0 we will go to ST_WR_BYTE
                        wstrb_l <= (4'b0001 << lane);
                    end
                end

                ST_PARTIAL_GAP: begin
                    if (gap_cnt != 0) begin
                        gap_cnt <= gap_cnt - 1;
                    end else begin
                        ps_ce_n <= 1'b0;
                        sh_start_tx(8'h38, width_t'(qpi_mode ? W_X4 : W_X1)); // write opcode (QPI: quad cmd)
                        cmd <= 8'h38;
                        st <= ST_CMD;
                    end
                end

                ST_WR_BYTE: begin
                    // not used directly; handled via ST_A0 branching below
                end

                // Override ST_A0 branching for the one-hot partial write case:
                // After header, send exactly one byte then advance lane.
                default: begin
                    // no-op
                end

            endcase

            // Special handling: when doing partial writes, after header we want to send exactly one byte.
            // We use the one-hot wstrb_l encoding set in ST_NEXT_BYTE to detect this.
            if (st == ST_A0 && sh_done_pulse && we_l && (wstrb_l != 4'b1111) && (wstrb_l != 4'b0000)) begin
                // one-hot lane assumed
                logic [1:0] ln;
                if      (wstrb_l[0]) ln = 2'd0;
                else if (wstrb_l[1]) ln = 2'd1;
                else if (wstrb_l[2]) ln = 2'd2;
                else                 ln = 2'd3;

                sh_start_tx(wbyte(wdata_l, ln), W_X4); // send one byte
                st <= ST_WR_BYTE;
            end
// NOTE: placeholder comment removed in cleanup
            if (st == ST_WR_BYTE && sh_done_pulse) begin
                // Clear the just-written lane from wstrb_orig and advance lane
                if      (wstrb_l[0]) begin
                    wstrb_orig <= wstrb_orig & 4'b1110;
                    lane <= 2'd1;
                end else if (wstrb_l[1]) begin
                    wstrb_orig <= wstrb_orig & 4'b1101;
                    lane <= 2'd2;
                end else if (wstrb_l[2]) begin
                    wstrb_orig <= wstrb_orig & 4'b1011;
                    lane <= 2'd3;
                end else begin
                    wstrb_orig <= wstrb_orig & 4'b0111;
                    lane <= 2'd3;
                end

                st <= ST_NEXT_BYTE;
            end

            // Finish transaction: deassert CE#, stop clock, respond to CPU.
            if (st == ST_FINISH) begin
                ps_ce_n    <= 1'b1;
                sclk_en    <= 1'b0;
                sio_oe_fsm <= 4'b0000;

                mem_rdata <= rdata_l;
                mem_ready <= 1'b1;   // one-cycle pulse; picorv32 will advance
                st <= ST_IDLE;
            end
        end
    end

endmodule
