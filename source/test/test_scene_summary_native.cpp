// 4번 턴 끝 결과 창(17차 P219): "외 N건" 으로 줄이지 않고 전부, 넘치면 창을 나눠 차례로. src/scene_summary.h 를 그대로 시험.
// 22차(P219): 넓은 창(사건 결과 창) = 제목(sumtitle) + 본문(sumtext, 기본 한 줄 40자·6줄) / 작은 창 대신(sumsmall) = 제목 + 3줄(27자), 넘치면 "그 외 N건"
#include <stdio.h>
#include <string>
#include <vector>
static int on=1;static bool ss_summary_on(){return on!=0;}
#include "../src/scene_summary.h"
static int fails;
#define CHECK(c,m) do{if(c)printf("ok   %s\n",m);else{printf("FAIL %s\n",m);fails++;}}while(0)
static std::vector<std::wstring> split(const std::wstring&s){std::vector<std::wstring>v;size_t a=0;for(;;){size_t b=s.find(L'\n',a);v.push_back(s.substr(a,b==std::wstring::npos?b:b-a));if(b==std::wstring::npos)break;a=b+1;}return v;}
static std::vector<std::wstring> smalls;
static std::vector<std::wstring> run_all(long*first_total){std::vector<std::wstring>p;long t=0;*first_total=0;smalls.clear();while(ss_summary_step(&t)){if(p.empty())*first_total=t;if(t)smalls.push_back(sumsmall);p.push_back(std::wstring(sumtitle)+L"\n"+sumtext);if(p.size()>40)break;}return p;}
static bool small_shape(const std::wstring&x){auto L=split(x);if(L.size()>4||L[0]!=L"이번 턴 결과")return false;for(auto&l:L)if(l.size()>34)return false;return true;}
int main(){
 long t;
 { auto p=run_all(&t);CHECK(p.empty(),"모은 게 없으면 창 없음"); }
 const wchar_t*a[]={L"장안에서 메뚜기떼 피해가 발생 중입니다.",L"낙양에 역병 발생",L"업에 역병 발생"};
 for(auto x:a)SsCollect(0,x,0);
 SsCollect(1,L"조운이 마초 등용에 성공했습니다",1);SsCollect(1,L"조운이 황충 등용에 실패했습니다",0);SsCollect(1,L"관우가 장비 등용에 성공",1);
 SsCollect(2,L"제갈량이 양양에서 맹덕신서를 발견",0);SsCollect(4,L"성도 증축 완료",0);
 { auto p=run_all(&t);CHECK(p.size()>=1&&t==8,"8건 묶음");std::wstring all;for(auto&x:p)all+=x+L"\n";
   bool every=true;for(auto x:a)if(all.find(x)==std::wstring::npos)every=false;
   for(auto x:{L"조운이 마초 등용에 성공했습니다",L"조운이 황충 등용에 실패했습니다",L"관우가 장비 등용에 성공",L"제갈량이 양양에서 맹덕신서를 발견",L"성도 증축 완료"})if(all.find(x)==std::wstring::npos)every=false;
   CHECK(every,"모은 글자가 하나도 안 빠짐");CHECK(all.find(L" 외 ")==std::wstring::npos,"\"외 N건\" 없음");
   CHECK(all.find(L"【재해 3건】")!=std::wstring::npos&&all.find(L"【등용 결과 3건(성공 2·실패 1)】")!=std::wstring::npos&&all.find(L"【보물 발견 1건】")!=std::wstring::npos&&all.find(L"【증축 1건】")!=std::wstring::npos,"종류별 머리말");
   bool shape=true;for(auto&x:p){auto L=split(x);if(L.size()>10||L[0].rfind(L"이번 턴 결과",0)!=0)shape=false;for(size_t i=1;i<L.size();i++)if(L[i].size()>40)shape=false;}
   CHECK(shape,"넓은 창마다 제목 + 9줄 이하, 한 줄 36자 안팎(외 N건 꼬리만 조금 넘침)");
   CHECK(smalls.size()==1&&small_shape(smalls[0]),"작은 창 대신 글자: 제목 + 3줄 이하, 한 줄 짧게");
   CHECK(split(smalls[0]).size()==4&&split(smalls[0])[3]==L"그 외 2건","작은 창: 종류 4개(재해·등용·보물·증축) → 2줄 + 그 외 2건(보물 1·증축 1)");
   auto q=run_all(&t);CHECK(q.empty(),"다 보여 준 뒤에는 원래 단계"); }
 SsCollect(3,L"적벽의 발전Lv이 상승",0);SsCollect(4,L"성도 증축 완료",0);
 { auto p=run_all(&t);CHECK(p.empty()&&pending_total()==0,"명승·증축 소식만이면 창 없이 비움"); }
 for(int i=0;i<30;i++){wchar_t b[64];swprintf(b,64,L"무장%02d가 인재%02d 등용에 성공했습니다",i,i);SsCollect(1,b,1);}
 { auto p=run_all(&t);std::wstring all;for(auto&x:p)all+=x+L"\n";bool every=true;
   for(int i=0;i<30;i++){wchar_t b[64];swprintf(b,64,L"무장%02d가 인재%02d 등용에 성공했습니다",i,i);if(all.find(b)==std::wstring::npos)every=false;}
   wchar_t h1[40],hn[40];swprintf(h1,40,L"이번 턴 결과 (1/%d)",(int)p.size());swprintf(hn,40,L"이번 턴 결과 (%d/%d)",(int)p.size(),(int)p.size());
   CHECK(p.size()>=3&&every&&all.find(L" 외 ")==std::wstring::npos,"30건: 창을 나눠 전부 표시");
   CHECK(p[0].rfind(h1,0)==0&&p.back().rfind(hn,0)==0,"창 제목에 (몇 번째/전체)");
   bool five=true;for(auto&x:p)if(split(x).size()>10)five=false;CHECK(five,"나눈 창도 9줄 이하");
   CHECK(smalls.size()==1&&small_shape(smalls[0])&&split(smalls[0]).size()==2&&smalls[0].find(L"【등용 결과 30건(")==std::wstring::npos&&smalls[0].find(L"【등용 결과 30건】")!=std::wstring::npos,"30건 작은 창: 제목 + 한 줄(건수 머리말)"); }
 for(int i=0;i<70;i++){wchar_t b[64];swprintf(b,64,L"도시%02d 역병",i);SsCollect(0,b,0);}
 { auto p=run_all(&t);std::wstring all;for(auto&x:p)all+=x+L"\n";CHECK(t==70&&all.find(L"도시63 역병")!=std::wstring::npos&&all.find(L"도시64 역병")==std::wstring::npos&&all.find(L" 외 6건")!=std::wstring::npos,"64개를 넘기면 그때만 외 N건(70건 → 외 6건)"); }
 { wchar_t lng[200];for(int i=0;i<199;i++)lng[i]=L'가';lng[199]=0;SsCollect(0,lng,0);auto p=run_all(&t);bool okw=true;for(auto&x:p)for(auto&l:split(x))if(l.size()>60)okw=false;CHECK(p.size()==1&&okw,"아주 긴 줄은 … 로 잘라 창 폭을 안 넘김"); }
 // 작은 창: 종류가 4개 이상이면 앞 2줄 + 그 외 N건(N = 못 보인 종류 건수 합)
 SsCollect(0,L"장안 역병",0);SsCollect(1,L"a 등용 성공",1);SsCollect(2,L"b 보물 발견",0);SsCollect(3,L"c 명승",0);SsCollect(3,L"d 명승",0);SsCollect(4,L"e 증축",0);
 { auto p=run_all(&t);auto L=split(smalls[0]);CHECK(L.size()==4&&L[1]==L"【재해 1건】 장안 역병"&&L[2]==L"【등용 결과 1건】 a 등용 성공"&&L[3]==L"그 외 4건","작은 창: 종류 5개 → 2줄 + 그 외 4건"); }
 SsCollect(0,L"진양에서 역병이 맹위를 떨치고 있습니다. 매우 긴 문장입니다",0);SsCollect(0,L"업 역병",0);
 { auto p=run_all(&t);auto L=split(smalls[0]);CHECK(L.size()==2&&L[1].size()<=27&&L[1].back()==L'\x2026'&&L[1].rfind(L"【재해 2건】 진양",0)==0,"작은 창: 긴 첫 항목은 27자에서 …"); }
 { SsCollect(0,L"장안 역병",0);auto p=run_all(&t);CHECK(smalls.size()==1&&smalls[0]==L"이번 턴 결과\n【재해 1건】 장안 역병","1건이면 작은 창도 그대로(그 외 없음)"); }
 { ss_summary_shape(30,4);for(int i=0;i<20;i++){wchar_t b[64];swprintf(b,64,L"무장%02d 등용 성공",i);SsCollect(1,b,1);}auto p=run_all(&t);bool ok=true;for(auto&x:p){auto L=split(x);if(L.size()>5)ok=false;for(size_t i=1;i<L.size();i++)if(L[i].size()>34)ok=false;}
   CHECK(p.size()>=2&&ok,"설정 SummaryWidth=30·SummaryLines=4 대로 나눔");ss_summary_shape(5,99);CHECK(kLineW==20&&kPageLines==10,"설정 값은 20~60·3~10 으로 묶음");ss_summary_shape(36,9); }
 // 23차: 풍작은 재해와 따로 【풍작】, 재해 바로 뒤에 / 풍작만 있어도 창
 SsCollect(0,L"강주에서 메뚜기떼가 맹위를 떨치고 있습니다.",0);SsCollect(5,L"올해는 낙양, 천수에서 풍작을 이루었습니다!",0);SsCollect(1,L"a 등용 성공",1);
 { auto p=run_all(&t);auto L=split(p[0]);CHECK(L.size()==4&&L[1].rfind(L"【재해 1건】 강주",0)==0&&L[2]==L"【풍작 1곳】 올해는 낙양, 천수에서 풍작을 이루었습니다!"&&L[3].rfind(L"【등용 결과 1건",0)==0,"풍작은 【풍작】 따로, 재해 다음 차례"); }
 SsCollect(5,L"올해는 성도에서 풍작",0);{ auto p=run_all(&t);CHECK(p.size()==1&&p[0].find(L"【풍작 1곳】")!=std::wstring::npos&&p[0].find(L"재해")==std::wstring::npos,"풍작만 있어도 창(전처럼)"); }
 // 24차: 게임 문장의 색 표시 ^05…^00 은 글자 수에서 뺌 — 화면 그대로 "올해는 회계, 강주에서 풍작을 이루었습니다!"(24자)가 머리말 줄에 다 들어감
 SsCollect(5,L"올해는 ^05회계^00, ^05강주^00에서 풍작을 이루었습니다!",0);
 { auto p=run_all(&t);auto L=split(p[0]);CHECK(L.size()==2&&L[1]==L"【풍작 2곳】 올해는 ^05회계^00, ^05강주^00에서 풍작을 이루었습니다!","색 표시는 셈에서 빼고 안 자름(화면 14:39 회귀)");
   auto S=split(smalls[0]);CHECK(S.size()==2&&vlen(S[1].c_str())<=27&&S[1].find(L"^05회계^00")!=std::wstring::npos&&S[1].back()==L'\x2026',"작은 창: 보이는 27자까지, 색 표시는 안 깨짐"); }
 { wchar_t o[64];vcut(o,64,L"가나^05다라마^00바",4);CHECK(std::wstring(o)==L"가나^05다^00\x2026","색 안에서 자르면 ^00 으로 닫고 …"); }
 { SsCollect(0,L"^05아주아주아주아주긴도시이름이에요정말로긴이름입니다^00에서 메뚜기떼가 맹위를 떨치고 있습니다",0);auto p=run_all(&t);bool ok=true;for(auto&l:split(p[0]))if(vlen(l.c_str())>36){ok=false;printf("  len %d\n",vlen(l.c_str()));}CHECK(ok,"색 표시가 든 긴 항목도 보이는 36자 안"); }
 // 26차(화면 15:20 "왜 풍작 1건이야"): 한 문장에 도시 4곳 → 【풍작 4곳】
 SsCollect(5,L"올해는 ^05남피^00, ^05강하^00, ^05신야^00, ^05강릉^00에서 풍작을 이루었습니다!",0);
 { auto p=run_all(&t);auto L=split(p[0]);CHECK(L[1].rfind(L"【풍작 4곳】",0)==0&&p[0].find(L"^05강릉^00에서 풍작을 이루었습니다!")!=std::wstring::npos&&p[0].find(L"\x2026")==std::wstring::npos,"풍작은 도시 수로: 4곳, 문장은 줄이지 않고 다 보임");
   CHECK(split(smalls[0])[1].rfind(L"【풍작 4곳】",0)==0,"작은 창도 4곳"); }
 on=0;SsCollect(0,L"x",0);{auto p=run_all(&t);CHECK(p.empty()&&pending_total()==0,"설정 끔: 안 모으고 창 없음");}on=1;
 SsCollect(0,L"a",0);SsCollect(0,L"b",0);{long s=0;int r=ss_summary_step(&s);on=0;long s2=0;int r2=ss_summary_step(&s2);on=1;CHECK(r==1&&r2==0&&page_cnt==0,"창을 띄우는 중에 끄면 남은 창 없음");}
 printf(fails?"FAILED %d\n":"scene_summary: all ok\n",fails);return fails?1:0;
}
