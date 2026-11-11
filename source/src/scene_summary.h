// 4번 턴 끝 결과 창 — 모으기·줄 만들기·창 나누기(게임을 안 부르는 순수 계산). scene_skip.cpp 가 포함하고, 시험(test/test_scene_summary_native.cpp)도 이 파일을 그대로 씀.
//  쓰는 쪽이 정해 줄 것: static bool ss_summary_on();  (모드 설정 '턴 끝 결과 창' 켬 여부)
#pragma once
#include <stddef.h>
#include <string.h>
static const int kCats=6,kItemCap=160,kItems=64,kLinesMax=70,kLineCapW=240;
// 24차(11-11 화면): 게임 문장에는 색 표시 "^05…^00"(도시 이름 초록) 이 들어 있음 — 글자 수는 이 표시를 빼고 셈(vlen), 자를 때도 표시를 안 깨뜨림(vcut).
// 22차(P219, 사용자 11-11 "증축 화면 창에 그냥 띄우게"): 넓은 창 = 게임 사건 결과 창(제목 줄 따로 + 본문). 한 줄 글자 수·창마다 줄 수는
//  SceneSkip.ini [SceneSkip] SummaryWidth(기본 36, 20~60)·SummaryLines(기본 9, 3~10) — 창 모양이 생각과 다르면 다시 빌드 없이 고칠 수 있게.
//  작은 창(넓은 창을 못 쓸 때 대신) = 나레이션 창: 제목 + 3줄(한 줄 27자), 넘치면 셋째 줄이 "그 외 N건".
static int kLineW=36,kPageLines=9;
static const int kSmallW=27,kSmallLines=3;
// 23차(11-11 화면 확인): 넓은 창 한 줄 36자·9줄(창 안쪽 폭·높이에 맞춤). 풍작은 재해와 따로(종류 5). 보여 주는 차례는 kOrder.
static const wchar_t*const kCatName[kCats]={L"재해",L"등용 결과",L"보물 발견",L"명승",L"증축",L"풍작"};
static const int kOrder[kCats]={0,5,1,2,3,4};
struct Cat{long n,ok,fail;int shown;long places;};   // places: 풍작은 한 문장에 도시가 여럿("올해는 남피, 강하, 신야, 강릉에서 풍작") → 도시 수(색 표시 ^05 개수)로 셈
static Cat cats[kCats];
// 17차(P219, 사용자 11-10): "외 N건" 으로 줄이지 않고 전부 보여 준다. 항목마다 글자를 따로 모아 두고(최대 64개),
//  한 줄에 여러 항목을 쉼표로 이어 붙임(한 줄 kLineW 자까지). 한 창에 kPageLines 줄, 넘치면 다음 창으로(창이 닫히면 이어서).
struct Item{int cat;wchar_t t[kItemCap];};
static Item items[kItems];static long nitems;
static wchar_t lines[kLinesMax][kLineCapW];static long lcum[kLinesMax];static int nlines,page_cur,page_cnt;
static long lines_total;  // 묶은 항목 수(종류별 건수 합)
static volatile long collected_total,summary_shown,summary_quiet;
static wchar_t sumtext[0x1000],sumtitle[64],sumsmall[0x800];
static void wput(wchar_t*o,size_t cap,const wchar_t*s){size_t n=0;while(n<cap&&o[n])n++;if(n>=cap)return;while(*s&&n+1<cap)o[n++]=*s++;o[n]=0;}
static void wnum(wchar_t*o,size_t cap,unsigned v){wchar_t b[12];int k=0;do{b[k++]=(wchar_t)(L'0'+v%10);v/=10;}while(v);wchar_t t[13];int j=0;while(k)t[j++]=b[--k];t[j]=0;wput(o,cap,t);}
static int wlen(const wchar_t*s){int n=0;while(s[n])n++;return n;}
static bool is_tag(const wchar_t*s){return s[0]==L'^'&&s[1]>=L'0'&&s[1]<=L'9'&&s[2]>=L'0'&&s[2]<=L'9';}
static int vlen(const wchar_t*s){int n=0;while(*s){if(is_tag(s)){s+=3;continue;}n++;s++;}return n;}   // 화면에 보이는 글자 수
// 보이는 글자 maxv 자까지(넘치면 maxv-1 자 + …). 색이 열린 채 잘리면 ^00 으로 닫고 … 
static void vcut(wchar_t*o,int cap,const wchar_t*s,int maxv){
 int k=0;if(vlen(s)<=maxv){while(*s&&k<cap-1)o[k++]=*s++;o[k]=0;return;}
 int v=0;bool open=false;
 while(*s&&k<cap-5){if(is_tag(s)){open=!(s[1]==L'0'&&s[2]==L'0');o[k++]=s[0];o[k++]=s[1];o[k++]=s[2];s+=3;continue;}if(v>=maxv-1)break;o[k++]=*s++;v++;}
 if(open){o[k++]=L'^';o[k++]=L'0';o[k++]=L'0';}o[k++]=L'\x2026';o[k]=0;
}
static void short_copy(wchar_t*o,int cap,const wchar_t*s){ // 한 줄로, 길면 … 로 자름
 int k=0;bool sp=true;const wchar_t*q=s;
 for(;q&&*q&&k<cap-1;q++){wchar_t c=*q;if(c==L'\r'||c==L'\n'||c==L'\t')c=L' ';if(c==L' '&&sp)continue;sp=(c==L' ');o[k++]=c;}
 if(q&&*q){if(k>cap-2)k=cap-2;if(k>0&&o[k-1]==L'^')k--;else if(k>1&&o[k-2]==L'^'&&o[k-1]>=L'0'&&o[k-1]<=L'9')k-=2;o[k++]=L'\x2026';}   // 색 표시 반쪽은 버림
 while(k>0&&o[k-1]==L' ')k--;o[k]=0;
}
extern "C" void SsCollect(int c,const wchar_t*line,int sub){
 if(c<0||c>=kCats||!ss_summary_on())return;Cat&k=cats[c];
 if(line&&line[0]&&nitems<kItems){Item&it=items[nitems];it.cat=c;short_copy(it.t,kItemCap,line);if(it.t[0]){nitems++;k.shown++;}}
 k.n++;if(c==1){if(sub)k.ok++;else k.fail++;}
 if(c==5){long t=0;for(const wchar_t*q=line;q&&*q;q++)if(q[0]==L'^'&&q[1]==L'0'&&q[2]==L'5')t++;k.places+=t?t:1;}
 collected_total++;
}
static long pending_total(){long t=0;for(auto&k:cats)t+=k.n;return t;}
static void push_line(const wchar_t*l,long cum){if(nlines<kLinesMax){lines[nlines][0]=0;wput(lines[nlines],kLineCapW,l);lcum[nlines]=cum;nlines++;}}
// width: 한 줄 글자 수. lcum[i] = i번째 줄까지 글자로 보여 준 항목 수(작은 창 "그 외 N건" 셈에 씀)
static void build_lines(int width){
 nlines=0;lines_total=0;long placed=0;wchar_t cur[kLineCapW];
 for(int oi=0;oi<kCats;oi++){int c=kOrder[oi];Cat&k=cats[c];if(!k.n)continue;lines_total+=k.n;
  cur[0]=0;wput(cur,kLineCapW,L"\x3010");wput(cur,kLineCapW,kCatName[c]);wput(cur,kLineCapW,L" ");
  if(c==5){wnum(cur,kLineCapW,(unsigned)k.places);wput(cur,kLineCapW,L"\xACF3");}else{wnum(cur,kLineCapW,(unsigned)k.n);wput(cur,kLineCapW,L"\xAC74");} // 【이름 N건 / 풍작은 N곳
  if(c==1){wput(cur,kLineCapW,L"(\xC131\xACF5 ");wnum(cur,kLineCapW,(unsigned)k.ok);wput(cur,kLineCapW,L"\x00B7\xC2E4\xD328 ");wnum(cur,kLineCapW,(unsigned)k.fail);wput(cur,kLineCapW,L")");} // (성공 a·실패 b)
  wput(cur,kLineCapW,L"\x3011 ");
  int on_line=0;
  for(long i=0;i<nitems;i++){if(items[i].cat!=c)continue;
   const wchar_t*tx=items[i].t;wchar_t cut[kItemCap];
   if(vlen(tx)>width-2){vcut(cut,kItemCap,tx,width-2);tx=cut;}   // 한 항목이 줄보다 길면 … 로(창 폭을 안 넘김, 색 표시는 셈에서 뺌)
   if(vlen(cur)+(on_line?2:0)+vlen(tx)>width&&vlen(cur)>2){push_line(cur,placed);cur[0]=0;wput(cur,kLineCapW,L"  ");on_line=0;}
   if(on_line)wput(cur,kLineCapW,L", ");wput(cur,kLineCapW,tx);on_line++;placed++;}
  if(k.n>k.shown&&k.shown){wput(cur,kLineCapW,L" \xC678 ");wnum(cur,kLineCapW,(unsigned)(k.n-k.shown));wput(cur,kLineCapW,L"\xAC74");placed+=k.n-k.shown;} // 64개를 넘겼을 때만: 외 N건
  else if(!k.shown)placed+=k.n;   // 글자 없이 셈만 있는 종류는 머리말 줄이 곧 그 항목
  push_line(cur,placed);
 }
}
// 작은 창 글자(넓은 창을 못 쓸 때): 제목 + 종류마다 한 줄("【재해 3건】 첫 항목…", 27자까지), 3줄까지.
//  종류가 4개 이상이면 앞 2줄 + "그 외 N건"(N = 못 보인 종류들의 건수 합). 각 줄의 건수 머리말로 몇 건인지는 다 보임.
static void small_head(wchar_t*cur,int c){Cat&k=cats[c];cur[0]=0;wput(cur,kLineCapW,L"\x3010");wput(cur,kLineCapW,kCatName[c]);wput(cur,kLineCapW,L" ");if(c==5){wnum(cur,kLineCapW,(unsigned)k.places);wput(cur,kLineCapW,L"\xACF3\x3011");}else{wnum(cur,kLineCapW,(unsigned)k.n);wput(cur,kLineCapW,L"\xAC74\x3011");}}
static void build_small(){
 sumsmall[0]=0;wput(sumsmall,0x400,L"\xC774\xBC88 \xD134 \xACB0\xACFC"); // 이번 턴 결과
 int used[kCats],nu=0;for(int oi=0;oi<kCats;oi++)if(cats[kOrder[oi]].n)used[nu++]=kOrder[oi];
 int show=nu<=kSmallLines?nu:kSmallLines-1;long rest=0;
 for(int j=0;j<nu;j++){int c=used[j];if(j>=show){rest+=cats[c].n;continue;}
  wchar_t cur[kLineCapW];small_head(cur,c);
  for(long i=0;i<nitems;i++)if(items[i].cat==c){int room=kSmallW-vlen(cur)-1;const wchar_t*tx=items[i].t;
   if(room>=4){wput(cur,kLineCapW,L" ");wchar_t cut[kItemCap];vcut(cut,kItemCap,tx,room);wput(cur,kLineCapW,cut);}
   break;}
  wput(sumsmall,0x400,L"\n");wput(sumsmall,0x400,cur);}
 if(rest){wput(sumsmall,0x400,L"\n\xADF8 \xC678 ");wnum(sumsmall,0x400,(unsigned)rest);wput(sumsmall,0x400,L"\xAC74");} // 그 외 N건
}
// 넓은 창 한 장: 제목(sumtitle) + 본문(sumtext, 줄을 \n 으로)
static void build_page(int pg){
 sumtitle[0]=0;wput(sumtitle,64,L"\xC774\xBC88 \xD134 \xACB0\xACFC"); // 이번 턴 결과
 if(page_cnt>1){wput(sumtitle,64,L" (");wnum(sumtitle,64,(unsigned)(pg+1));wput(sumtitle,64,L"/");wnum(sumtitle,64,(unsigned)page_cnt);wput(sumtitle,64,L")");}
 sumtext[0]=0;
 for(int i=pg*kPageLines;i<nlines&&i<(pg+1)*kPageLines;i++){if(sumtext[0])wput(sumtext,0x800,L"\n");wput(sumtext,0x800,lines[i]);}
}
static void build_all(){build_small();build_lines(kLineW);page_cnt=(nlines+kPageLines-1)/kPageLines;}
static void clear_collected(){memset(cats,0,sizeof cats);nitems=0;}
static void ss_summary_finish(){page_cur=page_cnt=0;}
static int clampi(int v,int lo,int hi){return v<lo?lo:v>hi?hi:v;}
static void ss_summary_shape(int width,int lines_per_page){kLineW=clampi(width,20,60);kPageLines=clampi(lines_per_page,3,10);}
// 한 걸음: 0 = 보여 줄 것 없음(원래 단계 실행) / 1 = 넓은 창 한 장(sumtitle·sumtext)을 띄울 것(못 쓰면 sumsmall 을 작은 창으로). *started = 이번에 새로 묶은 항목 수(이어지는 창이면 0).
//  17차: 줄이 많으면 창을 나눠 차례로 — 앞 창이 닫혀 이 단계가 다시 불릴 때 다음 창. 작은 창으로 대신 띄우면 ss_summary_finish() 로 나머지 장을 끝냄.
static int ss_summary_step(long*started){
 *started=0;
 if(!ss_summary_on()){page_cur=page_cnt=0;return 0;}
 if(page_cur<page_cnt){build_page(page_cur++);return 1;}
 page_cur=page_cnt=0;
 long total=pending_total();if(!total)return 0;
 // 사용자 10/29: 명승(발전 Lv 상승 등)·증축 소식만 있으면 창 없이 넘김(로그창 줄은 게임이 이미 넣음). 재해·풍작·등용·보물이 하나라도 있으면 같이 묶어 띄움.
 if(!cats[0].n&&!cats[1].n&&!cats[2].n&&!cats[5].n){clear_collected();summary_quiet++;return 0;}
 build_all();clear_collected();page_cur=0;*started=total;
 build_page(page_cur++);return 1;
}
