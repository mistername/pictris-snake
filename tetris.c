#include <xc.h>
#include <stdbool.h>
#include <stdint.h>     // - for uint8_t/uint16_t/int16_t

#include "shapes.h"
#include "config.h"
#include "randgen.h"
#include "screen.h"
#include "shared_logic.h"
#include "tetris.h"
#include "buttons.h"
#define RND_WINDOW       (55/NUMBER_OF_SHAPES)


volatile uint16_t ObjectData[8];
volatile uint16_t BackgroundData[8];
uint16_t tmpObjectData[8];
volatile bool EndOfGame;

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

uint8_t randomobjects[8];

uint8_t LimitedRotation;
uint8_t NumberOfLines;

bool IsRotated;

bool CheckForNewLines;

uint8_t countblocks;

volatile bool DropObject;
uint8_t OriginX, OriginY;
volatile bool game = false;

void tetris_screen(void){
memcpyvol(ObjectData, TETRIS, 16);
}
// Drop the object by one Y increment. Return FALSE if failed.
bool moveObjectDown(volatile uint16_t * pObject)
{
    bool result = true;
    // Semaphore, to protect shared variables
    pauseMultiplexing();
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
    set_mS(0); // reset the timer because of the moveDown()
    // resume multiplexing, shared resources are done with
    resumeMultiplexing();
    return result;
}

// check the passed object to see if it has reached the bottom
// return true if so
void moveObject(volatile uint16_t * pObject, direction_t direction, uint8_t cycles)
{
    uint8_t i, c;

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
    uint8_t x1, y1;

    // check to see if rotations are disabled for current object
    if (LimitedRotation == 2)
        memcpyvol(pTarget, pSource, 16); // 16 = ARRAY_SIZE(pSource) = 8 * sizeof uint16_t
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

void selectNextObject(volatile uint16_t *pTarget)
{
    uint8_t rndSelection, counter, selection;
    struct shape * s;
    bool check[NUMBER_OF_SHAPES];
    if(countblocks == 7){
        countblocks = 0;
        // create a random number from 1-255
        for(counter=0; counter<NUMBER_OF_SHAPES;counter++){ 
            randomobjects[counter] = 255;
            check[counter] = false;
        }
        for(counter=0; counter<NUMBER_OF_SHAPES;counter++){
            bool fill = false;
            do {
                rndSelection = rnd_get_num();
                rndSelection = rndSelection % NUMBER_OF_SHAPES;
                if (check[rndSelection] == false) {
                    randomobjects[counter] = rndSelection;
                    check[rndSelection] = true;
                    fill = true;
                }
            } while (!fill);
        }
    }
    s = &shapes[randomobjects[countblocks]];
    countblocks++;
    
    memcpyvol(pTarget, s->graphic, 16); // 16 = ARRAY_SIZE(s->graphic));
    OriginX = s->x;
    OriginY = s->y;
    IsRotated = false;
    LimitedRotation = s->limitedRotation;
    set_mS(0); // reset the timer
}

bool checkForBottom(volatile uint16_t * pObject)
{
    for (uint8_t i = 0; i < 8; i++)
        if (GBIT(pObject[i], 15))
            return true;
    return false;
}
 
// remove COMPLETED lines & return the number of lines found
void checkForLines(volatile uint16_t * pObject)
{
    uint8_t x, y, pixels;
 
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

bool tetris_button_left(void) {
    if (game)
    {
        if (!checkForLeftWall(ObjectData))
        {
            // move the object to a temporary buffer
            //mergeObjects(ObjectData, tmpObjectData, OVERRIDE);
            memcpyvol(tmpObjectData, ObjectData, 16);
            // Decrement the x axis
            moveObject(tmpObjectData, LEFT, 1);
            // ensure no collisions with background
            if (!collisionDetect(tmpObjectData, BackgroundData))
            {
                // semaphore - shared variables are about to be accessed
                pauseMultiplexing();
                // all went well, merge the array to the working buffer
                mergeObjects(tmpObjectData, ObjectData, OVERRIDE);// TODO: is moveObject(ObjectData) niet sneller
                // enable the debounce timer (prevents double presses etc)
                resumeMultiplexing();
                // always a chance that a new line was just made
                CheckForNewLines = true;
            }
        }
    }
    return game;
}

bool tetris_button_right(void){
    if (game)
    {
        if (!checkForRightWall(ObjectData))
        {
            mergeObjects(ObjectData, tmpObjectData, OVERRIDE);
            moveObject(tmpObjectData, RIGHT, 1);
            // ensure no collisions with objects
            if (!collisionDetect(tmpObjectData, BackgroundData))
            {
                pauseMultiplexing();
                mergeObjects(tmpObjectData, ObjectData, OVERRIDE);
                resumeMultiplexing();
                CheckForNewLines = true;
            }
        }
    }
    return game;
}

bool tetris_button_up(void){
    if (game)
    {
        newRotation(ObjectData, tmpObjectData, CW);
        // ensure nothing has fallen off due to near-wall rotations
        if (pixelCount(ObjectData) == pixelCount(tmpObjectData))
        {
            // check for object collision due to rotation
            if (!collisionDetect(tmpObjectData, BackgroundData))
            {
                // FINALLY, if it gets here then ALL WENT WELL.
                // semaphore - shared variables are about to be accessed
                pauseMultiplexing();
                // Temp buffer merged with working buffer
                mergeObjects(tmpObjectData, ObjectData, OVERRIDE);
                resumeMultiplexing();
                IsRotated = !IsRotated;
            }
        }
    }
    return game;
}

bool tetris_button_down(void) {
    if (game)
    {
    while (moveObjectDown(ObjectData))
            continue; 
    } 
    return game;
}
bool tetris_timer(void){
    if (game) {
    DropObject = true;
    }
    return game;
}

void initialise_tetris(void) {
    OriginX          = 0;
    OriginY          = 0;   
    //CurrentX         = 0;
    NumberOfLines    = 0;
    // initialise events
    DropObject       = false;
    CheckForNewLines = false;
    randomobjects[7] = 255;
    countblocks = 7;
    game = true;
}

void tetris_main(void) {
    uint8_t LastHighScore = readHighScore(0);
    show_score(LastHighScore);
    EndOfGame        = false;
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
    // loop forever, or until the EndOfGame event is set true
        } while (!EndOfGame);
        writeHighScore(0, LastHighScore, NumberOfLines);
        show_score(NumberOfLines);
}

