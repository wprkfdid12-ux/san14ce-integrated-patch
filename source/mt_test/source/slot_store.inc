/* Slot-bound two-generation companion store. The native .s14 is never tagged,
 * appended to or rewritten by this module. Identity = native Steam account +
 * exact native filename. Each slot retains at most two full snapshots and one
 * small checked journal; repeated rerolls cannot grow an unbounded ledger.
 *
 * Protocol: durable inactive snapshot -> durable INTENT -> native write ->
 * durable COMMITTED reference. On a crash, different native hashes identify
 * the matching generation. Identical native bytes with different gacha state
 * are deliberately ambiguous while INTENT: fail closed, never guess a result.
 */
#define SLOT_COMMITTED 0u
#define SLOT_INTENT 1u
#define SLOT_SCAN_LIMIT 4096u
static u64 native_account_id(void);
static int validate_loaded_disk(const u8*,long long);
static int read_slot_unlocked(const u8*);
static int save_slot_key_ok,load_slot_key_ok;
static u8 save_slot_key[32],load_slot_key[32],save_account_tag[16],load_account_tag[16];
typedef struct {
 u8 magic[8];u32 version,phase;
 u8 key[32],account_tag[16];
 u32 current,previous_exists;u64 sequence;
 u8 native_hash[32],body_hash[32],previous_native_hash[32],previous_body_hash[32];
 u32 previous,reserved;
 u8 checksum[32];
} SlotRef;
static SlotRef staged_slot,prior_slot;
static int slot_journal_active,slot_prior_exists;
static void slot_name(const u8 key[32],const u16*suffix,u16 out[1024]){
 static const char hex[]="0123456789abcdef";u32 i;
 mc_copy(out,state_path,(state_path_length+1)*2);
 for(i=0;i<32;i++){out[state_path_length+i*2]=(u16)hex[key[i]>>4];out[state_path_length+i*2+1]=(u16)hex[key[i]&15];}
 out[state_path_length+64]=0;append(out,1024,suffix);
}
static void slot_object_name(const u8 key[32],u32 generation,u16 out[1024]){
 slot_name(key,generation?L".slot1.mt4dat":L".slot0.mt4dat",out);
}
static int exact_read(const u16*path,void*out,u32 size){
 HANDLE f;long long n=0;u32 got=0;int ok;
 f=CreateFileW(path,0x80000000,1,0,3,0x80,0);if(f==INVALID_HANDLE)return GetLastError()==2?0:-1;
 ok=GetFileSizeEx(f,&n)&&n==size&&ReadFile(f,out,size,&got,0)&&got==size;CloseHandle(f);return ok?1:-1;
}
static int atomic_write(const u16*path,const void*bytes,u32 size){
 u16 tmp[1024];HANDLE f;u32 wrote=0;int ok;
 mc_copy(tmp,path,(wlen(path)+1)*2);if(!append(tmp,1024,L".tmp"))return 0;
 f=CreateFileW(tmp,0x40000000,0,0,2,0x80,0);if(f==INVALID_HANDLE)return 0;
 ok=WriteFile(f,bytes,size,&wrote,0)&&wrote==size&&FlushFileBuffers(f);CloseHandle(f);
 if(ok)ok=MoveFileExW(tmp,path,1|8);
 if(!ok)DeleteFileW(tmp);return ok;
}
static int slot_identity(void*string,u8 key[32],u8 tag[16]){
 u8 material[1064],hash[32],*text;u64 account;usize n,capacity;
 if(!rd(string,32))return 0;n=Q(string,0x10);capacity=Q(string,0x18);
 if(!n||n>1023||capacity<n)return 0;text=capacity>=16?P(string,0):(u8*)string;
 if(!rd(text,n+1)||text[n])return 0;
 account=native_account_id();if(!account)return 0;
 mc_zero(material,32);mc_copy(material,"SAN14CE.MeritTraits.slot.v1",sizeof("SAN14CE.MeritTraits.slot.v1")-1);mc_copy(material+32,&account,8);mc_copy(material+40,text,n);
 mc_sha256(material,40+n,key);mc_sha256(&account,8,hash);mc_copy(tag,hash,16);return 1;
}
static int slot_ref_valid(const SlotRef*r,const u8*key,const u8*tag){
 u8 h[32];
 if(!mc_equal(r->magic,"MT4SLOT1",8)||r->version!=1||r->phase>1||r->current>1||r->previous>1||r->previous_exists>1||!r->sequence||r->reserved)return 0;
 if(key&&!mc_equal(r->key,key,32))return 0;if(tag&&!mc_equal(r->account_tag,tag,16))return 0;
 if(r->previous_exists&&r->current==r->previous)return 0;
 mc_sha256(r,__builtin_offsetof(SlotRef,checksum),h);return mc_equal(h,r->checksum,32);
}
static int slot_write_ref(SlotRef*r){
 u16 path[1024];slot_name(r->key,L".mt4ref",path);
 mc_sha256(r,__builtin_offsetof(SlotRef,checksum),r->checksum);return atomic_write(path,r,sizeof(*r));
}
static int slot_read_ref(const u8*key,const u8*tag,SlotRef*out){
 u16 path[1024];int r;slot_name(key,L".mt4ref",path);r=exact_read(path,out,sizeof(*out));
 return r==1?(slot_ref_valid(out,key,tag)?1:-1):r;
}
static void fill_disk(const u8*bytes,u32 size){
 mc_zero(&disk,sizeof(disk));mc_copy(disk.magic,"S14MT001",8);disk.version=3;disk.record_size=sizeof(Profile);disk.count=MAX_OFFICERS;
 mc_sha256(bytes,size,disk.save_hash);mc_copy(disk.records,save_profiles,sizeof(save_profiles));mc_copy(disk.gacha,save_states,sizeof(save_states));
 mc_sha256(disk.records,sizeof(disk.records)+sizeof(disk.gacha),disk.body_hash);
}
static int slot_prepare_unlocked(const u8*bytes,u32 size){
 u16 path[1024];int status;slot_journal_active=0;sidecar_collision=0;
 if(!save_slot_key_ok||!pending_save||!size||size>256u*1024u*1024u||!rd(bytes,size))return 0;
 fill_disk(bytes,size);
 mc_zero(&prior_slot,sizeof(prior_slot));status=slot_read_ref(save_slot_key,save_account_tag,&prior_slot);slot_prior_exists=status==1;
 /* R40 A12: an interrupted earlier save (INTENT) or an unreadable journal no
    longer blocks every later save to this slot. Keep the last committed
    generation as "previous" and overwrite only the unfinished one. */
 if(status<0){report("MeritTraits R40: unreadable slot journal; starting a new journal for this slot (native save not blocked).");mc_zero(&prior_slot,sizeof(prior_slot));status=0;slot_prior_exists=0;}
 else if(status==1&&prior_slot.phase!=SLOT_COMMITTED){
  if(prior_slot.previous_exists){u32 cur=prior_slot.current;prior_slot.current=prior_slot.previous;prior_slot.previous=cur;
   mc_copy(prior_slot.native_hash,prior_slot.previous_native_hash,32);mc_copy(prior_slot.body_hash,prior_slot.previous_body_hash,32);prior_slot.previous_exists=0;prior_slot.phase=SLOT_COMMITTED;
   report("MeritTraits R40: interrupted earlier save found; kept the last committed generation, overwriting the unfinished one.");}
  else{u64 seq=prior_slot.sequence;mc_zero(&prior_slot,sizeof(prior_slot));prior_slot.sequence=seq;status=0;slot_prior_exists=0;
   report("MeritTraits R40: interrupted first save found; overwriting the unfinished generation.");}
 }
 if(status==1&&mc_equal(prior_slot.native_hash,disk.save_hash,32)&&mc_equal(prior_slot.body_hash,disk.body_hash,32)){
  slot_object_name(prior_slot.key,prior_slot.current,path);
  /* Recheck the referenced snapshot before treating this as an idempotent save. */
  if(exact_read(path,&existing_disk,sizeof(existing_disk))!=1||!mc_equal(&existing_disk,&disk,sizeof(disk)))return 0;
  return 1;
 }
 mc_zero(&staged_slot,sizeof(staged_slot));mc_copy(staged_slot.magic,"MT4SLOT1",8);staged_slot.version=1;staged_slot.phase=SLOT_INTENT;
 mc_copy(staged_slot.key,save_slot_key,32);mc_copy(staged_slot.account_tag,save_account_tag,16);
 staged_slot.current=status==1?1-prior_slot.current:0;staged_slot.sequence=status==1?prior_slot.sequence+1:(prior_slot.sequence?prior_slot.sequence+1:1);
 if(!staged_slot.sequence)return 0;
 mc_copy(staged_slot.native_hash,disk.save_hash,32);mc_copy(staged_slot.body_hash,disk.body_hash,32);
 if(status==1){
  staged_slot.previous_exists=1;staged_slot.previous=prior_slot.current;
  mc_copy(staged_slot.previous_native_hash,prior_slot.native_hash,32);mc_copy(staged_slot.previous_body_hash,prior_slot.body_hash,32);
 }
 slot_object_name(save_slot_key,staged_slot.current,path);
 if(!atomic_write(path,&disk,sizeof(disk))||!slot_write_ref(&staged_slot))return 0;
 slot_journal_active=1;return 1;
}
static int slot_prepare(const u8*bytes,u32 size){
 int result;AcquireSRWLockExclusive(&io_lock);result=slot_prepare_unlocked(bytes,size);ReleaseSRWLockExclusive(&io_lock);return result;
}
static int slot_complete(int native_ok){
 int result=native_ok;
 AcquireSRWLockExclusive(&io_lock);
 if(slot_journal_active){
  if(native_ok){staged_slot.phase=SLOT_COMMITTED;result=slot_write_ref(&staged_slot);}
  /* Do not erase INTENT after a failed native write: failure may have followed
   * a partial write. Recovery must inspect actual native bytes, not assume. */
  if(!result)report(native_ok?"MeritTraits: native save wrote successfully but reference commit failed; BOTH companion generations retained.":"MeritTraits: native save reported failure; companion journal and previous generation retained for checked recovery.");
  slot_journal_active=0;
 }
 ReleaseSRWLockExclusive(&io_lock);return result;
}
static int slot_load_snapshot(const SlotRef*r,u32 generation,const u8*h,const u8*body){
 u16 path[1024];slot_object_name(r->key,generation,path);
 if(exact_read(path,&disk,sizeof(disk))!=1||!mc_equal(disk.body_hash,body,32))return -1;
 return validate_loaded_disk(h,sizeof(disk));
}
static void slot_promote_previous(SlotRef*r){
 u8 h[32];u32 current=r->current;
 mc_copy(h,r->native_hash,32);mc_copy(r->native_hash,r->previous_native_hash,32);mc_copy(r->previous_native_hash,h,32);
 mc_copy(h,r->body_hash,32);mc_copy(r->body_hash,r->previous_body_hash,32);mc_copy(r->previous_body_hash,h,32);
 r->current=r->previous;r->previous=current;r->phase=SLOT_COMMITTED;
}
static int slot_choose(SlotRef*r,const u8*h,int recover){
 int current=mc_equal(r->native_hash,h,32),previous=r->previous_exists&&mc_equal(r->previous_native_hash,h,32);
 if(!current&&!previous)return -1;
 if(r->phase==SLOT_INTENT){
  if(!r->previous_exists&&!current)return -1;
  if(current&&previous&&!mc_equal(r->body_hash,r->previous_body_hash,32)){
   /* R40 A12: identical native bytes, different roll state -> use the last committed generation. */
   report("MeritTraits R40: interrupted save with identical native bytes; using the last committed generation.");current=0;
  }
 }
 int change=!current||r->phase==SLOT_INTENT;
 if(!current)slot_promote_previous(r);else if(r->phase==SLOT_INTENT)r->phase=SLOT_COMMITTED;
 /* Validate data before committing recovery: a corrupt candidate must not
  * replace the only durable description of the interrupted transaction. */
 if(slot_load_snapshot(r,r->current,h,r->body_hash)!=1)return -1;
 if(change&&recover&&!slot_write_ref(r))return -1;return 1;
}
/* Filename changes / copied saves are resolved only when every retained matching
 * generation in the same Steam account agrees. Never pick the newest timestamp. */
static int slot_find_copy(const u8*h){
 WinFindDataW data;HANDLE find;u16 pattern[1024],path[1024];SlotRef candidate,chosen;u8 chosen_body[32];u32 count=0,found=0,generation=0;int ambiguous=0;
 mc_copy(pattern,state_path,(state_path_length+1)*2);append(pattern,1024,L"*.mt4ref");
 find=FindFirstFileW(pattern,&data);if(find==INVALID_HANDLE)return GetLastError()==2||GetLastError()==18?0:-1;
 do{
  if(++count>SLOT_SCAN_LIMIT){ambiguous=1;break;}
  if(data.attributes&0x10)continue;
  mc_copy(path,state_path,(state_path_length+1)*2);if(!append(path,1024,data.name)){ambiguous=1;break;}
  if(exact_read(path,&candidate,sizeof(candidate))!=1||!slot_ref_valid(&candidate,0,0)){ambiguous=1;break;}
  if(!mc_equal(candidate.account_tag,load_account_tag,16))continue;
  for(u32 j=0;j<2;j++){
   if(j&&!candidate.previous_exists)continue;
   const u8*n=j?candidate.previous_native_hash:candidate.native_hash,*body=j?candidate.previous_body_hash:candidate.body_hash;
   if(!mc_equal(n,h,32))continue;
   if(candidate.phase==SLOT_INTENT){ambiguous=1;break;}
   if(found&&!mc_equal(chosen_body,body,32)){ambiguous=1;break;}
   if(!found){chosen=candidate;mc_copy(chosen_body,body,32);generation=j?candidate.previous:candidate.current;found=1;}
  }
  if(ambiguous)break;
 }while(FindNextFileW(find,&data));
 u32 error=GetLastError();FindClose(find);
 if(ambiguous||(error!=18&&error!=0)){report("MeritTraits: copied-save metadata is ambiguous/incomplete; refusing an inferred reroll state.");return -1;}
 if(!found)return 0;
 if(slot_load_snapshot(&chosen,generation,h,chosen_body)!=1)return -1;
 /* Pin a copy under its actual new filename without mutating the game save. */
 SlotRef alias;mc_zero(&alias,sizeof(alias));mc_copy(alias.magic,"MT4SLOT1",8);alias.version=1;alias.sequence=1;
 mc_copy(alias.key,load_slot_key,32);mc_copy(alias.account_tag,load_account_tag,16);mc_copy(alias.native_hash,h,32);mc_copy(alias.body_hash,chosen_body,32);
 slot_object_name(alias.key,0,path);if(!atomic_write(path,&disk,sizeof(disk))||!slot_write_ref(&alias))return -1;
 report("MeritTraits: renamed/copied save matched a unique companion state; new filename binding created.");return 1;
}
static int read_slot_unlocked(const u8*h){
 SlotRef r;int result=slot_read_ref(load_slot_key,load_account_tag,&r);
 if(result==1)return slot_choose(&r,h,1);
 if(result<0)return -1;
 return slot_find_copy(h);
}
