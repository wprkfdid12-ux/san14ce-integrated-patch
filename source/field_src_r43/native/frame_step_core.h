#pragma once
// The native 37a720 path rounds evaluated frame time and treats any rounding
// down as a loop. Preserve the actual evaluated float for private actors.
namespace FrameStep {
struct Result { float next,period; bool valid,wrapped; };
inline Result advance(float previous,float evaluated_seconds,float clip_rate,float clock_rate) {
    Result r={previous,0,false,false};
    if(!__builtin_isfinite(previous)||previous<0 || !__builtin_isfinite(evaluated_seconds)||evaluated_seconds<0 ||
       !__builtin_isfinite(clip_rate)||clip_rate<=0||clip_rate>1000 ||
       !__builtin_isfinite(clock_rate)||clock_rate<=0)return r;
    float evaluated=evaluated_seconds*clip_rate,step=clip_rate/clock_rate;
    if(!__builtin_isfinite(evaluated)||!__builtin_isfinite(step)||step<0||step>clip_rate*0.25f)return r;
    float magnitude=previous>evaluated?previous:evaluated;
    // Account only for float round-trip error, never half a frame.
    float tolerance=(magnitude>1?magnitude:1)*0.00000095367431640625f;
    if(__builtin_fabsf(evaluated-previous)<=tolerance)evaluated=previous;
    r.wrapped=evaluated<previous;
    if(r.wrapped)r.period=previous-evaluated;
    r.next=evaluated+step;
    r.valid=__builtin_isfinite(r.next)&&r.next>=0;
    return r;
}
}
