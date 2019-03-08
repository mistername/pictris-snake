#include <xc.h>
#include <stdint.h>     // - for uint8_t/uint16_t/int16_t
#include <stdbool.h>

#include "randgen.h"
#include "shared_logic.h"
#include "screen.h"
#include "buttons.h"


//for array positions for snake for each piece of the snake an x and y value.

struct position
{
    uint8_t x;
    uint8_t y;
};

volatile bool moveSnake; //trigger to move the snake

bool snake_screen(void)
{
    const uint16_t SNAKE[] = {
                              0xFB97, // ***** ***  * ***,
                              0xA955, // * * *  * * * * *,
                              0xAB9D, // * * * ***  *** *,
                              0x0000, //                 ,
                              0x03DF, //       **** *****,
                              0xB881, // * ***   *      *,
                              0x035F, //       ** * *****,
                              0x0000, //
    };
    set_splashscreen(SNAKE);
    waitms(3500);
    if(checkRight(false) && checkLeft(false))
    {
        return true;
    }
    return false;
}

void snake_button_left(uint8_t *direction, uint8_t *previous_direction)
{
    if(*previous_direction != 1)
    {
        *direction = 3;
    }
}

void snake_button_up(uint8_t *direction, uint8_t *previous_direction)
{
    if(*previous_direction != 2)
    {
        *direction = 0;
    }
}

void snake_button_right(uint8_t *direction, uint8_t *previous_direction)
{
    if(*previous_direction != 3)
    {
        *direction = 1;
    }
}

void snake_button_down(uint8_t *direction, uint8_t *previous_direction)
{
    if(*previous_direction != 0)
    {
        *direction = 2;
    };
}

uint8_t genBerry(uint8_t size)
{
    uint8_t temp;
    temp = rnd_get_num();
    temp = temp % size;
    return temp;
}

void CreateBerry(uint8_t *snakeLength, struct position *positions, struct position *berry)
{
    bool randomGood;
    randomGood = 1;
    do
    {
        randomGood = 0;
        berry->x = genBerry(8);
        berry->y = genBerry(16);
        for(uint8_t i = 0; i < *snakeLength; i++)
        {
            if(berry->x == positions[i].x && berry->y == positions[i].y)
            {
                randomGood = 1;
            }
        }
    }
    while(randomGood == 1);
}

void SnakeGraph(uint8_t *snakeLength, struct position *positions, struct position *berry)
{
    uint16_t ObjectData[8];
    clearArray(ObjectData, 8);
    {
        // for every bit 1 turned on rest off i.e. 1000000000000000 = SnakeYtext[0]
        const uint16_t SnakeYtext[] = {0x001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000};
        for(uint8_t i = 0; i <= *snakeLength; i++)
        {
            uint8_t j = positions[i].x;
            ObjectData[j] = (SnakeYtext[positions[i].y] | ObjectData[j]);
        }
        for(uint8_t i = 0; i < 8; i++)
        {
            if(berry->x == i)
            {
                ObjectData[i] = SnakeYtext[berry->y] | ObjectData[i];
            }
        }
    }
    set_screen(ObjectData);
}

void MoveSnakes(uint8_t *snakeLength, bool *EndOfGame, uint8_t *direction, uint8_t *previous_direction, struct position *positions, struct position *berry)
{
    switch(*direction)
    {
        case 0:
            positions[0].y = positions[0].y - 1;
            *previous_direction = 0;
            if(positions[0].y >= 200)
            {
                positions[0].y = 15;
            };
            break;
        case 1:
            positions[0].x = positions[0].x + 1;
            *previous_direction = 1;
            if(positions[0].x > 7 & positions[0].x < 200)
            {
                positions[0].x = 0;
            };
            break;
        case 2:
            positions[0].y = positions[0].y + 1;
            *previous_direction = 2;
            if(positions[0].y > 15 & positions[0].y < 200)
            {
                positions[0].y = 0;
            };
            break;
        case 3:
            positions[0].x = positions[0].x - 1;
            *previous_direction = 3;
            if(positions[0].x >= 200)
            {
                positions[0].x = 7;
            };
            break;
    };
    for(uint8_t i = 0; i <= *snakeLength; i++)
    {
        positions[(*snakeLength - i) + 1].y = positions[*snakeLength - i].y;
        positions[(*snakeLength - i) + 1].x = positions[*snakeLength - i].x;
    }
    if(positions[0].y == berry->y && positions[0].x == berry->x)
    {
        *snakeLength = *snakeLength + 1;
        CreateBerry(snakeLength, positions, berry);
    }
    for(uint8_t i = 4; i <= *snakeLength; i++)
    {
        if(positions[0].y == positions[i].y && positions[0].x == positions[i].x)
        {
            *EndOfGame = true;
        };
    };
    SnakeGraph(snakeLength, positions, berry);
}

void snake_timer(void)
{
    moveSnake = true;
}

void inistialize_snake(uint8_t *snakeLength, struct position *positions, struct position *berry)
{
    moveSnake = false;
    for(uint8_t i = 0; i < 4; i++)
    {
        positions[i].y = 3;
        positions[i].x = 4 - i;
    }
    positions[0].y = 3;
    positions[0].x = 3;
    CreateBerry(snakeLength, positions, berry);
}

void snake_buttons(uint8_t *direction, uint8_t *previous_direction)
{
    if(checkLeft(true))
    {
        snake_button_left(direction, previous_direction);
    }
    if(checkRight(true))
    {
        snake_button_right(direction, previous_direction);
    }
    if(checkUp(true))
    {
        snake_button_up(direction, previous_direction);
    }
    if(checkDown(true))
    {
        snake_button_down(direction, previous_direction);
    }
}

void aibuttons(uint8_t *direction, struct position *positions)
{
    if(*direction == 3)
    {
        if(positions[2].x == positions[1].x + 1 || positions[2].x == positions[1].x - 7)
        {
            if(positions[0].y == 15)
            {
                *direction = 0;
            }
            else if(positions[0].y == 0)
            {
                *direction = 2;
            }
        }
    }
    if(*direction == 0)
    {
        if(positions[0].y == 0)
        {
            *direction = 3;
        }
    }
    if(*direction == 2)
    {
        if(positions[0].y == 15)
        {
            *direction = 3;
        }
    }
}

void snake_main(void)
{
        uint8_t snakeLength = 3; //Length of snake;
        {
            bool ai = snake_screen();
            uint8_t LastHighScore = readHighScore(1);
            show_score(LastHighScore);
            {
                uint8_t direction = 1; //for direction of the snake 0=down 1=right 2=up 3=left
                uint8_t previous_direction = 3; //previous direction so as to not go back the same way
                struct position positions[128]; //array for every snake length (max length equals 128)
                struct position berry;

                inistialize_snake(&snakeLength, positions, &berry);
                bool EndOfGame = false;

                do
                {
                    snake_buttons(&direction, &previous_direction);
                    if(moveSnake == true)
                    {
                        if(ai)
                        {
                            aibuttons(&direction, positions);
                        }
                        moveSnake = false;
                        MoveSnakes(&snakeLength, &EndOfGame, &direction, &previous_direction, positions, &berry);
                    };
                }
                while(!EndOfGame);
            }
            writeHighScore(1, LastHighScore, snakeLength);
        }
        show_score(snakeLength);
}