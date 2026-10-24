#include "weapon_hand_offline_fixture.h"
int main(){
    unsigned cases{};
    {auto m=std::make_unique<Memory>();abi::GripGameState out{};assert(m->read(out));
        assert(out.known&&out.main_hand_held&&!out.busy&&!out.animation_owns_hands&&!out.mount_known&&out.kind==abi::WeaponKind::firearm);++cases;}
    {auto m=std::make_unique<Memory>(true);abi::GripGameState out{};assert(m->read(out));assert(out.kind==abi::WeaponKind::hand_thrown);++cases;}
    {auto m=std::make_unique<Memory>();fail_read=m->pawn+0x128;abi::GripGameState out{};assert(!m->read(out)&&!out.known);++cases;}
    {auto m=std::make_unique<Memory>();fail_read=m->pc+0x110;abi::GripGameState out{};assert(!m->read(out)&&!out.known);++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->pawn,0x128,3u);abi::GripGameState out{};assert(m->read(out)&&out.busy);++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->pawn,0x108,4u);abi::GripGameState out{};assert(!m->read(out));++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->weapon,8,4000u);abi::GripGameState out{};assert(!m->read(out));++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->weapon,0x110,m->world);abi::GripGameState out{};assert(!m->read(out));++cases;}
    {auto m=std::make_unique<Memory>();mutate_read=m->pawn+0x120;mutation=[&]{Memory::put(m->pawn,0x120,uptr(0));};
        abi::GripGameState out{};assert(!m->read(out));++cases;}
    {auto m=std::make_unique<Memory>();const auto package=m->node(L"FakeCore",m->package_type);
        const auto type=m->node(L"BoolProperty",m->class_type,package);Memory::put(m->switch_property,0x50,type);m->tables();
        abi::GripGameState out{};assert(!m->read(out));++cases;}
    {auto m=std::make_unique<Memory>();const auto p=m->node(L"bSwitchingWeapons",m->object_type,m->pawn_class);
        Memory::put(p,0x70,1u);Memory::put(p,0x74,4u);Memory::put(p,0x94,0x128u);
        Memory::put(p,0x68,Memory::get<uptr>(m->pawn_class,0x80));Memory::put(m->pawn_class,0x80,p);m->tables();
        abi::GripGameState out{};assert(!m->read(out));++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->actor_class,0x60,m->actor_class);abi::GripGameState out{};
        assert(!m->read(out));++cases;}
    {auto m=std::make_unique<Memory>();game_thread.store(0);abi::GripGameState out{};assert(!m->read(out));++cases;}
    {auto m=std::make_unique<Memory>();vrequip::model.reset();abi::GripGameState out{};
        assert(m->read(out)&&out.known&&!out.main_hand_held);++cases;}
    {auto m=std::make_unique<Memory>();weaponhand::held_binding={};abi::GripGameState out{};
        assert(m->read(out)&&out.known&&!out.main_hand_held);++cases;}
    {auto m=std::make_unique<Memory>();auto name=Memory::get<meta::RawName>(m->weapon,0x48);
        ++name.number;Memory::put(m->weapon,0x48,name);m->frame.weapon=m->identity(m->weapon);
        abi::GripGameState out{};assert(m->read(out)&&out.known&&!out.main_hand_held);++cases;}
    {auto m=std::make_unique<Memory>();auto other=m->equipment;other.world=m->pc;
        vrequip::model.reset();vrequip::model.observe(other);vrequip::model.tap(gears_weapon::BodySlot::weapon_2,other);
        abi::GripGameState out{};assert(m->read(out)&&out.known&&!out.main_hand_held);++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->mesh_property,0x50,m->object_type);abi::GripGameState out{};
        assert(!m->read(out));++cases;}
    {auto m=std::make_unique<Memory>();const auto old=vrequip::model.hold_generation();
        vrequip::model.reset();vrequip::model.observe(m->equipment);
        vrequip::model.tap(gears_weapon::BodySlot::weapon_2,m->equipment);
        assert(vrequip::model.hold_generation()&&vrequip::model.hold_generation()!=old);
        abi::GripGameState out{};assert(m->read(out)&&!out.main_hand_held);
        fail_read=m->pawn+0x128;weaponhand::equipment_hold_committed(m->pc,m->equipment);
        fail_read=0;assert(!weaponhand::held_binding.known&&m->read(out)&&!out.main_hand_held);++cases;}
    {auto m=std::make_unique<Memory>();const auto old=vrequip::model.hold_generation();
        assert(vrequip::model.begin_pickup(m->equipment).event==gears_weapon::EquipmentEvent::pickup_animation);
        assert(vrequip::model.finish_pickup(m->equipment,2).event==gears_weapon::EquipmentEvent::equipped);
        assert(vrequip::model.hold_generation()!=old);abi::GripGameState out{};
        assert(m->read(out)&&!out.main_hand_held); // No pickup bridge has certified this commit yet.
        weaponhand::equipment_hold_committed(m->pc,m->equipment);
        assert(m->read(out)&&out.main_hand_held);++cases;}
    {game_thread.store(0);assert(weaponhand::start(nullptr)==abi::Reply::unavailable);
        assert(weaponhand::start(GetModuleHandleW(L"kernel32.dll"))==abi::Reply::unavailable);
        assert(!weaponhand::native_module&&weaponhand::client.phase()==abi::ClientPhase::off);
        assert(weaponhand::stop()==abi::Reply::quiescent);++cases;}
    std::printf("WeaponHandClientOfflineTest passed: %u cases; owned artificial metadata only\n",cases);
}
