/***************************************************************************
 *   Copyright (C) 2012 by Max Thrun                                       *
 *                                                                         *
 *   Embedded System - Project 2                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

`include "constants.h"

module top(
    input CLOCK_50,
    input [3:0] KEY,
    input [9:0] SW,
    output [7:0] LEDG,
    output [9:0] LEDR,
    output [6:0] HEX0,
    output [6:0] HEX1,
    output [6:0] HEX2,
    output [6:0] HEX3
);

    // -------------------------------------------------
    //                  KEY MAPPING
    // -------------------------------------------------
 
    // invert the keys because we want to deal with
    // active high logic

    wire rst = ~KEY[0];
    wire start_stop = ~KEY[3];

    // -------------------------------------------------
    //              START / STOP TOGGLE
    // -------------------------------------------------

    // running flag indicates that the system is currently
    // processing numbers

    reg running = 0;

    always @(posedge start_stop, posedge rst)
        if (rst)
            running <= 0;
        else
            running <= ~running;

    // -------------------------------------------------
    //               RANDOM NUMBER GEN
    // -------------------------------------------------

    // on every clock cycle if we are running the lfsr
    // will output a new random 8-bit number

    wire signed [7:0] rand_value;

    LFSR lfsr(
        .clk(CLOCK_50),
        .en(running),
        .lfsr(rand_value)
    );

    // -------------------------------------------------
    //                  DIVIDE BY 3
    // -------------------------------------------------

    // on every clock cycle if we are running the divider
    // will divide the most recent random number by 3

    wire signed [7:0] div_value;

    div_3 div(
        .clk(CLOCK_50),
        .rst(rst),
        .en(running),
        .in(rand_value),
        .out(div_value)
    );

    // -------------------------------------------------
    //                   SUM LAST 3
    // -------------------------------------------------

    // the sum keeps track of the last three divided numbers
    // and outputs their sum

    wire signed [7:0] sum_value;

    sum_3 sum(
        .clk(CLOCK_50),
        .rst(rst),
        .en(running),
        .in(div_value),
        .out(sum_value)
    );

    assign LEDR = sum_value;

    // -------------------------------------------------
    //                    COUNTER
    // -------------------------------------------------
   
    // when the system is running the counter keeps track
    // of how many numbers we have generated. it outputs
    // the count in scientific notation: 00E0

    wire [6:0] base;
    wire [3:0] exponent;

    counter counter(
        .clk(CLOCK_50),
        .rst(rst),
        .en(running),
        .o_base(base),
        .o_exponent(exponent)
    );

    // -------------------------------------------------
    //                  BINARY 2 BCD
    // -------------------------------------------------

    // since the base from the counter is 2 digits we
    // need to split them up for the two 7-segments
  
    wire [3:0] base_bcd_ones;
    wire [3:0] base_bcd_tens;

    bin_2_bcd b2b_base(
        .bin(base),
        .ones(base_bcd_ones),
        .tens(base_bcd_tens)
    );

    // -------------------------------------------------
    //                 7-SEG DISPLAY
    // -------------------------------------------------

    seven_seg s0(exponent,  HEX0);
    seven_seg s1(`BCD_E,        HEX1);
    seven_seg s2(base_bcd_ones, HEX2);
    seven_seg s3(base_bcd_tens, HEX3);
            
endmodule

// vim: set textwidth=60:

