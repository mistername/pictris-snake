#include <xc.h>
#include <stdbool.h>
#include <limits.h>     // - for SHRT_MAX
#include <stdint.h>
#include "screen.h"


volatile int16_t Left_Delay,     Right_Delay,     Rotate_Delay,     Down_Delay;
volatile bool Rotate_Debounced, Left_Debounced, Right_Debounced, Down_Debounced;
volatile bool mtxButtons;

void start_button(void)
{
    Left_Delay       = -1;
    Left_Debounced   = false;

    Right_Delay      = -1;
    Right_Debounced  = false;

    Rotate_Delay     = -1;
    Rotate_Debounced = false;

    Down_Delay = -1;
    Down_Debounced   = false;
    
    mtxButtons       = true;
}

#  define DEBOUNCE_DELAY   50       // 20 ms
#  define REPETITION_DELAY 500      // 500 ms

#  define Left   PORTBbits.RB0
#  define Right  PORTBbits.RB1
#  define Rotate PORTBbits.RB2
#  define Down   PORTBbits.RB3

//clears trigger for checking buttons in interrupt
void pauseButtons()
{
    mtxButtons = false;
}
//fills trigger for checking buttons in interrupt
void resumeButtons()
{
    mtxButtons = true;
}

bool checkLeft(bool reset)
{
    if (Left_Debounced)
    {
        Left_Delay = REPETITION_DELAY;
        Left_Debounced = !reset;
        return true;
    } else return false;
}

bool checkRight(bool reset)
{
    if (Right_Debounced)
    {
        Right_Delay = REPETITION_DELAY;
        Right_Debounced = !reset;
        return true;
    } else return false;
}

bool checkUp(bool reset)
{
    if (Rotate_Debounced)
    {
        Rotate_Delay = REPETITION_DELAY;
        Rotate_Debounced = !reset;
        return true;
    } else return false;
}

bool checkDown(bool reset)
{
    if (Down_Debounced)
    {
        Down_Delay = REPETITION_DELAY;
        Down_Debounced = !reset;
        return true;
    } else return false;
}


//makes button unuseable for DEBOUNCE_DELAY in ms
void debounceButton(volatile bool button, volatile int16_t *delay, volatile bool *debounced)
{
    if (button == 0) // Button pressed
    {
        if (*delay < 0){ *delay = DEBOUNCE_DELAY; }
        else if ((*delay)-- == 0){ *debounced = true; }
    }
    else
    {
        *delay = -1;
        *debounced = false;
    }
}
// Check the state of each button. Because this routine is called often, the chances of missing a button press
// are very unlikely. 
void checkButtons(void)
{
    if (mtxButtons) // Is already claimed by the mainGameLoop()
    {
        debounceButton(Left,   &Left_Delay,   &Left_Debounced  );
        debounceButton(Right,  &Right_Delay,  &Right_Debounced );
        debounceButton(Rotate, &Rotate_Delay, &Rotate_Debounced);
        debounceButton(Down,   &Down_Delay,   &Down_Debounced  );
    }
}