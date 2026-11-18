#include "../vr-common/weapon_troika_right_controls.h"
// Same Native GT participant/left-hand owner; no second left decoder or arbiter.
// Only the pure owner state is changed here. No engine, view or C0 writer.
namespace troika_control_live {
namespace wire=gears_troika_control;namespace geo=gears_troika_geometry;namespace hand=gears_weapon_hand;
namespace input=cover_input_live;namespace shared=cover_shared_input_live;namespace source=troika_scene_tracking;
std::atomic<bool> stopped{};
#if defined(GEARS_TROIKA_MOUNTED_SOURCE) && GEARS_TROIKA_MOUNTED_SOURCE
geo::Source previous{},committed_source{};bool have_previous{},interrupted=true,committed_ready{},pending{};
std::uint64_t last_query{},pending_query{};
gears_vr::CoverInputOwnerTicket ticket{};
wire::Decision proposed{},committed{};
gears_vr::LeftHandInteractionLease incumbent{};
gears_troika_right::Controls right_controls{},pending_right{};
unsigned hand_handles[2]{};bool hand_handles_ready{};
bool cohort(geo::Source const& a,geo::Source const& b) noexcept {
    auto const& x=a.frame;auto const& y=b.frame;
    if(a.entry_binding!=b.entry_binding||a.entry_token!=b.entry_token||!gears_rig_tracking::same(x.native,y.native)||
       x.world_generation!=y.world_generation||x.reference_generation!=y.reference_generation||
       x.native_reference_revision!=y.native_reference_revision||x.native_reference.epoch!=y.native_reference.epoch||
       x.publisher_pid!=y.publisher_pid||x.publisher_start!=y.publisher_start||
       x.stereo.session!=y.stereo.session||x.stereo.space_epoch!=y.stereo.space_epoch||
       x.controllers.input_epoch!=y.controllers.input_epoch||x.controllers.profile_generation!=y.controllers.profile_generation||
       x.producer.stream!=y.producer.stream||x.producer.tracking[0]!=y.producer.tracking[0]||x.producer.tracking[1]!=y.producer.tracking[1])return false;
    return hand::same(x.world,y.world)&&hand::same(x.controller,y.controller)&&hand::same(x.turret,y.turret)&&
        hand::same(x.driver,y.driver)&&hand::same(x.driver_mesh,y.driver_mesh)&&hand::same(x.turret_mesh,y.turret_mesh)&&hand::same(x.weapon,y.weapon);
}
// Caller owns existing stereo_mutex, including ordinary/mounted transition.
void interrupt_locked() noexcept {
    try{input::participant.invalidate(input::left_hand_owner);}catch(...){stopped=true;}
    interrupted=true;committed_ready=false;incumbent={};pending=false;right_controls.lost();pending_right.lost();hand_handles_ready=false;
}
bool decide(geo::Request const& q,geo::Response const& geometry,wire::Decision& output) noexcept {
    output={};if(stopped.load()||GetCurrentThreadId()!=q.source.frame.native.game_thread)return false;
    try{
        std::lock_guard<decltype(stereo_mutex)> lock(stereo_mutex);
        if(q.query_id<=last_query||!geo::valid(geometry,q)||!source::geometry_current(q.source))return false;
        last_query=q.query_id;committed_ready=false;
        gears_weapon::TroikaMountedMapping mapped{};
        if(!source::mapped_for(q.source.frame,mapped)){interrupt_locked();return false;}
        auto const now=rig_tracking_live::clock_ns();auto const& raw=q.source.frame.controllers;
        const bool continuous=have_previous&&cohort(previous,q.source)&&raw.sequence>=previous.frame.controllers.sequence&&
            raw.sync_ns>=previous.frame.controllers.sync_ns&&now>=previous.frame.controllers.sync_ns&&now-previous.frame.controllers.sync_ns<=100000000;
        const bool boundary=interrupted||!continuous||mapped.continuity_changed||!hand_handles_ready||
            hand_handles[0]!=geometry.hand_handle[0]||hand_handles[1]!=geometry.hand_handle[1];
        auto const& left=raw.hands[0];
        const auto left_handle=1u<<geometry.hand_handle[0],right_handle=1u<<geometry.hand_handle[1];
        const bool original_left_press=!boundary&&left.last_squeeze_press_sequence>previous.frame.controllers.sequence&&
            left.last_squeeze_press_sequence<=raw.sequence&&(geometry.press_tracked_mask&1)&&geometry.press_contacts[0]==left_handle;
        if(boundary){
            interrupt_locked();auto epochs=input::epochs;
            if(!input::increment(shared::diagnostics.lifecycle)||!input::increment(epochs.world)||!input::increment(epochs.pawn)||
               !input::increment(epochs.cover)||!input::increment(epochs.slot)||!input::increment(epochs.session)||
               !input::increment(epochs.space)||!input::increment(epochs.reference)){stopped=true;return false;}
            input::epochs=epochs;
        }
        previous=q.source;have_previous=true;interrupted=false;
        hand_handles[0]=geometry.hand_handle[0];hand_handles[1]=geometry.hand_handle[1];hand_handles_ready=true;
        gears_vr::CoverInputOwnerFrame frame{};auto const& s=q.source.frame;
        frame.lifecycle=shared::diagnostics.lifecycle;frame.frame=s.scene_invocation;
        frame.cover.sequence=frame.frame;frame.cover.observed_ns=rig_tracking_live::clock_ns();frame.cover.epochs=input::epochs;
        frame.stereo.status=gears_vr::StereoStatus::accepted;frame.stereo.sample=s.stereo;
        frame.stereo.reference=s.reference;frame.stereo.reference_generation=s.reference_generation;
        frame.controllers=&s.controllers;frame.now_ns=rig_tracking_live::clock_ns();frame.units_per_metre=mapped.units_per_metre;
        frame.allow_sim=simulator_operator;frame.allow_index_pinch=false;
        frame.owner_context_valid=geometry.tracked_mask==3; // Either hand's loss interrupts turret control.
        const auto decoded=input::participant.begin(frame,input::left_hand_owner);
        if(!decoded.accepted){interrupt_locked();return false;}
        ticket=decoded.ticket;pending=true;pending_query=q.query_id;
        gears_vr::CoverWeaponContribution weapon{};weapon.state=gears_vr::CoverWeaponState::ready;
        // Acquire ONLY on a real left-palm/left-shaft contact. Existing owner
        // latches prevent an outside press from acquiring on held return.
        weapon.requests.support=original_left_press&&geometry.contacts[0]==left_handle&&geometry.tracked_mask==3;
        if(incumbent.kind==gears_vr::LeftHandInteraction::support&&decoded.left_grip_down&&
           decoded.left_grip_known&&decoded.left_tracked&&geometry.tracked_mask==3)weapon.incumbents[0]=incumbent;
        const auto candidate=input::participant.update_with_decoded(ticket,weapon,input::left_hand_owner);
        if(!candidate.accepted){interrupt_locked();return false;}
        proposed={};proposed.query_id=q.query_id;proposed.publication_sequence=s.publication_sequence;proposed.source_ticket=s.source_ticket;
        proposed.lifecycle=frame.lifecycle;proposed.press_token=decoded.press_token;auto const& d=candidate.left_hand;
        proposed.lease={static_cast<hand::LeaseKind>(d.lease.kind),0,d.lease.lifecycle,d.lease.id};
        proposed.status=static_cast<hand::DecisionStatus>(d.status);proposed.new_grant=d.new_grant?1u:0u;proposed.release_required=d.release_required?1u:0u;
        proposed.left_down=decoded.left_grip_down?1u:0u;proposed.left_known=decoded.left_grip_known?1u:0u;
        proposed.left_tracked=decoded.left_tracked?1u:0u;proposed.decided_ns=rig_tracking_live::clock_ns();
        pending_right=right_controls;
        const auto right=pending_right.advance(s.controllers,s.stereo,proposed.decided_ns,
            d.lease.kind==gears_vr::LeftHandInteraction::support,geometry.contacts[1]==right_handle,
            (geometry.press_tracked_mask&2)&&geometry.press_contacts[1]==right_handle,simulator_operator);
        proposed.right_known=right.grip_known;proposed.right_down=right.grip_down;proposed.right_owned=right.grip_owned;
        proposed.trigger_known=right.trigger_known;proposed.trigger_down=right.trigger_down;proposed.both_owned=right.both_owned;
        proposed.fire_requested=right.fire_requested;proposed.right_press_sequence=right.grip_press_sequence;
        proposed.fire_edge_sequence=right.fire_edge_sequence;proposed.right_window_begin_ns=right.grip_window_begin_ns;
        proposed.fire_window_begin_ns=right.fire_window_begin_ns;
        if(!wire::valid(proposed,q,proposed.decided_ns)||!source::geometry_current(q.source)){interrupt_locked();return false;}
        output=proposed;return true;
    }catch(...){stopped=true;return false;}
}
bool complete(geo::Request const& q,wire::Decision const& decision,bool accepted) noexcept {
    if(GetCurrentThreadId()!=q.source.frame.native.game_thread)return false;
    try{
        std::lock_guard<decltype(stereo_mutex)> lock(stereo_mutex);
        if(q.query_id<last_query)return false; // An old abort cannot clear a newer committed owner.
        if(!accepted){
            last_query=q.query_id;
            if(pending&&pending_query==q.query_id)input::participant.complete(ticket,false,input::left_hand_owner);
            else if(committed_ready&&committed.query_id==q.query_id)input::participant.complete(ticket,false,input::left_hand_owner);
            interrupt_locked();return false;
        }
        if(stopped.load()||!pending||pending_query!=q.query_id||std::memcmp(&decision,&proposed,sizeof decision)||
           !wire::valid(decision,q,rig_tracking_live::clock_ns())||!source::geometry_current(q.source)){
            interrupt_locked();return false;
        }
        const auto result=input::participant.complete(ticket,true,input::left_hand_owner);
        if(!result.accepted||unsigned(result.left_hand.lease.kind)!=unsigned(decision.lease.kind)||
           result.left_hand.lease.lifecycle!=decision.lease.lifecycle||result.left_hand.lease.id!=decision.lease.id){interrupt_locked();return false;}
        incumbent=result.left_hand.lease;right_controls=pending_right;committed_source=q.source;committed=decision;committed_ready=true;pending=false;return true;
    }catch(...){stopped=true;return false;}
}
// A consumer needs this exact real mounted Source AND its own tagged Driver
// context. No ordinary rig projection or engine-operation permission is made.
bool read_for(geo::Source const& expected,wire::Decision& output) noexcept {
    output={};if(stopped.load()||GetCurrentThreadId()!=expected.frame.native.game_thread)return false;
    try{std::lock_guard<decltype(stereo_mutex)> lock(stereo_mutex);
        if(!committed_ready||std::memcmp(&expected,&committed_source,sizeof expected)||!source::geometry_current(expected))return false;
        geo::Request q{};q.query_id=committed.query_id;q.source=expected;
        if(!wire::valid(committed,q,rig_tracking_live::clock_ns()))return false;output=committed;return true;
    }catch(...){return false;}
}
// Non-authoritative cached routing hint only. The caller must acquire the
// existing world lease BEFORE read_for performs actual mounted actor checks.
bool driver_hint(std::uintptr_t mesh,geo::Source& output) noexcept {
    output={};if(stopped.load())return false;
    try{std::lock_guard<decltype(stereo_mutex)> lock(stereo_mutex);
        if(!committed_ready||mesh!=committed_source.frame.driver_mesh.address||
           GetCurrentThreadId()!=committed_source.frame.native.game_thread||
           !geo::source_valid(committed_source,rig_tracking_live::clock_ns(),simulator_operator))return false;
        output=committed_source;return true;
    }catch(...){return false;}
}
#else
void interrupt_locked() noexcept {}
bool decide(geo::Request const&,geo::Response const&,wire::Decision&) noexcept {return false;}
bool complete(geo::Request const&,wire::Decision const&,bool) noexcept {return false;}
bool read_for(geo::Source const&,wire::Decision& output) noexcept {output={};return false;}
bool driver_hint(std::uintptr_t,geo::Source& output) noexcept {output={};return false;}
#endif
gears_rig_tracking::Reply read(geo::Source const* input,wire::Decision* output) noexcept {
    weapon_hand_runtime::detail::Call call;wire::Decision empty{};
    if(!output||!hand::weapon_broker_win32::copy(output,&empty,sizeof empty))return gears_rig_tracking::Reply::invalid;
    geo::Source q{};if(!input||!hand::weapon_broker_win32::copy(&q,input,sizeof q))return gears_rig_tracking::Reply::invalid;
    if(stopped.load())return gears_rig_tracking::Reply::stopping;
    wire::Decision value{},closing{};
    if(!read_for(q,value))return gears_rig_tracking::Reply::unavailable;
    if(!hand::weapon_broker_win32::copy(output,&value,sizeof value))return gears_rig_tracking::Reply::invalid;
    if(!read_for(q,closing)||std::memcmp(&value,&closing,sizeof value)){
        hand::weapon_broker_win32::copy(output,&empty,sizeof empty);return gears_rig_tracking::Reply::unavailable;
    }
    return gears_rig_tracking::Reply::accepted;
}
void stop() noexcept {stopped.store(true);}
} // namespace troika_control_live
