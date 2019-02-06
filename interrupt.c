/*
 * File:   interrupt.c
 * Author: ryan
 *
 * Created on 5 februari 2019, 22:14
 */
#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdbool.h>
#include <stdint.h>
#include "shared_logic.h"
#include "buttons.h"
#include "screen.h"
#include "snake.h"
#include "tetris.h"

volatile bool InterruptComplete;
volatile unsigned time;

void waitms(unsigned t)
{
    time = t;
    while(time)
        continue;		// time is decremented in timer0interrupt
}


void Interrupt(void) {
    
        time--;             // variable for waitms()
        
        add_mS(1);
        if (get_mS() >= 1600)
        {
            set_mS(0);
        }
        Debounce_check();
        if (get_mS() % 800 == 0 && tetris_timer())
            ;
        if(get_mS() % 200 == 0 && snake_timer())
            ;
        screen_update();
        InterruptComplete = true;
}

void waitForInterrupt(void)
{
    InterruptComplete = false;
    while (!InterruptComplete)
        continue;
}
