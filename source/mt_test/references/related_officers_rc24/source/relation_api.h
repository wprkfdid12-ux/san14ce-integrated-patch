#ifndef RELATION_API_H
#define RELATION_API_H
#define REL_ABI 0x00020001u
#define REL_SLOTS 9u
#define REL_WINDOWS 16u
/* Native metadata layout belongs to the CEKO resolver ABI, not a new UI. */
typedef struct { I32 *columns; const U16 *label; I32 resource; U32 pad; } RTab;
typedef struct {
 U32 id,pad0; const U16 *header; U32 width,flags;
 const U16 *(*Text)(void *); U32 align,pad1;
 void *(*Key)(void *,void *); SIZE color,aux,extra;
 U8 native_tail[16]; U32 zero,visible;
} RColumn;
typedef struct {
 U32 cfg_size,cfg_tabs,cfg_columns,cfg_bar,cfg_mode;
 U32 dlg_list,list_bodies,list_tabs,list_columns,list_index,list_count;
 U32 body_rows,body_count,body_tabs,body_columns,body_index,body_checked,event_index;
} RLayout;
typedef struct {
 void (*Attach)(void *,void *);       /* after native config copy/composition, before tab construction */
 SIZE (*Select)(void *,void *);       /* common checkbox/selection event */
 void (*Activate)(void *);            /* immediately before native body activation */
 SIZE (*Destroy)(void *);             /* native list destructor inner call */
 void (*Tick)(void *);               /* after native body input; no global input suppression */
 I32 (*Pin)(void *,void *);          /* consumes only readonly Relation row actions */
} RCallbacks;
typedef struct {
 SIZE (*Select)(void *,void *);
 SIZE (*Destroy)(void *);
} RNative;
typedef struct {
 U32 size,abi; RLayout l;
 RNative Native;
 I32 (*Valid)(void *);
 void *(*Pair)(void *,void *,void *);
 const U16 *(*RelationText)(I32);
 const U16 *(*Name)(void *);
 const U16 *(*Resource)(U32);
 void (*Headers)(void *,I32);
 void (*Rows)(void *);
 const U16 **empty_slot;
 void ***node_table_slot;
 U32 *node_capacity_slot;
 U32 base_column_count,compare_count,tab_resource;
 RColumn native_column;
 I32 (*Install)(const RCallbacks *);
 I32 (*ReadOnly)(void *,void *,void *);
 void *(*HoverPerson)(void *,void *);
} RAPI;
typedef const RAPI *(*RResolve)(const S14_CoreAPI *);
_Static_assert(sizeof(RTab)==24,"tab ABI");
_Static_assert(sizeof(RColumn)==96,"column ABI");
#endif
