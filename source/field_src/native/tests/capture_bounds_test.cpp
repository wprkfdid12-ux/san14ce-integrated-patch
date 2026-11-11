#include "../capture_bounds.h"
#include <assert.h>
#include <stdio.h>
int main() {
    FLCaptureRect r;
    assert(fl_capture_rect(100,100,1280,720,0,0,1920,1080,r));
    assert(r.x==100 && r.y==100 && r.width==1280 && r.height==720 && !r.cropped);
    puts("PASS fully visible game client preserves its exact bounds");
    assert(fl_capture_rect(-100,30,2200,1200,0,0,1920,1080,r));
    assert(r.x==0 && r.y==30 && r.width==1920 && r.height==1050 && r.cropped);
    puts("PASS partially offscreen game captures only the visible client intersection");
    assert(fl_capture_rect(-1800,-900,1280,720,-1920,-1080,3840,2160,r));
    assert(r.x==-1800 && r.y==-900 && !r.cropped);
    puts("PASS negative coordinates on another monitor remain valid");
    assert(!fl_capture_rect(1920,0,800,600,0,0,1920,1080,r));
    assert(!fl_capture_rect(0,0,0,600,0,0,1920,1080,r));
    assert(!fl_capture_rect(1900,0,800,600,0,0,1920,1080,r));
    puts("PASS absent or too-small visible client cannot capture unrelated desktop");
    assert(!fl_capture_rect(INT32_MAX-10,0,200,600,0,0,1920,1080,r));
    assert(!fl_capture_rect(0,0,7680,4320,0,0,7680,4320,r));
    assert(fl_capture_rect(-1,0,INT32_MAX,1080,0,0,1920,1080,r));
    assert(r.x==0 && r.width==1920 && r.cropped);
    puts("PASS extreme coordinates cannot overflow or exceed the image allocation limit");
}
