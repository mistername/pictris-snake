#include <stdint.h>

// http://en.wikipedia.org/wiki/Linear_feedback_shift_register

uint16_t _lfsr;

uint16_t rnd_get_num(void)
{
    /* taps: 8 6 5 4; feedback polynomial: x^8 + x^6 + x^5 + x^4 + 1 */
    unsigned b  = ((_lfsr >> 0) ^ (_lfsr >> 1) ^ (_lfsr >> 3) ^ (_lfsr >> 12) ) & 1;
    _lfsr =  (_lfsr >> 1) | (b << 15);
    return _lfsr;
}


void rnd_initialize(uint8_t input)
{
    _lfsr = input;
}