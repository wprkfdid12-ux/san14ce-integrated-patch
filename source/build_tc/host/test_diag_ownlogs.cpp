// 시험8 Diag 자기 기록 읽기 검사(리눅스): 실제 PC 기록(11Patch.log UTF-16·TraitHover·DragGive 등)으로 판정 확인
// g++ -std=c++17 -fshort-wchar -I src build_tc/host/test_diag_ownlogs.cpp -o /tmp/tdo
#include "diag_ownlogs.h"
#include <cstdio>
#include <vector>
#include <string>
using namespace ownlog;
static int pass=0,fail=0;static void check(bool c,const char*m){printf("  %s %s\n",c?"통과":"실패",m);c?pass++:fail++;}
static std::vector<char> rd(const char*p){std::vector<char> v;FILE*f=fopen(p,"rb");if(!f)return v;char b[65536];size_t n;while((n=fread(b,1,sizeof b,f))>0)v.insert(v.end(),b,b+n);fclose(f);return v;}
static void show(Hit*h,unsigned n){for(unsigned i=0;i<n;i++)printf("    [%d번 x%u] %s\n",h[i].feature,h[i].count,h[i].text);}
int main(int argc,char**argv){
 const char*G=argc>1?argv[1]:"/mnt/user-data/uploads/Romance_of_the_Three_Kingdoms_14";
 std::string g=G;auto ic=rd((g+"/SAN14CE_11Patch.log").c_str());check(ic.size()>1000,"11Patch.log 읽음");
 const W16*w=(const W16*)ic.data();size_t n=ic.size()/2;
 Hit h[48];unsigned k;
 printf("[통합편의 최신 실행 13:06 — 모드 버튼 정상]\n");
 size_t n13=n;{std::u16string s((const char16_t*)w,n);size_t c=s.find(u"2026-11-07 14:");if(c!=std::u16string::npos)n13=c;}   // 기록이 뒤로 계속 쌓이므로 13:06 실행 끝(14시 첫 줄)에서 자름
 k=scan_ic(w,n13,"2026-11-07 13:05:00",true,h,48,0);show(h,k);check(k==0,"실패 0건");
 // 04:14 실행만 보이게: 그 실행 끝(13:06 시작 줄 앞)까지 자름
 printf("[통합편의 04:14 실행 — 모드 설정 v2 실패하던 판]\n");
 {std::u16string s((const char16_t*)w,n);std::u16string mk=u"2026-11-07 13:06:42";size_t cut=s.find(mk);
  k=scan_ic(w,cut,"2026-11-07 04:14:00",true,h,48,0);show(h,k);
  bool m=false,i=false,c=false,y=false;for(unsigned j=0;j<k;j++){if(h[j].feature==1&&strstr(h[j].text,"모드 설정 v2"))m=true;if(h[j].feature==1&&strstr(h[j].text,"정보 메뉴"))i=true;if(h[j].feature==4&&strstr(h[j].text,"도시 증축"))c=true;if(h[j].feature==4&&strstr(h[j].text,"명승"))y=true;}
  check(k==4&&m&&i&&c&&y,"4건: 1번 모드 설정 v2 / 1번 정보 메뉴 / 4번 도시 증축 / 4번 명승");}
 printf("[통합편의 11-06 21:37 실행 — BGM·진행속도도 실패하던 판]\n");
 {std::u16string s((const char16_t*)w,n);size_t cut=s.find(u"2026-11-06 23:40:17");
  k=scan_ic(w,cut,"2026-11-06 21:37:00",true,h,48,0);show(h,k);
  bool bgm=false,sp=false;for(unsigned j=0;j<k;j++){if(h[j].feature==7)bgm=true;if(h[j].feature==5)sp=true;}
  check(bgm&&sp&&k>=8,"BGM(7번)·진행속도(5번) 포함 8건 이상");}
 printf("[이번 실행 시작 줄이 기준보다 앞이면(지난 실행) 아무것도 안 봄]\n");
 k=scan_ic(w,n13,"2026-11-07 13:07:00",true,h,48,0);check(k==0,"기준 뒤 시작 줄 없음 → 0건(이 경우는 기존 '시작 줄 없음' 검사가 잡음)");
 printf("[개성 9칸 UI 미설치면 'IC↔9 연결 실패'는 정상]\n");
 {const char16_t t[]=u"2026-11-07 13:06:42 지원 실행 파일 확인 완료. x\r\n2026-11-07 13:06:42 개성 9개 UI·설명 애드온 연결 실패.\r\n2026-11-07 13:06:43 무장 원본 직접 읽기: 오류 1 · 실패 아님\r\n";
  size_t tn=sizeof t/2-1;k=scan_ic((const W16*)t,tn,"2026-11-07 13:05:00",false,h,48,0);check(k==0,"미설치: 0건(무장 원본 줄도 제외)");
  k=scan_ic((const W16*)t,tn,"2026-11-07 13:05:00",true,h,48,0);show(h,k);check(k==1&&h[0].feature==9,"설치됨: 9번 1건");}
 printf("[TraitHover 기록 전체 — 이유별로 묶임, duel skipped 제외]\n");
 {auto th=rd((g+"/SAN14_TraitHover.log").c_str());k=scan_ascii(th.data(),th.size(),14,L"개성·전법 마우스 설명",true,"duel skipped",h,48,0);show(h,k);
  bool cb=false,rq=false,ub=false;for(unsigned j=0;j<k;j++){if(strstr(h[j].text,"callback")&&h[j].count==38)cb=true;if(strstr(h[j].text,"required code bytes"))rq=true;if(strstr(h[j].text,"unsupported game build"))ub=true;}
  check(cb&&rq&&ub&&k<=6,"콜백 38줄=1건(x38), 코드 바이트·옛 판 각 1건, duel 0건");}
 printf("[정상 실행 기록들 — 거짓 경보 없음]\n");
 for(const char*f:{"SAN14_DragGive.log","SAN14_DragLearn.log","SAN14_WheelRegion.log","SAN14_MinimapStartup.log","SAN14_Traits9.log","SAN14_Traits9_UI.log"}){auto v=rd((g+"/"+f).c_str());k=scan_ascii(v.data(),v.size(),30,L"x",false,nullptr,h,48,0);show(h,k);char m[120];snprintf(m,sizeof m,"%s: 0건",f);check(k==0,m);}
 printf("[실패 문구 예]\n");
 {const char t[]="DragGive: pattern matches S=1\nDragGive: C1 (panel off) exe+0x6CF9BA\nDragGive: orders preview: window code differs - preview fix off\nMINIMAP 1.5.1: ready, but d3dcompiler_47.dll was not found (nothing can be drawn)\nSAN14 Traits9 UI: SKIPPED - unsupported game build.\n";
  k=scan_ascii(t,sizeof t-1,30,L"x",false,nullptr,h,48,0);show(h,k);check(k==3,"3건('panel off' 는 정상 줄로 봄)");}
 printf("[10번 미설치: 일기토 연결 실패 줄]\n");
 {const wchar_t*t=L"2026-11-07 15:53:00 지원 실행 파일 확인 완료. 게임 코드 복원을 기다립니다.\n2026-11-07 15:53:01 일기토 연출 시험 호환성 검사 실패. 첫 메뉴를 변경하지 않았습니다.\n";
  std::vector<W16> w;for(const wchar_t*q=t;*q;q++)w.push_back((W16)*q);
  k=scan_ic(w.data(),w.size(),"2026-11-07 15:52:00",true,h,48,0,true);check(k==1,"10번 설치: 1건");
  k=scan_ic(w.data(),w.size(),"2026-11-07 15:52:00",true,h,48,0,false);check(k==0,"10번 미설치: 0건");}
 printf("[모드 설정에서 끈 기능의 실패 줄]\n");
 {const wchar_t*t=L"2026-11-07 16:30:00 지원 실행 파일 확인 완료.\n2026-11-07 16:30:01 진행속도 확장 설치 검사 실패. 원래 세 단계 유지.\n2026-11-07 16:30:02 BGM 확장 설치 실패.\n";
  std::vector<W16> w;for(const wchar_t*q=t;*q;q++)w.push_back((W16)*q);
  k=scan_ic(w.data(),w.size(),"2026-11-07 16:29:00",true,h,48,0,true);check(k==2,"다 켜짐: 2건");
  const wchar_t*off[]={L"진행속도"};k=scan_ic(w.data(),w.size(),"2026-11-07 16:29:00",true,h,48,0,true,off,1);check(k==1,"진행속도 끔: 1건(BGM만)");}
 printf("통과 %d 실패 %d\n",pass,fail);return fail?1:0;
}
