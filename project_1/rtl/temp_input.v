/***************************************************************************
 *   Copyright (C) 2012 by Max Thrun                                       *
 *   Copyright (C) 2012 by Ian Cathey                                      *
 *   Copyright (C) 2012 by Mark Labbato                                    *
 *                                                                         *
 *   Embedded System - Project 1                                           *
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

module temp_input(
    input rst,

    input enter,
    input [3:0] value,

    output [1:0] input_state,
    output reg new_number = 0,

    output [3:0] current_value,
    output reg [3:0] temp_value_ones = 0,
    output reg [3:0] temp_value_tens = 0,
    output reg [3:0] temp_value_huns = 0,

    output reg [3:0] temp_value_ones_old = 0,
    output reg [3:0] temp_value_tens_old = 0,
    output reg [3:0] temp_value_huns_old = 0
);

    // initialize the state variable to the 'DONE' state
    reg [1:0] state = `INPUT_STATE_DONE;

    // 'enter' signifies we want to clock in the next value
    always @(posedge enter, posedge rst) begin
        if (rst) begin
            state = `INPUT_STATE_DONE;
            new_number = 0;
            temp_value_ones = 0;
            temp_value_tens = 0;
            temp_value_huns = 0;
            temp_value_ones_old = 0;
            temp_value_tens_old = 0;
            temp_value_huns_old = 0;
        end else begin

            // lookup what state we are in
            // save the old value for that digit
            // latch in the new one
            // go to the next state

            case (state)
                `INPUT_STATE_ONES: begin 
                    temp_value_ones_old = temp_value_ones;
                    temp_value_ones = current_value;
                    state = `INPUT_STATE_TENS;
                end
                `INPUT_STATE_TENS: begin
                    temp_value_tens_old = temp_value_tens;
                    temp_value_tens = current_value;
                    state = `INPUT_STATE_HUNS;
                end
                `INPUT_STATE_HUNS: begin
                    temp_value_huns_old = temp_value_huns;
                    temp_value_huns = current_value;
                    new_number = 1;
                    state = `INPUT_STATE_DONE;
                end
                `INPUT_STATE_DONE: begin
                    new_number = 0;
                    state = `INPUT_STATE_ONES;
                end
            endcase

        end
    end

    // limit the current input value to 9 since bcd cannot
    // be greater than 9
    assign current_value = (value < 10) ? value : 9;

    // if we use the 'state' variable directly as an output
    // quartus won't recognize our state machine
    assign input_state = state;

endmodule
