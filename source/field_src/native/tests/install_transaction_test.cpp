#include "../install_transaction.h"
#include "../install_contract.h"
#include <cassert>
#include <vector>
#include <cstdio>
struct MemoryFS {
 bool active=false,dir=false,foreign=false,owned_flag=false,dirty=false;
 bool files[4]={},busy[fl_install::slot_count]={};int slot=-1,fail_write=-1,fail_erase=-1;unsigned checks=0,start_game_at=99;
 std::vector<unsigned> writes,deletes;
 bool running(){return active||++checks==start_game_at;}
 bool owned(){return owned_flag;}
 bool verify(){return !dirty;}
 bool complete(){for(bool v:files)if(!v)return false;return true;}
 bool repair(){for(unsigned i:{0u,1u,3u})if(!files[i]&&!create(i,slot))return false;return true;}
 bool directory_exists(){return dir;}
 int free_slot(){for(unsigned i=0;i<fl_install::slot_count;++i)if(!busy[i])return int(i);return -1;}
 bool make_directory(){return dir=true;}
 bool create(unsigned i,int s){if(int(i)==fail_write)return false;assert(!files[i]);slot=s;files[i]=true;writes.push_back(i);if(i==2)owned_flag=true;if(i==3){assert(!busy[s]);busy[s]=true;}return true;}
 bool erase(unsigned i,int s){if(int(i)==fail_erase)return false;if(files[i]){deletes.push_back(i);files[i]=false;if(i==2)owned_flag=false;if(i==3)busy[s]=false;}return true;}
 void remove_directory(){for(bool v:files)if(v)return;dir=false;}
 int owned_slot(){return slot;}
};
int main(){using namespace fl_install;
 for(int fail=-1;fail<4;++fail){MemoryFS fs;fs.fail_write=fail;auto r=install(fs);
  if(fail<0){assert(r==installed&&fs.complete());assert((fs.writes==std::vector<unsigned>{2,0,1,3}));assert(install(fs)==already_installed);assert(uninstall(fs)==removed&&!fs.dir);assert((fs.deletes==std::vector<unsigned>{3,0,1,2}));}
  else {assert(r==failed&&!fs.dir);for(bool v:fs.files)assert(!v);}
 }
 puts("PASS install activates last and every failed write rolls back only its own created files");
 for(unsigned occupied=0;occupied<(1u<<slot_count);++occupied){MemoryFS fs;for(unsigned i=0;i<slot_count;++i)fs.busy[i]=(occupied>>i)&1;auto r=install(fs);
  if(occupied==(1u<<slot_count)-1){assert(r==conflict&&!fs.dir&&fs.writes.empty());continue;}
  unsigned expected=0;while(occupied&(1u<<expected))++expected;
  assert(r==installed&&fs.slot==int(expected));assert(uninstall(fs)==removed);
  for(unsigned i=0;i<slot_count;++i)assert(fs.busy[i]==bool((occupied>>i)&1));
 }
 MemoryFS folder;folder.dir=true;assert(install(folder)==conflict&&folder.writes.empty());
 puts("PASS all 16 four-slot occupancy combinations preserve foreign proxies through install and uninstall");
 MemoryFS fs;fs.active=true;assert(install(fs)==running&&fs.writes.empty());fs.active=false;fs.checks=0;fs.start_game_at=2;assert(install(fs)==failed&&!fs.dir);
 fs=MemoryFS{};assert(install(fs)==installed);fs.dirty=true;assert(uninstall(fs)==changed&&fs.deletes.empty());assert(install(fs)==changed);fs.dirty=false;fs.active=true;assert(uninstall(fs)==running);
 puts("PASS live game checks and changed-file hashes block installation or deletion");
 for(int stop:{0,1,2}) {fs=MemoryFS{};assert(install(fs)==installed);fs.fail_erase=stop;assert(uninstall(fs)==failed&&fs.owned());fs.fail_erase=-1;assert(uninstall(fs)==removed&&!fs.dir);}
 fs=MemoryFS{};fs.dir=true;fs.slot=0;fs.create(2,0);assert(install(fs)==installed&&fs.complete());
 puts("PASS interruption after ownership creation repairs safely and a stopped uninstall can be resumed");
 for(unsigned occupied=0;occupied<(1u<<slot_count)-1;++occupied)for(int fail=0;fail<4;++fail){
  fs=MemoryFS{};fs.fail_write=fail;for(unsigned i=0;i<slot_count;++i)fs.busy[i]=(occupied>>i)&1;
  assert(install(fs)==failed&&!fs.dir);for(unsigned i=0;i<slot_count;++i)assert(fs.busy[i]==bool((occupied>>i)&1));
 }
 puts("PASS 60 failed-write cases preserve occupied slots while removing only new files");
 struct Directories {bool present[directory_count]={},owned[directory_count]={};bool owned_at(unsigned i){return owned[i];}bool exists_at(unsigned i){return present[i];}} dirs;
 for(unsigned occupied=0;occupied<(1u<<directory_count);++occupied){
  dirs=Directories{};for(unsigned i=0;i<directory_count;++i)dirs.present[i]=(occupied>>i)&1;
  int expected=-1;for(unsigned i=1;i<directory_count;++i)if(!dirs.present[i]){expected=int(i);break;}
  assert(choose_directory(dirs)==expected);
  for(unsigned recovery=0;recovery<directory_count;++recovery){dirs.owned[recovery]=true;assert(choose_directory(dirs)==int(recovery));dirs.owned[recovery]=false;}
 }
 puts("PASS 512 foreign-folder combinations and 4608 recovery cases keep legacy folders intact and find owned installs first");
 Manifest m={};std::memcpy(m.magic,manifest_magic,16);m.revision=23;
 for(unsigned i=0;i<slot_count;++i){m.slot=i;assert(valid_manifest(m));}
 m.slot=slot_count;assert(!valid_manifest(m));m.slot=0;m.revision=24;assert(!valid_manifest(m));m.revision=23;m.magic[0]='X';assert(!valid_manifest(m));
 puts("PASS legacy manifest format accepts four valid slots and rejects foreign magic/revision/out-of-range indices");
}
