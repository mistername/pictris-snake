#include <xc.h>
#include <stdbool.h>
#include <stdint.h>     // - for uint8_t/uint16_t/int16_t

#include "shapes.h"
#include "config.h"
#include "randgen.h"
#include "screen.h"
#include "shared_logic.h"
#include "buttons.h"
#define RND_WINDOW       (55/NUMBER_OF_SHAPES)

struct tetris 
{
    struct shape currentblock;
    bool IsRotated;
    uint8_t countblocks;
    uint8_t randomobjects[NUMBER_OF_SHAPES];
};


const uint16_t TETRIS[] = {
                           0x8841, // *   *    *     *,
                           0xFBDF, // ***** **** *****,
                           0x8841, // *   *    *     *,
                           0x0000, //                 ,
                           0x9BDF, // *  ** **** *****,
                           0xA955, // * * *  * * * * *,
                           0xCA91, // **  * * *  *   *,
                           0x0000, //
};

volatile bool DropObject;

void tetris_screen(void)
{
    set_splashscreen(TETRIS);
    waitms(3500);
}

void selectNextObject(struct tetris *tetris)
{
    struct shape *s;
    if (tetris->countblocks == NUMBER_OF_SHAPES)
    {
        bool check[NUMBER_OF_SHAPES];
        tetris->countblocks = 0;
        {
        uint8_t counter = NUMBER_OF_SHAPES;
        while(counter--)
        {
            tetris->randomobjects[counter] = 255;
            check[counter] = false;
        }
        }
        {
        uint8_t counter = NUMBER_OF_SHAPES;
        while(counter--)
        {
            bool fill = false;
            do
            {
                uint8_t rndSelection = rnd_get_num();
                rndSelection = rndSelection % NUMBER_OF_SHAPES;
                if(check[rndSelection] == false)
                {
                    tetris->randomobjects[counter] = rndSelection;
                    check[rndSelection] = true;
                    fill = true;
                }
            }
            while(!fill);
        }
        }
    }
    s = &shapes[tetris->randomobjects[tetris->countblocks]];
    struct shape *NewBlock = &tetris->currentblock;
    memcpyvol(NewBlock->graphic, s->graphic, 16); // 16 = ARRAY_SIZE(s->graphic));
    NewBlock->x = s->x;
    NewBlock->y = s->y;
    tetris->IsRotated = false;
    NewBlock->limitedRotation = s->limitedRotation;
    set_mS(0); // reset the timer
    tetris->countblocks++;
}

bool checkForBottom(volatile uint16_t * pObject)
{
    for(uint8_t i = 0; i < 8; i++)
        if(GBIT(pObject[i], 15))
            return true;
    return false;
}

// check the passed object to see if it has reached the bottom
// return true if so

void moveObjecttetris(struct shape *tetrisblock, direction_t direction, uint8_t cycles)
{
    uint8_t i, c;

    switch(direction)
    {
        case DOWN:
            for(c = 0; c < cycles; c++)
            {
                for(i = 0; i < 8; i++) {tetrisblock->graphic[i] <<= 1; }
                tetrisblock->y = tetrisblock->y + 1;
            }
            break;
        case UP:
            for(c = 0; c < cycles; c++)
            {
                for(i = 0; i < 8; i++) {tetrisblock->graphic[i] >>= 1; }
                tetrisblock->y = tetrisblock->y - 1;
            }
            break;
        case RIGHT:
            for(c = 0; c < cycles; c++)
            {
                for(i = 7; i > 0; i--) {tetrisblock->graphic[i] = tetrisblock->graphic[i - 1]; }
                tetrisblock->graphic[0] = 0;
                tetrisblock->x = tetrisblock->x + 1;
            }
            break;
        case LEFT:
            for(c = 0; c < cycles; c++)
            {
                for(i = 0; i < 7; i++) {tetrisblock->graphic[i] = tetrisblock->graphic[i + 1];}
                tetrisblock->graphic[7] = 0;
                tetrisblock->x = tetrisblock->x - 1;
            }
    }
}

// Drop the object by one Y increment. Return FALSE if failed.

bool moveObjectDown(struct tetris *tetris, uint16_t *BackgroundData, bool *CheckForNewLines, bool *EndOfGame)
{
    struct shape *tetrisblock = &tetris->currentblock;
    bool result = true;
    // Semaphore, to protect shared variables
    // check for a collision with the bottom of the screen
    if(checkForBottom(tetrisblock->graphic))
    {
        // ... yes it has - time to save it there
        result = false;
        // OR the object into the background
        mergeObjects(tetrisblock->graphic, BackgroundData, MERGE);
        // get a new object up-and-running
        selectNextObject(tetris);
        // check for new lines
        *CheckForNewLines = true;
    }
    else
    {
        // Move the object down
        moveObjecttetris(tetrisblock, DOWN, 1);
        // check for a collision with the background
        if(collisionDetect(tetrisblock->graphic, BackgroundData))
        {
            // the object HIT something in the background. The object needs to be 
            // moved BACK UP and saved to the background.
            result = false;
            // move the object up one
            moveObjecttetris(tetrisblock, UP, 1);
            // move the object to the background (OR it over)
            mergeObjects(tetrisblock->graphic, BackgroundData, MERGE);
            // get a new object up-and-running
            selectNextObject(tetris);
            // check if the object has collided with the background already. If yes, then
            // the game is over.
            *EndOfGame = collisionDetect(tetrisblock->graphic, BackgroundData);
            // it's always possible that new lines may have been made already
            *CheckForNewLines = true;
        }
    }
    set_mS(0); // reset the timer because of the moveDown()
    // resume multiplexing, shared resources are done with
    return result;
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

void newRotation(struct tetris *tetris, uint16_t * pTarget, rotation_t *rotation)
{
    int8_t x2, y2;
    uint8_t x1, y1;
    struct shape *tetrisblock = &tetris->currentblock;
    // check to see if rotations are disabled for current object
    if(tetrisblock->limitedRotation == 2)
    {
        memcpyvol(pTarget, tetrisblock->graphic, 16); // 16 = ARRAY_SIZE(pSource) = 8 * sizeof uint16_t
    }
    else
    {
        // clear the target array
        clearArray(pTarget, 8);

        // if the object is limited by rotation, then reverse
        // rotations 1 & 3. This will make the object turn like so
        // CW, CCW, CW, CCW
        if(tetrisblock->limitedRotation == 1 && tetris->IsRotated)
        {
            *rotation = CCW;
        }
        // analyse each pixel (working with 5x5 graphic)
        for(x1 = 0; x1 < 8; x1++)
        {
            for(y1 = 0; y1 < 16; y1++)
            {
                if(GBIT(tetrisblock->graphic[x1], y1))
                {
                    if(*rotation == CW)
                    {
                        x2 = tetrisblock->x + tetrisblock->y - y1;
                        y2 = x1 + tetrisblock->y - tetrisblock->x;
                    }
                    else
                    {
                        x2 = y1 + tetrisblock->x - tetrisblock->y;
                        y2 = tetrisblock->x + tetrisblock->y - x1;
                    }
                    if(x2 >= 0 && x2 < 8 && y2 >= 0 && y2 < 16)
                        SBIT(pTarget[x2], y2); //pDestinationObject(GetIndexY(Int(Y2)), GetIndexX(Int(X2))) = 1
                }
            }
        }
    }
}

// remove COMPLETED lines & return the number of lines found

uint8_t checkForLines(uint16_t * pObject)
{
    uint8_t x, y, pixels;
    uint8_t NumberOfLines = 0;
    for(y = 0; y < 16; y++)
    {
        pixels = 0;
        for(x = 0; x < 8; x++)
            if(GBIT(pObject[x], y))
                pixels++;
        if(pixels == 8)
        {
            removeLine(pObject, y);
            NumberOfLines++;
        }
    }
    return NumberOfLines;
}

bool tetris_button_left(struct shape *tetrisblock, uint16_t *BackgroundData)
{
    if(!checkForLeftWall(tetrisblock->graphic))
    {
        uint16_t tmpObjectData[8];

        // move the object to a temporary buffer
        //mergeObjects(ObjectData, tmpObjectData, OVERRIDE);
        memcpyvol(tmpObjectData, tetrisblock->graphic, 16);
        // Decrement the x axis
        moveObject(tmpObjectData, LEFT, 1);
        // ensure no collisions with background
        if(!collisionDetect(tmpObjectData, BackgroundData))
        {
            pauseMultiplexing();
            mergeObjects(tmpObjectData, tetrisblock->graphic, OVERRIDE); // TODO: is moveObject(ObjectData) niet sneller
            resumeMultiplexing();
            tetrisblock->x--;
            return true; //CheckForNewLines
        }
    }
    return false;

}

bool tetris_button_right(struct shape *tetrisblock, uint16_t *BackgroundData)
{
    if(!checkForRightWall(tetrisblock->graphic))
    {
        uint16_t tmpObjectData[8];

        mergeObjects(tetrisblock->graphic, tmpObjectData, OVERRIDE);
        moveObject(tmpObjectData, RIGHT, 1);
        // ensure no collisions with objects
        if(!collisionDetect(tmpObjectData, BackgroundData))
        {
            pauseMultiplexing();
            mergeObjects(tmpObjectData, tetrisblock->graphic, OVERRIDE); // TODO: is moveObject(ObjectData) niet sneller
            resumeMultiplexing();
            tetrisblock->x++;
            return true; //CheckForNewLines
        }
    }
    return false;
}

void tetris_button_up(struct tetris *tetris, uint16_t *BackgroundData, rotation_t *rotation)
{
    uint16_t tmpObjectData[8];
    clearArray(tmpObjectData, 8);

    newRotation(tetris, tmpObjectData, rotation);
    
    {
    struct shape *tetrisblock = &tetris->currentblock;
    // ensure nothing has fallen off due to near-wall rotations
    if(pixelCount(tetrisblock->graphic) == pixelCount(tmpObjectData)) //TODO: is testen tmpdata niet sneller
    {
        // check for object collision due to rotation
        if(!collisionDetect(tmpObjectData, BackgroundData))
        {
            // FINALLY, if it gets here then ALL WENT WELL.
            // semaphore - shared variables are about to be accessed
            // Temp buffer merged with working buffer
            mergeObjects(tmpObjectData, tetrisblock->graphic, OVERRIDE);
            tetris->IsRotated = ~tetris->IsRotated;
        }
    }
    }
}

void tetris_button_down(struct tetris *tetris, uint16_t *BackgroundData, bool *CheckForNewLines, bool *EndOfGame)
{
    while(moveObjectDown(tetris, BackgroundData, CheckForNewLines, EndOfGame))
    {
        continue;
    }
}

void tetris_timer(void)
{
    DropObject = true;
}

bool tetris_buttons(struct tetris *tetris, uint16_t *BackgroundData, bool *CheckForNewLines, bool *EndOfGame, rotation_t *rotation)
{
    bool returnbool = false;
    if(checkLeft(true))
    {
        *CheckForNewLines = tetris_button_left(&tetris->currentblock, BackgroundData);
        returnbool = true;
    }
    if(checkRight(true))
    {
        *CheckForNewLines = tetris_button_right(&tetris->currentblock, BackgroundData);
        returnbool = true;
    }
    if(checkUp(true))
    {
        tetris_button_up(tetris, BackgroundData, rotation);
        returnbool = true;
    }
    if(checkDown(true))
    {
        tetris_button_down(tetris, BackgroundData, CheckForNewLines, EndOfGame);
        returnbool = true;
    }
    return returnbool;
}

void tetris_main(void)
{
    uint16_t BackgroundData[8];
    clearArray(BackgroundData, 8);
    
    tetris_screen();
    uint8_t LastHighScore = readHighScore(0);
    show_score(LastHighScore);

    struct tetris tetris;
    clearArray(tetris.currentblock.graphic, 8);
    
    tetris.countblocks = NUMBER_OF_SHAPES;

    uint8_t NumberOfLines = 0;
    
    bool CheckForNewLines = false;

    selectNextObject(&tetris);
    set_screen(tetris.currentblock.graphic);

    rotation_t rotation;

    bool EndOfGame = false;
    DropObject = false;
    do
    {
        bool newScreen;
        newScreen = tetris_buttons(&tetris, BackgroundData, &CheckForNewLines, &EndOfGame, &rotation) || newScreen;
        if(DropObject)
        {
            DropObject = false;
            moveObjectDown(&tetris, BackgroundData, &CheckForNewLines, &EndOfGame);
            newScreen = true;
        }
        if(CheckForNewLines)
        {
            NumberOfLines = NumberOfLines + checkForLines(BackgroundData);
            newScreen = true;
        }
        if(newScreen)
        {
            uint16_t Screen_Data[8];
            mergeObjects(BackgroundData, Screen_Data, OVERRIDE);
            mergeObjects(tetris.currentblock.graphic, Screen_Data, MERGE);
            set_screen(Screen_Data);
            newScreen = false;
        }
    }
    while(!EndOfGame);
    writeHighScore(0, LastHighScore, NumberOfLines);
    show_score(NumberOfLines);
}

