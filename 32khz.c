//---------------------------------------------------------------------//
// PROJECT IDENTIFICATION                                              //
//---------------------------------------------------------------------//
/* Project            : PS-2 keyboard to Micronet RS-232 converter
 
   Author             : William G. Grimm
                        Avorex Designs
                        12811 Bluhill Road
                        Silver Spring, Md. USA
                        (301) 942-9342 Voice
                        (301) 942-1595 Fax
                        zdiode@avorex.com


   Copyright          : (c) Copyright 2006, Avorex

   Compiler           : Hi-Tech C V7.87pl2 Must have Assemble optimization & Global-6 optimization active
/**/
//---------------------------------------------------------------------//
// FILE IDENTIFICATION                                                 //
//---------------------------------------------------------------------//
/*
   File name          : PS2.C

   File description   : This is the main source code file for a
						the circuit board assembly that will interface
						Micronet RS-232 with a PS-2 Keyboard.

   Creation date      : April 5, 2004

   Last Changed       : Septmber 25, 2006

   Revision history   : Date        Version     Comments
                        ----        -------     --------
						4/7/04		0415g		Removed the time measurement, reception using the clock
						6/17/05		0524a		Added External pull ups, removed internal pull ups.  Keyboard is a stand alone prototype, no PC
						3/27/06		0613a		Added acceptance and removal of key presses
						7/04/06		0627d		Final reverse compatible version, Baud rate changed after this.
						8/7/2006	0631c		Crystal changed to 7.3728MHz, baud rate stays at 9600 baud, this clears
												 a bug that caused caps lock to lock up the keyboard.
						8/18/2006	0633d		Shortened the disconnect repeat quash time from 30 seconds to 3.
						9/25/2006	0639a		Removed ASCII compatible output to make more efficient and compatible with API.
*/

#define VERSION_CODE 0639b

#define CLTYPE	DEFAULT						// for North American, and most other applications
//#define CLTYPE	FRENCH						// for French and some european applications

/*
//---------------------------------------------------------------------//
// To Do    		                                                   //
//---------------------------------------------------------------------//

*/


//---------------------------------------------------------------------//
// Known Bugs                                                          //
//---------------------------------------------------------------------//
/*
	1. During tests It was found that when the Control button is pressed and
		Pause/Break is pressed, the keys returned are *0xE0, 0x14, 0x7e; not the
		sequence expected for Pause/Break.  I believe this to be an artifact
		of the keyboard I was testing with and have chosen not to change it.
		The specification indicates that the control key should not change the
		sequence from the keyboard, but in this case it does.
		Note: without the control key pressed Pause/Break produces 0xe1, 0x14, 0x77,
			0xe1, 0xf0, 0x14, 0xf0, 0x77 as expected.
		Note*: if Pause/Break is pressed with the right control key down, the sequence
			is as stated above.  If the left is pressed, the sequence is 0x14, 0xe0, 0x7e.
*/

/*
//---------------------------------------------------------------------//
// REFERENCE DOCUMENTATION                                             //
//---------------------------------------------------------------------//


//---------------------------------------------------------------------//
// TARGET HARDWARE                                                     //
//---------------------------------------------------------------------//
   PCB description    : Production prototype, 
   PCB number         : K-12a (Revision 3).
   Model number       : 
   Assembly number    : 
   Schematic          : 
   Processor		  : Microchip PIC16F628SA-I/SO
   Clock type		  : HS Oscillator
   Clock speed		  : 7.3728Mhz
*/
/**/
//---------------------------------------------------------------------//
//  OSCILLATOR DESCRIPTION			    							   //
/*---------------------------------------------------------------------//
   For time-based calculations, we will use a 7.3728 Mhz HS clock.
   The hardware prescaler, set in this application via the OPTION register to 16, 
   scales down the instruction cycle clock before passing it on to the
   hardware timer TMR0.  TMR0 counts from 0 to 255, then generates sets the
   overflow flag T0IF.  Setting this flag forms our application's "ticks",
   or the resolution unit of time for timing events.  The tick period is
   calculated as follows:

   cycles per instruction / Estimated clock freq * prescaler units * TMR0 clocks to interrupt
   (       4              /		7372800         )*		16		   *			256

   = 2.22 mS/tick, each instruction is 0.55 uSec
*/
#define XTAL_FREQ 7372800.0
#define TICK_RATE 2.2222
//---------------------------------------------------------------------//
// CONFIGURATION SETTINGS                                              //
//---------------------------------------------------------------------//
#include<pic.h>								// has code for configuration bits
											// and brings in the proper processor header file
#if defined(_16F628) || defined(_16F627)
	__CONFIG(0x2146);			// Code protect off, eecode accessable, LVP off, BOR on, MCLR off, Power up timer on, WDT on, HS oscillator
//	__CONFIG(0x046);			// Code protect on, eecode inaccessable, LVP off, BOR on, MCLR off, Power up timer on, WDT on, HS oscillator
#else
	__CONFIG(0x3ff6);			/* for use with PIC 16F877A,
									no code protect, power up timer is on,
									WDT enabled, HS oscillator */
#endif
     __IDLOC(VERSION_CODE);     /* ID and release number */


//---------------------------------------------------------------------//
// INCLUDE FILES   				  			                           //
//---------------------------------------------------------------------//
#include"pdefine.h"							// constant definitions
#include"pconst.h"							// has constants and structure shared by this Hub and the Micronet terminal.
#if defined(_16F628) || defined(_16F627)
#include"p627prts.h"						// port definitions for final hardware
#else
#include"pports.h"							// port definitions during development
#endif
#include"pmemory.h"							// all global memory
#include"pq.h"								// routines for queueing bytes
#include"pprintq.h"							// queue for printer returns.
void IntInit(void);							// prototype since this is called in a routine pkbdtx.h has in it.
#include"pkbdtx.h"							// routines for transmitting to the keyboard
#include"prs232.h"							// RS232 code
#include"ppackage.h"						// packages both keyboard presses and printer returns for transmission
#include"pkbd.h"							// routines for interpreting scan codes from the keyboard

//---------------------------------------------------------------------//
// IntInit - initialize the interrupts.                                //
//---------------------------------------------------------------------//
void IntInit(void) {
	RCIE = 1;								// enable the RS-232 interrupt
	PEIE = 1;
	INTF = 0;
	INTE = 1;
	INTEDG = 0;								// falling edge

	GIE = TRUE;
} /* IntInit() */

//////////////// END includes ///////////////////////////////////////////

/**/
//---------------------------------------------------------------------//
// FUNCTIONS AND PROCEDURES 									   	   //
//---------------------------------------------------------------------//

#define KbdFault() kbd_state = TIMING_OUT;

//*********************************************************************//
// Interrupt service routine                                           //
//*********************************************************************//
void interrupt service(void) {

	if(KBD_DATA()) 
		newest_bit = 1;						// record the state of the keyboard input

	while(INTF) {
		if(!sendtokeyboard) {
											// not sending, receiving
			INTF = 0;						// clear the interrupt flag
			switch(kbd_state) {
				case PRESS_READY:
					if(newest_bit) {		// check stop bit

						kbdbyteready = kbdbits;	// send it to the run time routine


						kbd_state = AT_REST;// ready for the next
						repeatcount = 0;	// every time we get a good byte from the keyboard, we
											// can reset the between press counter
					} else {
						KbdFault();
											// stop bit not received properly
					}
					break;
				case PARITY_COMING:
					if(newest_bit) {		// check parity
						parityflag = !parityflag;
					}
					if(parityflag) {		// check for odd parity
						kbd_state = PRESS_READY;
					} else {
						KbdFault();
					}
					break;
				case AT_REST:				// start bit is in
					if(!newest_bit) {
						kbd_state = 0;
						parityflag = 0;		// start it clear for the next reception
					} else {
						KbdFault();
					}
					break;
				case TIMING_OUT:			// timing out a bad reception, throw it all away
				case TIMING_OUT_MORE:
					break;
				default:
					kbdbits >>= 1;
					kbdbits |= 0x80;
					if(!newest_bit) {
						kbdbits &= 0x7f;
					} else {
						parityflag = !parityflag;
					}
					kbd_state++;
				break;
			} /* switch(kbd_state) */
		} else {
											// sending to keyboard
			do {
				INTF = 0;					// clear the interrupt flag
				switch(tokbdcount) {
					case 0:
						kbd_state = AT_REST;// ready to start receiving from the keyboard again
						sendtokeyboard = FALSE;
						parityflag = 0;
					break;
					case 1:
						RELEASE_DATA();
					break;
					case 2:
						if(parityflag) {
							KBD_DATA() = 1;	// set the parity bit to make odd parity
						} else {
							KBD_DATA() = 0;	// clear the parity bit to make odd parity
						}
					break;
					default:
						if(tokbd & 1) {		// send one more bit to the keyboard
							KBD_DATA() = 1;
							parityflag = !parityflag;
						} else {
							KBD_DATA() = 0;
						}
						tokbd >>= 1;
					break;
				} /* switch(tokbdcount) */
				tokbdcount--;
			} while(INTF);
		} /* if(!sendtokeyboard) */
	} /* if(INTF) */

	newest_bit = 0;							// ensure it is zero for next time

	if(RCIF) {
		printqStackOutgoing();				// read in the print return to package and send to the Micronet
	}
} /* interrupt Service() */

//*********************************************************************//
// MAIN PROGRAM LOOP		 									   	   //
//*********************************************************************//
void main(void)
{
	SETA_Direction();						// set up the port directions
	SETB_Direction();
	SETC_Direction();
	SETD_Direction();
/*

*/
											/* Set up Timer0, waited for a CLRWDT instruction  */
	OPTION = 0x93;							// TMR0 clocked on instruction clock
											// Port B pull ups off, INT interrupt interrupts on falling edge
											// prescale is 16 and assigned to Timer 0

	tokbdcount = TICKS_PER_SIXTEENTH;
	repeatcount = REPEAT_SNUFF;

	rs232Setup();							// set up the serial port

	kbdasked = FALSE;
	newest_bit = 0;							// start this at clear before the interrupts are enabled
	IntInit();								// start up the interrupts

	packageKeyPress(FIRSTLETTER);			// send an identification.

											// ---> MAIN LOOP <---
	while(TRUE)	{							// main program loop
		CLRWDT();							// WDT for the main loop - occurs in two places, here and the two second TEST start up
											// at least, that was the goal

		if(T0IF) {
			ticks++;
			repeatcount++;
			if(repeatcount < REPEAT_SNUFF) repeatcount++;
			if(repeatcount == REPEAT_SNUFF) {
				unsigned char i;
				for(i = 0; i < NUM_OF_HELD_KEYS; i++) {
					timed_keys[i] = 0;		// purge all keys, the keyboard has disconnected
				} /* for(i = 0; i < NUM_OF_HELD_KEYS; i++) */
				repeatcount++;				// put out of reach, so the purge will not be called constantly.
			} 
			if(ticks >= TICKS_PER_SIXTEENTH) {

				for(ticks = 0; TRMT && ticks < NUM_OF_HELD_KEYS; ticks++) {
											// if the transmitter is not ready, wait for the next 1/16th second opportunity
					if(timed_keys[ticks]) {
						timed_keys[ticks]++;
						if(timed_keys[ticks] == FIRSTREPEAT || timed_keys[ticks] == REPEATCONTROL) {
											// implement the repeat key system, 1/2 second to the first repeat
											// 1/8 second for all subsequint repeats.
							packageKeyPress(active_keys[ticks]);
						}
						if(timed_keys[ticks] >= REPEATCONTROL) {
							timed_keys[ticks] = FIRSTREPEAT;
						}
					} /* if(timed_keys[ticks]) */
				} /* for(ticks = 0; ticks < NUM_OF_HELD_KEYS; ticks++) */

				ticks = 0;					// reset the ticker every 1/16 th second

				if(kbd_state == TIMING_OUT) {
											// check for timing out, error recovery.
					kbd_state++;
				} else {
					if(kbd_state == TIMING_OUT_MORE) {
						kbd_state = AT_REST;
					}
				}

			}
			T0IF = FALSE;
		} /* if(T0IF) */

		if(kbdtxReady()) {
			kbdtxSend(kbdtxGetCode());		// send the code to the keyboard, then return control back to the keyboard
		} /* if(kbdtxReady()) */

		if(TRMT) {
			if(!qKbdEmpty()){
				rs232Send(qKbdGetByte());	// send the byte from the queue to the Micronet
			} else {
				if(!printqEmpty()) {
					packagePrintReturn();
				}
			}
		}

		if(kbdbyteready) {
			repeatindex = kbdbyteready;
			pkbdReadByte(kbdbyteready);
			kbdbyteready = 0;
		} /* if(kbdbyteready) */

	}										// ---> MAIN LOOP END <---
} /* main() */

/*************** End Code ***********************************************************/
