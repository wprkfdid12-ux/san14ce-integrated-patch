/* R17: bounded immutable records. Only the existing logger worker serializes
   award JSON or performs file I/O. It never reads officers, saves, observations,
   mutable trait rules, or RNG. Queue overflow affects diagnostics only. */
#define LOG_QUEUE 128
#define LOG_LINE 8192
#define LOG_CAP (2u*1024u*1024u)
static char log_queue[LOG_QUEUE][LOG_LINE];
static u8 log_kinds[LOG_QUEUE]; /* 0: ready text; 1: immutable award record. */
static u32 log_lengths[LOG_QUEUE],log_read,log_write,log_dropped;
static SRWLOCK log_lock;
static HANDLE log_event,log_thread;
static u64 event_sequence;
static int logger_attempted;
typedef struct{char text[LOG_LINE];u32 n;} J;
typedef struct {
 char event[64];GTarget target;GSlot result;
 u64 utc,sequence,session,epoch;u32 rules_revision,dropped,io_errors;
 u8 save_hash[32],save_hash_ok,has_distribution,has_evaluation,ai;
 u8 rerolls_used,passion_before,passion_after,stage;u16 x_mask;
 double budget_before,budget_after,heat_before,heat_after,elapsed,final_weight;
 GScore similar[G_TOP],candidates[G_TOP];u16 family[6];u32 family_count;
 double family_weight[6];u8 similar_stats[G_TOP][5];GEvaluation evaluation;
} LogRecord;
_Static_assert(sizeof(LogRecord)<=LOG_LINE,"award record exceeds bounded log slot");
static void format_log_record(const LogRecord*,J*);
/* Format a copied queue item, never a live game object. Also exposed in the
   diagnostic catalogue so tests exercise the same worker formatting path. */
static u32 format_log_item(u32 kind,const void*item,u32 n,void*out){
 if(kind==1){J b;format_log_record((const LogRecord*)item,&b);mc_copy(out,b.text,b.n);return b.n;}
 if(n>LOG_LINE)return 0;mc_copy(out,item,n);return n;
}
static int log_enqueue(const void*item,u32 size,u8 kind){
 int wake;AcquireSRWLockExclusive(&log_lock);
 if(log_write-log_read>=LOG_QUEUE){log_dropped++;ReleaseSRWLockExclusive(&log_lock);return 0;}
 u32 i=log_write%LOG_QUEUE;wake=log_write==log_read;
 mc_copy(log_queue[i],item,size);log_lengths[i]=size;log_kinds[i]=kind;log_write++;
 ReleaseSRWLockExclusive(&log_lock);if(wake)SetEvent(log_event);return 1;
}
static void jt(J*b,const char*s){if(!s)return;while(*s&&b->n<LOG_LINE-2)b->text[b->n++]=*s++;b->text[b->n]=0;}
static void ji(J*b,u64 v){char x[24];u32 n=0;do{x[n++]=(char)('0'+v%10);v/=10;}while(v);while(n&&b->n<LOG_LINE-2)b->text[b->n++]=x[--n];b->text[b->n]=0;}
static void jf(J*b,double v){if(!(v==v)||v>1e15||v< -1e15){jt(b,"null");return;}if(v<0){jt(b,"-");v=-v;}u64 i=(u64)v;u32 frac=(u32)((v-(double)i)*1000000.0+0.5),d=100000;if(frac>=1000000){i++;frac=0;}ji(b,i);jt(b,".");while(d){char c[2]={(char)('0'+frac/d),0};jt(b,c);frac%=d;d/=10;}}
static void js(J*b,const char*s){jt(b,"\"");while(s&&*s&&b->n<LOG_LINE-10){unsigned char c=(unsigned char)*s++;if(c=='"'||c=='\\'){char x[3]={'\\',(char)c,0};jt(b,x);}else if(c<32)jt(b," ");else{char x[2]={(char)c,0};jt(b,x);}}jt(b,"\"");}
static void ja(J*b,const u16*a,u32 n){u32 i;jt(b,"[");for(i=0;i<n;i++){if(i)jt(b,",");ji(b,a[i]);}jt(b,"]");}
static void log_path(u16*out,u32 index){mc_copy(out,state_path,(state_path_length+1)*2);append(out,1024,L"logs\\events.0.jsonl");out[wlen(out)-7]=(u16)('0'+index);}
static u32 log_io_errors;
static int log_append(const char*line,u32 len){u16 p[1024],q[1024];u32 written=0;long long size=0;HANDLE f;int j;
 if(!line||!len||len>LOG_LINE)return 0;
 log_path(p,0);f=CreateFileW(p,4,3,0,4,0x80,0);if(f==INVALID_HANDLE)goto failed;
 if(!GetFileSizeEx(f,&size)||size<0){CloseHandle(f);goto failed;}
 if((u64)size+len>LOG_CAP){
  CloseHandle(f);log_path(p,3);if(!DeleteFileW(p)&&GetLastError()!=2)goto failed;
  for(j=2;j>=0;j--){log_path(p,(u32)j);log_path(q,(u32)j+1);if(!MoveFileExW(p,q,1)&&GetLastError()!=2)goto failed;}
  log_path(p,0);f=CreateFileW(p,4,3,0,4,0x80,0);if(f==INVALID_HANDLE)goto failed;
  if(!GetFileSizeEx(f,&size)||size<0||(u64)size+len>LOG_CAP){CloseHandle(f);goto failed;}
 }
 int ok=WriteFile(f,line,len,&written,0)&&written==len;CloseHandle(f);if(ok)return 1;
failed: __sync_fetch_and_add(&log_io_errors,1);return 0;
}
static u32 log_worker(void*unused){static u8 item[LOG_LINE];static char line[LOG_LINE];u32 n,kind;(void)unused;
 for(;;){if(WaitForSingleObject(log_event,0xffffffffu)!=0)return 0;
  for(;;){AcquireSRWLockExclusive(&log_lock);
   if(log_read==log_write){ReleaseSRWLockExclusive(&log_lock);break;}
   u32 i=log_read%LOG_QUEUE;n=log_lengths[i];kind=log_kinds[i];mc_copy(item,log_queue[i],n);log_read++;
   ReleaseSRWLockExclusive(&log_lock);n=format_log_item(kind,item,n,line);log_append(line,n);
  }
 }
}
/* R13: Core.log does synchronous CreateFile/WriteFile/CloseHandle per call.
   Gameplay diagnostics use the EXISTING bounded event worker, not that disk
   path. Even worker startup failure never falls back to game-thread file I/O.
   Error/safety decisions happen before reporting; dropped logs cannot authorize
   an award or disable validation. Pre-initialization messages still use Core. */
static int runtime_diagnostic_deferred(const char*text){
 if(!logger_attempted)return 0;
 if(!log_thread){__sync_fetch_and_add(&log_dropped,1);return 1;}
 J line;mc_zero(&line,sizeof(line));jt(&line,"{\"mod\":\"RC4-R17-TurnTransitionPerf\",\"event\":\"runtime_diagnostic\",\"message\":");js(&line,text);jt(&line,"}\n");
 log_enqueue(line.text,line.n,0);return 1;
}
static int start_logger(void){u16 p[1024];if(logger_attempted)return log_thread!=0;logger_attempted=1;mc_copy(p,state_path,(state_path_length+1)*2);append(p,1024,L"logs");CreateDirectoryW(p,0);log_event=CreateEventW(0,0,0,0);if(!log_event)return 0;log_thread=CreateThread(0,0,log_worker,0,0,0);if(!log_thread){CloseHandle(log_event);log_event=0;report("MeritTraits: logger unavailable; gameplay remains enabled.");}return log_thread!=0;}
static void record_event(const char*event,const GTarget*t,const GState*before,const GState*after,u32 stage,const GDistribution*d,int ai,double elapsed){
 u64 sequence=++event_sequence;
 if(!log_thread){__sync_fetch_and_add(&log_dropped,1);return;}
 /* Reject a full queue before taking time/snapshot or formatting anything.
    Recheck inside enqueue in case another diagnostic producer filled it. */
 AcquireSRWLockExclusive(&log_lock);int full=log_write-log_read>=LOG_QUEUE;
 if(full)log_dropped++;ReleaseSRWLockExclusive(&log_lock);if(full)return;
 LogRecord r;mc_zero(&r,sizeof(r));r.sequence=sequence;r.session=session_id;r.epoch=world_epoch;
 GetSystemTimeAsFileTime(&r.utc);r.target=*t;r.result=after->slots[stage];r.stage=(u8)stage;
 r.rules_revision=g_rules_revision;r.save_hash_ok=(u8)active_save_hash_ok;mc_copy(r.save_hash,active_save_hash,32);
 for(u32 i=0;i<sizeof(r.event)-1&&event&&event[i];i++)r.event[i]=event[i];
 r.ai=(u8)ai;r.rerolls_used=after->rerolls_used;r.x_mask=after->x_mask;
 r.passion_before=before->passion;r.passion_after=after->passion;r.budget_before=before->budget;r.budget_after=after->budget;
 r.heat_before=before->heat;r.heat_after=after->heat;r.elapsed=elapsed;
 if(d){r.has_distribution=1;r.final_weight=d->weight[r.result.result];
  r.family_count=d->family_count;mc_copy(r.family,d->family,sizeof(r.family));mc_copy(r.family_weight,d->family_weight,sizeof(r.family_weight));
  mc_copy(r.similar,d->similar,sizeof(r.similar));mc_copy(r.candidates,d->candidates,sizeof(r.candidates));mc_copy(r.similar_stats,d->similar_stats,sizeof(r.similar_stats));
 }
 const Observation*obs=&observations[t->id][stage];
 if(r.result.outcome!=G_MISS&&obs->valid&&obs->evaluation.grade<6&&obs->result.roll_id==r.result.roll_id&&obs->result.seed==r.result.seed){r.has_evaluation=1;r.evaluation=obs->evaluation;}
 r.dropped=log_dropped;r.io_errors=log_io_errors;log_enqueue(&r,sizeof(r),1);
}
static void format_log_record(const LogRecord*r,J*out){
 J b;u32 i;const GTarget*t=&r->target;const GSlot*x=&r->result;int miss=x->outcome==G_MISS;
 const GTeacher*q=g_teacher(t->id);mc_zero(&b,sizeof(b));
 jt(&b,"{\"mod\":\"RC4-R17-TurnTransitionPerf\",\"algorithm\":6,\"game\":\"CEKO_6a6d9f72\",\"session_id\":\"");ji(&b,r->session);jt(&b,"\",\"save_hash\":\"");static const char hex[]="0123456789abcdef";if(r->save_hash_ok)for(i=0;i<32;i++){char xh[3]={hex[r->save_hash[i]>>4],hex[r->save_hash[i]&15],0};jt(&b,xh);}jt(&b,"\",\"event_id\":");ji(&b,r->sequence);jt(&b,",\"time_filetime\":\"");ji(&b,r->utc);jt(&b,"\",\"event\":");js(&b,r->event);jt(&b,",\"roll_id\":\"");ji(&b,x->roll_id);jt(&b,"\",\"seed\":\"");ji(&b,x->seed);jt(&b,"\",\"officer_id\":");ji(&b,t->id);jt(&b,",\"name\":");if(q)js(&b,q->name);else jt(&b,"null");jt(&b,",\"origin\":");js(&b,t->category==1?"official_primary":t->category==2?"official_auxiliary":"custom_or_unresolved");
 jt(&b,",\"sex\":");ji(&b,t->sex);jt(&b,",\"trait_rules_revision\":");ji(&b,r->rules_revision);jt(&b,",\"stats\":[");for(i=0;i<5;i++){if(i)jt(&b,",");ji(&b,t->stats[i]);}jt(&b,"],\"owned\":");ja(&b,t->traits,9);jt(&b,",\"slot_capacity\":9,\"star\":");ji(&b,t->star);jt(&b,",\"star_source\":\"live_unique_tactics_official_flag\",\"world_epoch\":");ji(&b,r->epoch);jt(&b,",\"stage\":");ji(&b,r->stage+1);jt(&b,",\"rerolls_used\":");ji(&b,r->rerolls_used);jt(&b,",\"rerolls_remaining\":");ji(&b,5-r->rerolls_used);jt(&b,",\"stage_rerolls\":");ji(&b,x->rerolls);
 jt(&b,",\"outcome\":");js(&b,miss?"miss":"trait");jt(&b,",\"result_trait\":");if(miss)jt(&b,"null");else ji(&b,x->result);
 jt(&b,",\"x_mask\":");ji(&b,r->x_mask);jt(&b,",\"x_slot\":");if(miss&&x->x_index<9)ji(&b,x->x_index);else jt(&b,"null");
 jt(&b,",\"passion_before\":");ji(&b,r->passion_before);jt(&b,",\"passion_after\":");ji(&b,r->passion_after);jt(&b,",\"passion_applied\":");ji(&b,x->passion_applied);
 jt(&b,",\"budget_before\":");jf(&b,r->budget_before);jt(&b,",\"budget_after\":");jf(&b,r->budget_after);jt(&b,",\"heat_before\":");jf(&b,r->heat_before);jt(&b,",\"heat_after\":");jf(&b,r->heat_after);jt(&b,",\"raw_weight\":");if(miss)jt(&b,"null");else jf(&b,x->raw);jt(&b,",\"probability\":");jf(&b,x->weight);jt(&b,",\"final_weight\":");if(r->has_distribution&&!miss)jf(&b,r->final_weight);else jt(&b,"null");jt(&b,",\"rarity_factor\":");if(miss)jt(&b,"null");else jf(&b,g_params.rarity_floor+(1-g_params.rarity_floor)*(double)g_trait_holders(x->result)/(g_trait_holders(x->result)+8.0));jt(&b,",\"fit_quality\":");if(miss)jt(&b,"null");else jf(&b,x->quality);jt(&b,",\"gold\":");if(miss)jt(&b,"null");else ji(&b,g_traits[x->result].color==3);jt(&b,",\"original_holders\":");if(miss)jt(&b,"null");else ji(&b,g_trait_holders(x->result));
 jt(&b,",\"anomaly\":");ji(&b,x->anomaly);jt(&b,",\"transcend\":");ji(&b,x->transcend);jt(&b,",\"replace_trait\":");if(miss)jt(&b,"null");else ji(&b,x->replace_trait);jt(&b,",\"result\":");if(miss)jt(&b,"null");else ji(&b,x->result);jt(&b,",\"result_name\":");if(miss)jt(&b,"null");else js(&b,g_traits[x->result].name);jt(&b,",\"phase\":");ji(&b,x->phase);jt(&b,",\"ai\":");ji(&b,r->ai);jt(&b,",\"ai_reroll_threshold\":");jf(&b,g_params.ai_threshold);jt(&b,",\"calculation_ms\":");if(r->elapsed<0)jt(&b,"null");else jf(&b,r->elapsed);
 jt(&b,",\"family\":");if(r->has_distribution)ja(&b,r->family,r->family_count);else jt(&b,"null");jt(&b,",\"similar\":[");if(r->has_distribution)for(i=0;i<G_TOP&&r->similar[i].id;i++){if(i)jt(&b,",");jt(&b,"{\"id\":");ji(&b,r->similar[i].id);jt(&b,",\"score\":");jf(&b,r->similar[i].score);jt(&b,",\"stats\":[");for(u32 k=0;k<5;k++){if(k)jt(&b,",");ji(&b,r->similar_stats[i][k]);}jt(&b,"]");jt(&b,"}");}jt(&b,"],\"top_candidates\":[");if(r->has_distribution)for(i=0;i<G_TOP&&r->candidates[i].id;i++){if(i)jt(&b,",");jt(&b,"{\"id\":");ji(&b,r->candidates[i].id);jt(&b,",\"probability\":");jf(&b,r->candidates[i].score);jt(&b,"}");}jt(&b,"]");
 jt(&b,",\"stats_source\":");js(&b,q?"official_original_database":"custom_native_base_no_db");jt(&b,",\"similarity_axes\":[\"leadership\",\"war\",\"intelligence\",\"politics\"],\"teacher_stats_source\":\"official_original_database\"");
 jt(&b,",\"family_weights\":[");if(r->has_distribution)for(i=0;i<r->family_count;i++){if(i)jt(&b,",");jf(&b,r->family_weight[i]);}jt(&b,"]");
 jt(&b,",\"evaluation\":");
 if(!miss&&r->has_evaluation){const GEvaluation*e=&r->evaluation;
  jt(&b,"{\"version\":");ji(&b,e->version);jt(&b,",\"grade\":");ji(&b,e->grade);
  static const char*const grades[]={"평범","무난","의외","대박","파격","충격"};jt(&b,",\"label\":");js(&b,grades[e->grade]);
  jt(&b,",\"score\":");jf(&b,e->score);jt(&b,",\"value_percentile\":");jf(&b,e->value);jt(&b,",\"surprise_percentile\":");jf(&b,e->surprise);
  jt(&b,",\"rarity\":");jf(&b,e->rarity);jt(&b,",\"special\":");jf(&b,e->special);jt(&b,"}");
 }else jt(&b,"null");jt(&b,",\"dropped_log_events\":");ji(&b,r->dropped);jt(&b,",\"log_io_errors\":");ji(&b,r->io_errors);jt(&b,"}\n");
 *out=b;
}
