#pragma once
// 22번 AI 내정 순환(군주 반영) — 순수 판정 규칙 (게임·윈도 의존 없음, test/test_domestic_ai.cpp 가 그대로 돌림)
//  규칙(2026-11-02 합의):
//   · 플레이어 세력 지역은 손대지 않음(원래 게임 판정 그대로).
//   · AI 세력 지역: 군주 정치가 70 이상이면 모든 지역, 40~69 는 절반, 40 미만은 3분의 1만 "골고루"(순환) 개발.
//     어느 지역이 순환 지역인지는 (지역 번호, 세력, 게임 연·월) 로 정해져 달이 바뀌면 섞임(게임 난수는 안 씀).
//   · 순환 = 상업·농업·병영 중 단계(100/300/600/1000)가 가장 낮은 것, 같으면 상업→농업→병영. 1000 이면 제외.
//   · 순환 아닌 지역 = 군주 내면 '물자'(금→상업, 군량→농업, 병사→병영) 한 가지. 그것이 1000 이면 순환 규칙으로.
//   · 결과가 지금 중점과 같고 담당관이 이미 있으면 0(바꿀 것 없음) — 게임 원래 함수와 같은 뜻.
namespace dai{
struct Area{unsigned commerce,agri,barracks;unsigned focus;bool officer_valid;};   // focus 1=상업 2=농업 3=병영
struct Ruler{int politics;int gold,food,troops;};                                  // 물자 선호 -20..20 (20 = 중시)
enum{ROT_ALL=100,ROT_HALF=50,ROT_THIRD=33};
inline unsigned share_for(int politics){return politics>=70?ROT_ALL:politics>=40?ROT_HALF:ROT_THIRD;}
inline unsigned level(unsigned v){return 1u+(v>=100u)+(v>=300u)+(v>=600u)+(v>=1000u);}
// 지역·세력·연월로 정해지는 0..99 값 (달이 바뀌면 지역마다 다르게 섞임)
inline unsigned mix(unsigned area_idx,unsigned force,int year,int month){
 unsigned ym=(unsigned)(year*12+month);
 unsigned h=area_idx*2654435761u;h^=force*40503u+0x9e3779b9u;h^=ym*2246822519u;
 h^=h>>15;h*=2246822519u;h^=h>>13;h*=3266489917u;h^=h>>16;return h%100u;
}
inline bool rotates(unsigned area_idx,unsigned force,int year,int month,int politics){return mix(area_idx,force,year,month)<share_for(politics);}
inline unsigned balanced(const Area&a){
 unsigned v[3]={a.commerce,a.agri,a.barracks};unsigned best=0,bl=99;
 for(unsigned i=0;i<3;i++){if(v[i]>=1000u)continue;unsigned l=level(v[i]);if(l<bl){bl=l;best=i+1;}}
 return best;   // 0 = 셋 다 1000
}
inline unsigned preferred(const Ruler&r){   // 물자 중시 항목 1..3 (같으면 금→군량→병사)
 int v[3]={r.gold,r.food,r.troops};unsigned best=1;for(unsigned i=1;i<3;i++)if(v[i]>v[best-1])best=i+1;return best;
}
struct Result{unsigned focus;bool rotated;unsigned chosen;};   // focus = 돌려줄 값(0=바꿀 것 없음), chosen = 규칙이 고른 항목
inline Result decide(const Area&a,const Ruler&r,unsigned area_idx,unsigned force,int year,int month){
 Result o{0,false,0};
 o.rotated=rotates(area_idx,force,year,month,r.politics);
 unsigned c=0;
 if(!o.rotated){unsigned p=preferred(r);unsigned pv=p==1?a.commerce:p==2?a.agri:a.barracks;if(pv<1000u)c=p;}
 if(!c)c=balanced(a);
 o.chosen=c;
 if(!c)return o;
 if(c==a.focus&&a.officer_valid)return o;   // 이미 그대로
 o.focus=c;return o;
}
}
