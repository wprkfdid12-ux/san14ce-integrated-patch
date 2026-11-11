/* Minimal memory functions for the CRT-free DLL build (the compiler may emit calls to them). */
#include <stddef.h>
void *memcpy(void *d, const void *s, size_t n) {
    unsigned char *a = (unsigned char *)d;
    const unsigned char *b = (const unsigned char *)s;
    while (n--) *a++ = *b++;
    return d;
}
void *memset(void *d, int v, size_t n) {
    unsigned char *a = (unsigned char *)d;
    while (n--) *a++ = (unsigned char)v;
    return d;
}
int memcmp(const void *x, const void *y, size_t n) {
    const unsigned char *a = (const unsigned char *)x, *b = (const unsigned char *)y;
    for (; n; n--, a++, b++)
        if (*a != *b) return *a - *b;
    return 0;
}
