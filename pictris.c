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

#ifdef SDCC // Include register vars
#  include <pic18fregs.h>
#elif defined __XC8
#  include <xc.h>
#else
#  error make the proper include for the regs-defs
#endif

#include <stddef.h>
#include <stdint.h>     // - for uint8_t/uint16_t/int16_t
#include <stdbool.h>    // - for bool/true/false
#include <string.h>     // - for memcpy()/memmove()
#include <limits.h>     // - for SHRT_MAX

#include "randgen.h"

/// Software configuration
#undef ENABLE_SWDT
#define ENABLE_DEBOUNCE
#define ENABLE_MUXDISPLAY
#define ENABLE_INTWAIT
#define ENABLE_PICTRIS
#undef ENABLE_MARTIJN
#undef ENABLE_DOBBELSTEEN
#undef ENABLE_TEST_SHAPE
#define ENABLE_SANDER_ICD
#undef customRandom

#define snake

#define highscore

typedef uint8_t byte;

//#define ARRAY_SIZE(a) ((sizeof a) * (sizeof a[0])) // Do not use: see MPLAB® XC8 C Compiler User’s Guide 5.2.1

// Bit manipulation
#define SBIT(reg,bit)	reg |=  (1<<bit)    // Macro defined for Setting  a bit of any register.
#define CBIT(reg,bit)	reg &= ~(1<<bit)    // Macro defined for Clearing a bit of any register.
#define GBIT(reg,bit)   reg &   (1<<bit)    // Macro defined for Getting  a bit of any register.
#define TBIT(reg,bit)   reg ^=  (1<<bit)    // Macro defined for Toggling a bit of any register.
#define VBIT(reg,bit,v) reg  = (reg & ~(1<<bit)) | ((v&1)<<bit)    // Macro defined for setting  a bit of any register to `v'


#include "shapes.h"
#include "numbers.h"


#ifdef SDCC // Configuration bits
#warning TODO: config here
#elif defined __XC8
// CONFIG1H
#  pragma config OSC = INTIO67    // Oscillator Selection bits (Internal oscillator block, port function on RA6 and RA7) == max. 8 MHz
#  pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
#  pragma config IESO = OFF       // Internal/External Oscillator Switchover bit (Oscillator Switchover mode disabled)

// CONFIG2L
#  pragma config PWRT = OFF       // Power-up Timer Enable bit (PWRT disabled)
#  pragma config BOREN = SBORDIS  // Brown-out Reset Enable bits (Brown-out Reset enabled in hardware only (SBOREN is disabled))
#  pragma config BORV = 3         // Brown Out Reset Voltage bits (Minimum setting)

// CONFIG2H
#  pragma config WDT = OFF        // Watchdog Timer Enable bit (WDT disabled)
#  pragma config WDTPS = 32768    // Watchdog Timer Postscale Select bits (1:32768)

// CONFIG3H
#  pragma config CCP2MX = PORTC   // CCP2 MUX bit (CCP2 input/output is multiplexed with RC1)
#  pragma config PBADEN = OFF     // PORTB A/D Enable bit (PORTB<4:0> pins are configured as digital I/O on Reset)
#  pragma config LPT1OSC = OFF    // Low-Power Timer1 Oscillator Enable bit (Timer1 configured for higher power operation)
#  pragma config MCLRE = ON       // MCLR Pin Enable bit (MCLR pin enabled; RE3 input pin disabled)

// CONFIG4L
#  pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#  pragma config LVP = OFF        // Single-Supply ICSP Enable bit (Single-Supply ICSP disabled) --> RB5 is I/O
#  pragma config XINST = OFF      // Extended Instruction Clear Enable bit (Instruction set extension and Indexed Addressing mode disabled)

// CONFIG5L
#  pragma config CP0 = OFF        // Code Protection bit (Block 0 (000800-001FFFh) not code-protected)
#  pragma config CP1 = OFF        // Code Protection bit (Block 1 (002000-003FFFh) not code-protected)
#  pragma config CP2 = OFF        // Code Protection bit (Block 2 (004000-005FFFh) not code-protected)
#  pragma config CP3 = OFF        // Code Protection bit (Block 3 (006000-007FFFh) not code-protected)

// CONFIG5H
#  pragma config CPB = OFF        // Boot Block Code Protection bit (Boot block (000000-0007FFh) not code-protected)
#  pragma config CPD = OFF        // Data EEPROM Code Protection bit (Data EEPROM not code-protected)

// CONFIG6L
#  pragma config WRT0 = OFF       // Write Protection bit (Block 0 (000800-001FFFh) not write-protected)
#  pragma config WRT1 = OFF       // Write Protection bit (Block 1 (002000-003FFFh) not write-protected)
#  pragma config WRT2 = OFF       // Write Protection bit (Block 2 (004000-005FFFh) not write-protected)
#  pragma config WRT3 = OFF       // Write Protection bit (Block 3 (006000-007FFFh) not write-protected)

// CONFIG6H
#  pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-3000FFh) not write-protected)
#  pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot block (000000-0007FFh) not write-protected)
#  pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write-protected)

// CONFIG7L
#  pragma config EBTR0 = OFF      // Table Read Protection bit (Block 0 (000800-001FFFh) not protected from table reads executed in other blocks)
#  pragma config EBTR1 = OFF      // Table Read Protection bit (Block 1 (002000-003FFFh) not protected from table reads executed in other blocks)
#  pragma config EBTR2 = OFF      // Table Read Protection bit (Block 2 (004000-005FFFh) not protected from table reads executed in other blocks)
#  pragma config EBTR3 = OFF      // Table Read Protection bit (Block 3 (006000-007FFFh) not protected from table reads executed in other blocks)

// CONFIG7H
#  pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot block (000000-0007FFh) not protected from table reads executed in other blocks)
#else
#  error Make proper configuration here
#endif

// Used for timing
#define _XTAL_FREQ 8000000

// PROGRAM DEFINES
#ifdef ENABLE_MUXDISPLAY
volatile uint16_t ObjectData[8];
volatile uint16_t BackgroundData[8];
uint16_t tmpObjectData[8];
#endif

// button definitions
#ifdef ENABLE_MARTIJN
#  define Rotate PORTBbits.RB0
#  define Left   PORTBbits.RB1
#  define Right  PORTBbits.RB2
#  define Down   PORTBbits.RB3
#else
#  define Left   PORTBbits.RB0
#  define Right  PORTBbits.RB1
#  define Rotate PORTBbits.RB2
#  define Down   PORTBbits.RB3
#endif

// Should be there, but are not in SDCC-3.4.0 or in XC8
#define TMR0IF  INTCONbits.TMR0IF
#define T0PS    T0CONbits.T0PS

#define TMR2IF  PIR1bits.TMR2IF
#define TMR2ON  T2CONbits.TMR2ON
#define TMR2IE  PIE1bits.TMR2IE

#define IDLEN   OSCCONbits.IDLEN
#define SCS     OSCCONbits.SCS
#define IRCF    OSCCONbits.IRCF

#define RBPU    INTCON2bits.RBPU

#define PCFG    ADCON1bits.PCFG

#ifdef ENABLE_DEBOUNCE
volatile bool mtxButtons;

volatile
        int16_t Left_Delay,     Right_Delay,     Rotate_Delay,     Down_Delay;
volatile
        bool Left_Debounced, Right_Debounced, Rotate_Debounced, Down_Debounced;

#  define DEBOUNCE_DELAY   20       // 10 ms
#  define REPETITION_DELAY 500      // 500 ms
#endif

#ifdef ENABLE_PICTRIS
byte LimitedRotation;
byte NumberOfLines;
byte LastHighScore;

bool IsRotated;

bool EndOfGame;
bool CheckForNewLines;

volatile bool DropObject;
#endif
byte OriginX, OriginY;

// used for random graphic selection
#define RND_WINDOW       (55/NUMBER_OF_SHAPES)

// TMR0 variables
volatile uint16_t mS;                    // mS register
//volatile byte     CurrentX;


#ifdef ENABLE_PICTRIS
// TEXT
const uint16_t TETRIS[] = 
{
    0x8841, // *   *    *     *,
    0xFBDF, // ***** **** *****,
    0x8841, // *   *    *     *,
    0x0000, //                 ,
    0x9BDF, // *  ** **** *****,
    0xA955, // * * *  * * * * *,
    0xCA91, // **  * * *  *   *,
    0x0000, //
};

const uint16_t SNAKE[] =
{
    0xFB97,
    0xA955,
    0xAB9D,
    0x0000,
    0x03DF,
    0xB881,
    0x035F,
    0x0000, 
};


typedef enum {
    CCW,
    CW
} rotation_t;
#endif

#ifdef snake
const uint16_t choose_screen[] =
{
    0xFFFF,
    0x8181,
    0xBD81,
    0x858D,
    0x8599,
    0xAD81,
    0x8181,
    0xFFFF,
};
#endif

typedef enum {
    DOWN,
    UP,
    LEFT,
    RIGHT
} direction_t;

typedef enum {
    OVERRIDE,
    MERGE,
    INVERT
} mode_t;

#ifdef snake
const uint16_t SnakeYtext[] = {0x001,0x0002 ,0x0004 ,0x0008 ,0x0010 ,0x0020 ,0x0040 ,0x0080 ,0x0100 ,0x0200 ,0x0400 ,0x0800 ,0x1000 ,0x2000 ,0x4000 ,0x8000 };

struct position{
    uint8_t x;
    uint8_t y;
};
uint8_t direction;
bool moveSnake;
struct position positions[100];
uint8_t snakeLength;
uint8_t berryX;
uint8_t berryY;
uint8_t previous_direction;
#endif
bool tetris;

#ifdef customRandom
int buttonPressed __at() ;
#endif

#define TMR0_RELOAD (~125)

#ifdef SDCC
  __sfr16 __at (0x0F820F83) port_display;
#elif defined __XC8
  extern volatile unsigned short port_display __at(0xF82);
  asm("port_display equ 0F82h");
#else
#  warning Make port_display happen on PORTC_ADDR & PORTD_ADDR
#endif

/// Should be there, but are not
// Taken from: http://www.geeksforgeeks.org/implement-itoa/

/* A utility function to swap two bytes */
void swap(char* a, char* b)
{
    char t = *a;
    *a = *b;
    *b = t;
}

/* A utility function to reverse a string  */
void reverse(char str[], int length)
{
    int start = 0;
    int end = length -1;
    while (start < end)
    {
        swap(str+start, str+end);
        start++;
        end--;
    }
}

char * itoa(int value, char * str, int bas)
{
    int i = 0;
    bool isNegative = false;
 
    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (value == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
 
    // In standard itoa(), negative numbers are handled only with 
    // base 10. Otherwise numbers are considered unsigned.
    if (value < 0 && bas == 10)
    {
        isNegative = true;
        value = -value;
    }
 
    // Process individual digits
    while (value != 0)
    {
        int rem = value % bas;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        value = value/bas;
    }
 
    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';
 
    str[i] = '\0'; // Append string terminator
 
    // Reverse the string
    reverse(str, i);
 
    return str;
}

#ifdef ENABLE_DEBOUNCE
void pauseButtons()
{
    //while (!mtxButtons)
    //    continue;
    mtxButtons = false;
}

void resumeButtons()
{
    mtxButtons = true;
}

void debounceButton(volatile bool button, volatile int16_t *delay, volatile bool *debounced)
{
    if (button == 0) // Button pressed
    {
        if (*delay < 0)
            *delay = DEBOUNCE_DELAY;
        else if ((*delay)-- == 0)
            *debounced = true;
    }
    else
    {
        *delay = -1;
        *debounced = false;
    }
}
#endif

#ifdef ENABLE_SWDT
volatile byte     sWDT;                  // Software Watch Dog Timer for entering SLEEP mode
void goSleep(void)
{
    sWDT++;
    if (sWDT == 30) // Gets reset in the mainGameLoop();
    {
        IDLEN = 1;      // Put the PIC in RC_IDLE mode on SLEEP
        SCS   = 0x02;   // Internal oscillator
        DDRA  = 0xFF;   // Make all I/Os high impedance inputs
        DDRB  = 0xFF;
        DDRC  = 0xFF;
        DDRD  = 0xFF;
        //DDRE?
        TMR2ON = 0;     // Disable TMR2
        // Configure oscillator for 31Khz
        IRCF = 0;
        Sleep();
    }
}
#endif // ENABLE_SWDT

#ifdef ENABLE_INTWAIT
volatile unsigned time;
void waitms(unsigned t)
{
    time = t;
    while(time)
        continue;		// time is decremented in timer0interrupt
}
#endif

#ifdef ENABLE_MUXDISPLAY
volatile bool UpdateScreen;
volatile bool InterruptComplete;

// Loop until an interrupt occurs.
// Useful for semaphores - allows shared resources to be used with little (no) impact on timing
void waitForInterrupt(void)
{
    InterruptComplete = false;
    while (!InterruptComplete)
        continue;
}

// This is the primary semaphore routine. If the program uses shared resources, this routine is called.
void pauseMultiplexing(void)
{
    if (UpdateScreen)
    {
        waitForInterrupt();
        UpdateScreen = false;
        //port_display = 0; // blank the screen
    }
}

// Enables multiplexing to resume
void resumeMultiplexing(void)
{
    UpdateScreen = true;
}
#endif

#ifdef SDCC
void isr(void) __interrupt
#else
void __interrupt() isr(void)
#endif
{
    if (TMR0IF)
    {
        TMR0IF = 0;         // reset flag
        TMR0 = TMR0_RELOAD; // set reload value to 1 ms (with internal 1MHz crystal)

#ifdef ENABLE_INTWAIT
        time--;             // variable for waitms()
#endif
        
        if (++mS >= 1000)
        {
            mS = 0;
#ifdef ENABLE_SWDT
            goSleep();
#endif
        }

#ifdef ENABLE_DEBOUNCE
        // Button Debounces
        // Ensures the button has been activated before performing a debounce.
        // Interrupt driven, the user has X mS between button presses.
        // Prevents double presses and and minimize the chance of switch chatter.
        // The sub routine 'CheckButtons' controls the state of various flags here.
        // check if buttons require debounce
        if (mtxButtons) // Is already claimed by the mainGameLoop()
        {
            debounceButton(Left,   &Left_Delay,   &Left_Debounced  );
            debounceButton(Right,  &Right_Delay,  &Right_Debounced );
            debounceButton(Rotate, &Rotate_Delay, &Rotate_Debounced);
            debounceButton(Down,   &Down_Delay,   &Down_Debounced  );
        }
#endif

#ifdef ENABLE_PICTRIS
        // Event Handler - Object Drop
        // Once every 800mS, a flag is set to initiate the next object drop.
        // This is one of few classic features of tetris
        if (tetris == true){
        DropObject |= mS == 800;}
#ifdef snake
        else if(mS % 200 == 0 ){
        moveSnake = true;
        }
#endif
#endif

#ifdef ENABLE_MUXDISPLAY
        // Displays content on 64 LEDs via multiplexing
        // Ensure multiplexing is enabled.
        // This is vital to protect shared variables such as ObjectData and BackgroundData.
        // Whenever a write to either is done outside of the interrupt, multiplexing should be disabled.
        if (UpdateScreen)
        {
            static byte CurrentX = 0;

            uint8_t  xmask = 1 << CurrentX;
            uint16_t ymask = ObjectData[CurrentX] | BackgroundData[CurrentX];

            port_display = 0;
#ifdef ENABLE_SANDER_ICD
            PORTA        = xmask;
#else
            PORTA        = (PORTA & 0xf0) | (xmask & 0x0f);
            PORTB        = (PORTB & 0x0f) | (xmask & 0xf0);
#endif
            port_display = ~ymask;

            // increment x axis, ready for the next multiplex
            CurrentX++;
            CurrentX &= 0x07; // wrap around after 8 states
            
            // a general purpose flag to inidcate the completion of an interrupt
            //InterruptComplete = true; // uit de if gehaald op 10-12-2015 na debuggen
        }
        InterruptComplete = true;
#endif
    }
    
}

// clears the passed array of type Word
void clearArray(volatile uint16_t *pArray, size_t size)
{
    byte i;
    for (i = 0; i < size; i++)
        pArray[i] = 0;
}

#if defined ENABLE_PICTRIS | defined ENABLE_TEST_SHAPE
// select a random object based on the number of objects defined
void selectNextObject(volatile uint16_t *pTarget)
{
    byte rndSelection, counter, selection;
    struct shape * s;

    // create a random number from 1-255
#ifdef customRandom
    rndSelection = rnd_get_namm(buttonPressed);
#else
    rndSelection = rnd_get_num();
#endif
    // make a selection based on the random number created
    counter = 0;
    selection = 0;
    do
    {
        counter += RND_WINDOW;
        selection++;
    } while (counter < rndSelection);
    //selection = rndSelection;

    // initalise object variables
    s = &shapes[selection % NUMBER_OF_SHAPES];
    memcpy(pTarget, s->graphic, 16); // 16 = ARRAY_SIZE(s->graphic));
    OriginX = s->x;
    OriginY = s->y;
#ifdef ENABLE_PICTRIS
    IsRotated = false;
    LimitedRotation = s->limitedRotation;
#endif
    mS = 0; // reset the timer
}
#endif
#ifdef snake
uint8_t genBerry(uint8_t size)
{
    uint8_t temp;
    temp = rnd_get_num();
    temp = temp % size;
    return temp;
}
void CreateBerry()
{
    bool randomGood;
    uint8_t i;
    randomGood = 1;
    do
    {
        randomGood = 0;
        berryX = genBerry(8);
        berryY = genBerry(16);
        for(i=0;i<snakeLength;i++)
        {
            if (berryX == positions[i].x && berryY == positions[i].y)
            {
                randomGood = 1;
            }
        }
    } while (randomGood == 1);
}

void SnakeGraph()
{
    uint8_t i;
    uint8_t j;
    clearArray(tmpObjectData, 8);
    for(i=0;i<=snakeLength;i++){
        j = positions[i].x;
        tmpObjectData[j] = (SnakeYtext[positions[i].y] | tmpObjectData[j]);
        
        
        
        //for(j=0;j<8;j++){
        //    if (positions[i].x == j) {
        //        ObjectData[j] = (SnakeYtext[positions[i].y] | ObjectData[j]);
        //    }
        //}
    }
    for(i=0;i<8;i++){
        if (berryX == i){
            tmpObjectData[i] = SnakeYtext[berryY] | tmpObjectData[i];
        }
    }
    pauseMultiplexing();
    clearArray(ObjectData, 8);
    for(i=0;i<8;i++){
        ObjectData[i] = tmpObjectData[i];
    }
    resumeMultiplexing();
}

void MoveSnakes()
{
    uint8_t i;
    switch (direction){
        case 0:
            positions[0].y = positions[0].y - 1;
            previous_direction = 0;
            if (positions[0].y >= 200){
                positions[0].y = 15;
            };
            break;
        case 1:
            positions[0].x = positions[0].x + 1;
            previous_direction = 1;
            if (positions[0].x > 7 & positions[0].x < 200){
                positions[0].x = 0;
            };
            break;
        case 2:
            positions[0].y = positions[0].y + 1;
            previous_direction = 2;
            if (positions[0].y > 15 & positions[0].y < 200) {
                positions[0].y = 0;
            };
            break;
        case 3:
            positions[0].x = positions[0].x - 1;
            previous_direction = 3;
            if (positions[0].x >= 200){ 
                positions[0].x = 7;
            };
            break;
    };
    for (i=0;i<=snakeLength;i++){
        positions[(snakeLength - i)+1].y = positions[snakeLength-i].y;
        positions[(snakeLength - i)+1].x = positions[snakeLength-i].x; 
    }
    if (positions[0].y == berryY && positions[0].x == berryX){
        snakeLength = snakeLength + 1;
        CreateBerry();
    }
    for (i=4;i<=snakeLength;i++){
        if (positions[0].y == positions[i].y && positions[0].x == positions[i].x){
            EndOfGame = 1;
        };
    };
    SnakeGraph();
}
#endif

// Merge two arrays with the selected mode
void mergeObjects(volatile uint16_t * pSource, volatile uint16_t * pTarget, mode_t mode)
{
    byte i;

    switch (mode)
    {
    case OVERRIDE:
        for (i = 0; i < 8; i++)
            pTarget[i] = pSource[i];
        break;
    case MERGE:
        for (i = 0; i < 8; i++)
            pTarget[i] |= pSource[i];
        break;
    case INVERT:
        for (i = 0; i < 8; i++)
            pTarget[i] ^= pSource[i];
        break;
    }
}

// Move an objects x axis in pDirection
void moveObject(volatile uint16_t * pObject, direction_t direction, byte cycles)
{
    byte i, c;

    switch (direction)
    {
    case DOWN:
        for (c = 0; c < cycles; c++)
        {
            for (i = 0; i < 8; i++)
                pObject[i] <<= 1;
            OriginY++;
        }
        break;
    case UP:
        for (c = 0; c < cycles; c++)
        {
            for (i = 0; i < 8; i++)
                pObject[i] >>= 1;
            OriginY--;
        }
        break;
    case RIGHT:
        for (c = 0; c < cycles; c++)
        {
            for (i = 7; i > 0; i--)
                pObject[i] = pObject[i-1];
            pObject[0] = 0;
            OriginX++;
        }
        break;
    case LEFT:
        for (c = 0; c < cycles; c++)
        {
            for (i = 0; i < 7; i++)
                pObject[i] = pObject[i+1];
            pObject[7] = 0;
            OriginX--;
        }
    }
}

#ifdef ENABLE_PICTRIS
// check the passed object to see if it has reached the bottom
// return true if so
bool checkForBottom(volatile uint16_t * pObject)
{
    for (byte i = 0; i < 8; i++)
        if (GBIT(pObject[i], 15))
            return true;
    return false;
}

// check the passed object to see if it is against the left wall already
// return true if so
bool checkForLeftWall(volatile uint16_t * pObject)
{
    return pObject[0] != 0;
}

// check the passed object to see if it is against the right wall already
// return true if so
bool checkForRightWall(volatile uint16_t * pObject)
{
    return pObject[7] != 0;
}

// compare the passed source and target for a collision
// return true if so
bool collisionDetect(volatile uint16_t * pSource, volatile uint16_t * pTarget)
{
    for (byte i = 0; i < 8; i++)
        if (pSource[i] & pTarget[i]) // if two bits overlap, this is true
            return true;
    return false; // no bits overlap, no collision
}

// An extremely quick way to rotate objects +90/-90 degrees. 
//
// Notes:
// Work out the centre of the block (to be used as a pivot point), i.e. the centre of the block shape. Call that (px, py).
// Each brick that makes up the block shape will rotate around that point. For each brick, you can apply the following calculation.
// Where each brick's width and height is q, the brick's current location (of the upper left corner) is (x1, y1) and the new brick location is (x2, y2):
// x2 = (y1 + px - py)
// y2 = (px + py - x1 - q)
// To rotate the opposite direction:
// x2 = (px + py - y1 - q)
// y2 = (x1 + py - px)
//
// Based on a 2D affine matrix transformation.
void newRotation(volatile uint16_t * pSource, uint16_t * pTarget, rotation_t rotation)
{
    int8_t x2, y2;
    byte x1, y1;

    // check to see if rotations are disabled for current object
    if (LimitedRotation == 2)
        memcpy(pTarget, pSource, 16); // 16 = ARRAY_SIZE(pSource) = 8 * sizeof uint16_t
    else
    {
        // clear the target array
        clearArray(pTarget, 8);

        // if the object is limited by rotation, then reverse
        // rotations 1 & 3. This will make the object turn like so
        // CW, CCW, CW, CCW
        if (LimitedRotation == 1 && IsRotated)
            rotation = CCW;
        // analyse each pixel (working with 5x5 graphic)
        for (x1 = 0; x1 < 8; x1++)
            for (y1 = 0; y1 < 16; y1++)
            {
                if (GBIT(pSource[x1], y1))
                {
                    if (rotation == CW)
                    {
                        x2 = OriginX + OriginY - y1;
                        y2 = x1 + OriginY - OriginX;
                    }
                    else
                    {
                        x2 = y1 + OriginX - OriginY;
                        y2 = OriginX + OriginY - x1;
                    }
                    if (x2 >= 0 && x2 < 8 && 
                        y2 >= 0 && y2 < 16)
                        SBIT(pTarget[x2], y2);    //pDestinationObject(GetIndexY(Int(Y2)), GetIndexX(Int(X2))) = 1
                }
            }
    }
}

// Count the number of pixels that equal "1". Standard bit-wise operators did not suit
// this routine as pixels could very well end up ANYWHERE after a rotation.
byte pixelCount(volatile uint16_t * pSource)
{
    byte r = 0, x, y;

    for (x = 0; x < 8; x++)
        for (y = 0; y < 16; y++)
            if (GBIT(pSource[x], y))
                r++;
    return r;
}

// Drop the object by one Y increment. Return FALSE if failed.
bool moveObjectDown(volatile uint16_t * pObject)
{
    bool result = true;
#ifdef ENABLE_MUXDISPLAY
    // Semaphore, to protect shared variables
    pauseMultiplexing();
#endif
    // check for a collision with the bottom of the screen
    if (checkForBottom(pObject))
    {
        // ... yes it has - time to save it there
        result = false;
        // OR the object into the background
        mergeObjects(pObject, BackgroundData, MERGE);
        // get a new object up-and-running
        selectNextObject(pObject);
        // check for new lines
        CheckForNewLines = true;
    }
    else
    {
        // Move the object down
        moveObject(pObject, DOWN, 1);
        // check for a collision with the background
        if (collisionDetect(pObject, BackgroundData))
        {
            // the object HIT something in the background. The object needs to be 
            // moved BACK UP and saved to the background.
            result = false;
            // move the object up one
            moveObject(pObject, UP, 1);
            // move the object to the background (OR it over)
            mergeObjects(pObject, BackgroundData, MERGE);
            // get a new object up-and-running
            selectNextObject(pObject);
            // check if the object has collided with the background already. If yes, then
            // the game is over.
            EndOfGame = collisionDetect(pObject, BackgroundData);
            // it's always possible that new lines may have been made already
            CheckForNewLines = true;
        }
    }
    mS = 0; // reset the timer because of the moveDown()
    // resume multiplexing, shared resources are done with
#ifdef ENABLE_MUXDISPLAY
    resumeMultiplexing();
#endif
    return result;
}
 
// Check the state of each button. Because this routine is called often, the chances of missing a button press
// are very unlikely. 
void checkButtons(void)
{
    // check if left button pressed
    if (Left_Debounced)
    {
    pauseButtons();
        Left_Debounced = false;
        Left_Delay = REPETITION_DELAY; // repetition delay
    resumeButtons();
//        Left_Delay = DEBOUNCE_DELAY;
        // ensure the object is not ALREADY against the left wall
    if (tetris == true){
        if (!checkForLeftWall(ObjectData))
        {
            // move the object to a temporary buffer
            //mergeObjects(ObjectData, tmpObjectData, OVERRIDE);
            memcpy(tmpObjectData, ObjectData, 16);
            // Decrement the x axis
            moveObject(tmpObjectData, LEFT, 1);
            // ensure no collisions with background
            if (!collisionDetect(tmpObjectData, BackgroundData))
            {
#ifdef ENABLE_MUXDISPLAY
                // semaphore - shared variables are about to be accessed
                pauseMultiplexing();
#endif
                // all went well, merge the array to the working buffer
                mergeObjects(tmpObjectData, ObjectData, OVERRIDE);// TODO: is moveObject(ObjectData) niet sneller
                // enable the debounce timer (prevents double presses etc)
#ifdef ENABLE_MUXDISPLAY
                resumeMultiplexing();
#endif
                // always a chance that a new line was just made
                CheckForNewLines = true;
            }
        }
    }
#ifdef snake
    else if (previous_direction != 1){ direction = 3 ;}
#endif
    }
    // check if the right button is pressed
    // NOTE: THESE COMMENTS ARE MUCH THE SAME AS ABOVE, excluded for that reason
    if (Right_Debounced)
    {
    pauseButtons();
        Right_Debounced = false;
        Right_Delay = REPETITION_DELAY;
    resumeButtons();
//        Right_Delay = DEBOUNCE_DELAY;
        // ensure not already on the right wall
    if (tetris == true) {
        if (!checkForRightWall(ObjectData))
        {
            mergeObjects(ObjectData, tmpObjectData, OVERRIDE);
            moveObject(tmpObjectData, RIGHT, 1);
            // ensure no collisions with objects
            if (!collisionDetect(tmpObjectData, BackgroundData))
            {
#ifdef ENABLE_MUXDISPLAY
                pauseMultiplexing();
#endif
                mergeObjects(tmpObjectData, ObjectData, OVERRIDE);
#ifdef ENABLE_MUXDISPLAY
                resumeMultiplexing();
#endif
                CheckForNewLines = true;
            }
        }
    }
#ifdef snake
    else if (previous_direction != 3){ direction = 1 ;}
#endif
    }
    // check if rotate button is pressed 
    if (Rotate_Debounced)
    {
    pauseButtons();
        Rotate_Debounced = false;
        Rotate_Delay = REPETITION_DELAY;
    resumeButtons();
        // new rotation method, named accordingly, more info there
    if (tetris == true ){
        newRotation(ObjectData, tmpObjectData, CW);
        // ensure nothing has fallen off due to near-wall rotations
        if (pixelCount(ObjectData) == pixelCount(tmpObjectData))
        {
            // check for object collision due to rotation
            if (!collisionDetect(tmpObjectData, BackgroundData))
            {
                // FINALLY, if it gets here then ALL WENT WELL.
#ifdef ENABLE_MUXDISPLAY
                // semaphore - shared variables are about to be accessed
                pauseMultiplexing();
#endif
                // Temp buffer merged with working buffer
                mergeObjects(tmpObjectData, ObjectData, OVERRIDE);
#ifdef ENABLE_MUXDISPLAY
                resumeMultiplexing();
#endif
                IsRotated = !IsRotated;
            }
        }
    }
#ifdef snake
    else if (previous_direction != 2){ direction = 0 ;}
#endif
    }
    // check if down is pressed
    if (Down_Debounced)
    {
        // move the current object straight to the bottom 
        if (tetris == true) {
        while (moveObjectDown(ObjectData))
            continue; } 
#ifdef snake
        else if (previous_direction != 0){ direction = 2 ;};
#endif
    pauseButtons();
        Down_Debounced = false;
        Down_Delay = SHRT_MAX; // I see you in 33 seconds
    resumeButtons();
    }
}
 
// remove the line at location pY
void removeLine(volatile uint16_t * pObject, byte pY)
{
    byte x, y, currentLine;
 
    // shift each line down
    for (y = pY-1; y < pY; y--) // y < pY --> y should reach 0 and than wraparound, so this will become true
    {
        currentLine = y + 1;
        for (x = 0; x < 8; x++)
            VBIT(pObject[x], currentLine, ((pObject[x] >> y) & 1));

    }
    // clear the top line
    for (x = 0; x < 8; x++)
        CBIT(pObject[x], 0);
}
 
// remove COMPLETED lines & return the number of lines found
void checkForLines(volatile uint16_t * pObject)
{
    byte x, y, pixels;
 
    for (y = 0; y < 16; y++)
    {
        pixels = 0;
        for (x = 0; x < 8; x++)
            if (GBIT(pObject[x], y))
                pixels++;
        if (pixels == 8)
        {
            removeLine(pObject, y);
            NumberOfLines++;
        }
    }
}
#endif

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
#ifdef ENABLE_SANDER_ICD
    DDRB  = 0xFF; PORTB = 0x00; RBPU = 0;   // PORTB all input; low; Weak pullup enabled
#else
    DDRB  = 0x0F; PORTB = 0x00; RBPU = 0;   // PORTB lower nibble input; low; Weak pullup enabled
#endif
    DDRC  = 0x00; PORTC = 0x00;             // PORTC all output; low -- upper display
    DDRD  = 0x00; PORTD = 0x00;             // PORTD all output; low -- lower display
    //DDRE; PORTE ?

    initialise_TMR0();

    IPEN = 0; // Enable all TMR0, RB, INT1 & INT2 interrupts
    GIE = 1;  // Enable all Interrupts
}
 
// initialise the program
void initialise_globals(void)
{
    // intialise variables
    clearArray(ObjectData, 8);
#ifdef ENABLE_MUXDISPLAY
    clearArray(BackgroundData, 8);
#endif

#ifdef ENABLE_PICTRIS
    OriginX          = 0;
    OriginY          = 0;   
    //CurrentX         = 0;
    NumberOfLines    = 0;
    // initialise events
    DropObject       = false;
    EndOfGame        = false;
    CheckForNewLines = false;
    //
    mS               = 0;
#endif
#ifdef ENABLE_SWDT
    sWDT             = 0;
#endif

#ifdef ENABLE_DEBOUNCE
    // initialise buttons
    mtxButtons       = true;
    //Left_Debounce    = false;
    Left_Delay       = -1;
    Left_Debounced   = false;

    //Right_Debounce   = false;
    Right_Delay      = -1;
    Right_Debounced  = false;

    //Rotate_Debounce  = false;
    Rotate_Delay     = -1;
    Rotate_Debounced = false;

    //Down_Debounce    = false;
    Down_Delay       = -1;
    Down_Debounced   = false;
#endif
#ifdef snake
    moveSnake = false;
    
    snakeLength = 3;
    previous_direction = 3;
    uint8_t i;
    for (i=0;i<4;i++){
        positions[i].y = 3;
        positions[i].x = 4-i;
    }
    positions[0].y = 3;
    positions[0].x = 3;
    direction = 1;  
    CreateBerry();
#else
    tetris = true;
#endif
}

#ifdef ENABLE_PICTRIS
// Changes global var ObjectData
void splash_screen(void)
{
#ifdef ENABLE_MUXDISPLAY
    // semaphore - shared variables are about to be accessed
    pauseMultiplexing();
#endif
    if (tetris == true){ memcpy(ObjectData, TETRIS, 16); } // 16 = ARRAY_SIZE(TETRIS));
    else {memcpy(ObjectData,SNAKE,16);}
#ifdef ENABLE_MUXDISPLAY
    resumeMultiplexing();
#endif
    waitms(3500); // 3.5 sec
}
#endif
#ifdef snake
void choosescreen(void)
{
    uint8_t i;
#ifdef ENABLE_MUXDISPLAY
    // semaphore - shared variables are about to be accessed
    pauseMultiplexing();
#endif
    // clear all graphic buffers
    clearArray(tmpObjectData,  8);
    clearArray(ObjectData,     8);
    clearArray(BackgroundData, 8);
    for (i = 0; i < 8; i++)
    {
        ObjectData[i] = choose_screen[i];
    }

#ifdef ENABLE_MUXDISPLAY
    resumeMultiplexing();
#endif

    // wait for "down" button to be depressed; should be, but let's check
     while (Down_Debounced || Rotate_Debounced)
        continue;
 
    // wait for button to be pressed
    while (!Down_Debounced && !Rotate_Debounced)
        continue;
    uint16_t mask[8];
    if(Down_Debounced == true) { tetris = false; 
    for (i=0;i<8;i++){ mask[i] = 0xFF00; }}
    else {if(Rotate_Debounced == true) { tetris = true; 
    for (i=0;i<8;i++){ mask[i] = 0x00FF; }};}

#ifdef ENABLE_MUXDISPLAY
    pauseMultiplexing();
    mergeObjects(mask, ObjectData, INVERT);
    resumeMultiplexing();
#endif

    // wait for button to be released
    while (Down_Debounced || Rotate_Debounced)
        continue;
    pauseButtons();
    Down_Delay = REPETITION_DELAY;
    resumeButtons();
}
#endif

void getNumber(byte pDigit, uint16_t * pTarget)
{
    memcpy(pTarget, Number[pDigit], 16); // 16 = ARRAY_SIZE(Number[pDigit]));
}

void show_score(byte score)
{
    byte i, currentNumber;
    char number[4]; // expect up to 3 digits + terminating \0

#ifdef ENABLE_MUXDISPLAY
    // semaphore - shared variables are about to be accessed
    pauseMultiplexing();
#endif
    // clear all graphic buffers
    clearArray(tmpObjectData,  8);
    clearArray(ObjectData,     8);
    clearArray(BackgroundData, 8);
 
    itoa(score, number, 10);
    for (i = 0; i < 3 && number[i]; i++) // TODO: change "< 3 && number[i]" to "< strnlen(number, 3)"
    {
        currentNumber = number[i] - '0';
        getNumber(currentNumber, tmpObjectData);
        // shift the result down, depending on index
        moveObject(tmpObjectData, DOWN,  (2-i) * 5);
        moveObject(tmpObjectData, RIGHT, i);
        // or result with graphic object
        mergeObjects(tmpObjectData, ObjectData, MERGE);
    }

#ifdef ENABLE_MUXDISPLAY
    resumeMultiplexing();
#endif

    // wait for "down" button to be depressed; should be, but let's check
     while (Down_Debounced)
        continue;
 
    // wait for button to be pressed
    while (!Down_Debounced)
        continue;

#ifdef ENABLE_MUXDISPLAY
    pauseMultiplexing();
    uint16_t mask[] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
    mergeObjects(mask, ObjectData, INVERT);
    resumeMultiplexing();
#endif

    // wait for button to be released
    while (Down_Debounced)
        continue;
    pauseButtons();
    Down_Delay = REPETITION_DELAY;
    resumeButtons();
}
#ifdef highscore
uint8_t ReadEEByte(uint8_t address)
{
EEADR=address; // load address of EEPROM to read
EECON1bits.EEPGD=0; // access EEPROM data memory
EECON1bits.CFGS=0; // do not access configuration registers
EECON1bits.RD=1; // initiate read
return EEDATA; // return EEPROM byte
}

// Write Byte to internal EEPROM
void WriteEEByte(byte address, byte data)
{
EECON1bits.WREN=1; // allow EEPROM writes
EEADR=address; // load address of write to EEPROM
EEDATA=data; // load data to write to EEPROM
EECON1bits.EEPGD=0;// access EEPROM data memory
EECON1bits.CFGS=0; // do not access configuration registers
INTCONbits.GIE=0; // disable interrupts for critical EEPROM write sequence
//===============//
EECON2=0x55;
EECON2=0xAA;
EECON1bits.WR=1;
//==============//
INTCONbits.GIE=1; // enable interrupts, critical sequence complete
while (EECON1bits.WR==1); // wait for write to complete
EECON1bits.WREN=0; // do not allow EEPROM writes
}
#endif

#ifdef ENABLE_PICTRIS
// TODO: Implement the EE routines
void readHighScore(uint8_t adress)
{
#ifdef highscore
    uint8_t tmpbyte;
    if (tetris == false)
    { adress = adress + 10;};
 
    tmpbyte = ReadEEByte(adress);
    if (tmpbyte == 0){
       WriteEEByte(adress,0); 
    };
    if (tmpbyte == 255) {
        tmpbyte = 0;
    }
    LastHighScore = tmpbyte;
#endif
}
 
void writeHighScore(uint8_t adress)
{
#ifdef highscore
    if (tetris == true){
    if (NumberOfLines > LastHighScore){ 
        WriteEEByte(adress,NumberOfLines);
    };
    }
#ifdef snake
    else if (tetris == false) {
        if (snakeLength > LastHighScore){
        WriteEEByte(adress,snakeLength);
        };
    };
#endif
#endif
}
 
// this is the main game loop for Tetris
void mainGameLoop(void)
{
    // re-initialise screen and variables (splash screen and highscores may have altered stuff..)
#ifdef ENABLE_MUXDISPLAY
    pauseMultiplexing();
#endif
    initialise_globals();
#ifdef ENABLE_MUXDISPLAY
    resumeMultiplexing();
#endif
    clearArray(ObjectData,8);
    clearArray(BackgroundData,8);
    clearArray(tmpObjectData,8);
#ifdef snake
   if (tetris == true){ 
#endif
    // select an object and place it in ObjectData
    selectNextObject(ObjectData);
     do {
        // check if drop object flag has been set (invoked occurs by the interrupt "TimerObjectDrop")
        if (DropObject)
        {
            DropObject = false;
            moveObjectDown(ObjectData);
        }

        // check for completed lines. any routine that moves an object should enable this flag.
        if (CheckForNewLines)
            checkForLines(BackgroundData);
        // scan buttons and action as necessary
        checkButtons();
#ifdef ENABLE_SWDT
        // reset software watch dog timer
        sWDT = 0;
#endif
    // loop forever, or until the EndOfGame event is set true
        } while (!EndOfGame);
        writeHighScore(10);
        show_score(NumberOfLines);
#ifdef snake
    }
    else {
       do{
            if (moveSnake == true) {
                moveSnake = false;
                MoveSnakes();
            };
            
            checkButtons();
#ifdef ENABLE_SWDT
                // reset software watch dog timer
                sWDT = 0;
#endif
            } while(!EndOfGame);
            writeHighScore(20);
            show_score(snakeLength);
    }
#endif
    // write the high score to EEPROM
    // display the score!
}
#endif

// main program start
void main(void)
{ 
#ifdef ENABLE_PICTRIS
    // seed randgen module
    rnd_initialize(0x77);
#ifdef RANDGEN
    rnd_set_max(NUMBER_OF_SHAPES);
#endif
    
    // initialise screen and variables
    initialise_hardware();
    initialise_globals();

#ifdef ENABLE_MUXDISPLAY
    // enable screen multiplexing & other timer functions
    UpdateScreen = true;
#endif
    // display the flash screen
#ifdef snake
    choosescreen(); 
#endif
    splash_screen();

    // Show last high score from EEPROM
    readHighScore(10);
    show_score(LastHighScore);

    for (;;)
        // enter main game program, will stay there until an END event occurs
        mainGameLoop();

#elif defined ENABLE_DOBBELSTEEN
    uint8_t digit, counter, selection, rndSelection;

    initialise_globals();
    initialise_hardware();

    rnd_initialize(0x77);

#ifdef ENABLE_TEST_SHAPE
    UpdateScreen = true;
#endif

    for(;;)
    {
#ifndef ENABLE_TEST_SHAPE
        rndSelection = rnd_get_num();

        // make a selection based on the random number created
        counter = 0;
        selection = 0;
        do
        {
            counter += RND_WINDOW;
            selection++;
        } while (counter < rndSelection);
        //selection = rndSelection;
        digit = selection % NUMBER_OF_SHAPES;
        show_score(digit);
#else
        selectNextObject(ObjectData);
        waitms(500);
#endif
    }
#else
    //uint16_t data[] = {0x8818, 0x4424, 0x2242, 0x1181, 0x1181, 0x2242, 0x4424, 0x8818};
    uint16_t data2[] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
    //uint16_t data[] = {0xaaaa, 0x0000, 0x5555, 0x0000, 0xaaaa, 0x0000, 0x5555, 0x0000};
    uint16_t data[] = {0x01ff, 0x03fe, 0x07fc, 0x0ff8, 0x1ff0, 0x3fe0, 0x7fc0, 0xff80};
    //uint16_t data[] = {0x01ff, 0x03fe, 0x07fc, 0x0ff8, 0x0ff8, 0x07fc, 0x03fe, 0x01ff};

    initialise_globals();
    initialise_hardware();

    pauseMultiplexing();
    //memcpy(ObjectData, data, ARRAY_SIZE(data));
    mergeObjects(data, ObjectData, OVERRIDE);
    resumeMultiplexing();

    byte prevState = 0xff, currState;

    for(;;)
    {
        if (mS == 500)
        {
            mergeObjects(data2, ObjectData, INVERT);
            mS = 0;
        }
#if 1
        if (Left_Debounced)
        {
            pauseMultiplexing();
            uint16_t tmp = ObjectData[0];
            for (byte i = 1; i < sizeof ObjectData; i++)
                ObjectData[i-1] = ObjectData[i];
            ObjectData[7] = tmp;
            resumeMultiplexing();
            waitms(25);
        }
        if (Right_Debounced)
        {
            pauseMultiplexing();
            uint16_t tmp = ObjectData[7];
            for (byte i = sizeof ObjectData - 1; i > 0; i--)
                ObjectData[i] = ObjectData[i-1];
            ObjectData[0] = tmp;
            resumeMultiplexing();
            waitms(25);
        }
        if (Rotate_Debounced)
        {
            bool tmp;
            pauseMultiplexing();
            for (byte i = 0; i < sizeof ObjectData; i++)
            {
                tmp = (ObjectData[i] & 0x0001) != 0;
                ObjectData[i] >>= 1;
                VBIT(ObjectData[i], 15, tmp);
            }
            resumeMultiplexing();
            waitms(25);
        }
        if (Down_Debounced)
        {
            bool tmp;
            pauseMultiplexing();
            for (byte i = 0; i < sizeof ObjectData; i++)
            {
                tmp = (ObjectData[i] & 0x8000) != 0;
                ObjectData[i] <<= 1;
                VBIT(ObjectData[i], 0, tmp);
            }
            resumeMultiplexing();
            waitms(25);
        }
#else
        currState = Left_Debounced << 3 | Right_Debounced << 2 | Rotate_Debounced << 1 | Down_Debounced;

        if (currState != prevState)
        {
            pauseMultiplexing();
            //if (Left == 0)   ObjectData[0] |= 1<<14; else ObjectData[0] &= ~(1<<14);
            VBIT(ObjectData[0], 14, Left_Debounced);
            VBIT(ObjectData[2], 14, Right_Debounced);
            VBIT(ObjectData[1], 13, Rotate_Debounced);
            VBIT(ObjectData[1], 15, Down_Debounced);
            VBIT(ObjectData[1], 14, (!Left_Debounced && !Right_Debounced && !Rotate_Debounced && !Down_Debounced));
            resumeMultiplexing();
            prevState = currState;
        }
#endif
    }
#endif
}
