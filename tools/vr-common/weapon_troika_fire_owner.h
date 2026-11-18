#pragma once
#include "troika_mounted_control.h"

// Original-call responsibility, separate from input intent and the mount ledger.
// No bridge, decoder, game pointer, timer for weapon warmup, or pose writer.
namespace gears_troika_fire {
namespace geo=gears_troika_geometry;namespace control=gears_troika_control;namespace hand=gears_weapon_hand;
enum class Dispatch:unsigned {not_dispatched,returned,uncertain};
struct Run {geo::Source source{};control::Decision control{};std::uint64_t serial{};};
class Backend {
public:
    virtual ~Backend()=default;
    // A distinct actual aim/muzzle/hand consumer must prove readiness; an input
    // receipt or a rollback-only boundary receipt cannot satisfy this check.
    virtual bool qualified(Run const&) noexcept=0;
    virtual Dispatch start(Run const&) noexcept=0;
    virtual Dispatch stop(Run const&) noexcept=0;
    virtual bool quiescent(Run const&) noexcept=0;
    virtual bool extinct(Run const&) noexcept=0;
};
inline bool same_mount(geo::Source const& a,geo::Source const& b) noexcept {
    auto const& x=a.frame;auto const& y=b.frame;
    return a.entry_binding==b.entry_binding&&a.entry_token==b.entry_token&&gears_rig_tracking::same(x.native,y.native)&&
        x.world_generation==y.world_generation&&hand::same(x.world,y.world)&&hand::same(x.controller,y.controller)&&
        hand::same(x.turret,y.turret)&&hand::same(x.driver,y.driver)&&hand::same(x.weapon,y.weapon)&&
        hand::same(x.driver_mesh,y.driver_mesh)&&hand::same(x.turret_mesh,y.turret_mesh);
}
inline bool same_input_life(geo::Source const& a,geo::Source const& b) noexcept {
    auto const& x=a.frame;auto const& y=b.frame;
    return same_mount(a,b)&&x.reference_generation==y.reference_generation&&x.native_reference_revision==y.native_reference_revision&&
        x.native_reference.epoch==y.native_reference.epoch&&x.publisher_pid==y.publisher_pid&&x.publisher_start==y.publisher_start&&
        x.controllers.source==y.controllers.source&&x.controllers.session==y.controllers.session&&x.controllers.space_epoch==y.controllers.space_epoch&&
        x.controllers.input_epoch==y.controllers.input_epoch&&x.controllers.profile_generation==y.controllers.profile_generation&&
        x.producer.stream==y.producer.stream&&x.producer.tracking[0]==y.producer.tracking[0]&&x.producer.tracking[1]==y.producer.tracking[1];
}
class Owner {
    Run run_{};geo::Source seen_{};std::uint64_t next_{1},consumed_edge_{};
    bool seen_ready_{},owed_{},stop_requested_{},stop_dispatched_{};
    Dispatch start_result_{Dispatch::not_dispatched},stop_result_{Dispatch::not_dispatched};
    void close() noexcept {owed_=stop_requested_=stop_dispatched_=false;run_={};}
public:
    bool owed() const noexcept {return owed_;}
    bool stopping() const noexcept {return stop_requested_;}
    Run const& run() const noexcept {return run_;}
    Dispatch start_result() const noexcept {return start_result_;}
    Dispatch stop_result() const noexcept {return stop_result_;}
    void request_stop() noexcept {if(owed_)stop_requested_=true;}
    void service(Backend& backend) noexcept {
        if(!owed_)return;
        // Extinction is a separately authenticated object/incarnation proof;
        // a read failure, new occupant, or missing input is never that proof.
        if(backend.extinct(run_)){close();return;}
        if(!stop_requested_)return;
        if(backend.quiescent(run_)){close();return;}
        if(!stop_dispatched_){
            stop_dispatched_=true; // Before the original call, including faults.
            stop_result_=Dispatch::uncertain;
            stop_result_=backend.stop(run_);
            if(stop_result_==Dispatch::not_dispatched)stop_dispatched_=false;
        }
        // Normal return alone does not establish that warmup/pending fire ended.
        // Even uncertain stop is never replayed; only observation can close it.
        if(backend.quiescent(run_))close();
    }
    bool offer(geo::Source const& source,control::Decision const& decision,std::int64_t now,Backend& backend) noexcept {
        geo::Request q{};q.query_id=decision.query_id;q.source=source;
        if(!control::valid(decision,q,now)){request_stop();service(backend);return false;}
        if(owed_){
            Run current{source,decision,run_.serial};
            if(!same_input_life(source,run_.source)||!decision.fire_requested||
               decision.fire_edge_sequence!=run_.control.fire_edge_sequence||
               decision.fire_window_begin_ns!=run_.control.fire_window_begin_ns||
               source.frame.publication_sequence<run_.source.frame.publication_sequence||
               source.frame.controllers.sequence<run_.source.frame.controllers.sequence||!backend.qualified(current))request_stop();
            service(backend);return false; // Never close old and start new in one call.
        }
        if(seen_ready_&&same_input_life(source,seen_)){
            if(source.frame.publication_sequence<=seen_.frame.publication_sequence||
               source.frame.controllers.sequence<seen_.frame.controllers.sequence||
               source.frame.controllers.sync_ns<seen_.frame.controllers.sync_ns)return false;
        }else consumed_edge_=0;
        seen_=source;seen_ready_=true;
        if(!decision.fire_requested||decision.fire_edge_sequence<=consumed_edge_)return false;
        if(now<decision.fire_window_begin_ns||now-decision.fire_window_begin_ns>100000000){
            consumed_edge_=decision.fire_edge_sequence;return false;
        }
        if(next_==UINT64_MAX)return false;
        Run candidate{source,decision,next_};
        if(!backend.qualified(candidate))return false;
        // One original ButtonPress R2 at most. An unknown call owns
        // stop responsibility immediately and cannot be retried from held R2.
        consumed_edge_=decision.fire_edge_sequence;++next_;run_=candidate;
        owed_=true;stop_requested_=true;stop_dispatched_=false;start_result_=Dispatch::uncertain;
        start_result_=backend.start(run_);
        if(start_result_==Dispatch::not_dispatched){close();return false;}
        if(start_result_==Dispatch::returned)stop_requested_=false;
        return true;
    }
};
} // namespace gears_troika_fire
