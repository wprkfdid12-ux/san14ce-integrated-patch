#pragma once
#include <cstring>
namespace fl_install {
// The first two indices and on-disk format remain compatible with R23.
constexpr unsigned slot_count=4;
constexpr unsigned directory_count=9;
static const wchar_t *const slot_names[]={L"version.dll",L"dinput8.dll",L"dxgi.dll",L"d3d9.dll"};
static const wchar_t *const directory_names[]={L"FieldLeader3D",L"FieldLeader3D_Auto",L"FieldLeader3D_Auto_2",L"FieldLeader3D_Auto_3",L"FieldLeader3D_Auto_4",L"FieldLeader3D_Auto_5",L"FieldLeader3D_Auto_6",L"FieldLeader3D_Auto_7",L"FieldLeader3D_Auto_8"};
struct Manifest { char magic[16]; unsigned revision,slot; unsigned char runtime[32],proxy[32],manager[32]; };
static const char manifest_magic[16]="FieldLeader3D23";
inline bool valid_manifest(const Manifest &m) {
 return !std::memcmp(m.magic,manifest_magic,16)&&m.revision==23&&m.slot<slot_count;
}
template<class Probe> int choose_directory(Probe &probe) {
 // Recover/manage an installation before considering a vacant directory.
 for(unsigned i=0;i<directory_count;++i)if(probe.owned_at(i))return int(i);
 // Never use the old, generic test-folder name for a new installation.
 for(unsigned i=1;i<directory_count;++i)if(!probe.exists_at(i))return int(i);
 return -1;
}
}
