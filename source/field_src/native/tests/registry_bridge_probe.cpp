#include "../registry_bridge.h"
using Reader=bool (*)(uintptr_t,void *,size_t);
extern "C" {
int registry_installed(Reader read,uintptr_t game,uintptr_t module) {uintptr_t table=0;return fl_registry::installed(read,game,module,table);}
uintptr_t registry_info(Reader read,uintptr_t game,uintptr_t module) {return fl_registry::info_cleanup(read,game,module);}
int registry_guard(uintptr_t game,uintptr_t table,unsigned rva,const unsigned char *expected,const unsigned char *actual,unsigned size) {return fl_registry::code_matches(game,table,rva,expected,actual,size);}
int registry_resolve(Reader read,uintptr_t game,uintptr_t module,uintptr_t table,unsigned id,int *body,int *color) {
 static fl_registry::Snapshot s;
 if(!fl_registry::collect(read,game,module,table,0x277df18,0x14680a0,s))return -1;
 if(id>=fl_registry::officer_count)return -2;
 const auto &r=s.rows[id];if(r.person && r.model>=0)fl_registry::override_model(r.model,*body,*color);
 return 1;
}
}
