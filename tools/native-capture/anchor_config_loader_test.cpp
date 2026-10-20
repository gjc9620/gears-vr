#include "anchor_config_loader.h"
#include <cstdio>
#include <map>
#include <string>
#include <array>

#define CHECK(c) do{++checks;if(!(c)){std::printf("failed %d: %s\n",__LINE__,#c);return 1;}}while(0)
using gears_anchor::Config;
struct Field {const wchar_t* name;unsigned Config::*member;};
static const Field fields[]={
    {L"identity_count",&Config::identity_count},{L"object_internal_index",&Config::object_internal_index},
    {L"object_name",&Config::object_name},{L"object_class",&Config::object_class},{L"object_outer",&Config::object_outer},
    {L"field_super",&Config::field_super},{L"name_entry_index",&Config::name_entry_index},{L"name_entry_text",&Config::name_entry_text},
    {L"role_controller",&Config::role_controller},{L"role_local",&Config::role_local},{L"role_camera",&Config::role_camera},
    {L"role_pawn",&Config::role_pawn},{L"role_level",&Config::role_level},
    {L"controller_pawn",&Config::controller_pawn},{L"pawn_controller",&Config::pawn_controller},{L"pawn_location",&Config::pawn_location},
    {L"pawn_rotation",&Config::pawn_rotation},{L"controller_rotation",&Config::controller_rotation},
    {L"eye_height",&Config::eye_height},{L"base_eye_height",&Config::base_eye_height},{L"player_camera",&Config::player_camera},
    {L"pc_owner",&Config::pc_owner},{L"player",&Config::player},{L"camera_view_target",&Config::camera_view_target},
    {L"camera_timestamp",&Config::camera_timestamp},{L"camera_pov",&Config::camera_pov}};
struct Fixture {
    static constexpr std::uintptr_t module=0x140000000;
    Config expected{};
    std::map<std::wstring,std::wstring> values;
    explicit Fixture(unsigned schema){
        values[L"schema"]=std::to_wstring(schema);
        expected.objects_header=0x100;expected.names_header=0x200;
        values[L"objects_header"]=L"0x100";values[L"names_header"]=L"0x200";
        unsigned n=0;
        for(const auto& field:fields){expected.*field.member=++n;values[field.name]=std::to_wstring(n);}
        expected.identity_count=5;values[L"identity_count"]=L"5";
        expected.role_controller=0;expected.role_local=1;expected.role_camera=2;expected.role_pawn=3;expected.role_level=4;
        for(const auto& field:fields)values[field.name]=std::to_wstring(expected.*field.member);
        for(unsigned i=0;i<5;++i){
            const auto prefix=L"id"+std::to_wstring(i)+L"_";auto& id=expected.identities[i];
            id.address=0x1000+0x100*i;id.index=100+i;id.name_index=200+i;id.number=i;
            id.cls=0x8000;id.outer=0x9000;id.check_super=i%2;id.super=0xa000;
            const auto name=L"Identity"+std::to_wstring(i);std::copy(name.begin(),name.end(),id.name.begin());
            values[prefix+L"address"]=std::to_wstring(id.address);values[prefix+L"index"]=std::to_wstring(id.index);
            values[prefix+L"name_index"]=std::to_wstring(id.name_index);values[prefix+L"number"]=std::to_wstring(id.number);
            values[prefix+L"cls"]=std::to_wstring(id.cls);values[prefix+L"outer"]=std::to_wstring(id.outer);
            values[prefix+L"check_super"]=std::to_wstring(id.check_super);values[prefix+L"super"]=std::to_wstring(id.super);
            values[prefix+L"name"]=name;
        }
        if(schema==2){
            expected.cinematic_enabled=true;expected.module_base=module;
            values[L"cinematic_enabled"]=L"1";values[L"module_base"]=L"0x140000000";
            expected.role_cinematic_property=1;expected.role_camera_actor_class=2;expected.cinematic_offset=0x180;
            expected.cinematic_mask=32;expected.role_world_info_property=3;expected.role_world_info=4;expected.world_info_offset=208;
            values[L"role_cinematic_property"]=L"1";values[L"role_camera_actor_class"]=L"2";
            values[L"cinematic_offset"]=L"0x180";values[L"cinematic_mask"]=L"32";
            values[L"role_world_info_property"]=L"3";values[L"role_world_info"]=L"4";values[L"world_info_offset"]=L"208";
        }
    }
    bool parse(Config& out,std::uintptr_t base=module){
        return gears_anchor::parse_config([&](const wchar_t* key,wchar_t* buffer,unsigned capacity){
            const auto found=values.find(key);
            const auto text=found==values.end()?std::wstring{}:found->second;
            const auto count=std::min<std::size_t>(text.size(),capacity-1);
            std::copy_n(text.data(),count,buffer);buffer[count]=0;return static_cast<unsigned>(count);
        },base,out);
    }
};
static bool unchanged_rejection(Fixture& fixture){
    Config out{};out.objects_header=0xbad;out.names_header=0xcafe;out.identity_count=17;
    out.cinematic_enabled=true;out.module_base=0x111;out.identities[127].address=0x222;
    out.identities[0].name[0]=L'X';
    std::array<unsigned char,sizeof(Config)> before{};std::memcpy(before.data(),&out,sizeof(out));
    return !fixture.parse(out)&&std::memcmp(before.data(),&out,sizeof(out))==0;
}
int main(){
    unsigned checks{};
    for(unsigned schema:{1u,2u}){
        Fixture fixture(schema);Config out{};out.cinematic_enabled=true;out.module_base=0xdead;
        out.identities[127].address=0xbad;CHECK(fixture.parse(out));const auto& expected=fixture.expected;
        CHECK(out.objects_header==expected.objects_header&&out.names_header==expected.names_header);
        for(const auto& field:fields)CHECK(out.*field.member==expected.*field.member);
        CHECK(out.cinematic_enabled==expected.cinematic_enabled&&out.module_base==expected.module_base);
        CHECK(out.role_cinematic_property==expected.role_cinematic_property&&out.role_camera_actor_class==expected.role_camera_actor_class);
        CHECK(out.cinematic_offset==expected.cinematic_offset&&out.cinematic_mask==expected.cinematic_mask);
        CHECK(out.role_world_info_property==expected.role_world_info_property&&out.role_world_info==expected.role_world_info&&out.world_info_offset==expected.world_info_offset);
        CHECK(out.identities[127].address==0);
        for(unsigned i=0;i<out.identity_count;++i){const auto& a=out.identities[i];const auto& b=expected.identities[i];
            CHECK(a.address==b.address&&a.cls==b.cls&&a.outer==b.outer&&a.super==b.super);
            CHECK(a.index==b.index&&a.name_index==b.name_index&&a.number==b.number&&a.check_super==b.check_super&&a.name==b.name);
        }
    }
    for(const auto& key:{L"objects_header",L"camera_pov",L"id2_super",L"id4_name"}){
        Fixture f(2);f.values.erase(key);CHECK(unchanged_rejection(f));
    }
    const std::pair<const wchar_t*,const wchar_t*> bad[]={
        {L"schema",L"0"},{L"schema",L"3"},{L"schema",L"-1"},{L"schema",L"+1"},
        {L"objects_header",L"18446744073709551616"},{L"objects_header",L"0x100oops"},
        {L"identity_count",L"0"},{L"identity_count",L"129"},{L"id4_number",L"4294967296"},
        {L"object_internal_index",L"93"},{L"object_name",L"89"},{L"object_class",L"89"},{L"object_outer",L"89"},
        {L"camera_pov",L"0x10000"},{L"id4_check_super",L"2"},{L"id4_address",L"0"},{L"id4_cls",L"0"},
        {L"id4_address",L"0x1100"},{L"cinematic_enabled",L"0"},{L"cinematic_enabled",L"2"},
        {L"module_base",L"0x140000001"},{L"cinematic_offset",L"0xfffd"}};
    for(const auto& item:bad){Fixture f(2);f.values[item.first]=item.second;CHECK(unchanged_rejection(f));}
    for(unsigned length:{0u,64u,65u,200u}){Fixture f(1);f.values[L"id4_name"]=std::wstring(length,L'x');CHECK(unchanged_rejection(f));}
    {Fixture f(1);f.values[L"id4_name"]=std::wstring(63,L'x');Config out{};CHECK(f.parse(out));CHECK(out.identities[4].name[63]==0);}
    {Fixture f(1);f.values[L"id4_name"]=L"\u73a9\u5bb6";Config out{};CHECK(f.parse(out));CHECK(std::wstring(out.identities[4].name.data())==L"\u73a9\u5bb6");}
    {Fixture f(2);f.values[L"objects_header"]=std::wstring(80,L'0');CHECK(unchanged_rejection(f));}
    {Fixture f(1);f.values[L"cinematic_enabled"]=L"unread";f.values[L"module_base"]=L"unread";Config out{};CHECK(f.parse(out));CHECK(!out.cinematic_enabled&&out.module_base==0);}
    {Fixture f(2);Config out{};CHECK(!f.parse(out,Fixture::module+1));}
    std::printf("anchor config loader: %u checks passed\n",checks);return 0;
}
