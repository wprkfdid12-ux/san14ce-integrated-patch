#include "../status_snapshot.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static FLStatus live={};
static uint32_t sequence;
static unsigned mode,reads,copies;
static bool read_memory(uintptr_t address,void *out,size_t size) {
    ++reads;
    if(mode==4) return false;
    if(address==0x2000) { assert(size==4); memcpy(out,&sequence,4); return true; }
    assert(address==0x1000 && size==sizeof live);
    memcpy(out,&live,sizeof live); ++copies;
    if(mode==2 || mode==3) {
        sequence+=2; ++live.frame;
        if(mode==2) mode=0;
    }
    return true;
}
static void reset(unsigned how=0) { live={}; live.protocol=FL_PROTOCOL; live.frame=42; sequence=0; mode=how; reads=copies=0; }
int main() {
    FLStatus out={}; reset();
    assert(fl_read_status(read_memory,0x1000,0x2000,out) && out.frame==42 && copies==1);
    puts("PASS stable sequence returns one complete snapshot");
    reset(); sequence=1; out.frame=999;
    assert(!fl_read_status(read_memory,0x1000,0x2000,out) && out.frame==999 && copies==0 && reads==3);
    puts("PASS in-progress writer is rejected within a bounded retry count");
    reset(2); assert(fl_read_status(read_memory,0x1000,0x2000,out) && out.frame==43 && copies==2);
    puts("PASS a write during the read is detected and retried with the new snapshot");
    reset(3); out.frame=999;
    assert(!fl_read_status(read_memory,0x1000,0x2000,out) && out.frame==999 && copies==3);
    puts("PASS continuous changes cannot publish torn counters or overwrite the last valid result");
    reset(); live.protocol=0x464c0002; out.frame=999;
    assert(!fl_read_status(read_memory,0x1000,0x2000,out) && out.frame==999);
    reset(4); assert(!fl_read_status(read_memory,0x1000,0x2000,out) && out.frame==999 && reads==1);
    puts("PASS older protocol and unreadable process memory never become a valid snapshot");
    reset(); assert(fl_read_audit(read_memory,0x1000,0x2000,out) && out.frame==42);
    reset(2); assert(fl_read_audit(read_memory,0x1000,0x2000,out) && out.frame==43 && copies==2);
    puts("PASS audit snapshots use the same coherent publication and retry after a racing write");
    reset(3); out.frame=999;
    assert(!fl_read_audit(read_memory,0x1000,0x2000,out) && out.frame==999 && copies==3);
    reset(); live.protocol=0; assert(!fl_read_audit(read_memory,0x1000,0x2000,out));
    puts("PASS incomplete or continuously changing audit snapshots are never published");
}
