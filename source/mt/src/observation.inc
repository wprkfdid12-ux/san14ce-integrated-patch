/* One immutable observation per completed calculation. Rendering and scrolling
 * do not call g_distribution, g_roll, entropy or scan the teacher database. */
typedef struct {
 u8 valid,revealed,top,lower;u16 flags;u8 notice_shown,passion_before;GTarget input;GSlot result;
 GScore similar[G_TOP],candidates[G_TOP];u16 family[6];u32 family_count,candidate_count;
 double budget_before,budget_after,heat_before,heat_after,total,elapsed;
 GEvaluation evaluation;u8 seed_hash[32];double family_weight[6];
 u8 similar_stats[G_TOP][5];u16 x_mask_before;
} Observation;
static Observation observations[MAX_OFFICERS][3];
static u64 calculation_count;
static const u16*const grade_names[]={L"평범",L"무난",L"의외",L"대박",L"파격",L"충격"};
static void snapshot_reset(void){mc_zero(observations,sizeof(observations));calculation_count=0;}
static void capture_snapshot(const GTarget*t,const GState*b,const GState*a,u32 stage,const GDistribution*d,double ms){
 if(t->id>=MAX_OFFICERS||stage>2)return;Observation*s=&observations[t->id][stage];u8*o=officer(t->id);
 mc_zero(s,sizeof(*s));s->valid=1;s->input=*t;s->result=a->slots[stage];s->candidate_count=d->count;
 s->passion_before=b->passion;s->x_mask_before=b->x_mask;
 if(o){s->top=B(o,OFF(O_UPPER));s->lower=B(o,OFF(O_LOWER));s->flags=S(o,OFF(O_FLAGS));}
 mc_copy(s->similar,d->similar,sizeof(s->similar));mc_copy(s->candidates,d->candidates,sizeof(s->candidates));
 mc_copy(s->family,d->family,sizeof(s->family));s->family_count=d->family_count;mc_copy(s->family_weight,d->family_weight,sizeof(s->family_weight));
 mc_copy(s->similar_stats,d->similar_stats,sizeof(s->similar_stats));
 s->budget_before=b->budget;s->budget_after=a->budget;s->heat_before=b->heat;s->heat_after=a->heat;s->total=d->total;s->elapsed=ms;
 g_evaluate(d,&s->result,&s->evaluation);mc_sha256(&s->result.seed,sizeof(s->result.seed),s->seed_hash);calculation_count++;
}
static void accept_snapshot(u16 id,u32 stage,const GState*s){if(id>=MAX_OFFICERS||stage>2)return;Observation*o=&observations[id][stage];
 if(o->valid&&o->result.roll_id==s->slots[stage].roll_id){o->budget_after=s->budget;o->heat_after=s->heat;o->result=s->slots[stage];o->revealed=1;}
}
#define OBS_LINES 256
#define OBS_CHARS 128
static u16 observation_lines[OBS_LINES][OBS_CHARS];static u32 observation_line_count;
typedef struct {u16 text[OBS_CHARS];u32 n;} W;
static void wt(W*w,const u16*t){while(t&&*t&&w->n<OBS_CHARS-1)w->text[w->n++]=*t++;w->text[w->n]=0;}
static void wi(W*w,u64 value){u16 b[24];u32 n=0;do{b[n++]=(u16)('0'+value%10);value/=10;}while(value);while(n&&w->n<OBS_CHARS-1)w->text[w->n++]=b[--n];w->text[w->n]=0;}
static void wf(W*w,double v){if(!(v>=-1e12&&v<=1e12)){wt(w,L"미측정");return;}if(v<0){wt(w,L"-");v=-v;}u64 z=(u64)(v*1000.0+.5);wi(w,z/1000);wt(w,L".");u16 t[4]={(u16)('0'+(z/100)%10),(u16)('0'+(z/10)%10),(u16)('0'+z%10),0};wt(w,t);}
static void wu(W*w,const char*t){while(t&&*t&&w->n<OBS_CHARS-2){u32 c=(u8)*t++;if(c>=0xc0&&c<0xe0){if(!*t)break;c=((c&31)<<6)|((u8)*t++&63);}else if(c>=0xe0&&c<0xf0){if(!t[0]||!t[1])break;c=((c&15)<<12)|(((u8)t[0]&63)<<6)|((u8)t[1]&63);t+=2;}else if(c>=0xf0){if(!t[0]||!t[1]||!t[2])break;c=((c&7)<<18)|(((u8)t[0]&63)<<12)|(((u8)t[1]&63)<<6)|((u8)t[2]&63);t+=3;if(c>0x10ffff)continue;c-=0x10000;w->text[w->n++]=(u16)(0xd800+(c>>10));c=0xdc00+(c&1023);}w->text[w->n++]=(u16)c;}w->text[w->n]=0;}
static void wperson(W*w,u16 id){const GTeacher*q=g_teacher(id);if(q)wu(w,q->name);else{wt(w,L"무장 #");wi(w,id);}}
static void obs_line(W*w,u32 width){u32 i=0,px=0,n=0;u16 *dst;
 if(!w->n||observation_line_count>=OBS_LINES)return;dst=observation_lines[observation_line_count++];
 while(i<w->n){u16 c=w->text[i++];u32 cost=c<128?8:16;
  if(px+cost>width||n>=OBS_CHARS-2){dst[n]=0;if(observation_line_count>=OBS_LINES)break;dst=observation_lines[observation_line_count++];px=n=0;}
  dst[n++]=c;px+=cost;
 }dst[n]=0;w->n=0;w->text[0]=0;
}
static void build_observation(u16 id,u32 stage,u32 width){W w={{0},0};u32 i;observation_line_count=0;
 if(width<48)width=48;if(id>=MAX_OFFICERS||stage>2){wt(&w,L"개성 해금칸을 선택하세요.");obs_line(&w,width);return;}
 GState*state=ui_gstate(id);GSlot*x=&state->slots[stage];Observation*s=&observations[id][stage];u8*o=officer(id);
 wt(&w,L"해금 ");wi(&w,stage+1);wt(&w,L" / 3");obs_line(&w,width);
 if(!x->phase){wt(&w,o&&B(o,OFF(O_UPPER))>stage?L"해금됨 / 미추첨":(inspected_available?L"원작 선택 조건 충족 / 아직 추첨 안 함":L"원작 해금 조건 미충족 / 아직 추첨 안 함"));obs_line(&w,width);return;}
 /* Nothing that identifies or grades the final result is emitted while spinning. */
 if(ui_mode==2&&id==active_id&&stage==active_stage){wt(&w,L"추첨 연출 중입니다. 결과 공개 후 계산 근거를 표시합니다.");obs_line(&w,width);return;}
 if(!s->valid||s->result.roll_id!=x->roll_id||s->result.seed!=x->seed){
  wt(&w,L"저장된 결과 요약");obs_line(&w,width);wt(&w,x->outcome==G_MISS?L"X 꽝 — 평가 없음":trait_name(x->result));obs_line(&w,width);
  wt(&w,L"이전 세션의 상세 계산 기록은 로그에서 확인하세요. 후보를 재계산하지 않습니다.");obs_line(&w,width);
  wt(&w,L"재추첨 잔여 ");wi(&w,G_REROLLS-state->rerolls_used);obs_line(&w,width);return;
 }
 s->revealed=1;wperson(&w,id);obs_line(&w,width);
 wt(&w,L"열정 버프 ");wt(&w,s->passion_before?L"적용":L"미적용");obs_line(&w,width);
 if(x->outcome==G_MISS){wt(&w,L"X 꽝 — 평가 없음");obs_line(&w,width);wt(&w,L"결정으로 꽝을 확정하면 다음 논공 기회에 열정 버프가 적용됩니다.");obs_line(&w,width);wt(&w,L"재추첨 잔여 ");wi(&w,G_REROLLS-state->rerolls_used);obs_line(&w,width);return;}
 wt(&w,L"원작 활성 ");wi(&w,s->top);wt(&w,L" / 상태 ");wi(&w,s->flags);obs_line(&w,width);
 wt(&w,g_teacher(id)?L"비교용 능력치: DB 원본":L"비교용 능력치: 신무장 현재값 (원본 DB 없음)");obs_line(&w,width);wt(&w,L"통/무/지/정/매");obs_line(&w,width);for(i=0;i<5;i++){if(i)wt(&w,L"/");wi(&w,s->input.stats[i]);}obs_line(&w,width);
 u32 occupied=0,xcount=0;for(i=0;i<9;i++){if(s->input.traits[i]&&s->input.traits[i]!=NO_TRAIT)occupied++;else if(s->x_mask_before&(1u<<i))xcount++;}wt(&w,L"점유 슬롯 ");wi(&w,occupied+xcount);wt(&w,L"/9 (실제 ");wi(&w,occupied);wt(&w,L" + X ");wi(&w,xcount);wt(&w,L")");obs_line(&w,width);wt(&w,L"기존 개성:");obs_line(&w,width);for(i=0;i<9;i++)if(s->input.traits[i]&&s->input.traits[i]<MAX_TRAITS){wu(&w,g_traits[s->input.traits[i]].name);obs_line(&w,width);}
 wt(&w,L"스타 ");wt(&w,s->input.star?L"예 (고유전법)":L"아니오");obs_line(&w,width);
 wt(&w,L"혈연 ");wi(&w,s->family_count);wt(&w,L"명");obs_line(&w,width);
 for(i=0;i<s->family_count&&i<6;i++){wperson(&w,s->family[i]);wt(&w,L" ");wf(&w,s->family_weight[i]);obs_line(&w,width);}
 wt(&w,L"혈연 총합 상한 ");wf(&w,g_params.family_cap);obs_line(&w,width);
 wt(&w,L"비교 기준: 공식 DB 원본 능력치");obs_line(&w,width);
 wt(&w,L"유사 무장:");obs_line(&w,width);for(i=0;i<G_TOP&&s->similar[i].id;i++){wperson(&w,s->similar[i].id);wt(&w,L" ");wf(&w,s->similar[i].score);obs_line(&w,width);for(u32 k=0;k<5;k++){if(k)wt(&w,L"/");wi(&w,s->similar_stats[i][k]);}wt(&w,L" (DB 원본)");obs_line(&w,width);}
 wt(&w,L"후보 ");wi(&w,s->candidate_count);wt(&w,L"개 / 상위:");obs_line(&w,width);
 for(i=0;i<G_TOP&&s->candidates[i].id;i++){wu(&w,g_traits[s->candidates[i].id].name);wt(&w,L" ");wf(&w,100*s->candidates[i].score);wt(&w,L"%");obs_line(&w,width);}
 wt(&w,L"선택: ");wt(&w,trait_name(x->result));obs_line(&w,width);
 wt(&w,L"원가중치 ");wf(&w,x->raw);obs_line(&w,width);wt(&w,L"최종가중치 ");wf(&w,x->weight*s->total);obs_line(&w,width);
 wt(&w,L"선택확률 ");wf(&w,100*x->weight);wt(&w,L"%");obs_line(&w,width);
 wt(&w,L"황금 ");wt(&w,g_traits[x->result].color==3?L"예":L"아니오");obs_line(&w,width);
 wt(&w,L"원본 보유자 ");wi(&w,g_trait_holders(x->result));obs_line(&w,width);
 wt(&w,L"희소도 보정 ");wf(&w,g_params.rarity_floor+(1-g_params.rarity_floor)*(double)g_trait_holders(x->result)/(g_trait_holders(x->result)+8.0));obs_line(&w,width);
 wt(&w,L"예산 ");wf(&w,s->budget_before);wt(&w,L" → ");wf(&w,s->budget_after);obs_line(&w,width);
 wt(&w,L"열기 ");wf(&w,s->heat_before);wt(&w,L" → ");wf(&w,s->heat_after);obs_line(&w,width);
 wt(&w,x->transcend?L"초월 발생":x->anomaly?L"이변 발생":L"일반 추첨");obs_line(&w,width);
 wt(&w,L"재추첨 ");wi(&w,state->rerolls_used);wt(&w,L"/5 (잔여 ");wi(&w,5-state->rerolls_used);wt(&w,L")");obs_line(&w,width);
 wt(&w,L"Roll ID ");wi(&w,x->roll_id);obs_line(&w,width);wt(&w,L"Seed hash ");static const u16 h[]=L"0123456789abcdef";for(i=0;i<8;i++){u16 q[3]={h[s->seed_hash[i]>>4],h[s->seed_hash[i]&15],0};wt(&w,q);}obs_line(&w,width);
 wt(&w,L"계산 ");wf(&w,s->elapsed);wt(&w,L" ms");obs_line(&w,width);
 wt(&w,L"이번 가챠 평가");obs_line(&w,width);wt(&w,grade_names[s->evaluation.grade]);wt(&w,L" (");wf(&w,s->evaluation.score);wt(&w,L"점)");obs_line(&w,width);
 wt(&w,L"상대 가치·놀라움·공식 희소성 기준 / 평가식 v1");obs_line(&w,width);
}
