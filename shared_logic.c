// Function proto is in string.h
#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "shared_logic.h"
#include "EEPROM.h"
#include "numbers.h"
#include "screen.h"
#include "buttons.h"
/**
 * 8-bit ``optizimed'' version of memcpy()
 */
volatile uint16_t mS;

void set_mS(uint16_t amount)
{
    mS = amount;
}

uint16_t add_mS(uint16_t amount)
{
    mS = mS + amount;
    return mS;
}

uint16_t get_mS(void)
{
    return mS; // mS register
}

void *memcpy(void *dest, const void *src, size_t n)
{
    char *d = (char*) dest, *s = (char*) src;

    while(n--)
    {
        *d++ = *s++;
    }

    return dest;
}

volatile void *memcpyvol(volatile void *dest, const void *src, size_t n)
{
    char *d = (char*) dest, *s = (char*) src;

    while(n--)
    {
        *d++ = *s++;
    }

    return dest;
}

/* A utility function to swap two uint8_ts */
void swap(char* a, char* b)
{
    char t = *a;
    *a = *b;
    *b = t;
}

/* A utility function to reverse a string  */
void reverse(char str[], int length)
{
    int start = 0;
    int end = length - 1;
    while(start < end)
    {
        swap(str + start, str + end);
        start++;
        end--;
    }
}

char * itoa(int value, char * str, int bas)
{
    int i = 0;
    bool isNegative = false;

    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if(value == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    // In standard itoa(), negative numbers are handled only with 
    // base 10. Otherwise numbers are considered unsigned.
    if(value < 0 && bas == 10)
    {
        isNegative = true;
        value = -value;
    }

    // Process individual digits
    while(value != 0)
    {
        int rem = value % bas;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        value = value / bas;
    }

    // If number is negative, append '-'
    if(isNegative)
    {
        str[i++] = '-';
    }

    str[i] = '\0'; // Append string terminator

    // Reverse the string
    reverse(str, i);

    return str;
}

// clears the passed array of type Word

void clearArray(volatile uint16_t *pArray, size_t size)
{
    while(size--)
    {
        pArray[size] = 0;
    }
}

void mergeObjects(volatile uint16_t * pSource, volatile uint16_t * pTarget, mode_t mode)
{
    uint8_t i = 8;
    switch(mode)
    {
        case OVERRIDE:
            while(i--)
            {
                pTarget[i] = pSource[i];
            }
            break;
        case MERGE:
            while(i--)
            {
                pTarget[i] |= pSource[i];
            }
            break;
        case INVERT:
            while(i--)
            {
                pTarget[i] ^= pSource[i];
            }
            break;
    }
}


// check the passed object to see if it is against the left wall already
// return true if so

bool checkForLeftWall(volatile uint16_t * pObject)
{
    return pObject[0] != 0;
}


// check the passed object to see if it is against the right wall already
// return true if so

bool checkForRightWall(volatile uint16_t * pObject)
{
    return pObject[7] != 0;
}

// compare the passed source and target for a collision
// return true if so

bool collisionDetect(volatile uint16_t * pSource, volatile uint16_t * pTarget)
{
    uint8_t i = 8;
    while(i--)
    {
        if(pSource[i] & pTarget[i]) // if two bits overlap, this is true
            return true;
    }
    return false; // no bits overlap, no collision
}

// Count the number of pixels that equal "1". Standard bit-wise operators did not suit
// this routine as pixels could very well end up ANYWHERE after a rotation.

uint8_t pixelCount(volatile uint16_t * pSource)
{
    uint8_t r = 0;
    uint8_t x = 8;
    while(x--)
    {
        uint8_t y = 16;
        while(y--)
        {
            if(GBIT(pSource[x], y))
            {
                r++;
            }
        }
    }
    return r;
}

// remove the line at location pY

void removeLine(volatile uint16_t * pObject, uint8_t pY)
{
    uint8_t y, currentLine;

    // shift each line down
    for(y = pY - 1; y < pY; y--) // y < pY --> y should reach 0 and than wraparound, so this will become true
    {
        currentLine = y + 1;
        uint8_t x = 8;
        while(x--)
        {
            VBIT(pObject[x], currentLine, ((pObject[x] >> y) & 1));
        }
    }
    // clear the top line
    uint8_t x = 8;
    while(x--)
    {
        CBIT(pObject[x], 0);
    }
}

void getNumber(uint8_t pDigit, uint16_t * pTarget)
{
    memcpy(pTarget, Number[pDigit], 16); // 16 = ARRAY_SIZE(Number[pDigit]));
}

uint8_t readHighScore(uint8_t adress)
{
    uint8_t tmpbyte = ReadEEByte(adress);;
    if(tmpbyte == 255)
    {
        tmpbyte = 0;
    }
    if(tmpbyte == 0)
    {
        WriteEEByte(adress, 0);
    };
    return tmpbyte;
}

void writeHighScore(uint8_t adress, uint8_t highscore, uint8_t scored)
{
    if(scored > highscore)
    {
        WriteEEByte(adress, scored);
    };
}

void moveObject(uint16_t * pObject, direction_t direction, uint8_t cycles)
{
    uint8_t i, c;

    switch(direction)
    {
        case DOWN:
            for(c = 0; c < cycles; c++)
            {
                for(i = 0; i < 8; i++)
                    pObject[i] <<= 1;
            }
            break;
        case UP:
            for(c = 0; c < cycles; c++)
            {
                for(i = 0; i < 8; i++)
                    pObject[i] >>= 1;
            }
            break;
        case RIGHT:
            for(c = 0; c < cycles; c++)
            {
                for(i = 7; i > 0; i--)
                    pObject[i] = pObject[i - 1];
                pObject[0] = 0;
            }
            break;
        case LEFT:
            for(c = 0; c < cycles; c++)
            {
                for(i = 0; i < 7; i++)
                    pObject[i] = pObject[i + 1];
                pObject[7] = 0;
            }
    }
}
void show_score(uint8_t score)
{
    uint16_t Numberscreen[8];
    clearArray(Numberscreen, 8);
    {
        char number[4]; // expect up to 3 digits + terminating \0
        itoa(score, number, 10);
        for(uint8_t i = 0; i < 3 && number[i]; i++) // TODO: change "< 3 && number[i]" to "< strnlen(number, 3)"
        {
            uint8_t currentNumber = number[i] - '0';
            uint16_t tmpObjectData[8];
            getNumber(currentNumber, tmpObjectData);
            moveObject(tmpObjectData, DOWN, (2 - i) * 5);
            moveObject(tmpObjectData, RIGHT, i);
            mergeObjects(tmpObjectData, Numberscreen, MERGE);
        }
    }
    pauseMultiplexing();
    set_screen(Numberscreen);
    resumeMultiplexing();

    while(checkDown(false))
    {
        continue;
    }

    while(!checkDown(false))
    {
        continue;
    }

    {
        uint16_t mask[] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
        mergeObjects(mask, Numberscreen, INVERT);
    }
    pauseMultiplexing();
    set_screen(Numberscreen);
    resumeMultiplexing();

    while(checkDown(false))
        continue;
    pauseButtons();
    resumeButtons();
}

