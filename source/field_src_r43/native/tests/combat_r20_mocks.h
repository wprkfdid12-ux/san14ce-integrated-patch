#pragma once
static unsigned normal_damage_calls,motion_effect_calls;
static int mock_damage=123;
static float normal_operands[4],mock_factor=1;
static uint32_t expected_effect_hash=23;
static float mock_scores[FL_PROXY_COUNT][2];
static void *normal_source,*normal_target;
static void normal_damage(void *a,float aa,float ad,void *d,float da,float dd,int *out) {
    assert(state_lock.depth==0); ++normal_damage_calls;
    normal_source=a; normal_target=d;
    normal_operands[0]=aa; normal_operands[1]=ad; normal_operands[2]=da; normal_operands[3]=dd;
    if(out)*out=mock_damage;
}
static void *motion_effect(void *a,uint32_t b,void *c,void *d,void *e,uint32_t f,
    float g,float h,float i,float j,float k,float l,float m,float n,float o,float p,float q,float r) {
    assert(a==manager && b==expected_effect_hash && c==app && d==engine && e==context && f==67);
    assert(g==1 && h==2 && i==3 && j==4 && k==5 && l==6 && m==7 && n==8 && o==9 && p==10 && q==11 && r==12);
    ++motion_effect_calls; return callback;
}
static int tactic_kind(void *p,int slot) { assert(slot>=0 && slot<2 && !state_lock.depth); return at<int>(p,slot*4); }
static float battle_power(void *army,void *town,int split,int flag) {
    assert(state_lock.depth==1 && !town && split==1 && flag==1);
    assert(army>=static_cast<void *>(armies) && army<static_cast<void *>(armies+FL_PROXY_COUNT)); return 100;
}
static void pair_attack(void *pair,void *out) {
    assert(state_lock.depth==1); void *ref=at<void *>(pair,0);
    assert(at<int>(ref,4)==0x1b && at<int>(pair,0xc)==0x1b);
    assert(at<uint16_t>(ref,0)>=1 && at<uint16_t>(ref,0)<=500 && at<uint16_t>(pair,8)>=1);
    float *s=(float *)out; assert(s[0]==100 && s[5]==100);
    s[1]=mock_scores[at<uint16_t>(ref,0)-1][0]; s[6]=mock_scores[at<uint16_t>(pair,8)-1][0];
}
static void pair_defense(void *pair,void *out) {
    assert(pair && state_lock.depth==1); float *s=(float *)out; void *ref=at<void *>(pair,0);
    s[2]=mock_scores[at<uint16_t>(ref,0)-1][1]; s[7]=mock_scores[at<uint16_t>(pair,8)-1][1];
}
static float tactic_factor(void *ctx,void *army,void *town) {
    assert(state_lock.depth==1 && at<void *>(ctx,0) && at<void *>(ctx,0x10) && army && !town);
    return mock_factor;
}
