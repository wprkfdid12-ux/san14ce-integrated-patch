// R42 A10 개성 9칸 공용 부품 확인창 (9·13번 공용).
//  9칸 자체는 공용 부품 두 파일이 맡는다: 게임 폴더 SAN14_Traits9.dll(통합편의가 시작 때 불러 5자리 5→9) +
//  SAN14_Traits9_UI.dll(3x3 편집 칸·정보창 9칸·개성 설명). 9번(콜라보)·13번(논공 가챠) 어느 쪽을 깔아도 같은 두 파일이 깔린다.
//  이 모듈은 게임 코드를 하나도 바꾸지 않는다(훅 없음). 시작 20초 뒤부터 읽기만 해서,
//   A) 5자리가 9칸 값이 아님(9칸 데이터 안 켜짐)  B) 화면 부품 자리(0x494431) 비어 있음(화면 부품 안 붙음)
//   C) 화면 부품 기록(SAN14_Traits9_UI.log)에 _SKIPPED / SKIPPED(화면 하나를 못 붙여 원래 5칸으로 보임)
//  이면 게임 안 오류창(글자 Ctrl+C 복사)으로 알리고 적용확인.log 에 R42T9 줄을 남긴다. 같은 글은 두 번까지만 띄움.
//  설정: SAN14Mods\modules\Traits9Check.ini [Traits9Check] Notice=1 (0이면 창 없이 기록만)
#include "common.h"
#include "r39_record.h"
using U64=uint64_t;

namespace {
const S14_CoreAPI*core;U8*game;volatile long notice=1,shown,checks,fault_a,fault_b,fault_c;
struct Site{U32 rva;U8 len;U8 after[8];};
const Site kSites[5]={
 {0x4a90b3,8,{0x48,0xc7,0x45,0xdc,0x09,0,0,0}},{0x4a96d3,8,{0x48,0xc7,0x45,0xdc,0x09,0,0,0}},
 {0x226fbf,4,{0x41,0x83,0xfc,0x09}},{0x2320aa,3,{0x83,0xfb,0x09}},{0x234737,4,{0x48,0x83,0xfb,0x09}}};
const U32 kUiGuard=0x494431;
wchar_t root[1024];

unsigned nine_sites(){unsigned n=0;for(const Site&s:kSites){U8*p=game+s.rva;if(core->Readable(p,s.len)&&!memcmp(p,s.after,s.len))n++;}return n;}
bool ui_hooked(){U8*p=game+kUiGuard;return core->Readable(p,6)&&p[0]==0xff&&p[1]==0x25&&at<U32>(p,2)==0;}
bool exists(const wchar_t*rel){wchar_t p[1200];p[0]=0;r39::wcat(p,1200,root);r39::wcat(p,1200,rel);return GetFileAttributesW(p)!=INVALID_FILE_ATTRIBUTES;}
// 게임 폴더 기록 파일 첫 줄(최대 300자). 없으면 false.
bool first_line(const wchar_t*rel,char*out,size_t cap){
 wchar_t p[1200];p[0]=0;r39::wcat(p,1200,root);r39::wcat(p,1200,rel);out[0]=0;
 HANDLE f=CreateFileW(p,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,0,OPEN_EXISTING,0,0);if(f==INVALID_HANDLE_VALUE)return false;
 char buf[600];DWORD n=0;BOOL ok=ReadFile(f,buf,sizeof buf-1,&n,0);CloseHandle(f);if(!ok)return false;
 size_t k=0;for(DWORD i=0;i<n&&k+1<cap&&k<300;i++){char c=buf[i];if(c=='\r'||c=='\n')break;if((unsigned char)c<0x20||(unsigned char)c>0x7e)c='?';out[k++]=c;}out[k]=0;return k>0;
}
bool has(const char*s,const char*w){size_t n=r39::slen(w);for(;*s;s++)if(!memcmp(s,w,n))return true;return false;}
U32 fnv(const wchar_t*s){U32 h=2166136261u;for(;*s;s++){h^=(U16)*s;h*=16777619u;}return h;}
// AI 모드가 '지금 설치돼' 있을 때만(폴더·백업만 남은 해제 상태는 아님): 게임 폴더 SAN14_CombatAI.dll, 또는 input_state.json 의 "installed": true
bool ai_present(){
 if(exists(L"SAN14_CombatAI.dll"))return true;
 wchar_t p[1200];p[0]=0;r39::wcat(p,1200,root);r39::wcat(p,1200,L"AI_TacticsMod\\backup\\input_state.json");
 HANDLE f=CreateFileW(p,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,0,OPEN_EXISTING,0,0);if(f==INVALID_HANDLE_VALUE)return false;
 char b[2048];DWORD n=0;BOOL ok=ReadFile(f,b,sizeof b-1,&n,0);CloseHandle(f);if(!ok)return false;b[n]=0;
 for(char*q=b;*q;q++){if(!memcmp(q,"\"installed\"",11)){q+=11;while(*q==' '||*q==':'||*q=='\t')q++;return !memcmp(q,"true",4);}}
 return false;
}
// 정식판 뒤(11-08): 등록 이름을 Mod09_Traits9Check.ini 로 바꿈(14번 ModPackBridge.ini 보다 먼저 올라오게). 옛 이름도 읽음.
void ini_path(wchar_t*p){p[0]=0;r39::wcat(p,1200,root);r39::wcat(p,1200,L"SAN14Mods\\modules\\Mod09_Traits9Check.ini");
 if(GetFileAttributesW(p)==INVALID_FILE_ATTRIBUTES){p[0]=0;r39::wcat(p,1200,root);r39::wcat(p,1200,L"SAN14Mods\\modules\\Traits9Check.ini");}}
// 같은 글은 두 실행까지만 창으로(그 뒤로는 기록만)
bool allow_box(U32 h){
 wchar_t p[1200];ini_path(p);wchar_t key[12];const wchar_t*d=L"0123456789abcdef";for(int i=0;i<8;i++)key[i]=d[(h>>(28-4*i))&15];key[8]=0;
 U32 c=(U32)GetPrivateProfileIntW(L"Shown",key,0,p);if(c>=2)return false;
 wchar_t v[4]={(wchar_t)(L'0'+c+1),0};WritePrivateProfileStringW(L"Shown",key,v,p);return true;
}
void report(const char*code,const char*detail,const wchar_t*what){
 char line[1100];line[0]=0;r39::scat(line,sizeof line,"R42T9\t");r39::scat(line,sizeof line,code);r39::scat(line,sizeof line,"\t");r39::stamp(line,sizeof line);r39::scat(line,sizeof line,"\t");r39::scat(line,sizeof line,detail);
 r39::append_line(line);
 if(!notice||shown>=3)return;
 static wchar_t t[2400];t[0]=0;const size_t C=2400;
 r39::wcat(t,C,what);
 r39::wcat(t,C,L"\n\n\xC544\xB798 \xAE00\xC790\xB97C \xADF8\xB300\xB85C \xB313\xAE00\xC5D0 \xC801\xC5B4 \xC8FC\xC138\xC694(\xC774 \xCC3D\xC5D0\xC11C Ctrl+C \xB85C \xBCF5\xC0AC\xB429\xB2C8\xB2E4).\n\n[R42 T9 ");
 wchar_t w[1000];size_t k=0;for(const char*s=code;*s&&k<20;s++)w[k++]=(wchar_t)*s;w[k++]=L']';w[k++]=L' ';for(const char*s=detail;*s&&k<990;s++)w[k++]=(wchar_t)*s;w[k]=0;r39::wcat(t,C,w);
 if(ai_present())r39::wcat(t,C,L"\n\nAI \xBAA8\xB4DC\xB97C \xAC19\xC774 \xC4F0\xBA74: AI \xBAA8\xB4DC \xAD00\xB9AC(\xD1B5\xD569\xD328\xCE58 \xD638\xD658\xD310)\xC5D0\xC11C '\xC801\xC6A9'\xC744 \xD55C \xBC88 \xB354 \xD558\xC138\xC694(\xAC1C\xC131 \xD654\xBA74 \xC790\xB8CC\xB97C \xC6D0\xB798\xB300\xB85C \xB9DE\xCDA4).");
 if(!allow_box(fnv(t)))return;
 InterlockedIncrement(&shown);
 // 정식판 뒤(11-08): 화면 칸 배치를 못 알아본 경우(unknown layout)는 다른 모드가 화면 자료를 바꾼 경우가 대부분 → 맨 위 큰 제목
 if(code[0]=='C'&&has(detail,"unknown layout")){
  r43c::show(L"\xAC1C\xC131 9\xCE78 - \xC624\xB958 \xCF54\xB4DC",ai_present()?L"다른 모드와 충돌 가능성 — AI 모드가 개성 화면 자료를 바꿨을 수 있음":L"다른 모드와 충돌 가능성 — 화면 자료(칸 배치)가 원본과 다름",t,0x50c30);return;}
 MessageBoxW(0,t,L"\xAC1C\xC131 9\xCE78 - \xC624\xB958 \xCF54\xB4DC",0x50c30);
}
// ── R42c: 9칸 화면 부품(SAN14_Traits9_UI.dll 49fb15b4)의 '요약 화면 칸 늘리기'(ALLSCREEN, 함수 +0x2280) 대신 ─────────
//  원래: 순정 개성 칸 5개(+0x20 x, +0x24 y, +0x28 w, +0x2c h, +0x8 부모)를 재서, 같은 부모·같은 높이·줄 사이 >= 높이·폭 30 이상일 때만
//  칸 9개를 새로 만들고(게임 표지 칸 0x188: 할당 0x3f90e0 → 0x84d4b0 → 0x8534e0(x,y,w,h,부모,0) → vt[0xc8](0)), 하나라도 어긋나면
//  "ALLSCREEN_SKIPPED - native label creation or measured geometry failed" 로 그 화면은 5칸으로 둠(명령 화면 대상·실행 무장 칸이 이 경우).
//  대신: 원래 조건이 다 맞으면 원래와 똑같이 계산(다른 화면 그대로). 어긋나면 이유를 기록하고, 줄 높이·간격 차이를 너그럽게 보아
//  (높이 ±8, 줄 간격은 높이의 절반 이상, 폭 20 이상, 같은 줄 판정 ±4) 다시 계산해 만듦. 만든 뒤 처리(순정 칸 기억 +0x60, +0x88,
//  각 칸 +0x84=1·0x89d1b0, 완료 기록)는 원래 함수와 같음. 그래도 안 되면 0(원래 실패 길: 5칸 + 오류 기록) + 자세한 이유를 오류창 글에 붙임.
static const U8 kUiSha[32]={0x7a,0x82,0x49,0x3f,0xb7,0xbc,0x68,0x6d,0x52,0x6f,0x53,0x94,0x82,0x67,0x7f,0x63,0xac,0x93,0x7a,0x2b,0xde,0x24,0x4d,0x49,0x12,0xae,0xa6,0xeb,0xeb,0xbd,0x29,0x57}; // R43: 새 판용 SAN14_Traits9_UI.dll(apply_new+apply_data_new) — upd_tools/update_hashes.py 가 갱신
static const U8 kAllEntry[14]={0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x55,0x57,0x56,0x53,0x48,0x81};
U8*ui;volatile long ui_patched,all_calls,all_relaxed,all_fail;
char all_diag[400];
// R42e(10-30 제보: CDuelPersonInfoLayout why=N, CUtilPInfoLayout why=G / 사용자: 어느 화면이든 9칸, 못 붙이면 어떤 경우든 자세한 오류창, 오류창이 떠도 게임은 안 튕김)
//  · 너그러운 다시 재기를 '거의 항상 되는' 방식으로: 순정 칸 5개가 서로 같은 부모이고 크기가 정상 범위(폭 8~4000, 높이 8~600)이기만 하면 만듦.
//    줄 판정 ±max(4, 가장 낮은 높이/2), 칸 사이는 같은 줄 오른쪽 이웃과의 간격 절반(없으면 높이/10, 2~40), 칸 폭은 최소 16, 높이는 가장 낮은 높이.
//    새 칸의 부모는 순정 칸의 부모(표 부모와 달라도 됨).
//  · 실패하면(UI 모듈 없음·순정 칸 못 읽음·부모 다름·크기 범위 밖·칸 만들기 실패) 이유 글을 pend 에 두고, 감시 스레드가 1초 안에 오류창(C)으로 알림.
//    그 뒤 화면 부품 기록의 ALLSCREEN_SKIPPED 한 줄은 같은 실패라 다시 띄우지 않음.
char fail_why[64];volatile long pend_on;char pend[700];
static bool rd(const void*p,size_t n){return p&&core->Readable(p,n);}
static void class_of(U8*obj,char*out,size_t cap){
 out[0]=0;if(!rd(obj,8))return;U8*vt=*(U8**)obj;if(vt<game+0x1000||vt>=game+S14_GAME_IMAGE||!rd(vt-8,8))return;U8*col=*(U8**)(vt-8);if(col<game||col>=game+S14_GAME_IMAGE||!rd(col,0x18))return;
 U32 td=*(U32*)(col+12);U8*nm=game+td+16;if(td>=S14_GAME_IMAGE||!rd(nm,64))return;
 size_t k=0;for(const char*q=(const char*)nm;*q&&k+1<cap&&k<60;q++){char c=*q;if(c=='.'||c=='?')continue;if(c=='@')break;out[k++]=c;}out[k]=0;
 if(k>2&&out[0]=='A'&&out[1]=='V'){for(size_t i=0;i+2<=k;i++)out[i]=out[i+2];}
}
static void sint(char*o,size_t cap,int v){if(v<0){r39::scat(o,cap,"-");v=-v;}r39::snum(o,cap,(unsigned)v);}
// 실패: 이유를 남기고(이미 알릴 글이 있으면 그대로) 0 — 원래 실패 길(그 화면 5칸)
static int all_fail_out(const char*why,U8*tbl,unsigned kind){
 if(why){size_t n=r39::slen(why);if(n>63)n=63;memcpy(fail_why,why,n);fail_why[n]=0;}
 if(pend_on)return 0;
 char m[700];m[0]=0;r39::scat(m,sizeof m,"SAN14 Traits9 UI: ALLSCREEN create failed - ");r39::scat(m,sizeof m,fail_why[0]?fail_why:"unknown");r39::scat(m,sizeof m," | ");
 if(all_diag[0])r39::scat(m,sizeof m,all_diag);
 else{char cls[80];class_of(tbl&&rd(tbl,8)?*(U8**)tbl:nullptr,cls,sizeof cls);r39::scat(m,sizeof m,"kind=");r39::snum(m,sizeof m,kind);r39::scat(m,sizeof m," parent=");r39::scat(m,sizeof m,cls[0]?cls:"?");}
 memcpy(pend,m,r39::slen(m)+1);InterlockedExchange(&pend_on,1);return 0;
}
// 줄(y) 모으기: 같은 줄 판정 ±tol, 위에서 아래 순서
static int rows_of(const int*Y,int tol,int*ry){int rows=0;for(int i=0;i<5;i++){int k=0;while(k<rows&&!(Y[i]>=ry[k]-tol&&Y[i]<=ry[k]+tol))k++;if(k==rows){int j=rows++;while(j>0&&ry[j-1]>Y[i]){ry[j]=ry[j-1];j--;}ry[j]=Y[i];}}return rows;}
extern "C" int T9AllCreate(U8*tbl,unsigned kind,U8**nat,void*arg4){
 all_diag[0]=0;fail_why[0]=0;InterlockedIncrement(&all_calls);
 if(!ui)ui=(U8*)GetModuleHandleW(L"SAN14_Traits9_UI.dll");if(!ui)return all_fail_out("ui module missing",tbl,kind);
 U8*parent=*(U8**)tbl;int X[5],Y[5],W[5],H[5];bool par=true;
 for(int i=0;i<5;i++){U8*n=nat[i];if(!rd(n,0x30))return all_fail_out("native label unreadable",tbl,kind);X[i]=at<int>(n,0x20);Y[i]=at<int>(n,0x24);W[i]=at<int>(n,0x28);H[i]=at<int>(n,0x2c);if(at<U8*>(n,8)!=parent)par=false;}
 // 원래 판정(같은 순서): P 부모 / R 폭·높이 범위 / E 높이 다름 / G 줄 간격 < 높이 / N 칸 폭 <= 29
 char why=0;int h0=H[0];
 for(int i=0;i<5&&!why;i++){if(!par)why='P';else if(W[i]<20||W[i]>2400||H[i]<10||H[i]>500)why='R';else if(H[i]!=h0)why='E';}
 int cols=0,rows=0,ry[5],gap=0,cw=0,minx=X[0],maxr=X[0]+W[0],hgt=h0;U8*lparent=parent;
 for(int i=0;i<5;i++){if(X[i]<minx)minx=X[i];if(X[i]+W[i]>maxr)maxr=X[i]+W[i];}
 if(!why){ // 원래 계산(줄은 y 가 같아야 함, 줄 간격 >= 높이, 칸 사이 = 가장 가까운 간격 절반, 폭 >= 30)
  rows=rows_of(Y,0,ry);
  for(int k=0;k+1<rows&&!why;k++)if(ry[k+1]-ry[k]<h0)why='G';
  if(!why){
   int mg=0x961;for(int i=0;i<5;i++)for(int j=0;j<5;j++){if(Y[j]!=Y[i])continue;int g=X[j]-(X[i]+W[i]);if(g>0&&g<mg)mg=g;}
   cols=(rows+8)/rows;gap=mg<=0x960?(mg+1)/2:(h0/10);if(gap<2)gap=2;
   cw=((maxr-minx)-(cols-1)*gap)/cols;if(cw<=29)why='N';
  }
 }
 char first=why;bool ok=true;char why2=0;const char*rfail=nullptr;
 if(why){ // 너그럽게 다시
  InterlockedIncrement(&all_relaxed);
  int hmin=H[0];for(int i=1;i<5;i++)if(H[i]<hmin)hmin=H[i];if(hmin>100000)hmin=100000;
  U8*p0=at<U8*>(nat[0],8);bool same=p0!=nullptr;for(int i=1;i<5;i++)if(at<U8*>(nat[i],8)!=p0)same=false;
  bool size=true;for(int i=0;i<5;i++)if(W[i]<8||W[i]>4000||H[i]<8||H[i]>600)size=false;
  if(!same){ok=false;why2='P';rfail="labels on different parents";}
  else if(!size){ok=false;why2='R';rfail="label size out of range";}
  else{
   int tol=hmin/2>=5?hmin/2:4;rows=rows_of(Y,tol,ry);
   bool found=false;int mg=0;
   for(int i=0;i<5;i++){int best=-1;for(int j=0;j<5;j++){if(j==i||Y[j]<Y[i]-tol||Y[j]>Y[i]+tol||X[j]<=X[i])continue;if(best<0||X[j]<X[best])best=j;}
    if(best>=0){int g=X[best]-(X[i]+W[i]);mg=found?(g<mg?g:mg):g;found=true;}}
   int g=!found?hmin/10:(mg>0?(mg+1)/2:2);if(g<3)g=2;if(g>40)g=40;gap=g;
   cols=(rows+8)/rows;cw=((maxr-minx)+(1-cols)*gap)/cols;if(cw<17)cw=16;
   hgt=hmin;lparent=p0;
  }
 }
 // 이유 글(오류창·기록)
 {char d[400];d[0]=0;char cls[80];class_of(parent,cls,sizeof cls);
  r39::scat(d,sizeof d,"why=");char c2[3]={first?first:'-',0,0};r39::scat(d,sizeof d,c2);r39::scat(d,sizeof d," kind=");r39::snum(d,sizeof d,kind);
  r39::scat(d,sizeof d," parent=");r39::scat(d,sizeof d,cls[0]?cls:"?");r39::scat(d,sizeof d," boxes=");
  for(int i=0;i<5;i++){r39::scat(d,sizeof d,"(");sint(d,sizeof d,X[i]);r39::scat(d,sizeof d,",");sint(d,sizeof d,Y[i]);r39::scat(d,sizeof d," ");sint(d,sizeof d,W[i]);r39::scat(d,sizeof d,"x");sint(d,sizeof d,H[i]);r39::scat(d,sizeof d,")");}
  if(first){r39::scat(d,sizeof d,ok?" relaxed=ok(":" relaxed=fail(");char c3[2]={ok?'+':why2,0};r39::scat(d,sizeof d,c3);r39::scat(d,sizeof d,")");}
  if(first){memcpy(all_diag,d,r39::slen(d)+1);char l[512];l[0]=0;r39::scat(l,sizeof l,"R42T9\tD\t");r39::stamp(l,sizeof l);r39::scat(l,sizeof l,"\t");r39::scat(l,sizeof l,d);r39::append_line(l);}
 }
 if(!ok){InterlockedIncrement(&all_fail);return all_fail_out(rfail,tbl,kind);}
 // 만들기(원래와 같은 호출)
 U8*rec=tbl+(size_t)kind*0x80;
 for(int i=0;i<9;i++){
  U8*o=fn<U8*(*)(U64)>(game,0x3f90e0)(0x188);if(!o){InterlockedIncrement(&all_fail);return all_fail_out("label alloc failed",tbl,kind);}
  fn<void(*)(U8*)>(game,0x84d4b0)(o);
  at<U8*>(rec,0x18+8*i)=o;
  int row=i/cols,col=i%cols;
  fn<void(*)(U8*,int,int,int,int,U8*,int)>(game,0x8534e0)(o,col*(cw+gap)+minx,ry[row<rows?row:rows-1],cw,hgt,lparent,0);
  ((void(*)(U8*,int))((*(void***)o)[0xc8/8]))(o,0);
  if(!at<U8*>(o,0x178)){InterlockedIncrement(&all_fail);memcpy(fail_why,"native label create failed",27);
   if(first){char l[200];l[0]=0;r39::scat(l,sizeof l,"R42T9\tD\t");r39::stamp(l,sizeof l);r39::scat(l,sizeof l,"\tlabel create failed at ");r39::snum(l,sizeof l,(unsigned)i);r39::append_line(l);}
   return all_fail_out(nullptr,tbl,kind);}
 }
 for(int i=0;i<5;i++)at<U8*>(rec,0x60+8*i)=nat[i];
 at<void*>(rec,0x88)=arg4;
 if(arg4)for(int i=0;i<9;i++){U8*o=at<U8*>(rec,0x18+8*i);at<int>(o,0x84)=1;fn<void(*)(void*,U8*)>(game,0x89d1b0)(arg4,o);}
 if(at<int>(tbl,8))((void(*)(U8*,int))(ui+0x1e50))(tbl,1);
 ((void(*)(const char*))(ui+0x1060))((const char*)(ui+0x83d8));       // ALLSCREEN_GRID_CREATED 기록(원래와 같음)
 return 1;
}
// 9칸 화면 부품을 확인하고(파일 해시) +0x2280 입구를 우리 함수로 돌림. 13번(MeritTraits)의 화면 부품 확인(시작 때 한 번) 뒤에 바꿈.
static void patch_ui(){
 if(ui_patched)return;HMODULE m=GetModuleHandleW(L"SAN14_Traits9_UI.dll");if(!m)return;
 if(!module_sha(m,kUiSha)){InterlockedExchange(&ui_patched,2);if(core->Log)core->Log("Traits9Check R42c: SAN14_Traits9_UI.dll is not the known build; summary-panel helper not attached.");
  r39::fail(9,"SAN14_Traits9Check.dll","ui_build","patch_ui() -> module_sha(SAN14_Traits9_UI.dll, kUiSha)","R43 Traits9_UI build (7a82493f…)","file hash differs (other build/edited)",L"개성 9칸 요약 화면 보강(ALLSCREEN 대체)이 안 붙음. 9칸 표시 자체는 화면 부품(Traits9_UI) 원래 방식대로");return;}
 U8*p=(U8*)m+0x2280;if(!rd(p,14)||memcmp(p,kAllEntry,14)){InterlockedExchange(&ui_patched,3);if(core->Log)core->Log("Traits9Check R42c: Traits9_UI +0x2280 differs (another fix?); left as is.");
  r39::fail_bytes(9,"SAN14_Traits9Check.dll","ui_entry",0x2280,kAllEntry,rd(p,14)?p:nullptr,14,L"개성 9칸 요약 화면 보강이 안 붙음(화면 부품 +0x2280 입구 바이트 다름)");return;}
 U8*cs=(U8*)m+0x2a92;if(!rd(cs,5)||cs[0]!=0xe8||cs+5+*(I32*)(cs+1)!=p){InterlockedExchange(&ui_patched,3);r39::fail(9,"SAN14_Traits9Check.dll","ui_callsite","patch_ui() -> Traits9_UI+0x2a92 call","e8 -> +0x2280","differs",L"개성 9칸 요약 화면 보강이 안 붙음(화면 부품 호출 자리 다름)");return;}
 ui=(U8*)m;DWORD old;if(!VirtualProtect(p,14,PAGE_EXECUTE_READWRITE,&old)){InterlockedExchange(&ui_patched,4);r39::fail(9,"SAN14_Traits9Check.dll","ui_protect","patch_ui() -> VirtualProtect(+0x2280,14)","RWX","failed",L"개성 9칸 요약 화면 보강이 안 붙음(쓰기 권한 실패)");return;}
 U8 j[14]={0xff,0x25,0,0,0,0};U64 a=(U64)(UPTR)&T9AllCreate;memcpy(j+6,&a,8);memcpy(p,j,14);
 VirtualProtect(p,14,old,&old);FlushInstructionCache(GetCurrentProcess(),p,14);
 InterlockedExchange(&ui_patched,1);
 if(core->Log)core->Log("Traits9Check R42c: summary-panel nine-slot helper attached (same result where the shared part already works; tolerant measuring + reason log where it failed).");
}
DWORD WINAPI watch(void*){
 // 13번(MeritTraits)은 시작(ModInit) 때 화면 부품 코드 지문을 한 번 확인 → 그 뒤, 게임 자료가 올라오고(화면이 열리기 전) 바꿈
 Sleep(4000);for(int i=0;i<112&&!ui_patched;i++){bool data=game&&*(U8**)(game+0x2780f60)!=nullptr;if(data||i>=32)patch_ui();if(!ui_patched)Sleep(500);}
 Sleep(16000);
 // A·B: 시작 결과(한 번)
 unsigned n=nine_sites();bool ui=ui_hooked();char l9[320],lu[320];first_line(L"SAN14_Traits9.log",l9,sizeof l9);first_line(L"SAN14_Traits9_UI.log",lu,sizeof lu);
 r39::start(9,"SAN14_Traits9Check.dll",(n==5&&ui)?"OK":"FAIL",n+(ui?1:0),6,"Notice","nine slots + 3x3 UI");
 if(n!=5){InterlockedIncrement(&fault_a);char d[400];d[0]=0;r39::scat(d,sizeof d,"slots=");r39::snum(d,sizeof d,n);r39::scat(d,sizeof d,"/5 log9=");r39::scat(d,sizeof d,l9[0]?l9:"(none)");
  report("A",d,L"\xAC1C\xC131 9\xCE78\xC774 \xCF1C\xC9C0\xC9C0 \xC54A\xC558\xC2B5\xB2C8\xB2E4(\xAC1C\xC131\xC740 \xC6D0\xB798 5\xCE78\xC73C\xB85C \xB3D9\xC791).");}
 else if(!ui){InterlockedIncrement(&fault_b);char d[400];d[0]=0;r39::scat(d,sizeof d,"ui=");r39::scat(d,sizeof d,exists(L"SAN14_Traits9_UI.dll")?"file":"nofile");r39::scat(d,sizeof d," logui=");r39::scat(d,sizeof d,lu[0]?lu:"(none)");
  report("B",d,L"\xAC1C\xC131 9\xCE78 \xD654\xBA74 \xBD80\xD488\xC744 \xBD99\xC774\xC9C0 \xBABB\xD588\xC2B5\xB2C8\xB2E4(9\xCE78\xC740 \xCF1C\xC838 \xC788\xC9C0\xB9CC \xD3B8\xC9D1 \xB4F1 \xD654\xBA74\xC5D0\xC11C 5\xCE78\xB9CC \xBCF4\xC784).");}
 // C: 화면 열 때마다 바뀌는 화면 부품 기록(마지막 한 줄만 남음)을 3초마다 확인
 char last[320];last[0]=0;if(fault_b)memcpy(last,lu,r39::slen(lu)+1); // B 로 이미 알린 시작 실패 줄은 C 로 또 띄우지 않음
 // R42e: 9칸 만들기 실패(pend)는 1초 안에 바로 오류창. 그 뒤 화면 부품 기록의 같은 실패(ALLSCREEN_SKIPPED) 한 줄은 다시 안 띄움.
 static const wchar_t kMsgC[]=L"\xD654\xBA74 \xD558\xB098\xC5D0 \xAC1C\xC131 9\xCE78\xC744 \xBD99\xC774\xC9C0 \xBABB\xD588\xC2B5\xB2C8\xB2E4(\xADF8 \xD654\xBA74\xC740 \xC6D0\xB798 5\xCE78\xC73C\xB85C \xBCF4\xC784).";
 bool quiet=false;
 for(;;){
  InterlockedIncrement(&checks);
  if(pend_on){char d[900];memcpy(d,pend,sizeof pend);InterlockedExchange(&pend_on,0);InterlockedIncrement(&fault_c);report("C",d,kMsgC);quiet=true;}
  if(first_line(L"SAN14_Traits9_UI.log",lu,sizeof lu)&&has(lu,"SKIPPED")&&!has(lu,"HOVER_SKIPPED")&&memcmp(lu,last,r39::slen(lu)+1)){
   memcpy(last,lu,r39::slen(lu)+1);
   if(quiet&&has(lu,"ALLSCREEN_SKIPPED")){quiet=false;continue;}
   InterlockedIncrement(&fault_c);char d[900];d[0]=0;r39::scat(d,sizeof d,lu);if(all_diag[0]){r39::scat(d,sizeof d," | ");r39::scat(d,sizeof d,all_diag);}
   report("C",d,kMsgC);
  }
  Sleep(1000);
 }
}
}
namespace{
// 9번 스스로 시작(R43 8차, 사용자 11-09 "9·13·14번이 서로 기생하지 말고 따로 작동하게").
//  9칸은 공용 부품 두 파일: SAN14_Traits9.dll(본체: 게임 5자리 5→9) + SAN14_Traits9_UI.dll(화면 부품: 그 5자리가 9 여야만 붙음).
//  예전: 본체는 통합편의가 원본 확인 뒤(1초+) 실음 → 14번과 순서 경쟁. 2차에 '14번 있을 때만 화면 부품을 먼저' 켰더니 본체보다 먼저라
//   화면 부품이 'Traits9 dependency absent' 로 SKIPPED(한 번만 시도) → 편집 화면 5칸(11-09 figekth 제보).
//  이제: 9번 모듈이 시작(ModInit, 이름 Mod09_ 라 14번 ModPackBridge 보다 먼저) 때 본체의 SAN14_ApplyTraits9 를 직접 부름
//   = 통합편의가 하는 것과 같은 일·같은 순서(본체 5자리 → 화면 부품), 시점만 앞당김. 14번 유무·설정과 상관없이 늘.
//   13번이 먼저 켰으면 본체는 ALREADY_APPLIED, 화면 부품은 InitOnce 라 그대로. 뒤에 통합편의가 또 불러도 InitOnce 라 아무 일 없음.
//   5자리(0x4a90b3·0x4a96d3·0x226fbf·0x2320aa·0x234737)와 화면 가드 0x494431 은 통합편의 원본 확인 표와 안 겹침(test_traits9_independent.py).
void early_nine(){
 wchar_t p9[1200];p9[0]=0;r39::wcat(p9,1200,root);r39::wcat(p9,1200,L"SAN14_Traits9.dll");
 HMODULE m=LoadLibraryExW(p9,nullptr,0x8);   // LOAD_WITH_ALTERED_SEARCH_PATH (통합편의가 실을 때와 같은 파일)
 auto apply=m?reinterpret_cast<void(*)()>(GetProcAddress(m,"SAN14_ApplyTraits9")):nullptr;
 if(!apply){r39::fail(9,"SAN14_Traits9Check.dll","early_nine","SAN14_ModInitV1 -> early_nine -> LoadLibraryExW(SAN14_Traits9.dll)/GetProcAddress(SAN14_ApplyTraits9)","module loaded + export",m?"export missing":"LoadLibraryExW failed",L"개성 9칸 본체(SAN14_Traits9.dll)를 못 불러옴 — 9번을 다시 설치하면 됨");return;}
 apply();
 if(core&&core->Log){char l[200];l[0]=0;r39::scat(l,sizeof l,"Traits9Check R43-8: nine-trait core+UI started by module 9 itself (slots=");r39::snum(l,sizeof l,nine_sites());r39::scat(l,sizeof l,"/5 ui=");r39::scat(l,sizeof l,ui_hooked()?"on":"off");r39::scat(l,sizeof l,").");core->Log(l);}
}
}
extern "C" __declspec(dllexport) void*SAN14_Traits9CheckTestTable(int i){void*t[]={(void*)&nine_sites,(void*)&ui_hooked,(void*)&first_line,(void*)&has,(void*)&shown,(void*)&fault_a,(void*)&fault_b,(void*)&fault_c,(void*)&notice,(void*)root,(void*)&report,(void*)&watch,(void*)&patch_ui,(void*)&T9AllCreate,(void*)&ui_patched,(void*)all_diag,(void*)&all_calls,(void*)&all_relaxed,(void*)&all_fail,(void*)&ui,(void*)&ai_present};return i>=0&&i<(int)(sizeof t/sizeof t[0])?t[i]:nullptr;}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,9,"SAN14_Traits9Check.dll"))return 0;core=api;game=api->resolved->image;
 if(!r39::game_root(root,1024)){r39::fail(9,"SAN14_Traits9Check.dll","root","SAN14_ModInitV1 -> r39::game_root (GetModuleFileNameW)","game folder path","failed",L"개성 9칸 확인(9번 Traits9Check)이 안 돌아감. 9칸 기능 자체는 그대로");return 0;}
 wchar_t p[1200];ini_path(p);notice=GetPrivateProfileIntW(L"Traits9Check",L"Notice",1,p)?1:0;
 early_nine();
 HANDLE h=CreateThread(0,0,watch,0,0,0);if(h)CloseHandle(h);else r39::fail(9,"SAN14_Traits9Check.dll","thread","SAN14_ModInitV1 -> CreateThread(watch)","thread handle","NULL",L"개성 9칸 확인(9번 Traits9Check)이 안 돌아감. 9칸 기능 자체는 그대로");
 if(core->Log)core->Log("Traits9Check R42: nine-trait shared part check scheduled (no game code changed; R42c summary-panel helper attaches to SAN14_Traits9_UI after start).");
 return h?1:0;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
