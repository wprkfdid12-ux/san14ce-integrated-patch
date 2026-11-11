#pragma once
#include <stddef.h>
#include "poc_contract.h"

/* The runtime publishes odd/even sequence changes under its state lock.
   Keep the caller's last valid snapshot on failure; never show a half-counted
   pool or reinterpret an older protocol. No remote engine call is involved. */
template<class Reader>
static bool fl_read_status(Reader read,uintptr_t status_address,uintptr_t sequence_address,FLStatus &out) {
    for(unsigned attempt=0;attempt<3;++attempt) {
        uint32_t before=0,after=0;
        FLStatus sample={};
        if (!read(sequence_address,&before,sizeof before)) return false;
        if (before&1u) continue;
        if (!read(status_address,&sample,sizeof sample) || !read(sequence_address,&after,sizeof after)) return false;
        if (before==after && !(after&1u) && sample.protocol==FL_PROTOCOL) {
            out=sample; return true;
        }
    }
    return false;
}

template<class Reader, class Snapshot>
static bool fl_read_audit(Reader read,uintptr_t address,uintptr_t sequence_address,Snapshot &out) {
    for(unsigned attempt=0;attempt<3;++attempt) {
        uint32_t before=0,after=0;
        Snapshot sample={};
        if (!read(sequence_address,&before,sizeof before)) return false;
        if (before&1u) continue;
        if (!read(address,&sample,sizeof sample) || !read(sequence_address,&after,sizeof after)) return false;
        if (before==after && !(after&1u) && sample.protocol==FL_PROTOCOL) { out=sample; return true; }
    }
    return false;
}
