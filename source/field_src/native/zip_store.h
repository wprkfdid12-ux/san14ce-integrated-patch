#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <vector>
#include <string>

/* Tiny ZIP_STORED writer for two bounded diagnostic logs. No shell, PowerShell,
   compression dependency or file dialog is needed on the user's machine. */
class FLZip {
    struct Entry { std::string name; uint32_t crc,size,offset; };
    std::vector<Entry> entries;
    static void u16(std::vector<unsigned char> &b,uint16_t x) {
        b.push_back((unsigned char)x); b.push_back((unsigned char)(x>>8));
    }
    static void u32(std::vector<unsigned char> &b,uint32_t x) { u16(b,(uint16_t)x); u16(b,(uint16_t)(x>>16)); }
    static uint32_t crc32(const unsigned char *p,size_t n) {
        uint32_t c=0xffffffffU;
        for (size_t i=0;i<n;++i) {
            c^=p[i]; for (unsigned k=0;k<8;++k) c=(c>>1)^(0xedb88320U & (0U-(c&1U)));
        }
        return c^0xffffffffU;
    }
public:
    std::vector<unsigned char> bytes;
    void add(const char *name,const unsigned char *data,size_t size) {
        Entry e={name,crc32(data,size),(uint32_t)size,(uint32_t)bytes.size()};
        uint16_t length=(uint16_t)strlen(name);
        u32(bytes,0x04034b50); u16(bytes,20); u16(bytes,0); u16(bytes,0);
        u16(bytes,0); u16(bytes,0x21); /* DOS date 1980-01-01, stable metadata. */
        u32(bytes,e.crc); u32(bytes,e.size); u32(bytes,e.size); u16(bytes,length); u16(bytes,0);
        bytes.insert(bytes.end(),name,name+length);
        if (size) bytes.insert(bytes.end(),data,data+size);
        entries.push_back(e);
    }
    void finish() {
        uint32_t start=(uint32_t)bytes.size();
        for (const Entry &e:entries) {
            uint16_t length=(uint16_t)e.name.size();
            u32(bytes,0x02014b50); u16(bytes,20); u16(bytes,20); u16(bytes,0); u16(bytes,0);
            u16(bytes,0); u16(bytes,0x21); u32(bytes,e.crc); u32(bytes,e.size); u32(bytes,e.size);
            u16(bytes,length); u16(bytes,0); u16(bytes,0); u16(bytes,0); u16(bytes,0); u32(bytes,0); u32(bytes,e.offset);
            bytes.insert(bytes.end(),e.name.begin(),e.name.end());
        }
        uint32_t size=(uint32_t)bytes.size()-start;
        u32(bytes,0x06054b50); u16(bytes,0); u16(bytes,0); u16(bytes,(uint16_t)entries.size());
        u16(bytes,(uint16_t)entries.size()); u32(bytes,size); u32(bytes,start); u16(bytes,0);
    }
};
