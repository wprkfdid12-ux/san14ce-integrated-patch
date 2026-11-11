// 34번: 지역담당관 공석 경고 (R41d 새로 만듦) — 내 거점 지도 이름판의 이름 글자색을 빨간색으로. 이름 글자·칸 폭은 손대지 않음.
//  이전 판(R39~R41c)은 이름 앞에 "^03!" 를 끼워 넣고 칸을 넓혔다가 되돌리는 방식이었는데, 되돌리기가 화면에 안 먹고
//  ("!" 가 남고 칸만 줄어 잘림) 다른 모드(29·14번)와 같은 글자 함수를 함께 써서 위험했음 → 글자는 아예 안 바꿈.
//  방식: 게임이 이름 칸에 쓰는 글자 모양(스타일) 번호만 바꿈. 게임 함수 0x9314a0(칸묶음, 칸 번호, 스타일 번호) 그대로 사용.
//   스타일 표 0x1a26e60(0x30 바이트 x 0x4e, 색은 A,B,G,R 바이트 순):
//    도시형(CCapitalTextLayout, [판+0x208]) 이름 칸 0x16: 게임 기본 0x13(흰 f0f0f0 + 테두리) → 공석 0x14(빨강 ff6445, 같은 테두리)
//      근거: 게임 초기화 0x1a06b0 이 (판+0x130, 0x16, 0x13) 을 씀
//    거점형(CFootholdInfoLayoutSimple, [판+0x218]) 이름 칸 0xe: 게임 기본 0 (흰 f0f0f0) → 공석 0x10(빨강 ff6445, 같은 꾸밈 없음)
//      근거: 게임 초기화 0x1a1ac0 이 (판+0x130, 0xe, 0) 을 씀
//  판정 자리(화면 스레드, 매 프레임): 이름판 갱신 함수(vf2) 끝 에필로그 — 게임은 vf2 안에서 필요할 때 vf72(새로고침)를 부름.
//   F1 0x1a92c5 CCapitalInfoLayout vf2 끝, F2 0x1a9241 CFootholdInfoLayout vf2 끝 (11바이트 에필로그 그대로 옮김)
//   R41f: AI 모드(편의 DLL)는 vf2 함수 앞부분(0x1a9250+0x5d, 0x1a91a0+0x40)을 해시로 검사해 다르면 지역담당관 초상화를 안 켬 → 그 범위 밖인 에필로그에 붙임.
//   한 판당 0.15초마다 판정. 공석은 0.3초 이어져야 빨강(게임이 다른 스레드에서 담당 칸을 잠깐 0 으로 쓰는 순간 무시).
//   빨강인 판은 1초마다 다시 칠함(게임이 판을 다시 초기화해 원래 색으로 돌린 경우 대비). 판의 데이터가 바뀌면(판 돌려쓰기) 바로 다시 판정.
//  판정 근거(게임 코드):
//   지역 = CAreaData (표 db+0x6caf0, 501칸, 크기 0x98). [지역+0x36](word)=담당 무장 id. 0 이거나 0x1770 초과거나 무장 무효(0x331780)면 공석.
//   도시형 이름판 데이터 = 지역 자체([판+0x1f0]). 거점 이름판 데이터의 지역 = 0x22b880(데이터).
//   플레이어 소유 = 0x22e260(지역) == 0x330b90(*(db+0x871e8)).
//  [판+0x220]!=0(도시형 사용자 글자 모드)이면 손대지 않음(칠했던 판이면 원래 색으로만 돌림).
//  설정: SAN14Mods\modules\RegionOfficer.ini [RegionOfficer] Warning=1. export SAN14_RegionOfficerGet/Set (1=공석 경고).
#include "common.h"
#include "r39_record.h"
typedef unsigned long long U64;
static const S14_CoreAPI*core;static U8*game;static volatile long warn_on=1;static unsigned warn_sets,restores,frames;
extern "C"{void RoFrameC();void RoFrameF();}
static U8*db(){return *(U8**)(game+0x2780f60);}
typedef U8*(*PtrFn)(U8*);typedef U8*(*PtrFn2)(U8*,int);typedef int(*IntFn)(U8*);typedef void(*SetStyle)(U8*,int,int);
static U8*area_ok(U8*a){
 U8*d=db();if(!d||!a)return nullptr;U8*base=*(U8**)(d+0x6caf0);if(!base||a<base)return nullptr;
 size_t off=(size_t)(a-base);if(off%0x98)return nullptr;size_t idx=off/0x98;if(idx<1||idx>500)return nullptr;
 return *(U8**)(d+0x6caf0+idx*8)==a?a:nullptr;
}
static bool vacant(U8*a){
 unsigned id=at<U16>(a,0x36);if(!id||id>0x1770)return true;
 U8*p=*(U8**)(db()+0x158+(size_t)id*8);if(!p)return true;
 return ((IntFn)(game+0x331780))(p)==0;
}
static bool player_owned(U8*a){
 U8*scen=*(U8**)(db()+0x871e8);if(!scen)return false;
 U8*me=((PtrFn)(game+0x330b90))(scen);U8*owner=((PtrFn)(game+0x22e260))(a);
 return me&&owner==me&&owner!=*(U8**)(db()+0xdf30);
}
extern "C" int SAN14_RegionOfficerTestJudge(U8*a){U8*x=area_ok(a);if(!x)return -1;return (player_owned(x)?2:0)|(vacant(x)?1:0);}

static const U64 DEBOUNCE=300,CHECK=150,REAPPLY=1000; // REAPPLY = 빨강 판 색 확인 간격
static const U32 VT_CAP=0x139d368,VT_FOOT=0x139c9f8,VT_CAPTEXT=0x139ceb8,VT_FOOTSIMPLE=0x139c148;   // 시험9: 뒤 둘은 RTTI(CCapitalTextLayout·CFootholdInfoLayoutSimple)로 다시 확인 — 이전 값은 엉뚱한 칸이라 이름판 글자칸을 못 알아봐 빨강이 안 칠해졌음
static const int CHILD[2]={0x16,0xe},NORMAL[2]={0x13,0},RED[2]={0x14,0x10};
static U64 vac_since[512];
static bool judge(U8*a,U64 now){
 if(!warn_on||!a)return false;U8*base=*(U8**)(db()+0x6caf0);size_t idx=(size_t)(a-base)/0x98;if(idx>=512)return false;
 if(!(player_owned(a)&&vacant(a))){vac_since[idx]=0;return false;}
 if(!vac_since[idx])vac_since[idx]=now?now:1;
 return now-vac_since[idx]>=DEBOUNCE;
}
// 이름판 기억: 판(layout)마다 칠했는지(red), 마지막 판정 시각, 그때의 데이터·글자판
//  R42f2(디시 장골 10-31: 성을 먹고 나서 계속 느려짐) — 기억 칸이 쌓여 꽉 차면 칠한 판 자리를 못 비워 매 프레임 32칸 훑기·실패가 반복되던 것:
//   · 칸마다 마지막으로 본 시각(seen). 10초 넘게 안 보인 칸은 빈칸처럼 다시 씀(stale_reuse). 훑는 범위 64칸, 그래도 없으면 그 판만 건너뜀(cache_full).
//   · 글자판 확인(vtable)은 판이 바뀔 때만(sub_ok 기억).
//   · 빨강 다시 칠하기: 1초마다 무조건 → 1초마다 지금 글자색을 읽어(color_is) 빨강이 아닐 때만. 못 읽으면 5초마다, 전체에서 50ms 에 한 판만.
//   · 빨강을 안 칠했는데 판이 빨강이면(돌려쓴 판에 남은 색) 원래 색으로(fresh_fix).
//   · 5분마다 적용확인.log 에 R42F 34 stats 줄(live·used·reuse·full·paints·flips·fresh_fix).
struct Ent{U8*layout,*sub,*data;U64 next,applied,seen,checked;U8 red,used,kind,sub_ok,fresh;U8 pad[3];};
static Ent cache[4096];static unsigned stale_reuse,cache_full,paints,restored_fresh,color_ok_n,color_unk_n,reapplied;static U64 next_stats,last_slow;
static const U64 STALE=10000;
static Ent*slot(U8*layout,U64 now){
 unsigned h=(unsigned)(((UPTR)layout>>4)*2654435761u)&4095;Ent*best=nullptr;bool stale=false;
 for(unsigned k=0;k<64;k++){Ent&e=cache[(h+k)&4095];
  if(!e.used){if(!best)best=&e;continue;}
  if(e.layout==layout){e.seen=now;return &e;}
  if(!best&&now-e.seen>STALE){best=&e;stale=true;}
 }
 if(!best){if(cache_full++==0)r39::counter(34,"SAN14_RegionOfficer.dll","cache_full",1);return nullptr;}
 if(stale)stale_reuse++;
 memset(best,0,sizeof(Ent));best->layout=layout;best->used=1;best->seen=now;return best;
}
static unsigned diag_n;
static void diag_flip(U8*a,bool on,int kind,const char*why){
 if(diag_n>=60||!a)return;diag_n++;U8*base=*(U8**)(db()+0x6caf0);
 char line[240];line[0]=0;r39::scat(line,240,"R41D\t34\tSAN14_RegionOfficer.dll\t");r39::scat(line,240,on?"red\t":"normal\t");r39::snum(line,240,(unsigned)((size_t)(a-base)/0x98));
 r39::scat(line,240,"\tofficer=");r39::snum(line,240,at<U16>(a,0x36));r39::scat(line,240,"\tkind=");r39::snum(line,240,(unsigned)kind);
 r39::scat(line,240,"\twhy=");r39::scat(line,240,why);
 {char nm[64];unsigned j=0;const wchar_t*w=(const wchar_t*)(a+0x10);for(unsigned i=0;i<12&&w[i]&&j+4<sizeof nm;i++){unsigned c=w[i];if(c<0x80)nm[j++]=(char)c;else if(c<0x800){nm[j++]=(char)(0xc0|(c>>6));nm[j++]=(char)(0x80|(c&63));}else{nm[j++]=(char)(0xe0|(c>>12));nm[j++]=(char)(0x80|((c>>6)&63));nm[j++]=(char)(0x80|(c&63));}}nm[j]=0;r39::scat(line,240,"\tname=");r39::scat(line,240,nm);}
 r39::append_line(line);
}
static bool child_ok(U8*group,int child){
 if(!core->Readable(group,0x18))return false;int cnt=at<int>(group,0x10);U8**arr=*(U8***)(group+8);
 if(child<0||child>=cnt||!core->Readable(arr,(size_t)cnt*8))return false;return arr[child]!=nullptr&&core->Readable(arr[child],0x120);
}
// 칸의 지금 글자색이 스타일 style 의 색과 같은가: 1 같음, 0 다름, -1 못 읽음. child_ok 뒤에만 부를 것.
//  글자 칸(+0x112==2)의 vf16(+0x80, 게임 코드 안일 때만)이 돌려주는 색 16바이트 vs 스타일 표 0x1a26e60 + style*0x30 앞 16바이트
static int color_is(U8*group,int child,int style){
 U8*el=(*(U8***)(group+8))[child];if(!core->Readable(el,0x120)||at<U8>(el,0x112)!=2)return -1;
 U8**slot=(U8**)(*(U8**)el+0x80);if(!core->Readable(slot,8))return -1;U8*fn=*slot;if(fn<game||fn>=game+0x133f000)return -1;
 U8*c=((PtrFn2)fn)(el,0);if(!c||!core->Readable(c,0x10))return -1;
 return memcmp(c,game+0x1a29e60+(size_t)style*0x30,16)==0?1:0;
}
static void paint(Ent&e,int kind,bool red,U64 now){
 U8*group=e.sub+0x130;int child=CHILD[kind];if(!child_ok(group,child))return;
 ((SetStyle)(game+0x9314a0))(group,child,red?RED[kind]:NORMAL[kind]);e.applied=now;paints++;
}
static void stats(U64 now){
 unsigned used=0,live=0;for(unsigned i=0;i<4096;i++)if(cache[i].used){used++;if(now-cache[i].seen<=STALE)live++;}
 char l[200];l[0]=0;r39::scat(l,sizeof l,"R42F\t34\tSAN14_RegionOfficer.dll\tstats\tlive=");r39::snum(l,sizeof l,live);
 r39::scat(l,sizeof l,"\tused=");r39::snum(l,sizeof l,used);r39::scat(l,sizeof l,"\treuse=");r39::snum(l,sizeof l,stale_reuse);
 r39::scat(l,sizeof l,"\tfull=");r39::snum(l,sizeof l,cache_full);r39::scat(l,sizeof l,"\tpaints=");r39::snum(l,sizeof l,paints);
 r39::scat(l,sizeof l,"\tflips=");r39::snum(l,sizeof l,warn_sets);r39::scat(l,sizeof l,"\tfresh_fix=");r39::snum(l,sizeof l,restored_fresh);r39::scat(l,sizeof l,"\t");
 r39::stamp(l,sizeof l);r39::append_line(l);
}
static volatile long frame_said;static volatile DWORD frame_tid;
// 매 프레임(이름판마다) 화면 스레드에서 불림. 빠르게 끝내야 함.
extern "C" void RoFrame(U8*layout,int kind){
 if(!layout||!game)return;
 U8*vt=*(U8**)layout;if(vt!=game+(kind?VT_FOOT:VT_CAP))return;
 frames++;
 if(!InterlockedExchange(&frame_said,1)){frame_tid=GetCurrentThreadId();r39::counter(34,"SAN14_RegionOfficer.dll","frame_hook_alive",1);}
 U64 now=GetTickCount64();
 if(now>=next_stats){if(next_stats){stats(now);next_stats=now+300000;}else next_stats=now+60000;}
 Ent*ep=slot(layout,now);if(!ep)return;Ent&e=*ep;
 U8*data=*(U8**)(layout+0x1f0);U8*sub=*(U8**)(layout+(kind?0x218:0x208));
 bool changed=e.data!=data||e.sub!=sub||e.kind!=(U8)kind;
 if(!changed&&now<e.next)return;
 e.next=now+CHECK;
 if(changed){
  // 판이 다른 지역으로 바뀜(돌려쓰기). 예전 글자판에 빨강을 칠했었다면 그 글자판은 원래 색으로.
  if(e.red&&e.sub&&e.sub==sub&&core->Readable(e.sub,0x140))paint(e,e.kind,false,now);
  e.red=0;e.data=data;e.sub=sub;e.kind=(U8)kind;e.fresh=1;
 }
 if(!data||!sub||!db())return;
 if(changed||!e.sub_ok){
  if(!core->Readable(sub,0x140)){e.sub_ok=0;return;}
  e.sub_ok=*(U8**)sub==game+(kind?VT_FOOTSIMPLE:VT_CAPTEXT);if(!e.sub_ok)return;
 }
 bool user_text=kind==0&&at<U32>(layout,0x220)!=0;
 U8*area=kind?((PtrFn)(game+0x22b880))(data):data;U8*a=area_ok(area);
 bool want=!user_text&&judge(a,now);
 U8*group=sub+0x130;int child=CHILD[kind];
 // 시험12: 돌려쓴 판에 남은 빨강 확인은 판이 바뀐 다음 판정 한 번만(이전엔 빨강 아닌 모든 판에서 0.15초마다 → 판 343개 x 메모리 확인 6번
 //  = 초당 1만 번 넘는 VirtualQuery 가 게임 작업 스레드에서 돌아 그 스레드가 커널 100% → 화면이 그 스레드를 기다려 1~2 FPS. Diag_Perf 15:18 표본으로 확인)
 if(!changed&&e.fresh){e.fresh=0;if(!want&&!e.red&&child_ok(group,child)&&color_is(group,child,RED[kind])==1){paint(e,kind,false,now);restored_fresh++;}}
 if(want){
  if(e.red&&!changed){ // 이미 빨강: 1초마다 지금 색을 읽어 빨강이 아닐 때만 다시 칠함
   int c=1;
   if(now-e.checked>=REAPPLY){e.checked=now;if(child_ok(group,child)){c=color_is(group,child,RED[kind]);
     if(c<0){if(color_unk_n++==0)r39::counter(34,"SAN14_RegionOfficer.dll","color_read_fallback",1);
      if(now-e.applied<5000||now-last_slow<50)return;last_slow=now;c=0;goto repaint;}}}
   if(color_ok_n++==0)r39::counter(34,"SAN14_RegionOfficer.dll","color_read_ok",1);
   if(c)return;
   if(reapplied++==0)r39::counter(34,"SAN14_RegionOfficer.dll","reapplied",1);
  }
  repaint:
  paint(e,kind,true,now);
  if(!e.red){warn_sets++;diag_flip(a,true,kind,changed?"new":"vacant");if(warn_sets==1)r39::counter(34,"SAN14_RegionOfficer.dll","warn_set",warn_sets);}
  e.red=1;
 }else if(e.red){paint(e,kind,false,now);e.red=0;restores++;diag_flip(a,false,kind,user_text?"usertext":(warn_on?"filled":"off"));}
}
extern "C" __declspec(dllexport) void*SAN14_RegionOfficerTestTable(int i){
 void*t[]={(void*)&RoFrameC,(void*)&RoFrameF,(void*)&RoFrame,(void*)&SAN14_RegionOfficerTestJudge,(void*)&warn_sets,(void*)&restores,(void*)&frames};
 return i>=0&&i<7?t[i]:nullptr;
}
extern "C" __declspec(dllexport) int SAN14_RegionOfficerGet(int which){return which==1?(int)warn_on:0;}
extern "C" __declspec(dllexport) void SAN14_RegionOfficerSet(int which,int on){if(which==1)InterlockedExchange(&warn_on,on?1:0);}
extern "C" __declspec(dllexport) void SAN14_RegionOfficerSetEnabled(int on){SAN14_RegionOfficerSet(1,on);}
extern "C" __declspec(dllexport) int SAN14_RegionOfficerState(){return SAN14_RegionOfficerGet(1);}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,34,"SAN14_RegionOfficer.dll"))return 0;core=api;game=api->resolved->image;
 wchar_t ini[1024];if(r39::game_root(ini,1024)){r39::wcat(ini,1024,L"SAN14Mods\\modules\\RegionOfficer.ini");warn_on=GetPrivateProfileIntW(L"RegionOfficer",L"Warning",1,ini)?1:0;}
 HookPlan plan(api);
 static const U8 epi[]={0x48,0x8b,0x5c,0x24,0x30,0x48,0x83,0xc4,0x20,0x5f,0xc3};
 plan.jump(0x1a92c5,epi,11,(void*)&RoFrameC);
 plan.jump(0x1a9241,epi,11,(void*)&RoFrameF);
 bool ok=plan.install("SAN14CE.RegionOfficer");
 if(core->Log)core->Log(ok?"RegionOfficer R42f2: 2 sites hooked (vacant area officer = red name color on own map nameplates; name text untouched).":"RegionOfficer R42f: hook group NOT installed (bytes differ / conflict). Nothing changed.");
 r39::start(34,"SAN14_RegionOfficer.dll",ok?(warn_on?"OK":"OFF"):"FAIL",ok?2:0,2,warn_on?"Warning=1 Style=RedName":"Warning=0",ok?"-":"hook bytes differ or another mod patched the site");
 if(!ok)r39::fail_plan(34,"SAN14_RegionOfficer.dll",plan,"map nameplate frame epilogue exe+0x1a92c5/0x1a9241",L"지역 담당관 공석 경고(34번, 붉은 이름)가 적용 안 됨. 다른 기능은 그대로");
 return ok?1:0;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
