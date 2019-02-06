#include <xc.h>
#include <stdbool.h>
#include <limits.h>     // - for SHRT_MAX
#include <stdint.h>
#include "screen.h"
#include "tetris.h"
#include "snake.h"


volatile int16_t Left_Delay,     Right_Delay,     Rotate_Delay,     Down_Delay;
volatile bool Left_Debounced, Right_Debounced, Rotate_Debounced, Down_Debounced;
volatile bool mtxButtons;

void start_button(void){
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
    //Down_Delay       = -1;
    Down_Delay = 500;
    Down_Debounced   = false;
    mtxButtons       = true;
}

#  define DEBOUNCE_DELAY   20       // 20 ms
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

bool checkLeft(void){
    return Left_Debounced;
}

bool checkRight(void){
    return Right_Debounced;
}

bool checkUp(void){
    return Rotate_Debounced;
}

bool checkDown(void){
    return Down_Debounced;
}


//makes button unuseable for DEBOUNCE_DELAY in ms
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

void Debounce_check()
{
    if (mtxButtons) // Is already claimed by the mainGameLoop()
    {
        debounceButton(Left,   &Left_Delay,   &Left_Debounced  );
        debounceButton(Right,  &Right_Delay,  &Right_Debounced );
        debounceButton(Rotate, &Rotate_Delay, &Rotate_Debounced);
        debounceButton(Down,   &Down_Delay,   &Down_Debounced  );
    }
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
    if (!tetris_button_left()){snake_button_left();}
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
    if (!tetris_button_right()){snake_button_right();} 
    }
    // check if rotate button is pressed 
    if (Rotate_Debounced)
    {
    pauseButtons();
        Rotate_Debounced = false;
        Rotate_Delay = REPETITION_DELAY;
    resumeButtons();
        // new rotation method, named accordingly, more info there
    if (!tetris_button_up()){snake_button_up();} 
    }
    // check if down is pressed
    if (Down_Debounced)
    {
        // move the current object straight to the bottom 
    if (!tetris_button_down()){snake_button_down();}
    pauseButtons();
        Down_Debounced = false;
        Down_Delay = SHRT_MAX; // I see you in 33 seconds
    resumeButtons();
    }
}