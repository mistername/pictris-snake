/** memcpy 
 * @author Jeroen Bol
 */
// Function proto is in string.h
#include <string.h>

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

