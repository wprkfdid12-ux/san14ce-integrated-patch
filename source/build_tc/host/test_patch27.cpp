// 27번 R3 실행 검사: 설정창 6페이지 연결(설정창 갱신 경로와 무관), 내정 메뉴 연결 유지, 이름 처리 부담
#include "mock.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
using namespace mock;
extern "C" { int SAN14_ModInitV1(const S14_CoreAPI*); unsigned Patch27Flags(); extern unsigned installed27; bool NamesInit();
 const wchar_t*NamesSurname(U8*); void Patch27UiTick(); int PostBusy(U8*); void Patch27SetFlags(unsigned); int FatalBattle(U8*,U8*,int,int,int,void*,int,int,int); int DeathAllowed(U8*); int EmployRemoteAllowed(U8*,U8*);
 int HexSteps(int,int); long long TravelDays(U8*,U8*,void*,U8*,void*); }
extern long long (*g_orig230)(U8*,U8*,void*,U8*,void*);
static U8 cells_mem[48400];static long long path_days=1;
extern "C" { static int m_cellidx(void*p){return int(((U8*)p-cells_mem));} static long long m_days(int hops,U8*,void*){return (hops*4+9)/10<1?1:(hops*4+9)/10;} static long long m_orig(U8*,U8*,void*,U8*,void*){return path_days;} }
static int bfs_steps(int a,int b){static int dist[48400];for(int&d:dist)d=-1;static int q[48400];int h=0,t=0;q[t++]=a;dist[a]=0;
 while(h<t){int c=q[h++];if(c==b)return dist[c];int x=c%220,y=c/220,odd=x&1;int nb[6][2]={{x,y-1},{x,y+1},{x-1,odd?y-1:y},{x+1,odd?y-1:y},{x-1,odd?y:y+1},{x+1,odd?y:y+1}};
  for(auto&n:nb){if(n[0]<0||n[0]>219||n[1]<0||n[1]>219)continue;int k=n[1]*220+n[0];if(dist[k]<0){dist[k]=dist[c]+1;q[t++]=k;}}}return -1;}
static int seen_dying=-1,seen_killer=-1,fatal_calls=0;
extern "C" { static int m_fatal(U8*a,U8*b,int reason,int,int notify,void*,int,int history,int silent){++fatal_calls;seen_dying=DeathAllowed(a);seen_killer=DeathAllowed(b);return reason==1&&notify==1&&history==1&&silent==0?7:0;} }
static int busy_val=1,order_type=0;static U8*order_obj;
extern "C" { static int m_busy(void*){return busy_val;} static U8*m_order(void*){return order_obj;} static int m_valid(void*p){return p!=nullptr;} static int m_type(void*){return order_type;} }
static U8 *ic,*dm,*owner;static int ic_render_calls,dom_render_calls,ic_click_calls,dom_click_calls;
static std::map<int,U8*> widgets;static std::map<U8*,std::u16string> labels;static std::map<U8*,int> shown;
static void (*dom_render_tramp)(U8*);static UPTR (*dom_click_tramp)(U8*,U8*);
extern "C" {
static void m_ic_render(U8*){++ic_render_calls;}
static UPTR m_ic_click(U8*,U8*c){++ic_click_calls;return 0;}
static void m_dom_render(U8*o){++dom_render_calls;dom_render_tramp(o);}
static UPTR m_dom_click(U8*o,U8*c){++dom_click_calls;return dom_click_tramp(o,c);}
static U8*m_widget(U8*,int id){if(!widgets.count(id)){U8*w=(U8*)calloc(1,0x100);*(int*)(w+0x80)=id;w[0x40]=7;widgets[id]=w;}return widgets[id];}
static void m_label(U8*w,const wchar_t*s){labels[w]=mock::w(s);}
static void m_visible(U8*w,int y){shown[w]=y;}
static void m_enable(U8*,int){}
static U8 m_one8(void*){return 1;}
static U8 m_one8b(void*,int){return 1;}
static unsigned m_wait(){return 1;}
}
static void jmp_rel(U8*at,U8*to){at[0]=0xe9;int32_t d=(int32_t)(to-(at+5));memcpy(at+1,&d,4);}
static void abs_jmp(U8*at,void*to){at[0]=0xff;at[1]=0x25;memset(at+2,0,4);memcpy(at+6,&to,8);}
static std::string u8(const std::u16string&s){std::string r;for(char16_t c:s){if(c<0x80)r+=char(c);else if(c<0x800){r+=char(0xc0|c>>6);r+=char(0x80|(c&63));}else{r+=char(0xe0|c>>12);r+=char(0x80|((c>>6)&63));r+=char(0x80|(c&63));}}return r;}
int main(){
 init();printf("[27번 메뉴·이름 검사]\n");
 native(0x92c390,(void*)m_widget);native(0x85c250,(void*)m_label);native(0x873ab0,(void*)m_visible);native(0x14020,(void*)m_enable);
 ic=alloc_exec(0x200000,(UPTR)game+0x10000000);modules[u"SAN14_IntegratedConvenience.dll"]=(HMODULE)ic;
 digests[u"SAN14_IntegratedConvenience.dll"]=hex("fa07cd5fdf5c26ab6decceb933f6256a73d2d0fcf05e95102f82812f3415da0d");procs["SAN14_WaitConvenienceV1"]=(void*)m_wait;
 const U8 r0[]={0x40,0x53,0x55,0x56,0x57,0x5f,0x5e,0x5d,0x5b};memcpy(ic+0x59100,r0,9);thunk(ic+0x59109,(void*)m_ic_render);
 const U8 c0[]={0x40,0x53,0x56,0x48,0x81,0xec,0x88,1,0,0,0x48,0x81,0xc4,0x88,1,0,0,0x5e,0x5b};memcpy(ic+0x297f0,c0,sizeof c0);thunk(ic+0x297f0+sizeof c0,(void*)m_ic_click);
 thunk(ic+0x493d0,(void*)m_one8);thunk(ic+0x280c0,(void*)m_one8b);
 owner=(U8*)calloc(1,0x600);*(int*)(owner+0x140)=0x52;*(U8**)(ic+0x1c10f0)=owner;ic[0x1c1100]=1; // 설정창이 열려 있는 상태로 시작
 // 내정자동화(DomesticCE)가 자기 시작 때 하는 메뉴 연결을 실제와 같은 모양(E9 -> FF25 중계)으로 재현
 dm=alloc_exec(0x30000);modules[u"SAN14_DomesticCE.dll"]=(HMODULE)dm;digests[u"SAN14_DomesticCE.dll"]=hex("468536c0ebb82a8c6d34f0b32f77f65e9b5bd6723d798a2a2f62f958d7589a8f");
 thunk(dm+0x23c0,(void*)m_dom_render);thunk(dm+0x25f0,(void*)m_dom_click);
 U8*b1=alloc_exec(0x1000,(UPTR)ic+0x59100);abs_jmp(b1,dm+0x23c0);memcpy(b1+0x20,r0,5);abs_jmp(b1+0x25,ic+0x59105);dom_render_tramp=(void(*)(U8*))(b1+0x20);jmp_rel(ic+0x59100,b1);
 U8*b2=alloc_exec(0x1000,(UPTR)ic+0x297f0);abs_jmp(b2,dm+0x25f0);memcpy(b2+0x20,c0,10);abs_jmp(b2+0x2a,ic+0x297fa);dom_click_tramp=(UPTR(*)(U8*,U8*))(b2+0x20);jmp_rel(ic+0x297f0,b2);memset(ic+0x297f5,0x90,5);
 // 관직 수여 훅 자리 2곳에 원본과 같은 call 바이트를 두어 한 묶음은 실제로 연결되게 함 (연결 0건이면 모듈이 작업 저장소를 반납함)
 for(unsigned at:{0x72ce95u,0x72d4ebu}){game[at]=0xe8;int32_t d=int32_t(0x231770-at-5);memcpy(game+at+1,&d,4);}
 // 타 세력 재야 등용 묶음(R12): 목록 판정 2곳 + 등용 명령의 매일 영토 확인 2곳을 원본과 같은 바이트로 둠
 {U8 c[5]={0xe8};int32_t d=int32_t(0x1eedf0-0x6dd049-5);memcpy(c+1,&d,4);memcpy(game+0x6dd049,c,5);
  const U8 e[]={0x3b,0xd8,0x0f,0x85,0xad,0,0,0};memcpy(game+0x1eef58,e,8);
  const U8 ea[]={0xe8,0xec,0x46,0x04,0x00,0x0f,0xb6,0x4e,0x71,0x38,0x48,0x14,0xeb,0x3a};memcpy(game+0x1e83ef,ea,14);
  const U8 eb[]={0xe8,0x23,0x43,0x04,0x00,0x0f,0xb6,0x4e,0x71,0x38,0x48,0x14,0xeb,0x3a};memcpy(game+0x1e87b8,eb,14);
  const U8 td[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18};memcpy(game+0x230a40,td,15);}
 SAN14_ModInitV1(&api);check((installed27&2)!=0,"관직 수여 훅 묶음 연결");
 check((installed27&1)!=0&&game[0x1e83ef]==0xe9&&game[0x1e87b8]==0xe9,"타 세력 재야 등용: 목록 + 등용 명령 매일 영토 확인 2곳이 한 묶음으로 연결 (R12)");check(game[0x230a40]==0xe9,"이동 일수 계산 시작점 연결 (R13, 별도 묶음)");installed27=7; // 나머지 기능도 연결된 상태를 가정
 usleep(80000);
 check(ic[0x59100]==0xe9&&ic+0x59105+*(int32_t*)(ic+0x59101)==b1,"설정창이 열려 있는 동안에는 화면 처리 코드를 바꾸지 않음");
 ic[0x1c1100]=0; // 설정창 닫힘
 for(int i=0;i<100&&ic+0x59105+*(int32_t*)(ic+0x59101)==b1;i++)usleep(10000);
 check(ic+0x59105+*(int32_t*)(ic+0x59101)!=b1,"설정창 갱신 함수가 한 번도 불리지 않아도 시작 직후 6페이지 연결");
 ic[0x1c1100]=1;
 auto render=(void(*)(U8*))(ic+0x59100);auto click=(UPTR(*)(U8*,U8*))(ic+0x297f0);
 *(int*)(ic+0x1c1140)=5;render(owner);
 check(ic_render_calls==1&&dom_render_calls==1,"6페이지 표시 때 내정자동화 메뉴와 원래 메뉴 처리가 그대로 이어서 실행");
 check(u8(labels[widgets[0x3c]])=="처음 (6/6)","6페이지 버튼 글자 '처음 (6/6)'");
 check(u8(labels[widgets[0x14]])=="타 세력 도시 재야 등용"&&u8(labels[widgets[0x1c]])=="행동 중 무장 관직 수여"&&u8(labels[widgets[0x24]])=="전사 일러스트·메시지·기록","세 기능 이름 표시");
 check(u8(labels[widgets[0x18]])=="ON"&&u8(labels[widgets[0x20]])=="ON"&&u8(labels[widgets[0x28]])=="ON","처음 설치 시 세 기능 모두 ON");
 check(shown[widgets[0x2c]]==0&&shown[widgets[0x34]]==0,"4·5번째 줄(이름 확장 등)은 숨김");
 *(int*)(ic+0x1c1140)=4;render(owner);check(u8(labels[widgets[0x3c]])=="다음 (5/6)","5페이지 버튼 글자 '다음 (5/6)'");
 tick+=1000;click(owner,m_widget(nullptr,0x3c));check(*(int*)(ic+0x1c1140)==5,"5페이지에서 '다음'을 누르면 6페이지로 이동");
 unsigned f=Patch27Flags();tick+=1000;click(owner,m_widget(nullptr,0x18));
 check((Patch27Flags()&1)==0&&(f&1)==1&&u8(labels[widgets[0x18]])=="OFF","첫 줄 ON→OFF 전환 및 표시");
 tick+=1000;click(owner,m_widget(nullptr,0x28));check((Patch27Flags()&4)==0&&u8(labels[widgets[0x28]])=="OFF","셋째 줄 ON→OFF 전환");
 tick+=1000;click(owner,m_widget(nullptr,0x18));check((Patch27Flags()&1)==1&&u8(labels[widgets[0x18]])=="ON","다시 누르면 OFF→ON");
 tick+=1000;click(owner,m_widget(nullptr,0x3c));check(*(int*)(ic+0x1c1140)==0,"6페이지에서 '처음'을 누르면 1페이지로");
 int before=ic_click_calls;*(int*)(ic+0x1c1140)=1;tick+=1000;click(owner,m_widget(nullptr,0x18));check(ic_click_calls==before+1&&dom_click_calls>=1,"1~4페이지 클릭은 원래 처리(내정자동화 경유)로 전달");
 Patch27UiTick();check(true,"설정창 갱신 경로가 뒤늦게 불려도 중복 연결 없음");
 // 이름 처리 부담: 일반 무장 객체는 운영체제 메모리 확인을 하지 않아야 함
 NamesInit();
 U8*normal=(U8*)calloc(1,0x200);*(U8**)normal=game+0x13a6b38;readable_calls=0;
 for(int i=0;i<10000;i++)NamesSurname(normal);
 check(readable_calls==0,"일반 무장 이름 1만 번 읽기에 메모리 확인 0회 (이전판은 1만 번 이상)");
 U8*ed=(U8*)calloc(1,0x4a0);*(U8**)ed=game+0x13a41d0;readable_calls=0;const wchar_t*s=NamesSurname(ed);
 check(readable_calls==1&&s==(const wchar_t*)(ed+0xc),"신무장 편집 기록은 기존처럼 확인 후 처리");
 // 관직 수여(R9): 행동 중인 무장 전체 허용, R2가 제외하던 명령 종류 1가지만 유지
 native(0x231770,(void*)m_busy);native(0x229fc0,(void*)m_order);native(0x32e4c0,(void*)m_valid);
 U8*p=(U8*)calloc(1,0x200);*(U8**)p=game+0x13a6b38;p[0xe6]=1;
 busy_val=0;check(PostBusy(p)==0,"원래 판정이 '행동 중 아님'이면 그대로");
 busy_val=1;check(PostBusy(p)==0,"명령 없이 행동 중(부대원 등)인 무장도 관직 수여 허용");
 U8**vt=(U8**)calloc(1,0x100);vt[0x98/8]=(U8*)m_type;order_obj=(U8*)calloc(1,0x40);*(U8***)order_obj=vt;
 order_type=3;check(PostBusy(p)==0,"명령 수행 중인 무장도 허용 (출진 지휘관 외 포함)");
 order_type=0x10;check(PostBusy(p)==1,"R2가 제외하던 명령 종류(0x10)만 원래대로 불가");
 unsigned keep=Patch27Flags();Patch27SetFlags(keep&~2u);order_type=3;check(PostBusy(p)==1,"기능 OFF면 원래 판정으로 복귀");Patch27SetFlags(keep);
 // 전사 알림(R10): 원본 전사 처리에 넘기는 동안 '죽는 무장'(첫째 값)이 허용 대상이어야 함 (R2는 둘째 값=상대 무장을 넣었음)
 native(0x294f20,(void*)m_fatal);U8*dying=(U8*)calloc(1,0x200);U8*killer=(U8*)calloc(1,0x200);Patch27SetFlags(keep|4u); // 전사 알림 ON (위에서 OFF로 바꿔 둔 상태 복구)
 int r=FatalBattle(dying,killer,1,0,1,nullptr,1,1,0);
 check(r==7&&fatal_calls==1,"원본 전사 처리를 아홉 값 그대로 한 번 호출");
 check(seen_dying==1,"처리 중에는 죽는 무장(첫째 값)이 전사 알림 대상");
 check(seen_killer==0,"상대 무장(둘째 값)은 대상이 아님 — R2의 바뀐 자리 수정");
 check(DeathAllowed(dying)==0,"처리가 끝나면 대상 해제");
 Patch27SetFlags(keep&~4u);FatalBattle(dying,killer,1,0,1,nullptr,1,1,0);check(seen_dying==0,"전사 알림 OFF면 대상 없음(원래 판정)");Patch27SetFlags(keep);
 // R12: 등용 명령 중 대상 재야의 도시가 우리 영토가 아닐 때 — 기능 ON + 아직 재야면 계속, 아니면 순정처럼 취소
 {unsigned keep2=Patch27Flags();U8*ronin=(U8*)calloc(1,0x200);ronin[0xe6]=5;U8*order=(U8*)calloc(1,0x100);
  Patch27SetFlags(keep2|1u);check(EmployRemoteAllowed(order,ronin)==1,"기능 ON: 타 세력 도시의 재야면 등용 이동을 취소하지 않음");
  ronin[0xe6]=1;check(EmployRemoteAllowed(order,ronin)==0,"대상이 이미 임관(재야 아님)이면 순정처럼 취소");ronin[0xe6]=5;
  Patch27SetFlags(keep2&~1u);check(EmployRemoteAllowed(order,ronin)==0,"기능 OFF면 순정 판정(취소)");Patch27SetFlags(keep2);}
 // R13: 이동 일수 — 순정 길찾기가 실패해 '1걸음(1일)'로 나오면 직선 육각 칸 수로 계산, 길찾기가 되면 순정 그대로
 {int bad=0;for(int i=0;i<4000;i++){int a=rand()%48400,b=rand()%48400;if(HexSteps(a,b)!=bfs_steps(a,b))bad++;}
  check(bad==0,"육각 칸 수 계산이 게임 이웃 규칙(0x2a6b80) 최단 걸음과 4000쌍 모두 일치");
  native(0x22d2c0,(void*)m_cellidx);native(0x230cc0,(void*)m_days);g_orig230=m_orig;unsigned keep3=Patch27Flags();Patch27SetFlags(keep3|1u);
  // 가상의 칸 번호: 출발 0, 도착 (x=40,y=30) → 직선 40칸 이상
  U8*from=cells_mem;U8*to=nullptr;int far=30*220+40;to=cells_mem+far; // 포인터 차이 = 칸 번호
  int steps=HexSteps(0,far);path_days=m_days(1,nullptr,nullptr);
  check(TravelDays(from,to,nullptr,nullptr,nullptr)==m_days(steps,nullptr,nullptr),"길찾기 실패(1일): 직선 칸 수 기준 일수로 바뀜");
  path_days=m_days(steps+7,nullptr,nullptr);
  check(TravelDays(from,to,nullptr,nullptr,nullptr)==path_days,"길찾기 성공: 순정 일수 그대로 (실제 길은 항상 직선 이상)");
  path_days=1;check(TravelDays(from,from,nullptr,nullptr,nullptr)==1,"같은 칸(같은 도시)은 순정 그대로");
  U8*near1=cells_mem+1;check(TravelDays(from,near1,nullptr,nullptr,nullptr)==1,"바로 옆 칸은 순정 그대로");
  Patch27SetFlags(keep3&~1u);check(TravelDays(from,to,nullptr,nullptr,nullptr)==1,"타 세력 재야 등용 OFF면 순정(1일) 그대로");Patch27SetFlags(keep3);}
 return finish();
}
