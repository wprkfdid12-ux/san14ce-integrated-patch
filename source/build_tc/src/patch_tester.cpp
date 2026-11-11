// SAN14_PatchTester.dll — 통합패치 자체 점검·시험 모듈 (R5)
// R5: 필드 부대 관찰(armies.txt) — 수송부대가 보통 진형으로 만들어지는지 추적
// R4 (성능 측정): 신호 파일에 "perf" 가 있으면 플레이어 턴마다 게임 자신의 진행 요청(CGameState+0x47c, 게임 세터 0x450360)을
//     넣어 게임을 혼자 진행시키고, 별도 작업 스레드가 20ms 마다 게임 상태 스택 맨 위(정적 관리자 game+0x1b620b0)만 읽어
//     상태가 바뀐 시각을 perf.txt 에 적는다(진행 1회·AI 처리·턴 넘김에 걸린 실제 시간). 게임 코드는 바꾸지 않고, 스레드를 멈추지 않는다.
//     모드별 프레임 시간은 13.5번 안개 엔진에 원래 들어 있는 측정 기능(SAN14_FogAI.ini Profile=1 → SAN14_FogPerf.log)을 쓴다.
// R3 (R18 패키지): 옛 25번(다른 군단 도시·장수 직접 명령)을 통합패치에서 빼면서 화면 번호를 당겼다.
//     점검 항목표에서 그 줄을 빼고, 옛 26·27번을 25·26번으로 표시한다. 등용 시험도 26번으로 부른다(신호 "27"도 받아 줌).
// R2: 플레이어 턴 칸(CUserStrategyState Tick, vtable 0x13d6368)을 ModCore 등록으로 잡으면 23번(DomesticTalent)이 먼저 잡은 칸과
//     겹쳐 거절되고, 그때 점검까지 통째로 건너뛰었다. 이제 점검은 늘 하고, 턴 칸은 다른 모듈 설치가 끝난 뒤 그 위에 이어 붙인다.
// 1) 게임 시작 뒤 연결 점검: 등록 모듈 로드 여부, ModCore 훅 그룹, 설정 ON/OFF, 설치 선택과의 차이 → check.txt
// 2) 신호 파일(SAN14Mods\PatchTester\test.flag)이 있으면 플레이어 턴에서 상황을 재현하고 날짜가 바뀔 때마다 판정 → result.txt
//    22번 포로 관직 유지·원래 소속 도시 귀환, 26번(옛 27번) 타 세력 도시 재야 등용(1일 취소 버그·이동 일수)
// 게임 함수는 모두 플레이어 전략 상태(CUserStrategyState)의 Enter/Tick 안, 즉 게임 스레드에서만 부른다.
#include "common.h"
namespace {
const S14_CoreAPI*core;U8*game;
wchar_t root[1024];unsigned rootLen;
wchar_t pathDir[1024],pathLog[1024],pathFlag[1024],pathResult[1024],pathCheck[1024],pathPerf[1024],pathArmies[1024];
bool ready;ULONGLONG lastPoll;int lastDate=-1;bool haveDate;unsigned turnsSeen;
volatile LONG checkState;        // 0 대기, 1 진행, 2 완료
volatile LONG checkPosted;       // 기록창에 점검 요약을 썼는지
volatile LONG checkRuns;         // 끝난 점검 횟수 (1: 시작 직후, 2: 첫 플레이어 턴에서 다시)
bool rerunStarted;
int checkProblems,checkLoaded,checkRegistered,checkGroups;wchar_t checkSummary[3][120];int checkSummaryCount;
SRWLOCK fileLock=SRWLOCK_INIT;

// ---------------------------------------------------------------- 문자열 도우미 (CRT 없이)
struct WBuf{wchar_t b[512];unsigned n=0;WBuf(){b[0]=0;}
 void add(const wchar_t*s){if(!s)return;while(*s&&n<511)b[n++]=*s++;b[n]=0;}
 void addn(const wchar_t*s,unsigned max){if(!s)return;for(unsigned i=0;i<max&&s[i]&&n<511;i++)b[n++]=s[i];b[n]=0;}
 void num(long long v){wchar_t t[24];unsigned k=0;bool neg=v<0;unsigned long long u=neg?0ull-(unsigned long long)v:(unsigned long long)v;do{t[k++]=wchar_t(L'0'+u%10);u/=10;}while(u);if(neg)t[k++]=L'-';while(k&&n<511)b[n++]=t[--k];b[n]=0;}
 void hex(unsigned long long v,int digits){for(int i=digits-1;i>=0&&n<511;i--){unsigned d=unsigned((v>>(4*i))&15);b[n++]=wchar_t(d<10?L'0'+d:L'a'+d-10);}b[n]=0;}
 void clear(){n=0;b[0]=0;}
 const wchar_t*c()const{return b;}
};
unsigned wlen(const wchar_t*s){unsigned n=0;while(s&&s[n])n++;return n;}
bool weq(const wchar_t*a,const wchar_t*b){while(*a&&*b){wchar_t x=*a,y=*b;if(x>=L'A'&&x<=L'Z')x+=32;if(y>=L'A'&&y<=L'Z')y+=32;if(x!=y)return false;a++;b++;}return *a==*b;}
bool wends(const wchar_t*s,const wchar_t*suffix){unsigned a=wlen(s),b=wlen(suffix);return a>=b&&weq(s+a-b,suffix);}
// 바이트열 검색 (대소문자 구분)
const char*findb(const char*hay,size_t n,const char*needle){size_t m=0;while(needle[m])m++;if(!m||n<m)return nullptr;for(size_t i=0;i+m<=n;i++){if(hay[i]==needle[0]&&!memcmp(hay+i,needle,m))return hay+i;}return nullptr;}
const char*findlast(const char*hay,size_t n,const char*needle){const char*last=nullptr;const char*p=hay;size_t left=n;while(true){const char*q=findb(p,left,needle);if(!q)break;last=q;p=q+1;left=n-(p-hay);}return last;}
bool contains(const char*hay,size_t n,const char*needle){return findb(hay,n,needle)!=nullptr;}
void a2w(const char*s,size_t n,WBuf&out){for(size_t i=0;i<n&&s[i]&&out.n<511;i++)out.b[out.n++]=wchar_t((unsigned char)s[i]);out.b[out.n]=0;}
int parseInt(const char*s,size_t n){int v=0;bool any=false;for(size_t i=0;i<n;i++){if(s[i]<'0'||s[i]>'9')break;v=v*10+(s[i]-'0');any=true;}return any?v:-1;}

// ---------------------------------------------------------------- 파일 도우미
bool pathJoin(wchar_t*out,const wchar_t*rel){unsigned n=rootLen;if(n+wlen(rel)+1>1000)return false;memcpy(out,root,n*2);wcscpy(out+n,rel);return true;}
bool fileExists(const wchar_t*p){DWORD a=GetFileAttributesW(p);return a!=INVALID_FILE_ATTRIBUTES&&!(a&FILE_ATTRIBUTE_DIRECTORY);}
void appendUtf8(const wchar_t*path,const wchar_t*text,bool bomIfNew){
 char buf[2048];int n=WideCharToMultiByte(CP_UTF8,0,text,-1,buf,sizeof(buf)-4,nullptr,nullptr);if(n<=0)return;n--; // 끝 NUL 제외
 buf[n++]='\r';buf[n++]='\n';
 AcquireSRWLockExclusive(&fileLock);
 HANDLE f=CreateFileW(path,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,0,nullptr);
 if(f!=INVALID_HANDLE_VALUE){LARGE_INTEGER size{};DWORD w=0;if(bomIfNew&&GetFileSizeEx(f,&size)&&size.QuadPart==0){const char bom[3]={char(0xef),char(0xbb),char(0xbf)};WriteFile(f,bom,3,&w,nullptr);}WriteFile(f,buf,(DWORD)n,&w,nullptr);CloseHandle(f);}
 ReleaseSRWLockExclusive(&fileLock);
}
void stamp(WBuf&b){SYSTEMTIME t;GetLocalTime(&t);b.add(L"[");if(t.wHour<10)b.add(L"0");b.num(t.wHour);b.add(L":");if(t.wMinute<10)b.add(L"0");b.num(t.wMinute);b.add(L":");if(t.wSecond<10)b.add(L"0");b.num(t.wSecond);b.add(L"] ");}
void logLine(const wchar_t*text){WBuf b;stamp(b);b.add(text);appendUtf8(pathLog,b.c(),true);if(core->Log){char a[1024];int n=WideCharToMultiByte(CP_UTF8,0,text,-1,a,sizeof(a),nullptr,nullptr);if(n>0)core->Log(a);}}
void resultLine(const wchar_t*text){appendUtf8(pathResult,text,true);WBuf b;b.add(L"결과: ");b.add(text);logLine(b.c());}
// 파일 전체 읽기 (VirtualAlloc). 실패 시 nullptr.
char*readAll(const wchar_t*path,size_t*outLen,size_t cap=64u<<20){
 HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);if(f==INVALID_HANDLE_VALUE)return nullptr;
 LARGE_INTEGER size{};char*buf=nullptr;
 if(GetFileSizeEx(f,&size)&&size.QuadPart>=0&&(size_t)size.QuadPart<=cap){
  buf=(char*)VirtualAlloc(nullptr,(size_t)size.QuadPart+16,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
  if(buf){size_t total=0;while(total<(size_t)size.QuadPart){DWORD got=0;DWORD want=(DWORD)((size_t)size.QuadPart-total>1u<<20?1u<<20:(size_t)size.QuadPart-total);if(!ReadFile(f,buf+total,want,&got,nullptr)||!got)break;total+=got;}*outLen=total;buf[total]=0;}
 }
 CloseHandle(f);return buf;
}
void freeAll(char*p){if(p)VirtualFree(p,0,MEM_RELEASE);}

// ---------------------------------------------------------------- 게임 접근
inline bool rd(const void*p,size_t n){return p&&(UPTR)p>0x10000&&core->Readable(p,n);}
inline U8*gm(){return at<U8*>(game,0x277df18);}
inline U8*scen(){U8*g=gm();if(!rd(g,0x871f0))return nullptr;U8*s=at<U8*>(g,0x871e8);return rd(s,0x460)?s:nullptr;} // 전역 관리자 0x87288바이트, 시나리오 객체는 +0x450까지만 읽음
inline int validObj(void*o){return rd(o,8)&&rd(at<void*>(o,0),0x20)?fn<int(*)(void*)>(game,0x32e4c0)(o):0;}
inline int curDate(){U8*s=scen();return s?at<int>(s,0x450):-1;}
inline int playerForceId(){U8*s=scen();return s?at<U8>(s,0x3a):-1;}
inline U8*force(int id){U8*g=gm();if(!g||id<0||id>0x33)return nullptr;U8*f=at<U8*>(g,0xdf30+8*id);return (rd(f,0x260)&&at<U8*>(f,0)==game+0x13a67f0&&validObj(f))?f:nullptr;}
inline U8*person(unsigned id){U8*g=gm();if(!g||id>0x1770)return nullptr;U8*p=at<U8*>(g,0x158+8*id);return (rd(p,0x1e8)&&at<U8*>(p,0)==game+0x13a6b38&&validObj(p))?p:nullptr;}
inline U8*cityById(int id){if(id<=0||id>0x3d)return nullptr;U8*c=fn<U8*(*)(U8*,int)>(game,0x32b110)(gm(),id);return (rd(c,0x164)&&at<U8*>(c,0)==game+0x13a6640&&at<U16>(c,0x4e)==id&&validObj(c))?c:nullptr;}
inline int cityId(U8*c){return at<U16>(c,0x4e);}
inline int cityOwner(U8*c){return reinterpret_cast<int(*)(void*)>(at<void*>(at<U8*>(c,0),0xb0))(c);}
inline int cityDistrict(U8*c){return at<U8>(c,0x30);}
inline int cityCellIndex(U8*c){U8*cell=fn<U8*(*)(void*)>(game,0x22ca60)(c);return rd(cell,0x20)?fn<int(*)(void*)>(game,0x22d2c0)(cell):-1;}
inline int personForceId(U8*p){return fn<int(*)(void*)>(game,0x22c380)(p);}
inline U8*personForce(U8*p){U8*f=fn<U8*(*)(void*)>(game,0x22bf60)(p);return rd(f,0x260)?f:nullptr;}
inline int status(U8*p){return at<U8>(p,0xe6);}
inline int rank(U8*p){return at<U8>(p,0xe7);}
inline int personCityId(U8*p){return at<U16>(p,0xe2);}
inline void container(U8*p,int&type,int&idx){type=-1;idx=-1;fn<void(*)(void*,int*,int*)>(game,0x231190)(p,&type,&idx);}
inline bool busy(U8*p){return fn<int(*)(void*)>(game,0x231770)(p)!=0;}
inline bool isRuler(U8*p){return fn<int(*)(void*)>(game,0x232b90)(p)!=0;}
inline U8*currentOrder(U8*p){U8*o=fn<U8*(*)(void*)>(game,0x229fc0)(p);return (rd(o,0x80)&&validObj(o))?o:nullptr;}
inline U8*rulerCity(U8*f){U8*c=fn<U8*(*)(void*)>(game,0x22dde0)(f);return rd(c,0x164)?c:nullptr;}
inline U8*areaCity(U8*c){U8*r=fn<U8*(*)(void*)>(game,0x229d00)(c);return rd(r,0x164)?r:nullptr;}
// 순정 해방 목적지: 그 세력 군주가 있는 도시(를 포함하는 지역의 도시)
int nativeReleaseDest(U8*f){U8*c=rulerCity(f);if(!c)return -1;U8*a=areaCity(c);return a?cityId(a):cityId(c);}
void personName(U8*p,WBuf&b){b.addn((const wchar_t*)(p+0x12),6);b.addn((const wchar_t*)(p+0x20),6);}
void cityNameOf(U8*c,WBuf&b){if(c){b.addn((const wchar_t*)(c+0x12),3);}b.add(L"(");b.num(c?cityId(c):-1);b.add(L")");}
void cityNameById(int id,WBuf&b){U8*c=cityById(id);if(c)cityNameOf(c,b);else{b.add(L"도시(");b.num(id);b.add(L")");}}
void forceNameOf(U8*f,WBuf&b){if(!f){b.add(L"?");return;}const wchar_t*s=fn<const wchar_t*(*)(void*)>(game,0x22e1f0)(f);if(rd(s,24))b.addn(s,10);else b.add(L"?");}
int hexSteps(int a,int b){int x1=a%220,y1=a/220,x2=b%220,y2=b/220;int q1=x1,r1=y1-((x1+(x1&1))>>1),q2=x2,r2=y2-((x2+(x2&1))>>1);int dq=q1-q2,dr=r1-r2,ds=dq+dr;if(dq<0)dq=-dq;if(dr<0)dr=-dr;if(ds<0)ds=-ds;return (dq+dr+ds)/2;}

// 플레이어 수도(군주 도시). 없으면 플레이어 소유 도시 중 첫 번째.
U8*playerCapital(){int pf=playerForceId();U8*f=force(pf);if(f){U8*c=rulerCity(f);if(c&&at<U8*>(c,0)==game+0x13a6640&&cityId(c)>0&&cityOwner(c)==pf)return c;}
 for(int id=1;id<=0x3d;id++){U8*c=cityById(id);if(c&&cityOwner(c)==pf)return c;}return nullptr;}
// 기록창(왼쪽 아래) 한 줄. 플레이어 수도 칸에 붙여 안개 모듈이 숨기지 않게 한다.
void history(const wchar_t*title,const wchar_t*body){
 U8*cap=playerCapital();int date=curDate();if(!cap||date<0)return;
 int district=cityDistrict(cap),cell=cityCellIndex(cap);if(district<0||district>0x33||cell<0||cell>=0xbd10)return;
 wchar_t t[55],b[85];unsigned i=0;for(;i<54&&title[i];i++)t[i]=title[i];t[i]=0;i=0;for(;i<84&&body[i];i++)b[i]=body[i];b[i]=0;
 using Hist=U8*(*)(void*,int,int,int,int,int,const wchar_t*,const wchar_t*,int,int,int,int,int);
 void*mgr=fn<void*(*)()>(game,0x16e290)();
 fn<Hist>(game,0x9338c0)(mgr,1,district,cell,1,date,t,b,0x1394,0,1,0,0);
}
void note(const wchar_t*title,const wchar_t*body){history(title,body);WBuf b;b.add(L"기록창: ");b.add(title);b.add(L" / ");b.add(body);logLine(b.c());}

// ---------------------------------------------------------------- 시험 22번: 포로 관직 유지 · 원래 소속 도시 귀환
struct Test22{int phase;unsigned pid;int home,rank0,forceId,natDest,heldCity,day0,dayRelease,loads0;bool hadRecord;wchar_t name[16];} t22;
struct Test27{int phase;unsigned execId,targetId;int day0,straight,targetCity,execCity,targetForce;wchar_t execName[16],targetName[16];} t27;
bool prisonerModule(){return GetModuleHandleW(L"SAN14_PrisonerReturn.dll")!=nullptr;}
// R2 모듈 질의: 기록 존재(원래 도시)·불러오기 적용 횟수. R1이면 둘 다 없음.
int prisonerRecord(unsigned id,int*home){HMODULE m=GetModuleHandleW(L"SAN14_PrisonerReturn.dll");if(!m)return -1;auto f=(int(*)(unsigned,int*,int*,int*))GetProcAddress(m,"PrisonerReturnRecord");if(!f)return -1;int h=0,r=0,fo=0;int ok=f(id,&h,&r,&fo);if(home)*home=h;return ok;}
int prisonerLoads(){HMODULE m=GetModuleHandleW(L"SAN14_PrisonerReturn.dll");if(!m)return -1;auto f=(int(*)())GetProcAddress(m,"PrisonerReturnLoads");return f?f():-1;}
int prisonerVersion(){HMODULE m=GetModuleHandleW(L"SAN14_PrisonerReturn.dll");if(!m)return 0;auto f=(int(*)())GetProcAddress(m,"PrisonerReturnVersion");return f?f():1;}
unsigned patch27Installed(){HMODULE m=GetModuleHandleW(L"SAN14_Patch27.dll");if(!m)return 0;auto f=(unsigned(*)())GetProcAddress(m,"Patch27Installed");return f?f():0;}
unsigned patch27Flags(){HMODULE m=GetModuleHandleW(L"SAN14_Patch27.dll");if(!m)return 0;auto f=(unsigned(*)())GetProcAddress(m,"Patch27Flags");return f?f():0;}
void copyName(wchar_t(&dst)[16],U8*p){WBuf b;personName(p,b);unsigned i=0;for(;i<15&&b.b[i];i++)dst[i]=b.b[i];dst[i]=0;}

bool setup22(){
 int pf=playerForceId();U8*PF=force(pf);if(!PF){resultLine(L"22|포로재현|SKIP|플레이어 세력을 찾지 못함");return false;}
 // 후보: 타 세력 소속, 임관(상태 1), 자기 세력 도시 안, 명령 없음, 군주 아님, 관직 있음, 원래 도시가 그 세력 군주 도시와 다름
 U8*best=nullptr;int bestRank=-1;int bestHome=0,bestNat=0,bestForce=0;
 for(unsigned id=1;id<=0x1770;id++){U8*p=person(id);if(!p||status(p)!=1||rank(p)==0x49)continue;
  int f=personForceId(p);if(f<0||f>=0x33||f==pf)continue;
  int type,idx;container(p,type,idx);if(type!=2)continue;U8*c=cityById(idx);if(!c||cityOwner(c)!=f||cityId(c)!=personCityId(p))continue;
  if(busy(p)||isRuler(p)||currentOrder(p))continue;
  if(at<U16>(c,0x46)==U16(id))continue; // 그 도시의 태수는 제외
  U8*F=force(f);if(!F)continue;int nat=nativeReleaseDest(F);if(nat<=0||nat==cityId(c))continue;
  int r=rank(p);if(r>bestRank){bestRank=r;best=p;bestHome=cityId(c);bestNat=nat;bestForce=f;}
 }
 if(!best){resultLine(L"22|포로재현|SKIP|조건에 맞는 타 세력 무장이 없음 (관직 있음·도시 대기·군주 도시와 다른 도시)");return false;}
 t22={};t22.pid=at<U16>(best,0x10);t22.home=bestHome;t22.rank0=bestRank;t22.forceId=bestForce;t22.natDest=bestNat;t22.day0=curDate();copyName(t22.name,best);
 // 이벤트 실행기(SExecCaptivePerson)와 같은 순서: 준비 → 포로 처리(부대 없음, 보고 없음, 포획 세력 = 플레이어)
 fn<int(*)(void*)>(game,0x26b550)(best);
 fn<void(*)(void*,void*,int,int,void*,void*)>(game,0x2e0000)(best,nullptr,0,0,PF,nullptr);
 U8*p=person(t22.pid);if(!p){resultLine(L"22|포로재현|FAIL|포로 처리 뒤 무장 객체가 사라짐");t22.phase=9;return true;}
 t22.heldCity=personCityId(p);int r1=rank(p),st=status(p);
 WBuf b;b.add(L"22|포로재현|");b.add(st==6?L"OK":L"FAIL");b.add(L"|무장=");b.add(t22.name);b.add(L"(");forceNameOf(force(bestForce),b);b.add(L") 원래 도시=");cityNameById(bestHome,b);b.add(L" 보관 도시=");cityNameById(t22.heldCity,b);b.add(L" 순정 해방 목적지=");cityNameById(bestNat,b);b.add(L" 상태=");b.num(st);resultLine(b.c());
 b.clear();bool pass=r1==bestRank;b.add(L"22|관직유지|");b.add(pass?L"PASS":(prisonerModule()?L"FAIL":L"NATIVE"));b.add(L"|포로 전 관직 ");b.num(bestRank);b.add(L" → 포로 후 ");b.num(r1);if(!pass&&r1==0x49)b.add(L" (관직 해제 = 순정 동작)");if(!prisonerModule())b.add(L" [22번 모듈 미로드]");resultLine(b.c());
 WBuf t;t.add(L"[시험22] ");t.add(t22.name);t.add(L" 포로 재현");WBuf d;d.add(pass?L"관직 유지 확인. ":L"관직이 해제됨. ");d.add(L"다음 턴에 해방 → 귀환 도시 판정");note(t.c(),d.c());
 // R2 모듈이면 기록(원래 도시)이 바로 남았는지, 저장→불러오기 뒤에도 남는지 볼 준비
 {int h=0;int rec=prisonerRecord(t22.pid,&h);t22.loads0=prisonerLoads();t22.hadRecord=rec==1;
  if(rec>=0){WBuf b;b.add(L"22|기록|");b.add(rec==1&&h==t22.home?L"PASS":L"FAIL");b.add(L"|22번 모듈(R");b.num(prisonerVersion());b.add(L")에 원래 도시 기록 ");b.add(rec==1?L"있음: ":L"없음");if(rec==1)cityNameById(h,b);b.add(L" — 이 상태에서 저장→불러오기를 하면 기록 유지도 판정합니다(선택)");resultLine(b.c());}}
 t22.phase=1;return true;
}
void judge22Release(U8*p,const wchar_t*how){
 int dest=personCityId(p);bool pass=dest==t22.home;
 WBuf b;b.add(L"22|귀환경로|");b.add(pass?L"PASS":(prisonerModule()?L"FAIL":L"NATIVE"));b.add(L"|");b.add(how);b.add(L" 목적지=");cityNameById(dest,b);b.add(L" 원래 도시=");cityNameById(t22.home,b);b.add(L" 순정 목적지=");cityNameById(t22.natDest,b);b.add(L" 해방 후 관직=");b.num(rank(p));b.add(L" 상태=");b.num(status(p));if(!prisonerModule())b.add(L" [22번 모듈 미로드]");
 U8*homeCity=cityById(t22.home);if(homeCity&&cityOwner(homeCity)!=t22.forceId)b.add(L" (원래 도시가 그 세력 소유가 아니어서 순정 목적지가 정상)");
 resultLine(b.c());
 WBuf t;t.add(L"[시험22] ");t.add(t22.name);t.add(L" 해방");WBuf d;d.add(pass?L"원래 소속 도시 ":L"순정 목적지 ");cityNameById(dest,d);d.add(pass?L"로 귀환 → 통과":L"로 귀환 → 실패");note(t.c(),d.c());
 t22.dayRelease=curDate();t22.phase=2;
}
void day22(){
 U8*p=person(t22.pid);
 if(!p){resultLine(L"22|귀환경로|SKIP|무장 객체를 더 찾을 수 없음");t22.phase=9;return;}
 if(t22.phase==1){
  int st=status(p);
  // (선택) 그 사이 저장→불러오기를 했으면: 불러오기 뒤에도 원래 도시 기록이 살아 있는지
  if(t22.hadRecord){int loads=prisonerLoads();if(loads>=0&&loads!=t22.loads0){int h=0;int rec=prisonerRecord(t22.pid,&h);WBuf b;b.add(L"22|저장복원|");b.add(rec==1&&h==t22.home?L"PASS":L"FAIL");b.add(L"|불러오기 ");b.num(loads-t22.loads0);b.add(L"회 뒤 원래 도시 기록 ");b.add(rec==1?L"유지: ":L"사라짐");if(rec==1)cityNameById(h,b);resultLine(b.c());t22.loads0=loads;}}
  if(st==6){ // 아직 우리 포로 → 해방 (이벤트 실행기와 같은 경로, 보고 없음)
   fn<int(*)(void*,int)>(game,0x2e68d0)(p,0);
   p=person(t22.pid);if(!p){resultLine(L"22|귀환경로|SKIP|해방 뒤 무장 객체가 사라짐");t22.phase=9;return;}
   judge22Release(p,L"시험 모듈이 해방.");
  }else if(st==4){judge22Release(p,L"이미 해방되어 이동 중.");}
  else if(st==1&&personForceId(p)==playerForceId()){resultLine(L"22|귀환경로|SKIP|포로가 우리 세력에 등용되어 귀환 판정 불가");t22.phase=9;}
  else{WBuf b;b.add(L"22|귀환경로|SKIP|예상 밖 상태 ");b.num(st);b.add(L" (세력 ");b.num(personForceId(p));b.add(L")");resultLine(b.c());t22.phase=9;}
  return;
 }
 if(t22.phase==2){
  int type,idx;container(p,type,idx);
  if(status(p)==1&&type==2&&idx==t22.home){WBuf b;b.add(L"22|귀환완료|OK|");b.num(curDate()-t22.dayRelease);b.add(L"일 만에 원래 도시 ");cityNameById(t22.home,b);b.add(L" 복귀, 관직=");b.num(rank(p));resultLine(b.c());WBuf t;t.add(L"[시험22] ");t.add(t22.name);t.add(L" 귀환 완료");WBuf d;cityNameById(t22.home,d);d.add(L" 복귀. 관직 ");d.num(rank(p));note(t.c(),d.c());t22.phase=3;}
  else if(status(p)==1&&type==2&&idx!=t22.home){WBuf b;b.add(L"22|귀환완료|FAIL|다른 도시 ");cityNameById(idx,b);b.add(L"에 도착");resultLine(b.c());t22.phase=9;}
  else if(curDate()-t22.dayRelease>60){resultLine(L"22|귀환완료|SKIP|60일 안에 도착하지 않음");t22.phase=9;}
 }
}

// ---------------------------------------------------------------- 시험 26번(옛 27번): 타 세력 도시 재야 등용 (1일 취소 버그·이동 일수)
bool setup27(){
 int pf=playerForceId();U8*PF=force(pf);U8*cap=playerCapital();if(!PF||!cap){resultLine(L"26|명령발행|SKIP|플레이어 세력/도시를 찾지 못함");return false;}
 unsigned inst=patch27Installed(),flags=patch27Flags();bool remoteOn=(inst&flags&1)!=0;
 // 실행자: 수도에 대기 중인 우리 무장(명령 없음, 군주·태수 아님). 관직이 가장 낮은 무장.
 U8*exec=nullptr;int execRank=-1;
 for(unsigned id=1;id<=0x1770;id++){U8*p=person(id);if(!p||status(p)!=1||personForceId(p)!=pf)continue;int type,idx;container(p,type,idx);if(type!=2||idx!=cityId(cap))continue;if(busy(p)||isRuler(p)||currentOrder(p)||at<U16>(cap,0x46)==U16(id))continue;int r=rank(p);if(r>execRank){execRank=r;exec=p;}}
 if(!exec){resultLine(L"26|명령발행|SKIP|수도에 명령을 내릴 수 있는 대기 무장이 없음");return false;}
 int fromCell=cityCellIndex(cap);if(fromCell<0){resultLine(L"26|명령발행|SKIP|수도 위치를 읽지 못함");return false;}
 // 대상: 다른 세력이 지배하는 도시의 재야(상태 5). 직선 이동 2일 이상 중 가장 가까운 무장.
 U8*target=nullptr;int bestDays=1<<30,bestCity=0,bestForce=0;
 for(unsigned id=1;id<=0x1770;id++){U8*p=person(id);if(!p||status(p)!=5)continue;int type,idx;container(p,type,idx);if(type!=2)continue;U8*c=cityById(idx);if(!c)continue;int owner=cityOwner(c);if(owner<0||owner>=0x33||owner==pf)continue;if(busy(p))continue;
  int cell=cityCellIndex(c);if(cell<0)continue;int steps=hexSteps(fromCell,cell);if(steps<2)continue;
  long long days=fn<long long(*)(int,void*,void*)>(game,0x230cc0)(steps,exec,PF);if(days<2)continue;
  if(days<bestDays){bestDays=(int)days;target=p;bestCity=idx;bestForce=owner;}
 }
 if(!target){resultLine(L"26|명령발행|SKIP|다른 세력 도시에 재야 무장이 없음 (직선 2일 이상)");return false;}
 t27={};t27.execId=at<U16>(exec,0x10);t27.targetId=at<U16>(target,0x10);t27.day0=curDate();t27.straight=bestDays;t27.targetCity=bestCity;t27.execCity=cityId(cap);t27.targetForce=bestForce;copyName(t27.execName,exec);copyName(t27.targetName,target);
 struct Input{U8*target;U8*exec;U32 wait;U32 pad;} in{target,exec,0,0};
 if(!fn<int(*)(void*)>(game,0x1ef5d0)(&in)){resultLine(L"26|명령발행|FAIL|게임이 등용 명령 입력을 거부");return false;}
 fn<void(*)(void*,int)>(game,0x1ed070)(&in,0);
 U8*o=currentOrder(exec);bool issued=o&&(at<U8*>(o,0)==game+0x13a1f70||at<U8*>(o,0)==game+0x13a20b8);
 WBuf b;b.add(L"26|명령발행|");b.add(issued?L"OK":L"FAIL");b.add(L"|실행=");b.add(t27.execName);b.add(L"@");cityNameOf(cap,b);b.add(L" 대상=");b.add(t27.targetName);b.add(L"@");cityNameById(bestCity,b);b.add(L"(");forceNameOf(force(bestForce),b);b.add(L" 소유) 직선 최소 ");b.num(bestDays);b.add(L"일 / 26번 타세력 등용 설정 ");b.add(remoteOn?L"ON":L"OFF");if(!issued)b.add(L" (명령 객체가 생기지 않음)");resultLine(b.c());
 if(!issued)return false;
 WBuf t;t.add(L"[시험26] ");t.add(t27.execName);t.add(L"→");t.add(t27.targetName);t.add(L" 등용 명령");WBuf d;d.add(L"타 세력 도시 ");cityNameById(bestCity,d);d.add(L" 재야. 직선 최소 ");d.num(bestDays);d.add(L"일. 턴을 넘겨 주세요");note(t.c(),d.c());
 t27.phase=1;return true;
}
void day27(){
 U8*exec=person(t27.execId);U8*target=person(t27.targetId);int k=curDate()-t27.day0;
 if(!exec){resultLine(L"26|결과|SKIP|실행 무장 객체를 더 찾을 수 없음");t27.phase=9;return;}
 U8*o=currentOrder(exec);bool alive=o&&(at<U8*>(o,0)==game+0x13a1f70||at<U8*>(o,0)==game+0x13a20b8);
 if(alive){WBuf b;b.add(L"26|진행|OK|");b.num(k);b.add(L"일차 등용 명령 진행 중");resultLine(b.c());if(k>90){resultLine(L"26|결과|SKIP|90일이 지나도 명령이 끝나지 않음");t27.phase=9;}return;}
 // 명령 종료 → 판정
 bool recruited=target&&status(target)==1&&personForceId(target)==playerForceId();
 bool stillRonin=target&&status(target)==5;
 unsigned inst=patch27Installed(),flags=patch27Flags();bool remoteOn=(inst&flags&1)!=0;
 WBuf b;b.add(L"26|결과|");
 if(k<=1&&!recruited){b.add(remoteOn?L"FAIL":L"NATIVE");b.add(L"|1일 만에 명령 종료(취소). ");}
 else if(k<t27.straight&&!recruited){b.add(L"FAIL|직선 최소 ");b.num(t27.straight);b.add(L"일보다 빨리 종료. ");}
 else{b.add(L"PASS|");}
 b.num(k);b.add(L"일 만에 종료 (직선 최소 ");b.num(t27.straight);b.add(L"일). 대상 ");b.add(t27.targetName);b.add(recruited?L": 등용 성공(우리 세력 임관)":stillRonin?L": 재야 그대로(설득 실패 또는 취소)":L": 다른 상태");if(target){b.add(L" 상태=");b.num(status(target));b.add(L" 세력=");b.num(personForceId(target));}if(!remoteOn)b.add(L" [26번 타세력 등용 설정 OFF → 순정 동작]");
 resultLine(b.c());
 WBuf t;t.add(L"[시험26] 등용 명령 ");t.num(k);t.add(L"일 만에 종료");WBuf d;d.add(recruited?L"등용 성공. ":stillRonin?L"설득 실패/취소. ":L"");d.add(k<=1&&!recruited?L"1일 취소 → 실패":k<t27.straight&&!recruited?L"너무 빨리 종료 → 실패":L"일수 정상 → 통과");note(t.c(),d.c());
 t27.phase=9;
}



// ---------------------------------------------------------------- R5: 필드 부대 관찰 (수송부대 그림 문제 추적)
// 전략 화면에 있는 동안 부대표(gm+0x7e838, 1~500)를 보고, 새로 생기거나 진형이 바뀐 부대를 armies.txt 에 적는다.
// 진형 번호 +0x1b: 0x13 = 수송(게임 판정 0x2320f0 은 0x13~0x14 를 수송으로 봄). 게임 쪽 진형표(gm+0x77428)의 수송 항목 상태값(+0x7e)도 같이 적는다.
struct ArmySeen{U16 leader;U8 formation;U8 valid;};ArmySeen armySeen[512];bool armyTableLogged;
inline U8*armyAt(unsigned i){U8*g=gm();if(!g||i==0||i>500)return nullptr;U8*a=at<U8*>(g,0x7e838+8*i);return (rd(a,0x60)&&validObj(a))?a:nullptr;}
void armyLine(const wchar_t*s){appendUtf8(pathArmies,s,true);}
void watchArmies(){
 U8*g=gm();if(!g)return;
 if(!armyTableLogged){armyTableLogged=true;WBuf b;b.add(L"진형표|");
  for(unsigned k=0;k<21;k++){U8*f=at<U8*>(g,0x77428+8*k);b.hex(k,2);b.add(L"=");if(rd(f,0x80)){b.num(at<U8>(f,0x7e));}else b.add(L"?");b.add(L" ");}
  b.add(L"| (0x13 수송 항목의 값이 2면 게임이 수송 진형을 넣지 못함)");armyLine(b.c());}
 for(unsigned i=1;i<=500;i++){
  U8*a=armyAt(i);ArmySeen&s=armySeen[i];
  if(!a){if(s.valid){s.valid=0;WBuf b;b.add(L"부대 사라짐|");b.num(i);armyLine(b.c());}continue;}
  U16 leader=at<U16>(a,0x12);U8 form=at<U8>(a,0x1b);
  if(s.valid&&s.leader==leader&&s.formation==form)continue;
  s.valid=1;s.leader=leader;s.formation=form;
  WBuf b;b.add(L"부대|");b.num(i);b.add(L"|무장=");U8*p=person(leader);if(p)personName(p,b);else{b.add(L"?");b.num(leader);}
  b.add(L"|진형=0x");b.hex(form,2);b.add(fn<int(*)(void*)>(game,0x2320f0)(a)?L"(수송)":L"(보통)");
  b.add(L"|공성=");b.num(fn<int(*)(void*)>(game,0x232400)(a));
  b.add(L"|날짜=");b.num(curDate());armyLine(b.c());
 }
}

// ---------------------------------------------------------------- R4 성능 측정: 게임 혼자 진행 + 상태 전환 시각 기록
struct PerfRun{volatile bool on,watching,done;ULONGLONG start,lastReq,warmEnd,limitMs,retryMs;volatile LONG runs;int maxRuns,requests,date0;} pf;
int perfOptRuns=60,perfOptMinutes=12,perfOptWarm=20000,perfOptRetry=3000; // 신호 파일의 runs= minutes= warm= retry= 로 바꿀 수 있음
const UPTR StateMgrRva=0x1b620b0; // 0x10f20 이 돌려주는 정적 상태 관리자 (+0x20 스택 배열, +0x10 개수)
const UPTR VtUser=0x13d6340,VtProgress=0x13d6610;
// 다른 스레드에서 읽기만 한다: 스택 맨 위 상태 객체의 vtable RVA (없으면 0)
UPTR topStateRva(){
 U8*m=game+StateMgrRva;if(!rd(m,0x28))return 0;UPTR n=at<UPTR>(m,0x10);U8*arr=at<U8*>(m,0x20);
 if(n==0||n>64||!rd(arr,8*n))return 0;U8*top=at<U8*>(arr,8*(n-1));if(!rd(top,8))return 0;UPTR vt=at<UPTR>(top,0);
 UPTR base=(UPTR)game;return vt>base&&vt<base+0x3000000?vt-base:0;
}
void perfLine(const wchar_t*s){appendUtf8(pathPerf,s,true);}
struct StateStat{UPTR vt;unsigned n;ULONGLONG total,max;};
DWORD WINAPI perfWatch(LPVOID){
 StateStat st[32]={};int ns=0;UPTR cur=topStateRva();LARGE_INTEGER f,t0,t;QueryPerformanceFrequency(&f);QueryPerformanceCounter(&t0);
 ULONGLONG since=0;auto ms=[&](){QueryPerformanceCounter(&t);return (ULONGLONG)((t.QuadPart-t0.QuadPart)*1000/f.QuadPart);};
 int day=curDate();
 {WBuf b;b.add(L"STATE|0|시작 vt=");b.hex(cur,7);b.add(L"|date=");b.num(day);perfLine(b.c());}
 while(!pf.done){
  Sleep(20);
  UPTR v=topStateRva();int d=curDate();ULONGLONG now=ms();
  if(v!=cur){
   ULONGLONG len=now-since;int k=0;for(;k<ns&&st[k].vt!=cur;k++){}
   if(k==ns&&ns<32){st[ns].vt=cur;ns++;}
   if(k<ns){st[k].n++;st[k].total+=len;if(len>st[k].max)st[k].max=len;}
   WBuf b;b.add(L"STATE|");b.num((long long)now);b.add(L"|");b.hex(cur,7);b.add(L"→");b.hex(v,7);b.add(L"|머문 ms=");b.num((long long)len);b.add(L"|date=");b.num(d);perfLine(b.c());
   if(cur==VtProgress&&v==VtUser)InterlockedIncrement(&pf.runs); // 진행 1회가 끝나 플레이어 턴으로 돌아옴
   cur=v;since=now;
  }
  if(d!=day&&d>0){day=d;}
 }
 // 끝: 상태별 합계
 {ULONGLONG now=ms();int k=0;for(;k<ns&&st[k].vt!=cur;k++){} if(k<ns){st[k].n++;st[k].total+=now-since;}}
 for(int k=0;k<ns;k++){WBuf b;b.add(L"SUM|vt=");b.hex(st[k].vt,7);b.add(st[k].vt==VtUser?L"(플레이어 턴)":st[k].vt==VtProgress?L"(진행)":L"");b.add(L"|횟수=");b.num(st[k].n);b.add(L"|합계 ms=");b.num((long long)st[k].total);b.add(L"|최대 ms=");b.num((long long)st[k].max);perfLine(b.c());}
 pf.watching=false;return 0;
}
void perfStart(){
 pf={};pf.on=true;pf.start=GetTickCount64();pf.warmEnd=pf.start+(ULONGLONG)perfOptWarm;pf.limitMs=(ULONGLONG)perfOptMinutes*60*1000;pf.retryMs=(ULONGLONG)perfOptRetry;pf.maxRuns=perfOptRuns;pf.date0=curDate();
 if(fileExists(pathPerf)){wchar_t old[1024];pathJoin(old,L"SAN14Mods\\PatchTester\\perf_prev.txt");MoveFileExW(pathPerf,old,MOVEFILE_REPLACE_EXISTING);}
 {WBuf b;b.add(L"PERF|시작|date=");b.num(pf.date0);b.add(L"|pid=");b.num(GetCurrentProcessId());b.add(L"|");b.num(perfOptWarm/1000);b.add(L"초 쉬고 나서 최대 ");b.num(pf.maxRuns);b.add(L"번 또는 ");b.num(perfOptMinutes);b.add(L"분 동안 혼자 진행");perfLine(b.c());resultLine(b.c());}
 {WBuf d;d.num(perfOptWarm/1000);d.add(L"초 뒤부터 게임이 혼자 진행합니다. 손대지 않아도 됩니다");note(L"[성능 측정] 시작",d.c());}
 pf.watching=true;HANDLE t=CreateThread(nullptr,0,perfWatch,nullptr,0,nullptr);if(t)CloseHandle(t);else pf.watching=false;
}
void perfFinish(const wchar_t*why){
 if(pf.done)return;pf.done=true;
 WBuf b;b.add(L"PERF|DONE|");b.add(why);b.add(L"|진행 요청 ");b.num(pf.requests);b.add(L"회, 플레이어 턴 복귀 ");b.num(pf.runs);b.add(L"회, 날짜 ");b.num(pf.date0);b.add(L"→");b.num(curDate());b.add(L", ");b.num((long long)((GetTickCount64()-pf.start)/1000));b.add(L"초");
 perfLine(b.c());resultLine(b.c());note(L"[성능 측정] 끝",L"측정이 끝났습니다. 도구가 게임을 닫습니다");
}
// 게임 스레드(플레이어 턴 처리)에서만 부른다.
void perfTick(){
 if(!pf.on||pf.done)return;ULONGLONG now=GetTickCount64();
 if(now<pf.warmEnd)return;
 if(pf.runs>=pf.maxRuns){perfFinish(L"정한 횟수 끝");return;}
 if(now-pf.start>pf.limitMs){perfFinish(L"정한 시간 끝");return;}
 if(pf.lastReq&&now-pf.lastReq<pf.retryMs)return; // 요청이 아직 안 받아들여졌으면 몇 초마다 다시
 U8*app=game+StateMgrRva;U8*gs=fn<U8*(*)(void*,const char*)>(game,0x56f130)(app,"CGameState");
 if(!rd(gs,0x490))return;
 fn<void(*)(void*,int)>(game,0x450360)(gs,1);pf.requests++;pf.lastReq=now;
}

// ---------------------------------------------------------------- 신호 파일 → 시험 시작, 날짜 변화 → 판정
bool want22,want27,wantPerf,running;
bool readFlag(){
 if(!fileExists(pathFlag))return false;size_t n=0;char*s=readAll(pathFlag,&n,65536);
 want22=want27=wantPerf=false;if(s){if(contains(s,n,"perf")||contains(s,n,"PERF")){wantPerf=true;
   auto opt=[&](const char*key,int&v,int lo,int hi){const char*k=findb(s,n,key);if(!k)return;size_t m=0;while(key[m])m++;int x=parseInt(k+m,(size_t)(n-(k+m-s)));if(x>=lo&&x<=hi)v=x;};
   opt("runs=",perfOptRuns,1,1000);opt("minutes=",perfOptMinutes,1,120);opt("warm=",perfOptWarm,0,600000);opt("retry=",perfOptRetry,100,60000);}
  else{if(contains(s,n,"22"))want22=true;if(contains(s,n,"26")||contains(s,n,"27"))want27=true;if(contains(s,n,"all")||contains(s,n,"ALL")){want22=want27=true;}}freeAll(s);}
 if(!want22&&!want27&&!wantPerf){want22=want27=true;}
 DeleteFileW(pathFlag);return true;
}
void finishIfDone(){
 bool a=!want22||t22.phase>=2||t22.phase==0;bool b=!want27||t27.phase>=9||t27.phase==0;
 if(running&&a&&b){running=false;resultLine(L"DONE|시험 끝. (22번 귀환 완료 소식은 도착하면 추가로 기록됩니다)");}
}
void startTests(){
 if(wantPerf){if(fileExists(pathResult)){wchar_t old[1024];pathJoin(old,L"SAN14Mods\\PatchTester\\result_prev.txt");MoveFileExW(pathResult,old,MOVEFILE_REPLACE_EXISTING);}perfStart();return;}
 if(fileExists(pathResult)){wchar_t old[1024];pathJoin(old,L"SAN14Mods\\PatchTester\\result_prev.txt");MoveFileExW(pathResult,old,MOVEFILE_REPLACE_EXISTING);}
 WBuf b;b.add(L"START|day=");b.num(curDate());b.add(L"|tests=");if(want22)b.add(L"22 ");if(want27)b.add(L"26");b.add(L"|player=");b.num(playerForceId());b.add(L"|pid=");b.num(GetCurrentProcessId());resultLine(b.c());
 running=true;t22={};t27={};
 if(want22){if(!setup22())want22=false;}
 if(want27){if(!setup27())want27=false;}
 if(!want22&&!want27){running=false;resultLine(L"DONE|재현할 수 있는 시험이 없었습니다");}
}
void onDay(){
 if(want22&&t22.phase>=1&&t22.phase<=2)day22(); // 귀환 완료 소식은 DONE 뒤에도 기록
 if(running&&want27&&t27.phase==1)day27();
 finishIfDone();
}
DWORD WINAPI checkThread(LPVOID);
void postCheckSummary(){
 if(checkState!=2||checkRuns<2||checkPosted)return;checkPosted=1;
 WBuf t;t.add(L"[패치 점검] 모듈 ");t.num(checkLoaded);t.add(L"/");t.num(checkRegistered);t.add(L" 로드, 훅 ");t.num(checkGroups);t.add(L"개, 문제 ");t.num(checkProblems);t.add(L"건");
 WBuf d;d.add(checkProblems?L"자세한 내용: SAN14Mods\\PatchTester\\check.txt":L"선택한 패치가 모두 연결되었습니다");
 note(t.c(),d.c());
 for(int i=0;i<checkSummaryCount;i++){WBuf tt;tt.add(L"[패치 점검] 문제 ");tt.num(i+1);note(tt.c(),checkSummary[i]);}
}
void pump(U8*state,bool fromTick){
 if(!ready)return;
 if(fromTick){ULONGLONG now=GetTickCount64();if(now-lastPoll<700)return;lastPoll=now;}
 U8*s=scen();if(!s)return;
 int d=curDate();
 bool top=fn<int(*)(void*)>(game,0x56f310)(state)!=0;
 // 게임이 실제로 돌아가는 첫 플레이어 턴에 한 번 더 점검한다 (늦게 로드되는 모듈 반영). 그 결과를 기록창에 알린다.
 if(top&&checkState==2&&checkRuns==1&&!rerunStarted){rerunStarted=true;InterlockedExchange(&checkState,1);HANDLE t=CreateThread(nullptr,0,checkThread,nullptr,0,nullptr);if(t)CloseHandle(t);else InterlockedExchange(&checkState,2);}
 if(top&&checkState==2&&!checkPosted)postCheckSummary();
 if(!haveDate){haveDate=true;lastDate=d;}
 else if(d!=lastDate){lastDate=d;turnsSeen++;onDay();}
 if(top)watchArmies();
 if(top&&!running&&!pf.on&&readFlag())startTests();
}

// ---------------------------------------------------------------- 연결 점검 (작업 스레드, 파일·모듈 목록만 읽음)
struct Item{const wchar_t*num;const wchar_t*name;const wchar_t*dlls;const char*groups;const wchar_t*iniSection;const wchar_t*iniKey;int internalId;};
// dlls: ';' 구분 (경로는 게임 폴더 기준), groups: ';' 구분 ModCore 훅 그룹 이름(부분 일치), 빈 문자열은 해당 없음
const Item items[]={
 {L"1",L"인재·포로 편의",L"SAN14_IntegratedConvenience.dll","",L"Convenience",L"PrisonerPersuasion",1},
 {L"2",L"자동인사·자동포상",L"SAN14_IntegratedConvenience.dll;SAN14_AutoPersonnelRepair.dll","",L"Convenience",L"AutoPersonnelModule",2},
 {L"3",L"논공 동시",L"SAN14_IntegratedConvenience.dll;SAN14_DualCreditSaveRepair.dll","SAN14CE.DualCreditSaveRepair",L"Convenience",L"DualCredit",3},
 {L"4",L"문전박대",L"SAN14_IntegratedConvenience.dll","",L"Convenience",L"AudienceGate",4},
 {L"5",L"명승지·보고·연출",L"SAN14_IntegratedConvenience.dll;SAN14_SettingsHelpScenic.dll","SAN14CE.SceneryMovieSkip;SAN14CE.SceneryDiscoverySkip",L"Convenience",L"SkipReports",5},
 {L"6",L"다수 부대 이동·수행",L"SAN14_IntegratedConvenience.dll","",L"Convenience",L"FormationMoveModule",6},
 {L"7",L"진행 배속",L"SAN14_IntegratedConvenience.dll","",L"Convenience",L"ProgressSpeedModule",7},
 {L"8",L"모드 설정 UI",L"SAN14_IntegratedConvenience.dll;SAN14_SettingsHelpScenic.dll","SAN14CE.SettingsHelp",L"Modules",L"ModSettingsUI",8},
 {L"9",L"BGM 추가·플레이어",L"SAN14_IntegratedConvenience.dll","",L"Modules",L"BgmExpansionModule",9},
 {L"10",L"이민족 성 일반화·지역·증축",L"","",nullptr,nullptr,10},
 {L"11",L"개성 9개·콜라보 해금",L"SAN14_IntegratedConvenience.dll","",L"Modules",L"Traits9Module",11},
 {L"12",L"고유무장 등록·모델 ID 확장",L"SAN14_IntegratedConvenience.dll","",L"Modules",L"ModelRegistryModule",12},
 {L"13",L"필드 부대이미지 3D조형으로",L"SAN14_Compat20_Entry.dll;FieldLeader3D_Managed20\\FieldLeader3D_Runtime.exe","",nullptr,nullptr,13},
 {L"13.5",L"필드 안개,3D조형최적화포함",L"SAN14_FogTrial.dll;SAN14_FogFieldPair_Entry.dll;SAN14Mods\\Fog121\\SAN14_FogEngine121.dll","",nullptr,nullptr,22},
 {L"14",L"논공 개성 가챠, 군주도 적용",L"SAN14_MeritTraits.dll","SAN14CE.MeritTraits",nullptr,nullptr,14},
 {L"15",L"개성 마우스 설명",L"SAN14Mods\\Compat20\\SAN14_TraitHover.dll","",nullptr,nullptr,15},
 {L"16",L"모든 정보탭에 연계탭 추가",L"SAN14_RelatedOfficersTab.dll","RelatedOfficersTab",nullptr,nullptr,16},
 {L"17",L"출진 프리셋",L"SAN14_SortiePresets.dll","SortiePresets",nullptr,nullptr,17},
 {L"18",L"능력치 상한 200",L"SAN14_Stats200_CEKO.dll","SAN14CE.Stats200",nullptr,nullptr,18},
 {L"19",L"경험치로 능력치 엄청 안오름",L"","",nullptr,nullptr,19},
 {L"20",L"120프레임",L"SAN14_FPS120.dll","SAN14CE.FPS120",nullptr,nullptr,20},
 {L"21",L"글씨체 변경 · 고운 바탕체",L"","",nullptr,nullptr,21},
 {L"22",L"포로 관직 유지 · 원래 소속 도시 귀환",L"SAN14_PrisonerReturn.dll","SAN14CE.PrisonerReturn",nullptr,nullptr,23},
 {L"23",L"내정 자동화 + 저돌 수정",L"SAN14_DomesticCE.dll;SAN14_DomesticTalent.dll","",nullptr,nullptr,24},
 {L"24",L"도시 자원 · 병력 상한 확장",L"version.dll","",nullptr,nullptr,25},
 // R18: 옛 25번(내부 26, 다른 군단 도시 · 장수 직접 명령)은 통합패치에서 뺐다.
 {L"25",L"수여로 오르는 능력치 미적용",L"","",nullptr,nullptr,27},
 {L"26",L"등용·관직·전사 알림·신무장 이름 확장",L"SAN14_Patch27.dll","SAN14CE.Patch27",nullptr,nullptr,28},
};
void checkLine(const wchar_t*s){appendUtf8(pathCheck,s,true);}
void addProblem(const wchar_t*s){checkProblems++;if(checkSummaryCount<3){unsigned i=0;for(;i<119&&s[i];i++)checkSummary[checkSummaryCount][i]=s[i];checkSummary[checkSummaryCount][i]=0;checkSummaryCount++;}WBuf b;b.add(L"문제|");b.add(s);checkLine(b.c());}
// 모듈 파일 이름(경로 포함 가능)이 현재 프로세스에 로드되었는지. version.dll 은 게임 폴더의 것인지 본다.
bool moduleLoaded(const wchar_t*rel){
 const wchar_t*base=rel;for(const wchar_t*p=rel;*p;p++)if(*p==L'\\'||*p==L'/')base=p+1;
 HMODULE m=GetModuleHandleW(base);if(!m)return false;
 if(weq(base,L"version.dll")){wchar_t path[1024];DWORD n=GetModuleFileNameW(m,path,1024);if(!n||n<rootLen)return false;wchar_t save=path[rootLen];path[rootLen]=0;bool same=weq(path,root);path[rootLen]=save;return same;}
 return true;
}
bool selected[32];bool haveSelection;
void readSelection(){
 wchar_t p[1024];pathJoin(p,L".san14ce_compat_r3\\state.xml");size_t n=0;char*s=readAll(p,&n,8u<<20);if(!s){checkLine(L"선택|설치 기록(.san14ce_compat_r3\\state.xml)을 읽지 못함 → 선택 비교 생략");return;}
 const char*sel=findb(s,n,"<Selected");if(sel){haveSelection=true;const char*end=findb(sel,n-(sel-s),"</Selected>");size_t m=end?(size_t)(end-sel):n-(sel-s);const char*q=sel;
  while(true){const char*f=findb(q,m-(q-sel),"<Feature ");if(!f)break;const char*idp=findb(f,m-(f-sel),"id=\"");const char*en=findb(f,m-(f-sel),"enabled=\"");const char*close=findb(f,m-(f-sel),">");
   if(idp&&en&&close&&idp<close&&en<close){int id=parseInt(idp+4,8);if(id>=1&&id<=28&&!memcmp(en+9,"true",4))selected[id]=true;}
   q=f+8;}
 }
 WBuf b;b.add(L"선택|설치 기록에서 선택된 항목(내부 번호): ");for(int i=1;i<=28;i++)if(selected[i]){b.num(i);b.add(L" ");}checkLine(b.c());freeAll(s);
}
DWORD WINAPI checkThread(LPVOID){
 // 다른 모듈의 시작 작업(통합편의 시작 대기 등)이 끝날 시간을 준다.
 if(checkRuns==0){if(HMODULE common=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll")){auto wait=reinterpret_cast<U32(*)()>(GetProcAddress(common,"SAN14_WaitConvenienceV1"));if(wait)wait();}Sleep(6000);}
 checkProblems=checkLoaded=checkRegistered=checkGroups=0;checkSummaryCount=0;haveSelection=false;for(auto&b:selected)b=false;
 DeleteFileW(pathCheck);
 {WBuf b;SYSTEMTIME t;GetLocalTime(&t);b.add(L"세션|pid=");b.num(GetCurrentProcessId());b.add(checkRuns==0?L"|게임 시작 직후":L"|첫 플레이어 턴");b.add(L"|시작=");b.num(t.wYear);b.add(L"-");if(t.wMonth<10)b.add(L"0");b.num(t.wMonth);b.add(L"-");if(t.wDay<10)b.add(L"0");b.num(t.wDay);b.add(L" ");if(t.wHour<10)b.add(L"0");b.num(t.wHour);b.add(L":");if(t.wMinute<10)b.add(L"0");b.num(t.wMinute);b.add(L"|게임폴더=");b.add(root);checkLine(b.c());}
 readSelection();
 // 1) 등록된 모듈(SAN14Mods\modules\*.ini) 로드 여부
 wchar_t pat[1024];pathJoin(pat,L"SAN14Mods\\modules\\*.ini");WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(pat,&fd);
 if(h!=INVALID_HANDLE_VALUE){do{if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)continue;wchar_t ini[1024];pathJoin(ini,L"SAN14Mods\\modules\\");wcscpy(ini+wlen(ini),fd.cFileName);
   wchar_t file[260]={},en[16]={};GetPrivateProfileStringW(L"Mod",L"File",L"",file,260,ini);GetPrivateProfileStringW(L"Mod",L"Enabled",L"1",en,16,ini);
   if(!file[0])continue;checkRegistered++;bool enabled=en[0]!=L'0';bool loaded=moduleLoaded(file);if(loaded)checkLoaded++;
   WBuf b;b.add(L"등록|");b.add(fd.cFileName);b.add(L"|");b.add(file);b.add(L"|");b.add(!enabled?L"꺼짐(Enabled=0)":loaded?L"로드됨":L"로드 안 됨");checkLine(b.c());
   if(enabled&&!loaded){WBuf p;p.add(L"등록 모듈이 로드되지 않음: ");p.add(file);p.add(L" (");p.add(fd.cFileName);p.add(L")");addProblem(p.c());}
  }while(FindNextFileW(h,&fd));FindClose(h);}
 else checkLine(L"등록|SAN14Mods\\modules 폴더를 읽지 못함");
 // 2) 통합편의 모듈 확인값
 if(HMODULE ic=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll")){bool known=known_common()!=nullptr;WBuf b;b.add(L"통합편의|로드됨|");b.add(known?L"확인값 일치(P1/R11)":L"알 수 없는 판 → 5·8·26번 설명창/명승 생략이 꺼질 수 있음");checkLine(b.c());if(!known)addProblem(L"통합편의 모듈이 알 수 없는 판입니다 (5번 명승 생략·8번 설명창·26번 설정 페이지가 꺼질 수 있음)");(void)ic;}
 else checkLine(L"통합편의|로드 안 됨");
 // 2b) 22번 모듈 판
 if(GetModuleHandleW(L"SAN14_PrisonerReturn.dll")){WBuf b;b.add(L"22번|모듈 판 R");b.num(prisonerVersion());b.add(prisonerVersion()>=2?L"|세이브 슬롯 사이드카(저장마다 파일 안 늘어남)":L"|R1: 저장마다 <해시>.bin 생성·무장 전원 해시 (R16 갱신 권장)");checkLine(b.c());}
 // 3) 26번(Patch27) 모듈 자체 상태
 {unsigned inst=patch27Installed(),flags=patch27Flags();if(GetModuleHandleW(L"SAN14_Patch27.dll")){WBuf b;b.add(L"26번|설치 비트=");b.hex(inst,2);b.add(L" 설정 비트=");b.hex(flags,2);b.add(L"|타세력 등용 ");b.add((inst&1)?((flags&1)?L"ON":L"OFF"):L"사용 불가");b.add(L", 행동 중 관직 ");b.add((inst&2)?((flags&2)?L"ON":L"OFF"):L"사용 불가");b.add(L", 전사 알림 ");b.add((inst&4)?((flags&4)?L"ON":L"OFF"):L"사용 불가");b.add(L", 이름 확장 ");b.add((inst&8)?L"설치":L"사용 불가");checkLine(b.c());if(!inst)addProblem(L"26번 모듈이 로드됐지만 기능 그룹이 하나도 설치되지 않음");}}
 // 4) 설정 파일 값
 wchar_t iniPath[1024];pathJoin(iniPath,L"San14ModSettings.ini");bool haveIni=fileExists(iniPath);
 // 5) ModCore 로그: 마지막 세션의 훅 그룹과 문제 문구
 char*log=nullptr;size_t logLen=0;const char*sess=nullptr;size_t sessLen=0;
 {wchar_t local[1024];DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",local,900);if(n&&n<900){wcscpy(local+n,L"\\SAN14CE_ModCore\\runtime.log");log=readAll(local,&logLen);}
  if(!log){wchar_t alt[1024];pathJoin(alt,L"SAN14Mods\\Core\\runtime.log");log=readAll(alt,&logLen);}
  if(log){sess=findlast(log,logLen,"SAN14 ModCore 1.0");if(sess)sessLen=logLen-(sess-log);}
  if(!sess)checkLine(L"로더|ModCore runtime.log 를 읽지 못함 → 훅 그룹 확인 생략");
  else{const char*p=sess;size_t left=sessLen;
   while(true){const char*q=findb(p,left," RVA=");if(!q)break;checkGroups++;p=q+5;left=sessLen-(p-sess);}
   WBuf b;b.add(L"로더|이번 세션 훅 ");b.num(checkGroups);b.add(L"개 등록");checkLine(b.c());
   static const char*bad[]={"NOT installed","not installed","refused","unavailable","unknown","failed","incomplete","no game hooks changed","conflict","rejected","left inactive"};
   size_t hits[64];int nhits=0,shown=0,total=0;
   for(const char*k:bad){const char*q=sess;size_t l=sessLen;while(const char*h2=findb(q,l,k)){size_t start=(size_t)(h2-sess);q=h2+1;l=sessLen-(q-sess);
     bool dup=false;for(int i=0;i<nhits;i++)if((hits[i]>start?hits[i]-start:start-hits[i])<100)dup=true; // 같은 문장은 한 번만
     if(dup)continue;if(nhits<64)hits[nhits++]=start;total++;
     if(shown<12){size_t from=start>70?start-70:0;size_t to=start+50<sessLen?start+50:sessLen;WBuf w;w.add(L"로더 문구|...");a2w(sess+from,to-from,w);w.add(L"...");for(unsigned i=0;i<w.n;i++)if(w.b[i]==L'\r'||w.b[i]==L'\n')w.b[i]=L' ';checkLine(w.c());shown++;}}}
   if(total){WBuf pb;pb.add(L"로더 로그에 실패/거부/알 수 없음 문구 ");pb.num(total);pb.add(L"건 (check.txt의 '로더 문구' 줄 참고)");addProblem(pb.c());}
  }
 }
 // 6) 항목별 정리
 for(const Item&it:items){
  WBuf b;b.add(L"항목|");b.add(it.num);b.add(L"|");b.add(it.name);b.add(L"|");
  bool sel=haveSelection?selected[it.internalId]:false;
  int need=0,have=0;WBuf detail;
  if(it.dlls[0]){wchar_t buf[400];unsigned i=0;const wchar_t*s=it.dlls;while(true){unsigned j=0;while(s[i]&&s[i]!=L';'&&j<399)buf[j++]=s[i++];buf[j]=0;need++;
    bool exe=wends(buf,L".exe");wchar_t full[1024];pathJoin(full,buf);bool present=fileExists(full);bool loaded=!exe&&moduleLoaded(buf);
    if(loaded||(exe&&present))have++;
    detail.add(buf);detail.add(loaded?L"=로드 ":exe&&present?L"=있음 ":present?L"=파일만 있음(로드 안 됨) ":L"=없음 ");
    if(!s[i])break;i++;}}
  int groups=0;if(sess&&it.groups[0]){char g[64];unsigned i=0;const char*s=it.groups;while(true){unsigned j=0;while(s[i]&&s[i]!=';'&&j<62)g[j++]=s[i++];g[j]=0;
    const char*q=sess;size_t l=sessLen;while(const char*h2=findb(q,l,g)){const char*after=h2;size_t rest=sessLen-(h2-sess);const char*rva=findb(after,rest<96?rest:96," RVA=");if(rva)groups++;q=h2+1;l=sessLen-(q-sess);}
    if(!s[i])break;i++;}}
  bool settingOn=true,settingKnown=false;
  if(it.iniKey&&haveIni){wchar_t v[16]={};GetPrivateProfileStringW(it.iniSection,it.iniKey,L"",v,16,iniPath);if(v[0]){settingKnown=true;settingOn=v[0]!=L'0';}}
  bool full=need&&have==need&&(groups||!it.groups[0]||!sess);
  const wchar_t*state;bool problem=false;
  // R3: 15번을 14번(또는 11번)과 함께 고르면 설치기는 15번 DLL을 따로 깔지 않고 그 번호의 설명 UI를 같이 쓴다(등록 기록 provider="shared:14").
  //     이때 15번 DLL이 로드되지 않는 것은 정상이므로 문제로 세지 않는다.
  bool sharedUi=false;
  if(it.internalId==15&&sel){wchar_t rp[1024];pathJoin(rp,L"SAN14Mods\\Compat20\\Selections\\15.xml");size_t rn=0;char*rs=readAll(rp,&rn,65536);if(rs){sharedUi=contains(rs,rn,"provider=\"shared:");freeAll(rs);}}
  if(!it.dlls[0])state=L"파일 패치(여기서 확인 안 함)";
  else if(sharedUi)state=L"공유 적용(14번·11번 설명 UI를 같이 씀)";
  else if(haveSelection){
   if(sel){state=full?(settingKnown&&!settingOn?L"연결됨, 설정 OFF":L"정상"):have==0?L"선택했는데 로드 안 됨":have<need?L"일부만 로드됨":L"로드됨, 훅 없음";problem=!full;}
   else state=have?L"선택 안 함(모듈은 로드됨)":L"선택 안 함";
  }else state=full?(settingKnown&&!settingOn?L"연결됨, 설정 OFF":L"정상"):have==0?L"미설치/로드 안 됨":have<need?L"일부만 로드됨":L"로드됨, 훅 없음";
  b.add(state);b.add(L"|");b.add(detail.c());if(it.groups[0]){b.add(L"훅 ");b.num(groups);b.add(L"개 ");}if(settingKnown){b.add(L"설정 ");b.add(it.iniKey);b.add(L"=");b.add(settingOn?L"ON":L"OFF");b.add(L" ");}if(haveSelection){b.add(sel?L"[설치기에서 선택]":L"[선택 안 함]");}
  checkLine(b.c());
  if(!it.dlls[0])continue;
  if(problem){WBuf p;p.add(it.num);p.add(L"번 ");p.add(it.name);p.add(L": 설치기에서 선택했지만 ");p.add(have==0?L"모듈이 로드되지 않음":have<need?L"일부 모듈이 로드되지 않음":L"로더 로그에 훅 그룹이 없음");addProblem(p.c());}
  else if(have==need&&it.groups[0]&&sess&&!groups){WBuf p;p.add(it.num);p.add(L"번 ");p.add(it.name);p.add(L": 모듈은 로드됐지만 이번 세션 로더 로그에 훅 그룹이 없음");addProblem(p.c());}
  if(haveSelection&&!sel&&have&&have==need&&!it.iniKey&&!weq(it.dlls,L"version.dll")){WBuf p;p.add(L"참고|");p.add(it.num);p.add(L"번 ");p.add(it.name);p.add(L": 설치기 선택 기록에는 없지만 모듈이 로드됨(외부 설치 가능)");checkLine(p.c());}
 }
 {WBuf b;b.add(L"요약|등록 모듈 ");b.num(checkLoaded);b.add(L"/");b.num(checkRegistered);b.add(L" 로드|훅 ");b.num(checkGroups);b.add(L"개|문제 ");b.num(checkProblems);b.add(L"건");checkLine(b.c());logLine(b.c());}
 freeAll(log);
 __atomic_add_fetch(&checkRuns,1,__ATOMIC_SEQ_CST);InterlockedExchange(&checkState,2);return 0;
}

// ---------------------------------------------------------------- 상태 훅 (R2: 앞 모듈 위에 이어 붙이기)
using Tick4=void(*)(void*,void*,void*,void*);
volatile Tick4 nextTick;volatile LONG turnHooked;
bool fogConfigured(){wchar_t ini[1024];if(!pathJoin(ini,L"SAN14Mods\\modules\\FogFieldPair.ini"))return false;return GetPrivateProfileIntW(L"Mod",L"Enabled",0,ini)==1;}
}
extern "C" {
__declspec(dllexport) void TesterTick(U8*state,void*a,void*b,void*c){Tick4 n=nextTick;if(!n)n=fn<Tick4>(game,0x44bb40);n(state,a,b,c);if(pf.on&&!pf.done&&ready&&fn<int(*)(void*)>(game,0x56f310)(state))perfTick();pump(state,true);}
}
namespace{
// 23번(DomesticTalent)·안개 엔진이 이 칸을 먼저 쓰도록 기다린 뒤, 지금 칸에 있는 처리 위에 잇는다(앞 처리는 그대로 먼저 실행).
// 안개가 설정돼 있으면 안개가 시작 때 이 칸이 원본인지 확인하므로 원본 위에는 먼저 걸지 않는다.
DWORD WINAPI tickInstaller(LPVOID){
 U8*slot=game+0x13d6368;void*native=(void*)(game+0x44bb40);
 HMODULE dt=GetModuleHandleW(L"SAN14_DomesticTalent.dll");void*dtTick=dt?GetProcAddress(dt,"DomesticTalentTick"):nullptr;bool fog=fogConfigured();
 void*last=nullptr;unsigned stable=0;
 for(unsigned i=0;i<1200;i++){
  Sleep(500);
  void*cur=__atomic_load_n((void**)slot,__ATOMIC_ACQUIRE);
  if(cur==(void*)&TesterTick)return 0;
  if(cur==last)stable++;else{stable=0;last=cur;}
  if(stable<6)continue;                                   // 3초 동안 안 바뀌면 다른 모듈 설치가 끝난 것으로 본다
  if(dtTick&&cur!=dtTick&&i<240)continue;                 // 23번이 있으면 23번이 잡을 때까지(최대 2분)
  if(fog&&cur==native){if(i<240)continue;logLine(L"플레이어 턴 연결 안 함: 안개 설정이 있는데 턴 칸이 원본 그대로(안개 시작 검사 보호)");return 0;}
  DWORD old=0;if(!VirtualProtect(slot,8,PAGE_READWRITE,&old)){logLine(L"플레이어 턴 연결 실패: 칸 쓰기 권한 없음");return 0;}
  nextTick=(Tick4)cur;
  LONG64 seen=InterlockedCompareExchange64((volatile LONG64*)slot,(LONG64)(UPTR)&TesterTick,(LONG64)(UPTR)cur);
  if(seen!=(LONG64)(UPTR)cur){nextTick=nullptr;stable=0;last=nullptr;continue;}
  InterlockedExchange(&turnHooked,1);
  WBuf b;b.add(L"플레이어 턴 연결: ");b.add(cur==native?L"원본 처리":cur==dtTick?L"23번(DomesticTalent) 처리":L"다른 모듈 처리");b.add(L" 위에 이어 붙임");logLine(b.c());return 0;
 }
 logLine(L"플레이어 턴 연결 안 함: 10분 동안 턴 칸이 안정되지 않음");return 0;
}
}
extern "C" {
__declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!valid_api(api))return 0;core=api;game=api->resolved->image;
 DWORD n=GetModuleFileNameW(nullptr,root,1024);if(!n||n>=900)return 0;while(n&&root[n-1]!=L'\\'&&root[n-1]!=L'/')n--;if(!n)return 0;root[n]=0;rootLen=n;
 pathJoin(pathDir,L"SAN14Mods");CreateDirectoryW(pathDir,nullptr);pathJoin(pathDir,L"SAN14Mods\\PatchTester");CreateDirectoryW(pathDir,nullptr);
 pathJoin(pathPerf,L"SAN14Mods\\PatchTester\\perf.txt");pathJoin(pathArmies,L"SAN14Mods\\PatchTester\\armies.txt");DeleteFileW(pathArmies);pathJoin(pathLog,L"SAN14Mods\\PatchTester\\PatchTester.log");pathJoin(pathFlag,L"SAN14Mods\\PatchTester\\test.flag");pathJoin(pathResult,L"SAN14Mods\\PatchTester\\result.txt");pathJoin(pathCheck,L"SAN14Mods\\PatchTester\\check.txt");
 {WBuf b;b.add(L"PatchTester R5 시작 pid=");b.num(GetCurrentProcessId());b.add(L" / 점검 시작, 플레이어 턴 연결은 다른 모듈 설치 뒤 이어 붙임");logLine(b.c());}
 ready=true;
 HANDLE t=CreateThread(nullptr,0,checkThread,nullptr,0,nullptr);if(t)CloseHandle(t);else InterlockedExchange(&checkState,2);
 HANDLE u=CreateThread(nullptr,0,tickInstaller,nullptr,0,nullptr);if(u)CloseHandle(u);
 return 1;
}
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
