// v1.0
// - release of the source code. Tetris without any bells + whistles.
// 
// v1.1
// - Added scorekeeping & highscores. The game will display the last high score at the start.
// (If your score beats the highscore, it will be saved to the EEPROM as the new high score.)
//
// v1.2
// - Sander says: Don't point your PIC at another variable: MPLAB® XC8 C Compiler User’s Guide 5.2.1
//   ARRAY_SIZE removed because of this. This FIXED the randomness of the shape selection
// - Rerouted the displays to entire RA (rather than RA0-3 + RB4-7)
//

#  include <xc.h>

#include <stddef.h>
#include <stdint.h>     // - for uint8_t/uint16_t/int16_t
#include <stdbool.h>    // - for bool/true/false

#include "randgen.h"    //for randomnumber
#include "shared_logic.h"     //for memcpy() and memcpyvol()
#include "config.h"     //for config
#include "shapes.h"     //for tetris shapes
#include "numbers.h"    //for number shapes
#include "EEPROM.h"     //for readhighscore and writehighscore
#include "tetris.h"     //for tetris
#include "snake.h"      //for snake
#include "buttons.h"
#include "screen.h"
#include "interrupt.h"
#include "screen.h"
/// Software configuration
#undef ENABLE_SWDT
#define ENABLE_MUXDISPLAY
#define ENABLE_INTWAIT
#define ENABLE_PICTRIS
#define ENABLE_SNAKE
#undef ENABLE_MARTIJN
#undef ENABLE_DOBBELSTEEN
#undef ENABLE_TEST_SHAPE
#define ENABLE_SANDER_ICD
#undef customRandom

#define highscore
#define RANDOM2

#define _XTAL_FREQ 8000000

// PROGRAM DEFINES
#ifdef ENABLE_MUXDISPLAY
volatile uint16_t ObjectData[8];
volatile uint16_t BackgroundData[8];
uint16_t tmpObjectData[8];
#endif

uint8_t LastHighScore;
// TMR0 variables


//game choosing variable
volatile bool tetris;


#define TMR0_RELOAD (~125)

void __interrupt() isr(void)
{
    if (TMR0IF)
    {
        TMR0IF = 0;         // reset flag
        TMR0 = TMR0_RELOAD; // set reload value to 1 ms (with internal 1MHz crystal)
        Interrupt();
    }
}

// TiMeR0 initialise
void initialise_TMR0(void)
{
    T0PS   = 0x3;   // Prescaler 16, -> (Fosc / 4) / 2 => 8MHz/4 / 16 = 125kHz
    PSA    = 0;     // Prescaler ON,
    T0CS   = 0;     // Source Internal
    TMR0ON = 1;     // TMR0 ON
    TMR0   = TMR0_RELOAD; // reload on 125, so once upon a 1ms
    TMR0IF = 0;     // Clear flag
    TMR0IE = 1;     // Enable TMR0 interrupts
    TMR0ON = 1;     // Enable TMR0
}

void initialise_hardware(void)
{
    // Oscillator settings
    IDLEN = 0; // Don't go to sleep
    IRCF  = 0x7; // 8MHz klok source
    SCS   = 0x2; // Internal clock

    // make all inputs digital
    PCFG  = 0x0F;
 
    // configure I/O
    DDRA  = 0x00; PORTA = 0x00;             // PORTA all output; low
    DDRB  = 0xFF; PORTB = 0x00; RBPU = 0;   // PORTB all input; low; Weak pullup enabled
    DDRC  = 0x00; PORTC = 0x00;             // PORTC all output; low -- upper display
    DDRD  = 0x00; PORTD = 0x00;             // PORTD all output; low -- lower display

    initialise_TMR0();

    IPEN = 0; // Enable all TMR0, RB, INT1 & INT2 interrupts
    GIE = 1;  // Enable all Interrupts
}
 
// initialise the program
void initialise_globals(void)
{
    // intialise variables
    clearArray(ObjectData, 8);
    clearArray(BackgroundData, 8);
    set_mS(0);
    // initialise buttons
}

// Changes global var ObjectData
void splash_screen(void)
{
    if (true){ tetris_screen(); } // 16 = ARRAY_SIZE(TETRIS));
    else {snake_screen();}
    waitms(3500);
}
// this is the main game loop for Tetris
void mainGameLoop(void)
{
    // re-initialise screen and variables (splash screen and highscores may have altered stuff..)
    pauseMultiplexing();
    initialise_globals();
    resumeMultiplexing();
    clearArray(ObjectData,8);
    clearArray(BackgroundData,8);
    clearArray(tmpObjectData,8);
    if (true){
       tetris_main();
    }
    else {
       snake_main();
    }
}

// main program start
void main(void)
{
    // initialise screen and variables
    initialise_hardware();
    initialise_globals();
    start_button();
    initialise_screen();

    // enable screen multiplexing & other timer functions
    // display the flash screen
    choosescreen();
    
    splash_screen();

    // Show last high score from EEPROM
    rnd_initialize(get_mS());

    for (;;)
        // enter main game program, will stay there until an END event occurs
        mainGameLoop();
}
