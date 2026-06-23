

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
#include "clcd.h"
#include "i2c.h"
#include "adc.h"
#include "ds1307.h"
#include "matrix_keypad.h"
#include "external_eeprom.h"
#include "uart.h"
#include "ds1307.h"


// global variables.

extern unsigned char key;
extern State_t state;
extern unsigned char time[9];

unsigned char gear_mode[9][3] = {"GN", "G1", "G2", "G3", "G4", "G5", "GR", "C ", "ON"};
unsigned char collision = 1;
unsigned char gear_pos = 8;
unsigned short speed = 0;
unsigned char on_event_flag = 0;

unsigned char main_menu_contents[5][15] = {"View Log    ", "Download Log", "Clear Log   ", "Set Time    "};
unsigned char arrow_pos = 0;
unsigned char selected_menu_pos = 0;
unsigned char menu_index = 0;

unsigned int event_count = 0;
unsigned char write_addr = 0x00;

unsigned char event_logs[11][15];
unsigned char read_addr = 0x00;
unsigned char view_log_scroll = 0;

unsigned long int clear_delay = 0;

unsigned long int download_delay = 0;

unsigned char hour = 0;
unsigned char min = 0;
unsigned char sec = 0;
unsigned char time_field = 0;
unsigned int edit_time_delay = 0;



void view_dashboard(void)
{
    clcd_print("TIME     E  SP", LINE1(2));

    speed = read_adc(SPEED_ADC_CHANNEL);    // read adc function is called.
    speed = speed / 10.33;                  // speed is divided by 10.33 and result is updated to speed.
    
    
    get_time();                             // get time function is called.
    display_time();                         // display time function is called.
    
    // gear position is printed at line-2 of index 11 and 12.
    clcd_print(gear_mode[gear_pos], LINE2(11));
    
    //speed digit before last digit is displayed at line-2 index 14.
    clcd_putch((speed / 10) + '0', LINE2(14));
    //speed last digit is displayed at line-2 index 15.
    clcd_putch((speed % 10) + '0', LINE2(15));
    
    
    if(on_event_flag == 0)              // when board is switched on and reset is pressed, by default 1st ON event is stored to external eeprom.
    {
        event_store();
        on_event_flag = 1;
    }
       
    
    if(key != 0xFF)
    {
        if(key == MK_SW1)               // switch - 1 pressed.
        {
            if(collision == 1)          // after collision start from gear-neutral.
            {
                collision = 0;
                gear_pos = 0;  
                
                event_store();          // event store function called.
            }
            else if(gear_pos < 6)       // increase gear till gear-6.
            {
                gear_pos++;
                
                event_store();          // event store function called.
            }
        }
        else if(key == MK_SW2)          // switch - 2 pressed.
        {
            if(collision == 1)          // after collision start from gear-neutral.
            {
                collision = 0;
                gear_pos = 0;
                
                event_store();          // event store function called.
            }
            else if(gear_pos > 0)       // decrease gear till gear-neutral.
            {
                gear_pos--;
                
                event_store();          // event store function called.
            }
        }
        else if(key == MK_SW3)          // switch - 3 pressed.
        {
            if(gear_pos != 7)           // if no collision before pressing switch-3 (to store event only if change in gear mode).
            {
                collision = 1;
                gear_pos = 7;

                event_store();          // event store function called.
            }
        }
        else if(key == MK_SW11)         // switch - 11 pressed.
        {
            state = e_main_menu;        // if switch-11 pressed, then enter to main menu from dashboard view.
        }
    }
}



void display_main_menu(void)
{
    if(arrow_pos == 0)                  // if arrow is in line-1.
    {
        clcd_putch(0x7E, LINE1(0));
        clcd_print(" ", LINE2(0));
    }
    else if(arrow_pos == 1)             // if arrow is in line-2.
    {
        clcd_print(" ", LINE1(0));
        clcd_putch(0x7E, LINE2(0));
    }
    
    menu_index = selected_menu_pos - arrow_pos;
    // displays in line-1.
    clcd_print(main_menu_contents[menu_index], LINE1(2));
    // displays in line-2.
    clcd_print(main_menu_contents[menu_index + 1], LINE2(2));
    
    if(key != 0xFF)
    {
        if(key == MK_SW1)               // switch - 1 pressed, then scroll up.
        {
            if(arrow_pos != 0)
            {
                arrow_pos = 0;          // arrow in line-1.
            }

            if(selected_menu_pos > 0)   // from 4 menu options scroll-up.
            {
                selected_menu_pos--;
            }
        }
        else if(key == MK_SW2)          // switch - 2 pressed, then scroll-down.
        {
            if(arrow_pos != 1)
            {
                arrow_pos = 1;          // arrow in line-2.
            }

            if(selected_menu_pos < 3)   // from 4 menu options scroll-down.
            {
                selected_menu_pos++;
            }
        }
        else if(key == MK_SW11)         // switch - 11 pressed, then select and enter to particular arrow pointing menu option.
        {
            state = selected_menu_pos + 2;
        }
        else if(key == MK_SW12)         // switch - 12 pressed, then exit from main menu and go back to view dashboard.
        {
            state = e_dashboard;
            // update values to zero while exiting.
            selected_menu_pos = 0;
            arrow_pos = 0;
        }
    }
}

void event_store(void)
{   
    // get time function called.
    get_time();
    
    for(unsigned char i = 0; i < 11; i++)
    {
        if(i < 8)                       // 8 bytes stored to external eeprom.
        {
            write_external_eeprom(write_addr, time[i]);
        }
        else if(i < 10)                 // 2 bytes stored to external eeprom.
        {
            write_external_eeprom(write_addr, gear_mode[gear_pos][i - 8]);
        }
        else                            // 1 bytes stored to external eeprom.
        {
            write_external_eeprom(write_addr, speed);
        }
        
        write_addr++;                   // increment external eeprom address to write next.
        
        if(write_addr >= 110 )          // after storing 10 event, next event overwrite in place of first event and so on.
        {
            write_addr = 0x00;
        }
    }
    
    event_count++;                      // incremented event count after each event is stored.
}



void event_reader(void)
{   
    if(event_count != 0)                // if minimum of 1 event is stored.
    {
        if (event_count > 10)           // if count is greater than 10, then read address start from last digit of event count multiply by 11 index.
        {
            // (example: 12 % 10 = 2 * 11 = 22 -> starts from event-3 to event-10 and then event-1 and event-2 is displayed last.)
            read_addr = ((event_count % 10) * 11);
        }
        else                            // if event count is less than 11, then read address start from first event address.
        {
            read_addr = 0;
        }

        
        // if event count is greater than or equal to 10, then there are 10 events and if event count is less than 10 then event count number of events present.
        for(unsigned char i = 0; i < ((event_count >= 10) ? 10 : event_count) ; i++)
        {
            // total 11 bytes each event data and two loop to store spaces in between.
            for(unsigned char j = 0; j < 13; j++)
            {
                if(j < 8)               // 8 bytes read.
                {
                    event_logs[i][j] = read_external_eeprom(read_addr);
                }
                else if(j < 9)          // 1 byte stored to string.
                {
                    event_logs[i][j] = ' ';
                }
                else if(j < 11)         // 2 bytes read.
                {
                    event_logs[i][j] = read_external_eeprom(read_addr);
                }
                else if(j < 12)         // 1 byte stored to string.
                {
                    event_logs[i][j] = ' ';
                }
                else if(j == 12)        // 1 byte read.
                {
                    unsigned char temp_speed;
                    temp_speed = read_external_eeprom(read_addr);
                    
                    // stored first digit of speed.
                    event_logs[i][j] = (temp_speed / 10) + '0';
                    // stored second digit of speed.
                    event_logs[i][j + 1] = (temp_speed % 10) + '0';
                }

                // while storing two spaces read address is not incremented.
                if(j != 8 && j != 11)
                {
                    read_addr++;        // increment read address.
                }
                
                if (read_addr >= 110)   // after reading 10th event update next read address to 0x00.
                {
                    read_addr = 0;
                }
            }
            
            // null character stored at end of each string.
            event_logs[i][14] = '\0';
        }
    }
}



void view_log(void)
{   
    if(event_count != 0)
    {
        // event reader function is called.
        event_reader();
        
        // line-1 and line-2 in clcd printed.
        clcd_print("# TIME     E  SP", LINE1(0));               // line-1.
        clcd_putch(view_log_scroll + '0', LINE2(0));            // event index start from zero.
        clcd_putch(' ', LINE2(1));                              // one space after index.
        clcd_print(event_logs[view_log_scroll], LINE2(2));      // time, gear, speed displayed.
    }
    else                                                        // if no events is stored.
    {
        clcd_print("View Log Menu   ", LINE1(0));
        clcd_print("No Log present  ", LINE2(0));
    }
    
    if(key != 0xFF)
    {
        if(key == MK_SW1)               // switch - 1 pressed, then scroll-up.
        {
            if(view_log_scroll > 0)
            {
                view_log_scroll--;
            }
        }
        else if(key == MK_SW2)          // switch - 2 pressed, then scroll-down.
        {
            if(view_log_scroll < ((event_count >= 10) ? 9 : (event_count - 1)))
            {
                view_log_scroll++;
            }
        }
        else if(key == MK_SW12)         // switch - 12 pressed, then exit view logs menu and return to main menu.
        {
            state = e_main_menu;
            view_log_scroll = 0;
        }
    }
}





void download_log(void)
{
    if(event_count != 0)                // when minimum of 1 event is present.
    {
        if(download_delay == 0)         // when delay is 0.
        {
            event_reader();             // event reader function is called.
            
            // print in clcd.
            clcd_print("Downloading logs", LINE1(0));
            clcd_print("To Tera Term ...", LINE2(0));
        }
        else if(download_delay == 49000)  // when delay is 49000, print in Tera Term.
        {
            puts("# TIME     E  SP");
            puts("\r\n");
            // all events printed in tera term.
            for(unsigned char i = 0; i < ((event_count >= 10) ? 10 : event_count); i++)
            {
                putch(i + '0');
                puts(" ");
                puts(event_logs[i]);
                puts("\r\n");
            }
            puts("\r\n\n");
        }
        else if(download_delay == 50000)  // when delay is 50000, re-print in clcd.
        {
            clcd_print("Downloaded logs ", LINE1(0));
            clcd_print("  Successfully  ", LINE2(0));
        }
    }
    else                                // when no event is present, print in clcd.
    {
        if(download_delay == 800)
        {
            clcd_print("No event present", LINE1(0));
            clcd_print("Download failed ", LINE2(0));
            
            puts("No event present, download failed");
            puts("\r\n");
        }
    }
    
    download_delay++;               // increment download delay.
    
    if(download_delay >= 200000)       // when download delay is greater than or equal to 200, then exit download log menu and return to main menu.
    {
        state = e_main_menu;
        download_delay = 0;
    }
}

void clear_log(void)
{
    if(clear_delay == 0)            // when delay is zero.
    {
        if(event_count != 0)        // when minimum of 1 event is present.
        {
            event_count = 0;        // update event count to zero. 
            write_addr = 0;         // update event address to zero (0x00).
        
            // prints in Tera Term.
            puts("Successfully cleared all event logs \r\n\n");
        
            // prints in clcd.
            clcd_print("Cleared all logs", LINE1(0));
            clcd_print("  successfully  ", LINE2(0));
        }
        else                        // when no event is present.
        {
            // prints in Tera Term.
            puts("No event logs present \r\n\n");
        
            // prints in clcd.
            clcd_print(" No event logs  ", LINE1(0));
            clcd_print("    present     ", LINE2(0));
        }
    }
    
    clear_delay++;                  // increment clear delay.
    if(clear_delay >= 100000)       // when clear delay is greater than or equal to 100000, then exit clear log menu and return to main menu.
    {
        state = e_main_menu;
        clear_delay = 0;
    }
}


void set_time(void)
{
    // displays on clcd line-1. 
    clcd_print("    HH:MM:SS    ", LINE1(0));
    // displays on clcd line-2.
    clcd_print("    ", LINE2(0));
    
    // if hour field is selected to edit, then blink that field.
    if(time_field == 2 && edit_time_delay >= 100)
    {
        clcd_putch(' ', LINE2(4));
        clcd_putch(' ', LINE2(5));
        if(edit_time_delay >= 200)
        {
            edit_time_delay = 0;
        }
    }
    else
    {
        clcd_putch((hour / 10) + '0', LINE2(4));
        clcd_putch((hour % 10) + '0', LINE2(5));
    }
    
    clcd_putch(':', LINE2(6));
    
    // if minute field is selected to edit, then blink that field.
    if(time_field == 1 && edit_time_delay >= 100)
    {
        clcd_putch(' ', LINE2(7));
        clcd_putch(' ', LINE2(8));
        if(edit_time_delay >= 200)
        {
            edit_time_delay = 0;
        }
    }
    else
    {
        clcd_putch((min / 10) + '0', LINE2(7));
        clcd_putch((min % 10) + '0', LINE2(8));
    }
    
    clcd_putch(':', LINE2(9));
    
    // if seconds field is selected, then blink that field.
    if(time_field == 0 && edit_time_delay >= 100)
    {
        clcd_putch(' ', LINE2(10));
        clcd_putch(' ', LINE2(11));
        if(edit_time_delay >= 200)
        {
            edit_time_delay = 0;
        }
    }
    else
    {
        clcd_putch((sec / 10) + '0', LINE2(10));
        clcd_putch((sec % 10) + '0', LINE2(11));
    }
    
    // displays space at end of line-2.
    clcd_print("    ", LINE2(12));
    
    
    
    if(key == MK_SW1)               // switch - 1 pressed, then change selected field to edit (0 to 2).
    {
        time_field++;
        if(time_field >= 3)
        {
            time_field = 0;
        }
        edit_time_delay = 0;
    }
    else if(key == MK_SW2)          // switch - 2 pressed, then increase selected field value.
    {
        if(time_field == 0)         // for seconds, 0 to 59 and next again start from 0 and so on.
        {
            sec++;
            if(sec >= 60)
            {
                sec = 0;
            }
        }
        else if(time_field == 1)    // for minutes, 0 to 59 and next again start from 0 and so on.
        {
            min++;
            if(min >= 60)
            {
                min = 0;
            }
        }
        else if(time_field == 2)    // for hours, 0 to 23 and next again start from 0 and so on.
        {
            hour++;
            if(hour >= 24)
            {
                hour = 0;
            }
        }
    }
    else if(key == MK_SW11)         // switch - 11 pressed, then save edited time to RTC and exit from set time logs and return to main menu.
    {
        // write to rtc function is called. 
        write_to_rtc(hour, min, sec);
        
        state = e_main_menu;
        edit_time_delay = 0;
        time_field = 0;
        hour = 0;
        min = 0;
        sec = 0;
    }
    else if(key == MK_SW12)         // switch - 12 pressed, then without saving edited time to RTC, exit from set time logs and return to main menu.
    {
        state = e_main_menu;
        edit_time_delay = 0;
        time_field = 0;
        hour = 0;
        min = 0;
        sec = 0;
    }
    
    
    edit_time_delay++;
}



void write_to_rtc(unsigned char h, unsigned char m, unsigned char s)
{
    // second, minute and hour is written to RTC. 
    for(unsigned char i = 0; i < 3; i++)
    {
        unsigned char val = ((i == 0) ? s : (i == 1) ? m : h);
        unsigned char data = decimal_to_bcd(val);
        
        if(i == 0)
        {
            data = data & 0x7F;         // clear CH bit.
        }
 
        write_ds1307(i, data);          // write ds1307 function is called.
    }
}
 

 
unsigned char decimal_to_bcd(unsigned char val)
{
    // first digit shifted 4 times left and bitwise OR with second digit and result is returned.
    return (((val / 10) << 4) | (val % 10));
}

