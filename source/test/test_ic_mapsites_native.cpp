// ICGuard K (P225): 다른 모드의 "바꾼 자리" 파일로 통합편의 [포로 회유]·[등용문] 대조 맞추기 — src/ic_mapsites.h 를 그대로 시험
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "../src/ic_mapsites.h"
static int fails;
#define CHECK(c,m) do{if(c)printf("ok   %s\n",m);else{printf("FAIL %s\n",m);fails++;}}while(0)
static const uint32_t ST=0x6ad06f53,IMG=0x2a02000;
static kms::File F;
static void P(const std::string&s,uint32_t pid=777){kms::parse(s.data(),s.size(),ST,pid,IMG,F);}
int main(){
 // 1) 형식
 P("stamp=0x6ad06f53\r\nmod=맵 확장 137\r\npid=777\r\n22c250 1\r\n2fc726 3\r\n2fc72a 5\r\n");
 CHECK(F.ok&&F.n==3&&F.s[0].rva==0x22c250&&F.s[0].len==1&&F.s[2].rva==0x2fc72a&&F.s[2].len==5&&!strcmp(F.mod,"맵 확장 137"),"형식: stamp·mod·pid·자리 3줄(CRLF)");
 P("\xef\xbb\xbfstamp=6ad06f53\n# 주석\n\nmod=x\n  22c250   1  \n");CHECK(F.ok&&F.n==1,"BOM·주석·빈 줄·앞뒤 공백·0x 없는 stamp");
 P("stamp=0x6ad06f53\nmod=x\npid=778\n22c250 1\n");CHECK(!F.ok&&F.pid_seen&&!F.pid_ok,"pid 가 다르면(파일만 남은 경우) 무시");
 P("stamp=0x6ad06f53\nmod=x\n22c250 1\n");CHECK(F.ok,"pid 줄이 없으면 그대로 씀");
 P("stamp=0x6a983c25\nmod=x\n22c250 1\n");CHECK(!F.ok&&F.stamp_seen&&!F.stamp_ok,"게임 판이 다르면 무시");
 P("mod=x\n22c250 1\n");CHECK(!F.ok,"stamp 줄이 없으면 무시");
 P("stamp=0x6ad06f53\nzz 1\n22c250 0\n22c250 999\n0 4\n2a01fff 4\n22c250 1x\n22c250 2\n");CHECK(F.ok&&F.n==1&&F.bad_lines==6&&F.s[0].len==2,"틀린 줄(16진 아님·길이 0·너무 김·이미지 밖·뒤 찌꺼기)은 그 줄만 버림");
 // 2) 적용: 가짜 게임 이미지 + 통합편의 기대 바이트
 static uint8_t game[0x400000];for(unsigned i=0;i<sizeof game;i++)game[i]=(uint8_t)(i*7);
 uint8_t e1[0x20],e2[0x10],e3[8];memcpy(e1,game+0x2fc710,0x20);memcpy(e2,game+0x22c240,0x10);memcpy(e3,game+0x100000,8);
 kms::Entry E[3]={{0x2fc710,0x20,e1},{0x22c240,0x10,e2},{0x100000,8,e3}};
 unsigned ei;uint32_t off;
 CHECK(!kms::first_diff(game,E,3,&ei,&off),"처음엔 대조 통과");
 // 맵 확장처럼 바꿈: 0x2fc710 jmp 5바이트, 0x2fc726 9바이트, 0x22c250 1바이트(33→3c)
 const uint8_t j1[5]={0xe9,1,2,3,4},j2[9]={0xe9,5,6,7,8,0x90,0x90,0x90,0x90};memcpy(game+0x2fc710,j1,5);memcpy(game+0x2fc726,j2,9);game[0x22c250]=0x3c;
 CHECK(kms::first_diff(game,E,3,&ei,&off)&&ei==0&&off==0,"맵 확장이 바꾸면 대조 실패(통합편의 '열기 실패' 상황)");
 auto wr=[](uint8_t*d,const uint8_t*s,uint32_t n){memcpy(d,s,n);};
 P("stamp=0x6ad06f53\nmod=맵 확장 137\n2fc710 5\n2fc726 3\n2fc72a 5\n22c250 1\n");   // 2fc729 은 우연히 같아 빠진 경우
 unsigned ch=kms::apply(game,E,3,F,wr);
 bool rest=kms::first_diff(game,E,3,&ei,&off);
 CHECK(rest&&ei==0&&off==0x19,"목록에서 빠진 바이트(0x2fc729)는 그대로 남음 → 그 자리만 실패로 보고");
 P("stamp=0x6ad06f53\nmod=맵 확장 137\n2fc710 5\n2fc726 9\n22c250 1\n");ch=kms::apply(game,E,3,F,wr);
 CHECK(!kms::first_diff(game,E,3,&ei,&off),"목록 자리를 맞추면 대조 통과");
 CHECK(e3[0]==(uint8_t)(0x100000*7)&&!memcmp(e3,game+0x100000,8),"목록과 안 겹치는 표 줄은 그대로");
 CHECK(kms::apply(game,E,3,F,wr)==0,"두 번째 누름: 이미 맞아 바꿀 것 없음");
 // 표 경계 걸침: 자리가 표 줄 앞뒤로 삐져나와도 겹친 만큼만
 game[0x22c23f]^=0xff;game[0x22c240]^=0xff;P("stamp=0x6ad06f53\n22c23e 4\n");ch=kms::apply(game,E,3,F,wr);
 CHECK(ch==1&&e2[0]==game[0x22c240]&&!kms::first_diff(game,E,3,&ei,&off),"표 줄 경계에 걸친 자리: 겹친 바이트만 맞춤");
 // 다른 모드가 목록 없이 고친 자리
 game[0x100003]^=0x55;CHECK(kms::first_diff(game,E,3,&ei,&off)&&ei==2&&off==3,"목록에 없는 자리가 다르면 그대로 실패(오류 창 대상)");
 P("stamp=0x6a983c25\n100003 1\n");CHECK(kms::apply(game,E,3,F,wr)==0&&kms::first_diff(game,E,3,&ei,&off),"판이 다른 파일로는 안 맞춤");
 printf(fails?"FAILED %d\n":"ic_mapsites: all ok\n",fails);return fails?1:0;
}
