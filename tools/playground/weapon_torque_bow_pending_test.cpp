#include "weapon_hand_offline_fixture.h"
namespace {
#include "weapon_torque_bow_pending.inl"
struct PendingMemory:Memory {
    uptr manager{},array{},inner{},int_type{},byte_type{};
    std::array<std::int32_t,4> values{};
    PendingMemory(){
        const auto manager_class=cls(L"InventoryManager",engine);
        const auto array_type=cls(L"ArrayProperty",core);
        int_type=cls(L"IntProperty",core);byte_type=cls(L"ByteProperty",core);
        array=property(manager_class,L"PendingFire",array_type,0x110,16);
        inner=property(array,L"Inner",int_type,0,4);put(array,0xd0,inner);
        manager=node(L"CurrentInventoryManager",manager_class,pawn);
        put(manager,0x110,meta::Table{reinterpret_cast<uptr>(values.data()),4,4});tables();
    }
    bool read(torquebow_pending::Observed& out){
        meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
        return meta::tables(base,schema.objects,schema.names)&&torquebow_pending::read(r,manager,out);
    }
};
}
int main(){
    unsigned checks{};
    auto verify=[&](auto mutate,bool expected){auto m=std::make_unique<PendingMemory>();mutate(*m);
        const auto before=m->values;torquebow_pending::Observed observed{};
        assert(m->read(observed)==expected);assert(m->values==before);++checks;};
    verify([](auto&){},true);
    verify([](auto& m){m.values[0]=1;},true);
    // A nonzero upper byte must never be misread as a neutral low byte.
    verify([](auto& m){m.values[0]=0x100;},false);
    verify([](auto& m){m.values[0]=-1;},false);
    verify([](auto& m){m.put(m.inner,0x50,m.byte_type);m.put(m.inner,0x74,1u);},false);
    verify([](auto& m){m.put(m.inner,0x74,1u);},false);
    verify([](auto& m){m.put(m.inner,0x70,2u);},false);
    verify([](auto& m){m.put(m.inner,0x94,4u);},false);
    verify([](auto& m){m.put(m.inner,0x40,m.weapon);},false);
    verify([](auto& m){m.put(m.manager,0x110,meta::Table{});},false);
    verify([](auto& m){m.put(m.manager,0x110,meta::Table{reinterpret_cast<uptr>(m.values.data()),4,129});},false);
    verify([](auto& m){m.put(m.manager,0x110,meta::Table{0,4,4});},false);
    verify([](auto& m){m.put(m.manager,0x110,meta::Table{1,4,4});},false);
    std::printf("Bow PendingFire actual typed metadata cases: %u, failures0; no Engine calls or value writes\n",checks);
}
