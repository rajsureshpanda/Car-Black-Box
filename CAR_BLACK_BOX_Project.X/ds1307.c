


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
#include "ds1307.h"
#include "i2c.h"
#include "clcd.h"


/* 
 * DS1307 Slave address
 * D0  -  Write Mode
 * D1  -  Read Mode
 */


// global variables.
extern unsigned char clock_reg[3];
extern unsigned char time[9];


// initialization of ds1307 function definition.
void init_ds1307(void)
{
	unsigned char dummy;

	/* Setting the CH bit of the RTC to Stop the Clock */
	dummy = read_ds1307(SEC_ADDR);
	write_ds1307(SEC_ADDR, dummy | 0x80); 

	/* Seting 24 Hr Format */
	dummy = read_ds1307(HOUR_ADDR);
	write_ds1307(HOUR_ADDR, dummy & ~(0x40)); 

	/* 
	 * Control Register of DS1307
	 * Bit 7 - OUT
	 * Bit 6 - 0
	 * Bit 5 - OSF
	 * Bit 4 - SQWE
	 * Bit 3 - 0
	 * Bit 2 - 0
	 * Bit 1 - RS1
	 * Bit 0 - RS0
	 * 
	 * Seting RS0 and RS1 as 11 to achive SQW out at 32.768 KHz
	 */ 
	write_ds1307(CNTL_ADDR, 0x93); 

	/* Clearing the CH bit of the RTC to Start the Clock */
	dummy = read_ds1307(SEC_ADDR);
	write_ds1307(SEC_ADDR, dummy & 0x7F); 
}

// write to ds1307 function definition.
void write_ds1307(unsigned char address, unsigned char data)
{
	i2c_start();
	i2c_write(SLAVE_WRITE);
	i2c_write(address);
	i2c_write(data);
	i2c_stop();
}

// read from ds1307 function definition.
unsigned char read_ds1307(unsigned char address)
{
	unsigned char data;

	i2c_start();
	i2c_write(SLAVE_WRITE);
	i2c_write(address);
	i2c_rep_start();
	i2c_write(SLAVE_READ);
	data = i2c_read();
	i2c_stop();

	return data;
}

// display time function definition.
void display_time(void)
{
	clcd_print(time, LINE2(2));

	if (clock_reg[0] & 0x40)
	{
		if (clock_reg[0] & 0x20)
		{
			clcd_print("PM", LINE2(0)); // 12
		}
		else
		{
			clcd_print("AM", LINE2(0)); // 12
		}
	}
}

// get time function definition.
void get_time(void)
{
	clock_reg[0] = read_ds1307(HOUR_ADDR);
	clock_reg[1] = read_ds1307(MIN_ADDR);
	clock_reg[2] = read_ds1307(SEC_ADDR);

	if (clock_reg[0] & 0x40)
	{
		time[0] = '0' + ((clock_reg[0] >> 4) & 0x01);
		time[1] = '0' + (clock_reg[0] & 0x0F);
	}
	else
	{
		time[0] = '0' + ((clock_reg[0] >> 4) & 0x03);
		time[1] = '0' + (clock_reg[0] & 0x0F);
	}
	time[2] = ':';
	time[3] = '0' + ((clock_reg[1] >> 4) & 0x0F);
	time[4] = '0' + (clock_reg[1] & 0x0F);
	time[5] = ':';
	time[6] = '0' + ((clock_reg[2] >> 4) & 0x0F);
	time[7] = '0' + (clock_reg[2] & 0x0F);
	time[8] = '\0';
}

