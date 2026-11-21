#define main existing_fire_owner_tests
#include "../vr-common/weapon_troika_fire_owner_test.cpp"
#undef main
#include <atomic>
#include <cstring>
#include <new>
#include <stdexcept>
#include "continuous_hand_owner_entry.h"
#include "cover_pose_phase.h"
#include "../vr-common/weapon_troika_driver_scope.h"
#define GEARS_TROIKA_MOUNTED_SOURCE 1
std::uintptr_t base=0x140000000;
std::int64_t fixture_now=2010000000;
unsigned world_leases{},hint_calls{},source_reads{},mapping_reads{};bool world_allowed=true,throw_world=false,attestation_ok=true,source_current=true;
struct Scope {
    Scope(){if(throw_world)throw std::runtime_error("fixture world constructor");++world_leases;}
    ~Scope(){CHECK(world_leases);--world_leases;}
    bool world()const{return world_allowed;}
};
namespace api=gears_troika_driver;api::Snapshot selected;
namespace rig_tracking_live {std::int64_t clock_ns(){return fixture_now;}}
namespace troika_control_live {
bool driver_hint(std::uintptr_t mesh,g::Source& out) noexcept {++hint_calls;if(!source_current||mesh!=selected.source.frame.driver_mesh.address)return false;out=selected.source;return true;}
bool read_for(g::Source const& q,c::Decision& out) noexcept {++source_reads;CHECK(world_leases);if(!source_current||std::memcmp(&q,&selected.source,sizeof q))return false;out=selected.control;return true;}
}
namespace troika_scene_tracking {
bool mapped_for(gears_troika_source::Frame const& q,gears_weapon::TroikaMountedMapping& out) noexcept {
    ++mapping_reads;CHECK(world_leases);if(!source_current||std::memcmp(&q,&selected.source.frame,sizeof q))return false;out=selected.mapping;return true;
}
}
#include "troika_driver_scope_live.inl"
namespace live=troika_driver_scope_live;using Event=gears_continuous_hand_entry::Entry;
bool certify(std::uintptr_t mesh){CHECK(world_leases&&mesh==selected.source.frame.driver_mesh.address);return attestation_ok;}
void setup(){Fixture f;selected={};selected.source=f.source;selected.source.frame.native.game_thread=GetCurrentThreadId();selected.control=f.decision;
    auto const& s=selected.source.frame;auto& m=selected.mapping;
    m.identity={s.world.address,s.world_generation,s.driver.address,s.controller.address,s.turret.address,selected.source.entry_binding,
        s.weapon.address,selected.source.entry_binding,s.controllers.session,s.controllers.space_epoch,s.controllers.input_epoch,s.controllers.profile_generation};
    m.publication_sequence=s.publication_sequence;m.source_ticket=s.source_ticket;m.units_per_metre=100;
    m.hands.input_sequence=s.controllers.sequence;m.hands.input_sync_ns=s.controllers.sync_ns;m.hands.input_received_ns=s.controllers.received_ns;
    m.hands.stereo_received_ns=s.stereo.received_ns;m.hands.tracked_mask=3;fixture_now=f.now;CHECK(api::valid(selected,fixture_now));
}
std::uint64_t serial=100;
Event tick(){Event e{};e.kind=2;e.rcx=selected.source.frame.driver_mesh.address;e.ra=base+0x12e537;e.thread=GetCurrentThreadId();e.invocation=++serial;return e;}
Event compose(){Event e{};e.kind=0;e.rcx=selected.source.frame.driver_mesh.address;e.rdx=e.rcx+0x27c;e.r8=e.rcx+0x28c;
    e.ra=base+gears_cover_pose_phase::compose_caller;e.thread=GetCurrentThreadId();e.invocation=++serial;return e;}
int main(int argc,char** argv){setup();
    if(argc==2&&!std::strcmp(argv[1],"--constructor-fault")){
        throw_world=true;auto e=tick();CHECK(live::begin_tick(e,certify,false));
        CHECK(live::failed.load()&&live::pending_count()==1&&!world_leases);
        CHECK(!live::begin_tick(e,certify,false));live::request_drain();CHECK(live::pending_count()==1);
        std::printf("Troika driver constructor fault retains debt: %u checks passed; synthetic world only.\n",checks);return 0;
    }
    for(unsigned fault=0;fault<10;++fault){const auto original=selected;
        switch(fault){case 0:selected.mapping.identity.pawn=selected.source.frame.turret.address;break;
        case 1:selected.mapping.units_per_metre=1;break;case 2:++selected.mapping.hands.input_sync_ns;break;
        case 3:selected.mapping.hands.tracked_mask=0;break;case 4:selected.mapping.world_head.orientation={0,0,0,0};break;
        case 5:selected.mapping.hands.world[1].orientation={0,0,0,0};break;
        case 6:++selected.control.source_ticket;break;case 7:++selected.mapping.identity.world_generation;break;
        case 8:selected.source.frame.native.registration_cookie=0;break;default:++selected.source.frame.native_reference_revision;break;}
        CHECK(!api::valid(selected,fixture_now));auto e=tick();CHECK(live::begin_tick(e,certify,false)&&!live::pending_count()&&!world_leases);selected=original;
    }
    auto t=tick();auto wrong=t;wrong.rcx=selected.source.frame.turret_mesh.address;CHECK(!live::begin_tick(wrong,certify,false)&&!world_leases&&!live::pending_count());
    CHECK(live::begin_tick(t,certify,true)&&!world_leases&&!live::pending_count());
    attestation_ok=false;CHECK(live::begin_tick(t,certify,false)&&!world_leases&&!live::pending_count());attestation_ok=true;
    world_allowed=false;CHECK(live::begin_tick(t,certify,false)&&!world_leases&&!live::pending_count());world_allowed=true;
    gears_native_context::StereoMutex stereo;
    {std::lock_guard<gears_native_context::StereoMutex> lock(stereo);const auto hints=hint_calls;CHECK(!live::begin_tick(t,certify,false)&&hint_calls==hints&&!world_leases);}
    CHECK(live::begin_tick(t,certify,false)&&live::pending_count()==1&&world_leases==1&&!t.selected&&!t.tick_depth);
    api::Snapshot tick_source{};CHECK(live::copy_for_tick(tick_source)&&!std::memcmp(&tick_source,&selected,sizeof selected));
    auto e=compose();CHECK(live::begin_compose(e,true)&&!e.selected&&!e.compose_depth);
    api::Snapshot out{};CHECK(live::copy_for_compose(e,out)&&out.source.frame.controllers.sync_ns==selected.source.frame.controllers.sync_ns);
    const auto reads=source_reads;auto copy=e;CHECK(!live::copy_for_compose(copy,out)&&source_reads>reads); // Address is part of the scope.
    ++e.r9;CHECK(!live::copy_for_compose(e,out));--e.r9;CHECK(live::copy_for_compose(e,out));
    source_current=false;CHECK(!live::copy_for_compose(e,out));source_current=true;
    CHECK(live::end(e,true)&&live::pending_count()==1&&world_leases==1);CHECK(!live::copy_for_compose(e,out));
    CHECK(live::end(t,true)&&!live::pending_count()&&!world_leases);
    t=tick();CHECK(live::begin_tick(t,certify,false));e=compose();e.r8+=8;CHECK(live::begin_compose(e,true)&&!live::copy_for_compose(e,out));
    CHECK(live::end(e,true)&&live::end(t,true)&&!live::pending_count()&&!world_leases);
    t=tick();CHECK(live::begin_tick(t,certify,false));auto nested=tick();CHECK(live::begin_tick(nested,certify,false));
    e=compose();CHECK(live::begin_compose(e,true)&&!live::copy_for_compose(e,out));CHECK(live::end(nested,true)&&live::pending_count()==1);
    CHECK(live::end(t,true)&&!live::pending_count()&&!world_leases);
    t=tick();CHECK(live::begin_tick(t,certify,false));fixture_now+=100000001;e=compose();
    CHECK(live::begin_compose(e,true)&&!live::copy_for_compose(e,out));CHECK(live::end(t,true)&&!live::pending_count()&&!world_leases);setup();
    t=tick();CHECK(live::begin_tick(t,certify,false));e=compose();CHECK(live::begin_compose(e,true)&&live::copy_for_compose(e,out));
    Event const* held_outer{};Scope* held_world{};
    CHECK(live::owns_compose(e)&&live::retained_for_compose(e,out,held_outer,held_world)&&held_outer==&t&&held_world&&held_world->world());
    fixture_now+=60000000;
    CHECK(live::copy_for_compose(e,out)&&!gears_troika_source::frame_valid(out.source.frame,fixture_now,50000000,false));
    fixture_now+=50000000;
    CHECK(!live::copy_for_compose(e,out)&&live::retained_for_compose(e,out,held_outer,held_world));
    live::request_drain();CHECK(!live::copy_for_compose(e,out)&&live::pending_count()==1);
    CHECK(!live::copy_for_tick(tick_source));
    CHECK(live::retained_for_compose(e,out,held_outer,held_world)&&held_outer==&t);
    auto wrong_end=e;CHECK(!live::retained_for_compose(wrong_end,out,held_outer,held_world));
    CHECK(live::end(e,true)&&live::end(t,true)&&!live::pending_count()&&!world_leases);
    CHECK(!live::owns_compose(e)&&!live::retained_for_compose(e,out,held_outer,held_world));
    CHECK(!live::begin_tick(t,certify,false));
    std::printf("Troika actual Native driver-scope adapter: %u checks passed; synthetic world/source only, no ordinary Frame or writer.\n",checks);
}
