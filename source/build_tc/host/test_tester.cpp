// SAN14_PatchTester 논리 검사 (리눅스, 실제 파일 + 가짜 게임 세계)
// clang++ -std=c++17 -fshort-wchar -fdeclspec -Wno-unknown-attributes -I host -I src -O1 -g host/test_tester.cpp src/patch_tester.cpp -o host/ttester -lpthread
#include "winstubs.h"
extern "C" int SAN14_ModInitV1(const S14_CoreAPI*);
// ------------------------------------------------------------------ 가짜 게임 세계
static U8*game;static U8*gmBlock;static U8*S;
struct Person{U8 raw[0x200];};struct City{U8 raw[0x170];int owner;int cellIndex;U8 cell[0x40];};struct Force{U8 raw[0x260];int ruler;const wchar_t*name;};
static Person persons[64];static City cities[64];static Force forces[16];static U8 districts[8][0x30];static U8 orderObj[0x80];static int orderOwner=-1;
static bool prisonerPatched=true;static int capturedHome=-1;static std::vector<std::string> history;static int historyDistrict=-1,historyCell=-1;
static void thunk(U8*at,void*target){at[0]=0x48;at[1]=0xb8;memcpy(at+2,&target,8);at[10]=0xff;at[11]=0xe0;}
static void native(U32 rva,void*t){thunk(game+rva,t);}
template<class T>T&at(void*p,size_t o){return *reinterpret_cast<T*>(static_cast<U8*>(p)+o);}
static U8*P(int id){return persons[id].raw;}
static void setName(U8*p,const wchar_t*sur,const wchar_t*giv){wcscpy((wchar_t*)(p+0x12),sur);wcscpy((wchar_t*)(p+0x20),giv);}
static void makePerson(int id,const wchar_t*sur,const wchar_t*giv,int district,int city,int status,int rank){U8*p=P(id);memset(p,0,0x200);at<U8*>(p,0)=game+0x13a6b38;at<U16>(p,0x10)=id;setName(p,sur,giv);p[0xe0]=district;at<U16>(p,0xe2)=city;p[0xe6]=status;p[0xe7]=rank;at<U8*>(gmBlock,0x158+8*id)=p;}
static void makeCity(int id,const wchar_t*name,int owner,int district,int x,int y){City&c=cities[id];memset(c.raw,0,sizeof c.raw);at<U8*>(c.raw,0)=game+0x13a6640;at<U16>(c.raw,0x4e)=id;wcscpy((wchar_t*)(c.raw+0x12),name);c.raw[0x30]=district;at<U16>(c.raw,0x46)=0x1770;c.owner=owner;c.cellIndex=y*220+x;}
static void makeForce(int id,int ruler,const wchar_t*name){Force&f=forces[id];memset(f.raw,0,sizeof f.raw);at<U8*>(f.raw,0)=game+0x13a67f0;f.ruler=ruler;f.name=name;at<U8*>(gmBlock,0xdf30+8*id)=f.raw;}
static int personForceId(U8*p){return at<int>(districts[p[0xe0]],0x10);}
static City*cityOfId(int id){return (id>0&&id<64&&cities[id].raw[0])?&cities[id]:nullptr;}
extern "C" {
static int m_valid(void*o){return o!=nullptr;}
static U8*m_cityById(U8*,int id){City*c=cityOfId(id);return c?c->raw:nullptr;}
static U8*m_cityCell(U8*c){return ((City*)c)->cell;}
static int m_cellIndex(U8*cell){return ((City*)(cell-offsetof(City,cell)))->cellIndex;}
static int m_personForceId(U8*p){return personForceId(p);}
static U8*m_personForce(U8*p){return forces[personForceId(p)].raw;}
static void m_container(U8*p,int*type,int*idx){int st=p[0xe6];*type=st==6?5:2;*idx=at<U16>(p,0xe2);}
static int m_busy(U8*){return 0;}
static int m_isRuler(U8*p){return forces[personForceId(p)].ruler==at<U16>(p,0x10);}
static U8*m_order(U8*p){return orderOwner==at<U16>(p,0x10)?orderObj:nullptr;}
static U8*m_rulerCity(U8*f){Force*F=(Force*)f;return cities[at<U16>(P(F->ruler),0xe2)].raw;}
static U8*m_areaCity(U8*c){return c;}
static int m_prep(U8*){return 0;}
static void m_capture(U8*p,void*army,int flag,int x,U8*force,void*cp){(void)army;(void)flag;(void)x;(void)cp;capturedHome=at<U16>(p,0xe2);if(!prisonerPatched)p[0xe7]=0x49;p[0xe6]=6;at<U16>(p,0xe2)=at<U16>(m_rulerCity(force),0x4e);}
static int m_release(U8*p,int){p[0xe6]=4;int f=personForceId(p);int nat=at<U16>(m_rulerCity(forces[f].raw),0x4e);at<U16>(p,0xe2)=prisonerPatched?capturedHome:nat;return 1;}
static long long m_days(int steps,void*,void*){return (steps*4+9)/10;}
static int m_issueValid(void*){return 1;}
static void m_issue(U8*in,int){U8*exec=at<U8*>(in,8);memset(orderObj,0,sizeof orderObj);at<U8*>(orderObj,0)=game+0x13a1f70;orderOwner=at<U16>(exec,0x10);}
static U8*m_history(void*,int kind,int district,int cell,int flag,int date,const wchar_t*title,const wchar_t*body,int speaker,int,int,int,int){(void)kind;(void)flag;(void)date;(void)speaker;historyDistrict=district;historyCell=cell;history.push_back(u8(title)+" / "+u8(body));printf("    [기록창] %s / %s\n",u8(title).c_str(),u8(body).c_str());return nullptr;}
static void*m_histMgr(){return (void*)0x1234;}
static int m_top(void*){return 1;}
static const wchar_t*m_forceName(U8*f){return ((Force*)f)->name;}
static int m_cityOwner(U8*c){return ((City*)c)->owner;}
static UPTR m_enter(void*,void*,UPTR,UPTR){return 7;}
static void m_tick(void*){}
// R4 성능 측정용: 정적 상태 관리자(+0x10 개수, +0x20 배열), CGameState 찾기, 진행 요청 세터
static U8 stUser[0x40],stProgress[0x40],stGame[0x500];static U8*stArr[8];static int reqCount=0;static volatile int reqFlag=0;
static U8*m_findState(U8*mgr,const char*name){(void)mgr;return strcmp(name,"CGameState")==0?stGame:nullptr;}
static void m_setReq(U8*gs,int v){if(gs==stGame){at<int>(gs,0x47c)=v;reqFlag=v;reqCount++;}}
}
static S14_Resolved resolved;static S14_CoreAPI api;static std::vector<std::string> logs;
static I32 Readable(const void*p,UPTR n){UPTR a=(UPTR)p;if(!a||a<0x10000)return 0;UPTR page=a&~(UPTR)0xfff,end=(a+n+0xfff)&~(UPTR)0xfff;unsigned char v;for(UPTR q=page;q<end;q+=0x1000)if(mincore((void*)q,0x1000,&v))return 0;return 1;}
static I32 InstallHooks(const char*id,const S14_Hook*h,U32 n){for(U32 i=0;i<n;i++)if(memcmp(h[i].target,h[i].expected,h[i].length)){printf("    InstallHooks 거부 %s\n",id);return 0;}for(U32 i=0;i<n;i++)memcpy(h[i].target,h[i].replacement,h[i].length);return 1;}
static U8*MakeRelay(const void*,const void*){return nullptr;}static void FreeRelay(void*){}
static void Log(const char*s){logs.push_back(s);printf("    [로더 기록] %s\n",s);}
static void buildWorld(){
 game=(U8*)mmap((void*)0x140000000,0x29ff000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);if(game==MAP_FAILED){perror("game");exit(2);}
 gmBlock=(U8*)calloc(1,0x100000);S=(U8*)calloc(1,0x2000);at<U8*>(game,0x277df18)=gmBlock;at<U8*>(gmBlock,0x871e8)=S;S[0x3a]=2;at<int>(S,0x450)=1000;
 for(int i=0;i<8;i++)at<U8*>(gmBlock,0xe0d0+8*i)=districts[i];
 at<int>(districts[0],0x10)=2;at<int>(districts[1],0x10)=7;at<int>(districts[2],0x10)=9;
 makeForce(2,1,L"유비군");makeForce(7,10,L"조조군");makeForce(9,20,L"손권군");
 makeCity(5,L"성도",2,0,10,10);makeCity(6,L"한중",2,0,12,8);makeCity(20,L"허창",7,1,30,10);makeCity(21,L"진류",7,1,14,10);makeCity(40,L"건업",9,2,60,60);
 at<U16>(cities[5].raw,0x46)=2; // 성도 태수 = 관우
 makePerson(1,L"유",L"비",0,5,1,0x30);makePerson(2,L"관",L"우",0,5,1,60);makePerson(3,L"장",L"비",0,5,1,65);makePerson(4,L"조",L"운",0,5,1,70);
 makePerson(10,L"조",L"조",1,20,1,0x30);makePerson(11,L"하후",L"돈",1,21,1,62);makePerson(12,L"순",L"욱",1,20,1,58);makePerson(13,L"장",L"료",1,21,1,70);
 makePerson(20,L"손",L"권",2,40,1,0x30);
 makePerson(30,L"서",L"서",1,21,5,0x49);makePerson(31,L"방",L"통",0,5,5,0x49);makePerson(32,L"황",L"충",2,40,5,0x49);
 // vtable 자리: 상태 Enter/Tick 은 게임 RVA 로 가리켜야 훅 기대값이 맞는다
 native(0x449150,(void*)m_enter);native(0x44bb40,(void*)m_tick);at<U8*>(game,0x13d6348)=game+0x449150;at<U8*>(game,0x13d6368)=game+0x44bb40;
 at<void*>(game,0x13a6640+0xb0)=(void*)m_cityOwner;
 native(0x32e4c0,(void*)m_valid);native(0x32b110,(void*)m_cityById);native(0x22ca60,(void*)m_cityCell);native(0x22d2c0,(void*)m_cellIndex);native(0x22c380,(void*)m_personForceId);native(0x22bf60,(void*)m_personForce);
 native(0x231190,(void*)m_container);native(0x231770,(void*)m_busy);native(0x232b90,(void*)m_isRuler);native(0x229fc0,(void*)m_order);native(0x22dde0,(void*)m_rulerCity);native(0x229d00,(void*)m_areaCity);
 native(0x26b550,(void*)m_prep);native(0x2e0000,(void*)m_capture);native(0x2e68d0,(void*)m_release);native(0x230cc0,(void*)m_days);native(0x1ef5d0,(void*)m_issueValid);native(0x1ed070,(void*)m_issue);
 native(0x9338c0,(void*)m_history);native(0x16e290,(void*)m_histMgr);native(0x56f310,(void*)m_top);native(0x22e1f0,(void*)m_forceName);
 resolved={};resolved.size=sizeof resolved;resolved.abi=S14_ABI;resolved.layout_abi=1;resolved.image_size=0x29ff000;resolved.image=game;
 api={};api.size=sizeof api;api.abi=S14_ABI;api.resolved=&resolved;api.Readable=Readable;api.InstallHooks=InstallHooks;api.MakeRelay=MakeRelay;api.FreeRelay=FreeRelay;api.Log=Log;
}
static unsigned p27inst=1,p27flags=1;static int fakeLoads=0;static bool fakeRecordOn=true;extern "C"{static unsigned m_p27i(){return p27inst;}static unsigned m_p27f(){return p27flags;}
static int m_prRecord(unsigned id,int*h,int*r,int*f){if(!fakeRecordOn||id!=13)return 0;if(h)*h=21;if(r)*r=70;if(f)*f=7;return 1;}static int m_prLoads(){return fakeLoads;}static int m_prVersion(){return 2;}}
static void buildFiles(bool withPrisoner){
 writeFile(tmpRoot+"/game/SAN14Mods/modules/IntegratedConvenience_GreenlandCompat.ini","[Mod]\r\nId=SAN14CE.IntegratedConvenience.Compat\r\nFile=SAN14_IntegratedConvenience.dll\r\nEnabled=1\r\n");
 writeFile(tmpRoot+"/game/SAN14Mods/modules/Patch27.ini","[Mod]\r\nId=SAN14CE.Patch27\r\nFile=SAN14_Patch27.dll\r\nEnabled=1\r\n");
 writeFile(tmpRoot+"/game/SAN14Mods/modules/PrisonerReturn.ini","[Mod]\r\nId=SAN14CE.PrisonerReturn\r\nFile=SAN14_PrisonerReturn.dll\r\nEnabled=1\r\n");
 writeFile(tmpRoot+"/game/SAN14Mods/modules/Ghost.ini","[Mod]\r\nId=X\r\nFile=SAN14_Ghost.dll\r\nEnabled=1\r\n");
 writeFile(tmpRoot+"/game/SAN14Mods/modules/Off.ini","[Mod]\r\nId=Y\r\nFile=SAN14_Off.dll\r\nEnabled=0\r\n");
 writeFile(tmpRoot+"/game/SAN14_IntegratedConvenience.dll","x");writeFile(tmpRoot+"/game/SAN14_Patch27.dll","x");writeFile(tmpRoot+"/game/SAN14_FPS120.dll","x");
 if(withPrisoner)writeFile(tmpRoot+"/game/SAN14_PrisonerReturn.dll","x");
 writeFile(tmpRoot+"/game/San14ModSettings.ini","[Convenience]\r\nSkipReports=1\r\nPrisonerPersuasion=0\r\n[Modules]\r\nModSettingsUI=1\r\n");
 writeFile(tmpRoot+"/game/.san14ce_compat_r3/state.xml","<?xml version=\"1.0\"?><State><Selected><Feature id=\"1\" enabled=\"true\"/><Feature id=\"5\" enabled=\"true\"/><Feature id=\"15\" enabled=\"true\"/><Feature id=\"20\" enabled=\"true\"/><Feature id=\"23\" enabled=\"true\"/><Feature id=\"28\" enabled=\"true\"/><Feature id=\"4\" enabled=\"false\"/></Selected></State>");
writeFile(tmpRoot+"/game/SAN14Mods/Compat20/Selections/15.xml","\xef\xbb\xbf<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<Compat20Selection id=\"15\" provider=\"shared:14\" />");
 std::string log="SAN14 ModCore 1.0 / old session\r\nSAN14CE.FPS120.R8 RVA=00151647 len=5\r\n";
 log+="SAN14 ModCore 1.0 / standalone SortiePresets RC23 startup\r\nCE KO: timestamp 6a6d9f72\r\nPatch27 R3: six settings pages attached.SAN14CE.Patch27.RemoteRecruit RVA=006dd049 len=5\r\nSAN14CE.Patch27.BattleDeaths RVA=002589a2 len=5\r\n";
 if(withPrisoner)log+="SAN14CE.PrisonerReturn RVA=002e0000 len=5\r\nSAN14CE.PrisonerReturn RVA=002385e0 len=6\r\nPrisonerReturn loaded: capture rank retained.";
 log+="Scenery R4.3: discovery movie skip NOT installed (conflict/version). Other patches preserved.\r\n";
 writeFile(tmpRoot+"/local/SAN14CE_ModCore/runtime.log",log);
}
static void joinThreads(){for(auto&t:threads)pthread_join(t,nullptr);threads.clear();}
static bool has(const std::string&s,const char*needle){return s.find(needle)!=std::string::npos;}
typedef void(*Tick4)(void*,void*,void*,void*);
static int dtCalls=0;extern "C" { static void m_dtTick(void*st,void*a,void*b,void*c){dtCalls++;((Tick4)(game+0x44bb40))(st,a,b,c);} }
static void callTick(){U8 state[0x600]={};tickNow+=1000;((Tick4)at<void*>(game,0x13d6368))(state,nullptr,nullptr,nullptr);}
static void callEnter(){callTick();}
static void nextDay(){at<int>(S,0x450)++;callTick();}
int main(){
 char tmpl[]="/tmp/ttester_XXXXXX";tmpRoot=mkdtemp(tmpl);mkdirs(tmpRoot+"/game");mkdirs(tmpRoot+"/local");
 buildWorld();buildFiles(true);
 // 모듈 목록: 통합편의(R11 확인값), 27번, 22번
 const char*r11="9c50b73bdfe13dbfd9bfea0f3deb560882d6892aea45df78b13c57f4a8d7c282";for(int i=0;i<32;i++)sscanf(r11+2*i,"%2hhx",&icDigest[i]);
 modules["san14_integratedconvenience.dll"]=(void*)0x1001;modules["san14_patch27.dll"]=(void*)0x1002;modules["san14_prisonerreturn.dll"]=(void*)0x1003;modules["san14_fps120.dll"]=(void*)0x1004;
 moduleNames["san14_integratedconvenience.dll"]="SAN14_IntegratedConvenience.dll";moduleNames["san14_patch27.dll"]="SAN14_Patch27.dll";moduleNames["san14_prisonerreturn.dll"]="SAN14_PrisonerReturn.dll";moduleNames["san14_fps120.dll"]="SAN14_FPS120.dll";
 procs["Patch27Installed"]=(void*)m_p27i;procs["Patch27Flags"]=(void*)m_p27f;procs["PrisonerReturnRecord"]=(void*)m_prRecord;procs["PrisonerReturnLoads"]=(void*)m_prLoads;procs["PrisonerReturnVersion"]=(void*)m_prVersion;
 // 23번(DomesticTalent)이 먼저 턴 칸을 잡은 상태 (기본 설치와 같음)
 modules["san14_domestictalent.dll"]=(void*)0x1005;moduleNames["san14_domestictalent.dll"]="SAN14_DomesticTalent.dll";procs["DomesticTalentTick"]=(void*)m_dtTick;at<void*>(game,0x13d6368)=(void*)m_dtTick;
 printf("1) 모듈 초기화와 연결 점검\n");
 int ok=SAN14_ModInitV1(&api);check(ok==1,"ModInit 성공 (23번이 턴 칸을 먼저 잡아도)");
 joinThreads();
 check(at<void*>(game,0x13d6348)==game+0x449150,"Enter 칸은 건드리지 않음");
 check(at<void*>(game,0x13d6368)!=(void*)m_dtTick&&at<void*>(game,0x13d6368)!=game+0x44bb40,"Tick 칸: 23번 처리 위에 시험 모듈이 이어 붙음");
 check(has(readFile(tmpRoot+"/game/SAN14Mods/PatchTester/PatchTester.log"),"플레이어 턴 연결: 23번(DomesticTalent) 처리 위에 이어 붙임"),"연결 기록");
 std::string chk=readFile(tmpRoot+"/game/SAN14Mods/PatchTester/check.txt");printf("%s\n",chk.c_str());
 check(has(chk,"세션|pid=4242"),"check.txt 세션 줄");
 check(has(chk,"등록|Ghost.ini|SAN14_Ghost.dll|로드 안 됨")&&has(chk,"문제|등록 모듈이 로드되지 않음: SAN14_Ghost.dll"),"등록됐지만 로드 안 된 모듈을 문제로 보고");
 check(has(chk,"등록|Off.ini|SAN14_Off.dll|꺼짐(Enabled=0)")&&!has(chk,"SAN14_Off.dll (Off.ini)"),"Enabled=0 모듈은 문제 아님");
 check(has(chk,"통합편의|로드됨|확인값 일치"),"통합편의 확인값");
 check(has(chk,"26번|설치 비트=01 설정 비트=01|타세력 등용 ON"),"27번 비트 보고");
 check(has(chk,"선택|설치 기록에서 선택된 항목(내부 번호): 1 5 15 20 23 28"),"state.xml 선택 읽기");
 check(has(chk,"항목|15|개성 마우스 설명|공유 적용(14번·11번 설명 UI를 같이 씀)|")&&!has(chk,"문제|15번"),"R3: 14번 설명 UI를 공유하는 15번은 문제 아님");
 check(has(chk,"항목|22|포로 관직 유지 · 원래 소속 도시 귀환|정상|SAN14_PrisonerReturn.dll=로드 훅 2개 [설치기에서 선택]"),"22번 항목 정상");
 check(has(chk,"22번|모듈 판 R2|세이브 슬롯 사이드카"),"22번 모듈 판 표시");
 check(has(chk,"항목|26|등용·관직·전사 알림·신무장 이름 확장|정상|SAN14_Patch27.dll=로드 훅 2개 [설치기에서 선택]"),"27번 항목 정상");
 check(!has(chk,"다른 군단")&&!has(chk,"Legion")&&has(chk,"항목|25|수여로 오르는 능력치 미적용|"),"R3: 옛 25번 줄 없음, 수여 능력치는 25번");
 check(has(chk,"항목|20|120프레임|로드됨, 훅 없음")&&has(chk,"문제|20번 120프레임: 설치기에서 선택했지만 로더 로그에 훅 그룹이 없음"),"20번: 선택했지만 훅 없음 → 문제");
 check(has(chk,"항목|5|명승지·보고·연출|일부만 로드됨")&&has(chk,"문제|5번 명승지·보고·연출: 설치기에서 선택했지만 일부 모듈이 로드되지 않음"),"5번: 설명창 모듈 없음 → 일부만 로드");
 check(has(chk,"항목|10|이민족 성 일반화·지역·증축|파일 패치(여기서 확인 안 함)|[선택 안 함]"),"파일 패치 항목은 빈 상세");
 check(!has(chk,"참고|4번"),"통합편의 기능 항목에는 참고 줄 없음");
 check(has(chk,"항목|1|인재·포로 편의|연결됨, 설정 OFF|")&&has(chk,"설정 PrisonerPersuasion=OFF"),"1번: 설정 OFF 표시");
 check(has(chk,"항목|4|문전박대|선택 안 함(모듈은 로드됨)"),"4번: 선택 안 함");
 check(has(chk,"로더 로그에 실패/거부/알 수 없음 문구 1건"),"로더 문제 문구 집계(같은 문장은 한 번)");
 check(has(chk,"요약|등록 모듈 3/5 로드"),"요약 줄");
 {int before=dtCalls;callTick();check(dtCalls==before+1,"이어 붙은 뒤에도 23번 처리가 매번 먼저 실행됨");}
 printf("2) 신호 없이 Tick → 아무 일 없음 (첫 턴에 점검 다시 → 요약)\n");
 joinThreads();chk=readFile(tmpRoot+"/game/SAN14Mods/PatchTester/check.txt");check(has(chk,"세션|pid=4242|첫 플레이어 턴|"),"첫 플레이어 턴에 점검을 다시 함");callTick();callEnter();check(!has(readFile(tmpRoot+"/game/SAN14Mods/PatchTester/result.txt"),"START"),"신호 없으면 결과 파일 없음");
 check(history.size()>=1&&has(history[0],"[패치 점검] 모듈 3/5 로드, 훅 4개, 문제 4건"),"기록창에 점검 요약");
 check(historyDistrict==0&&historyCell==10*220+10,"기록창 줄은 플레이어 수도 칸에 붙임");
 printf("3) 신호 파일 → 22·27 재현 (Tick에서 바로)\n");
 writeFile(tmpRoot+"/game/SAN14Mods/PatchTester/test.flag","all\r\n");history.clear();
 callTick();
 std::string res=readFile(tmpRoot+"/game/SAN14Mods/PatchTester/result.txt");printf("%s\n",res.c_str());
 check(access((tmpRoot+"/game/SAN14Mods/PatchTester/test.flag").c_str(),F_OK)!=0,"신호 파일 삭제됨");
 check(has(res,"START|day=1000|tests=22 26|player=2"),"START 줄");
 check(has(res,"22|포로재현|OK|무장=장료(조조군) 원래 도시=진류(21) 보관 도시=성도(5) 순정 해방 목적지=허창(20) 상태=6"),"22: 장료(관직 가장 낮음, 군주 도시와 다른 도시) 선택·포로 재현");
 check(P(13)[0xe6]==6&&at<U16>(P(13),0xe2)==5,"장료가 성도의 포로가 됨");
 check(has(res,"22|관직유지|PASS|포로 전 관직 70 → 포로 후 70"),"22: 관직 유지 PASS");
 check(has(res,"22|기록|PASS|22번 모듈(R2)에 원래 도시 기록 있음: 진류(21)"),"22: R2 기록 존재 PASS");
 check(has(res,"26|명령발행|OK|실행=조운@성도(5) 대상=서서@진류(21)(조조군 소유) 직선 최소 2일 / 26번 타세력 등용 설정 ON"),"27: 조운(태수 관우 제외, 관직 가장 낮음)→서서 명령 발행");
 check(orderOwner==4,"등용 명령 객체가 조운에게 생김");
 check(history.size()==2&&has(history[0],"[시험22] 장료 포로 재현 / 관직 유지 확인.")&&has(history[1],"[시험26] 조운→서서 등용 명령"),"기록창 두 줄");
 printf("4) 하루 뒤: 해방 → 귀환 경로 판정, 27 진행 (그 사이 저장→불러오기 1회 흉내)\n");
 fakeLoads=1;nextDay();res=readFile(tmpRoot+"/game/SAN14Mods/PatchTester/result.txt");
 check(has(res,"22|귀환경로|PASS|시험 모듈이 해방. 목적지=진류(21) 원래 도시=진류(21) 순정 목적지=허창(20) 해방 후 관직=70 상태=4"),"22: 귀환 경로 PASS");
 check(has(res,"26|진행|OK|1일차 등용 명령 진행 중"),"27: 1일차 진행");
 check(has(res,"22|저장복원|PASS|불러오기 1회 뒤 원래 도시 기록 유지: 진류(21)"),"22: 불러오기 뒤 기록 유지 PASS");
 check(!has(res,"DONE"),"아직 DONE 아님");
 printf("5) 이틀 뒤 명령 종료(등용 성공) → 27 PASS, DONE\n");
 nextDay();orderOwner=-1;P(30)[0xe6]=1;P(30)[0xe0]=0;at<U16>(P(30),0xe2)=5;nextDay();res=readFile(tmpRoot+"/game/SAN14Mods/PatchTester/result.txt");
 check(has(res,"26|진행|OK|2일차"),"27: 2일차 진행");
 check(has(res,"26|결과|PASS|3일 만에 종료 (직선 최소 2일). 대상 서서: 등용 성공(우리 세력 임관)"),"27: 결과 PASS");
 check(has(res,"DONE|시험 끝"),"DONE 기록");
 printf("6) 귀환 완료 소식 (DONE 뒤)\n");
 P(13)[0xe6]=1;nextDay();res=readFile(tmpRoot+"/game/SAN14Mods/PatchTester/result.txt");
 check(has(res,"22|귀환완료|OK|3일 만에 원래 도시 진류(21) 복귀, 관직=70"),"22: 귀환 완료");
 printf("7) 순정 동작(22번 모듈 없음) + 27번 1일 취소 → FAIL/NATIVE\n");
 modules.erase("san14_prisonerreturn.dll");prisonerPatched=false;
 makePerson(13,L"장",L"료",1,21,1,70);makePerson(30,L"서",L"서",1,21,5,0x49);
 writeFile(tmpRoot+"/game/SAN14Mods/PatchTester/test.flag","22 26");callTick();
 res=readFile(tmpRoot+"/game/SAN14Mods/PatchTester/result.txt");
 check(has(res,"22|관직유지|NATIVE|포로 전 관직 70 → 포로 후 73 (관직 해제 = 순정 동작) [22번 모듈 미로드]"),"22: 모듈 없으면 NATIVE");
 check(access((tmpRoot+"/game/SAN14Mods/PatchTester/result_prev.txt").c_str(),F_OK)==0,"이전 결과는 result_prev.txt 로");
 orderOwner=-1;nextDay();res=readFile(tmpRoot+"/game/SAN14Mods/PatchTester/result.txt");
 check(has(res,"22|귀환경로|NATIVE|시험 모듈이 해방. 목적지=허창(20) 원래 도시=진류(21) 순정 목적지=허창(20)"),"22: 순정 목적지 → NATIVE");
 check(has(res,"26|결과|FAIL|1일 만에 명령 종료(취소). 1일 만에 종료 (직선 최소 2일). 대상 서서: 재야 그대로(설득 실패 또는 취소)"),"27: 1일 취소 → FAIL");
 check(has(res,"DONE"),"DONE");
 printf("8) 재야 후보 없음 → SKIP\n");
 P(30)[0xe6]=1;P(32)[0xe6]=1;writeFile(tmpRoot+"/game/SAN14Mods/PatchTester/test.flag","27");callTick();res=readFile(tmpRoot+"/game/SAN14Mods/PatchTester/result.txt");
 check(has(res,"26|명령발행|SKIP|다른 세력 도시에 재야 무장이 없음")&&has(res,"DONE|재현할 수 있는 시험이 없었습니다"),"27: 후보 없음 SKIP + DONE");
 printf("8b) 필드 부대 관찰: 수송 진형 부대가 armies.txt 에 기록됨\n");
 {static U8 army[0x80];memset(army,0,sizeof army);at<U8*>(army,0)=game+0x1342458;at<U16>(army,0x12)=4;army[0x1b]=0x13;at<U8*>(gmBlock,0x7e838+8*3)=army;
  native(0x2320f0,(void*)+[](U8*a)->int{return a[0x1b]>=0x13&&a[0x1b]<=0x14;});native(0x232400,(void*)+[](U8*)->int{return 0;});
  static U8 form13[0x80];form13[0x7e]=1;at<U8*>(gmBlock,0x77428+8*0x13)=form13;
  callTick();callTick();std::string ar=readFile(tmpRoot+"/game/SAN14Mods/PatchTester/armies.txt");
  check(has(ar,"진형표|")&&has(ar,"13="),"진형표 줄(수송 항목 상태값 자리)");
  check(has(ar,"부대|3|무장=조운|진형=0x13(수송)|공성=0|날짜="),"수송 부대 한 줄 기록");
  callTick();check(ar==readFile(tmpRoot+"/game/SAN14Mods/PatchTester/armies.txt"),"같은 부대는 다시 안 적음");
  army[0x1b]=0;callTick();check(has(readFile(tmpRoot+"/game/SAN14Mods/PatchTester/armies.txt"),"진형=0x00(보통)"),"진형이 바뀌면 다시 적음");
  at<U8*>(gmBlock,0x7e838+8*3)=nullptr;callTick();check(has(readFile(tmpRoot+"/game/SAN14Mods/PatchTester/armies.txt"),"부대 사라짐|3"),"사라진 부대 기록");}
 printf("9) 성능 측정: 신호 perf → 게임 혼자 진행(진행 요청) + 상태 전환 시각 기록 → 정한 횟수에서 DONE\n");
 {U8*mgr=game+0x1b620b0;at<U8*>(stUser,0)=game+0x13d6340;at<U8*>(stProgress,0)=game+0x13d6610;stArr[0]=stUser;at<U8**>(mgr,0x20)=stArr;at<UPTR>(mgr,0x10)=1;
  native(0x56f130,(void*)m_findState);native(0x450360,(void*)m_setReq);history.clear();
  writeFile(tmpRoot+"/game/SAN14Mods/PatchTester/test.flag","perf warm=0 retry=500 runs=3 minutes=120");
  callTick(); // 신호를 읽고 측정 시작(같은 턴에는 바로 요청하지 않을 수도 있음)
  int guard=0;
  while(guard++<40){
   callTick();
   if(reqFlag){ // 게임이 요청을 받아 진행 상태로 넘어갔다가 10일 뒤 플레이어 턴으로 돌아오는 것을 흉내
    reqFlag=0;at<int>(stGame,0x47c)=0;stArr[1]=stProgress;at<UPTR>(mgr,0x10)=2;usleep(150000);at<int>(S,0x450)+=10;at<UPTR>(mgr,0x10)=1;usleep(80000);
   }
   if(has(readFile(tmpRoot+"/game/SAN14Mods/PatchTester/result.txt"),"PERF|DONE"))break;
  }
  joinThreads();
  std::string pr=readFile(tmpRoot+"/game/SAN14Mods/PatchTester/perf.txt");printf("%s\n",pr.c_str());res=readFile(tmpRoot+"/game/SAN14Mods/PatchTester/result.txt");
  check(has(res,"PERF|시작|date=")&&has(res,"0초 쉬고 나서 최대 3번 또는 120분 동안 혼자 진행"),"PERF 시작 줄(신호 옵션 반영)");
  check(reqCount>=3,"플레이어 턴마다 게임 자신의 진행 요청 세터로 요청");
  check(has(res,"PERF|DONE|정한 횟수 끝|")&&has(res,"플레이어 턴 복귀 3회"),"진행 3회 뒤 DONE");
  check(has(pr,"STATE|")&&has(pr,"13d6340→13d6610")&&has(pr,"13d6610→13d6340"),"상태 전환(플레이어 턴↔진행) 시각 기록");
  check(has(pr,"SUM|vt=13d6610(진행)|횟수=3|"),"진행 상태 합계 3회");
  int before=reqCount;callTick();callTick();check(reqCount==before,"DONE 뒤에는 더 요청하지 않음");
  check(history.size()==2&&has(history[0],"[성능 측정] 시작")&&has(history[1],"[성능 측정] 끝"),"기록창 시작·끝 알림");
 }
 printf("기록 파일:\n%s\n",readFile(tmpRoot+"/game/SAN14Mods/PatchTester/PatchTester.log").c_str());
 printf("결과: 통과 %d / 실패 %d\n",passes,failures);return failures?1:0;
}
