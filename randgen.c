#include "randgen.h"
#include "EEPROM.h"

#ifdef RANDWIKI
// http://en.wikipedia.org/wiki/Linear_feedback_shift_register

#ifdef RANDGENPLUS
//https://electronics.stackexchange.com/questions/178053/breaking-a-16-bit-long-int-to-write-into-eeprom
uint16_t _lfsr;
uint8_t lower_8bits;
uint8_t upper_8bits;
volatile uint8_t counter;

void add_counter(uint8_t add)
{
    counter += add;
}
uint8_t rnd_get_num(void)
{
    /* taps: 8 6 5 4; feedback polynomial: x^8 + x^6 + x^5 + x^4 + 1 */
    unsigned b  = ((_lfsr >> 0) ^ (_lfsr >> 1) ^ (_lfsr >> 3) ^ (_lfsr >> 12) ) & 1;
    _lfsr =  (_lfsr >> 1) | (b << 15);
    if (counter >= 50){
    lower_8bits = _lfsr & 0xff;
    WriteEEByte(UINT8_MAX, lower_8bits);
    upper_8bits = (_lfsr >> 8) & 0xff;
    WriteEEByte(UINT8_MAX-1, upper_8bits);
    counter = 0;
    }
    return _lfsr;
}
/*
uint8_t rnd_get_namm(int feedback)
{
    //taps: 8 6 5 4; feedback polynomial: x^8 + x^6 + x^5 + x^4 + 1
    unsigned b  = ((_lfsr >> 0) ^ (_lfsr >> 2) ^ (_lfsr >> 3) ^ (_lfsr >> 4) ) & feedback;
    _lfsr =  (_lfsr >> 1) | (b << 7);
    return _lfsr;
} 
*/


void rnd_initialize(void)
{
    lower_8bits = ReadEEByte(UINT8_MAX);
    upper_8bits = ReadEEByte(UINT8_MAX-1);
    _lfsr = (upper_8bits << 8) | lower_8bits;
}
#else
uint8_t _lfsr = -1;

uint8_t rnd_get_num(void)
{
    /* taps: 8 6 5 4; feedback polynomial: x^8 + x^6 + x^5 + x^4 + 1 */
    unsigned b  = ((_lfsr >> 0) ^ (_lfsr >> 2) ^ (_lfsr >> 3) ^ (_lfsr >> 4) ) & 1;
    _lfsr =  (_lfsr >> 1) | (b << 7);
    return _lfsr;
}
/*
uint8_t rnd_get_namm(int feedback)
{
    //taps: 8 6 5 4; feedback polynomial: x^8 + x^6 + x^5 + x^4 + 1
    unsigned b  = ((_lfsr >> 0) ^ (_lfsr >> 2) ^ (_lfsr >> 3) ^ (_lfsr >> 4) ) & feedback;
    _lfsr =  (_lfsr >> 1) | (b << 7);
    return _lfsr;
} 
*/
void rnd_initialize(uint8_t seed)
{
    _lfsr = seed;
}
#endif

#elif defined RANDBYTE
//*****************************************************************************
//*  Name    : RandByte.BAS                                                   *
//*  Author  : Gavin Wiggett                                                  *
//*          :                                                                *
//*  Date    : 20/05/2013                                                     *
//*  Version : 1.0                                                            *
//*  Notes   : call InitializeRNDbyte(pValue) with a value between 0-255      *
//*          : call GetRNDbyte() to get a Byte 0 to 255                       *
//*          : many thanks goes to David Eather for the original code.        *
//*****************************************************************************

uint8_t _lcg   = 84;
uint8_t _glfsr = 1;

uint8_t rnd_get_num(void)
{
    //LCG
    _lcg = (uint8_t)(7 * _lcg + 0x11);

    //Galios LFSR
    if ((_glfsr & 1) == 1)
    {
        _glfsr = (uint8_t)(((_glfsr ^ 0x87) >> 1) | 0x80); //135 is the tap
    }
    else
    {
        _glfsr = (uint8_t)(_glfsr >> 1);
    }
    return (uint8_t)(_glfsr ^ _lcg);
}

void rnd_initialize(uint8_t seed)
{
    if (seed != 0)
        _lcg = seed;
    else
        _lcg = 84;
    _glfsr = (uint8_t)(_lcg ^ 0x55); //just making the start values very different - not realy important
    if (_glfsr == 0)   //except that GLFSR must not be zero
        _glfsr = 1;
}


#elif defined RANDGEN

//*****************************************************************************
//*  Name    : RandGen.BAS                                                    *
//*  Author  : Gavin Wiggett                                                  *
//*          :                                                                *
//*  Date    : 20/05/2013                                                     *
//*  Version : 1.0                                                            *
//*                                                                           *
//*  Notes   : A Rudimentary Pseudo Random Number Generator (Modulo based)    *
//*  Usage   : Call Initialize() to initialize the Initial value of the       *
//*          : generator. The generator gives better values when the initial  *
//*          : seed value is a Prime Number.                                  *
//*          : For the same Initial Seed, you will get the same serie of      *
//*          : generated values. This let you repeat some experiences (and    *
//*          : this is why it//s called a PSEUDO-random number generator.      *
//*          : If you need an automatic different initial value each time you *
//*          : start the number generator, you can set the initial value to   *
//*          : the read of an ADC value on a FLOATING Analog PIN of a PIC.    *
//*          : Call Rand() to get/generate a new random value                 *
//*          :                                                                *
//*          : You can try to change the Magic Values to change the           *
//*          : Pseudo-Random Number Generator Behaviour                       *
//*          :                                                                *
//*          : Many thanks to Ahmed Lazreg (Octal) for the original code      *
//*****************************************************************************

const uint32_t _MagicA = 1103515245ul;
const uint32_t _MagicB = 12345ul;

uint32_t _seed;

//****************************************************************************
//* Name    : GetRndNum()                                                    *
//* Purpose : Return a new Pseudo Random Number each time called             *
//****************************************************************************
uint16_t rnd_get_num(void)
{
   _seed = _seed * _MagicA + _MagicB;
   return _seed >> 16;
}

//****************************************************************************
//* Name    : InitializeRandGen()                                            *
//* Purpose : Initialize the Random number generator                         *
//*         : The initial value could be a Value read from a Floating Analog *
//*         : PIC Pin.                                                       *
//****************************************************************************
void rnd_initialize(uint16_t seed)
{
    _seed = seed;
}

//****************************************************************************
//* Name    : SetRndMax()                                                    *
//* Purpose : Sets the Max Value that the Random number gen can generate     *
//****************************************************************************
void rnd_set_max(uint16_t rnd_max)
{
    _rnd_max = rnd_max;
}

#endif