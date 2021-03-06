/***************************************************************************
 *   Copyright (C) 2012 by Max Thrun                                       *
 *   Copyright (C) 2012 by Ian Cathey                                      *
 *   Copyright (C) 2012 by Mark Labbato                                    *
 *                                                                         *
 *   Embedded System - Traffic Light                                       *
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "traffic.h"
#include "draw.h"

// task stacks
OS_STK stack_a[TASK_STACKSIZE];
OS_STK stack_b[TASK_STACKSIZE];
OS_STK stack_c[TASK_STACKSIZE];
OS_STK stack_d[TASK_STACKSIZE];
OS_STK stack_e[TASK_STACKSIZE];
OS_STK stack_f[TASK_STACKSIZE];
OS_STK stack_input[TASK_STACKSIZE];

// traffic light values
int lights[6];

// traffic light mutex
OS_EVENT *light_lock;

// keyboard input
char key_down[256];

// keyboard input lock
OS_EVENT *input_lock;

// state of the lights in task a
int traffic_state = PRI_GREEN;

// flag indicating if we are in manual mode or not
// 1 - Manual mode enabled
// 0 - Manual mode disabled
int manual_mode = 0;

// when in manual mode this value keeps track of which light is currently selected
int selected_light = 0;

// number of cycles to stay in the emergency state
int emergency_duration = 5;

// helper functions to make code more readable
inline void pend(OS_EVENT *pevent) { INT8U rt;  OSSemPend(pevent, 0, &rt); alt_ucosii_check_return_code(rt);}
inline void post(OS_EVENT *pevent) { INT8U rt = OSSemPost(pevent);         alt_ucosii_check_return_code(rt);}

//=============================================================================
//
//  KEY PRESSED
//
//  Lookup 'key' in the key_down array and return 1 if the flag for that key is
//  set, 0 if it's not.
//
//=============================================================================
int key_pressed(char key) 
{
    // flag to signal if 'key' has been pressed or not
    int pressed = 0;

    // wait for the input lock
    pend(input_lock);

    // check if this key has been pressed
    if (key_down[key]) 
    {
        pressed = 1;        // key has been pressed so return true
        key_down[key] = 0;  // we just handled this key so clear it
    } 

    // release the lock
    post(input_lock);

    // return whether the key was pressed or not
    return pressed;
}

//=============================================================================
//
//  TASK INPUT
//
//  Continually monitor the jtag uart for received bytes and set a flag in
//  key_down[ ] indexed by that characters ascii code.
//
//=============================================================================
void task_input(void *pdata) 
{
    FILE* fp = fopen("/dev/jtag_uart_0", "r+");

    while(1) 
    {
        // if there was an error opening the file display a message
        if (!fp) 
        {
            draw_status(2, "Error opening file");
            continue;
        }

        // get the next character
        int c = getc(fp);

        // check if it's a valid character
        if (c != EOF) 
        {
            // get control of the lock and mark this character as pressed
            pend(input_lock);
            key_down[c] = 1;
            post(input_lock);
        }

        // global reset drawing function
        if (key_pressed('r')) draw_reset();

        // allow other tasks to run
        delay_ms(10);
    }
}

//=============================================================================
//
//  TASK A
//
//  Primary Street is a main thoroughfare, with heavy traffic. Secondary Street
//  is used by fewer vehicles.  This difference should be reflected in the
//  relative times that traffic on each street has a green signal.
//
//=============================================================================
void task_a(void *pdata) 
{

    // how much time to delay after updating the lights
    int delay_time = 0;

    while(1) 
    { 
        // get control of the lights
        pend(light_lock);

        // make all lights red so we have a 
        // known starting condition
        lights[PRI_1] = RED;
        lights[PRI_2] = RED;
        lights[TURN_1] = RED;
        lights[TURN_2] = RED;
        lights[SEC_1] = RED;
        lights[SEC_2] = RED;

        // figure out what state we are currently in
        switch (traffic_state) 
        {
            case PRI_GREEN:

                lights[PRI_1] = GREEN;
                lights[PRI_2] = GREEN;
                delay_time = PRI_GREEN_TIME;
                traffic_state = PRI_YELLOW;
                draw_status(0, "Primary green");
                break;

            case PRI_YELLOW:

                lights[PRI_1] = YELLOW;
                lights[PRI_2] = YELLOW;
                delay_time = PRI_YELLOW_TIME;
                traffic_state = PRI_RED;
                draw_status(0, "Primary yellow");
                break;

            case PRI_RED:

                lights[PRI_1] = RED;
                lights[PRI_2] = RED;
                delay_time = PRI_RED_TIME;
                traffic_state = SEC_GREEN;
                draw_status(0, "Primary red");
                break;

            case SEC_GREEN:

                lights[SEC_1] = GREEN;
                lights[SEC_2] = GREEN;
                delay_time = SEC_GREEN_TIME;
                traffic_state = SEC_YELLOW;
                draw_status(0, "Secondary green");
                break;
   
            case SEC_YELLOW:

                lights[SEC_1] = YELLOW;
                lights[SEC_2] = YELLOW;
                delay_time = SEC_YELLOW_TIME;
                traffic_state = SEC_RED;
                draw_status(0, "Secondary yellow");
                break;

            case SEC_RED:

                lights[SEC_1] = RED;
                lights[SEC_2] = RED;
                delay_time = SEC_RED_TIME;
                traffic_state = PRI_GREEN;
                draw_status(0, "Secondary red");
                break;
        }

        // update the display with new light values
        draw_lights();  

        // we are done changing the lights
        post(light_lock);

        // delay for however long this light is on for
        delay(delay_time);   
    }
}

//=============================================================================
//
//  TASK B
//
//  Primary Street also has left turn lanes onto Secondary Street (in both
//  directions). If there is a vehicle in either of these lanes when it is time
//  for Primary Street to have a green signal, there should first be a task run
//  to enable these vehicles to make left turns, while the other traffic on
//  Primary Street and Secondary Street gets a stop signal.
//
//=============================================================================
void task_b(void *pdata) 
{

    // start off checking for cars
    int turn_state = CHECK_CARS;

    while(1) 
    {
        // figure out which state of the turn we are in
        switch (turn_state) 
        {

            case CHECK_CARS:

                // check if there is a car in the turn lane
                if (key_pressed('c')) 
                {
                    // there is a car so draw it
                    // display a message, and then wait for green
                    draw_car(1);
                    draw_status(1, "Car waiting");
                    turn_state = WAIT_FOR_GREEN;
                }

                break;

            case WAIT_FOR_GREEN:

                // is the primary street about to turn green?
                if (traffic_state == PRI_GREEN) 
                {
                    // take control of the lights
                    pend(light_lock);

                    // indicate our the current traffic state
                    draw_status(1, "Turn in progress");
                   
                    // turn light is now green
                    turn_state = TURN_GREEN;
                }

                break;

            case TURN_GREEN:

                // its green, cars are gone
                draw_car(0);

                // turn on the turn lights
                lights[TURN_1] = GREEN;
                lights[TURN_2] = GREEN;

                // all other lights should be red
                lights[SEC_1] = RED;
                lights[SEC_2] = RED;
                lights[PRI_1] = RED;
                lights[PRI_2] = RED;
               
                // update the drawing with new light values
                draw_lights();

                draw_status(0, "Turn green");

                // turn green light duration
                delay(TURN_GREEN_TIME);

                turn_state = TURN_YELLOW;

                break;

            case TURN_YELLOW:

                // turn lights are now yellow
                lights[TURN_1] = YELLOW;
                lights[TURN_2] = YELLOW;

                draw_status(0, "Turn yellow");

                // update the diagram with new light values
                draw_lights();

                // turn yellow duration
                delay(TURN_YELLOW_TIME);

                turn_state = TURN_RED;

                break;

            case TURN_RED:

                // turn lights are now red
                lights[TURN_1] = RED;
                lights[TURN_2] = RED;

                draw_status(0, "Turn red");
                draw_status(1, "Turn complete");

                // update the diagram with new light values
                draw_lights();

                // go back to wait for more cars
                turn_state = CHECK_CARS;

                // release control of the lights
                post(light_lock);

                break;
        }

        delay_ms(10);
    }
}

//=============================================================================
//
//  TASK C
//
//  There are also pedestrian buttons for crossing both Primary Street and
//  Secondary Street. If any of these buttons is pushed, a task to make all
//  vehicles stop and allow for pedestrians to cross the streets should be run.
//
//=============================================================================
void task_c(void *pdata) 
{

    int walk_state = CHECK_WALK;

    while(1) 
    {

        switch(walk_state) 
        {
            case CHECK_WALK:

                if (key_pressed('w')) 
                {
                    draw_walk(WAITING);
                    draw_status(2, "Walk button pressed");
                    walk_state = WAIT_FOR_RED;
                }

                break;

            case WAIT_FOR_RED:

                // are the lights about to turn red?
                if (traffic_state == PRI_GREEN || traffic_state == SEC_GREEN) 
                {
                    // try to get control of the lights
                    pend(light_lock);

                    // indicate a walk is in progress
                    draw_status(2, "Walk in progress");

                    // turn the walk signal green
                    walk_state = WALK_GREEN;
                }

                break;

            case WALK_GREEN:

                draw_walk(GREEN);
                delay(WALK_GREEN_TIME);
                walk_state = WALK_YELLOW;
                break;

            case WALK_YELLOW:

                draw_walk(YELLOW);
                delay(WALK_YELLOW_TIME);
                walk_state = WALK_RED;
                break;

            case WALK_RED:

                draw_walk(RED);
                draw_status(2, "Walk complete");
                delay(WALK_RED_TIME);
                walk_state = CHECK_WALK;

                post(light_lock);

                break;
        }

        delay_ms(10);
    }
}

//=============================================================================
//
//  TASK D
//
//  There is also an emergency setting, which sets lights in all directions to
//  flashing red. This setting is triggered by a special signal which includes
//  a specific duration to be in this state. At the end of this time, the
//  system should return to task A.
//
//=============================================================================
void task_d(void *pdata) 
{

    int emergency_state = CHECK_EMERGENCY;
    int flash_count = 0;

    while(1) 
    {
        
        switch (emergency_state) 
        {

            case CHECK_EMERGENCY:

                if (key_pressed('-')) 
                {
                    // decrement the emergency duration and update the
                    // display to show its current value
                    emergency_duration--;
                    if (emergency_duration < 0) emergency_duration = 0;
                    draw_keymap();
                }

                if (key_pressed('+')) 
                {
                    // increment the emergency duration and update the
                    // display to show its current value
                    emergency_duration++;
                    if (emergency_duration > 9) emergency_duration = 9;
                    draw_keymap();
                }

                if (key_pressed('e')) 
                {
                    // wait to gain control of them
                    pend(light_lock);
                    flash_count = 0;
                    draw_status(0, "Emergency!");
                    emergency_state = FLASH_RED_ON;
                }

                break;

            case FLASH_RED_ON:

                // turn the lights on
                lights[PRI_1] = RED;
                lights[PRI_2] = RED;
                lights[TURN_1] = RED;
                lights[TURN_2] = RED;
                lights[SEC_1] = RED;
                lights[SEC_2] = RED;

                draw_lights();
                delay(FLASH_TIME);

                // we only want to flash a ceratin number of times
                flash_count++;
                if (flash_count == emergency_duration) 
                {
                    // we're done flashing, go back and check the button again
                    emergency_state = CHECK_EMERGENCY;

                    // release control of the lights
                    post(light_lock);
                } 
                else 
                {
                    // we're not done flashing yet, flash them again
                    emergency_state = FLASH_RED_OFF;
                }

                break;

            case FLASH_RED_OFF:

                // turn the lights off
                lights[PRI_1] = OFF;
                lights[PRI_2] = OFF;
                lights[TURN_1] = OFF;
                lights[TURN_2] = OFF;
                lights[SEC_1] = OFF;
                lights[SEC_2] = OFF;

                draw_lights();
                delay(FLASH_TIME);

                emergency_state = FLASH_RED_ON;

                break;

        }

        delay_ms(10);
    }
}

//=============================================================================
//
//  TASK E
//
//  There is also a broken setting which is activated when there is a power
//  outage, e.g., and which sets the signals on Primary Street to flashing
//  YELLOW and the signals on Secondary Street to flashing RED.  This setting
//  is deactivated manually, with a return to task A.
//
//=============================================================================
void task_e(void *pdata) 
{

    int broken = 0;
    int broken_state = CHECK_BROKEN;

    while(1) 
    {

        switch(broken_state) 
        {
        
            case CHECK_BROKEN:

                // check to see if the toggle key was pressed
                if (key_pressed('b')) 
                {
                    // check if we are already broken
                    if (broken) 
                    {
                        // we not broken anymore
                        broken = 0;
                        draw_status(0, "Lights not broken");

                        // release the lights
                        post(light_lock);
                    } 
                    else 
                    {
                        // lights are now broken
                        broken = 1;
                        draw_status(0, "Lights broken!");

                        // wait to gain control of them
                        pend(light_lock);

                        // flash them
                        broken_state = FLASH_ON;
                    }
                } 
                else 
                { 
                    // the toggle key wasnt pressed
                    // are we still broken? if so flash the lights again
                    if (broken) broken_state = FLASH_ON;
                }

                break;

            case FLASH_ON:

                // turn the lights on
                lights[PRI_1] = YELLOW;
                lights[PRI_2] = YELLOW;
                lights[TURN_1] = YELLOW;
                lights[TURN_2] = YELLOW;
                lights[SEC_1] = RED;
                lights[SEC_2] = RED;

                draw_lights();
                delay(FLASH_TIME);

                broken_state = FLASH_OFF;

                break;

            case FLASH_OFF:

                // turn the lights off
                lights[PRI_1] = OFF;
                lights[PRI_2] = OFF;
                lights[TURN_1] = OFF;
                lights[TURN_2] = OFF;
                lights[SEC_1] = OFF;
                lights[SEC_2] = OFF;

                draw_lights();
                delay(FLASH_TIME);

                // go back and see if we're still broken
                broken_state = CHECK_BROKEN;

                break;

        }

        delay_ms(10);
    }
}

//=============================================================================
//
//  TASK F
//
//  And finally there is a manual setting in which the signals are switched by
//  hand. This task is triggered by a switch operated by a human and is turned
//  off by another switch, at which time the system should return to task A.
//
//=============================================================================
void task_f(void *pdata) 
{

    while(1) 
    {

        // check for manual mode key press
        if (key_pressed('m')) 
        {
            // we are already in manual mode
            if (manual_mode) 
            {
                // come out of manual mode
                manual_mode = 0;

                // set all the lights to red
                lights[PRI_1] = RED;
                lights[PRI_2] = RED;
                lights[TURN_1] = RED;
                lights[TURN_2] = RED;
                lights[SEC_1] = RED;
                lights[SEC_2] = RED;
                draw_lights();

                // release the lights
                post(light_lock);

            } 
            else 
            {
                // we are now in manual mode
                manual_mode = 1;

                // get control of the lights
                pend(light_lock);

                draw_status(0, "Manual mode");
                draw_lights();
            }
        }

        // if we're in manual mode handle the key presses
        if (manual_mode) 
        {
            // check which key has been pressed
            // need to use if-else tree because multiple
            // keys could be pushed at same time

            if (key_pressed('j')) 
            {
                selected_light--;
                if (selected_light < 0) selected_light = 5;
                draw_lights();
            }
            else if (key_pressed('k')) 
            {
                selected_light++;
                if (selected_light > 5) selected_light = 0;
                draw_lights();
            }
            else if (key_pressed('1')) 
            {
                lights[selected_light] = RED;
                draw_lights();
            }
            else if (key_pressed('2')) 
            {
                lights[selected_light] = YELLOW;
                draw_lights();
            }
            else if (key_pressed('3')) 
            {
                lights[selected_light] = GREEN;
                draw_lights();
            }

        }

        delay_ms(10);
    }
}

void init(void) 
{
    // initialize draw function
    draw_init();

    // initialize the input mutex
    input_lock = OSSemCreate(1);

    // initialize all keys to not pressed
    memset(key_down, 0, sizeof(key_down));

    // initialize light mutex
    light_lock = OSSemCreate(1);

    // initialize all lights to RED
    int i = 0;
    for (i = 0; i < 6; i++)
        lights[i] = RED;

    int rt;
    rt = OSTaskCreate(task_a, NULL, (void*)&stack_a[TASK_STACKSIZE-1], 6); alt_ucosii_check_return_code(rt);
    rt = OSTaskCreate(task_b, NULL, (void*)&stack_b[TASK_STACKSIZE-1], 5); alt_ucosii_check_return_code(rt);
    rt = OSTaskCreate(task_c, NULL, (void*)&stack_c[TASK_STACKSIZE-1], 4); alt_ucosii_check_return_code(rt);
    rt = OSTaskCreate(task_d, NULL, (void*)&stack_d[TASK_STACKSIZE-1], 3); alt_ucosii_check_return_code(rt);
    rt = OSTaskCreate(task_e, NULL, (void*)&stack_e[TASK_STACKSIZE-1], 2); alt_ucosii_check_return_code(rt);
    rt = OSTaskCreate(task_f, NULL, (void*)&stack_f[TASK_STACKSIZE-1], 1); alt_ucosii_check_return_code(rt);
    rt = OSTaskCreate(task_input, NULL, (void*)&stack_input[TASK_STACKSIZE-1], 0); alt_ucosii_check_return_code(rt);
}

int main (int argc, char* argv[], char* envp[]) 
{
    init();
    OSStart();
    return 0;
}

