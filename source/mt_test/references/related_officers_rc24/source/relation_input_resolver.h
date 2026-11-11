/* CEKO-specific native input adapter, isolated from feature state/behaviour.
   All function addresses are obtained from validated native code references.
   No global mouse hook, no input flag writes, no check-registry writes. */
static U8 *(*rel_get_app)(void);
static U8 *(*rel_get_input)(void);
static U8 *(*rel_hover_target)(void *);
static I32 (*rel_row_index)(void *,I32);
static U8 *(*rel_row_widget)(void *,I32);
static I32 ri_read(const void *p,SIZE n){return p && host->Readable(p,n);}
static U8 *ri_ptr(const void *p,U32 off){return *(U8 *const *)((const U8 *)p+off);}
static I32 ri_int(const void *p,U32 off){return *(const I32 *)((const U8 *)p+off);}
static I32 read_only_mode(void *dlg,void *list,void *body){
 U8 *cfg;
 if(!ri_read(dlg,0x1b0) || !ri_read(list,0x230) || !ri_read(body,0x1c0))return 0;
 cfg=ri_ptr(dlg,0x1a8);
 if(!ri_read(cfg,0x20) || ri_ptr(dlg,0x180)!=list || ri_ptr(body,8)!=list)return 0;
 if(ri_ptr(cfg,8)!=ri_ptr(list,0x150) || ri_ptr(body,0x1a8)!=ri_ptr(list,0x150) ||
    ri_ptr(body,0x1b0)!=ri_ptr(list,0x158))return 0;
 /* Verified native enum propagation, NOT an empty current check list test. */
 return ri_int(cfg,0x18)==1 && ri_int(list,0x228)==1 && ri_int(body,0x170)==1;
}
static I32 ri_rect(U8 *w,I32 *out){
 U32 depth=0;U8 *p=w;long long x=0,y=0;
 if(!ri_read(w,0x44) || ri_int(w,0x28)<=0 || ri_int(w,0x2c)<=0)return 0;
 out[2]=ri_int(w,0x28);out[3]=ri_int(w,0x2c);
 while(p && depth++<30){
  if(!ri_read(p,0x44) || (ri_int(p,0x40)&3)!=3)return 0;
  x+=ri_int(p,0x20);y+=ri_int(p,0x24);p=ri_ptr(p,8);
 }
 if(p || x<-2147483648LL || x>2147483647LL || y<-2147483648LL || y>2147483647LL)return 0;
 out[0]=(I32)x;out[1]=(I32)y;return 1;
}
static I32 ri_contains(U8 *w,const I32 *point,I32 xonly){
 I32 rect[4];long long x,y;if(!ri_rect(w,rect))return 0;
 x=(long long)point[0]-rect[0];y=(long long)point[1]-rect[1];
 return x>=0 && x<rect[2] && (xonly || (y>=0 && y<rect[3]));
}
static void *hover_person(void *body,void *list){
 U8 *app,*input,*target,*resource,*header,*person;U8 **headers,**end,**rows,**bodies;
 I32 point[2],tab,count,visible,index,nrows;SIZE n;
 if(!ri_read(body,0x1c0) || !ri_read(list,0x230) || ri_ptr(body,8)!=list)return 0;
 tab=ri_int(list,0x208);count=ri_int(list,0x20c);bodies=(U8 **)ri_ptr(list,0x130);
 if(tab<0 || count<1 || count>64 || tab>=count || !ri_read(bodies,count*8) || bodies[tab]!=body)return 0;
 app=rel_get_app();input=rel_get_input();
 if(!ri_read(app,0x190) || !ri_read(input,0x3c))return 0;
 target=rel_hover_target(app);
 if(!ri_read(target,0x84) || ri_ptr(target,0x48)!=body)return 0;
 point[0]=ri_int(input,0x34);point[1]=ri_int(input,0x38);
 if(!ri_contains(list,point,0) || !ri_contains(body,point,0) || !ri_contains(target,point,0))return 0;
 /* First native displayed column is the officer name. Require the live name
    header's horizontal span, not the whole row, for hover-only previews. */
 headers=(U8 **)ri_ptr(list,0x210);end=(U8 **)ri_ptr(list,0x218);
 if(!headers || (SIZE)end<(SIZE)headers || ((SIZE)end-(SIZE)headers)%8)return 0;
 n=((SIZE)end-(SIZE)headers)/8;
 if(n<2 || n>1025 || !ri_read(headers,n*8))return 0;
 header=headers[1];
 if(!ri_read(header,0x84) || ri_int(header,0x80)!=0 || !ri_contains(header,point,1))return 0;
 resource=ri_ptr(body,0x138);
 if(!ri_read(resource,0x28) || !ri_read(resource=ri_ptr(resource,0x20),0x28) ||
    !ri_read(resource=ri_ptr(resource,0x20),0x78))return 0;
 visible=ri_int(target,0x80);
 if(visible<0 || visible>=ri_int(resource,0x58))return 0;
 if(rel_row_widget(resource,visible)!=target)return 0;
 index=rel_row_index(resource,visible);nrows=ri_int(body,0x1a0);rows=(U8 **)ri_ptr(body,0x198);
 if(index<0 || nrows<1 || nrows>6000 || index>=nrows || !ri_read(rows,(SIZE)nrows*8))return 0;
 person=rows[index];
 if(!ri_read(person,8) || !ri_read(ri_ptr(person,0),32) || !api.Valid(person))return 0;
 return person;
}
