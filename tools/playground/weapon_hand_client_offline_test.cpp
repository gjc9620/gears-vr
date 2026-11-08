#include <cstdio>
#include "../vr-common/weapon_hand_client.h"
#define GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW 1
namespace {
FILE* log_file{};
unsigned support_mount_reads{},part_proposals{};
// Exercise the production observer's actual follow-enabled routing. Geometry
// and arbitration are separate owners; these spies cannot grant a hand lease.
namespace nativelancerfollow {
bool fill_support_mount(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState& game) noexcept {
    ++support_mount_reads;game.mount_known=true;return true;
}
}
namespace lancerreload_contact {
void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&) noexcept {++part_proposals;}
void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&) noexcept {}
}
namespace hammerburst_runtime {
void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&) noexcept {++part_proposals;}
}
namespace snub_runtime {
void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&) noexcept {++part_proposals;}
void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&) noexcept {}
}
}
#include "weapon_hand_offline_fixture.h"
int main(){
    unsigned cases{};
    for(bool source_only:{false,true}){
    {auto m=std::make_unique<Memory>();abi::GripGameState out{};
        support_mount_reads=part_proposals=0;assert(m->read(out,source_only));
        assert(out.known&&out.main_hand_held&&!out.busy&&!out.animation_owns_hands&&out.kind==abi::WeaponKind::firearm);
        assert(out.mount_known&&support_mount_reads==1u&&part_proposals==(source_only?0u:3u));++cases;}
    {auto m=std::make_unique<Memory>(true);abi::GripGameState out{};assert(m->read(out,source_only));assert(out.kind==abi::WeaponKind::hand_thrown);++cases;}
    {auto m=std::make_unique<Memory>();fail_read=m->pawn+0x128;abi::GripGameState out{};assert(!m->read(out,source_only)&&!out.known);++cases;}
    {auto m=std::make_unique<Memory>();fail_read=m->pc+0x110;abi::GripGameState out{};assert(!m->read(out,source_only)&&!out.known);++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->pawn,0x128,3u);abi::GripGameState out{};assert(m->read(out,source_only)&&out.busy);++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->pawn,0x108,4u);abi::GripGameState out{};assert(!m->read(out,source_only));++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->weapon,8,4000u);abi::GripGameState out{};assert(!m->read(out,source_only));++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->weapon,0x110,m->world);abi::GripGameState out{};assert(!m->read(out,source_only));++cases;}
    {auto m=std::make_unique<Memory>();mutate_read=m->pawn+0x120;mutation=[&]{Memory::put(m->pawn,0x120,uptr(0));};
        abi::GripGameState out{};assert(!m->read(out,source_only));++cases;}
    {auto m=std::make_unique<Memory>();const auto package=m->node(L"FakeCore",m->package_type);
        const auto type=m->node(L"BoolProperty",m->class_type,package);Memory::put(m->switch_property,0x50,type);m->tables();
        abi::GripGameState out{};assert(!m->read(out,source_only));++cases;}
    {auto m=std::make_unique<Memory>();const auto p=m->node(L"bSwitchingWeapons",m->object_type,m->pawn_class);
        Memory::put(p,0x70,1u);Memory::put(p,0x74,4u);Memory::put(p,0x94,0x128u);
        Memory::put(p,0x68,Memory::get<uptr>(m->pawn_class,0x80));Memory::put(m->pawn_class,0x80,p);m->tables();
        abi::GripGameState out{};assert(!m->read(out,source_only));++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->actor_class,0x60,m->actor_class);abi::GripGameState out{};
        assert(!m->read(out,source_only));++cases;}
    {auto m=std::make_unique<Memory>();game_thread.store(0);abi::GripGameState out{};assert(!m->read(out,source_only));++cases;}
    {auto m=std::make_unique<Memory>();vrequip::model.reset();abi::GripGameState out{};
        assert(m->read(out,source_only)&&out.known&&!out.main_hand_held);++cases;}
    {auto m=std::make_unique<Memory>();weaponhand::held_binding={};abi::GripGameState out{};
        assert(m->read(out,source_only)&&out.known&&!out.main_hand_held);++cases;}
    {auto m=std::make_unique<Memory>();auto name=Memory::get<meta::RawName>(m->weapon,0x48);
        ++name.number;Memory::put(m->weapon,0x48,name);m->frame.weapon=m->identity(m->weapon);
        abi::GripGameState out{};assert(m->read(out,source_only)&&out.known&&!out.main_hand_held);++cases;}
    {auto m=std::make_unique<Memory>();auto other=m->equipment;other.world=m->pc;
        vrequip::model.reset();vrequip::model.observe(other);vrequip::model.tap(gears_weapon::BodySlot::weapon_2,other);
        abi::GripGameState out{};assert(m->read(out,source_only)&&out.known&&!out.main_hand_held);++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->mesh_property,0x50,m->object_type);abi::GripGameState out{};
        assert(!m->read(out,source_only));++cases;}
    {auto m=std::make_unique<Memory>();const auto old=vrequip::model.hold_generation();
        vrequip::model.reset();vrequip::model.observe(m->equipment);
        vrequip::model.tap(gears_weapon::BodySlot::weapon_2,m->equipment);
        assert(vrequip::model.hold_generation()&&vrequip::model.hold_generation()!=old);
        abi::GripGameState out{};assert(m->read(out,source_only)&&!out.main_hand_held);
        fail_read=m->pawn+0x128;weaponhand::equipment_hold_committed(m->pc,m->equipment);
        fail_read=0;assert(!weaponhand::held_binding.known&&m->read(out,source_only)&&!out.main_hand_held);++cases;}
    {auto m=std::make_unique<Memory>();const auto old=vrequip::model.hold_generation();
        assert(vrequip::model.begin_pickup(m->equipment).event==gears_weapon::EquipmentEvent::pickup_animation);
        assert(vrequip::model.finish_pickup(m->equipment,2).event==gears_weapon::EquipmentEvent::equipped);
        assert(vrequip::model.hold_generation()!=old);abi::GripGameState out{};
        assert(m->read(out,source_only)&&!out.main_hand_held); // No pickup bridge has certified this commit yet.
        weaponhand::equipment_hold_committed(m->pc,m->equipment);
        assert(m->read(out,source_only)&&out.main_hand_held);++cases;}
    {game_thread.store(0);assert(weaponhand::start(nullptr)==abi::Reply::unavailable);
        assert(weaponhand::start(GetModuleHandleW(L"kernel32.dll"))==abi::Reply::unavailable);
        assert(!weaponhand::native_module&&weaponhand::client.phase()==abi::ClientPhase::off);
        assert(weaponhand::stop()==abi::Reply::quiescent);++cases;}
    }
    {auto m=std::make_unique<Memory>();abi::GripGameState out{};support_mount_reads=part_proposals=0;
        assert(weaponhand::observe_liveness(nullptr,m->frame,weaponhand::clock_ns(nullptr),out));
        assert(out.known&&out.main_hand_held&&!out.mount_known&&support_mount_reads==0&&part_proposals==0);++cases;}
    {auto m=std::make_unique<Memory>();fail_read=m->pawn+0x128;abi::GripGameState out{};support_mount_reads=part_proposals=0;
        assert(!weaponhand::observe_liveness(nullptr,m->frame,weaponhand::clock_ns(nullptr),out)&&!out.known);
        assert(support_mount_reads==0&&part_proposals==0);++cases;}
    {auto m=std::make_unique<Memory>();mutate_read=m->pawn+0x120;mutation=[&]{Memory::put(m->pawn,0x120,uptr(0));};
        abi::GripGameState out{};assert(!weaponhand::observe_liveness(nullptr,m->frame,weaponhand::clock_ns(nullptr),out)&&!out.known);++cases;}
    std::printf("WeaponHandClientOfflineTest passed: %u cases; full/source retain live mount, boolean liveness omits geometry; owned artificial memory only\n",cases);
}
