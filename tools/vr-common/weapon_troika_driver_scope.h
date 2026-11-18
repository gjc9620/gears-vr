#pragma once
#include "troika_mounted_control.h"
#include "weapon_troika_mounted_space.h"

// Distinct Native mounted Driver source for the EXISTING outer Tick/C0 lane.
// This type is never an ordinary CoreSnapshot/hand Frame or a write permit.
namespace gears_troika_driver {
namespace geo=gears_troika_geometry;namespace control=gears_troika_control;
struct Snapshot {geo::Source source{};control::Decision control{};gears_weapon::TroikaMountedMapping mapping{};};
struct Entry {std::uintptr_t address{},mesh{},caller{},arg1{},arg2{},arg3{},stack{};std::uint64_t invocation{};unsigned thread{},kind{};};
inline bool same(Entry const& a,Entry const& b) noexcept {
    return a.address==b.address&&a.mesh==b.mesh&&a.caller==b.caller&&a.arg1==b.arg1&&a.arg2==b.arg2&&a.arg3==b.arg3&&a.stack==b.stack&&
        a.invocation==b.invocation&&a.thread==b.thread&&a.kind==b.kind;
}
inline bool valid(Snapshot const& x,std::int64_t now) noexcept {
    geo::Request q{};q.source=x.source;q.query_id=x.control.query_id;
    if(!control::valid(x.control,q,now))return false;
    auto const& f=x.source.frame;auto const& m=x.mapping;auto const& i=m.identity;
    unsigned tracked{};for(unsigned side=0;side<2;++side){auto const& hand=f.controllers.hands[side];
        if(gears_vr::controller_pose_usable(hand.grip,hand.grip_flags,hand.grip_active))tracked|=1u<<side;}
    if(m.publication_sequence!=f.publication_sequence||m.source_ticket!=f.source_ticket||!i.valid()||
       i.world!=f.world.address||i.world_generation!=f.world_generation||i.controller!=f.controller.address||i.pawn!=f.driver.address||
       i.turret!=f.turret.address||i.weapon!=f.weapon.address||i.turret_generation!=x.source.entry_binding||i.weapon_generation!=x.source.entry_binding||
       i.xr_session!=f.controllers.session||i.space_epoch!=f.controllers.space_epoch||i.input_epoch!=f.controllers.input_epoch||i.profile_generation!=f.controllers.profile_generation||
       m.hands.input_sequence!=f.controllers.sequence||m.hands.input_sync_ns!=f.controllers.sync_ns||m.hands.input_received_ns!=f.controllers.received_ns||
       m.hands.stereo_received_ns!=f.stereo.received_ns||m.hands.tracked_mask!=tracked||!std::isfinite(m.units_per_metre)||
       m.units_per_metre<10||m.units_per_metre>500||!gears_vr::stereo_pose_valid(m.world_head))return false;
    for(unsigned side=0;side<2;++side)if((tracked&(1u<<side))&&!gears_vr::stereo_pose_valid(m.hands.world[side]))return false;
    return true;
}
class Scope {
    Snapshot source_{};Entry tick_{},compose_{};bool active_{},compose_active_{},compose_seen_{},poisoned_{};
public:
    bool active() const noexcept {return active_;}
    bool composing() const noexcept {return compose_active_;}
    bool poisoned() const noexcept {return poisoned_;}
    Snapshot const& source() const noexcept {return source_;}
    Entry const& tick() const noexcept {return tick_;}
    bool copy_tick(Entry const& e,std::int64_t now,Snapshot& out) const noexcept {
        if(!active_||poisoned_||!same(e,tick_)||!valid(source_,now))return false;out=source_;return true;
    }
    bool begin_tick(Entry const& e,Snapshot const& source,std::int64_t now,std::uintptr_t certified_caller,bool attested,bool ordinary_active) noexcept {
        if(active_){poisoned_=true;return false;}
        if(ordinary_active||!attested||!certified_caller||e.kind!=2||e.caller!=certified_caller||!e.address||!e.invocation||
           e.thread!=source.source.frame.native.game_thread||e.mesh!=source.source.frame.driver_mesh.address||!valid(source,now))return false;
        source_=source;tick_=e;compose_={};compose_active_=compose_seen_=poisoned_=false;active_=true;return true;
    }
    bool begin_compose(Entry const& e,std::int64_t now,std::uintptr_t certified_caller,bool attested) noexcept {
        if(!active_)return false;
        if(poisoned_||compose_seen_||!attested||!certified_caller||e.kind!=0||!e.address||!e.invocation||
           e.thread!=tick_.thread||e.mesh!=tick_.mesh||e.caller!=certified_caller||e.arg1!=e.mesh+0x27c||e.arg2!=e.mesh+0x28c||!valid(source_,now)){
            poisoned_=true;return false;
        }
        compose_=e;compose_active_=compose_seen_=true;return true;
    }
    bool copy(Entry const& e,std::int64_t now,Snapshot& out) const noexcept {
        if(!active_||!compose_active_||poisoned_||!same(e,compose_)||!valid(source_,now))return false;
        out=source_;return true;
    }
    bool matches_compose(Entry const& e) const noexcept {
        return active_&&compose_active_&&same(e,compose_);
    }
    // Existing transaction closing only: no source-age renewal and no new
    // preparation/commit permission. Stop may close the exact retained event.
    bool retained(Entry const& e,Snapshot& out) const noexcept {
        if(!matches_compose(e)||poisoned_)return false;out=source_;return true;
    }
    bool end_compose(Entry const& e,bool returned) noexcept {
        if(!active_||!compose_active_||!same(e,compose_)){if(active_)poisoned_=true;return false;}
        compose_active_=false;if(!returned)poisoned_=true;return true;
    }
    bool end_tick(Entry const& e,bool returned) noexcept {
        if(!active_||compose_active_||!same(e,tick_)){if(active_)poisoned_=true;return false;}
        if(!returned)poisoned_=true;active_=false;return true;
    }
};
} // namespace gears_troika_driver
