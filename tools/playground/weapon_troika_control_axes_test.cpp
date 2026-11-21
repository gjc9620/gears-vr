// Actual reflection/reader over owned synthetic original-control instances.
// No engine dispatch, game process, memory writer outside these owned pages.
#include "weapon_hand_offline_fixture.h"
#include <cmath>
namespace {
#include "weapon_troika_reflection.inl"
namespace troika_mounted_parts {
namespace socket_contract {namespace metadata=weaponhand::meta;using U32=std::uint32_t;
#include "weapon_muzzle_getter_call.inl"
}
#include "weapon_troika_control_axes.inl"
#include "weapon_troika_enum_getter.inl"
}
namespace axes=troika_mounted_parts::control_axes;
struct AxisMemory:Memory {
    uptr turret_class{},control_base{},control_class{},object_class{},enum_class{},axis_enum{},space_enum{};
    uptr turret{},latitude{},longitude{},byte_class{},float_class{},name_class{},struct_class{},vector_type{};
    uptr cached_lat{},cached_long{},component_field{},axis_field{},rotation_field{},control_name{},target_field{};
    std::array<uptr,11> bits{};std::array<uptr,4> floats{};
    AxisMemory(){
        object_class=cls(L"Object",core);enum_class=cls(L"Enum",core);
        control_base=cls(L"SkelControlBase",engine,object_class);control_class=cls(L"SkelControlLookAt",engine,control_base);
        turret_class=cls(L"Turret_TroikaCabal_base",warfare,pawn_class);
        turret=node(L"OriginalTurret",turret_class,world);
        latitude=node(L"OriginalLatitude",control_class,mesh);longitude=node(L"OriginalLongitude",control_class,mesh);
        axis_enum=node(L"EAxis",enum_class,object_class);space_enum=node(L"EBoneControlSpace",enum_class,control_base);
        byte_class=cls(L"ByteProperty",core);float_class=cls(L"FloatProperty",core);name_class=cls(L"NameProperty",core);
        struct_class=cls(L"StructProperty",core);const auto script_class=cls(L"ScriptStruct",core);
        vector_type=node(L"Vector",script_class,object_class);put(vector_type,0x88,12u);
        property(vector_type,L"X",float_class,0,4);property(vector_type,L"Y",float_class,4,4);property(vector_type,L"Z",float_class,8,4);
        cached_lat=property(turret_class,L"Pivot_Latitude",object_type,0x130,8);put(cached_lat,0xd0,control_class);
        cached_long=property(turret_class,L"Pivot_Longitude",object_type,0x138,8);put(cached_long,0xd0,control_class);
        put(turret,0x130,latitude);put(turret,0x138,longitude);
        component_field=property(control_base,L"SkelComponent",component_type,0x100,8);put(component_field,0xd0,mesh_class);
        control_name=property(control_base,L"ControlName",name_class,0x108,8);
        const wchar_t* bytes[]={L"LookAtAxis",L"UpAxis",L"AllowRotationSpace",L"TargetLocationSpace"};
        for(unsigned i=0;i<4;++i){const auto p=property(control_class,bytes[i],byte_class,0x110+i,1);
            put(p,0xd0,i<2?axis_enum:space_enum);if(i==0)axis_field=p;if(i==2)rotation_field=p;}
        property(control_class,L"TargetSpaceBoneName",name_class,0x118,8);
        property(control_class,L"AllowRotationOtherBoneName",name_class,0x120,8);
        const wchar_t* bit_labels[]={L"bAllowRotationX",L"bAllowRotationY",L"bAllowRotationZ",L"bInvertLookAtAxis",
            L"bDefineUpAxis",L"bInvertUpAxis",L"bEnableLimit",L"bLimitBasedOnRefPose",L"bDisableBeyondLimit",
            L"bNotifyBeyondLimit",L"bIgnoreWhenNotRendered"};
        for(unsigned i=0;i<11;++i)bits[i]=property(i==10?control_base:control_class,bit_labels[i],bool_type,0x128,4,1u<<i);
        const wchar_t* f[]={L"MaxAngle",L"DeadZoneAngle",L"ControlStrength",L"LookAtAlpha"};
        for(unsigned i=0;i<4;++i)floats[i]=property(control_class,f[i],float_class,0x12c+i*4,4);
        const wchar_t* vectors[]={L"TargetLocation",L"BaseLookDir",L"LimitLookDir",L"BaseBonePos"};
        for(unsigned i=0;i<4;++i){const auto p=property(control_class,vectors[i],struct_class,0x140+i*12,12);put(p,0xd0,vector_type);if(!i)target_field=p;}
        const auto none=text(L"None");
        for(unsigned i=0;i<2;++i){const auto c=i?longitude:latitude;
            put(c,0x100,mesh);put(c,0x108,text(i?L"Pivot_Longitude":L"Pivot_Latitude"));
            for(unsigned j=0;j<4;++j)put(c,0x110+j,static_cast<unsigned char>(j<2?j+1:j+4)); // Deliberately NOT our ordinal convention.
            put(c,0x118,none);put(c,0x120,none);put(c,0x128,(1u<<i)|(1u<<7));
            put(c,0x12c,80.f);put(c,0x130,0.f);put(c,0x134,1.f);put(c,0x138,1.f);
            for(unsigned j=0;j<4;++j)put(c,0x140+j*12,std::array<float,3>{float(j+1),float(i+2),float(j+3)});
        }
        tables();
    }
    bool sample(axes::Snapshot& output,std::function<bool()> current=[](){return true;}){
        return axes::read(identity(turret),identity(mesh),current,output);
    }
};
struct EnumMemory:AxisMemory {
    uptr fn{},input_enum{},input_index{},returned{},function_class{};
    std::array<meta::RawName,4> results{};unsigned calls{},result_mode{};bool fail_dispatch{};
    std::function<void()> after_call;
    EnumMemory(){
        function_class=cls(L"Function",core);fn=node(L"GetEnum",function_class,object_class);
        input_enum=property(fn,L"E",object_type,0,8);put(input_enum,0xd0,enum_class);put(input_enum,0x78,std::uint64_t(0x80));
        input_index=property(fn,L"I",cls(L"IntProperty",core),8,4);put(input_index,0x78,std::uint64_t(0x80));
        returned=property(fn,L"ReturnValue",name_class,16,8);put(returned,0x78,std::uint64_t(0x480));
        put(fn,0x88,24u);put(fn,0x106,std::uint16_t(24));put(fn,0x108,std::uint16_t(16));
        results={text(L"AXIS_Z"),text(L"AXIS_X"),text(L"BCS_BoneSpace"),text(L"BCS_WorldSpace")};tables();
    }
    struct Call {
        EnumMemory* memory{};bool valid{true};
        struct Request {uptr function{};unsigned size{24};unsigned char params[64]{};} r;
        bool run(){
            auto& m=*memory;++m.calls;if(m.fail_dispatch)return false;
            uptr e{};int i{};std::memcpy(&e,r.params,8);std::memcpy(&i,r.params+8,4);
            const auto selected=e==m.axis_enum?(i==1?0u:i==2?1u:4u):e==m.space_enum?(i==6?2u:i==7?3u:4u):4u;
            if(selected==4)return false;
            auto result=m.results[selected];
            if(m.result_mode==1)result=m.text(L"None");
            if(m.result_mode==2)result.number=1;
            if(m.result_mode==3&&m.calls>4)result=m.results[(selected+1)%4];
            std::memcpy(r.params+16,&result,8);if(m.after_call)m.after_call();return true;
        }
    };
    auto factory(){return [this](uptr receiver){Call c{};c.memory=this;c.r.function=fn;c.valid=receiver==turret;return c;};}
};
unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::printf("failed %d: %s\n",__LINE__,#x);return 1;}}while(0)
}
#if !defined(GEARS_TROIKA_AXES_FIXTURE_ONLY)
int main(){
    axes::Snapshot sentinel{};std::memset(&sentinel,0x57,sizeof sentinel);
    {
        auto m=std::make_unique<AxisMemory>();axes::Snapshot out=sentinel;
        CHECK(m->sample(out));CHECK(out.controls[0].object.address==m->latitude&&out.controls[1].object.address==m->longitude);
        CHECK(out.controls[0].flags==129&&out.controls[1].flags==130);
        CHECK(out.controls[0].bytes[2].value==6&&out.controls[0].bytes[2].enumeration.address==m->space_enum);
        CHECK(out.controls[0].bytes[0].value==1&&out.controls[0].bytes[0].enumeration.address==m->axis_enum);
        CHECK(out.controls[0].max_angle==80&&out.controls[0].strength==1);
        auto const original=out;CHECK(m->sample(out));CHECK(!std::memcmp(&out,&original,sizeof out));
    }
    for(unsigned fault=0;fault<26;++fault){
        auto m=std::make_unique<AxisMemory>();axes::Snapshot out=sentinel;
        if(fault==0)m->put(m->turret,0x130,uptr(0));
        if(fault==1)m->put(m->turret,0x138,m->latitude);
        if(fault==2)m->put(m->latitude,0x100,m->weapon);
        if(fault==3)m->put(m->latitude,0x108,m->text(L"Pivot_Longitude"));
        if(fault==4)m->put(m->latitude,0x108,meta::RawName{m->text(L"Pivot_Latitude").index,1});
        if(fault==5)m->put(m->latitude,0x50,m->actor_class);
        if(fault==6)m->put(m->cached_lat,0x50,m->component_type);
        if(fault==7)m->put(m->component_field,0x50,m->object_type);
        if(fault==8)m->put(m->cached_lat,0xd0,m->pawn_class);
        if(fault==9)m->put(m->axis_field,0xd0,m->space_enum);
        if(fault==10)m->put(m->rotation_field,0xd0,m->axis_enum);
        if(fault==11)m->put(m->axis_enum,0x50,m->class_type);
        if(fault==12)m->put(m->space_enum,0x40,m->actor_class);
        if(fault==13)m->put(m->bits[1],0xd0,1u); // Overlapping masks cannot describe two independent axis flags.
        if(fault==14)m->put(m->bits[1],0xd0,3u);
        if(fault==15)m->put(m->bits[1],0x94,0x129u); // Partly overlapping words.
        if(fault==16)m->put(m->floats[0],0x50,m->object_type);
        if(fault==17)m->put(m->latitude,0x12c,std::numeric_limits<float>::quiet_NaN());
        if(fault==18)m->put(m->longitude,0x140,std::numeric_limits<float>::infinity());
        if(fault==19)m->put(m->target_field,0xd0,m->pawn_class);
        if(fault==20)m->put(m->target_field,0x94,506u);
        if(fault==21)fail_read=m->latitude+0x128;
        if(fault==22)m->put(m->cached_long,0x68,m->cached_lat);
        if(fault==23)m->put(m->latitude,8,999u);
        if(fault==24){m->property(m->control_base,L"ControlName",m->name_class,0x108,8);m->tables();}
        if(fault==25){const auto replacement=m->cls(L"SkelControlLookAt",m->engine,m->control_base);m->put(m->cached_lat,0xd0,replacement);m->tables();}
        const bool accepted=m->sample(out);if(accepted)std::printf("unexpected accepted fault=%u\n",fault);
        CHECK(!accepted);CHECK(!std::memcmp(&out,&sentinel,sizeof out));
    }
    for(unsigned mutation_case=0;mutation_case<6;++mutation_case){
        auto m=std::make_unique<AxisMemory>();axes::Snapshot out=sentinel;unsigned current_calls{};
        CHECK(!m->sample(out,[&](){++current_calls;if(current_calls==2){
            if(mutation_case==0)m->put(m->turret,0x138,m->latitude);
            if(mutation_case==1)m->put(m->latitude,0x110,static_cast<unsigned char>(2));
            if(mutation_case==2)m->put(m->latitude,0x128,0u);
            if(mutation_case==3)m->put(m->latitude,0x12c,81.f);
            if(mutation_case==4)m->put(m->component_field,0xd0,m->weapon_class);
            if(mutation_case==5)return false;
        }return true;}));
        CHECK(current_calls==2);CHECK(!std::memcmp(&out,&sentinel,sizeof out));
    }
    {
        auto m=std::make_unique<AxisMemory>();axes::Snapshot out=sentinel;
        mutate_read=m->latitude+0x140;mutation=[&](){m->put(m->latitude,0x140,123.f);};
        CHECK(!m->sample(out));CHECK(!std::memcmp(&out,&sentinel,sizeof out));
        CHECK(!m->sample(out,[](){return false;}));CHECK(!std::memcmp(&out,&sentinel,sizeof out));
    }
    {
        auto m=std::make_unique<EnumMemory>();axes::Snapshot config{};CHECK(m->sample(config));axes::Decoded decoded{};
        CHECK(axes::decode(m->identity(m->turret),config,m->factory(),[](){return true;},decoded));
        CHECK(decoded.look[0]==axes::Axis::z&&decoded.up[0]==axes::Axis::x);
        CHECK(decoded.rotation[0]==axes::Space::bone&&decoded.target[0]==axes::Space::world);
        CHECK(decoded.look[1]==decoded.look[0]&&m->calls==8); // Four distinct values, each independently rechecked.
    }
    for(unsigned fault=0;fault<18;++fault){
        auto m=std::make_unique<EnumMemory>();axes::Snapshot config{};CHECK(m->sample(config));
        axes::Decoded saved{};std::memset(&saved,0x56,sizeof saved);auto decoded=saved;
        if(fault==0)m->put(m->fn,0x40,m->pawn_class);
        if(fault==1)m->put(m->fn,0x50,m->class_type);
        if(fault==2)m->put(m->fn,0x106,std::uint16_t(23));
        if(fault==3)m->put(m->fn,0x108,std::uint16_t(8));
        if(fault==4)m->put(m->input_enum,0xd0,m->pawn_class);
        if(fault==5)m->put(m->input_index,0x94,4u); // Aliases enum pointer bytes.
        if(fault==6)m->put(m->input_index,0x50,m->float_class);
        if(fault==7)m->put(m->returned,0x78,std::uint64_t(0x80));
        if(fault==8)m->put(m->returned,0x68,m->input_enum);
        if(fault==9)m->fail_dispatch=true;
        if(fault==10)m->result_mode=1;
        if(fault==11)m->result_mode=2;
        if(fault==12)m->result_mode=3;
        if(fault==13)m->after_call=[&](){m->put(m->input_index,0x94,12u);};
        if(fault==14)m->after_call=[&](){m->put(m->axis_enum,8,999u);};
        if(fault==15){const auto p=m->property(m->fn,L"I",m->object_type,8,4);m->put(p,0x78,std::uint64_t(0x80));m->tables();}
        if(fault==16)m->put(m->fn,0x108,std::uint16_t(17));
        if(fault==17){const auto replacement=m->cls(L"Enum",m->core);m->tables();m->after_call=[&,replacement](){m->put(m->input_enum,0xd0,replacement);};}
        CHECK(!axes::decode(m->identity(m->turret),config,m->factory(),[](){return true;},decoded));
        CHECK(!std::memcmp(&decoded,&saved,sizeof decoded));
    }
    {
        auto m=std::make_unique<EnumMemory>();axes::Snapshot config{};CHECK(m->sample(config));
        axes::Decoded saved{};std::memset(&saved,0x56,sizeof saved);auto decoded=saved;
        CHECK(!axes::decode(m->identity(m->turret),config,m->factory(),[](){return false;},decoded));
        CHECK(!m->calls&&!std::memcmp(&decoded,&saved,sizeof decoded));
    }
    std::printf("Troika original controls/GetEnum: %u checks passed; actual Reader/schema over owned synthetic objects and emulated getter dispatch, no original engine/aim/fire proof.\n",checks);
}
#endif
