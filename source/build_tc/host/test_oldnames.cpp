#include "mock.h"
#include <stdio.h>
#include <stdlib.h>
using namespace mock;
extern "C" { bool NamesInit(); const wchar_t*NamesSurname(U8*); }
extern const S14_CoreAPI*core27; extern U8*game27;
int main(){init();core27=&api;game27=game;NamesInit();U8*n=(U8*)calloc(1,0x200);*(U8**)n=game+0x13a6b38;readable_calls=0;for(int i=0;i<10000;i++)NamesSurname(n);printf("이전판: 일반 무장 이름 1만 번 읽기에 메모리 확인 %u회\n",readable_calls.load());}
