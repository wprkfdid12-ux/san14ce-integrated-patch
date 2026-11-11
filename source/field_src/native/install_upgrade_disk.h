#pragma once
// Included after Disk. Never touches files outside the owned installation and
// its owned proxy. Journal/staging name collisions are preserved, not replaced.
struct UpgradeDisk {
    Disk &disk;
    struct Journal { char magic[16]; Manifest before,after; } journal={};
    wstring journal_path;
    unsigned char before_hash[4][32]={},after_hash[4][32]={};
    UpgradeDisk(Disk &d):disk(d),journal_path(d.dir+L"\\upgrade.dat"){}
    wstring auxiliary(unsigned i,bool backup) {return disk.dir+L"\\r24_"+std::to_wstring(i)+(backup?L".old":L".new");}
    wstring target(unsigned i){return disk.path(i,journal.after.slot);}
    bool running(){return disk.running();}
    bool digest(const void *p,unsigned n,unsigned char out[32]) {
        BCRYPT_ALG_HANDLE a=NULL;BCRYPT_HASH_HANDLE h=NULL;bool ok=false;
        if(BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,NULL,0)>=0&&BCryptCreateHash(a,&h,NULL,0,NULL,0,0)>=0)
            ok=BCryptHashData(h,(PUCHAR)p,n,0)>=0&&BCryptFinishHash(h,out,32,0)>=0;
        if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);return ok;
    }
    bool validate() {
        if(!disk.owned()||!fl_install::plain_directory(disk.dir))return false;
        Manifest desired=disk.manifest;memcpy(desired.runtime,fl_runtime_sha256,32);
        memcpy(desired.proxy,fl_proxy_payloads[desired.slot].sha256,32);
        if(!hash_file(self_path,desired.manager))return false;
        if(exists(journal_path)) {
            if(!regular(journal_path))return false;
            HANDLE f=CreateFileW(journal_path.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
            if(f==INVALID_HANDLE_VALUE)return false;
            DWORD n=0;LARGE_INTEGER size={};bool ok=GetFileSizeEx(f,&size)&&size.QuadPart==sizeof journal&&ReadFile(f,&journal,sizeof journal,&n,NULL)&&n==sizeof journal;
            CloseHandle(f);
            static const char magic[16]="FL3DUpdate24";
            if(!ok||memcmp(journal.magic,magic,16)||!fl_install::valid_manifest(journal.before)||
               !fl_install::valid_manifest(journal.after)||journal.before.slot!=journal.after.slot||
               memcmp(&journal.after,&desired,sizeof desired))return false;
        } else {
            if(!disk.verify())return false;
            memcpy(journal.magic,"FL3DUpdate24",12);journal.before=disk.manifest;journal.after=desired;
            // Reserved names must be vacant before creating a new journal.
            for(unsigned i=0;i<4;++i)if(exists(auxiliary(i,false))||exists(auxiliary(i,true)))return false;
        }
        for(unsigned i=0;i<4;++i) {
            const unsigned char *a=i==0?journal.before.runtime:i==1?journal.before.manager:journal.before.proxy;
            const unsigned char *b=i==0?journal.after.runtime:i==1?journal.after.manager:journal.after.proxy;
            if(i==2) {if(!digest(&journal.before,sizeof(Manifest),before_hash[i])||!digest(&journal.after,sizeof(Manifest),after_hash[i]))return false;}
            else {memcpy(before_hash[i],a,32);memcpy(after_hash[i],b,32);}
            if(exists(target(i))&&!matches(target(i),before_hash[i])&&!matches(target(i),after_hash[i]))return false;
            if(exists(auxiliary(i,false))&&!matches(auxiliary(i,false),after_hash[i]))return false;
            if(exists(auxiliary(i,true))&&!matches(auxiliary(i,true),before_hash[i]))return false;
        }
        return true;
    }
    bool stage() {
        if(!exists(journal_path)&&!write_new(journal_path,(unsigned char *)&journal,sizeof journal))return false;
        for(unsigned i=0;i<4;++i) {
            if(matches(target(i),after_hash[i])||matches(auxiliary(i,false),after_hash[i]))continue;
            bool ok=false;wstring p=auxiliary(i,false);
            if(i==0)ok=write_new(p,fl_runtime,sizeof fl_runtime);
            if(i==1)ok=CopyFileW(self_path.c_str(),p.c_str(),TRUE)!=0;
            if(i==2)ok=write_new(p,(unsigned char *)&journal.after,sizeof(Manifest));
            if(i==3){const auto &x=fl_proxy_payloads[journal.after.slot];ok=write_new(p,x.bytes,x.size);}
            if(!ok||!matches(p,after_hash[i]))return false;
        }
        return true;
    }
    bool commit(unsigned i) {
        if(matches(target(i),after_hash[i]))return true;
        if(exists(target(i))) {
            if(!matches(target(i),before_hash[i]))return false;
            if(!exists(auxiliary(i,true))&&!CopyFileW(target(i).c_str(),auxiliary(i,true).c_str(),TRUE))return false;
            if(!matches(auxiliary(i,true),before_hash[i]))return false;
        }
        if(!matches(auxiliary(i,false),after_hash[i]))return false;
        if(!MoveFileExW(auxiliary(i,false).c_str(),target(i).c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return false;
        return matches(target(i),after_hash[i]);
    }
    bool finish() {
        for(unsigned i=0;i<4;++i)if(!matches(target(i),after_hash[i]))return false;
        for(unsigned i=0;i<4;++i)for(bool backup:{false,true}) {
            wstring p=auxiliary(i,backup);
            if(exists(p)&&(!matches(p,backup?before_hash[i]:after_hash[i])||!DeleteFileW(p.c_str())))return false;
        }
        return DeleteFileW(journal_path.c_str())!=0;
    }
};
