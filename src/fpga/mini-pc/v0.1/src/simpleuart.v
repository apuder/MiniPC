/*
 *  PicoSoC - A simple example SoC using PicoRV32
 *
 *  Copyright (C) 2017  Claire Xenia Wolf <claire@yosyshq.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

module simpleuart #(
	parameter integer DEFAULT_DIV = 1,
	parameter integer DEFAULT_STOP_BITS = 1
) (
	input clk,
	input resetn,

	output ser_tx,
	input  ser_rx,

	input   [3:0] reg_div_we,
	input  [31:0] reg_div_di,
	output [31:0] reg_div_do,

	input   [3:0] reg_stop_we,
	input  [31:0] reg_stop_di,
	output [31:0] reg_stop_do,

	input         reg_dat_we,
	input         reg_dat_re,
	input  [31:0] reg_dat_di,
	output [31:0] reg_dat_do,
	output        reg_dat_wait,
	output        rx_ready_pulse
);
	reg [31:0] cfg_divider;
	reg [1:0] cfg_stop_bits;

	reg [3:0] recv_state;
	reg [31:0] recv_divcnt;
	reg [7:0] recv_pattern;
	reg [7:0] recv_buf_data;
	reg recv_buf_valid;
	reg rx_ready_pulse_q;
	reg [1:0] recv_stopcnt;

	reg [9:0] send_pattern;
	reg [3:0] send_bitcnt;
	reg [31:0] send_divcnt;
	reg send_dummy;

	assign reg_div_do = cfg_divider;
	assign reg_stop_do = cfg_stop_bits;

	assign reg_dat_wait = reg_dat_we && (send_bitcnt || send_dummy);
	assign reg_dat_do = recv_buf_valid ? recv_buf_data : ~0;
	assign rx_ready_pulse = rx_ready_pulse_q;

	always @(posedge clk) begin
		if (!resetn) begin
			cfg_divider <= DEFAULT_DIV;
			if (DEFAULT_STOP_BITS <= 1)
				cfg_stop_bits <= 1;
			else
				cfg_stop_bits <= 2;
		end else begin
			if (reg_div_we[0]) cfg_divider[ 7: 0] <= reg_div_di[ 7: 0];
			if (reg_div_we[1]) cfg_divider[15: 8] <= reg_div_di[15: 8];
			if (reg_div_we[2]) cfg_divider[23:16] <= reg_div_di[23:16];
			if (reg_div_we[3]) cfg_divider[31:24] <= reg_div_di[31:24];
			if (reg_stop_we[0]) begin
				if (reg_stop_di[0])
					cfg_stop_bits <= 2;
				else
					cfg_stop_bits <= 1;
			end else if (reg_stop_we[1]) begin
				if (reg_stop_di[8])
					cfg_stop_bits <= 2;
				else
					cfg_stop_bits <= 1;
			end else if (reg_stop_we[2]) begin
				if (reg_stop_di[16])
					cfg_stop_bits <= 2;
				else
					cfg_stop_bits <= 1;
			end else if (reg_stop_we[3]) begin
				if (reg_stop_di[24])
					cfg_stop_bits <= 2;
				else
					cfg_stop_bits <= 1;
			end
		end
	end

	always @(posedge clk) begin
		if (!resetn) begin
			recv_state <= 0;
			recv_divcnt <= 0;
			recv_pattern <= 0;
			recv_buf_data <= 0;
			recv_buf_valid <= 0;
			rx_ready_pulse_q <= 0;
			recv_stopcnt <= 0;
		end else begin
			rx_ready_pulse_q <= 0;
			recv_divcnt <= recv_divcnt + 1;
			if (reg_dat_re)
				recv_buf_valid <= 0;
			case (recv_state)
				0: begin
					if (!ser_rx)
						recv_state <= 1;
					recv_divcnt <= 0;
				end
				1: begin
					if (2*recv_divcnt > cfg_divider) begin
						recv_state <= 2;
						recv_divcnt <= 0;
					end
				end
				10: begin
					if (recv_divcnt > cfg_divider) begin
						recv_divcnt <= 0;
						if (!ser_rx) begin
							recv_state <= 0;
						end else if (recv_stopcnt > 1) begin
							recv_stopcnt <= recv_stopcnt - 1;
						end else begin
							recv_buf_data <= recv_pattern;
							recv_buf_valid <= 1;
							rx_ready_pulse_q <= 1;
							recv_state <= 0;
						end
					end
				end
				default: begin
					if (recv_divcnt > cfg_divider) begin
						recv_pattern <= {ser_rx, recv_pattern[7:1]};
						if (recv_state == 9) begin
							recv_state <= 10;
							recv_stopcnt <= cfg_stop_bits;
						end else begin
							recv_state <= recv_state + 1;
						end
						recv_divcnt <= 0;
					end
				end
			endcase
		end
	end

	assign ser_tx = send_pattern[0];

	always @(posedge clk) begin
		if (reg_div_we)
			send_dummy <= 1;
		send_divcnt <= send_divcnt + 1;
		if (!resetn) begin
			send_pattern <= ~0;
			send_bitcnt <= 0;
			send_divcnt <= 0;
			send_dummy <= 1;
		end else begin
			if (send_dummy && !send_bitcnt) begin
				send_pattern <= ~0;
				send_bitcnt <= 15;
				send_divcnt <= 0;
				send_dummy <= 0;
			end else
			if (reg_dat_we && !send_bitcnt) begin
				send_pattern <= {1'b1, reg_dat_di[7:0], 1'b0};
				send_bitcnt <= 9 + cfg_stop_bits;
				send_divcnt <= 0;
			end else
			if (send_divcnt > cfg_divider && send_bitcnt) begin
				send_pattern <= {1'b1, send_pattern[9:1]};
				send_bitcnt <= send_bitcnt - 1;
				send_divcnt <= 0;
			end
		end
	end
endmodule
