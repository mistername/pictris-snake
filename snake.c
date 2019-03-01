#include <xc.h>
#include <stdint.h>     // - for uint8_t/uint16_t/int16_t
#include <stdbool.h>

#include "randgen.h"
#include "shared_logic.h"
#include "screen.h"
#include "buttons.h"

volatile bool game = false;

volatile uint16_t ObjectData[8];
volatile uint16_t BackgroundData[8];
uint16_t tmpObjectData[8];

const uint16_t SNAKE[] =
{
    0xFB97, // ***** ***  * ***,
    0xA955, // * * *  * * * * *,
    0xAB9D, // * * * ***  *** *,
    0x0000, //                 ,
    0x03DF, //       **** *****,
    0xB881, // * ***   *      *,
    0x035F, //       ** * *****,
    0x0000, //
};

void snake_screen(void){
    memcpyvol(ObjectData,SNAKE,16);
}

// for every bit 1 turned on rest off i.e. 1000000000000000 = SnakeYtext[0]
const uint16_t SnakeYtext[] = {0x001,0x0002 ,0x0004 ,0x0008 ,0x0010 ,0x0020 ,0x0040 ,0x0080 ,0x0100 ,0x0200 ,0x0400 ,0x0800 ,0x1000 ,0x2000 ,0x4000 ,0x8000 };

//for array positions for snake for each piece of the snake an x and y value.
struct position{
    uint8_t x;
    uint8_t y;
};
uint8_t direction; //for direction of the snake 0=down 1=right 2=up 3=left
volatile bool moveSnake; //trigger to move the snake
struct position positions[128]; //array for every snake length (max length equals 100)
uint8_t snakeLength; //Length of snake;
//position of berry on screen
uint8_t berryX;
uint8_t berryY;
uint8_t previous_direction; //previous direction so as to not go back the same way
volatile bool EndOfGameSnake;

void snake_button_left(void){if (previous_direction != 1){ direction = 3 ;}}
void snake_button_up(void){if (previous_direction != 2){ direction = 0 ;}}
void snake_button_right(void){if (previous_direction != 3){ direction = 1 ;}}
void snake_button_down(void){if (previous_direction != 0){ direction = 2 ;};}

uint8_t genBerry(uint8_t size)
{
    uint8_t temp;
    temp = rnd_get_num();
    //add_counter(1);
    temp = temp % size;
    return temp;
}
void CreateBerry(void)
{
    bool randomGood;
    uint8_t i;
    randomGood = 1;
    do
    {
        randomGood = 0;
        berryX = genBerry(8);
        berryY = genBerry(16);
        for(i=0;i<snakeLength;i++)
        {
            if (berryX == positions[i].x && berryY == positions[i].y)
            {
                randomGood = 1;
            }
        }
    } while (randomGood == 1);
}

void SnakeGraph(void)
{
    uint8_t i;
    uint8_t j;
    clearArray(tmpObjectData, 8);
    for(i=0;i<=snakeLength;i++){
        j = positions[i].x;
        tmpObjectData[j] = (SnakeYtext[positions[i].y] | tmpObjectData[j]);
    }
    for(i=0;i<8;i++){
        if (berryX == i){
            tmpObjectData[i] = SnakeYtext[berryY] | tmpObjectData[i];
        }
    }
    clearArray(ObjectData, 8);
    for(i=8;i!=0;i++){
        ObjectData[i] = tmpObjectData[i];
    }
    set_screen(ObjectData);
}

void MoveSnakes()
{
    uint8_t i;
    switch (direction){
        case 0:
            positions[0].y = positions[0].y - 1;
            previous_direction = 0;
            if (positions[0].y >= 200){
                positions[0].y = 15;
            };
            break;
        case 1:
            positions[0].x = positions[0].x + 1;
            previous_direction = 1;
            if (positions[0].x > 7 & positions[0].x < 200){
                positions[0].x = 0;
            };
            break;
        case 2:
            positions[0].y = positions[0].y + 1;
            previous_direction = 2;
            if (positions[0].y > 15 & positions[0].y < 200) {
                positions[0].y = 0;
            };
            break;
        case 3:
            positions[0].x = positions[0].x - 1;
            previous_direction = 3;
            if (positions[0].x >= 200){ 
                positions[0].x = 7;
            };
            break;
    };
    for (i=0;i<=snakeLength;i++){
        positions[(snakeLength - i)+1].y = positions[snakeLength-i].y;
        positions[(snakeLength - i)+1].x = positions[snakeLength-i].x; 
    }
    if (positions[0].y == berryY && positions[0].x == berryX){
        snakeLength = snakeLength + 1;
        CreateBerry();
    }
    for (i=4;i<=snakeLength;i++){
        if (positions[0].y == positions[i].y && positions[0].x == positions[i].x){
            EndOfGameSnake = 1;
        };
    };
    SnakeGraph();
}

bool snake_timer(void){
    if (game){
    moveSnake = true;
    }
    return game;
}

void inistialize_snake(void) {
    moveSnake = false;
    snakeLength = 3;
    previous_direction = 3;
    uint8_t i;
    for (i=0;i<4;i++){
        positions[i].y = 3;
        positions[i].x = 4-i;
    }
    positions[0].y = 3;
    positions[0].x = 3;
    direction = 1;  
    CreateBerry();
    game = true;
}

void snake_main(void){
    uint8_t LastHighScore = readHighScore(1);
    show_score(LastHighScore);
    EndOfGameSnake        = false;
    do{
        if (moveSnake == true) 
        {
            moveSnake = false;
            MoveSnakes();
        };
    } while(!EndOfGameSnake);
    writeHighScore(1, LastHighScore, snakeLength);
    show_score(snakeLength);
}