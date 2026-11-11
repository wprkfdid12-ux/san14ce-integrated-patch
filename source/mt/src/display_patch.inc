/* The supplied ModCore InstallHooks accepts GAME-image targets only.
 * Never widen its range or modify the shared Core. Verified display adapters
 * own their small DLL-local E8 call edits separately. Callers must fingerprint
 * the exact module and whitelist each original call before reaching here.
 * Caller-owned rollback retains relays if protection/cache restoration fails.
 */
static int install_display_calls(u8*module,u32 image_size,CorePatch*patches,u32 count){
 u32 i,j,opened=0,old[16],ignored;int ok=0;
 if(!module||!patches||!count||count>16||image_size<0x1000)return 0;
 for(i=0;i<count;i++){CorePatch*p=&patches[i];usize at=(usize)p->address,base=(usize)module;
  if(p->length!=5||at<base+0x1000||at-base>image_size-5||p->before[0]!=0xe8||p->after[0]!=0xe8||!rd(p->address,5)||!mc_equal(p->address,p->before,5))return 0;
  for(j=0;j<i;j++){usize other=(usize)patches[j].address;if(at<other+5&&other<at+5)return 0;}
 }
 /* Open all pages before writing, then recheck every original byte. Reverse
    restoration handles several call sites sharing the same native page. */
 for(i=0;i<count;i++){if(!VirtualProtect(patches[i].address,5,0x40,&old[i]))goto restore;opened++;}
 for(i=0;i<count;i++)if(!mc_equal(patches[i].address,patches[i].before,5))goto restore;
 for(i=0;i<count;i++)mc_copy(patches[i].address,patches[i].after,5);
 ok=FlushInstructionCache(GetCurrentProcess(),module,image_size)!=0;
restore:
 while(opened){i=--opened;if(!VirtualProtect(patches[i].address,5,old[i],&ignored))ok=0;}
 return ok;
}
