#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdbool.h>
#include <stdint.h>
#include "shared_logic.h"
#include "buttons.h"
#include "screen.h"
#include "gamesconfig.h"

volatile bool InterruptComplete;

volatile unsigned RemainingWaitTime;
void waitms(unsigned t)
{
    RemainingWaitTime = t;
    while(RemainingWaitTime)
    {
        continue; 
    }                // time is decremented in Interrupt
}


void Interrupt(bool game) 
{
    RemainingWaitTime--;             // variable for waitms()
    
    add_mS(1);
    if(get_mS() >= LCM_TIME){set_mS(0);}
    if(game == true && get_mS() % GAME1TIME == 0){GAME1TIMER();}
    if(game != true && get_mS() % GAME2TIME == 0){GAME2TIMER();}
    checkButtons();
    screen_update();
    InterruptComplete = true;
}

void waitForInterrupt(void)
{
    InterruptComplete = false;
    while (!InterruptComplete){ continue; }
}