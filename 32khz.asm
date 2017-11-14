;****************************
;*
;*     Oscillator
;*
;*     Version 1.0
;*
;*     Written by:  William G. Grimm
;*
;*
;*     by      Avorex Design, Microchip Consultant's number 904-218
;*             12811 Bluhill Road
;*             Silver Spring, Md. 20906
;*
;*             zdiode@avorex.com
;*             www.avorex.com
;*
;*
;*     Description:  This is firware is used to just run the 
;*             processor as a 32 Khz clock, nothing more.
;*
;*     Processor:   Microchip PIC12C508A
;*
;*     Frequency:   $ MHz internal clock
;*
;*     Last Changed:  May 29, 2007 
;*
;*
;*     1.0     First release
;*
;*
;****************************
;
;****************************
; Configuration fuses, See page 37
;
#include	"p12C508A.inc"
	__config _MCLRE_OFF & _CP_OFF & _WDT_ON & _IntRC_OSC
                              ; Internal RC oscillator, watchdog timer on,
                              ; and code protect on.



;****************************
; Assembler Switches
;****************************
;
;#define SIM                   ; adjustment for simulation
;
;****************************
; Assembler controls
;****************************
     ERRORLEVEL -305          ; eliminate nuisance messages
     ERRORLEVEL -224          ; do not flag the TRIS and OPTION instructions
     ERRORLEVEL -306          ; eliminate nuisance messages
;
;****************************
; Memory
;****************************
#define count 7
;****************************
;*     Reset Vector
;****************************
     ORG      0
     movwf    OSCCAL
     goto     Start
;
;                             leave space for modifying code
;
     org      0xf8
Wait:
     nop                     ; for fine tuning
     nop                     ; for fine tuning
WaitLoop:
     decfsz   count,f
     goto     WaitLoop
     retlw    0
;
Start:
     movlw    0xba            ; 32:1 WDT on, TMR0 disabled, Pullups on
     option
     movlw    0x38
     tris     GPIO
Loop:
     movlw    0xfe            ; Place output ins one state
     movwf    GPIO            ; Pin 7 low pin 6 high
     movlw    2
     movwf    count
     call     Wait
     movlw    0xfd            ; Place output ins one state
     movwf    GPIO            ; Pin 7 high pin 6 low
     movlw    2
     movwf    count
     call     Wait
     movlw    0xfe            ; Place output ins one state
     movwf    GPIO            ; Pin 7 low pin 6 high
     movlw    2
     movwf    count
     call     Wait
     movlw    0xfd            ; Place output ins one state
     movwf    GPIO            ; Pin 7 high pin 6 low
     movlw    2
     movwf    count
     nop
     call     WaitLoop
     goto     Loop

	 end
