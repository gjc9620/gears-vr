#pragma once
#include "../vr-common/weapon_hand_participant.h"
#include "../vr-common/weapon_input_router.h"
#include "../vr-common/equipment_intent_bridge.h"
#include "../vr-common/controller_continuity_frame.h"
#include <cstdint>

// Owned PG GT input state, not a shared-memory producer or game authority.
// Call only with the current authenticated Native Frame. No Frame pointer,
// engine object, callback, mapping or original-call permission is retained.
namespace gears_equipment_input {
using Frame=gears_weapon_hand::Frame;
using Calibration=gears_weapon::ArmspanCalibration;
using Event=gears_weapon::EquipmentIntentEvent;
// Caller supplies this only after matching both authenticated read-only input
// publications to this exact Native Frame. This private POD grants no action.
struct Continuity {
    gears_hand_continuity::TrackedInput tracking{};
    gears_hand_continuity::Publisher publisher{};
    gears_hand_continuity::TrackedInput right_press{};
};
enum class Status : std::uint8_t {
    reset,baseline,quiet,event,invalid_frame,invalid_calibration,invalid_grip,
    invalid_clock,cohort_changed,source_gap,source_regressed,counter_regressed,
    invalid_press,cursor_mismatch,exhausted
};
inline char const* status_name(Status s) noexcept {
    switch(s){
#define GEARS_EQUIPMENT_INPUT_STATUS(name) case Status::name:return #name
        GEARS_EQUIPMENT_INPUT_STATUS(reset);GEARS_EQUIPMENT_INPUT_STATUS(baseline);
        GEARS_EQUIPMENT_INPUT_STATUS(quiet);GEARS_EQUIPMENT_INPUT_STATUS(event);
        GEARS_EQUIPMENT_INPUT_STATUS(invalid_frame);GEARS_EQUIPMENT_INPUT_STATUS(invalid_calibration);
        GEARS_EQUIPMENT_INPUT_STATUS(invalid_grip);GEARS_EQUIPMENT_INPUT_STATUS(invalid_clock);
        GEARS_EQUIPMENT_INPUT_STATUS(cohort_changed);GEARS_EQUIPMENT_INPUT_STATUS(source_gap);
        GEARS_EQUIPMENT_INPUT_STATUS(source_regressed);GEARS_EQUIPMENT_INPUT_STATUS(counter_regressed);
        GEARS_EQUIPMENT_INPUT_STATUS(invalid_press);GEARS_EQUIPMENT_INPUT_STATUS(cursor_mismatch);
        GEARS_EQUIPMENT_INPUT_STATUS(exhausted);
#undef GEARS_EQUIPMENT_INPUT_STATUS
    }
    return "invalid_status";
}
struct Evidence {
    Status status{Status::reset};
    std::uint64_t serial{},sequence{},press_sequence{},press_counter{},created_ms{};
    // Without an exact ring edge, created_ms uses the earlier observed sync as
    // its conservative bound. An authenticated recorded_press_sync_ns instead
    // supplies the original producer edge stamp. Neither uses the poll as a
    // new input timestamp. Keep both original stamps/sequence evidence.
    std::int64_t previous_sync_ns{},sync_ns{},received_ns{};
    std::int64_t observed_ns{},previous_observed_ns{};
    bool continuous_source_gap{};
    enum class Gap : std::uint8_t {none,current_sync_future,current_sync_stale,sync_not_advancing,producer_sync_gap} gap{};
    std::int64_t recorded_press_sync_ns{};
    unsigned control{gears_vr::control_squeeze};
};
inline char const* gap_name(Evidence::Gap gap) noexcept {
    switch(gap){
    case Evidence::Gap::none:return "none";
    case Evidence::Gap::current_sync_future:return "current_sync_future";
    case Evidence::Gap::current_sync_stale:return "current_sync_stale";
    case Evidence::Gap::sync_not_advancing:return "sync_not_advancing";
    case Evidence::Gap::producer_sync_gap:return "producer_sync_gap";
    }
    return "invalid_gap";
}

class Input {
    struct Cohort {
        gears_weapon_hand::FrameKey native{};
        gears_weapon_hand::ObjectIdentity world{},controller{},pawn{},mesh{},weapon{};
        gears_vr::StereoSource source{};
        std::uint64_t session{},space{},profile{},input{},reference_generation{};
        gears_vr::Quaternion orientation{};
        Calibration calibration{};
    } cohort_{};
    gears_weapon::WeaponInputRouter router_{};
    Evidence evidence_{};
    bool bound_{};
    std::uint64_t serial_{},sequence_{},press_sequence_{},last_ms_{};
    std::uint32_t pressed_{},released_{};
    std::uint32_t index_pressed_{},index_released_{};
    bool index_eligible_{};
    std::int64_t sync_ns_{},received_ns_{},last_ns_{};
    gears_hand_continuity::Publisher publisher_{};
    std::uint64_t stream_{},right_tracking_{};bool continuity_known_{};
    static constexpr std::int64_t maximum_gap_ns=100000000;
    static Cohort cohort(Frame const& f,Calibration const& calibration) noexcept {
        auto const& c=f.controllers;
        return {f.key,f.world,f.controller,f.pawn,f.mesh,f.weapon,c.source,c.session,c.space_epoch,
            c.profile_generation,c.input_epoch,f.reference_generation,f.reference.orientation,calibration};
    }
    static bool same(Cohort const& a,Cohort const& b) noexcept {
        using gears_weapon_hand::same;
        auto const& x=a.native;auto const& y=b.native;
        return x.game_start==y.game_start&&x.native_started_qpc==y.native_started_qpc&&
            x.registration_cookie==y.registration_cookie&&x.lifecycle==y.lifecycle&&
            x.world_generation==y.world_generation&&x.game_pid==y.game_pid&&x.thread_id==y.thread_id&&
            same(a.world,b.world)&&same(a.controller,b.controller)&&same(a.pawn,b.pawn)&&
            same(a.mesh,b.mesh)&&same(a.weapon,b.weapon)&&a.source==b.source&&a.session==b.session&&
            a.space==b.space&&a.profile==b.profile&&a.input==b.input&&
            a.reference_generation==b.reference_generation&&
            a.orientation.x==b.orientation.x&&a.orientation.y==b.orientation.y&&
            a.orientation.z==b.orientation.z&&a.orientation.w==b.orientation.w&&
            a.calibration.version==b.calibration.version&&
            a.calibration.armspan_metres==b.calibration.armspan_metres&&
            a.calibration.hands_below_head_metres==b.calibration.hands_below_head_metres;
    }
    // Route right squeeze and optional unheld index acquisition. Other controls cannot
    // emit actions or make the existing router discard a valid right press.
    static gears_vr::ControllerInputFrame right_only(Frame const& f,bool index=false) noexcept {
        auto c=f.controllers;
        for(unsigned h=0;h<2;++h)for(unsigned b=0;b<gears_vr::control_count;++b)
            if(h!=1||(b!=gears_vr::control_squeeze&&(!index||b!=gears_vr::control_trigger)))c.hands[h].pressed[b]=c.hands[h].released[b]=0;
        return c;
    }
    static bool valid(Continuity const& c,Frame const& f) noexcept {
        return c.publisher.pid&&c.publisher.start&&gears_hand_continuity::tracked_input_shape_valid(c.tracking)&&
            gears_hand_continuity::same_input(c.tracking.input,f.controllers);
    }
    bool same_continuity(Continuity const& c) const noexcept {
        return c.publisher.pid==publisher_.pid&&c.publisher.start==publisher_.start&&
            c.tracking.stream_epoch==stream_&&c.tracking.tracking_epoch[1]==right_tracking_;
    }
    void observe(Frame const& f,std::int64_t now_ns,std::uint64_t now_ms,Continuity const* continuity,bool index=false) noexcept {
        auto const& c=f.controllers;auto const& r=c.hands[1];
        sequence_=c.sequence;sync_ns_=c.sync_ns;received_ns_=c.received_ns;
        pressed_=r.pressed[gears_vr::control_squeeze];released_=r.released[gears_vr::control_squeeze];
        press_sequence_=r.last_squeeze_press_sequence;last_ns_=now_ns;last_ms_=now_ms;
        index_pressed_=r.pressed[gears_vr::control_trigger];index_released_=r.released[gears_vr::control_trigger];index_eligible_=index;
        continuity_known_=continuity!=nullptr;
        if(continuity){publisher_=continuity->publisher;stream_=continuity->tracking.stream_epoch;
            right_tracking_=continuity->tracking.tracking_epoch[1];}
    }
    bool reject(Status status,std::uint64_t& cursor) noexcept {
        bound_=continuity_known_=index_eligible_=false;router_.reset();cursor=UINT64_MAX;evidence_.status=status;return false;
    }
    bool reject_gap(Evidence::Gap gap,std::uint64_t& cursor) noexcept {
        evidence_.gap=gap;return reject(Status::source_gap,cursor);
    }
public:
    Input()=default;Input(Input const&)=delete;Input& operator=(Input const&)=delete;
    // Drop the input baseline; never reuse a previously emitted serial.
    void reset() noexcept {bound_=continuity_known_=index_eligible_=false;router_.reset();evidence_={};}
    Evidence const& evidence() const noexcept {return evidence_;}
    // Read-planning only, never a press/continuity grant. next() can consume a
    // historical squeeze witness only for an increased counter in this bound
    // cohort. Baselines, releases and index acquisition use current tracking.
    bool needs_recorded_right_press(Frame const& f,Calibration const& calibration,std::uint64_t cursor) const noexcept {
        return bound_&&cursor!=UINT64_MAX&&same(cohort_,cohort(f,calibration))&&
            f.controllers.hands[1].pressed[gears_vr::control_squeeze]>pressed_;
    }
    bool next(Frame const& f,Calibration const& calibration,std::int64_t now_ns,
              std::uint64_t now_ms,std::uint64_t& cursor,Event& out,Continuity const* continuity=nullptr,bool allow_index_acquire=false) noexcept {
        evidence_={Status::quiet,serial_,f.controllers.sequence,f.controllers.hands[1].last_squeeze_press_sequence,
            f.controllers.hands[1].pressed[gears_vr::control_squeeze],0,bound_?sync_ns_:0,
            f.controllers.sync_ns,f.controllers.received_ns,now_ns,bound_?last_ns_:0};
        if(!gears_weapon_hand::frame_active(f,now_ns))return reject(Status::invalid_frame,cursor);
        if(continuity&&!valid(*continuity,f))return reject(Status::invalid_frame,cursor);
        if(!calibration.valid())return reject(Status::invalid_calibration,cursor);
        auto const& c=f.controllers;auto const& r=c.hands[1];
        const bool index_mode=allow_index_acquire&&r.trigger_active==1;
        if((!r.squeeze_active&&!index_mode)||!gears_vr::controller_pose_usable(r.grip,r.grip_flags,r.grip_active))
            return reject(Status::invalid_grip,cursor);
        // received_ns can be fresh while an old sync is repeated. Both original
        // clocks must be current; neither is replaced with this poll's clock.
        if(c.sync_ns>now_ns)return reject_gap(Evidence::Gap::current_sync_future,cursor);
        if(now_ns-c.sync_ns>maximum_gap_ns)return reject_gap(Evidence::Gap::current_sync_stale,cursor);
        if(bound_&&(now_ns<last_ns_||now_ms<last_ms_))return reject(Status::invalid_clock,cursor);
        const auto key=cohort(f,calibration);
        const gears_vr::Pose body{f.stereo.head.position,f.reference.orientation};
        const auto routed_frame=right_only(f,index_mode);
        if(cursor==UINT64_MAX){
            router_.reset();
            if(router_.advance(routed_frame,f.stereo,calibration,body,now_ns,(f.flags&gears_weapon_hand::allow_sim)!=0).status!=gears_weapon::RouterStatus::baseline)
                return reject(Status::invalid_frame,cursor);
            cohort_=key;bound_=true;observe(f,now_ns,now_ms,continuity,index_mode);cursor=serial_;
            evidence_.status=Status::baseline;return false;
        }
        if(!bound_||!same(cohort_,key))return reject(Status::cohort_changed,cursor);
        if(continuity_known_&&continuity&&!same_continuity(*continuity))return reject(Status::cohort_changed,cursor);
        if(cursor!=serial_)return reject(Status::cursor_mismatch,cursor);
        const auto press=r.pressed[gears_vr::control_squeeze],release=r.released[gears_vr::control_squeeze];
        if(c.sequence<sequence_||c.sync_ns<sync_ns_||c.received_ns<received_ns_)
            return reject(Status::source_regressed,cursor);
        if(press<pressed_||release<released_)return reject(Status::counter_regressed,cursor);
        if(index_mode&&index_eligible_&&(r.pressed[gears_vr::control_trigger]<index_pressed_||
           r.released[gears_vr::control_trigger]<index_released_))return reject(Status::counter_regressed,cursor);
        if(c.sequence==sequence_){
            if(c.sync_ns!=sync_ns_||c.received_ns!=received_ns_||press!=pressed_||release!=released_||
               r.last_squeeze_press_sequence!=press_sequence_||
               (index_mode&&index_eligible_&&(r.pressed[gears_vr::control_trigger]!=index_pressed_||
                r.released[gears_vr::control_trigger]!=index_released_)))return reject(Status::source_regressed,cursor);
            last_ns_=now_ns;last_ms_=now_ms;return false;
        }
        if(c.sync_ns<=sync_ns_)return reject_gap(Evidence::Gap::sync_not_advancing,cursor);
        // Freshness of this frame was checked against now above. Continuity is
        // the interval between the two original producer sync stamps, not the
        // previous stamp plus this consumer's processing latency. Preserve the
        // earlier stamp below as the conservative event creation bound.
        const bool recorded_press=press>pressed_&&continuity_known_&&continuity&&same_continuity(*continuity)&&
            gears_hand_continuity::recorded_right_squeeze_press(continuity->tracking,continuity->right_press)&&
            continuity->right_press.input.sequence>sequence_&&
            continuity->right_press.input.sync_ns<=now_ns&&
            now_ns-continuity->right_press.input.sync_ns<=maximum_gap_ns;
        if(c.sync_ns-sync_ns_>maximum_gap_ns){
            // A game poll can cross100ms because two still-fresh producer
            // samples have different read latency. Require actual publisher
            // stream/right-tracking continuity plus either a <=100ms consumer
            // interval OR an exact <=100ms recorded edge. A quiet, fresh poll
            // may only advance the baseline: it emits no action. A source/
            // pause epoch still fails. No deadline or edge time is renewed.
            if(!continuity_known_||!continuity||!same_continuity(*continuity)||
               (now_ns-last_ns_>maximum_gap_ns&&press>pressed_&&!recorded_press))
                return reject_gap(Evidence::Gap::producer_sync_gap,cursor);
            evidence_.continuous_source_gap=true;
        }
        const auto previous_sync=sync_ns_;
        const bool new_press=press>pressed_;
        const bool index_press=!new_press&&index_mode&&index_eligible_&&
            r.pressed[gears_vr::control_trigger]>index_pressed_;
        if(new_press&&(!r.last_squeeze_press_sequence||r.last_squeeze_press_sequence<=sequence_||
           !gears_vr::controller_pose_usable(r.last_squeeze_press_grip,r.last_squeeze_press_flags,1)))
            return reject(Status::invalid_press,cursor);
        // There is no cached producer index-press pose. Admit only ONE new,
        // still-held edge on this actual current grip, within the original
        // short observation interval. Do not relabel it as a squeeze edge,
        // reconstruct a missed pose, or repurpose an already held trigger.
        if(index_press&&(r.pressed[gears_vr::control_trigger]-index_pressed_!=1||
           r.released[gears_vr::control_trigger]!=index_released_||
           !gears_weapon_hand::control_held(r,gears_vr::control_trigger)||c.sync_ns-sync_ns_>maximum_gap_ns))
            return reject(Status::invalid_press,cursor);
        const auto routed=router_.advance(routed_frame,f.stereo,calibration,body,now_ns,(f.flags&gears_weapon_hand::allow_sim)!=0);
        observe(f,now_ns,now_ms,continuity,index_mode);
        if(!new_press&&!index_press)return false;
        auto slot=gears_weapon::BodySlot::none;
        if(index_press&&routed.status==gears_weapon::RouterStatus::accepted){
            const auto candidate=gears_weapon::body_slot(calibration,body,r.grip.position,1);
            if(candidate==gears_weapon::BodySlot::weapon_1||candidate==gears_weapon::BodySlot::weapon_2)slot=candidate;
        }else if(!index_press&&routed.status==gears_weapon::RouterStatus::accepted){
            for(unsigned i=0;i<routed.count;++i){const auto candidate=gears_weapon::equipment_body_slot(routed.intents[i]);
                if(candidate!=gears_weapon::BodySlot::none)slot=candidate;}
        }else if(!index_press&&routed.status==gears_weapon::RouterStatus::ambiguous){
            // The general router rejects multiple unseen edges. Here the wire
            // certifies ONE latest press pose/sequence inside the bounded sync
            // interval. Discard older presses; route only that latest edge with
            // the same existing body-slot geometry, never replay its counter.
            slot=gears_weapon::body_slot(calibration,body,r.last_squeeze_press_grip.position,1);
        }
        const auto inventory_slot=gears_weapon::original_inventory_slot(slot);
        if(!inventory_slot||!f.weapon.address)return false;
        // A genuinely slow game poll may still see a new edge with its exact
        // <=100ms original stamp in the existing authenticated ring. Never
        // renew an old edge to now or enlarge the input/action age bounds.
        const auto event_sync=recorded_press?continuity->right_press.input.sync_ns:previous_sync;
        const auto age_ns=std::uint64_t(now_ns-event_sync);
        const auto age_ms=(age_ns+999999)/1000000;
        if(now_ms<age_ms)return reject(Status::invalid_clock,cursor);
        if(serial_>=UINT64_MAX-1)return reject(Status::exhausted,cursor);
        Event event{};event.serial=++serial_;event.created_ms=now_ms-age_ms;
        event.input_session=c.session;event.input_sequence=index_press?c.sequence:r.last_squeeze_press_sequence;
        event.space_epoch=c.space_epoch;event.input_epoch=c.input_epoch;
        event.world_generation=f.key.world_generation;event.controller=f.controller.address;
        event.pawn=f.pawn.address;event.weapon=f.weapon.address;event.inventory_slot=*inventory_slot;
        if(index_press)event.kind=gears_weapon::EquipmentIntentKind::index_acquire_slot;
        const auto previous_observed=evidence_.previous_observed_ns;const auto continuous=evidence_.continuous_source_gap;
        evidence_={Status::event,event.serial,c.sequence,r.last_squeeze_press_sequence,press,event.created_ms,
            previous_sync,c.sync_ns,c.received_ns,now_ns,previous_observed,continuous};
        evidence_.recorded_press_sync_ns=recorded_press?event_sync:0;
        evidence_.control=index_press?gears_vr::control_trigger:gears_vr::control_squeeze;
        if(index_press){evidence_.press_sequence=c.sequence;evidence_.press_counter=r.pressed[gears_vr::control_trigger];}
        cursor=event.serial;out=event;return true;
    }
};
} // namespace gears_equipment_input
