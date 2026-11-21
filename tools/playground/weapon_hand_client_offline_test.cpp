#include <cstdio>
#include "../vr-common/weapon_hand_client.h"
#define GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW 1
namespace {
FILE* log_file{};
unsigned support_mount_reads{},part_proposals{};
unsigned lancer_proposals{},hammerburst_proposals{};
unsigned boltok_proposals{};
bool simulate_lancer_pending{};
// Exercise the production observer's actual follow-enabled routing. Geometry
// and arbitration are separate owners; these spies cannot grant a hand lease.
namespace nativelancerfollow {
bool fill_support_mount(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState& game) noexcept {
    ++support_mount_reads;game.mount_known=true;return true;
}
}
namespace lancerreload_contact {
void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState& game) noexcept {
    ++part_proposals;++lancer_proposals;
    if(simulate_lancer_pending)game.magazine_observation_pending=true;
}
void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&) noexcept {}
}
namespace hammerburst_runtime {
void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&) noexcept {++part_proposals;++hammerburst_proposals;}
}
namespace snub_runtime {
void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&) noexcept {++part_proposals;}
void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&) noexcept {}
}
namespace boltok_runtime {
void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&) noexcept {++boltok_proposals;}
void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&) noexcept {}
}
namespace longshot_runtime {
void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&) noexcept {++part_proposals;}
void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&) noexcept {}
}
namespace torquebow_runtime {
bool fill_support_mount(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState& g)noexcept {g.mount_known=true;return true;}
void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&)noexcept {}
void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&)noexcept {}
}
namespace gnasher_runtime {
void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&)noexcept {}
void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&)noexcept {}
}
namespace boomshot_runtime {
bool fill_support_mount(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&)noexcept {return false;}
void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&)noexcept {}
void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&)noexcept {}
}
}
#include "weapon_hand_offline_fixture.h"
int main(){
    unsigned cases{};
    // Exercise the production observer against owned reflected memory. Hold
    // consumers get identical logical flags without requesting unused geometry.
    for(unsigned variant=0;variant<10;++variant){
        auto m=std::make_unique<Memory>(variant==1);
        if(variant==2)Memory::put(m->pawn,0x128,3u);
        if(variant==3)Memory::put(m->pawn,0x108,4u);
        if(variant==4)Memory::put(m->weapon,8,4000u);
        if(variant==5)Memory::put(m->weapon,0x110,m->world);
        if(variant==6)weaponhand::held_binding.known=false;
        if(variant==7)Memory::put(m->mesh_property,0x94,0xfffffff0u);
        if(variant==8)fail_read=m->pawn+0x128;
        if(variant==9){mutate_read=m->pawn+0x120;mutation=[&]{Memory::put(m->pawn,0x120,uptr(0));};}
        abi::GripGameState full{};
        const bool expected=weaponhand::observe_source(nullptr,m->frame,0,full);
        const auto mounts=support_mount_reads,proposals=part_proposals;
        weaponhand::HoldSourceObservation hold{};
        const bool accepted=weaponhand::observe_hold_source(nullptr,m->frame,0,hold);
        assert(accepted==expected&&support_mount_reads==mounts&&part_proposals==proposals);
        if(accepted)assert(hold.known==bool(full.known)&&hold.main_hand_held==bool(full.main_hand_held)&&
            hold.busy==bool(full.busy)&&hold.animation_owns_hands==bool(full.animation_owns_hands)&&hold.kind==full.kind);
        else assert(!hold.known&&!hold.main_hand_held&&!hold.busy&&!hold.animation_owns_hands);
        ++cases;
    }
    {auto m=std::make_unique<Memory>();
        Memory::put(m->concrete,0x48,m->text(L"Weap_LocustPistol"));m->tables();
        abi::GripGameState out{};lancer_proposals=boltok_proposals=0;simulate_lancer_pending=true;
        assert(m->read(out)&&out.known&&out.main_hand_held);
        assert(lancer_proposals==0&&boltok_proposals==1&&!out.magazine_observation_pending);
        boltok_proposals=0;assert(m->read(out,true)&&boltok_proposals==0);
        simulate_lancer_pending=false;++cases;
    }
    {auto m=std::make_unique<Memory>();
        Memory::put(m->concrete,0x48,m->text(L"Weap_LocustAssaultRifle"));m->tables();
        abi::GripGameState out{};support_mount_reads=part_proposals=lancer_proposals=hammerburst_proposals=0;
        simulate_lancer_pending=true;
        assert(m->read(out)&&out.known&&out.main_hand_held&&out.mount_known);
        assert(support_mount_reads==1&&part_proposals==1&&lancer_proposals==0&&hammerburst_proposals==1);
        assert(!out.magazine_observation_pending);simulate_lancer_pending=false;++cases;
    }
    {auto m=std::make_unique<Memory>();
        Memory::put(m->concrete,0x48,m->text(L"Weap_LocustAssaultRifle"));
        const auto foreign=m->node(L"ForeignWeaponPackage",m->package_type);
        Memory::put(m->concrete,0x40,foreign);m->tables();
        abi::GripGameState out{};part_proposals=lancer_proposals=hammerburst_proposals=boltok_proposals=0;
        assert(m->read(out)&&out.main_hand_held&&part_proposals==0&&lancer_proposals==0&&hammerburst_proposals==0&&boltok_proposals==0);++cases;
    }
    {auto m=std::make_unique<Memory>();
        Memory::put(m->concrete,0x48,m->text(L"Weap_LocustAssaultRifle"));m->tables();
        abi::GripGameState out{};support_mount_reads=part_proposals=0;
        assert(m->read(out,true)&&out.mount_known&&support_mount_reads==1&&part_proposals==0);++cases;
    }
    for(bool source_only:{false,true}){
    {auto m=std::make_unique<Memory>();abi::GripGameState out{};
        support_mount_reads=part_proposals=0;assert(m->read(out,source_only));
        assert(out.known&&out.main_hand_held&&!out.busy&&!out.animation_owns_hands&&out.kind==abi::WeaponKind::firearm);
        assert(out.mount_known&&support_mount_reads==1u&&part_proposals==(source_only?0u:1u));++cases;}
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
    {auto m=std::make_unique<Memory>();support_mount_reads=part_proposals=0;
        weaponhand::equipment_hold_committed(m->pc,m->equipment);++cases;
        if(!weaponhand::held_binding.known||support_mount_reads||part_proposals){
            std::printf("hold boolean witness created unrelated geometry/proposals: mount=%u parts=%u known=%u\n",support_mount_reads,part_proposals,unsigned(weaponhand::held_binding.known));
            return 4;
        }
        abi::GripGameState out{};assert(m->read(out,false)&&out.main_hand_held&&out.mount_known);
        // The selected Lancer producer runs once; foreign producers do not.
        assert(support_mount_reads==1&&part_proposals==1);++cases;
    }
    std::printf("WeaponHandClientOfflineTest passed: %u cases; full/source retain live mount, boolean liveness omits geometry; owned artificial memory only\n",cases);
}
