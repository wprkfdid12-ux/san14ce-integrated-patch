/* FPS120 R8 / duel60 with stock engine timing. Exact captured CE KO build.
 * Unlike R6/R8, override the engine's per-frame FPS query and fixed-step read
 * during actual duels. This restores stock60 engine timing as well as pacing.
 * No stored graphics settings, global timing table or animation multiplier is
 * modified. Outside duels, original R5 values and native callees are retained.
 */
typedef unsigned char u8;
typedef unsigned int u32;
typedef int i32;
typedef unsigned long long u64;
typedef long long i64;
typedef struct {u32 size,abi,layout,fn_count,offset_count,image_size;u8*game;void**functions;u32*offsets;void*extra;} Resolver;
typedef struct {void*address;u32 length;u8 before[16],after[16];u32 pad;} Patch;
typedef struct {
 u32 size,abi;const Resolver*resolver;
 int(*readable)(const void*,u64);int(*patch)(const char*,Patch*,u32);
 void*(*relay)(void*,void*);void(*free_relay)(void*);void(*log)(const char*);
} Core;
typedef int(*CounterFn)(i64*);
typedef void(*SleepFn)(u32);
typedef void*(*ModuleFn)(const char*);
typedef void*(*ProcFn)(void*,const char*);
typedef void*(*CreateTimerFn)(void*,const unsigned short*,u32,u32);
typedef int(*SetTimerFn)(void*,const i64*,i32,void*,void*,int);
typedef u32(*WaitFn)(void*,u32);
typedef int(*CloseFn)(void*);
typedef u64(*ClockFn)(void);
typedef void(*DuelFn)(void*,u32);
typedef struct {i64 last;void*manager;void*timer;u32 active;} Limiter;

static const Core*g_core;
static u8*g_game;
static int g_installed,g_logged_duel;
static CounterFn g_qpc;
static SleepFn g_sleep;
static SetTimerFn g_set_timer;
static WaitFn g_wait;
static CloseFn g_close;
static ClockFn g_original_clock;
static DuelFn g_original_duel;
static i64 g_frequency,g_step;
static void*g_relays[5];
static Limiter g_render,g_duel;

static const u8 before_fps[]={0xe8,0xe4,0x32,0x7e,0x00};
static const u8 after_fps[]={0xb8,0x78,0x00,0x00,0x00};
static const u8 before_interval[]={0xf3,0x0f,0x10,0x0d,0x66,0xf9,0x1e,0x01,0xf3,0x0f,0x5e,0xc8};
static const u8 after_interval[]={0xb8,0x89,0x88,0x08,0x3c,0x66,0x0f,0x6e,0xc8,0x90,0x90,0x90};
static const u8 before_direction[]={0x45,0x33,0xc0,0xf3,0x48,0x0f,0x2c,0xd0,0xff,0x53,0x18};
static const u8 before_engine_fps[]={0x48,0x8b,0x53,0x08,0x48,0x8d,0x4b,0x08,0x48,0x89,0x44,0x24,0x40,0xff,0x52,0x28};
static const u8 before_engine_step[]={0x8b,0x40,0x34,0x41,0x89,0x47,0x18};
static const u8 before_duel[]={0xe8,0x5b,0xf4,0xf2,0xff};
static const u8 before_present_clock[]={0xe8,0x61,0x48,0xfe,0xff};

typedef struct {u32 size,version,flags,fps_rva,interval_rva,wait_rva,shared_fps_rva,shared_frame_rva,render_hz,shared_hz,portrait_hz,duel_hz,duel_root_rva,duel_active_offset,present_hook_rva,duel_hook_rva,engine_fps_hook_rva,engine_step_hook_rva;} Info;
static const Info g_info={sizeof(Info),0x80000,0x7f,0x151647,0x15322a,0x36a7b5,0x1a8a5ec,0x1a8a5f4,120,60,0,60,0x2783730,0x588,0x953fda,0x44ad50,0xf3e21f,0xf3e4ad};
const Info*SAN14_FPS120InfoV1(void){return &g_info;}
static void logline(const char*s){if(g_core&&g_core->log)g_core->log(s);}
static int equal(const u8*a,const u8*b,u32 n){u32 i;for(i=0;i<n;i++)if(a[i]!=b[i])return 0;return 1;}
static void copy(u8*d,const u8*s,u32 n){u32 i;for(i=0;i<n;i++)d[i]=s[i];}
static void zero(void*vp,u32 n){u32 i;u8*p=vp;for(i=0;i<n;i++)p[i]=0;}
static u32 read32(const u8*p){return (u32)p[0]|(u32)p[1]<<8|(u32)p[2]<<16|(u32)p[3]<<24;}
static int guard(u32 rva,const u8*bytes,u32 n){return g_core->readable(g_game+rva,n)&&equal(g_game+rva,bytes,n);}

/* Only the original CDuelManager singleton is eligible. FieldLeader3D's
 * private CUnit@duel objects / CUnitDuel actors do not satisfy this predicate.
 * Constructor clears +588; scene setup sets it to 1; teardown clears it.
 * The original update itself tests the same field before its state machine.
 */
static void*active_duel(void){
 u8*p=*(u8*volatile*)(g_game+0x2783730);
 if(!p||!g_core->readable(p,0x58c))return 0;
 if(*(u64*)p!=(u64)(g_game+0x13b5f98))return 0;
 return *(volatile u32*)(p+0x588)==1?p:0;
}

/* Independent clocks for rendering and duel logic cover both single-threaded
 * and separate update/render scheduling. Elapsed render/update work is part
 * of the budget: two gates do not each add a fixed 16.7 ms sleep.
 * A late frame never causes catch-up updates. Exit resets the private clock.
 */
static void pace(Limiter*s,void*manager){
 i64 now,deadline,left,due;
 if(!manager){s->active=0;s->manager=0;return;}
 if(!g_qpc(&now)){s->active=0;return;}
 if(!s->active||s->manager!=manager||now<s->last){
  s->last=now;s->manager=manager;s->active=1;return;
 }
 deadline=s->last+g_step;
 while(now<deadline){
  left=deadline-now;
  /* A private high-resolution, one-shot timer avoids changing the system's
   * timer resolution or the game's own scheduling configuration.
   * Leave the final 0.5 ms to yielding + QPC, and recheck after every wake.
   */
  if(s->timer&&left>g_frequency/1000){
   due=-((left-g_frequency/2000)*10000000/g_frequency);
   if(g_set_timer(s->timer,&due,0,0,0,0))g_wait(s->timer,30);
   else g_sleep(0);
  }else g_sleep(0);
  if(!g_qpc(&now)){s->active=0;return;}
  if(now<s->last){s->last=now;return;}
 }
 s->last=now;
}

static u64 present_clock(void){
 pace(&g_render,active_duel());
 return g_original_clock();
}
static void duel_update(void*p,u32 flag){
 void*manager=active_duel();
 if(manager!=p)manager=0;
 if((manager!=0)!=g_logged_duel){
  g_logged_duel=(manager!=0);
  logline(manager?"FPS120 R8: DUEL60 active (render and duel update capped at 60 Hz; engine FPS and fixed step restored to stock60).":"FPS120 R8: duel ended; original R5 render120 path restored.");
 }
 pace(&g_duel,manager);
 g_original_duel(p,flag);
}

/* Original engine F3E170: the virtual FPS property is read before its native
 * wait and before 1/FPS is used as a fallback timestep. Query the original
 * object exactly once, then supply stock60 only for the actual duel scene.
 * The existing config pointer in RAX is saved by the replacement block.
 */
static const u32 g_stock60=60;
static const u32*engine_fps(void*p){
 const u32*old=((const u32*(*)(void*))(*(void***)p)[5])(p);
 return active_duel()?&g_stock60:old;
}

/* Called at the original fixed-step load F3E4AD. Integer bit patterns preserve
 * the nonduel 1/120 value exactly; stock1/60 is the original float division
 * result (1.0f / 60.0f), not an arbitrary animation speed multiplier.
 */
u32 engine_step_bits(u32 original){return active_duel()?0x3c888889U:original;}
void engine_step_bridge(void) __attribute__((visibility("hidden")));

/* Preserve the original direction controller virtual call and return value.
 * Its original stock60 input is trunc((1/60)*1000) ==16 milliseconds.
 * Nonduel keeps the R5 input8. This is a timestep consumer, not proven to be
 * a universal frame sleep routine.
 */
static u64 direction_update(void*p){
 u64 ms=active_duel()?16:8;
 return ((u64(*)(void*,u64,u32))(*(void***)p)[3])(p,ms,0);
}

static void release_setup(void){
 u32 i;
 for(i=0;i<5;i++)if(g_relays[i]){g_core->free_relay(g_relays[i]);g_relays[i]=0;}
 if(g_render.timer&&g_close)g_close(g_render.timer);
 if(g_duel.timer&&g_close)g_close(g_duel.timer);
 zero(&g_render,sizeof(g_render));zero(&g_duel,sizeof(g_duel));
}
static int call_patch(Patch*p,u32 rva,const u8*before,void*target,u32 index){
 i64 displacement;void*relay=g_core->relay(g_game+rva,target);
 if(!relay)return 0;
 g_relays[index]=relay;displacement=(i64)(u64)relay-(i64)(u64)(g_game+rva+5);
 if(displacement<(-2147483647LL-1)||displacement>2147483647LL)return 0;
 p->address=g_game+rva;p->length=5;copy(p->before,before,5);
 p->after[0]=0xe8;*(i32*)(p->after+1)=(i32)displacement;return 1;
}
static void fixed_patch(Patch*p,u32 rva,const u8*before,const u8*after,u32 n){
 p->address=g_game+rva;p->length=n;copy(p->before,before,n);copy(p->after,after,n);
}

static int block_call(Patch*p,u32 rva,const u8*before,u32 n,void*target,u32 index,int engine){
 u32 i,at=0;void*relay=g_core->relay(g_game+rva,target);i64 disp;
 if(!relay)return 0;
 g_relays[index]=relay;
 p->address=g_game+rva;p->length=n;copy(p->before,before,n);
 for(i=0;i<n;i++)p->after[i]=0x90;
 if(engine){
  static const u8 setup[]={0x48,0x89,0x44,0x24,0x40,0x48,0x8d,0x4b,0x08};
  copy(p->after,setup,9);at=9;
 }
 disp=(i64)(u64)relay-(i64)(u64)(g_game+rva+at+5);
 if(disp<(-2147483647LL-1)||disp>2147483647LL)return 0;
 p->after[at]=0xe8;*(i32*)(p->after+at+1)=(i32)disp;return 1;
}

int SAN14_ModInitV1(const Core*c){
 Patch patches[7];u32 off,i;u8*nt;i64 probe;CounterFn qpf;ModuleFn get_module;ProcFn get_proc;void*kernel;CreateTimerFn create_timer;
 static const u8 stock_fps[]={0x3c,0,0,0},stock_frame[]={0x88,0x88,0x88,0x3c};
 static const u8 duel_ctor[]={0x48,0x8d,0x05,0x91,0x56,0x04,0x01,0x48,0x89,0x01};
 static const u8 duel_enter[]={0xc7,0x81,0x88,0x05,0,0,1,0,0,0};
 static const u8 duel_exit[]={0x89,0xb1,0x88,0x05,0,0,0x89,0x71,0x10};
 static const u8 duel_guard[]={0x83,0xb9,0x88,0x05,0,0,0,0x48,0x8b,0xd9};
 static const u8 present_prefix[]={0x48,0x89,0x5c,0x24,8,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18};
 static const u8 root_load[]={0x48,0x8b,0x0d,0xe0,0x89,0x33,0x02};
 if(g_installed)return c&&c->resolver&&c->resolver->game==g_game;
 if(!c||c->size<56||c->abi!=0x10000||!c->resolver||!c->readable||!c->patch||!c->relay||!c->free_relay)return 0;
 if(c->resolver->abi!=0x10000||c->resolver->image_size!=0x29ff000||!c->resolver->game)return 0;
 g_core=c;g_game=c->resolver->game;
 if(!c->readable(g_game,4096)||g_game[0]!='M'||g_game[1]!='Z')return 0;
 off=read32(g_game+0x3c);if(off>0xf00)return 0;nt=g_game+off;
 if(read32(nt)!=0x4550||read32(nt+8)!=0x6a6d9f72||read32(nt+0x50)!=0x29ff000)return 0;
 if(!guard(0x1a8a5ec,stock_fps,4)||!guard(0x1a8a5f4,stock_frame,4)){
  logline("FPS120 R8: native 60-mode table differs from stock; no patches applied.");return 0;
 }
 if(!guard(0x151647,before_fps,5)||!guard(0x15322a,before_interval,12)||!guard(0x36a7b5,before_direction,11)||!guard(0xf3e21f,before_engine_fps,16)||!guard(0xf3e4ad,before_engine_step,7)||
    !guard(0x44ad50,before_duel,5)||!guard(0x953fda,before_present_clock,5)||
    !guard(0x370900,duel_ctor,10)||!guard(0x372c25,duel_enter,10)||!guard(0x379c04,duel_exit,9)||
    !guard(0x37a1b6,duel_guard,10)||!guard(0x953fc0,present_prefix,15)||!guard(0x44ad49,root_load,7)||
    !c->readable(g_game+0x2783730,8)||!c->readable(g_game+0x1340000,0x540)){
  logline("FPS120 R8: unsupported code or hook conflict; no patches applied.");return 0;
 }
 g_qpc=*(CounterFn*)(g_game+0x13400e8);qpf=*(CounterFn*)(g_game+0x13400f0);
 g_sleep=*(SleepFn*)(g_game+0x13400e0);g_wait=*(WaitFn*)(g_game+0x13401e0);
 get_module=*(ModuleFn*)(g_game+0x1340228);get_proc=*(ProcFn*)(g_game+0x1340318);
 if(!g_qpc||!qpf||!g_sleep||!g_wait||!get_module||!get_proc)return 0;
 if(!qpf(&g_frequency)||g_frequency<60||g_frequency>1000000000000LL||!g_qpc(&probe))return 0;
 g_step=(g_frequency+59)/60;
 g_original_clock=(ClockFn)(g_game+0x938840);g_original_duel=(DuelFn)(g_game+0x37a1b0);
 kernel=get_module("kernel32.dll");
 if(kernel){
  create_timer=(CreateTimerFn)get_proc(kernel,"CreateWaitableTimerExW");
  g_set_timer=(SetTimerFn)get_proc(kernel,"SetWaitableTimer");g_close=(CloseFn)get_proc(kernel,"CloseHandle");
  if(create_timer&&g_set_timer&&g_close){
   g_render.timer=create_timer(0,0,2,0x001f0003);g_duel.timer=create_timer(0,0,2,0x001f0003);
  }
 }
 zero(patches,sizeof(patches));
 fixed_patch(patches,0x151647,before_fps,after_fps,5);
 fixed_patch(patches+1,0x15322a,before_interval,after_interval,12);
 if(!call_patch(patches+2,0x953fda,before_present_clock,(void*)present_clock,0)||
    !call_patch(patches+3,0x44ad50,before_duel,(void*)duel_update,1)||
    !block_call(patches+4,0x36a7b5,before_direction,11,(void*)direction_update,2,0)||
    !block_call(patches+5,0xf3e21f,before_engine_fps,16,(void*)engine_fps,3,1)||
    !block_call(patches+6,0xf3e4ad,before_engine_step,7,(void*)engine_step_bridge,4,0)){
  release_setup();logline("FPS120 R8: relay allocation failed; no patches applied.");return 0;
 }
 if(!c->patch("SAN14CE.FPS120.R8.Duel60_StockTiming_Other120",patches,7)){
  release_setup();logline("FPS120 R8: atomic patch transaction rejected; no patches applied.");return 0;
 }
 for(i=0;i<7;i++)if(!equal(patches[i].address,patches[i].after,patches[i].length)){
  /* Never free executable relays after a successful commit. */
  logline("FPS120 R8: post-commit verification warning; committed relays retained.");g_installed=1;return 1;
 }
 g_installed=1;
 logline("FPS120 R8 ACTIVE: duel60 with stock engine FPS/fixed-step/direction timing; other scenes retain R5 render120; no extra slowdown.");
 return 1;
}
int DllMain(void*module,u32 reason,void*reserved){(void)module;(void)reason;(void)reserved;return 1;}
