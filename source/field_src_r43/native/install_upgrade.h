#pragma once
#include "install_transaction.h"
namespace fl_install {
// A durable journal and exact hashes allow resuming every interruption. The
// manifest is replaced last: automatic loading cannot accept a partial runtime.
template<class FS> Result upgrade(FS &fs) {
    if(fs.running())return running;
    if(!fs.validate())return changed;
    if(!fs.stage())return failed;
    for(unsigned i:{0u,1u,3u,2u}) {
        if(fs.running())return running;
        if(!fs.commit(i))return failed;
    }
    return fs.finish()?installed:failed;
}
}
