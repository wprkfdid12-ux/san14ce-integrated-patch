static const char*const kFuncName[F_COUNT]={"COMMIT","BONUS","ROW_REFRESH","STATUS","CONFIRM","STREAM_OPEN","LOAD","NEWGAME","SAVE_PREP","SAVE_END","VALID","LABEL_COLOR","CAPTION_COLOR","UI_EXIT","ELIGIBLE","STATE_TICK","CLOSE","PRESENT_RESUME","PRESENT_FINISH","BUTTON_EVENT","ROW_DESTROY"};
static void gcat(char*o,u32 cap,const char*a){u32 n=0;while(n<cap-1&&o[n])n++;while(*a&&n<cap-1)o[n++]=*a++;o[n]=0;}
static void ghex(char*o,u32 cap,u64 v,int digits){char t[20];static const char h[]="0123456789abcdef";int i;for(i=0;i<digits;i++)t[i]=h[(v>>((digits-1-i)*4))&15];t[digits]=0;gcat(o,cap,t);}
/* 다른 모듈의 점프 연결이면 덮인 길이(5/14), 아니면 0. 대상 주소를 *to 에 */
static u32 foreign_jump(const u8*p,usize*to){
 if(p[0]==0xe9){*to=(usize)(p+5)+(usize)(long long)(int)D(p,1);return 5;}
 if(p[0]==0xff&&p[1]==0x25&&D(p,2)==0){*to=*(const usize*)(p+6);return 14;}
 return 0;
}
static int function_guards_ok(void){u32 i,tolerated=0;char m[260];
 for(i=0;i<F_COUNT;i++){const u8*p=game+res->funcs[i];const u8*g=res->function_guards[i];usize to=0;u32 k;
  if(!rd(p,16)){m[0]=0;gcat(m,sizeof m,"MeritTraits: native function F_");gcat(m,sizeof m,kFuncName[i]);gcat(m,sizeof m," exe+0x");ghex(m,sizeof m,res->funcs[i],6);gcat(m,sizeof m," unreadable; refusing to hook.");report(m);return 0;}
  if(mc_equal(p,g,16))continue;
  k=(i==F_ELIGIBLE)?0:foreign_jump(p,&to);
  if(k&&(k>=16||mc_equal(p+k,g+k,16-k))&&rd((const void*)to,16)&&!address_in_module(GetModuleHandleW(0),to)){tolerated++;
   m[0]=0;gcat(m,sizeof m,"MeritTraits: F_");gcat(m,sizeof m,kFuncName[i]);gcat(m,sizeof m," exe+0x");ghex(m,sizeof m,res->funcs[i],6);gcat(m,sizeof m," starts with another module's jump (to 0x");ghex(m,sizeof m,to,12);gcat(m,sizeof m,"); 13 only calls it, so it is used as is.");report(m);continue;}
  m[0]=0;gcat(m,sizeof m,"MeritTraits: native target function was modified; refusing to hook. F_");gcat(m,sizeof m,kFuncName[i]);gcat(m,sizeof m," exe+0x");ghex(m,sizeof m,res->funcs[i],6);gcat(m,sizeof m," first bytes ");
  {u32 j;for(j=0;j<8;j++){ghex(m,sizeof m,p[j],2);}}
  if(foreign_jump(p,&to)){gcat(m,sizeof m," jump to 0x");ghex(m,sizeof m,to,12);}
  gcat(m,sizeof m,i==F_ELIGIBLE?" (13 overwrites this function's start, so it must be original).":" (not a recognizable jump hook).");report(m);return 0;}
 (void)tolerated;return 1;
}
