/* CRT-free helpers. Built with -fno-builtin so these never call themselves. */
#include <stddef.h>
void *memcpy(void *d, const void *s, size_t n) { volatile unsigned char *o = d; const unsigned char *i = s; while (n--) *o++ = *i++; return d; }
void *memmove(void *d, const void *s, size_t n) {
  volatile unsigned char *o = d; const unsigned char *i = s;
  if (o < i) { while (n--) *o++ = *i++; } else { o += n; i += n; while (n--) *--o = *--i; }
  return d;
}
void *memset(void *d, int c, size_t n) { volatile unsigned char *o = d; while (n--) *o++ = (unsigned char)c; return d; }
int memcmp(const void *a, const void *b, size_t n) {
  const unsigned char *x = a, *y = b;
  for (; n; --n, ++x, ++y) if (*x != *y) return *x < *y ? -1 : 1;
  return 0;
}
wchar_t *wcscpy(wchar_t *d, const wchar_t *s) { wchar_t *r = d; while ((*d++ = *s++)) {} return r; }
