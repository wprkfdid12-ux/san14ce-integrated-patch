#pragma once
// File transactions are independent of the GUI; original game/other mods are
// never replaced. Backends must implement create-only writes and exact hashes.
namespace fl_install {
enum Result { installed, already_installed, conflict, failed, running, removed, absent, changed };
template<class FS> Result install(FS &fs) {
    if(fs.running()) return running;
    if(fs.owned()) return !fs.verify()?changed:fs.complete()?already_installed:fs.repair()?installed:failed;
    if(fs.directory_exists()) return conflict;
    int slot=fs.free_slot();if(slot<0)return conflict;
    if(!fs.make_directory())return failed;
    const unsigned order[]={2,0,1,3};
    bool created[4]={};
    for(unsigned step=0;step<4;++step) {
        unsigned i=order[step];
        // Ownership first permits interruption recovery; activation is last.
        if((i==3 && fs.running()) || !fs.create(i,slot)) {
            for(int j=int(step)-1;j>=0;--j)if(created[order[j]])fs.erase(order[j],slot);
            fs.remove_directory();return failed;
        }
        created[i]=true;
    }
    return installed;
}
template<class FS> Result uninstall(FS &fs) {
    if(fs.running())return running;
    if(!fs.owned())return absent;
    if(!fs.verify())return changed;
    int slot=fs.owned_slot();
    // Disable automatic loading before removing the helper, retaining manifest
    // until the other files are gone. A stopped uninstall can safely be retried.
    if(!fs.erase(3,slot))return failed;
    for(unsigned i=0;i<3;++i)if(!fs.erase(i,slot))return failed;
    fs.remove_directory();return removed;
}
}
