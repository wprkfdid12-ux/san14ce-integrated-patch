#pragma once
// R43 시험8: 자기 기록 파일에만 실패를 적는 모듈의 기록을 종합 진단(Diag)이 읽는다.
//  왜: 통합편의(IC)는 "모드 설정 v2 설치 실패" 같은 실패를 SAN14CE_11Patch.log(UTF-16)에만 적고, 창을 안 띄운다.
//      개성 9칸 UI·개성 설명·연출·미니맵·편의성 묶음도 마찬가지(각자 .log). Diag 는 runtime.log·적용확인.log 만 봐서 조용히 지나갔다.
//  어떻게: 게임 시작 30초/90초 확인 때 한 번씩만 읽는다(계속 읽지 않음 → 렉 없음). 이번 실행 부분만 본다.
//   · 통합편의: 마지막 "지원 실행 파일 확인 완료" 줄(시각 ≥ 이번 실행 기준) 뒤 줄 중 실패 문구가 있는 줄.
//   · 새로 쓰는 기록(DragGive 등): 이번 실행에 써진 파일(수정 시각 ≥ 프로세스 시작)만, 실패 문구 줄.
//   · 이어 쓰는 기록(TraitHover): Diag 시작 때 크기 뒤로 붙은 줄(같은 이유는 한 건으로 묶어 개수 표시).
//  이 파일은 Windows 없이 시험할 수 있게 버퍼만 다룬다(build_tc/host/test_diag_ownlogs.cpp).
#include <stddef.h>
#include <string.h>
namespace ownlog{
typedef unsigned short W16;
struct Hit{int feature;const wchar_t*name;char key[160];char text[240];unsigned count;};
inline size_t wl(const wchar_t*s){size_t n=0;while(s[n])n++;return n;}
inline bool whas(const W16*a,size_t n,const wchar_t*k){size_t m=wl(k);if(!m||n<m)return false;for(size_t i=0;i+m<=n;i++){size_t j=0;while(j<m&&a[i+j]==(W16)k[j])j++;if(j==m)return true;}return false;}
inline size_t sl(const char*s){size_t n=0;while(s[n])n++;return n;}
inline int sc(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return (unsigned char)*a-(unsigned char)*b;}
inline bool chas(const char*a,size_t n,const char*k){size_t m=sl(k);if(!m||n<m)return false;for(size_t i=0;i+m<=n;i++)if(a[i]==k[0]&&!memcmp(a+i,k,m))return true;return false;}
// UTF-16 → UTF-8 (한 줄, 잘라서)
inline void u8(const W16*s,size_t n,char*o,size_t cap){size_t k=0;
 for(size_t i=0;i<n&&k+4<cap;i++){unsigned c=s[i];
  if(c>=0xd800&&c<0xdc00&&i+1<n){unsigned d=s[i+1];if(d>=0xdc00&&d<0xe000){c=0x10000+((c-0xd800)<<10)+(d-0xdc00);i++;}}
  if(c<0x80)o[k++]=(char)c;else if(c<0x800){o[k++]=(char)(0xc0|(c>>6));o[k++]=(char)(0x80|(c&63));}
  else if(c<0x10000){o[k++]=(char)(0xe0|(c>>12));o[k++]=(char)(0x80|((c>>6)&63));o[k++]=(char)(0x80|(c&63));}
  else{o[k++]=(char)(0xf0|(c>>18));o[k++]=(char)(0x80|((c>>12)&63));o[k++]=(char)(0x80|((c>>6)&63));o[k++]=(char)(0x80|(c&63));}}
 o[k]=0;}
inline void cpy(const char*s,size_t n,char*o,size_t cap){size_t k=0;while(k<n&&k+1<cap&&s[k]!='\r'&&s[k]!='\n'){o[k]=s[k];k++;}
 // 잘린 UTF-8 글자 끝 정리
 while(k>0&&((unsigned char)o[k-1]&0xc0)==0x80){size_t b=k;while(b>0&&((unsigned char)o[b-1]&0xc0)==0x80)b--;if(b==0)break;unsigned char lead=(unsigned char)o[b-1];size_t need=lead>=0xf0?4:lead>=0xe0?3:lead>=0xc0?2:1;if(k-(b-1)>=need)break;k=b-1;}
 o[k]=0;}
inline void scpy(char*o,size_t cap,const char*s){size_t k=0;while(s[k]&&k+1<cap){o[k]=s[k];k++;}o[k]=0;}
// 같은 기능·같은 key 면 개수만 늘림
inline unsigned push(Hit*out,unsigned cap,unsigned n,int feature,const wchar_t*name,const char*key,const char*text){
 char kk[160];scpy(kk,sizeof kk,key);
 for(unsigned i=0;i<n;i++)if(out[i].feature==feature&&!sc(out[i].key,kk)){out[i].count++;return n;}
 if(n>=cap)return n;
 Hit&h=out[n];h.feature=feature;h.name=name;h.count=1;scpy(h.key,sizeof h.key,kk);scpy(h.text,sizeof h.text,text);return n+1;}

// ---- 통합편의(UTF-16, 줄 앞 "YYYY-MM-DD HH:MM:SS ")
struct IcMap{const wchar_t*kw;int feature;const wchar_t*name;};
static const IcMap kIcMap[]={
 {L"모드 설정",1,L"모드 설정 창"},{L"정보 메뉴",1,L"모드 설정 창(정보 메뉴 버튼)"},
 {L"자동인사",2,L"턴 종료 자동 포상·투자·논공·자동인사"},{L"자동 투자",2,L"턴 종료 자동 포상·투자·논공·자동인사"},{L"논공",2,L"턴 종료 자동 포상·투자·논공·자동인사"},
 {L"문전박대",3,L"사신 자동 거절(문전박대)"},
 {L"도시 증축",4,L"연출 생략"},{L"명승",4,L"연출 생략"},{L"행동보고",4,L"연출 생략"},{L"자동 포커스",4,L"연출 생략"},{L"카메라",4,L"연출 생략"},
 // R43 11-08 바로잡음: 설치기 번호 그대로 — 5 수행·대열, 6 진행 배속, 7 인재·포로, 8 BGM (옛 표는 하나씩 밀려 '5번 진행 배속'으로 떴음)
 {L"진행속도",6,L"진행 배속"},{L"수행",5,L"여러 부대 이동·수행"},{L"대열 이동",5,L"여러 부대 이동(대열 이동)"},
 {L"인재",7,L"인재·포로 편의"},{L"포로",7,L"인재·포로 편의"},{L"등용문",7,L"인재·포로 편의"},
 {L"BGM",8,L"BGM 추가·플레이어"},
 {L"개성 9개",9,L"개성 9개(통합편의 연결)"},
 {L"일기토",10,L"고유 무장 등록(일기토 연출)"},{L"고유무장",10,L"고유 무장 등록"},
};
static const wchar_t*const kIcBad[]={L"실패",L"미적용",L"못했",L"못 찾"};
static const wchar_t*const kIcIgnore[]={L"무장 원본 직접 읽기"};   // 매 실행 '오류 1' (원본 자료 쪽 알려진 값) — 기능 실패 아님
static const wchar_t kIcMarker[]=L"지원 실행 파일 확인 완료";
// since: "YYYY-MM-DD HH:MM:SS" 이번 실행 기준. traits9_loaded: 개성 9칸 UI 가 안 깔렸으면 'IC↔9 연결 실패'는 정상이라 뺀다.
// feat10: 10번(고유 무장 등록·추가 모델) 설치 여부. 안 깔렸으면 통합편의의 일기토 연출 시험 연결이 '호환성 검사 실패'로 그냥 넘어가는 게 정상이라 뺀다(시험13, 사용자 15:54).
// off/noff: 사용자가 모드 설정 창에서 끈 기능의 낱말(그 낱말이 든 실패 줄은 꺼서 안 된 것이라 문제 아님, 시험14 사용자 지시)
inline unsigned scan_ic(const W16*b,size_t n,const char*since,bool traits9_loaded,Hit*out,unsigned cap,unsigned nhit,bool feat10=true,const wchar_t*const*off=nullptr,unsigned noff=0){
 // 줄 나누기 + 마지막 '시작 줄' 찾기
 size_t start=(size_t)-1;
 for(size_t s=0;s<n;){size_t e=s;while(e<n&&b[e]!='\n')e++;
  if(e-s>20&&whas(b+s,e-s,kIcMarker)){bool ok=true;for(int i=0;i<19;i++)if(b[s+i]!=(W16)(unsigned char)since[i]){ok=b[s+i]>(W16)(unsigned char)since[i];break;}if(ok)start=s;}
  s=e+1;}
 if(start==(size_t)-1)return nhit;
 for(size_t s=start;s<n;){size_t e=s;while(e<n&&b[e]!='\n')e++;size_t len=e-s;if(len&&b[s+len-1]=='\r')len--;
  const W16*l=b+s;bool bad=false;for(const wchar_t*k:kIcBad)if(whas(l,len,k)){bad=true;break;}
  for(const wchar_t*k:kIcIgnore)if(whas(l,len,k))bad=false;
  if(bad&&!traits9_loaded&&whas(l,len,L"개성 9개"))bad=false;
  if(bad&&!feat10&&(whas(l,len,L"일기토")||whas(l,len,L"고유무장")))bad=false;
  for(unsigned i=0;bad&&i<noff;i++)if(off[i]&&whas(l,len,off[i]))bad=false;
  if(bad){int f=1;const wchar_t*nm=L"통합편의";for(const IcMap&m:kIcMap)if(whas(l,len,m.kw)){f=m.feature;nm=m.name;break;}
   const W16*body=l;size_t bl=len;if(bl>20&&l[4]=='-'&&l[13]==':'){body=l+20;bl-=20;}
   char t[240];u8(body,bl,t,sizeof t);nhit=push(out,cap,nhit,f,nm,t,t);}
  s=e+1;}
 return nhit;
}
// ---- 한 줄씩 쓰는 기록(ASCII/UTF-8)
static const char*const kAsciiBad[]={"SKIPPED","FAILED","failed","WARNING","differs","differ ","not found","unsupported","not installed","fix off","feature off","off (","conflict","mismatch","missing","stopped","already patched","not built","not started","outside","does not match","out of image"};
// group_site: "site=0x... owner=... reason=X" 를 reason 으로 묶음(TraitHover). ignore: 정상 진행 중 생기는 줄(예: duel skipped)
inline unsigned scan_ascii(const char*b,size_t n,int feature,const wchar_t*name,bool group_site,const char*ignore,Hit*out,unsigned cap,unsigned nhit){
 for(size_t s=0;s<n;){size_t e=s;while(e<n&&b[e]!='\n')e++;size_t len=e-s;const char*l=b+s;
  bool bad=false;
  if(group_site&&len>5&&!memcmp(l,"site=",5))bad=true;
  else for(const char*k:kAsciiBad)if(chas(l,len,k)){bad=true;break;}
  if(bad&&ignore&&chas(l,len,ignore))bad=false;
  if(bad){char t[240];
   if(group_site&&chas(l,len,"reason=")){const char*r=l;while(r<l+len-7&&memcmp(r,"reason=",7))r++;r+=7;char key[160];cpy(r,(size_t)(l+len-r),key,sizeof key);
    char site[40];size_t k=0;const char*p=l+5;while(p<l+len&&*p!=' '&&k+1<sizeof site)site[k++]=*p++;site[k]=0;
    t[0]=0;scpy(t,sizeof t,key);size_t m=sl(t);for(const char*q=" (first site ";*q&&m+1<sizeof t;q++)t[m++]=*q;for(const char*q=site;*q&&m+1<sizeof t;q++)t[m++]=*q;if(m+1<sizeof t)t[m++]=')';t[m]=0;
    nhit=push(out,cap,nhit,feature,name,key,t);}
   else{cpy(l,len,t,sizeof t);nhit=push(out,cap,nhit,feature,name,t,t);}
  }
  s=e+1;}
 return nhit;
}
}
