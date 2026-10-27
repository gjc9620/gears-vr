#include "weapon_hand_offline_fixture.h"
#include <algorithm>
#include <vector>
namespace {
using Fields=weaponhand::PawnObserveFields;
struct Batch {
    Memory& memory;meta::Snapshot schema{};meta::Reader reader{schema,meta::Stage::final_check};Fields fields;
    explicit Batch(Memory& value):memory(value){schema.module_base=base;assert(meta::tables(base,schema.objects,schema.names));}
    bool begin(){return fields.begin(reader,memory.pawn);}
    bool values(){
        meta::PointerField link{};weaponhand::BoolField flag{};
        return fields.pointer(reader,Fields::controller_link,memory.pc,link)&&
            fields.pointer(reader,Fields::mesh_link,memory.mesh,link)&&
            fields.pointer(reader,Fields::weapon_link,memory.weapon,link)&&
            fields.boolean(reader,Fields::deleted_flag,flag)&&flag.mask==4&&!(flag.bits&flag.mask)&&
            fields.boolean(reader,Fields::switching_flag,flag)&&flag.mask==1&&!(flag.bits&flag.mask)&&
            fields.boolean(reader,Fields::no_firing_flag,flag)&&flag.mask==2&&!(flag.bits&flag.mask);
    }
};
// The unchanged production single-field selectors are the static comparison.
// No alternate implementation of reflection or batch selection lives here.
bool original_selects(Memory& memory){
    meta::Snapshot schema{};meta::Reader r{schema,meta::Stage::final_check};weaponhand::BoolField field{};
    return meta::tables(base,schema.objects,schema.names)&&
        weaponhand::field(r,memory.pawn,L"Controller",L"ObjectProperty",8,field)&&
        weaponhand::field(r,memory.pawn,L"Mesh",L"ComponentProperty",8,field)&&
        weaponhand::field(r,memory.pawn,L"Weapon",L"ObjectProperty",8,field)&&
        weaponhand::field(r,memory.pawn,L"bDeleteMe",L"BoolProperty",4,field)&&
        weaponhand::field(r,memory.pawn,L"bSwitchingWeapons",L"BoolProperty",4,field)&&
        weaponhand::field(r,memory.pawn,L"bNoWeaponFiring",L"BoolProperty",4,field);
}
void rejects(Memory& memory){assert(!original_selects(memory));Batch b(memory);assert(!b.begin());}
uptr first(Memory& memory){return Memory::get<uptr>(memory.pawn_class,0x80);}
uptr prepend(Memory& memory,wchar_t const* label,uptr type,unsigned offset,unsigned width){
    const auto p=memory.node(label,type,memory.pawn_class);
    Memory::put(p,0x70,1u);Memory::put(p,0x74,width);Memory::put(p,0x94,offset);
    Memory::put(p,0x68,first(memory));Memory::put(memory.pawn_class,0x80,p);memory.tables();return p;
}
struct LongChildren {
    std::vector<Block> nodes;std::vector<uptr> registry;
    LongChildren(Memory& memory,unsigned extra):nodes(extra),registry(1001+memory.node_count+extra){
        assert(extra&&registry.size()>memory.objects.size());
        std::copy(memory.objects.begin(),memory.objects.end(),registry.begin());
        const auto name=memory.text(L"IgnoredField");memory.tables();
        for(unsigned i=0;i<extra;++i){
            const auto address=reinterpret_cast<uptr>(nodes[i].bytes.data());const auto index=1001+memory.node_count+i;
            Memory::put(address,8,index);Memory::put(address,0x40,memory.pawn_class);Memory::put(address,0x48,name);
            Memory::put(address,0x50,memory.object_type);registry[index]=address;
            if(i)Memory::put(reinterpret_cast<uptr>(nodes[i-1].bytes.data()),0x68,address);
        }
        auto tail=first(memory);while(Memory::get<uptr>(tail,0x68))tail=Memory::get<uptr>(tail,0x68);
        Memory::put(tail,0x68,reinterpret_cast<uptr>(nodes[0].bytes.data()));
        Memory::put(base,0x1c24730,meta::Table{reinterpret_cast<uptr>(registry.data()),int(registry.size()),int(registry.size())});
    }
};
}
int main(){
    unsigned cases{};
    const auto passed=[&](char const* label){++cases;std::printf("%02u %s\n",cases,label);};
    {auto m=std::make_unique<Memory>();assert(original_selects(*m));Batch b(*m);assert(b.begin()&&b.values()&&b.fields.finish(b.reader));
        assert(!b.values()&&!b.fields.finish(b.reader));passed("six fields, inherited delete, finish consumes binding");}
    {auto m=std::make_unique<Memory>();const auto derived=m->cls(L"DerivedPawn",m->warfare,m->pawn_class);
        const auto selected=m->property(derived,L"Controller",m->object_type,0x130,8);Memory::put(m->pawn,0x50,derived);
        Memory::put(m->pawn,0x130,m->pc);m->tables();Batch b(*m);assert(original_selects(*m)&&b.begin());meta::PointerField link{};
        assert(b.fields.pointer(b.reader,Fields::controller_link,m->pc,link)&&
            b.schema.identities[link.property.identity].address==selected&&b.values()&&b.fields.finish(b.reader));
        passed("derived first match selects its own offset");}
    {auto m=std::make_unique<Memory>();m->property(m->pawn_class,L"Controller",m->bool_type,0x130,4,1);m->tables();
        Batch b(*m);assert(original_selects(*m)&&b.begin()&&b.values()&&b.fields.finish(b.reader));passed("later same-name wrong type does not replace first");}
    {auto m=std::make_unique<Memory>();prepend(*m,L"Controller",m->bool_type,0x110,8);rejects(*m);passed("first same-name wrong type rejects");}
    {auto m=std::make_unique<Memory>();Memory::put(first(*m),0x48,m->text(L"controller"));m->tables();rejects(*m);passed("case-sensitive exact label remains");}
    {auto m=std::make_unique<Memory>();auto name=Memory::get<meta::RawName>(first(*m),0x48);++name.number;
        Memory::put(first(*m),0x48,name);rejects(*m);passed("numbered first field rejects");}
    {auto m=std::make_unique<Memory>();Memory::put(first(*m),0x40,m->actor_class);rejects(*m);passed("property outer must be declaring class");}
    {auto m=std::make_unique<Memory>();Memory::put(m->mesh_property,0x50,m->object_type);rejects(*m);passed("Mesh keeps exact ComponentProperty");}
    {auto m=std::make_unique<Memory>();const auto package=m->node(L"FakeCore",m->package_type);
        const auto type=m->node(L"BoolProperty",m->class_type,package);Memory::put(m->switch_property,0x50,type);m->tables();
        rejects(*m);passed("type package must be Core");}
    {auto m=std::make_unique<Memory>();const auto nested=m->node(L"Core",m->package_type,m->warfare);
        const auto type=m->node(L"ObjectProperty",m->class_type,nested);Memory::put(first(*m),0x50,type);m->tables();
        rejects(*m);passed("Core package must have no outer");}
    {auto m=std::make_unique<Memory>();Memory::put(first(*m),0x70,2u);rejects(*m);passed("dimension one remains");}
    {auto m=std::make_unique<Memory>();Memory::put(m->switch_property,0x74,8u);rejects(*m);passed("BoolProperty width four remains");}
    {auto m=std::make_unique<Memory>();Memory::put(first(*m),0x94,509u);rejects(*m);passed("offset plus width must fit class extent");}
    {auto m=std::make_unique<Memory>();Memory::put(m->actor_class,0x88,0x108u);rejects(*m);passed("inherited property fits declaring extent");}
    {auto m=std::make_unique<Memory>();const auto derived=m->cls(L"SmallDerived",m->warfare,m->pawn_class);
        Memory::put(derived,0x88,0x128u);Memory::put(m->pawn,0x50,derived);m->tables();rejects(*m);passed("inherited property fits object extent");}
    {auto m=std::make_unique<Memory>();Memory::put(m->switch_property,8,4000u);rejects(*m);passed("selected descriptor registration remains");}
    {auto m=std::make_unique<Memory>();const auto tail=Memory::get<uptr>(m->switch_property,0x68);
        Memory::put(tail,0x68,first(*m));rejects(*m);passed("child-list cycle before inherited field rejects");}
    {auto m=std::make_unique<Memory>();Memory::put(m->actor_class,0x80,uptr(0));Memory::put(m->actor_class,0x60,m->actor_class);
        rejects(*m);passed("parent cycle before missing field rejects");}
    {auto m=std::make_unique<Memory>();auto cls=m->pawn_class;for(unsigned i=0;i<38;++i){
            const auto label=L"Derived"+std::to_wstring(i);cls=m->cls(label.c_str(),m->warfare,cls);}
        Memory::put(m->pawn,0x50,cls);m->tables();Batch b(*m);assert(original_selects(*m)&&b.begin()&&b.values()&&b.fields.finish(b.reader));
        passed("forty parent classes accepted");}
    {auto m=std::make_unique<Memory>();auto cls=m->pawn_class;for(unsigned i=0;i<39;++i){
            const auto label=L"Derived"+std::to_wstring(i);cls=m->cls(label.c_str(),m->warfare,cls);}
        Memory::put(m->pawn,0x50,cls);m->tables();rejects(*m);passed("forty-one parent classes reject");}
    {auto m=std::make_unique<Memory>();LongChildren storage(*m,4091);Batch b(*m);
        assert(original_selects(*m)&&b.begin()&&b.values()&&b.fields.finish(b.reader));passed("4096 children before inherited field accepted");}
    {auto m=std::make_unique<Memory>();LongChildren storage(*m,4092);rejects(*m);passed("4097 children before inherited field reject");}
    {auto m=std::make_unique<Memory>();Batch b(*m);b.schema.identity_count=meta::max_identities;
        assert(!b.begin()&&b.schema.failure.reason==meta::Reason::capacity);passed("identity workspace capacity rejects");}
    {auto m=std::make_unique<Memory>();Batch b(*m);b.schema.name_count=meta::max_names;
        assert(!b.begin()&&b.schema.failure.reason==meta::Reason::capacity);passed("name workspace capacity rejects");}
    {auto m=std::make_unique<Memory>();Batch b(*m);assert(b.begin());Memory::put(first(*m),0x70,2u);meta::PointerField value{};
        assert(!b.fields.pointer(b.reader,Fields::controller_link,m->pc,value)&&!b.fields.finish(b.reader));
        passed("descriptor dimension change after begin rejects");}
    {auto m=std::make_unique<Memory>();Batch b(*m);assert(b.begin());Memory::put(m->pawn,0x50,m->pc_class);meta::PointerField value{};
        assert(!b.fields.pointer(b.reader,Fields::controller_link,m->pc,value)&&!b.fields.finish(b.reader));passed("pawn class change after begin rejects");}
    {auto m=std::make_unique<Memory>();Batch b(*m);assert(b.begin());const auto previous=first(*m);
        const auto replacement=prepend(*m,L"Controller",m->object_type,0x110,8);
        assert(replacement!=previous&&first(*m)==replacement&&!b.fields.finish(b.reader));passed("new first-match address before finish rejects");}
    {auto m=std::make_unique<Memory>();Batch b(*m);assert(b.begin());fail_read=m->pawn+0x110;meta::PointerField value{};
        assert(!b.fields.pointer(b.reader,Fields::controller_link,m->pc,value));passed("unreadable pointer remains unknown");}
    {auto m=std::make_unique<Memory>();Batch b(*m);assert(b.begin());fail_read=m->switch_property+0xd0;weaponhand::BoolField flag{};
        assert(!b.fields.boolean(b.reader,Fields::switching_flag,flag));passed("unreadable bool mask rejects");}
    {auto m=std::make_unique<Memory>();Batch b(*m);assert(b.begin());Memory::put(m->switch_property,0xd0,3u);weaponhand::BoolField flag{};
        assert(!b.fields.boolean(b.reader,Fields::switching_flag,flag));passed("non-single-bit bool mask rejects");}
    {auto m=std::make_unique<Memory>();Batch b(*m);assert(b.begin());fail_read=m->pawn+0x128;weaponhand::BoolField flag{};
        assert(!b.fields.boolean(b.reader,Fields::switching_flag,flag));passed("unreadable false bool is not known false");}
    {auto m=std::make_unique<Memory>();Batch b(*m);assert(b.begin());meta::PointerField link{};weaponhand::BoolField flag{};
        assert(!b.fields.pointer(b.reader,Fields::deleted_flag,m->pc,link)&&!b.fields.boolean(b.reader,Fields::mesh_link,flag));
        passed("field kinds cannot be interchanged");}
    {auto m=std::make_unique<Memory>();abi::GripGameState out{};assert(m->read(out));Memory::put(first(*m),0x94,0x130u);
        Memory::put(m->pawn,0x130,m->pc);assert(m->read(out)&&out.known);passed("next observe resolves changed valid offset afresh");}
    {auto m=std::make_unique<Memory>();abi::GripGameState out{};mutate_read=m->pawn+0x120;
        mutation=[&]{Memory::put(m->pawn,0x120,uptr(0));};assert(!m->read(out)&&!out.known);
        passed("observe preserves final pointer value reread");}
    {auto m=std::make_unique<Memory>();abi::GripGameState out{};mutate_read=m->pawn+0x128;
        mutation=[&]{Memory::put(m->pawn,0x128,3u);};assert(!m->read(out)&&!out.known);
        passed("observe preserves final bool value reread");}
    {auto m=std::make_unique<Memory>();abi::GripGameState out{};mutate_read=m->pawn+0x128;
        mutation=[&]{prepend(*m,L"Controller",m->object_type,0x110,8);};assert(!m->read(out)&&!out.known);
        passed("observe finish rejects changed first-match binding");}
    std::printf("WeaponHandPawnReadTest passed: %u cases; production local batch and owned artificial memory only\n",cases);
}
