#include <initializer_list>
#include "../install_upgrade.h"
#include <cassert>
#include <cstdio>
struct Disk {
 int contents[4]={1,1,1,1},staged[4]={},backup[4]={};bool journal=false,active=false,foreign=false;
 int fail=-1,step=0,game_at=-1;bool mutate(){return step++!=fail;}
 bool running(){return active||(game_at>=0&&step>=game_at);}
 bool validate(){if(foreign)return false;for(int x:contents)if(x!=0&&x!=1&&x!=2)return false;return true;}
 bool stage(){if(!journal){if(!mutate())return false;journal=true;}for(unsigned i=0;i<4;++i)if(!staged[i]&&contents[i]!=2){if(!mutate())return false;staged[i]=2;}return true;}
 bool commit(unsigned i){if(contents[i]==2)return true;if(!backup[i]&&contents[i]){if(!mutate())return false;backup[i]=1;}if(!mutate())return false;assert(staged[i]==2);contents[i]=2;staged[i]=0;return true;}
 bool finish(){for(unsigned i=0;i<4;++i){assert(contents[i]==2);if(backup[i]){if(!mutate())return false;backup[i]=0;}if(staged[i]){if(!mutate())return false;staged[i]=0;}}if(!mutate())return false;journal=false;return true;}
};
int main(){using namespace fl_install;unsigned cases=0;
 for(int cut=-1;cut<20;++cut){Disk d;d.fail=cut;auto first=upgrade(d);if(first!=installed){assert(first==failed);d.fail=-1;assert(upgrade(d)==installed);}for(int x:d.contents)assert(x==2);assert(!d.journal);++cases;}
 for(int cut=0;cut<15;++cut){Disk d;d.game_at=cut;auto result=upgrade(d);assert(result==running||result==installed);d.game_at=-1;assert(upgrade(d)==installed);++cases;}
 Disk d;d.foreign=true;assert(upgrade(d)==changed&&!d.journal&&d.step==0);d.foreign=false;d.contents[0]=9;assert(upgrade(d)==changed&&d.step==0);
 printf("PASS %u interrupted upgrade and live-game cases resume with manifest last; foreign files rejected\n",cases);
}
