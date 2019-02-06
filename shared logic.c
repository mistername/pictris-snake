// Function proto is in string.h
#include <xc.h> // include processor files - each processor file is guarded.  

/**
 * 8-bit ``optizimed'' version of memcpy()
 */
void *memcpy(void *dest, const void *src, size_t n)
{
    char *d = (char*)dest, *s = (char*)src;

    while(n--)
        *d++ = *s++;

    return dest;
}

volatile void *memcpyvol(volatile void *dest, volatile const void *src, size_t n)
{
    char *d = (char*)dest, *s = (char*)src;

    while(n--)
        *d++ = *s++;

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
    int end = length -1;
    while (start < end)
    {
        swap(str+start, str+end);
        start++;
        end--;
    }
}

char * itoa(int value, char * str, int bas)
{
    int i = 0;
    bool isNegative = false;
 
    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (value == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
 
    // In standard itoa(), negative numbers are handled only with 
    // base 10. Otherwise numbers are considered unsigned.
    if (value < 0 && bas == 10)
    {
        isNegative = true;
        value = -value;
    }
 
    // Process individual digits
    while (value != 0)
    {
        int rem = value % bas;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        value = value/bas;
    }
 
    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';
 
    str[i] = '\0'; // Append string terminator
 
    // Reverse the string
    reverse(str, i);
 
    return str;
}

// clears the passed array of type Word
void clearArray(volatile uint16_t *pArray, size_t size)
{
    uint8_t i;
    for (i = 0; i < size; i++)
        pArray[i] = 0;
}

void mergeObjects(volatile uint16_t * pSource, volatile uint16_t * pTarget, mode_t mode)
{
    uint8_t i;

    switch (mode)
    {
    case OVERRIDE:
        for (i = 0; i < 8; i++)
            pTarget[i] = pSource[i];
        break;
    case MERGE:
        for (i = 0; i < 8; i++)
            pTarget[i] |= pSource[i];
        break;
    case INVERT:
        for (i = 0; i < 8; i++)
            pTarget[i] ^= pSource[i];
        break;
    }
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
    for (uint8_t i = 0; i < 8; i++)
        if (pSource[i] & pTarget[i]) // if two bits overlap, this is true
            return true;
    return false; // no bits overlap, no collision
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

// Count the number of pixels that equal "1". Standard bit-wise operators did not suit
// this routine as pixels could very well end up ANYWHERE after a rotation.
uint8_t pixelCount(volatile uint16_t * pSource)
{
    uint8_t r = 0, x, y;

    for (x = 0; x < 8; x++)
        for (y = 0; y < 16; y++)
            if (GBIT(pSource[x], y))
                r++;
    return r;
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
    mS = 0; // reset the timer because of the moveDown()
    // resume multiplexing, shared resources are done with
    resumeMultiplexing();
    return result;
}

void getNumber(uint8_t pDigit, uint16_t * pTarget)
{
    memcpy(pTarget, Number[pDigit], 16); // 16 = ARRAY_SIZE(Number[pDigit]));
}

void readHighScore(uint8_t adress)
{
    uint8_t tmpbyte;
    if (tetris == false)
    { adress = adress + 1;};
 
    tmpbyte = ReadEEByte(adress);
    if (tmpbyte == 0){
       WriteEEByte(adress,0); 
    };
    if (tmpbyte == 255) {
        tmpbyte = 0;
    }
    LastHighScore = tmpbyte;
}

void writeHighScore(uint8_t adress)
{
    if (tetris == true){
    if (NumberOfLines > LastHighScore){ 
        WriteEEByte(adress,NumberOfLines);
    };
    }
    else if (tetris == false) {
        if (snakeLength > LastHighScore){
        WriteEEByte(adress,snakeLength);
        };
    };
}
 