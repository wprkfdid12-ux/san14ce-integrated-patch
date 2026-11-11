#pragma once
#include "common.h"
extern const S14_CoreAPI*core27;extern U8*game27;extern U32 installed27;extern volatile LONG flags27;
enum Feature27:U32{RemoteRecruit=1,DeployedPosts=2,BattleDeaths=4,ExtendedNames=8};
inline bool enabled27(U32 feature){return (U32(flags27)&installed27&feature)!=0;}
extern "C" int RemoteAllowed();
extern "C" int DeathAllowed(U8*person);
