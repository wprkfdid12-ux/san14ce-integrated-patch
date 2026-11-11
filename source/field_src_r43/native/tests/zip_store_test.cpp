#include "../zip_store.h"
#include <stdio.h>
int main(int argc,char **argv) {
    if (argc!=2) return 2;
    const unsigned char message[]={0,1,2,0xff,'\n'};
    FLZip z; z.add("diagnostic.bin",message,sizeof message); z.add("empty.log",NULL,0);
    for(unsigned i=0;i<12;++i) {
        char name[80]; snprintf(name,sizeof name,"Slot_%u_%s.bin",i/3,i%3==0?"body":i%3==1?"horse":"weapon");
        z.add(name,message,sizeof message); memset(name,'X',sizeof name);
    }
    z.finish();
    FILE *f=fopen(argv[1],"wb"); if (!f) return 3;
    bool ok=fwrite(z.bytes.data(),1,z.bytes.size(),f)==z.bytes.size(); fclose(f); return ok?0:4;
}
