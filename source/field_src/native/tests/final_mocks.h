#pragma once
static unsigned char pair_markers[501][16],battle_pairs[501][64],battle_nodes[501][16];
static void *battle_table[501];
static bool bone_available;
static float bone_position[3];
static int native_valid(void *p) { return p && at<uint8_t>(p,0x10); }
static bool bone_transform(void *,int id,void *out) { assert(id==1); if(!bone_available)return false;float scale[3]={3,3,3};memcpy(out,scale,12);memcpy((char *)out+0x1c,bone_position,12);return true; }
static void set_position(void *m,void *p) { memcpy((char *)m+0x18,p,12); }
static void *info_destroy(void *p,unsigned flags) { assert(!state_lock.depth && flags==1);return p; }
static unsigned ui_size_writes,ui_position_writes,ui_queries;
static InfoPoint *ui_query_size(void *repo,InfoPoint *out,int id) {++ui_queries;void *o=at<void *>(at<void *>(repo,8),size_t(id)*8);*out={at<int16_t>(o,0xf0),at<int16_t>(o,0xf2)};return out;}
static InfoPoint *ui_query_position(void *repo,InfoPoint *out,int id) {++ui_queries;void *o=at<void *>(at<void *>(repo,8),size_t(id)*8);void *t=at<void *>(o,0xe8);*out={int(at<float>(t,0x30)),int(at<float>(t,0x34))};return out;}
static void ui_size(void *o,int x,int y) {++ui_size_writes;at<int16_t>(o,0xf0)=x;at<int16_t>(o,0xf2)=y;}
static void ui_position(void *o,int x,int y) {++ui_position_writes;void *t=at<void *>(o,0xe8);at<float>(t,0x30)=float(x);at<float>(t,0x34)=float(y);}
static void ambient_source(int sound,unsigned unit=0) {
 static unsigned char node[0x30];memset(node,0,sizeof node);
 at<void *>(node,0)=armies[unit];at<int>(node,8)=1;at<int>(node,0xc)=sound;at<int>(node,0x10)=-1;
 ambient_start_cursor=node;
 if(ambient_level_owner!=current_sound_controller()) { memset(ambient_levels,0,sizeof ambient_levels);ambient_level_owner=current_sound_controller(); }
}
