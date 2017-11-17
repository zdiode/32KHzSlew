;****************************
;*
;*     Slew Rate fix
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
;*     Description:  This is firware is used to just limit the 
;*             slew rate of the clock line to the memory chip.
;*
;*     Processor:   Microchip PIC12C508A
;*
;*     Frequency:   4 MHz internal clock
;*
;*     Last Changed:  November 17, 2017 
;*
;*
;*     1.0     First release
;*     1746a   Changed a line as a GIT experiment.
;*
;*
;****************************
;
;****************************
; Configuration fuses, See page 37
;
#include	"p12C508A.inc"
	__config _MCLRE_OFF & _CP_OFF & _WDT_OFF & _IntRC_OSC
                              ; Internal RC oscillator, watchdog timer off,
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
; none used
;
#define CLK_IN	3             ; clock in on GPIO, pin 4
#define CLK_OUT 1             ; clock out on GPIO, pin 6
;
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
Start:
     movlw    0xba            ; 32:1 prescaler on WDT, TMR0 disabled, Pullups on.
     option
     movlw    0x08            ; all that can be outputs, are.
     tris     GPIO
     btfss    GPIO,CLK_IN     ; start it off
     goto     ClockLow        ; start the clock off low
Loop:
ClockHi:
     bsf      GPIO,CLK_OUT    ; Output clock high
     btfsc    GPIO,CLK_IN     ; skip when clock goes low
     goto     $-1             ; /
;
ClockLow:
     bcf      GPIO,CLK_OUT    ; Output clock low
     btfss    GPIO,CLK_IN     ; skip when clock goes high
     goto     $-1             ; /
     bsf      GPIO,CLK_OUT    ; Output clock high
     goto     Loop
     goto     Start           ; changed as a GIT experiment.  Anyway if program flow got here something bad happened.

	 end
