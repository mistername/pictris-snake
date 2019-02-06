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

void set_screen(uint16_t newData[]){
    int i;
    clearArray(ScreenData, 8);
    for(i=0;i<8;i++){
        ScreenData[i] = newData[i];
    }
}
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

void choosescreen(void)
{
    uint8_t i;
    // semaphore - shared variables are about to be accessed
    pauseMultiplexing();
    // clear all graphic buffers
    clearArray(ScreenData,  8);
    for (i = 0; i < 8; i++)
    {
        ScreenData[i] = choose_screen[i];
    }

    resumeMultiplexing();
    
    // wait for "down" button to be depressed; should be, but let's check
     while (checkDown() || checkUp())
        continue;
 
    // wait for button to be pressed
    while (!checkDown() && !checkUp())
        continue;
    uint16_t mask[8];
    if(checkDown() == true) { for (i=0;i<8;i++){ mask[i] = 0xFF00; }}
    else {if(checkUp() == true) { for (i=0;i<8;i++){ mask[i] = 0x00FF; }};}

    pauseMultiplexing();
    mergeObjects(mask, ScreenData, INVERT);
    resumeMultiplexing();

    // wait for button to be released
    while (checkDown() || checkUp())
        continue;
    pauseButtons();
    //Down_Delay = 500;
    resumeButtons();
}



void screen_update(void)
{
    if (UpdateScreen)
        {
            static uint8_t CurrentX = 0;

            uint8_t  xmask = 1 << CurrentX;
            uint16_t ymask = ScreenData[CurrentX];

            port_display = 0;
            
            PORTA        = xmask;
            
            port_display = ~ymask;

            // increment x axis, ready for the next multiplex
            CurrentX++;
            CurrentX &= 0x07; // wrap around after 8 states
            
        }
}