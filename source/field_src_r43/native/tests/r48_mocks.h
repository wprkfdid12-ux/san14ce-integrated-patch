#pragma once
static float r48_evaluated_seconds,r48_input_seconds;
static bool r48_evaluator_writes=true;
static unsigned char r48_evaluator_result=1;
static unsigned r48_evaluations,r48_commits;
static void *r48_evaluation_model;
static unsigned char r48_evaluate(void *model,void *frame,void *e,int action,float time,void *options,float *out) {
    assert(model==r48_evaluation_model && frame==frame_arg && e==engine);
    assert(action==55 && options==app && time==r48_input_seconds);
    ++r48_evaluations;
    if(r48_evaluator_writes && out)*out=r48_evaluated_seconds;
    return r48_evaluator_result;
}
static void r48_commit(void *setting,float frame) { ++r48_commits;at<float>(setting,0x24)=frame; }
