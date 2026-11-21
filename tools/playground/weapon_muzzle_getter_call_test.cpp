// Actual production schema reader and local parameter transfer, owned memory
// only. No engine dispatch, Native registration, pose or action authority.
#define GEARS_WEAPON_METADATA_PARSE_TEST_STATS 1
#include <cstdio>
namespace {FILE* log_file{};}
#include "weapon_hand_offline_fixture.h"
namespace {
namespace getter {
namespace metadata=weaponhand::meta;
using U32=std::uint32_t;
#include "weapon_muzzle_getter_call.inl"
}
using Layout=getter::SocketGetterLayout;
struct Call {
    struct Request {uptr function{};unsigned size{};unsigned char params[4096]{};} r;
    bool valid=true;
};
struct GetterMemory:Memory {
    uptr function{},in{},location{},rotation{},returned{},space{};
    Call call;
    explicit GetterMemory(bool optional_space=true){
        const auto function_type=cls(L"Function",core),name_type=cls(L"NameProperty",core);
        const auto struct_type=cls(L"StructProperty",core),script_type=cls(L"ScriptStruct",core);
        const auto int_type=cls(L"IntProperty",core),object_owner=node(L"Object",class_type,core);
        const auto vector=node(L"Vector",script_type,object_owner),rotator=node(L"Rotator",script_type,object_owner);
        put(vector,0x88,12u);put(rotator,0x88,12u);
        function=node(L"GetSocketWorldLocationAndRotation",function_type,mesh_class);
        put(function,0x88,40u);put(function,0x106,std::uint16_t(40));put(function,0x108,std::uint16_t(32));
        in=property(function,L"InSocketName",name_type,0,8);put(in,0x78,0x80ull);
        location=property(function,L"OutLocation",struct_type,8,12);put(location,0xd0,vector);put(location,0x78,0x180ull);
        rotation=property(function,L"OutRotation",struct_type,20,12);put(rotation,0xd0,rotator);put(rotation,0x78,0x180ull);
        returned=property(function,L"ReturnValue",bool_type,32,4,1);put(returned,0x78,0x580ull);
        if(optional_space){space=property(function,L"Space",int_type,36,4);put(space,0x78,0x90ull);}
        text(L"Muzzle");tables();call.r.function=function;call.r.size=40;
    }
};
struct Query {
    meta::Snapshot schema{};meta::Reader r{schema,meta::Stage::final_check};
    Layout layout{};unsigned mask{},identities{};
    Query(){schema.module_base=base;assert(meta::tables(base,schema.objects,schema.names));}
    bool begin(Call const& call){
        const auto okay=getter::socket_getter_schema(call,r,mask,&layout);
        identities=schema.identity_count;return okay;
    }
    bool finish(Call const& call){
        Layout after{};unsigned after_mask{};
        return getter::socket_getter_schema(call,r,after_mask,&after)&&schema.identity_count==identities&&
            mask==after_mask&&layout.function==after.function&&layout.bytes==after.bytes&&layout.offsets==after.offsets;
    }
};
unsigned cases{},unique_closing_reads{},duplicated_closing_reads{};
template<class Change>void reject(Change change){
    auto m=std::make_unique<GetterMemory>();change(*m);Query q;Layout untouched{};untouched.bytes=77;unsigned mask{};
    const bool accepted=getter::socket_getter_schema(m->call,q.r,mask,&untouched);
    if(accepted)std::fprintf(stderr,"Unexpected schema acceptance at case %u\n",cases);
    assert(!accepted&&untouched.bytes==77);++cases;
}
template<class Change>void reject_after(Change change){
    auto m=std::make_unique<GetterMemory>();Query q;assert(q.begin(m->call));change(*m);assert(!q.finish(m->call));++cases;
}
}
int main(){
    for(bool optional:{false,true}){
        auto m=std::make_unique<GetterMemory>(optional);Query q;assert(q.begin(m->call));
        assert((q.layout.valid&&q.layout.offsets==std::array<unsigned,4>({0,8,20,32})&&q.mask==1));
        const auto name=m->text(L"Muzzle");m->tables();
        // Parameter transfer consumes the already-proved stack-local offsets.
        // No extra object/property selector or game-memory read is performed.
        unsigned reads{};read_observer=[&](uptr,std::size_t){++reads;};
        assert(q.layout.write(m->call,Layout::input_name,name));
        const std::array<float,3> position{1.25f,-2.5f,3.75f};const std::array<int,3> angle{10,-20,30};
        std::memcpy(m->call.r.params+8,position.data(),12);std::memcpy(m->call.r.params+20,angle.data(),12);
        const unsigned yes=1;std::memcpy(m->call.r.params+32,&yes,4);
        std::array<float,3> actual{};std::array<int,3> rotation{};unsigned returned{};
        assert(q.layout.read(m->call,Layout::output_location,actual)&&actual==position);
        assert(q.layout.read(m->call,Layout::output_rotation,rotation)&&rotation==angle);
        assert(q.layout.read(m->call,Layout::result,returned)&&returned==1);
        assert(reads==0);read_observer={};
        assert(q.finish(m->call));++cases;
        const auto field_count=q.schema.field_count;
        assert(field_count==(optional?5u:4u));
        // Compare the same production closing verifier with the old repeated
        // records. Both still read all distinct descriptors, names and tables;
        // only duplicate field closures should disappear.
        unsigned closing_reads{};
        read_observer=[&](uptr,std::size_t){++closing_reads;};
        assert(getter::retained_metadata(q.r));unique_closing_reads+=closing_reads;
        for(unsigned i=0;i<field_count;++i)q.schema.fields[q.schema.field_count++]=q.schema.fields[i];
        closing_reads=0;assert(getter::retained_metadata(q.r));
        duplicated_closing_reads+=closing_reads;read_observer={};
        assert(duplicated_closing_reads>unique_closing_reads);
        q.schema.field_count=field_count;
        assert(q.finish(m->call)&&q.schema.field_count==field_count);++cases;
        for(unsigned index=0;index<4;++index){
            auto bad=q.layout;bad.offsets[index]=39;unsigned sentinel=99;
            if(index==0){const auto before=m->call.r.params[39];assert(!bad.write(m->call,Layout::input_name,name)&&m->call.r.params[39]==before);}
            else if(index==3)assert(!bad.read(m->call,Layout::result,sentinel)&&sentinel==99);
            else {std::array<float,3> sentinel3{9,8,7};assert(!bad.read(m->call,Layout::Index(index),sentinel3)&&sentinel3[0]==9);}
            ++cases;
        }
        auto other=m->call;other.r.function+=8;unsigned sentinel=99;
        assert(!q.layout.read(other,Layout::result,sentinel)&&sentinel==99);++cases;
        other=m->call;other.r.size=4097;assert(!q.layout.read(other,Layout::result,sentinel));++cases;
        assert(!q.layout.write(m->call,Layout::result,yes));assert(!q.layout.read(m->call,Layout::input_name,returned));++cases;
    }
    reject([](auto& m){m.call.valid=false;});
    reject([](auto& m){m.call.r.function=0;});
    reject([](auto& m){m.call.r.size=4097;});
    reject([](auto& m){m.call.r.size=39;});
    reject([](auto& m){Memory::put(m.function,0x48,m.text(L"OtherGetter"));m.tables();});
    reject([](auto& m){Memory::put(m.function,0x40,m.pc_class);});
    reject([](auto& m){Memory::put(m.function,0x108,std::uint16_t(31));});
    reject([](auto& m){Memory::put(m.in,0x74,4u);});
    reject([](auto& m){Memory::put(m.in,0x78,0x180ull);});
    reject([](auto& m){Memory::put(m.location,0xd0,m.pc_class);});
    reject([](auto& m){Memory::put(m.rotation,0x78,0x80ull);});
    reject([](auto& m){Memory::put(m.returned,0xd0,3u);});
    reject([](auto& m){Memory::put(m.returned,0x94,31u);});
    reject([](auto& m){Memory::put(m.space,0x78,0x80ull);});
    reject([](auto& m){Memory::put(m.space,0x68,m.in);});
    reject([](auto& m){const auto duplicate=m.property(m.function,L"InSocketName",m.bool_type,36,4);Memory::put(duplicate,0x78,0x80ull);m.tables();});
    reject_after([](auto& m){Memory::put(m.location,0x94,12u);});
    reject_after([](auto& m){Memory::put(m.rotation,0x74,16u);});
    reject_after([](auto& m){Memory::put(m.returned,0xd0,2u);});
    reject_after([](auto& m){Memory::put(m.function,0x106,std::uint16_t(36));});
    reject_after([](auto& m){Memory::put(m.in,0x68,m.returned);});
    reject_after([](auto& m){Memory::put(m.location,0x40,m.pc_class);});
    reject_after([](auto& m){Memory::put(m.function,0x48,m.text(L"OtherGetter"));m.tables();});
    std::printf("WeaponMuzzleGetterCallTest: %u schema/storage/change cases passed; local argument transfers game-memory reads=0; closing fixture reads duplicated=%u unique=%u; no engine/game/FPS proof\n",cases,duplicated_closing_reads,unique_closing_reads);
}
