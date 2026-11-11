#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
int memcmp(const void *, const void *, size_t);
wchar_t *wcscpy(wchar_t *, const wchar_t *);
#ifdef __cplusplus
}
#endif
