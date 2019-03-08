#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "interrupt.h"
#include "shared_logic.h"
#include "buttons.h"
#include "gamesconfig.h"

volatile uint16_t port_display __at(0xF82);
volatile bool UpdateScreen;
volatile uint16_t ScreenData[8];

void initialise_screen(void)
{
    UpdateScreen = true;
}

void pauseMultiplexing(void)
{
    if(UpdateScreen)
    {
        waitForInterrupt();
        UpdateScreen = false;
    }
}

void resumeMultiplexing(void)
{
    UpdateScreen = true;
}

void set_screen(uint16_t *newData)
{
    pauseMultiplexing();
    memcpyvol(ScreenData, newData, 16);
    resumeMultiplexing();
}

void set_splashscreen(const uint16_t *newData)
{
    pauseMultiplexing();
    memcpyvol(ScreenData, newData, 16);
    resumeMultiplexing();
}

bool choosescreen(void)
{
    {
        CHOOSESCREEN;
        set_splashscreen(choosescreen);
    }
    // wait for "down" button to be depressed; should be, but let's check
    while(checkDown(false) || checkUp(false))
    {
        continue;
    }
    // wait for button to be pressed
    while(!checkDown(false) && !checkUp(false))
    {
        continue;
    }
    bool game;
    {
        uint16_t mask[8];
        if(checkDown(false) == true)
        {
            for(int i = 0; i < 8; i++)
            {
                mask[i] = 0xFF00;
            }
            game = false;
        }
        else
        {
            if(checkUp(false) == true)
            {
                for(int i = 0; i < 8; i++)
                {
                    mask[i] = 0x00FF;
                }
                game = true;
            };
        }
        pauseMultiplexing();
        mergeObjects(mask, ScreenData, INVERT);
        resumeMultiplexing();
    }

    // wait for button to be released
    while(checkDown(false) || checkUp(false))
    {
        continue;
    }
    return game;
}

void screen_update(void)
{
    if(UpdateScreen)
    {
        static uint8_t CurrentX = 0;

        uint8_t xmask = 1 << CurrentX;
        uint16_t ymask = ScreenData[CurrentX];
        port_display = ~0;
        PORTA = xmask;
        port_display = ~ymask;

        // increment x axis, ready for the next multiplex
        CurrentX++;
        CurrentX &= 0x07; // wrap around after 8 states

    }
}