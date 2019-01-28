/** memcpy
 * @author Jeroen Bol
 */
// Function proto is in string.h
#include <string.h>

/**
 * 8-bit ``optizimed'' version of memcpy()
 */
void *memmove(void *dest, const void *src, size_t n)
{
    char *d = (char*)dest, *s = (char*)src;

    if (d > s && (d+n) > s ||
        d+n < s            ) // forward copy is ok
    {
        while(n--)
            *d++ = *s++;
    }
    else // reverse copy required
    {
        d += n;
        s += n;
        while (n--)
            *d-- = *s--;
    }
    return dest;
}
