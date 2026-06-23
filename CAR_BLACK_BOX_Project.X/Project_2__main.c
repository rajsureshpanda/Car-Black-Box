


/*
    Name        :   Raj Suresh Panda

    Date        :   08 / 05 / 2026

    Description :   Car Black Box Project 

                    1. Embedded platform :
                        -> 1 PIC (18F4580) board.


                    2. Development environment and tools :
                        -> Dev environment: MPLAB.
                        -> Compilers: XC8 (tiny bootloader and tiny multi bootloader).
                        -> Tera Term.


                    3. Communication protocols used :
                        -> I2C communication protocol.
                        -> Uart serial communication protocol.


                    4. Required components :
                        -> Potentiometer (speed reading range from 0 t0 99).
                        -> Matrix keypad (switch-1, switch-2, switch-3, switch-11, switch-12).
                        -> DS1307 (Real Time Clock).
                        -> External EEPROM.
                        -> CLCD display. 


                    5. Different states (6) :

                        a). Dashboard :
                            -> When board is powered on or reset is pressed, default ON event is stored to external eeprom.
                            -> Integrated DS1307 Real-Time Clock (RTC) used for timestamp recording.
                            -> Matrix keypad used for changing gear and switch to collision mode.
                            -> Potentiometer is used to read speed which ranges from 0 to 99.
                            -> Whenever gear is changed the event is recorded to external eeprom.
                            -> Maximum of 10 events are stored and if more that 10 events then previous stored event is overwritten.
                            -> Overwritting is done similar to FIFO (First In First Out) sequence.
                            -> Matrix keypad :
                                -> Switch - 1  : Increment gear.
                                -> Switch - 2  : Decrement gear.
                                -> Switch - 3  : Collision mode.
                                -> Switch - 11 : Enter to main menu.
                            -> Gear sequence for increment and decrement : Gear-neutral, gear-1, gear-2, gear-3, gear-4, gear-5, gear-reverse. 
                            -> After gear is switched to collision mode and next after presssing increment or decrement button, the gear mode will always start from gear-neutral.

                        b). Main menu (4) :
                            -> There are four options - view log, download log, clear log and set time.
                            -> Matrix keypad :
                                -> Switch - 1  : Scroll up.
                                -> Switch - 2  : Scroll down.
                                -> Switch - 11 : Enter to arrow pointing option.
                                -> Switch - 12 : Exit from main menu and return to dashboard. 

                        c). View log :
                            -> If no event is present, then no log present message is printed.
                            -> If event is present, then it is printed in line-2 of clcd.
                            -> Matrix keypad :
                                -> Switch - 1  : Scroll up.
                                -> Switch - 2  : Scroll down.
                                -> Switch - 12 : Exit from view log and return to main menu.

                        d). Download log :
                            -> case-1 : If no event is present, then error message is printed in both clcd and tera term.
                            -> case-2 : If event is present, then downloading message is printed in clcd. 
                            -> After some delay all stored logs are printed in tera term and after that downloaded successfully message is printed in clcd.
                            -> For both cases, after some delay it is automatically exited from download log and return to main menu.
                            -> Here non-blocking delay is used.

                        e). Clear log :
                            -> case-1 : If no event is present, then no event present message is printed in both clcd and tera term.
                            -> case-2 : If event is present, then successfully clear all event logs message is printed in both clcd and tera term. 
                            -> For both cases, after some delay it is automatically exited from clear log and return to main menu.
                            -> Here non-blocking delay is used.

                        f). Set time :
                            -> Hour, minute and second is display to set time, that is change time of Real time clock (RTC).
                            -> Default value of hour, minute and second will be zero (0).
                            -> While entering to set time at first, by default second field will be blinking.
                            -> If field is blinking, then that field is in edit mode.
                            -> Matrix keypad :
                                -> Switch - 1  : Change blinking field to edit.
                                -> Switch - 2  : Increment blinking field value.
                                -> Switch - 11 : Write set time to RTC and then exit from set time & return to main menu.
                                -> Switch - 12 : Without making any changes to RTC, exit from set time and return to main menu.
                            -> Hour value ranges from 0 to 23.
                            -> Minute and second value ranges from 0 to 59.
                            -> Since only increment switch is used so after max value is reached then next it again starts from 0.
                            -> To write to RTC, decimal value is converted to bcd and then that bcd value is written to rtc of respective field.
                            -> Non-blocking delay is used to blink field for editing.
*/



#include <xc.h>
#include "black_box.h"
#include "matrix_keypad.h"
#include "adc.h"
#include "uart.h"
#include "clcd.h"
#include "i2c.h"
#include "ds1307.h"


// global variables.

State_t state;
State_t temp_state;

unsigned char key = 0;

unsigned char clear_clcd_line[17] = "                ";


// static initial configuration function definition.
static void init_config()
{   
    init_matrix_keypad();
    init_adc();
    init_uart();
    init_clcd();
    init_i2c();
	init_ds1307();
    
    state = e_dashboard;
    temp_state = 9;
}


void main(void) 
{
    // initial configuration function called.
    init_config();

    while(1)
    {
        // Detect key press

        key = read_switches(STATE_CHANGE);              // read switches function called (switch is edge trigger).
        
        if(temp_state != state)                         // if state is changed, then clear clcd line-1 and line-2.
        {
            clcd_print(clear_clcd_line, LINE1(0));
            clcd_print(clear_clcd_line, LINE2(0));
            
            temp_state = state;
        }
        
        switch (state)
        {
            case e_dashboard:
                // view dashboard function called.
                view_dashboard();
                break;
            
            case e_main_menu:
                // display main menu function called.
                display_main_menu();
                break;
            
            case e_view_log:
                // view log function called.
                view_log();
                break;
                 
            case e_download_log:
                // download log function called.
                download_log();
                break;
                
            case e_clear_log:
                // clear log function called.
                clear_log();
                break;
                
                      
            case e_set_time:
                // set time function called.
                set_time();
                break;      
        }
    }
    return;
}

