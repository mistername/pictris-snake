#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "interrupt.h"
#include "shared_logic.h"
#include "buttons.h"


extern volatile unsigned short port_display __at(0xF82);
asm("port_display equ 0F82h");    //    -\_(-,-)_/-

const uint16_t choose_screen[] =
{
    0xFFFF, // ****************,
    0x8181, // *      **      *,
    0xBD81, // * **** **      *,
    0x858D, // *    * **   ** *,
    0x8599, // *    * **  **  *,
    0xAD81, // * * ** **      *,
    0x8181, // *      **      *,
    0xFFFF, // ****************
};

volatile bool UpdateScreen;
volatile uint16_t ScreenData[8];

void initialise_screen(void){
    UpdateScreen = true;
}
// Loop until an interrupt occurs.
// Useful for semaphores - allows shared resources to be used with little (no) impact on timing
// This is the primary semaphore routine. If the program uses shared resources, this routine is called.
void pauseMultiplexing(void)
{
    if (UpdateScreen)
    {
        waitForInterrupt();
        UpdateScreen = false;
    }
}

// Enables multiplexing to resume
void resumeMultiplexing(void)
{
    UpdateScreen = true;
}

void set_screen(volatile uint16_t newData[])
{
    pauseMultiplexing();
    clearArray(ScreenData, 8);
    int i;
    for(i=0;i<8;i++){
        ScreenData[i] = newData[i];
    }
    resumeMultiplexing();
}

bool choosescreen(void)
{
    bool tetris;
    int i;
    set_screen(choose_screen);
    // wait for "down" button to be depressed; should be, but let's check
     while (checkDown(false) || checkUp(false))
        continue;
 
    // wait for button to be pressed
    while (!checkDown(false) && !checkUp(false))
        continue;
    uint16_t mask[8];
    if(checkDown(false) == true) { for (i=0;i<8;i++){ mask[i] = 0xFF00; } tetris = true;}
    else {if(checkUp(false) == true) { for (i=0;i<8;i++){ mask[i] = 0x00FF; } tetris = false; };}

    pauseMultiplexing();
    mergeObjects(mask, ScreenData, INVERT);
    resumeMultiplexing();

    // wait for button to be released
    while (checkDown(false) || checkUp(false))
    {   continue;   }
    return tetris;
}



void screen_update(void)
{
    if (UpdateScreen)
        {
            static uint8_t CurrentX = 0;

            uint8_t  xmask = 1 << CurrentX;
            uint16_t ymask = ScreenData[CurrentX];

            port_display = 0;
            
            PORTA = xmask;
            
            port_display = ~ymask;

            // increment x axis, ready for the next multiplex
            CurrentX++;
            CurrentX &= 0x07; // wrap around after 8 states
            
        }
}