#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* Diagnostic decoding only. These results never authorize a patch or a call.
   Only common unconditional branches and calls are recognized. */
struct FLBranch {
    const char *kind;
    uintptr_t target, slot;
    bool resolved;
};
static inline bool fl_relative_address(uintptr_t pc, size_t length, int32_t delta, uintptr_t &out) {
    if (pc>UINTPTR_MAX-length) return false;
    uintptr_t next=pc+length;
    if (delta>=0) {
        if (next>UINTPTR_MAX-(uint32_t)delta) return false;
        out=next+(uint32_t)delta;
    } else {
        uint64_t magnitude=(uint64_t)(-(int64_t)delta);
        if (next<magnitude) return false;
        out=next-(uintptr_t)magnitude;
    }
    return true;
}
template<class Read>
static FLBranch fl_probe_branch(uintptr_t pc,const unsigned char *b,size_t n,Read read) {
    FLBranch result={};
    if (n>=5 && (b[0]==0xe8 || b[0]==0xe9)) {
        int32_t delta; memcpy(&delta,b+1,4);
        result.kind=b[0]==0xe8?"CALL_REL32":"JMP_REL32";
        result.resolved=fl_relative_address(pc,5,delta,result.target);
    } else if (n>=2 && b[0]==0xeb) {
        result.kind="JMP_REL8";
        result.resolved=fl_relative_address(pc,2,(int8_t)b[1],result.target);
    } else if (n>=6 && b[0]==0xff && (b[1]==0x25 || b[1]==0x15)) {
        int32_t delta; memcpy(&delta,b+2,4);
        result.kind=b[1]==0x25?"JMP_RIP_SLOT":"CALL_RIP_SLOT";
        result.resolved=fl_relative_address(pc,6,delta,result.slot) &&
                        read(result.slot,&result.target,sizeof result.target);
    } else if (n>=12 && b[0]==0x48 && b[1]==0xb8 && b[10]==0xff && b[11]==0xe0) {
        result.kind="MOV_RAX_JMP"; memcpy(&result.target,b+2,8); result.resolved=true;
    } else if (n>=13 && b[0]==0x49 && (b[1]==0xba || b[1]==0xbb) && b[10]==0x41 &&
               b[11]==0xff && b[12]==(b[1]==0xba?0xe2:0xe3)) {
        result.kind=b[1]==0xba?"MOV_R10_JMP":"MOV_R11_JMP";
        memcpy(&result.target,b+2,8); result.resolved=true;
    }
    return result;
}
