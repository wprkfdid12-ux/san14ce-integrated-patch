// SAN14_PrisonerReturn R3 논리 검사 (리눅스): 가짜 게임 이미지에 실제 훅 바이트를 심고 포획→저장→불러오기→해방을 실행
// clang++ -std=c++17 -fshort-wchar -fdeclspec -Wno-unknown-attributes -Wno-ignored-attributes -I host -I src -O1 -g host/test_prisoner.cpp src/prisoner_return.cpp host/prisoner_bridge_host.S -o host/tprisoner -lpthread
#include "winstubs.h"
extern "C" {
int SAN14_ModInitV1(const S14_CoreAPI*);
int PrisonerReturnRecord(unsigned,int*,int*,int*);int PrisonerReturnLoads();int PrisonerReturnSaves();int PrisonerReturnVersion();
}
static bool has(const std::string&s,const char*needle){return s.find(needle)!=std::string::npos;}
// ------------------------------------------------------------------ 가짜 게임 세계
static U8*game;static U8*gmBlock;
struct City{U8 raw[0x170];int owner;int district;};
static U8 persons[64][0x200];static City cities[64];static U8 districts[8][0x30];
template<class T>T&at(void*p,size_t o){return *reinterpret_cast<T*>(static_cast<U8*>(p)+o);}
static void thunk(U8*at_,void*target){at_[0]=0x48;at_[1]=0xb8;memcpy(at_+2,&target,8);at_[10]=0xff;at_[11]=0xe0;}
static U8*P(int id){return persons[id];}
static void makePerson(int id,const wchar_t*sur,const wchar_t*giv,int district,int city,int status,int rank){U8*p=P(id);memset(p,0,0x200);at<U8*>(p,0)=game+0x13a6b38;at<U16>(p,0x10)=id;wcscpy((wchar_t*)(p+0x12),sur);wcscpy((wchar_t*)(p+0x20),giv);p[0xe0]=district;at<U16>(p,0xe2)=city;p[0xe6]=status;p[0xe7]=rank;at<U8*>(gmBlock,0x158+8*id)=p;}
static void makeCity(int id,int owner,int district){City&c=cities[id];memset(c.raw,0,sizeof c.raw);at<U8*>(c.raw,0)=game+0x13a6640;at<U16>(c.raw,0x4e)=id;c.owner=owner;c.district=district;}
static int captureCalls,travelCalls,serializeCalls,setDistrictCalls,lastDistrict;static U8*lastTravel;static int capturedR9;static void*capturedForce;
extern "C" {
// 0x2e0005 이후: 원본 포로 처리 흉내 — 관직 없음(0x49), 상태 6, 보관 도시 = 포획 세력 군주 도시(5)
static int m_capture(U8*p,void*army,int flag,int r9,void*force,void*cp){(void)army;(void)flag;(void)cp;captureCalls++;capturedR9=r9;capturedForce=force;p[0xe7]=0x49;p[0xe6]=6;at<U16>(p,0xe2)=5;return 0x5a;}
static int m_travel(U8*p,int,int){travelCalls++;lastTravel=p;return 1;}
static UPTR m_serialize(U8*,U8*,UPTR a,UPTR b){serializeCalls++;return 0x7000+a+b;}
static U8*m_cityById(U8*,int id){return (id>0&&id<64&&cities[id].raw[0])?cities[id].raw:nullptr;}
static void m_setDistrict(U8*p,int d,int){setDistrictCalls++;lastDistrict=d;p[0xe0]=d;}
static int m_cityOwner(U8*c){return ((City*)c)->owner;}
static int m_cityDistrict(U8*c){return ((City*)c)->district;}
}
static S14_Resolved resolved;static S14_CoreAPI api;static std::vector<std::string> coreLogs;
static I32 Readable(const void*p,UPTR n){UPTR a=(UPTR)p;if(!a||a<0x10000)return 0;UPTR page=a&~(UPTR)0xfff,end=(a+n+0xfff)&~(UPTR)0xfff;unsigned char v;for(UPTR q=page;q<end;q+=0x1000)if(mincore((void*)q,0x1000,&v))return 0;return 1;}
static std::vector<std::string> installedGroups;
static I32 InstallHooks(const char*id,const S14_Hook*h,U32 n){for(U32 i=0;i<n;i++)if(memcmp(h[i].target,h[i].expected,h[i].length)){printf("    InstallHooks 거부 %s (%u번째)\n",id,i);return 0;}for(U32 i=0;i<n;i++)memcpy(h[i].target,h[i].replacement,h[i].length);installedGroups.push_back(id);return 1;}
static U8*MakeRelay(const void*target,const void*dest){UPTR near=(UPTR)target&~(UPTR)0xffff;U8*r=nullptr;for(UPTR off=0x10000;off<0x40000000&&!r;off+=0x10000){void*p=mmap((void*)(near+off),0x1000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);if(p!=MAP_FAILED)r=(U8*)p;}if(!r)return nullptr;r[0]=0xff;r[1]=0x25;memset(r+2,0,4);memcpy(r+6,&dest,8);return r;}
static void FreeRelay(void*){}
static void Log(const char*s){coreLogs.push_back(s);printf("    [로더 기록] %s\n",s);}
static std::string tmpRootStr;
static void buildWorld(){
 game=(U8*)mmap((void*)0x140000000,0x29ff000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);if(game==MAP_FAILED){perror("game");exit(2);}
 gmBlock=(U8*)calloc(1,0x100000);at<U8*>(game,0x277df18)=gmBlock;
 for(int i=0;i<8;i++)at<U8*>(gmBlock,0xe0d0+8*i)=districts[i];
 at<int>(districts[0],0x10)=2;at<int>(districts[1],0x10)=7;at<int>(districts[2],0x10)=9;at<int>(districts[3],0x10)=7;
 makeCity(5,2,0);makeCity(20,7,1);makeCity(21,7,3);makeCity(40,9,2);
 makePerson(1,L"유",L"비",0,5,1,0x30);makePerson(2,L"관",L"우",0,5,1,60);
 makePerson(10,L"조",L"조",1,20,1,0x30);makePerson(11,L"하후",L"돈",3,21,1,62);makePerson(12,L"순",L"욱",1,20,1,58);makePerson(13,L"장",L"료",3,21,1,70);
 // 게임 코드 자리: 0x2e0000 원본 5바이트 + 0x2e0005 부터 가짜 원본
 const U8 cap[]={0x44,0x89,0x4c,0x24,0x20};memcpy(game+0x2e0000,cap,5);thunk(game+0x2e0005,(void*)m_capture);
 // 0x2e6d27: sub rsp,0x28 ; nop×4 ; [0x2e6d2f] call 0x2e99e0 ; add rsp,0x28 ; ret   (해방 루틴의 호출 자리 흉내)
 U8*s=game+0x2e6d27;const U8 sub[]={0x48,0x83,0xec,0x28};memcpy(s,sub,4);memset(s+4,0x90,4);s[8]=0xe8;I32 rel=(I32)(0x2e99e0-(0x2e6d2f+5));memcpy(s+9,&rel,4);const U8 add[]={0x48,0x83,0xc4,0x28,0xc3};memcpy(s+13,add,5);
 thunk(game+0x2e99e0,(void*)m_travel);
 // 0x940357 (게임 파일 계층의 CreateFileW 호출 자리): R3는 건드리지 않아야 한다
 const U8 callind[]={0xff,0x15,0x93,0xff,0x9f,0x00};memcpy(game+0x940357,callind,6);
 // CPersonData vtable 직렬화 칸, 원본 직렬화 자리
 at<U8*>(game,0x13a6b60)=game+0x2385e0;thunk(game+0x2385e0,(void*)m_serialize);
 thunk(game+0x32b110,(void*)m_cityById);thunk(game+0x2401d0,(void*)m_setDistrict);
 at<void*>(game,0x13a6640+0xb0)=(void*)m_cityOwner;at<void*>(game,0x13a6640+0x78)=(void*)m_cityDistrict;
 resolved={};resolved.size=sizeof resolved;resolved.abi=S14_ABI;resolved.layout_abi=1;resolved.image_size=0x29ff000;resolved.image=game;
 api={};api.size=sizeof api;api.abi=S14_ABI;api.resolved=&resolved;api.Readable=Readable;api.InstallHooks=InstallHooks;api.MakeRelay=MakeRelay;api.FreeRelay=FreeRelay;api.Log=Log;
}
typedef int(*CaptureFn)(U8*,void*,int,int,void*,void*);typedef int(*ReleaseSite)(U8*,int,int);typedef UPTR(*SerializeFn)(U8*,U8*,UPTR,UPTR);
static int capture(U8*p){return ((CaptureFn)(game+0x2e0000))(p,nullptr,0,0x77,(void*)0xf0,nullptr);}
static int release(U8*p){return ((ReleaseSite)(game+0x2e6d27))(p,0,0);}
// 게임의 파일 스트림: +0 std::string 이름(MSVC: 15자 이하 내장, 그 이상 힙 포인터), +0x10 길이, +0x18 용량, +0x20 모드(1 읽기)
struct Stream{U8 raw[0x100];};
static Stream*makeStream(const char*name,int mode){Stream*st=new Stream();memset(st->raw,0,sizeof st->raw);size_t n=strlen(name);
 if(n<16){memcpy(st->raw,name,n+1);at<UPTR>(st->raw,0x18)=15;}else{char*h=(char*)malloc(n+1);memcpy(h,name,n+1);at<char*>(st->raw,0)=h;at<UPTR>(st->raw,0x18)=n;}
 at<UPTR>(st->raw,0x10)=n;at<int>(st->raw,0x20)=mode;return st;}
static UPTR serialize(U8*p,U8*stream){return ((SerializeFn)at<void*>(game,0x13a6b60))(p,stream,3,4);}
static std::string logText(){return readFile(tmpRootStr+"/game/SAN14Mods/PrisonerReturn/PrisonerReturn.log");}
int main(){
 char tmpl[]="/tmp/tprisoner_XXXXXX";tmpRootStr=mkdtemp(tmpl);tmpRoot=tmpRootStr;mkdirs(tmpRoot+"/game/SAN14Mods/PrisonerReturn");mkdirs(tmpRoot+"/game/saves");
 // R1 흔적: 저장마다 생긴 .bin 3개
 for(int i=0;i<3;i++)writeFile(tmpRoot+"/game/SAN14Mods/PrisonerReturn/"+std::string(64,'a'+i)+".bin",std::string(38,'x'));
 buildWorld();

 printf("1) 초기화·훅 설치·레거시 정리\n");
 check(SAN14_ModInitV1(&api)==1,"ModInit 성공");
 check(installedGroups.size()==2&&installedGroups[0]=="SAN14CE.PrisonerReturn"&&installedGroups[1]=="SAN14CE.PrisonerReturn.SaveSlot","훅 묶음 2개 설치(포획·해방 / 인물 직렬화)");
 check(game[0x2e0000]==0xe9&&game[0x2e6d2f]==0xe8&&at<U8*>(game,0x13a6b60)!=game+0x2385e0,"5바이트 jmp / 호출 자리 / vtable 칸이 바뀜");
 check(game[0x940357]==0xff&&game[0x940358]==0x15,"파일 열기 자리(0x940357)는 원본 그대로 — R3는 걸지 않음");
 check(PrisonerReturnVersion()==4,"판 번호 4");
 {DIR*d=opendir((tmpRoot+"/game/SAN14Mods/PrisonerReturn").c_str());int bins=0;while(dirent*e=readdir(d))if(strstr(e->d_name,".bin"))bins++;closedir(d);check(bins==0,"R1 .bin 사이드카 정리됨");}
 check(has(logText(),"removed 3 legacy per-save .bin sidecars")&&has(logText(),"PrisonerReturn R4 loaded")&&has(logText(),"from the save stream name"),"로그: 정리·시작 줄");
 check(coreLogs.size()==1,"ModCore 로그는 시작 줄 하나만");
 printf("2) 포획: 관직 유지·기록\n");
 unsigned opensBefore=createfile_calls.load();
 int cr=capture(P(13));
 check(createfile_calls.load()==opensBefore,"R4: 포획 때 파일 열기 0회 (기록 파일은 열어 둔 핸들에 덧붙임)");
 check(captureCalls==1&&capturedR9==0x77&&capturedForce==(void*)0xf0&&cr==0x5a,"원본 포로 처리로 인자(r9·5번째)가 그대로 넘어가고 반환값도 그대로");
 check(P(13)[0xe6]==6&&at<U16>(P(13),0xe2)==5&&P(13)[0xe7]==70,"장료: 포로 상태·보관 도시 5·관직 70 유지");
 int home=0,rank=0,force=0;check(PrisonerReturnRecord(13,&home,&rank,&force)==1&&home==21&&rank==70&&force==7,"기록: 원래 도시 21, 관직 70, 세력 7");
 check(has(logText(),"captured; rank retained: id=13 home=21 rank=70 force=7 장료"),"포획 로그");
 printf("3) 저장(스트림 쓰기 모드): 세이브 이름의 사이드카 1개\n");
 Stream*sv=makeStream("svdexceKO03.s14",0);
 UPTR sr=serialize(P(1),sv->raw);
 check(serializeCalls==1&&sr==0x7007,"원본 직렬화 반환값·3·4번째 인자 그대로");
 std::string sc=readFile(tmpRoot+"/game/SAN14Mods/PrisonerReturn/svdexceKO03.s14.prison");
 check(sc.size()==8+36&&sc.substr(0,4)=="SPR2"&&(unsigned char)sc[4]==1,"사이드카 svdexceKO03.s14.prison: 기록 1개 (이름 15자 = 스트림 안에 내장)");
 for(int i=0;i<50;i++)serialize(P(2),sv->raw);check(PrisonerReturnSaves()==1,"같은 저장 안의 다음 무장들은 다시 쓰지 않음");
 check(has(logText(),"sidecar saved: svdexceKO03.s14 prisoners=1"),"저장 로그");
 Stream*other=makeStream("0010_KO/fixdataexce.s14",0);serialize(P(2),other->raw);check(PrisonerReturnSaves()==1,"세이브 슬롯이 아닌 파일 쓰기는 무시");
 Stream*cc=makeStream("svdexceccKO07.s14",0);serialize(P(2),cc->raw);
 sc=readFile(tmpRoot+"/game/SAN14Mods/PrisonerReturn/svdexceccKO07.s14.prison");check(PrisonerReturnSaves()==2&&sc.size()==8+36,"이름 17자(힙 포인터) 세이브도 인식");
 printf("4) 다른 포로 추가 뒤 불러오기 → 사이드카 내용으로 표 교체\n");
 capture(P(11));check(PrisonerReturnRecord(11,&home,&rank,&force)==1&&home==21&&rank==62,"하후돈 기록 추가");
 Stream*ld=makeStream("svdexceKO03.s14",1);serialize(P(1),ld->raw);
 check(PrisonerReturnLoads()==1,"불러오기 첫 무장 직렬화에서 사이드카 적용");
 check(PrisonerReturnRecord(13,&home,&rank,&force)==1&&home==21&&PrisonerReturnRecord(11,nullptr,nullptr,nullptr)==0,"표 = 저장 시점 기록(장료만, 하후돈 없음)");
 for(int i=0;i<50;i++)serialize(P(2),ld->raw);check(PrisonerReturnLoads()==1,"같은 불러오기 안의 다음 무장들은 다시 적용하지 않음");
 check(has(logText(),"sidecar loaded: svdexceKO03.s14 prisoners=1"),"불러오기 로그");
 printf("5) 해방: 원래 도시로 경로 변경, 관직 유지\n");
 P(13)[0xe6]=4;at<U16>(P(13),0xe2)=20;P(13)[0xe0]=1;   // 원본 해방 루틴이 한 일: 이동 상태, 군주 도시(20), 군주 군단(1)
 int r=release(P(13));
 check(r==1&&travelCalls==1&&lastTravel==P(13),"원본 이동 함수(0x2e99e0)가 같은 무장으로 호출됨");
 check(at<U16>(P(13),0xe2)==21&&setDistrictCalls==1&&lastDistrict==3,"목적지 = 원래 도시 21, 군단도 그 도시 군단(3)으로");
 check(P(13)[0xe7]==70,"관직 70 유지(같은 세력에 70 가진 다른 무장 없음)");
 check(PrisonerReturnRecord(13,nullptr,nullptr,nullptr)==0,"기록은 소비됨");
 check(has(logText(),"release route redirected: id=13 home=21 장료"),"해방 로그");
 printf("6) 관직 중복: 같은 세력 다른 무장이 그 관직을 쓰면 관직 없음\n");
 makePerson(13,L"장",L"료",3,21,1,70);capture(P(13));P(12)[0xe7]=70;P(13)[0xe6]=4;at<U16>(P(13),0xe2)=20;release(P(13));
 check(P(13)[0xe7]==0x49&&has(logText(),"rank occupied; duplicate prevented: id=13 rank=70"),"관직 중복 → 0x49");
 P(12)[0xe7]=58;
 printf("7) 원래 도시를 잃음 → 순정 목적지 유지\n");
 makePerson(13,L"장",L"료",3,21,1,70);capture(P(13));cities[21].owner=2;P(13)[0xe6]=4;at<U16>(P(13),0xe2)=20;setDistrictCalls=0;release(P(13));
 check(at<U16>(P(13),0xe2)==20&&setDistrictCalls==0&&has(logText(),"home lost; native destination kept: id=13 home=21 owner=2"),"소유 세력이 다르면 목적지 그대로");
 check(P(13)[0xe7]==70,"관직은 그대로 복원");cities[21].owner=7;
 printf("8) 기록 없는 무장·세력 바뀐 무장은 원본 그대로\n");
 makePerson(11,L"하후",L"돈",3,21,4,62);at<U16>(P(11),0xe2)=20;travelCalls=0;release(P(11));check(travelCalls==1&&at<U16>(P(11),0xe2)==20,"기록 없음 → 그냥 원본 호출");
 makePerson(13,L"장",L"료",3,21,1,70);capture(P(13));P(13)[0xe0]=0;P(13)[0xe6]=4;at<U16>(P(13),0xe2)=5;travelCalls=0;release(P(13));check(travelCalls==1&&at<U16>(P(13),0xe2)==5,"세력이 바뀐 무장(등용됨) → 기록 무시");
 printf("9) 포로 0명 저장·사이드카 없는 세이브 불러오기\n");
 Stream*au=makeStream("autosdexceKO00.s14",0);serialize(P(1),au->raw);sc=readFile(tmpRoot+"/game/SAN14Mods/PrisonerReturn/autosdexceKO00.s14.prison");check(sc.size()==8&&(unsigned char)sc[4]==0,"자동 저장, 포로 없을 때 기록 0개 파일");
 makePerson(13,L"장",L"료",3,21,1,70);capture(P(13));check(PrisonerReturnRecord(13,nullptr,nullptr,nullptr)==1,"기록 있음");
 Stream*l9=makeStream("svdexceKO09.s14",1);serialize(P(1),l9->raw);
 check(PrisonerReturnRecord(13,nullptr,nullptr,nullptr)==0&&has(logText(),"sidecar loaded: svdexceKO09.s14 (없음) prisoners=0"),"사이드카 없는 세이브 → 표 비움");
 printf("10) 세이브 슬롯이 아닌 읽기(새 게임 시나리오 등) → 표 비움\n");
 capture(P(13));Stream*sce=makeStream("0010_KO/scedaexce03.s14",1);serialize(P(1),sce->raw);
 check(PrisonerReturnRecord(13,nullptr,nullptr,nullptr)==0&&has(logText(),"load without save-slot name"),"시나리오 읽기 → 비움");
 printf("11) 저장 때 묵은 기록은 걸러짐\n");
 makePerson(13,L"장",L"료",3,21,1,70);capture(P(13));P(13)[0xe6]=1;   // 포로가 아니게 됨(예: 등용)
 Stream*s5=makeStream("svdexceKO05.s14",0);serialize(P(1),s5->raw);sc=readFile(tmpRoot+"/game/SAN14Mods/PrisonerReturn/svdexceKO05.s14.prison");check(sc.size()>=8&&(unsigned char)sc[4]==0,"상태 6 아닌 기록은 쓰지 않음");
 printf("12) 같은 주소의 스트림이 곧바로 다른 모드로 쓰이면 새 작업으로 본다\n");
 makePerson(13,L"장",L"료",3,21,1,70);capture(P(13));
 Stream*re=makeStream("svdexceKO11.s14",0);serialize(P(1),re->raw);int saves=PrisonerReturnSaves(),loads=PrisonerReturnLoads();
 at<int>(re->raw,0x20)=1;serialize(P(1),re->raw);check(PrisonerReturnSaves()==saves&&PrisonerReturnLoads()==loads+1&&PrisonerReturnRecord(13,nullptr,nullptr,nullptr)==1,"저장 직후 같은 객체로 불러오기 → 적용됨, 기록 유지");
 printf("13) 이름 길이·문자 검사\n");
 Stream*bad=makeStream("svdexce\x01KO.s14",0);int before=PrisonerReturnSaves();serialize(P(1),bad->raw);check(PrisonerReturnSaves()==before,"이상한 문자 이름은 무시");
 printf("14) R4: 포획·해방 200회에 파일 열기 0회, 디스크 동기화 0회, 기록은 모두 남음\n");
 {unsigned opens=createfile_calls.load(),flushes=flush_calls.load();size_t lines=0;
  for(int i=0;i<200;i++){makePerson(13,L"장",L"료",3,21,1,70);capture(P(13));P(13)[0xe6]=4;at<U16>(P(13),0xe2)=20;release(P(13));}
  std::string t=logText();for(char c:t)if(c=='\n')lines++;
  check(createfile_calls.load()==opens,"포획·해방 400번 동안 CreateFileW 0회");
  check(flush_calls.load()==flushes,"FlushFileBuffers 0회");
  check(t.find("captured; rank retained: id=13")!=std::string::npos&&lines>=400,"기록 줄은 모두 파일에 남음");}
 printf("15) R4: 1MB 넘으면 .prev.log 로 돌리고 새 파일로 이어 씀\n");
 {std::string big=tmpRoot+"/game/SAN14Mods/PrisonerReturn/PrisonerReturn.log";
  unsigned opens=createfile_calls.load();
  for(int i=0;i<18000;i++){makePerson(13,L"장",L"료",3,21,1,70);capture(P(13));}   // 약 1.15MB 분량의 기록 줄
  struct stat st{};bool prev=stat((tmpRoot+"/game/SAN14Mods/PrisonerReturn/PrisonerReturn.prev.log").c_str(),&st)==0&&st.st_size>1000000;
  struct stat cur{};stat(big.c_str(),&cur);
  check(prev,"PrisonerReturn.prev.log 생김(1MB 이상)");
  check(cur.st_size>0&&cur.st_size<1000000,"새 PrisonerReturn.log 는 작게 다시 시작");
  check(createfile_calls.load()==opens+1,"교체 때만 파일을 한 번 다시 열었음");
  check(has(logText(),"captured; rank retained"),"새 파일에도 기록이 이어짐");}
 printf("기록(마지막 부분):\n%s\n",logText().substr(logText().size()>2000?logText().size()-2000:0).c_str());
 printf("결과: 통과 %d / 실패 %d\n",passes,failures);return failures?1:0;
}
