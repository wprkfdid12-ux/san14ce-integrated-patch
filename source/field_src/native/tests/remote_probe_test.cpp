#include "../remote_probe.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>

int main() {
    static_assert(sizeof(uintptr_t)==8,"x64 diagnostic test");
    auto no_read=[](uintptr_t,void*,size_t) { return false; };
    const unsigned char original[]={0xe8,0x03,0x0e,0xd8,0xff};
    auto r=fl_probe_branch(0x14044a478,original,sizeof original,no_read);
    assert(r.kind && r.resolved && r.target==0x1401cb280);
    puts("PASS original field call resolves the signed backward displacement");

    const unsigned char stub[]={0xff,0x25,0,0,0,0};
    unsigned reads=0;
    auto slot_read=[&](uintptr_t address,void *out,size_t count) {
        ++reads; assert(address==0x180000006 && count==8);
        uintptr_t value=0x7ffc12345678; memcpy(out,&value,8); return true;
    };
    r=fl_probe_branch(0x180000000,stub,sizeof stub,slot_read);
    assert(r.resolved && r.slot==0x180000006 && r.target==0x7ffc12345678 && reads==1);
    r=fl_probe_branch(0x180000000,stub,sizeof stub,no_read);
    assert(r.kind && !r.resolved);
    puts("PASS indirect jump reads only its pointer slot and preserves read failure");

    for (size_t n=0;n<sizeof stub;++n) {
        r=fl_probe_branch(0x180000000,stub,n,slot_read); assert(!r.kind);
    }
    assert(reads==1);
    const unsigned char conditional[]={0x75,0xfe};
    r=fl_probe_branch(0x1000,conditional,sizeof conditional,no_read); assert(!r.kind);
    puts("PASS truncated and unrecognized instructions never trigger pointer reads");

    uintptr_t out=0;
    assert(!fl_relative_address(UINTPTR_MAX-2,5,0,out));
    assert(!fl_relative_address(UINTPTR_MAX-5,5,1,out));
    assert(!fl_relative_address(0,5,INT32_MIN,out));
    assert(fl_relative_address(0x80000000,5,INT32_MIN,out) && out==5);
    puts("PASS relative address arithmetic rejects both overflow directions");

    const unsigned char absolute[]={0x49,0xbb,0x78,0x56,0x34,0x12,0xfc,0x7f,0,0,0x41,0xff,0xe3};
    r=fl_probe_branch(0x1000,absolute,sizeof absolute,no_read);
    assert(r.resolved && r.target==0x7ffc12345678);
    r=fl_probe_branch(0x1000,absolute,sizeof absolute-1,no_read); assert(!r.kind);
    puts("PASS complete absolute jump retains the full 64-bit target");
}
