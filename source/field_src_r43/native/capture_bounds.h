#pragma once
#include <stdint.h>

struct FLCaptureRect { int x,y,width,height; bool cropped; };
/* Work in signed 64-bit coordinates: monitors can lie left/above the primary
   monitor, and an oversized game window may extend beyond the visible screen. */
static inline bool fl_capture_rect(int x,int y,int width,int height,
    int vx,int vy,int vw,int vh,FLCaptureRect &out) {
    out={};
    if(width<=0 || height<=0 || vw<=0 || vh<=0) return false;
    int64_t left=x>vx?x:vx, top=y>vy?y:vy;
    int64_t right=(int64_t)x+width, bottom=(int64_t)y+height;
    if(right>(int64_t)vx+vw) right=(int64_t)vx+vw;
    if(bottom>(int64_t)vy+vh) bottom=(int64_t)vy+vh;
    int64_t w=right-left,h=bottom-top;
    if(w<64 || h<64 || w>7680 || h>4320 || w*h>16000000 ||
        left<INT32_MIN || left>INT32_MAX || top<INT32_MIN || top>INT32_MAX) return false;
    out={(int)left,(int)top,(int)w,(int)h,left!=x || top!=y || w!=width || h!=height};
    return true;
}
