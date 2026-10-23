#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "anchor_config_loader.h"
#include "ui_runtime_binding.h"
#include "mounted_recovery_guard.h"
#include "world_profile_snapshot.h"
#include <unordered_map>
#include <fstream>
#include <string>
#include <cstdio>
#define CHECK(x) do{++checks;if(!(x)){std::printf("failed %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct Fixture {
    std::unordered_map<std::uintptr_t,unsigned char> memory;
    std::wstring path;gears_anchor::Config anchor{};gears_ui::Config ui{};gears_profile::Snapshot profile;bool cached{};
    DWORD text(const wchar_t* section,const wchar_t* key,wchar_t* out,unsigned size){return gears_profile::get(cached?&profile:nullptr,section,key,out,size,path.c_str());}
    bool load(const wchar_t* name,bool cache=true){wchar_t current[MAX_PATH]{};GetCurrentDirectoryW(MAX_PATH,current);path=std::wstring(current)+L"\\"+name+L".ini";
        cached=cache;
        if(cached){HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
            const bool loaded=profile.load(file);if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);if(!loaded)return false;}
        std::ifstream input((std::wstring(current)+L"\\"+name+L".memory").c_str(),std::ios::binary);memory.clear();
        std::uintptr_t address{};unsigned char byte{};while(input.read(reinterpret_cast<char*>(&address),8)&&input.read(reinterpret_cast<char*>(&byte),1))memory[address]=byte;
        auto get=[&](const wchar_t* key,wchar_t* out,unsigned size){return text(L"AnchorObserver",key,out,size);};
        if(!gears_anchor::parse_config(get,0x140000000,anchor))return false;
        auto number=[&](const wchar_t* key,auto& value){wchar_t text[64]{};if(!this->text(L"UIObserver",key,text,64))return false;
            value=static_cast<std::remove_reference_t<decltype(value)>>(std::wcstoull(text,nullptr,0));return true;};
        ui={};auto& c=ui;unsigned schema{};if(!number(L"schema",schema))return false;c.mounted=schema==2;
#define LOAD(x) if(!number(L## #x,c.x))return false
        LOAD(base);LOAD(objects_header);LOAD(names_header);LOAD(identity_count);LOAD(super_count);
        LOAD(object_internal_index);LOAD(object_name);LOAD(object_class);LOAD(object_outer);LOAD(field_super);LOAD(name_entry_index);LOAD(name_entry_text);LOAD(property_offset_internal);
        LOAD(role_controller);LOAD(role_camera);LOAD(role_pawn);LOAD(role_hud);LOAD(role_inventory);LOAD(role_canvas_class);
#define FIELD(x) LOAD(x);LOAD(x##_property);
        GEARS_UI_FIELDS(FIELD)
#undef FIELD
        if(c.mounted){LOAD(role_controlled);LOAD(controlled_driver);LOAD(controlled_driver_property);LOAD(pawn_driven_vehicle);LOAD(pawn_driven_vehicle_property);LOAD(controlled_controller);LOAD(controlled_controller_property);}
#undef LOAD
        if(c.identity_count>c.identities.size()||c.super_count>c.supers.size())return false;
        for(unsigned i=0;i<c.identity_count;++i){auto& id=c.identities[i];wchar_t key[64]{};
            auto field=[&](const wchar_t* label,auto& value){std::swprintf(key,64,L"id%u_%ls",i,label);return number(key,value);};
            if(!field(L"address",id.address)||!field(L"index",id.index)||!field(L"name_index",id.name_index)||!field(L"number",id.number)||!field(L"cls",id.cls)||!field(L"outer",id.outer))return false;
            std::swprintf(key,64,L"id%u_name",i);if(!text(L"UIObserver",key,id.name.data(),64))return false;
        }
        for(unsigned i=0;i<c.super_count;++i){wchar_t key[64]{};std::swprintf(key,64,L"super%u_identity",i);if(!number(key,c.supers[i].identity))return false;
            std::swprintf(key,64,L"super%u_parent",i);if(!number(key,c.supers[i].parent))return false;}
        return true;
    }
    bool read(void* out,const void* source,size_t size){auto address=reinterpret_cast<std::uintptr_t>(source);auto p=static_cast<unsigned char*>(out);
        for(size_t i=0;i<size;++i){auto found=memory.find(address+i);if(found==memory.end())return false;p[i]=found->second;}return true;}
    template<class T> void put(std::uintptr_t address,T const& value){auto p=reinterpret_cast<const unsigned char*>(&value);for(size_t i=0;i<sizeof(value);++i)memory[address+i]=p[i];}
};
int main(){unsigned checks{};Fixture f;CHECK(f.load(L"mounted"));auto read=[&](void* dest,const void* source,size_t size){return f.read(dest,source,size);};
    gears_anchor::Anchor anchor{};std::uint32_t stamp{};CHECK(gears_anchor::guard(f.anchor,read,anchor,stamp));
    CHECK(anchor.pawn==0x80000&&(anchor.location==std::array<float,3>{100,200,300})&&anchor.eye_height==0&&anchor.base_eye_height==0);
    gears_ui_binding::Binding binding{};CHECK(gears_ui_binding::acquire(f.ui,f.anchor,read,binding));CHECK(binding.pawn.address==0x20000&&binding.inventory.address==0x81000);
    gears_crosshair_assignment::OwnedConfig suspended;
    auto get=[&](const wchar_t* key,wchar_t* out,unsigned size){return f.text(L"CrosshairAssignment",key,out,size);};
    CHECK(gears_crosshair_assignment::parse_config(get,123,0x140000000,suspended)&&suspended.mounted_suspended&&suspended.suspended_controlled==anchor.pawn);
    CHECK(gears_crosshair_assignment::mounted_recovery_fresh(suspended,f.anchor,f.ui,123,0x140000000,read));
    const auto baseline=f.memory;
    // An advancing mounted camera is a discarded frame, not a stale identity.
    // Simulate the engine updating POV after the first camera buffer was read.
    for(bool roll_time:{false,true})for(bool break_link:{false,true}){
        f.memory=baseline;unsigned pov_reads{};
        auto camera_race=[&](void* dest,const void* source,size_t size){
            const auto address=reinterpret_cast<std::uintptr_t>(source);
            if(address==f.anchor.identities[f.anchor.role_camera].address+f.anchor.camera_pov&&++pov_reads==2){
                f.put(address,400.f);
                if(roll_time)f.put(f.anchor.identities[f.anchor.role_camera].address+f.anchor.camera_timestamp,11.f);
                if(break_link)f.put(0x20810,std::uintptr_t(0));
            }
            return read(dest,source,size);
        };
        gears_anchor::GuardDiagnostic diagnostic{};
        CHECK(!gears_anchor::strict_guard(f.anchor,camera_race,anchor,stamp,&diagnostic));
        CHECK(diagnostic.timestamp_only==(roll_time&&!break_link));
        if(roll_time&&!break_link)CHECK(gears_anchor::strict_guard(f.anchor,read,anchor,stamp)&&anchor.location[0]==400.f);
        f.memory=baseline;pov_reads=0;
        CHECK(!gears_crosshair_assignment::mounted_recovery_fresh(suspended,f.anchor,f.ui,123,0x140000000,camera_race,&diagnostic));
        CHECK(diagnostic.timestamp_only==(roll_time&&!break_link));
        CHECK(gears_anchor::timestamp_retryable(diagnostic)==(roll_time&&!break_link));
        if(roll_time&&!break_link){
            auto altered=diagnostic;altered.reason=gears_anchor::GuardReason::read_failure;
            CHECK(!gears_anchor::timestamp_retryable(altered));
            altered=diagnostic;altered.timestamp_only=false;CHECK(!gears_anchor::timestamp_retryable(altered));
        }
    }
    f.memory=baseline;
    // A previous timestamp receipt cannot survive success, role mismatch or a
    // hard read failure. The caller must not retry these as stale diagnostics.
    for(unsigned fault=0;fault<4;++fault){
        auto candidate=suspended;
        gears_anchor::GuardDiagnostic diagnostic{};
        diagnostic.timestamp_only=true;diagnostic.reason=gears_anchor::GuardReason::strict_timestamp_changed;
        if(fault==1)candidate.suspended_driver+=8;
        if(fault==2)candidate.suspended_camera+=8;
        auto checked_read=[&](void* dest,const void* source,size_t size){
            if(fault==3&&reinterpret_cast<std::uintptr_t>(source)==f.anchor.objects_header)return false;
            return read(dest,source,size);
        };
        CHECK(gears_crosshair_assignment::mounted_recovery_fresh(candidate,f.anchor,f.ui,123,0x140000000,checked_read,&diagnostic)==(fault==0));
        CHECK(!gears_anchor::timestamp_retryable(diagnostic));
    }
    // Native wire4 cannot silently consume legacy mounted wire3 or omit Owner.
    for(auto mode:{0,1}){
        auto incompatible=[&](const wchar_t* key,wchar_t* out,unsigned size){
            if(mode==0&&!std::wcscmp(key,L"schema")){out[0]=L'3';out[1]=0;return DWORD(1);}
            if(mode==1&&!std::wcscmp(key,L"mounted_property9_role")){out[0]=0;return DWORD(0);}
            return GetPrivateProfileStringW(L"AnchorObserver",key,L"",out,size,f.path.c_str());};
        gears_anchor::Config rejected{};CHECK(!gears_anchor::parse_config(incompatible,0x140000000,rejected));
    }
    for(auto owner:{std::uintptr_t(0),std::uintptr_t(0x80000)}){
        f.memory=baseline;f.put(0x8108c,owner);CHECK(!gears_anchor::guard(f.anchor,read,anchor,stamp));}
    for(auto outer:{std::uintptr_t(0),std::uintptr_t(0x20000)}){
        f.memory=baseline;f.put(0x81040,outer);CHECK(!gears_anchor::guard(f.anchor,read,anchor,stamp));}
    f.memory=baseline;
    unsigned owner_hits=0;
    auto owner_race=[&](void* dest,const void* source,size_t size){const bool ok=read(dest,source,size);
        if(reinterpret_cast<std::uintptr_t>(source)==0x8108c&&++owner_hits==1)f.put(0x8108c,std::uintptr_t(0));return ok;};
    CHECK(!gears_anchor::guard(f.anchor,owner_race,anchor,stamp)&&owner_hits);
    for(auto address:{std::uintptr_t(0x80800),std::uintptr_t(0x20810),std::uintptr_t(0x800d0),std::uintptr_t(0x300d0),std::uintptr_t(0x20820),std::uintptr_t(0x30500)}){
        f.memory=baseline;f.put(address,std::uintptr_t(0));CHECK(!gears_anchor::guard(f.anchor,read,anchor,stamp));CHECK(!gears_ui_binding::acquire(f.ui,f.anchor,read,binding));}
    f.memory=baseline;f.put(0x10224,std::uintptr_t(0xa0000));CHECK(!gears_anchor::guard(f.anchor,read,anchor,stamp));
    f.memory=baseline;f.put(0x20250,std::uintptr_t(0x10000));CHECK(!gears_anchor::guard(f.anchor,read,anchor,stamp));
    f.memory=baseline;auto const property=f.anchor.identities[f.anchor.mounted_properties[2].role].address;
    f.put(property+0x74,4u);CHECK(!gears_anchor::guard(f.anchor,read,anchor,stamp));
    f.memory=baseline;unsigned hits=0;auto race=[&](void* dest,const void* source,size_t size){auto addr=reinterpret_cast<std::uintptr_t>(source);const bool ok=read(dest,source,size);
        if(addr==0x80800&&++hits==1)f.put(0x20810,std::uintptr_t(0));return ok;};CHECK(!gears_anchor::guard(f.anchor,race,anchor,stamp)&&hits);
    for(auto name:{L"streamed",L"streamed_cinematic"}){
        CHECK(f.load(name));CHECK(gears_anchor::guard(f.anchor,read,anchor,stamp));
        CHECK(anchor.level==0xa0000&&anchor.pawn==0x80000&&(anchor.location==std::array<float,3>{100,200,300}));
        CHECK(gears_ui_binding::acquire(f.ui,f.anchor,read,binding)&&binding.pawn.address==0x20000);
        const auto memory=f.memory;const auto config=f.anchor;
        for(unsigned n=0;n<5;++n){
            const std::uintptr_t level=0xa0000+n*0x1000;
            for(auto offset:{8u,64u,72u,76u,80u}){
                f.memory=memory;f.memory[level+offset]^=1;
                CHECK(!gears_anchor::guard(f.anchor,read,anchor,stamp));
                CHECK(!gears_ui_binding::acquire(f.ui,f.anchor,read,binding));}
            f.memory=memory;f.put(level+80,std::uintptr_t(0x8000));
            CHECK(!gears_anchor::guard(f.anchor,read,anchor,stamp));
            f.memory=memory;hits=0;
            auto level_race=[&](void* dest,const void* source,size_t size){const bool ok=read(dest,source,size);
                if(reinterpret_cast<std::uintptr_t>(source)==level&&size==96&&++hits==1)f.put(level+76,1u);return ok;};
            CHECK(!gears_anchor::guard(f.anchor,level_race,anchor,stamp)&&hits);
        }
        f.memory=memory;
        for(auto actor:{std::uintptr_t(0x80000),std::uintptr_t(0x20000),std::uintptr_t(0x10000),std::uintptr_t(0x30000)}){
            f.memory=memory;f.put(actor+0xd0,std::uintptr_t(0x80000));
            CHECK(!gears_anchor::guard(f.anchor,read,anchor,stamp));}
        // Complete numeric INI with an uncertified outer must fail at parsing,
        // before any process read. Existing identities supply all Level roles.
        auto get_missing=[&](const wchar_t* key,wchar_t* out,unsigned size){
            wchar_t changed[64]{};std::swprintf(changed,64,L"id%u_outer",config.role_camera);
            if(!std::wcscmp(key,changed)){const wchar_t text[]=L"3735928559";std::copy(std::begin(text),std::end(text),out);return DWORD(10);}
            return GetPrivateProfileStringW(L"AnchorObserver",key,L"",out,size,f.path.c_str());};
        gears_anchor::Config rejected{};CHECK(!gears_anchor::parse_config(get_missing,0x140000000,rejected));
        f.anchor=config;
        unsigned level_index{};for(;level_index<f.anchor.identity_count;++level_index)if(f.anchor.identities[level_index].address==0xa1000)break;
        CHECK(level_index<f.anchor.identity_count);
        f.anchor.identities[level_index].cls=0x8000;CHECK(!gears_anchor::mounted_levels_config(f.anchor));f.anchor=config;
        const wchar_t default_name[]=L"Default__Level";std::copy(std::begin(default_name),std::end(default_name),f.anchor.identities[level_index].name.begin());
        CHECK(!gears_anchor::mounted_levels_config(f.anchor));f.anchor=config;
        f.memory=memory;
    }
    CHECK(f.load(L"normal"));CHECK(!f.anchor.mounted_enabled&&!f.ui.mounted);CHECK(gears_anchor::guard(f.anchor,read,anchor,stamp));
    CHECK(gears_ui_binding::acquire(f.ui,f.anchor,read,binding)&&binding.pawn.address==0x20000);
    for(auto name:{L"mounted",L"streamed",L"streamed_cinematic",L"normal"})for(bool cached:{false,true}){
        CHECK(f.load(name,cached));CHECK(gears_anchor::guard(f.anchor,read,anchor,stamp));
        CHECK(gears_ui_binding::acquire(f.ui,f.anchor,read,binding));
    }
    std::printf("mounted anchor/UI/config, link/layout races and dismount restoration: %u checks passed\n",checks);return 0;
}
