#include "anchor_observer.h"
#include <cstdio>
#include <vector>
#include <limits>
#define CHECK(c) do{if(!(c)){std::printf("failed %d: %s\n",__LINE__,#c);return __LINE__;}}while(0)
using namespace gears_anchor;
static Receipt receipt(){Receipt r{};r.key.local=1;r.key.family=r.key.owner=2;r.key.source=3;r.key.stamp=4;
    r.anchor.pawn=5;r.anchor.location={1,2,3};r.anchor.eye_height=4;r.invocation=1;r.thread=7;r.scene_qpc=10;return r;}
static Reason offer(Context& c,Receipt r){c.begin_scene();auto why=c.offer(r);c.end_scene();return why;}
static int cinematic_tests(){
    std::vector<unsigned char> mem(300000);
    const uintptr_t module=0x140000000,vtable=module+0x2000000,native_slot=module+cinematic_gnatives_rva+0x2d*8,accessor_slot=vtable+0x2a0;
    uintptr_t native=module+cinematic_bool_native_rva,accessor=module+cinematic_bool_accessor_rva;
    auto put=[&](uintptr_t address,auto const& value){std::memcpy(mem.data()+address,&value,sizeof(value));};
    auto read=[&](void* dest,void const* source,size_t size){const auto address=reinterpret_cast<uintptr_t>(source);
        if(size==8&&(address==native_slot||address==accessor_slot)){const auto value=address==native_slot?native:accessor;std::memcpy(dest,&value,8);return true;}
        if(address>mem.size()||size>mem.size()-address)return false;std::memcpy(dest,mem.data()+address,size);return true;};
    Config cfg{};cfg.objects_header=100;cfg.names_header=200;cfg.identity_count=19;
    cfg.object_internal_index=8;cfg.object_name=12;cfg.object_class=24;cfg.object_outer=32;cfg.field_super=96;
    cfg.name_entry_index=0;cfg.name_entry_text=8;cfg.role_controller=0;cfg.role_local=1;cfg.role_camera=2;cfg.role_pawn=3;cfg.role_level=4;
    cfg.controller_pawn=120;cfg.pawn_controller=120;cfg.player_camera=128;cfg.pc_owner=120;cfg.player=136;
    cfg.camera_view_target=128;cfg.camera_timestamp=140;cfg.pawn_location=140;cfg.eye_height=152;cfg.base_eye_height=156;
    cfg.cinematic_enabled=true;cfg.role_cinematic_property=13;cfg.role_camera_actor_class=12;cfg.cinematic_offset=0x180;cfg.cinematic_mask=0x20;
    cfg.module_base=0x140000000;cfg.role_world_info_property=16;cfg.role_world_info=17;cfg.world_info_offset=0xd0;
    Registry objects{1000,2001,2100},names{20000,2001,2100};put(100,objects);put(200,names);
    const wchar_t* labels[]={L"PC",L"Local",L"Camera",L"Pawn",L"LevelObj",L"WarLocalPlayer",L"WarPlayerCamera",L"WarPawn",L"Level",L"Class",L"BoolProperty",L"PlayerController",L"CameraActor",L"bCinematicMode",L"Actor",L"ObjectProperty",L"WorldInfo",L"WorldInfo_0",L"WorldInfo"};
    for(unsigned i=0;i<cfg.identity_count;++i){auto& id=cfg.identities[i];id.address=0x10000+i*0x200;id.index=id.name_index=i;
        id.cls=0x10000+(i==0?11:i==1?5:i==2?6:i==3?7:i==4?8:i==13?10:i==16?15:i==17?18:9)*0x200;
        id.outer=i==3?cfg.identities[0].address+4*0x200:i==13?0x10000+11*0x200:i==16?0x10000+14*0x200:i==17?0x10000+4*0x200:0;
        id.check_super=(i>=5&&i<=12)||i==14||i==15||i==18;
        if(i==7||i==11||i==12||i==18)id.super=0x10000+14*0x200;std::copy_n(labels[i],std::wcslen(labels[i])+1,id.name.begin());
        put(id.address+8,id.index);put(id.address+12,id.name_index);put(id.address+24,id.cls);put(id.address+32,id.outer);
        if(id.check_super)put(id.address+cfg.field_super,id.super);
        put(objects.data+i*8,id.address);const auto entry=uintptr_t(0x18000+i*0x200);put(names.data+i*8,entry);put(entry,id.name_index);put(entry+8,id.name);
    }
    const auto pc=cfg.identities[0].address,local=cfg.identities[1].address,camera=cfg.identities[2].address,pawn=cfg.identities[3].address,level=cfg.identities[4].address;
    const auto property=cfg.identities[13].address,target=uintptr_t(0x20000),entry=uintptr_t(0x21000);const unsigned target_index=100,target_name=101;
    put(pc+120,pawn);put(pawn+120,pc);put(pc+128,camera);put(camera+120,pc);put(pc+136,local);put(camera+128,target);
    put(camera+140,1.f);put(pawn+140,std::array<float,3>{1,2,3});put(pawn+152,4.f);put(pawn+156,5.f);
    put(property+0x70,1u);put(property+0x74,4u);put(property+0x94,cfg.cinematic_offset);put(property+0xd0,cfg.cinematic_mask);put(pc+cfg.cinematic_offset,cfg.cinematic_mask);
    put(property,vtable);
    put(target+8,target_index);put(target+12,target_name);put(target+24,cfg.identities[12].address);put(target+32,level);
    put(objects.data+target_index*8,target);put(names.data+target_name*8,entry);put(entry,target_name);
    std::array<wchar_t,64> target_text{};std::copy_n(L"CineCamera",11,target_text.begin());put(entry+8,target_text);
    const auto world_property=cfg.identities[16].address,world=cfg.identities[17].address;
    put(world_property+0x70,1u);put(world_property+0x74,8u);put(world_property+0x94,cfg.world_info_offset);
    put(pc+cfg.world_info_offset,world);put(pawn+cfg.world_info_offset,world);put(target+cfg.world_info_offset,world);
    const auto baseline=mem;const auto initial=cfg;
    Anchor anchor{};uint32_t stamp{};PawnBinding binding{};
    CHECK(guard(cfg,read,anchor,stamp)&&anchor.cinematic&&anchor.pawn==pawn&&anchor.target_identity.address==target);
    CHECK(capture_pawn_binding(cfg,read,binding)&&scene_binding_matches(binding,anchor));
    // NameEntry storage after the first NUL is not part of UObject identity.
    auto trailing_text=target_text;std::fill(trailing_text.begin()+11,trailing_text.end(),L'x');put(entry+8,trailing_text);
    Anchor normalized{};CHECK(guard(cfg,read,normalized,stamp)&&equal_anchor(anchor,normalized));
    CHECK(normalized.target_identity.name==target_text);
    PawnBinding normalized_binding{};CHECK(capture_pawn_binding(cfg,read,normalized_binding)&&equal_binding(binding,normalized_binding));
    trailing_text.back()=L'y';put(entry+8,trailing_text);
    CHECK(guard(cfg,read,normalized,stamp)&&equal_anchor(anchor,normalized));
    // Even irrelevant trailing storage must be stable across one raw read pair.
    bool changed_tail=false;auto changing_tail=[&](void* dest,void const* source,size_t size){const auto address=reinterpret_cast<uintptr_t>(source);const bool ok=read(dest,source,size);
        if(!changed_tail&&address==entry+8&&size==sizeof(trailing_text)){changed_tail=true;trailing_text.back()=L'z';put(entry+8,trailing_text);}return ok;};
    CHECK(!guard(cfg,changing_tail,normalized,stamp)&&changed_tail);
    mem=baseline;
    // Streaming CameraActor Levels may differ, but must share the certified WorldInfo.
    const uintptr_t streamed_level=0x23000,streamed_entry=0x24000;const unsigned streamed_index=104,streamed_name=105;
    put(streamed_level+8,streamed_index);put(streamed_level+12,streamed_name);put(streamed_level+24,cfg.identities[8].address);
    put(objects.data+streamed_index*8,streamed_level);put(names.data+streamed_name*8,streamed_entry);put(streamed_entry,streamed_name);
    auto streamed_text=target_text;streamed_text.fill(0);std::copy_n(L"PersistentLevel",16,streamed_text.begin());put(streamed_entry+8,streamed_text);
    put(target+32,streamed_level);
    CHECK(guard(cfg,read,normalized,stamp)&&normalized.target_level_identity.address==streamed_level);
    CHECK(capture_pawn_binding(cfg,read,normalized_binding)&&scene_binding_matches(normalized_binding,normalized));
    CHECK(!equal_anchor(anchor,normalized)&&!equal_binding(binding,normalized_binding));
    const auto streamed_baseline=mem;
    for(unsigned fault=0;fault<12;++fault){mem=streamed_baseline;
        if(fault==0)put(target+cfg.world_info_offset,uintptr_t(world+8));
        if(fault==1)put(pc+cfg.world_info_offset,uintptr_t(0));
        if(fault==2)put(pawn+cfg.world_info_offset,uintptr_t(0));
        if(fault==3)put(streamed_level+24,cfg.identities[12].address);
        if(fault==4)put(objects.data+streamed_index*8,uintptr_t(0));
        if(fault==5)put(streamed_entry,streamed_name+1);
        if(fault==6)put(world_property+0x70,2u);
        if(fault==7)put(world_property+0x74,4u);
        if(fault==8)put(world_property+0x94,cfg.world_info_offset+8);
        if(fault==9)put(world+32,streamed_level);
        if(fault==10)put(world+24,cfg.identities[12].address);
        if(fault==11){auto text=streamed_text;std::copy_n(L"Default__Level",15,text.begin());put(streamed_entry+8,text);}
        CHECK(!guard(cfg,read,normalized,stamp));
    }
    for(unsigned fault=0;fault<4;++fault){mem=streamed_baseline;bool mutated=false;
        auto changing_world=[&](void* dest,void const* source,size_t size){const auto address=reinterpret_cast<uintptr_t>(source);const bool ok=read(dest,source,size);
            if(!mutated&&address==streamed_level&&size==96){mutated=true;
                if(fault==0)put(streamed_level+16,1u);
                if(fault==1)put(target+cfg.world_info_offset,uintptr_t(0));
                if(fault==2)put(pc+cfg.world_info_offset,uintptr_t(0));
                if(fault==3)put(world_property+0x74,4u);}
            return ok;};
        CHECK(!guard(cfg,changing_world,normalized,stamp)&&mutated);
    }
    for(unsigned fault=0;fault<6;++fault){mem=streamed_baseline;cfg=initial;
        if(fault==0)cfg.role_world_info_property=128;
        if(fault==1)cfg.role_world_info=128;
        if(fault==2)cfg.role_world_info_property=cfg.role_cinematic_property;
        if(fault==3)cfg.role_world_info=cfg.role_controller;
        if(fault==4){cfg.identities[7].super=0;put(cfg.identities[7].address+cfg.field_super,uintptr_t(0));}
        if(fault==5){cfg.identities[12].super=0;put(cfg.identities[12].address+cfg.field_super,uintptr_t(0));}
        CHECK(!guard(cfg,read,normalized,stamp));
    }
    cfg=initial;
    mem=baseline;
    auto changed=anchor;changed.target_identity.number++;CHECK(!equal_anchor(anchor,changed)&&!scene_binding_matches(binding,changed));
    changed=anchor;changed.cinematic=false;CHECK(!equal_anchor(anchor,changed)&&!scene_binding_matches(binding,changed));
    auto changed_binding=binding;changed_binding.target_identity.number++;CHECK(!equal_binding(binding,changed_binding));
    changed_binding=binding;changed_binding.cinematic=false;CHECK(!equal_binding(binding,changed_binding));
    for(unsigned fault=0;fault<33;++fault){mem=baseline;cfg=initial;
        if(fault==0)cfg.cinematic_enabled=false;
        if(fault==1)put(pc+cfg.cinematic_offset,0u);
        if(fault==2)cfg.cinematic_mask=0;
        if(fault==3)cfg.cinematic_mask=3;
        if(fault==4)put(target+24,cfg.identities[6].address);
        if(fault==5)put(target+32,uintptr_t(level+0x100));
        if(fault==6)put(camera+128,uintptr_t(0));
        if(fault==7)put(pc+120,uintptr_t(0));
        if(fault==8)put(pawn+120,local);
        if(fault==9)put(camera+120,local);
        if(fault==10)put(pc+136,pawn);
        if(fault==11)put(pc+128,pawn);
        if(fault==12)put(property+0x94,cfg.cinematic_offset+4);
        if(fault==13)put(property+0x70,2u);
        if(fault==14)put(property+0x74,1u);
        if(fault==15)put(property+0xd0,cfg.cinematic_mask<<1);
        if(fault==16)put(property+24,cfg.identities[9].address);
        if(fault==17)cfg.identities[13].outer=cfg.identities[12].address;
        if(fault==18)cfg.role_cinematic_property=128;
        if(fault==19)cfg.role_camera_actor_class=128;
        if(fault==20)put(target+8,unsigned(objects.count));
        if(fault==21)put(target+12,unsigned(names.count));
        if(fault==22)put(objects.data+target_index*8,pawn);
        if(fault==23)put(entry,target_name+1);
        if(fault==24){auto text=target_text;std::copy_n(L"Default__CameraActor",20,text.begin());put(entry+8,text);}
        if(fault==25)cfg.module_base=0;
        if(fault==26)put(cfg.identities[12].address+24,uintptr_t(123));
        if(fault==27)cfg.role_cinematic_property=cfg.role_camera_actor_class;
        if(fault==28)cfg.identities[13].name.fill(L'x');
        if(fault==29)cfg.role_camera_actor_class=10;
        if(fault==30)cfg.identities[10].cls=cfg.identities[11].address;
        if(fault==31)put(property+8,100u);
        if(fault==32)put(property,uintptr_t(0));
        auto sentinel=anchor;const auto old_stamp=stamp;
        CHECK(!guard(cfg,read,anchor,stamp)&&equal_anchor(sentinel,anchor)&&old_stamp==stamp);
    }
    mem=baseline;cfg=initial;++native;CHECK(!guard(cfg,read,anchor,stamp));--native;
    ++accessor;CHECK(!guard(cfg,read,anchor,stamp));--accessor;
    bool other_bit=false;auto unrelated_flag=[&](void* dest,void const* source,size_t size){const auto address=reinterpret_cast<uintptr_t>(source);const bool ok=read(dest,source,size);
        if(address==pc+cfg.cinematic_offset&&!other_bit){other_bit=true;put(address,cfg.cinematic_mask|1u);}return ok;};
    CHECK(guard(cfg,unrelated_flag,anchor,stamp)&&anchor.cinematic); // Other packed bools are not this flag.
    mem=baseline;cfg=initial;put(camera+128,pawn);put(pc+cfg.cinematic_offset,0u);
    CHECK(guard(cfg,read,anchor,stamp)&&!anchor.cinematic&&anchor.pawn==pawn&&anchor.target_identity.address==pawn);
    CHECK(equal_identity(anchor.target_level_identity,cfg.identities[cfg.role_level]));
    put(pawn+cfg.world_info_offset,uintptr_t(0));CHECK(!guard(cfg,read,anchor,stamp));put(pawn+cfg.world_info_offset,world);
    put(pc+cfg.world_info_offset,uintptr_t(0));CHECK(!guard(cfg,read,anchor,stamp));put(pc+cfg.world_info_offset,world);
    put(pc+cfg.cinematic_offset,cfg.cinematic_mask);CHECK(guard(cfg,read,anchor,stamp)&&anchor.cinematic&&anchor.target_identity.address==pawn);
    // The optional configuration never changes the old disabled Pawn route.
    cfg.cinematic_enabled=false;CHECK(guard(cfg,read,anchor,stamp)&&!anchor.cinematic&&!anchor.target_identity.address);
    for(unsigned fault=0;fault<4;++fault){mem=baseline;cfg=initial;unsigned reads=0;
        auto changing=[&](void* dest,void const* source,size_t size){const auto address=reinterpret_cast<uintptr_t>(source);const bool ok=read(dest,source,size);
            if(address==target&&size==96&&++reads==1){
                if(fault==0)put(target+16,1u);
                if(fault==1)put(camera+128,pawn);
                if(fault==2)put(pc+cfg.cinematic_offset,0u);
                if(fault==3)put(names.data+target_name*8,entry+0x100);
            }return ok;};
        CHECK(!guard(cfg,changing,anchor,stamp));
    }
    mem=baseline;cfg=initial;
    const auto other_target=uintptr_t(0x22000);const unsigned other_index=102;
    std::memcpy(mem.data()+other_target,mem.data()+target,96);put(other_target+8,other_index);put(other_target+16,1u);put(objects.data+other_index*8,other_target);put(other_target+cfg.world_info_offset,world);
    CHECK(guard(cfg,read,anchor,stamp));CHECK(capture_pawn_binding(cfg,read,binding));
    put(camera+128,other_target);CHECK(guard(cfg,read,changed,stamp)&&changed.target_identity.address==other_target);
    CHECK(!equal_anchor(anchor,changed)&&!scene_binding_matches(binding,changed));
    // Initial binding A -> a fully stable strict observation B -> final A must
    // not mix a B anchor into an A scene association.
    put(camera+128,target);bool entered=false,restored=false;unsigned other_headers=0;
    auto changing_between=[&](void* dest,void const* source,size_t size){const auto address=reinterpret_cast<uintptr_t>(source);const bool ok=read(dest,source,size);
        if(!entered&&address==camera+140){entered=true;put(camera+128,other_target);}
        if(address==other_target&&size==96)++other_headers;
        if(!restored&&other_headers==4&&address==camera+128&&size==8){restored=true;put(camera+128,target);}
        return ok;};
    CHECK(!guard(cfg,changing_between,anchor,stamp)&&entered&&restored);
    // The opt-in unpossessed route is restricted to the immutable certified
    // Pawn. CameraPOV supplies the origin only after null links and all the
    // existing cinematic/WorldInfo/identity certificates have passed.
    mem=baseline;cfg=initial;cfg.camera_pov=0x160;
    put(pc+120,uintptr_t(0));put(pawn+120,uintptr_t(0));put(camera+128,pawn);
    const std::array<float,7> pov{101,202,303,0,0,0,90};put(camera+cfg.camera_pov,pov);
    const auto unpossessed_memory=mem;
    CHECK(!cfg.unpossessed_camera_enabled&&!capture_pawn_binding(cfg,read,binding)&&!guard(cfg,read,anchor,stamp));
    CHECK(!strict_guard(cfg,read,anchor,stamp,nullptr,true));
    cfg.unpossessed_camera_enabled=true;const auto unpossessed_config=cfg;
    CHECK(capture_pawn_binding(cfg,read,binding)&&binding.unpossessed&&binding.pawn.address==pawn);
    CHECK(guard(cfg,read,anchor,stamp)&&anchor.unpossessed&&anchor.cinematic&&scene_binding_matches(binding,anchor));
    CHECK((anchor.location==std::array<float,3>{101,202,303})&&anchor.eye_height==0&&anchor.base_eye_height==0);
    CHECK(equal_identity(anchor.pawn_identity,cfg.identities[cfg.role_pawn])&&equal_identity(anchor.target_identity,anchor.pawn_identity));
    CHECK(!strict_guard(cfg,read,anchor,stamp)&&strict_guard(cfg,read,anchor,stamp,nullptr,true)&&anchor.unpossessed);
    const auto unpossessed_anchor=anchor;const auto unpossessed_binding=binding;
    changed=anchor;changed.unpossessed=false;CHECK(!equal_anchor(anchor,changed)&&!scene_binding_matches(binding,changed));
    changed_binding=binding;changed_binding.unpossessed=false;CHECK(!equal_binding(binding,changed_binding));
    // The cinematic flag itself may be clear, but it must remain equal.
    put(pc+cfg.cinematic_offset,0u);CHECK(guard(cfg,read,anchor,stamp)&&anchor.unpossessed&&!anchor.cinematic);
    CHECK(!equal_anchor(anchor,unpossessed_anchor));
    mem=unpossessed_memory;
    GuardDiagnostic unpossessed_diagnostic{};
    // Failed reads that happen to zero their destination never select null mode.
    for(const auto failed_address:{pc+120,pawn+120}){
        auto zero_failure=[&](void* dest,void const* source,size_t size){
            if(reinterpret_cast<uintptr_t>(source)==failed_address){std::memset(dest,0,size);return false;}
            return read(dest,source,size);};
        CHECK(!capture_pawn_binding(cfg,zero_failure,binding));
        CHECK(!guard(cfg,zero_failure,anchor,stamp,&unpossessed_diagnostic)&&!unpossessed_diagnostic.timestamp_only&&
            unpossessed_diagnostic.reason==GuardReason::read_failure);
    }
    for(unsigned fault=0;fault<21;++fault){mem=unpossessed_memory;cfg=unpossessed_config;
        if(fault==0)cfg.cinematic_enabled=false;
        if(fault==1)cfg.mounted_enabled=true;
        if(fault==2)put(pawn+120,pc);
        if(fault==3)put(pawn+120,local);
        if(fault==4)put(camera+128,target); // Otherwise valid CameraActor.
        if(fault==5)put(pc+cfg.world_info_offset,uintptr_t(0));
        if(fault==6)put(pawn+cfg.world_info_offset,uintptr_t(0));
        if(fault==7)put(pawn+16,1u);
        if(fault==8)put(objects.data+cfg.identities[cfg.role_pawn].index*8,uintptr_t(0));
        if(fault==9)put(pawn+32,uintptr_t(0));
        if(fault==10)put(pawn+24,cfg.identities[12].address);
        if(fault==11)put(camera+cfg.camera_pov+24,0.f);
        if(fault==12)put(camera+cfg.camera_pov+24,180.f);
        if(fault==13)put(camera+cfg.camera_pov+24,std::numeric_limits<float>::quiet_NaN());
        if(fault==14)put(camera+cfg.camera_pov,std::numeric_limits<float>::infinity());
        if(fault==15)put(camera+140,std::numeric_limits<float>::quiet_NaN());
        if(fault==16)put(world_property+0x94,cfg.world_info_offset+8);
        if(fault==17)put(property+0xd0,cfg.cinematic_mask<<1);
        if(fault==18)put(world+32,uintptr_t(0));
        if(fault==19)put(camera+120,local);
        if(fault==20)put(pc+136,pawn);
        anchor=unpossessed_anchor;const auto old_stamp=stamp;
        CHECK(!guard(cfg,read,anchor,stamp,&unpossessed_diagnostic)&&!unpossessed_diagnostic.timestamp_only&&
            equal_anchor(anchor,unpossessed_anchor)&&stamp==old_stamp);
    }
    cfg=unpossessed_config;mem=unpossessed_memory;
    // A valid same-class Pawn is legal for ordinary possession, but null mode
    // cannot expand to it or remember the most recent reassociated Pawn.
    const uintptr_t other_pawn=0x25000,other_pawn_entry=0x26000;const unsigned other_pawn_index=106,other_pawn_name=107;
    std::memcpy(mem.data()+other_pawn,mem.data()+pawn,0x100);
    put(other_pawn+8,other_pawn_index);put(other_pawn+12,other_pawn_name);
    put(objects.data+other_pawn_index*8,other_pawn);put(names.data+other_pawn_name*8,other_pawn_entry);put(other_pawn_entry,other_pawn_name);
    std::array<wchar_t,64> other_pawn_text{};std::copy_n(L"OtherPawn",10,other_pawn_text.begin());put(other_pawn_entry+8,other_pawn_text);
    put(camera+128,other_pawn);CHECK(!guard(cfg,read,anchor,stamp,&unpossessed_diagnostic)&&!unpossessed_diagnostic.timestamp_only);
    put(pc+120,other_pawn);put(other_pawn+120,pc);
    CHECK(guard(cfg,read,anchor,stamp)&&!anchor.unpossessed&&anchor.pawn==other_pawn);
    CHECK((anchor.location==std::array<float,3>{1,2,3})&&anchor.eye_height==4&&anchor.base_eye_height==5);
    put(pc+120,uintptr_t(0));put(other_pawn+120,uintptr_t(0));
    CHECK(!guard(cfg,read,anchor,stamp,&unpossessed_diagnostic)&&!unpossessed_diagnostic.timestamp_only);
    // Turnover at either strict timestamp boundary is a hard rejection, even
    // when the timestamp also advances. Rotation bytes participate in POV equality.
    for(unsigned fault=0;fault<8;++fault){mem=unpossessed_memory;cfg=unpossessed_config;bool mutated=false;
        auto changing_unpossessed=[&](void* dest,void const* source,size_t size){const auto address=reinterpret_cast<uintptr_t>(source);const bool ok=read(dest,source,size);
            const bool at_pov=address==camera+cfg.camera_pov&&size==28;
            const bool at_time=address==camera+140&&size==4;
            if(!mutated&&(fault<2?at_time:at_pov)){mutated=true;
                if(fault==0){put(pc+120,pawn);put(pawn+120,pc);}
                if(fault==1)put(pc+120,pawn);
                if(fault==2)put(pawn+120,pc);
                if(fault==3)put(camera+cfg.camera_pov+12,1u);
                if(fault==4)put(pc+cfg.cinematic_offset,0u);
                if(fault==5)put(pawn+cfg.world_info_offset,uintptr_t(0));
                if(fault==6)put(pawn+16,1u);
                if(fault==7)put(camera+128,target);
                put(camera+140,2.f);
            }return ok;};
        CHECK(!guard(cfg,changing_unpossessed,anchor,stamp,&unpossessed_diagnostic)&&mutated&&!unpossessed_diagnostic.timestamp_only);
    }
    // Final binding turnover after an otherwise complete strict timestamp race
    // must suppress the startup retry as well.
    mem=unpossessed_memory;cfg=unpossessed_config;unsigned unpossessed_stamp_reads=0;bool final_mode_changed=false;
    auto final_turnover=[&](void* dest,void const* source,size_t size){
        const auto address=reinterpret_cast<uintptr_t>(source);
        if(address==camera+140&&size==4&&++unpossessed_stamp_reads==2)put(camera+140,2.f);
        if(!final_mode_changed&&unpossessed_stamp_reads==2&&address==cfg.objects_header){final_mode_changed=true;
            put(pc+120,pawn);put(pawn+120,pc);}
        return read(dest,source,size);};
    CHECK(!guard(cfg,final_turnover,anchor,stamp,&unpossessed_diagnostic)&&final_mode_changed&&
        unpossessed_diagnostic.reason==GuardReason::final_binding&&!unpossessed_diagnostic.timestamp_only);
    mem=unpossessed_memory;unpossessed_stamp_reads=0;final_mode_changed=false;
    std::uint64_t unpossessed_ms=0;unsigned unpossessed_sleeps=0;BoundedGuardDiagnostic bounded_unpossessed{};
    auto unpossessed_clock=[&](){return unpossessed_ms;};
    auto unpossessed_pause=[&](unsigned ms){unpossessed_ms+=ms;++unpossessed_sleeps;};
    CHECK(!bounded_guard(cfg,final_turnover,unpossessed_clock,unpossessed_pause,anchor,stamp,&bounded_unpossessed)&&
        bounded_unpossessed.attempts==1&&!unpossessed_sleeps&&!bounded_unpossessed.last.timestamp_only);
    mem=unpossessed_memory;cfg=unpossessed_config;put(pc+120,pawn);put(pawn+120,pc);bool lost_possession=false;
    auto controlled_turnover=[&](void* dest,void const* source,size_t size){const bool ok=read(dest,source,size);
        if(!lost_possession&&reinterpret_cast<uintptr_t>(source)==camera+140&&size==4){lost_possession=true;
            put(pc+120,uintptr_t(0));put(pawn+120,uintptr_t(0));}return ok;};
    CHECK(!guard(cfg,controlled_turnover,anchor,stamp,&unpossessed_diagnostic)&&lost_possession&&!unpossessed_diagnostic.timestamp_only);
    mem=unpossessed_memory;cfg=unpossessed_config;unpossessed_stamp_reads=0;
    auto only_time=[&](void* dest,void const* source,size_t size){
        if(reinterpret_cast<uintptr_t>(source)==camera+140&&size==4&&++unpossessed_stamp_reads==2)put(camera+140,2.f);
        return read(dest,source,size);};
    CHECK(!guard(cfg,only_time,anchor,stamp,&unpossessed_diagnostic)&&unpossessed_diagnostic.timestamp_only&&
        unpossessed_diagnostic.reason==GuardReason::strict_timestamp_changed);
    mem=unpossessed_memory;unpossessed_stamp_reads=0;
    CHECK(bounded_guard(cfg,only_time,unpossessed_clock,unpossessed_pause,anchor,stamp,&bounded_unpossessed)&&
        bounded_unpossessed.attempts==2&&unpossessed_sleeps==1&&anchor.unpossessed);
    CHECK(guard(cfg,read,anchor,stamp)&&anchor.unpossessed);
    CHECK(capture_pawn_binding(cfg,read,binding)&&equal_binding(binding,unpossessed_binding));
    return 0;
}
int main(){
    if(auto failure=cinematic_tests())return failure;
    Context c;auto r=receipt();Receipt output{};
    CHECK(c.consume(r.key,r.anchor,7,30,true,1,0x3a1595,output)==no_receipt);
    CHECK(offer(c,r)==accepted);
    {Context::FamilyScope family(c,r.key,r.anchor,7,20,true,1,0x2c8909);CHECK(family.reason==accepted);
        CHECK(c.consume(r.key,r.anchor,7,30,true,1,0x3a1595,output)==accepted);
        CHECK(output.invocation==1&&output.scene_qpc==10&&output.family_qpc==20&&output.renderer_qpc==30);
        CHECK(c.consume(r.key,r.anchor,7,31,true,1,0x3a1595,output)==already_consumed);}
    CHECK(c.consume(r.key,r.anchor,7,32,true,1,0x3a1595,output)==no_receipt);
    // Submission is bound to the current source's full128-byte local snapshot,
    // including projection changes after the previous view64 association.
    auto bound=output;auto snapshot=bound.renderer_matrices;
    auto submission=[&](Receipt const* value){return submission_matches(value,2,3,1,7,snapshot.data(),40);};
    CHECK(submission(&bound));CHECK(!submission(nullptr));
    auto bad=bound;bad.invocation=0;CHECK(!submission(&bad));
    bad=bound;bad.thread=0;CHECK(!submission(&bad));
    CHECK(!submission_matches(&bound,2,3,1,0,snapshot.data(),40));
    CHECK(!submission_matches(&bound,2,3,1,8,snapshot.data(),40));
    CHECK(!submission_matches(&bound,4,3,1,7,snapshot.data(),40));
    CHECK(!submission_matches(&bound,2,4,1,7,snapshot.data(),40));
    CHECK(!submission_matches(&bound,2,3,4,7,snapshot.data(),40));
    CHECK(!submission_matches(&bound,0,3,1,7,snapshot.data(),40));
    CHECK(!submission_matches(&bound,2,0,1,7,snapshot.data(),40));
    CHECK(!submission_matches(&bound,2,3,0,7,snapshot.data(),40));
    CHECK(!submission_matches(&bound,2,3,1,7,nullptr,40));
    bad=bound;bad.key.owner=4;CHECK(!submission(&bad));
    const std::array<std::array<std::int64_t,3>,4> bad_times{{{0,20,30},{21,20,30},{10,31,30},{10,20,41}}};
    for(auto const& times:bad_times){
        bad=bound;bad.scene_qpc=times[0];bad.family_qpc=times[1];bad.renderer_qpc=times[2];CHECK(!submission(&bad));}
    // Snapshot first, then the current source changed: old receipt must reject.
    snapshot[0]=1;CHECK(!submission(&bound));snapshot=bound.renderer_matrices;
    snapshot[64]=1;CHECK(!submission(&bound));snapshot=bound.renderer_matrices;
    float nonfinite=std::numeric_limits<float>::infinity();
    bad=bound;std::memcpy(bad.renderer_matrices.data()+64,&nonfinite,sizeof(nonfinite));
    CHECK(!submission_matches(&bad,2,3,1,7,bad.renderer_matrices.data(),40));
    bad=bound;std::memcpy(&bad.key.stamp,&nonfinite,sizeof(nonfinite));CHECK(!submission(&bad));
    bad=bound;bad.anchor.location[0]=nonfinite;CHECK(!submission(&bad));
    bad=bound;bad.anchor.eye_height=nonfinite;CHECK(!submission(&bad));
    bad=bound;bad.anchor.base_eye_height=nonfinite;CHECK(!submission(&bad));
    bad=bound;bad.anchor.location[2]=bad.anchor.eye_height=std::numeric_limits<float>::max();CHECK(!submission(&bad));
    // A rejected stale source binding has no persistent stop effect. A fresh
    // current one-use family scope supplies its own invocation/source snapshot.
    auto next=r;next.invocation=2;next.key.source=4;next.scene_qpc=50;next.key.matrices[64]=1;
    CHECK(!submission_matches(&bound,2,4,1,7,next.key.matrices.data(),80));
    CHECK(offer(c,next)==accepted);
    {Context::FamilyScope family(c,next.key,next.anchor,7,60,true,1,0x2c8909);CHECK(family.reason==accepted);
        Receipt current{};CHECK(c.consume(next.key,next.anchor,7,70,true,1,0x3a1595,current)==accepted);
        CHECK(submission_matches(&current,2,4,1,7,next.key.matrices.data(),80));
        CHECK(c.consume(next.key,next.anchor,7,71,true,1,0x3a1595,current)==already_consumed);}
    CHECK(offer(c,r)==accepted);CHECK(offer(c,r)==ambiguous);CHECK(offer(c,r)==accepted); // finite boundary recovery
    {Context::FamilyScope family(c,r.key,r.anchor,7,20,true,1,0x2c8909);
        {Context::FamilyScope nestedfamily(c,r.key,r.anchor,7,21,true,1,0x2c8909);CHECK(nestedfamily.reason==nested);}
        CHECK(c.consume(r.key,r.anchor,7,30,true,1,0x3a1595,output)==nested);}
    CHECK(offer(c,r)==accepted);
    {Context::FamilyScope family(c,r.key,r.anchor,7,20,true,1,0x2c8909);
        CHECK(c.consume(r.key,r.anchor,7,30,true,1,123,output)==caller_mismatch);
        CHECK(c.consume(r.key,r.anchor,7,31,true,1,0x3a1595,output)==already_consumed);}
    CHECK(offer(c,r)==accepted);auto changed=r.key;changed.stamp++;
    {Context::FamilyScope family(c,changed,r.anchor,7,20,true,1,0x2c8909);CHECK(family.reason==stamp_mismatch);}
    CHECK(offer(c,r)==accepted);changed=r.key;changed.matrices[64]=1;
    {Context::FamilyScope family(c,changed,r.anchor,7,20,true,1,0x2c8909);CHECK(family.reason==accepted);
        CHECK(c.consume(changed,r.anchor,7,30,true,1,0x3a1595,output)==accepted);CHECK(output.family_matrices[64]==1);}
    CHECK(offer(c,r)==accepted);changed=r.key;changed.matrices[0]=1;
    {Context::FamilyScope family(c,changed,r.anchor,7,20,true,1,0x2c8909);CHECK(family.reason==matrix_mismatch);}
    CHECK(offer(c,r)==accepted);
    {Context::FamilyScope family(c,r.key,r.anchor,8,20,true,1,0x2c8909);CHECK(family.reason==no_receipt);}
    CHECK(offer(c,r)==accepted);
    {Context::FamilyScope family(c,r.key,r.anchor,7,20,true,2,0x2c8909);CHECK(family.reason==multiple_views);}
    c.begin_scene();c.begin_scene();CHECK(c.offer(r)==nested);c.end_scene();CHECK(c.offer(r)==nested);c.end_scene();
    CHECK(offer(c,r)==accepted);{Context::FamilyScope family(c,r.key,r.anchor,7,20,true,1,0x2c8909);
        c.begin_scene();CHECK(c.offer(r)==nested);c.end_scene();CHECK(c.consume(r.key,r.anchor,7,30,true,1,0x3a1595,output)==nested);}
    Context other;CHECK(other.consume(r.key,r.anchor,7,30,true,1,0x3a1595,output)==no_receipt);
    // Full production guard against synthetic registered objects, including
    // legal count growth between calls and corruption within a timestamp bracket.
    std::vector<unsigned char> mem(300000);
    auto put=[&](std::uintptr_t address,auto const& value){std::memcpy(mem.data()+address,&value,sizeof(value));};
    auto read=[&](void* dest,const void* ptr,std::size_t size){auto address=reinterpret_cast<std::uintptr_t>(ptr);
        if(address+size>mem.size())return false;std::memcpy(dest,mem.data()+address,size);return true;};
    Config cfg{};cfg.objects_header=100;cfg.names_header=200;cfg.identity_count=9;
    cfg.object_internal_index=8;cfg.object_name=12;cfg.object_class=24;cfg.object_outer=32;cfg.field_super=96;
    cfg.name_entry_index=0;cfg.name_entry_text=8;
    cfg.role_controller=0;cfg.role_local=1;cfg.role_camera=2;cfg.role_pawn=3;cfg.role_level=4;
    cfg.controller_pawn=120;cfg.pawn_controller=120;cfg.player_camera=128;cfg.pc_owner=120;cfg.player=136;
    cfg.camera_view_target=128;cfg.camera_timestamp=140;cfg.pawn_location=140;cfg.eye_height=152;cfg.base_eye_height=156;
    Registry objects{1000,2001,2100},names{20000,2001,2100};put(100,objects);put(200,names);
    const wchar_t* labels[]={L"PC",L"Local",L"Camera",L"Pawn",L"LevelObj",L"WarLocalPlayer",L"WarPlayerCamera",L"WarPawn",L"Level"};
    for(unsigned i=0;i<9;++i){auto& id=cfg.identities[i];id.address=40000+i*256;id.index=id.name_index=i;
        id.cls=i==1?41280:i==2?41536:i==3?41792:i==4?42048:41280;id.outer=i==3?41024:0;
        id.check_super=i>=5;std::wcsncpy(id.name.data(),labels[i],63);put(id.address+8,id.index);put(id.address+12,id.name_index);
        put(id.address+24,id.cls);put(id.address+32,id.outer);put(objects.data+i*8,id.address);
        std::uintptr_t entry=50000+i*256;put(names.data+i*8,entry);put(entry,id.name_index);
        std::memcpy(mem.data()+entry+8,id.name.data(),sizeof(id.name));}
    auto pc=cfg.identities[0].address,local=cfg.identities[1].address,camera=cfg.identities[2].address,
        pawn=cfg.identities[3].address,level=cfg.identities[4].address;
    put(pc+120,pawn);put(pawn+120,pc);put(pc+128,camera);put(camera+120,pc);put(pc+136,local);put(camera+128,pawn);
    put(camera+140,1.f);put(pawn+140,std::array<float,3>{1,2,3});put(pawn+152,4.f);put(pawn+156,5.f);
    Anchor anchor{};std::uint32_t stamp{};CHECK(guard(cfg,read,anchor,stamp)&&anchor.level==level);
    objects.count++;names.count++;put(100,objects);put(200,names);CHECK(guard(cfg,read,anchor,stamp));
    put(objects.data+3*8,std::uintptr_t(999));CHECK(!guard(cfg,read,anchor,stamp));put(objects.data+3*8,pawn);
    auto corrupt_read=[&](void* dest,const void* ptr,std::size_t size){auto ok=read(dest,ptr,size);
        if(reinterpret_cast<std::uintptr_t>(ptr)==pawn+140)put(camera+140,2.f);return ok;};
    CHECK(!guard(cfg,corrupt_read,anchor,stamp));CHECK(guard(cfg,read,anchor,stamp));
    put(pawn+140,std::array<float,3>{1,2,std::numeric_limits<float>::max()});put(pawn+152,std::numeric_limits<float>::max());
    CHECK(!guard(cfg,read,anchor,stamp));
    put(pawn+140,std::array<float,3>{1,2,3});put(pawn+152,4.f);
    PawnBinding before_scene{};CHECK(capture_pawn_binding(cfg,read,before_scene));
    CHECK(guard(cfg,read,anchor,stamp)&&scene_binding_matches(before_scene,anchor));
    // Pawn position and camera time may advance inside original_scene. Only
    // the possession/identity bracket is required to remain unchanged.
    put(pawn+140,std::array<float,3>{8,9,10});put(camera+140,3.f);
    CHECK(guard(cfg,read,anchor,stamp)&&scene_binding_matches(before_scene,anchor));
    auto original_config=cfg;
    const uintptr_t new_pawn=60000,new_name_entry=62000;
    const unsigned new_index=50,new_name_index=50,new_number=2;
    put(new_pawn+8,new_index);put(new_pawn+12,new_name_index);put(new_pawn+16,new_number);
    put(new_pawn+24,cfg.identities[3].cls);put(new_pawn+32,cfg.identities[3].outer);
    put(new_pawn+120,pc);put(new_pawn+140,std::array<float,3>{11,12,13});put(new_pawn+152,4.f);put(new_pawn+156,5.f);
    put(objects.data+new_index*8,new_pawn);put(names.data+new_name_index*8,new_name_entry);put(new_name_entry,new_name_index);
    std::array<wchar_t,64> new_name{};std::copy_n(L"Pawn_Respawn",sizeof(L"Pawn_Respawn")/sizeof(wchar_t),new_name.begin());put(new_name_entry+8,new_name);
    put(pc+120,new_pawn);put(camera+128,new_pawn);
    CHECK(guard(cfg,read,anchor,stamp)&&anchor.pawn==new_pawn&&anchor.pawn_identity.number==new_number);
    CHECK(!scene_binding_matches(before_scene,anchor));
    CHECK(equal_identity(cfg.identities[3],original_config.identities[3])); // No global mutation.
    CHECK(cfg.identities[3].address==pawn);CHECK(memcmp(&cfg,&original_config,sizeof(cfg))==0);
    // Old Pawn remains registered and readable: it is deliberately not used.
    CHECK(capture_pawn_binding(cfg,read,before_scene));CHECK(scene_binding_matches(before_scene,anchor));
    auto stable_memory=mem;
    PawnBindingDiagnostic binding_diagnostic{};
    CHECK(capture_pawn_binding(cfg,read,before_scene,&binding_diagnostic)&&binding_diagnostic.stage==PawnBindingStage::success);
    CHECK(!std::strcmp(pawn_binding_stage_name(binding_diagnostic.stage),"success"));
    CHECK(!std::strcmp(pawn_binding_field_name(PawnBindingField::camera_view_target),"camera_view_target"));
    const PawnBindingField link_fields[]={PawnBindingField::controller_pawn,PawnBindingField::pawn_controller,
        PawnBindingField::camera_view_target,PawnBindingField::player_camera,PawnBindingField::pc_owner,PawnBindingField::player};
    const uintptr_t link_addresses[]={pc+120,new_pawn+120,camera+128,pc+128,camera+120,pc+136};
    const uintptr_t link_expected[]={new_pawn,pc,new_pawn,camera,pc,local};
    for(unsigned fault=0;fault<8;++fault){
        mem=stable_memory;
        const auto address=fault<6?link_addresses[fault]:new_pawn+(fault==6?24:32);
        const auto expected=fault<6?link_expected[fault]:fault==6?cfg.identities[3].cls:cfg.identities[3].outer;
        const uintptr_t actual=fault==0?0:fault==2?pawn:expected+256;
        put(address,actual);auto sentinel=before_scene;
        CHECK(!capture_pawn_binding(cfg,read,sentinel,&binding_diagnostic)&&equal_binding(sentinel,before_scene));
        CHECK(binding_diagnostic.stage==(fault==0?PawnBindingStage::pawn:fault<6?PawnBindingStage::links:PawnBindingStage::identity));
        CHECK(binding_diagnostic.field==(fault<6?link_fields[fault]:fault==6?PawnBindingField::cls:PawnBindingField::outer));
        CHECK(binding_diagnostic.address==address&&binding_diagnostic.actual_valid&&binding_diagnostic.actual==actual);
        if(fault)CHECK(binding_diagnostic.expected_valid&&binding_diagnostic.expected==expected);
        CHECK(!capture_pawn_binding(cfg,read,sentinel));
    }
    mem=stable_memory;
    std::vector<std::pair<uintptr_t,std::size_t>> plain_reads,diagnostic_reads;
    auto trace=[&](auto& rows){return [&](void* dest,void const* ptr,std::size_t size){rows.emplace_back(reinterpret_cast<uintptr_t>(ptr),size);return read(dest,ptr,size);};};
    CHECK(capture_pawn_binding(cfg,trace(plain_reads),before_scene));
    CHECK(capture_pawn_binding(cfg,trace(diagnostic_reads),before_scene,&binding_diagnostic)&&plain_reads==diagnostic_reads);
    put(objects.data+cfg.identities[3].index*8,uintptr_t(0));put(pawn+24,uintptr_t(0));
    CHECK(guard(cfg,read,anchor,stamp)); // Obsolete startup Pawn is no longer certified.
    mem=stable_memory;
    for(unsigned fault=0;fault<13;++fault){mem=stable_memory;
        if(fault==0)put(new_pawn+24,cfg.identities[2].cls);
        if(fault==1)put(new_pawn+32,uintptr_t(level+256));
        if(fault==2)put(pc+128,uintptr_t(camera+256));
        if(fault==3)put(camera+120,uintptr_t(pc+256));
        if(fault==4)put(level+32,uintptr_t(123));
        if(fault==5)put(new_pawn+8,cfg.identities[0].index); // Aliased registered slot.
        if(fault==6)put(pc+120,pc); // Aliased certificate object.
        if(fault==7)put(new_pawn+8,unsigned(objects.count));
        if(fault==8)put(new_pawn+12,unsigned(names.count));
        if(fault==9)put(new_name_entry,new_name_index+1);
        if(fault==10)put(new_pawn+120,uintptr_t(pc+256));
        if(fault==11)put(camera+128,pawn);
        if(fault==12)put(pc+24,uintptr_t(123));
        CHECK(!guard(cfg,read,anchor,stamp));
        PawnBinding plain{},diagnosed{};
        CHECK(capture_pawn_binding(cfg,read,plain)==capture_pawn_binding(cfg,read,diagnosed,&binding_diagnostic));}
    for(unsigned fault=0;fault<3;++fault){mem=stable_memory;auto text=new_name;
        if(fault==0)std::copy_n(L"Default__WarPawn",sizeof(L"Default__WarPawn")/sizeof(wchar_t),text.begin());
        if(fault==1)text.fill(L'x');if(fault==2)text.fill(0);
        put(new_name_entry+8,text);CHECK(!guard(cfg,read,anchor,stamp));}
    mem=stable_memory;
    unsigned pawn_header_reads=0;
    auto changing_identity=[&](void* dest,const void* ptr,std::size_t size){auto ok=read(dest,ptr,size);
        if(reinterpret_cast<uintptr_t>(ptr)==new_pawn&&size==96&&++pawn_header_reads==1)put(new_pawn+16,new_number+1);
        return ok;};
    CHECK(!guard(cfg,changing_identity,anchor,stamp));
    mem=stable_memory;pawn_header_reads=0;
    CHECK(!capture_pawn_binding(cfg,changing_identity,before_scene,&binding_diagnostic)&&binding_diagnostic.stage==PawnBindingStage::reread_header);
    mem=stable_memory;pawn_header_reads=0;
    auto changing_link=[&](void* dest,const void* ptr,std::size_t size){auto ok=read(dest,ptr,size);
        if(reinterpret_cast<uintptr_t>(ptr)==new_pawn&&size==96&&++pawn_header_reads==2)put(camera+128,pawn);
        return ok;};
    CHECK(!capture_pawn_binding(cfg,changing_link,before_scene,&binding_diagnostic)&&binding_diagnostic.stage==PawnBindingStage::reread_links&&
        binding_diagnostic.field==PawnBindingField::camera_view_target&&binding_diagnostic.expected==new_pawn&&binding_diagnostic.actual==pawn);
    mem=stable_memory;CHECK(guard(cfg,read,anchor,stamp));
    auto changed_identity=anchor;changed_identity.pawn_identity.number++;
    CHECK(!equal_anchor(anchor,changed_identity));CHECK(!scene_binding_matches(before_scene,changed_identity));
    auto observed=r;observed.anchor=anchor;Context reassociated;
    CHECK(offer(reassociated,observed)==accepted);
    {Context::FamilyScope family(reassociated,observed.key,changed_identity,7,20,true,1,0x2c8909);
        CHECK(family.reason==identity_failed);}
    CHECK(offer(reassociated,observed)==accepted);
    {Context::FamilyScope family(reassociated,observed.key,anchor,7,20,true,1,0x2c8909);CHECK(family.reason==accepted);
        CHECK(reassociated.consume(observed.key,changed_identity,7,30,true,1,0x3a1595,output)==identity_failed);}
    // Snapshot reuse of the startup address with a changed FName signature is
    // refused rather than accepting a replacement solely by its pointer.
    mem=stable_memory;put(pc+120,pawn);put(camera+128,pawn);put(pawn+16,1u);
    CHECK(!guard(cfg,read,anchor,stamp));
    // Startup retries only a timestamp race whose complete identity bracket
    // passed. Inject camera changes at exact reads, not generic false returns.
    mem=stable_memory;put(camera+140,1.f);
    std::uint64_t milliseconds=0;unsigned sleeps=0,stamp_reads=0;
    auto clock=[&](){return milliseconds;};auto pause=[&](unsigned ms){milliseconds+=ms;++sleeps;};
    BoundedGuardDiagnostic diagnostic{};GuardDiagnostic single{};
    Anchor sentinel{};sentinel.pawn=123456;sentinel.location={9,8,7};const std::uint32_t sentinel_stamp=99;
    auto unchanged=[&](){return equal_anchor(anchor,sentinel)&&stamp==sentinel_stamp;};
    auto reset=[&](){mem=stable_memory;put(camera+140,1.f);milliseconds=0;sleeps=stamp_reads=0;anchor=sentinel;stamp=sentinel_stamp;};
    reset();auto race_once=[&](void* dest,const void* ptr,std::size_t size){
        if(reinterpret_cast<uintptr_t>(ptr)==camera+140&&++stamp_reads==2)put(camera+140,2.f);
        return read(dest,ptr,size);};
    CHECK(bounded_guard(cfg,race_once,clock,pause,anchor,stamp,&diagnostic));
    CHECK(diagnostic.attempts==2&&sleeps==1&&diagnostic.reason==GuardReason::success&&anchor.pawn==new_pawn);
    reset();CHECK(!guard(cfg,race_once,anchor,stamp,&single));
    CHECK(single.reason==GuardReason::strict_timestamp_changed&&single.timestamp_only&&single.before_stamp!=single.strict_after_stamp&&unchanged());
    reset();auto final_race=[&](void* dest,const void* ptr,std::size_t size){
        if(reinterpret_cast<uintptr_t>(ptr)==camera+140&&++stamp_reads==3)put(camera+140,2.f);
        return read(dest,ptr,size);};
    CHECK(!guard(cfg,final_race,anchor,stamp,&single));
    CHECK(single.reason==GuardReason::final_timestamp_changed&&single.before_stamp==single.strict_after_stamp&&single.final_stamp!=single.before_stamp&&unchanged());
    reset();CHECK(bounded_guard(cfg,final_race,clock,pause,anchor,stamp,&diagnostic)&&diagnostic.attempts==2);
    reset();auto constant_race=[&](void* dest,const void* ptr,std::size_t size){
        if(reinterpret_cast<uintptr_t>(ptr)==camera+140&&++stamp_reads%3==2)put(camera+140,float(stamp_reads));
        return read(dest,ptr,size);};
    CHECK(!bounded_guard(cfg,constant_race,clock,pause,anchor,stamp,&diagnostic));
    CHECK(diagnostic.attempts==5&&sleeps==4&&diagnostic.reason==GuardReason::attempt_limit&&unchanged());
    reset();auto invalidate_during_pause=[&](unsigned ms){milliseconds+=ms;++sleeps;put(pc+24,uintptr_t(123));};
    CHECK(!bounded_guard(cfg,race_once,clock,invalidate_during_pause,anchor,stamp,&diagnostic));
    CHECK(diagnostic.attempts==2&&sleeps==1&&diagnostic.reason==GuardReason::strict_validation&&unchanged());
    reset();put(new_pawn+24,uintptr_t(123));CHECK(!bounded_guard(cfg,read,clock,pause,anchor,stamp,&diagnostic));
    CHECK(diagnostic.attempts==1&&!sleeps&&diagnostic.reason==GuardReason::initial_binding&&unchanged());
    reset();put(pc+24,uintptr_t(123));CHECK(!bounded_guard(cfg,read,clock,pause,anchor,stamp,&diagnostic));
    CHECK(diagnostic.attempts==1&&!sleeps&&diagnostic.reason==GuardReason::strict_validation&&unchanged());
    reset();auto hard_read=[&](void* dest,const void* ptr,std::size_t size){
        if(reinterpret_cast<uintptr_t>(ptr)==new_pawn+140)return false;return read(dest,ptr,size);};
    CHECK(!bounded_guard(cfg,hard_read,clock,pause,anchor,stamp,&diagnostic));
    CHECK(diagnostic.attempts==1&&!sleeps&&diagnostic.reason==GuardReason::read_failure&&diagnostic.last.stage==GuardStage::strict_validation&&unchanged());
    reset();auto initial_read_fail=[&](void* dest,const void* ptr,std::size_t size){
        if(reinterpret_cast<uintptr_t>(ptr)==cfg.objects_header)return false;return read(dest,ptr,size);};
    CHECK(!bounded_guard(cfg,initial_read_fail,clock,pause,anchor,stamp,&diagnostic));
    CHECK(diagnostic.attempts==1&&!sleeps&&diagnostic.reason==GuardReason::read_failure&&diagnostic.last.stage==GuardStage::initial_binding&&unchanged());
    reset();unsigned final_headers=0;auto final_read_fail=[&](void* dest,const void* ptr,std::size_t size){
        if(reinterpret_cast<uintptr_t>(ptr)==new_pawn&&size==96&&++final_headers==5)return false;return race_once(dest,ptr,size);};
    CHECK(!bounded_guard(cfg,final_read_fail,clock,pause,anchor,stamp,&diagnostic));
    CHECK(diagnostic.attempts==1&&!sleeps&&diagnostic.reason==GuardReason::read_failure&&diagnostic.last.stage==GuardStage::final_binding&&unchanged());
    reset();auto identity_and_time=[&](void* dest,const void* ptr,std::size_t size){
        if(reinterpret_cast<uintptr_t>(ptr)==camera+140&&++stamp_reads==2){put(camera+140,2.f);put(new_pawn+16,new_number+1);}
        return read(dest,ptr,size);};
    CHECK(!bounded_guard(cfg,identity_and_time,clock,pause,anchor,stamp,&diagnostic));
    CHECK(diagnostic.attempts==1&&!sleeps&&diagnostic.reason==GuardReason::final_binding&&!diagnostic.last.timestamp_only&&unchanged());
    reset();auto link_and_time=[&](void* dest,const void* ptr,std::size_t size){
        if(reinterpret_cast<uintptr_t>(ptr)==camera+140&&++stamp_reads==2){put(camera+140,2.f);put(new_pawn+120,uintptr_t(123));}
        return read(dest,ptr,size);};
    CHECK(!bounded_guard(cfg,link_and_time,clock,pause,anchor,stamp,&diagnostic));
    CHECK(diagnostic.attempts==1&&!sleeps&&diagnostic.reason==GuardReason::strict_validation&&!diagnostic.last.timestamp_only&&unchanged());
    reset();unsigned clock_calls=0;auto late_clock=[&]()->std::uint64_t{return ++clock_calls>=3?200:0;};
    CHECK(!bounded_guard(cfg,read,late_clock,pause,anchor,stamp,&diagnostic));
    CHECK(diagnostic.attempts==1&&diagnostic.reason==GuardReason::deadline&&diagnostic.last.reason==GuardReason::success&&unchanged());
    reset();auto over_budget_pause=[&](unsigned){milliseconds=200;++sleeps;};
    CHECK(!bounded_guard(cfg,constant_race,clock,over_budget_pause,anchor,stamp,&diagnostic));
    CHECK(diagnostic.attempts==1&&diagnostic.reason==GuardReason::deadline&&sleeps==1&&unchanged());
    reset();auto slow_race=[&](void* dest,const void* ptr,std::size_t size){auto ok=constant_race(dest,ptr,size);
        if(reinterpret_cast<uintptr_t>(ptr)==camera+140&&stamp_reads%3==0)milliseconds+=80;return ok;};
    CHECK(!bounded_guard(cfg,slow_race,clock,pause,anchor,stamp,&diagnostic));
    CHECK(diagnostic.attempts==2&&diagnostic.reason==GuardReason::deadline&&unchanged());
    std::printf("anchor scope/binding/submission, unpossessed CameraPOV and bounded startup timestamp-race CPU tests passed\n");return 0;
}
