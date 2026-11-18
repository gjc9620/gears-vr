#include "lancer_socket_rank.h"
// Ordinary GT publication for an actual part lease or a neutral proximity
// socket. Proximity fits skin only and never supplies that lease. Destination:
// existing Native C0 skin solver, never a controller/input transform.
#include "../vr-common/weapon_lancer_grasp_pose.h"
#include <optional>
#include "../vr-common/weapon_lancer_main_hand.h"
#include "../vr-common/weapon_lancer_main_hold.h"
#include "../vr-common/weapon_lancer_part_hand.h"
#include "../vr-common/weapon_lancer_natural_magazine.h"
#include "../vr-common/weapon_lancer_owner_grabposes.h"
#include "../vr-common/weapon_lancer_support_socket.h"
#include "../vr-common/hand_part_grasp.h"
#include "../vr-common/weapon_hammerburst_support.h"
#include "../vr-common/weapon_hammerburst_owner_magazine.h"
#include "../vr-common/weapon_hammerburst_part_hand.h"
#include "../vr-common/weapon_support_socket_mode.h"
namespace lancer_grasp {
bool source_scope_eligible(gears_weapon_hand::Frame const& f) noexcept {
    return nativelancerfollow::bound_key_known&&nativelancerfollow::bound_is_lancer&&
        gears_weapon_hand::same(nativelancerfollow::bound_key.weapon,f.weapon)&&
        nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation();
}
bool hammerburst_source_scope_eligible(gears_weapon_hand::Frame const& f) noexcept {
    return nativelancerfollow::bound_key_known&&nativelancerfollow::bound_is_hammerburst&&
        gears_weapon_hand::same(nativelancerfollow::bound_key.weapon,f.weapon)&&
        nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation();
}
bool boltok_source_scope_eligible(gears_weapon_hand::Frame const& f) noexcept {
    return nativelancerfollow::bound_key_known&&nativelancerfollow::bound_is_boltok&&
        gears_weapon_hand::same(nativelancerfollow::bound_key.weapon,f.weapon)&&
        nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation();
}

namespace api=gears_hand_object_grasp;namespace hand=gears_weapon_hand;
using P=gears_vr::Pose;using Q=gears_vr::Quaternion;using V=gears_vr::Vector;
std::uint64_t magazine_token{};hand::ObjectIdentity magazine_weapon{};
hand::FrameKey magazine_key{};std::uint64_t magazine_reference{},magazine_session{},magazine_space{},
    magazine_input{},magazine_profile{},magazine_hold{},magazine_stream{},magazine_tracking{};
bool magazine_known{};
unsigned proximity_gate{}; // Existing bounded report only; no new reader or authority.
hand::Frame main_hold_parent{};hand::ExistingActionLifetime main_hold_lifetime{};
gears_lancer_main_hold::TrackingPin main_hold_tracking;
std::uint64_t main_hold_generation{};bool main_hold_pin_known{};
bool main_hold_boltok{};
// HB/Boltok POD observations; no callback I/O and no additional source reads.
struct MainHoldTrace {
    unsigned phase{},gate{},reader_failure{},lifetime_failure{},game_flags{};bool pin_known{};
    std::uint64_t frame{},sequence{},generation{},publisher_pid{},publisher_start{},stream{},left_tracking{},right_tracking{};
    std::int64_t source_sync{},observed_ns{},begin_ns{};
};
std::array<MainHoldTrace,16> main_hold_trace{};unsigned main_hold_trace_count{};
bool main_hold_trace_hb{};
void record_main_hold(unsigned phase,unsigned gate,hand::Frame const& f,std::int64_t observed_ns=0,
    std::int64_t begin=0,unsigned reader=UINT_MAX,unsigned lifetime=UINT_MAX,unsigned game_flags=0,
    gears_hand_continuity::TrackedInput const* raw=nullptr,gears_hand_continuity::Publisher const* publisher=nullptr) noexcept {
    if(!main_hold_trace_hb||!weaponhand::current_gt(nullptr)||main_hold_trace_count>=main_hold_trace.size())return;
    for(unsigned i=0;i<main_hold_trace_count;++i){auto const& old=main_hold_trace[i];
        if(old.phase==phase&&old.gate==gate&&old.pin_known==main_hold_pin_known&&
           old.generation==main_hold_generation&&old.reader_failure==reader&&old.lifetime_failure==lifetime&&old.game_flags==game_flags)return;}
    auto& row=main_hold_trace[main_hold_trace_count++];row.phase=phase;row.gate=gate;
    row.reader_failure=reader;row.lifetime_failure=lifetime;row.game_flags=game_flags;row.pin_known=main_hold_pin_known;
    row.frame=f.key.frame;row.sequence=f.controllers.sequence;row.source_sync=f.controllers.sync_ns;
    row.generation=main_hold_generation;row.observed_ns=observed_ns;row.begin_ns=begin;
    if(raw){row.stream=raw->stream_epoch;row.left_tracking=raw->tracking_epoch[0];row.right_tracking=raw->tracking_epoch[1];}
    if(publisher){row.publisher_pid=publisher->pid;row.publisher_start=publisher->start;}
}
void report_main_hold_trace(FILE* file) noexcept {
    if(!file)return;
    for(unsigned i=0;i<main_hold_trace_count;++i){auto const& r=main_hold_trace[i];
        fwprintf(file,L"hammerburst_main_hold phase=%u gate=%u pin_known=%u parent_frame=%llu parent_sequence=%llu parent_sync_ns=%lld pin_generation=%llu observed_ns=%lld begin_ns=%lld reader_failure=%u lifetime_failure=%u game_flags=%u publisher_pid=%llu publisher_start=%llu stream=%llu left_tracking=%llu right_tracking=%llu\n",
            r.phase,r.gate,unsigned(r.pin_known),r.frame,r.sequence,r.source_sync,r.generation,r.observed_ns,r.begin_ns,
            r.reader_failure,r.lifetime_failure,r.game_flags,r.publisher_pid,r.publisher_start,r.stream,r.left_tracking,r.right_tracking);
    }
}
bool boltok_hold_trace_for(hand::Frame const& f) noexcept {
    return main_hold_boltok&&hand::same(main_hold_parent.weapon,f.weapon);
}
void committed_main_hold(hand::Frame const& f) noexcept {
    main_hold_pin_known=false;main_hold_tracking.reset();main_hold_lifetime={};main_hold_boltok=false;
    if(!weaponhand::current_gt(nullptr))return;
    const bool lancer=playground::instance(f.weapon.address,L"Weap_AssaultRifle")&&
        playground::named(f.weapon.klass,L"Weap_AssaultRifle");
    const bool hammerburst=playground::instance(f.weapon.address,L"Weap_LocustAssaultRifle")&&
        playground::named(f.weapon.klass,L"Weap_LocustAssaultRifle");
    const bool boltok=playground::instance(f.weapon.address,L"Weap_LocustPistol")&&
        playground::named(f.weapon.klass,L"Weap_LocustPistol");
    main_hold_trace_hb=hammerburst||boltok;
    main_hold_boltok=boltok;
    if((!lancer&&!hammerburst&&!boltok)||
       !vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)||
       !weaponhand::held_binding.known||!hand::same(weaponhand::held_binding.weapon,f.weapon)){
        record_main_hold(1,1,f);return;}
    gears_hand_continuity::TrackedInput tracked{};gears_hand_continuity::Publisher publisher{};
    hand::ExistingActionLifetime lifetime{};
    gears_current_tracking::Failure failure{};
    if(!nativeequipment::read_input_continuity(f,tracked,publisher,nullptr,&failure)){
        record_main_hold(1,2,f,0,0,unsigned(failure));return;}
    const auto bind_observed_ns=weaponhand::clock_ns(nullptr); // Original bind clock, not a renewed source stamp.
    if(!main_hold_tracking.bind(f.stereo,tracked,publisher,bind_observed_ns,
           f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated)){
        record_main_hold(1,3,f,bind_observed_ns,0,unsigned(failure),UINT_MAX,0,&tracked,&publisher);return;}
    if(!weaponhand::client.capture_existing_action_lifetime_on_gt(f.key,lifetime)){
        record_main_hold(1,4,f,bind_observed_ns,0,unsigned(failure),UINT_MAX,0,&tracked,&publisher);return;}
    main_hold_parent=f;main_hold_lifetime=lifetime;main_hold_generation=vrequip::model.hold_generation();
    main_hold_pin_known=main_hold_generation!=0;
    record_main_hold(1,main_hold_pin_known?0u:5u,f,bind_observed_ns,0,unsigned(failure),UINT_MAX,0,&tracked,&publisher);
}
bool retain_main_during_pose_gap(uptr pc) noexcept {
    // ONLY an already committed Lancer, Hammerburst or exact Boltok. Preserve its logical hold, not a
    // missing render pose or action; Runtime still suspends all input.
    unsigned lifetime_failure=UINT_MAX;
    if(!main_hold_pin_known||!weaponhand::current_gt(nullptr)||pc!=main_hold_parent.controller.address||
       !nativeequipment::domain_ready()||!nativeequipment::restoration_quiet()||
       main_hold_generation!=vrequip::model.hold_generation()||
       !vrequip::model.held_for(main_hold_parent.world.address,main_hold_parent.pawn.address,main_hold_parent.weapon.address)||
       !weaponhand::client.existing_action_lifetime_on_gt(main_hold_lifetime,&lifetime_failure)){
        record_main_hold(2,1,main_hold_parent,0,0,UINT_MAX,lifetime_failure);return false;}
    const auto begin=weaponhand::clock_ns(nullptr);weaponhand::HoldSourceObservation game{};
    if(begin<=0||!weaponhand::observe_hold_source(nullptr,main_hold_parent,begin,game)||
       !game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||game.kind!=hand::WeaponKind::firearm){
        const unsigned flags=unsigned(game.known)|(unsigned(game.main_hand_held)<<1)|(unsigned(game.busy)<<2)|(unsigned(game.animation_owns_hands)<<3);
        record_main_hold(2,2,main_hold_parent,begin,begin,UINT_MAX,lifetime_failure,flags);return false;}
    gears_vr::StereoSample stereo{};gears_vr::ControllerInputFrame input{};
    gears_hand_continuity::TrackedInput tracked{};gears_hand_continuity::Publisher publisher{};
    const bool sim=main_hold_parent.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated;
    const auto failure=nativeequipment::input_continuity_reader.read(GetCurrentProcessId(),[](){return weaponhand::clock_ns(nullptr);},
       sim,stereo,input,tracked,publisher);
    if(failure!=gears_current_tracking::Failure::none){
        record_main_hold(2,3,main_hold_parent,0,begin,unsigned(failure),lifetime_failure);return false;}
    const auto end=weaponhand::clock_ns(nullptr);
    if(!(end>=begin&&end-begin<=50000000)){
        record_main_hold(2,4,main_hold_parent,end,begin,unsigned(failure),lifetime_failure,0,&tracked,&publisher);return false;}
    if(!main_hold_tracking.refresh(stereo,tracked,publisher,end,sim)){
        record_main_hold(2,5,main_hold_parent,end,begin,unsigned(failure),lifetime_failure,0,&tracked,&publisher);return false;}
    const bool kept=weaponhand::client.existing_action_lifetime_on_gt(main_hold_lifetime,&lifetime_failure);
    record_main_hold(2,kept?0u:6u,main_hold_parent,end,begin,unsigned(failure),lifetime_failure,0,&tracked,&publisher);
    return kept;
}


gears_weapon_lancer_grasp::PartFit part_fit;
unsigned part_pose_gate{};
unsigned support_entry_kind{};
api::Kind nearby_kind{};hand::Frame nearby_source{};
V nearby_physical_anchor{};bool nearby_anchor_known{};
// Preparation only, while neutral. Original lookup retains its3ms/8192
// bound and fresh source checks; acquired-part/action work does not pay it.
void prepare_neutral_starter(hand::Frame const& f,hand::GripGameState const& game,
    hand::Decision const& decision) noexcept {
    if(game.known&&game.main_hand_held&&game.kind==hand::WeaponKind::firearm&&!game.busy&&!game.animation_owns_hands&&
       !f.left_grip_down&&decision.lease.kind==hand::LeaseKind::none&&
       f.controllers.hands[1].trigger_active==1&&f.controllers.hands[1].trigger<=.35f&&
       nativelancerfollow::bound_key_known&&nativelancerfollow::bound_is_lancer&&
       hand::same(nativelancerfollow::bound_key.weapon,f.weapon)&&
       nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation()&&
       !lancersaw_original::prepared())lancersaw_original::prepare(f);
}
bool publish_main(hand::Frame const& f,hand::GripGameState const& game) noexcept {
    // Main fingers remain held while the left hand reloads/operates parts.
    // This cosmetic slot grants no lease and cannot move controller targets.
    if(!game.known||!game.main_hand_held||game.kind!=hand::WeaponKind::firearm||game.animation_owns_hands||
       !nativelancerfollow::bound_key_known||!nativelancerfollow::bound_is_lancer||
       !hand::same(nativelancerfollow::bound_key.weapon,f.weapon)||
       nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation()||
       !nativeequipment::current(nullptr,f))return false;
    namespace authored=gears_hand_authored_grasp;
    const auto call=lancersaw_visual::function<authored::Publish>(authored::publish_export);
    if(!call)return false;
    const auto q=gears_lancer_main_hand::request(f);return call(&q);
}
bool natural_magazine_current(hand::Frame const& f) noexcept {
    return lancer_magazine_appearance::active_original(f);
}
bool publish_hammerburst_profile(api::Request const& pose) noexcept {
    if(!nativelancerfollow::bound_key_known||!nativelancerfollow::bound_is_hammerburst||
       !hand::same(nativelancerfollow::bound_key.weapon,pose.source.weapon)||
       nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation()||
       !nativeequipment::current(nullptr,pose.source))return false;
    namespace authored=gears_hand_authored_grasp;
    authored::Request q{};
    if(!gears_hammerburst_part_hand::request(pose.kind,pose.source,pose.grip,q))return false;
    q.proximity=pose.proximity;
    const auto call=lancersaw_visual::function<authored::Publish>(authored::publish_export);
    if(!call)return false;
    if(!q.proximity){
        auto fingers=q;fingers.grip={};
        const auto publish=lancersaw_visual::function<authored::Publish>(authored::part_fingers_export);
        if(publish)(void)publish(&fingers);
    }
    return call(&q);
}
bool publish_part_profile(api::Request const& pose) noexcept {
    if(nativelancerfollow::bound_is_hammerburst)return publish_hammerburst_profile(pose);
    // The fallback PH prop has different geometry. Keep its existing visual
    // profile; it cannot silently select a preset authored for the original.
    if(pose.kind==api::Kind::magazine&&!natural_magazine_current(pose.source)){
        const auto call=lancersaw_visual::function<api::Publish>(api::publish_export);
        return call&&call(&pose);
    }
    gears_hand_authored_grasp::Request q{};
    if(!gears_lancer_part_hand::request(pose.kind,pose.source,pose.grip,q))return false;
    q.proximity=pose.proximity;
    // The local15 profile is independent of this frame's dynamic world wrist
    // goal. Native149 still requires its OWN current same-grasp part frame.
    // E/C may not lose their fingers just because the world target expired.
    auto fingers=q;fingers.grip={};
    const auto publish_fingers=lancersaw_visual::function<gears_hand_authored_grasp::Publish>(
        gears_hand_authored_grasp::part_fingers_export);
    if(!q.proximity&&publish_fingers)(void)publish_fingers(&fingers);
    const auto call=lancersaw_visual::function<gears_hand_authored_grasp::Publish>(
        gears_hand_authored_grasp::publish_export);
    return call&&call(&q);
}
bool publish_support(api::Request const& q,double model_scale=0) noexcept {
    if(q.kind!=api::Kind::support||!nativeequipment::current(nullptr,q.source))return false;
    if(nativelancerfollow::bound_is_hammerburst)return publish_hammerburst_profile(q);
    if(nativelancerfollow::bound_is_lancer){
        // Same optional V3 wire and sole C0 writer; only this already
        // authenticated Lancer branch selects its fitted skin. Other guns
        // retain their existing automatic support profile and V2 behavior.
        if(q.proximity){
            namespace socket=gears_lancer_support_socket;
            const auto call=lancersaw_visual::function<socket::Publish>(socket::publish_export);
            if(call){
                socket::Descriptor d{};d.binding=q.source;d.component=nativelancerfollow::bound_key.component;
                d.asset=nativelancerfollow::bound_key.asset;d.hold_generation=nativelancerfollow::bound_key.hold_generation;
                d.geometry=gears_hand_authored_mount::marcus115_signature;
                d.model_scale=model_scale;d.units_per_metre=nativelancerfollow::bound_units;
                if(!nativelancerfollow::support_geometry(q.source,d.in_main_grip)||!socket::structural(d))return false;
                return call(&d);
            }
        }
        gears_hand_part_grasp::Request fitted{};fitted.pose=q;
        fitted.preset=api::Preset::support_surface;
        const auto call=lancersaw_visual::function<gears_hand_part_grasp::Publish>(gears_hand_part_grasp::publish_export);
        return call&&call(&fitted);
    }
    const auto call=lancersaw_visual::function<api::Publish>(api::publish_export);
    return call&&call(&q);
}
bool prefer_early_passive_support(hand::Frame const& f,hand::GripGameState const& game,
    hand::Decision const& decision) noexcept {
    // Scheduling only: retain no pose, source clock, lease or admission.
    // tick must still check this fresh Frame and actual unsnapped geometry.
    if(nearby_kind!=api::Kind::support||!nativelancerfollow::bound_key_known||
       !nativelancerfollow::bound_is_lancer||!hand::same(nativelancerfollow::bound_key.weapon,f.weapon)||
       nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation()||
       !game.known||!game.main_hand_held||game.kind!=hand::WeaponKind::firearm||game.busy||game.animation_owns_hands||
       decision.new_grant||decision.release_required||decision.lease.kind!=hand::LeaseKind::none||
       !f.left_tracked||!f.left_grip_known||f.left_grip_down||
       f.controllers.hands[0].squeeze_active!=1||hand::control_held(f.controllers.hands[0],gears_vr::control_squeeze))return false;
    return lancerreload_inventory::identity(nearby_source)==lancerreload_inventory::identity(f)&&
        hand::same(nearby_source.controller,f.controller)&&hand::same(nearby_source.mesh,f.mesh)&&
        nearby_source.key.game_pid==f.key.game_pid&&
        nearby_source.key.registration_cookie==f.key.registration_cookie&&nearby_source.key.lifecycle==f.key.lifecycle&&
        nearby_source.reference_generation==f.reference_generation&&nearby_source.stereo.session==f.stereo.session&&
        nearby_source.stereo.space_epoch==f.stereo.space_epoch&&nearby_source.controllers.input_epoch==f.controllers.input_epoch&&
        nearby_source.controllers.profile_generation==f.controllers.profile_generation;
}
hand::SupportSocketPreference support_socket;
bool support_socket_preferred(hand::Frame const& f,hand::GripGameState const& game) noexcept {
    if(!nativelancerfollow::bound_is_lancer||!hand::same(nativelancerfollow::bound_key.weapon,f.weapon)||
       lancerreload_runtime::stage!=lancerreload_runtime::Stage::idle||lancerreload_runtime::pending_start.frame||
       lancerreload_runtime::needs_priority()||lancerreload_visual::active||lancerreload_state::lease.acquired||
       lancer_charging::required(f)){
        support_socket.reset();return false;
    }
    return support_socket.observe(f,game,vrequip::model.hold_generation(),weaponhand::clock_ns(nullptr),selected_starter(f));
}
// Preference only; the contact proposal still proves current real C geometry
// and Native alone grants the actual new Grip lease.
bool selected_starter(hand::Frame const& f) noexcept {
    return lancersaw_starter::released_socket_preferred(f)||(nearby_kind==api::Kind::starter&&
        lancerreload_inventory::identity(nearby_source)==lancerreload_inventory::identity(f)&&
        nearby_source.reference_generation==f.reference_generation&&nearby_source.stereo.session==f.stereo.session&&
        nearby_source.stereo.space_epoch==f.stereo.space_epoch&&nearby_source.controllers.input_epoch==f.controllers.input_epoch&&
        nearby_source.controllers.profile_generation==f.controllers.profile_generation);
}
bool prefer_early_part(hand::Frame const& f,hand::Decision const& d) noexcept {
    // Scheduling only; tick still rechecks current input/lease/actual geometry.
    return nativelancerfollow::bound_key_known&&nativelancerfollow::bound_is_lancer&&
        hand::same(nativelancerfollow::bound_key.weapon,f.weapon)&&
        nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation()&&
        (d.lease.kind==hand::LeaseKind::magazine||d.lease.kind==hand::LeaseKind::ammo)&&
        !selected_starter(f)&&!lancersaw_starter::same_hold(f);
}
unsigned early_magazine_gate{};std::uint64_t early_magazine_frame{};
bool publish_magazine_fingers_early(hand::Frame const& f,hand::GripGameState const& game,hand::Decision const& d) noexcept {
    early_magazine_frame=f.key.frame;early_magazine_gate=1;
    // Boolean source witnesses only. No model/equipment action or pose is
    // cached, and every recorded byte is freshly revalidated each guard.
    std::optional<weaponhand::source_validation::Scope> source_reads;
    if(!weaponhand::source_validation::active&&nativelancerfollow::bound_key_known&&nativelancerfollow::bound_is_lancer&&
       hand::same(nativelancerfollow::bound_key.weapon,f.weapon)&&
       nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation())source_reads.emplace(f);

    // Static preset selected from CURRENT original magazine geometry; no
    // item/wrist transform or old Frame is replayed as a current skin request.
    if(!game.known||!game.main_hand_held||game.kind!=hand::WeaponKind::firearm)return false;
    early_magazine_gate=2;
    if(!nativelancerfollow::bound_key_known||!nativelancerfollow::bound_is_lancer||
       !hand::same(nativelancerfollow::bound_key.weapon,f.weapon)||
       nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation())return false;
    early_magazine_gate=3;
    if(!f.left_tracked||!f.left_grip_known||!f.left_grip_down||d.release_required||
       (d.status!=hand::DecisionStatus::granted&&d.status!=hand::DecisionStatus::retained)||
       !hand::lease_valid(d.lease,f.key.lifecycle)||d.lease.id!=f.press_token||
       (d.lease.kind!=hand::LeaseKind::magazine&&d.lease.kind!=hand::LeaseKind::ammo))return false;
    early_magazine_gate=4;
    if(selected_starter(f)||lancersaw_starter::same_hold(f)||
       (lancer_charging::current_geometry(f)&&lancer_charging::required(f)&&
        lancer_charging::pending_identity==lancerreload_inventory::identity(f)&&hand::same(lancer_charging::held,d.lease)))return false;
    early_magazine_gate=5;if(!natural_magazine_current(f))return false;
    early_magazine_gate=6;if(!nativeequipment::current(nullptr,f))return false;
    namespace authored=gears_hand_authored_grasp;
    const auto call=lancersaw_visual::function<authored::Publish>(authored::part_fingers_export);
    authored::Request q{};
    early_magazine_gate=7;if(!call||!gears_lancer_part_hand::request(api::Kind::magazine,f,{},q))return false;
    early_magazine_gate=8;if(!call(&q))return false;
    early_magazine_gate=0;return true;
}
bool stationary_starter(hand::Frame const& f) noexcept {
    if(lancersaw_starter::released_socket_preferred(f))return true;
    if(!selected_starter(f)||!nearby_anchor_known||!f.left_tracked||!f.left_grip_known)return false;
    const auto& a=f.controllers.hands[0];
    if(!gears_vr::controller_pose_usable(a.grip,a.grip_flags,a.grip_active))return false;
    const auto p=a.grip.position,q=nearby_physical_anchor;
    return std::hypot(std::hypot(p.x-q.x,p.y-q.y),p.z-q.z)<=.03;
}
bool same_magazine_capture(hand::Frame const& f,gears_committed_interaction::Frame const& actual) noexcept {
    return magazine_known&&magazine_token==f.press_token&&hand::same(magazine_weapon,f.weapon)&&
        magazine_key.registration_cookie==f.key.registration_cookie&&magazine_key.lifecycle==f.key.lifecycle&&
        magazine_key.game_start==f.key.game_start&&magazine_key.native_started_qpc==f.key.native_started_qpc&&
        magazine_key.world_generation==f.key.world_generation&&magazine_reference==f.reference_generation&&
        magazine_session==f.stereo.session&&magazine_space==f.stereo.space_epoch&&
        magazine_input==f.controllers.input_epoch&&magazine_profile==f.controllers.profile_generation&&
        magazine_hold==actual.right.hold_generation&&magazine_stream==actual.right.producer.stream&&
        magazine_tracking==actual.right.producer.tracking[0];
}
bool hammerburst_magazine_interaction(hand::Frame const& f,gears_committed_interaction::Frame& out) noexcept {
    out={};gears_committed_interaction::Frame candidate{};
    if(!nativelancerfollow::bound_key_known||!nativelancerfollow::bound_is_hammerburst||
       !hand::same(nativelancerfollow::bound_key.weapon,f.weapon)||
       nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation())return false;
    if(!handrig::read_interaction(candidate)||
       !gears_committed_interaction::matches(candidate,f,vrequip::model.hold_generation(),
           weaponhand::clock_ns(nullptr),f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated))return false;
    out=candidate;return true;
}
bool grip_for_wrist(P const& wrist,double units,double model_scale,P& grip) noexcept {
    gears_hand_authored_mount::Mount m{};
    if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
       gears_hand_authored_mount::Side::left,model_scale,units,m))return false;
    grip.orientation=gears_vr::normalized(gears_vr::multiply(wrist.orientation,gears_vr::inverse(m.calibration.grip_to_wrist_rotation)));
    const auto d=gears_vr::transform(gears_vr::rotation_matrix(grip.orientation),m.calibration.grip_to_wrist_metres);
    grip.position={wrist.position.x-d.x*units,wrist.position.y-d.y*units,wrist.position.z-d.z*units};
    return gears_vr::stereo_pose_valid(grip);
}
bool publish_starter_socket(hand::Frame const& f,lancersaw_visual::Observation const& c,
    double model_scale,double units) noexcept {
    namespace socket=gears_lancer_support_socket;
    if(c.angle<gears_lancer_original_handle::maximum_angle-.000001||std::abs(c.pull_raw)>.000001||
       !source_scope_eligible(f)||!nativeequipment::current(nullptr,f))return false;
    const auto call=lancersaw_visual::function<socket::Publish>(socket::starter_publish_export);
    if(!call)return false; // Older Native keeps the existing V4 world publication.
    std::array<float,16> local_matrix{};P gun{},local{},wrist{},grip{};double scale{};
    if(!nativelancerfollow::mount.model_matrix({{},{}},local_matrix)||
       !hammerburst_runtime::matrix(local_matrix,gun,scale)||
       !gears_lancer_owner_grabposes::starter(gears_lancer_original_handle::maximum_angle,0.,local)||
       !gears_lancer_owner_grabposes::world_wrist(gun,scale,local,wrist)||
       !grip_for_wrist(wrist,units,model_scale,grip))return false;
    socket::Descriptor d{};d.binding=f;d.component=nativelancerfollow::bound_key.component;
    d.asset=nativelancerfollow::bound_key.asset;d.hold_generation=nativelancerfollow::bound_key.hold_generation;
    d.geometry=gears_hand_authored_mount::marcus115_signature;d.model_scale=model_scale;d.units_per_metre=units;
    d.in_main_grip=grip;d.in_main_grip.position={grip.position.x/units,grip.position.y/units,grip.position.z/units};
    return socket::structural(d)&&nativeequipment::current(nullptr,f)&&call(&d);
}
bool part_pose(hand::Frame const& f,api::Kind kind,P& grip,
    lancersaw_visual::Observation const* verified_part=nullptr,
    gears_committed_interaction::Frame const* verified_hand=nullptr) noexcept {
    part_pose_gate=1;
    lancersaw_visual::Observation actual_part{};
    // Immediate neutral C call only: reuse its bracketed stack observations,
    // never a cross-tick pose, refreshed timestamp or additional authority.
    if(bool(verified_part)!=bool(verified_hand)||
       (verified_part&&(kind!=api::Kind::starter||!weapon_part_assets::gt()||
        !source_scope_eligible(f)||!lancersaw_visual::same_binding(f))))return false;
    if(kind==api::Kind::starter){
        if(verified_part)actual_part=*verified_part;
        else if(!lancersaw_visual::observe(f,actual_part))return false;
    }
    else{
        vrequip::MeshPoseProbe probe{};
        if(!vrequip::mesh_pose_probe(f.weapon.address,probe)||
           probe.mesh!=lancer_charging::geometry.component.address||probe.asset!=lancer_charging::geometry.asset.address)return false;
        std::memcpy(actual_part.gun.data(),probe.local_to_world,sizeof(probe.local_to_world));
    }
    part_pose_gate=2;P gun{};double scale{};if(!hammerburst_runtime::matrix(actual_part.gun,gun,scale))return false;
    V captured_motion{};
    if(kind==api::Kind::loading){
        if(!lancer_charging::current_geometry(f))return false;
        const auto& g=lancer_charging::geometry.slide;const double distance=lancer_charging::last_observed;
        if(!std::isfinite(distance)||distance<0||distance>g.travel+.001)return false;
        const double units=nativelancerfollow::bound_units;
        captured_motion={g.axis.x*distance*units,g.axis.y*distance*units,g.axis.z*distance*units};
    }
    gears_committed_interaction::Frame hand{};
    part_pose_gate=3;
    if(verified_hand){
        // Recheck the same source/hold/cohort now. Final current-frame and
        // Native publication admissions remain independent below.
        if(!gears_committed_interaction::matches(*verified_hand,f,vrequip::model.hold_generation(),
            weaponhand::clock_ns(nullptr),f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated))return false;
        hand=*verified_hand;
    }else if(!lancerreload_visual::interaction(f,hand))return false;
    part_pose_gate=4;
    // Real owner-captured wrist AND15-joint table. C was captured deployed:
    // its closed local frame is transformed by THIS observed angle/stroke.
    // E follows THIS observed slide; neither pose supplies contact or input.
    namespace captured=gears_lancer_owner_grabposes;
    P local=kind==api::Kind::loading?captured::loading_wrist_in_gun:captured::starter_wrist_closed;
    if(kind==api::Kind::starter){
        if(!captured::starter(actual_part.angle,actual_part.pull_raw,local))return false;
    }else{
        local.position.x+=captured_motion.x;local.position.y+=captured_motion.y;local.position.z+=captured_motion.z;
    }
    P captured_wrist{};
    if(!captured::world_wrist(gun,scale,local,captured_wrist))return false;
    return grip_for_wrist(captured_wrist,hand.right.units_per_metre,hand.model_scale,grip);

}
void report_grasp_attempt(hand::Frame const& f,api::Kind kind,bool published,unsigned gate,unsigned part_gate,
    hand::GripGameState const* game=nullptr,hand::Decision const* decision=nullptr) noexcept {
    static std::uint64_t previous{~std::uint64_t(0)};static unsigned rows{},token_rows{};static std::uint64_t token{UINT64_MAX};
    if(token!=f.press_token){token=f.press_token;token_rows=0;previous=~std::uint64_t(0);}
    const unsigned hb_observation=game&&nativelancerfollow::bound_is_hammerburst?
        1u+(unsigned(game->main_hand_held)<<1)+(unsigned(game->mount_known)<<2)+(unsigned(game->busy)<<3)+
        (unsigned(game->animation_owns_hands)<<4)+(unsigned(f.left_grip_down)<<5):0u;
    const unsigned lancer_observation=game&&nativelancerfollow::bound_is_lancer?
        1u|(unsigned(hand::support_contact(f,*game,false))<<1)|(unsigned(hand::support_contact(f,*game,true))<<2)|
        (unsigned(nearby_kind==api::Kind::support)<<3)|(unsigned(support_entry_kind==unsigned(api::Kind::support))<<4):0u;
    const std::uint64_t state=std::uint64_t(unsigned(kind)*4096+gate*64+part_gate+(hb_observation<<16)+(lancer_observation<<24))|(std::uint64_t(proximity_gate)<<40);
    if(log_file&&rows<16&&token_rows<4&&previous!=state){fwprintf(log_file,
        L"lancer_context_grasp kind=%u published=%u frame=%llu visual_only=1 gate=%u part_gate=%u age_ns=%lld hb_observed=%u known=%u main_held=%u mount=%u busy=%u animation=%u down=%u input_known=%u tracked=%u control=%u decision=%u lease=%u pending=%u token=%llu early_magazine_frame=%llu early_magazine_gate=%u support_predicates=%u proximity_gate=%u\n",
        unsigned(kind),unsigned(published),f.key.frame,gate,part_gate,
        weaponhand::clock_ns(nullptr)-f.controllers.sync_ns,hb_observation,game?unsigned(game->known):0u,
        game?unsigned(game->main_hand_held):0u,game?unsigned(game->mount_known):0u,game?unsigned(game->busy):0u,
        game?unsigned(game->animation_owns_hands):0u,f.left_grip_down,f.left_grip_known,f.left_tracked,f.interaction_control,
        decision?unsigned(decision->status):0u,decision?unsigned(decision->lease.kind):0u,
        game?unsigned(game->magazine_observation_pending):0u,f.press_token,early_magazine_frame,early_magazine_gate,lancer_observation,proximity_gate);fflush(log_file);++rows;++token_rows;}previous=state;
}
bool proximity_tick(hand::Frame const& f,hand::GripGameState const& game,hand::Decision const& decision) noexcept {
    proximity_gate=1;support_entry_kind=unsigned(nearby_kind);
    if(nativelancerfollow::bound_is_longshot){
        const bool retained=nearby_kind==api::Kind::support&&
            lancerreload_inventory::identity(nearby_source)==lancerreload_inventory::identity(f)&&
            nearby_source.reference_generation==f.reference_generation&&nearby_source.stereo.session==f.stereo.session&&
            nearby_source.stereo.space_epoch==f.stereo.space_epoch&&nearby_source.controllers.input_epoch==f.controllers.input_epoch&&
            nearby_source.controllers.profile_generation==f.controllers.profile_generation;
        using Mode=gears_weapon_support_socket::Mode;
        const auto mode=gears_weapon_support_socket::mode(f,game,decision,weaponhand::clock_ns(nullptr),retained);
        if(mode==Mode::grab){nearby_kind=api::Kind::support;nearby_source=f;return false;}
        nearby_kind={};
        if(mode!=Mode::snap)return false;
        const auto& plan=nativelancerfollow::plan;P geometry{};
        if(!plan.valid||!hand::same_frame(plan.source,f)||
           !nativelancerfollow::support_geometry(f,geometry)||!nativeequipment::current(nullptr,f))return false;
        const auto& right=plan.right_grip;
        const auto offset=gears_vr::transform(gears_vr::rotation_matrix(right.orientation),geometry.position);
        const double units=plan.prepared.units_per_metre;
        if(!std::isfinite(units)||units<10||units>500)return false;
        api::Request q{};q.source=f;q.kind=api::Kind::support;q.proximity=1;
        q.grip={{right.position.x+offset.x*units,right.position.y+offset.y*units,right.position.z+offset.z*units},
            gears_vr::normalized(gears_vr::multiply(right.orientation,geometry.orientation))};
        const auto call=lancersaw_visual::function<api::Publish>(api::publish_export);
        if(!call||!nativeequipment::current(nullptr,f))return false;
        const bool published=call(&q);
        if(published){nearby_kind=api::Kind::support;nearby_source=f;}
        return published;
    }
    if(nativelancerfollow::bound_is_hammerburst){
        nearby_kind={};
        using Mode=gears_weapon_hammerburst_support::Mode;
        if(gears_weapon_hammerburst_support::mode(f,game,decision,weaponhand::clock_ns(nullptr))!=Mode::snap)return false;
        const auto& plan=nativelancerfollow::plan;
        P geometry{};
        if(!plan.valid||!hand::same_frame(plan.source,f)||
           !nativelancerfollow::support_geometry(f,geometry)||!nativeequipment::current(nullptr,f))return false;
        const auto& right=plan.right_grip;
        const auto offset=gears_vr::transform(gears_vr::rotation_matrix(right.orientation),geometry.position);
        const double units=plan.prepared.units_per_metre;
        if(!std::isfinite(units)||units<10||units>500)return false;
        api::Request q{};q.source=f;q.kind=api::Kind::support;q.proximity=1;
        q.grip={{right.position.x+offset.x*units,right.position.y+offset.y*units,right.position.z+offset.z*units},
            gears_vr::normalized(gears_vr::multiply(right.orientation,geometry.orientation))};
        if(!nativeequipment::current(nullptr,f))return false;
        const bool published=publish_hammerburst_profile(q);
        if(published){nearby_kind=api::Kind::support;nearby_source=f;}
        return published;
    }
    // An unavailable observation withdraws THIS skin publication, not the
    // stationary cosmetic hover. No pose, grant or motor permission is kept.
    // Identity/epochs/tracking and <=3cm physical displacement are checked by
    // stationary_starter; known outside contact below explicitly clears it.
    proximity_gate=10;const bool stationary_c=stationary_starter(f);
    lancersaw_starter::cosmetic_nearby=stationary_c;
    if(stationary_c)lancersaw_starter::cosmetic_identity=lancerreload_inventory::identity(f);
    const bool neutral=f.controllers.hands[0].squeeze_active==1&&
        !hand::control_held(f.controllers.hands[0],gears_vr::control_squeeze);
    // Auto-fitting selected C before the real Grip edge. Preserve only that
    // cosmetic selection/deployment during its handoff, using NEW actual
    // geometry. Do not publish a passive pose or supply an action lease here.
    if(!neutral&&selected_starter(f)&&f.left_tracked&&f.left_grip_known&&
       hand::control_held(f.controllers.hands[0],gears_vr::control_squeeze)&&
       (decision.lease.kind==hand::LeaseKind::none||lancersaw_starter::same_hold(f))&&
       lancersaw_starter::idle_reload(f)&&nativeequipment::current(nullptr,f)){
        gears_committed_interaction::Frame actual{};lancersaw_visual::Observation c{};
        if(lancerreload_visual::interaction(f,actual)&&lancersaw_visual::observe(f,c)){
            const auto p=actual.physical_left_grip.position;
            const double distance=std::hypot(std::hypot(p.x-c.grasp.x,p.y-c.grasp.y),p.z-c.grasp.z)/actual.right.units_per_metre;
            if(gears_lancer_workbench_profile::starter()?lancersaw_starter::configured_contact(f,c,actual):(std::isfinite(distance)&&distance<=.22)){
                lancersaw_starter::cosmetic_nearby=true;lancersaw_starter::cosmetic_identity=lancerreload_inventory::identity(f);
                nearby_source=f;nearby_physical_anchor=f.controllers.hands[0].grip.position;nearby_anchor_known=true;
                return false;
            }
        }
    }
    // The released Frame can still carry the old part lease even after the
    // game's busy bit clears. Keep the stationary cosmetic C selection through
    // BOTH transitions; neither one supplies a new skin goal or action lease.
    if(neutral&&stationary_starter(f)&&(game.busy||game.animation_owns_hands||
       decision.lease.kind!=hand::LeaseKind::none)&&
       (lancersaw_original::active()||lancersaw_starter::locked_rail)&&
       lancersaw_starter::idle_reload(f)&&nativeequipment::current(nullptr,f)){
        gears_committed_interaction::Frame actual{};lancersaw_visual::Observation c{};
        if(lancerreload_visual::interaction(f,actual)&&lancersaw_visual::observe(f,c)){
            const auto p=actual.physical_left_grip.position;
            const double distance=std::hypot(std::hypot(p.x-c.grasp.x,p.y-c.grasp.y),p.z-c.grasp.z)/actual.right.units_per_metre;
            if(gears_lancer_workbench_profile::starter()?lancersaw_starter::configured_contact(f,c,actual):(std::isfinite(distance)&&distance<=.22)){
                lancersaw_starter::cosmetic_nearby=true;lancersaw_starter::cosmetic_identity=lancerreload_inventory::identity(f);
                nearby_source=f;return false;
            }
        }
    }
    proximity_gate=20;
    if(!f.left_tracked||!f.left_grip_known||
       (nearby_kind==api::Kind::starter&&!selected_starter(f))){
        nearby_kind={};return false;
    }
    // A spent immutable Frame withdraws publication, not this cosmetic
    // acquisition preference. A later new Grip still needs fresh actual C
    // contact and the unchanged Native arbiter; old geometry grants nothing.
    proximity_gate=21;if(!nativeequipment::current(nullptr,f))return false;
    const bool reload_idle=lancersaw_starter::idle_reload(f);
    proximity_gate=22;if(!neutral||decision.lease.kind!=hand::LeaseKind::none||!reload_idle){
        // Grip changes control ownership, not the existing hover selection.
        // Retaining this enum publishes nothing and grants no support lease.
        // A later neutral frame still rechecks the original identity/epochs
        // and current unsnapped distance before its cosmetic publication.
        nearby_kind=api::proximity_control_handoff(nearby_kind,decision.lease.kind,reload_idle);
        return false;
    }
    // Reuse the game observation from this exact authenticated frame copy.
    // Reopening the full observer here adds no new authority or geometry.
    proximity_gate=23;if(!game.known||!game.main_hand_held||game.kind!=hand::WeaponKind::firearm||game.busy||game.animation_owns_hands){
        nearby_kind={};return false;
    }
    proximity_gate=24;gears_committed_interaction::Frame actual{};P geometry{},right{};
    if(!lancerreload_visual::interaction(f,actual)||!nativelancerfollow::support_geometry(f,geometry)||
       !gears_weapon_lancer_wrist_mount::grip_from_palm_endpoint(actual.right.right_grip,right))return false;
    proximity_gate=25;const double units=actual.right.units_per_metre;
    const auto offset=gears_vr::transform(gears_vr::rotation_matrix(right.orientation),geometry.position);
    const P support{{right.position.x+offset.x*units,right.position.y+offset.y*units,right.position.z+offset.z*units},
        gears_vr::normalized(gears_vr::multiply(right.orientation,geometry.orientation))};
    const auto distance=[&](V p){const auto q=actual.physical_left_grip.position;
        return std::hypot(std::hypot(p.x-q.x,p.y-q.y),p.z-q.z)/units;};
    const bool same=nearby_kind!=api::Kind::none&&
        lancerreload_inventory::identity(nearby_source)==lancerreload_inventory::identity(f)&&
        nearby_source.reference_generation==f.reference_generation&&nearby_source.stereo.session==f.stereo.session&&
        nearby_source.stereo.space_epoch==f.stereo.space_epoch&&nearby_source.controllers.input_epoch==f.controllers.input_epoch&&
        nearby_source.controllers.profile_generation==f.controllers.profile_generation;
    const double support_distance=distance(support.position);
    double support_rank=support_distance;
    if(const auto region=gears_lancer_workbench_profile::support()){
        std::array<float,16> neutral_mesh{};
        const auto local=gears_weapon::rotate(gears_vr::inverse(right.orientation),
            {actual.physical_left_grip.position.x-right.position.x,
             actual.physical_left_grip.position.y-right.position.y,
             actual.physical_left_grip.position.z-right.position.z});
        if(!nativelancerfollow::mount.model_matrix({{},{}},neutral_mesh)||
           !gears_lancer_socket_rank::distance(*region,neutral_mesh,local,units,support_rank))return false;
    }
    const auto incumbent=lancersaw_starter::released_socket_preferred(f)?api::Kind::starter:
        same?nearby_kind:api::Kind::none;
    // A retained support within3cm of its ranking centre cannot lose to C:
    // switching requires C rank+3cm < support rank. Avoid reopening an unrelated original
    // C owner here, while retaining this frame's actual support geometry and
    // every publication/Native freshness gate. All other selection is unchanged.
    const bool support_cannot_switch=incumbent==api::Kind::support&&
        std::isfinite(support_rank)&&support_rank>=0&&support_rank<=.03;
    lancersaw_visual::Observation c{};
    const bool c_known=!support_cannot_switch&&lancersaw_visual::observe(f,c);
    if(!c_known&&incumbent==api::Kind::starter){
        // Unknown evaluated C geometry cannot prove that the MOVING hand left
        // its selected socket. The old1e9 sentinel withdrew C during an ordinary
        // approach once it exceeded3cm from the first hover point. Retain ONLY
        // cosmetic selection/deployment on this current same-identity frame.
        // Publish no skin pose/contact/grant; a known distance below still
        // performs ordinary exit/switch selection with the unchanged volumes.
        lancersaw_starter::cosmetic_nearby=true;
        lancersaw_starter::cosmetic_identity=lancerreload_inventory::identity(f);
        return false;
    }
    const double c_distance=c_known?distance(c.grasp):1e9;
    double c_rank=c_distance;
    if(c_known)if(const auto region=gears_lancer_workbench_profile::starter())
        if(!gears_lancer_socket_rank::distance(*region,c.gun,actual.physical_left_grip.position,
            units,c_rank,&c.part_delta))return false;
    const bool c_region=gears_lancer_workbench_profile::starter()!=nullptr;
    const bool c_contact=c_known&&c_region&&lancersaw_starter::configured_contact(f,c,actual);
    const auto selected=(game.support_region.enabled||c_region)?
        api::proximity_socket(incumbent,
            api::ProximityContact{support_rank,game.support_region.enabled?hand::support_contact(f,game,false):support_distance<=gears_weapon::lancer_contact::support_radius,
                game.support_region.enabled?hand::support_contact(f,game,true):support_distance<=.22},
            api::ProximityContact{c_rank,c_region?c_contact:c_distance<=gears_weapon::lancer_contact::starter_radius,c_region?c_contact:c_distance<=.22},
            stationary_starter(f)):
        api::proximity_socket(incumbent,support_distance,c_distance,
            gears_weapon::lancer_contact::support_radius,gears_weapon::lancer_contact::starter_radius,stationary_starter(f));
    proximity_gate=27;if(selected==api::Kind::none){lancersaw_starter::cosmetic_nearby=false;nearby_kind={};part_fit.reset();return false;}
    if(selected==api::Kind::starter&&(!same||nearby_kind!=api::Kind::starter))
        lancersaw_starter::remember_neutral_contact(f,c,actual);
    if(!same||selected!=nearby_kind)part_fit.reset();
    P goal=support;
    if(selected==api::Kind::starter){
        // Actual current C geometry and unsnapped physical proximity already
        // selected this socket. Keep cosmetic deployment through its pending
        // Compose acknowledgement; do NOT publish a wrist/finger pose until
        // part_pose verifies evaluated ready geometry below.
        lancersaw_starter::cosmetic_nearby=true;
        lancersaw_starter::cosmetic_identity=lancerreload_inventory::identity(f);
        // Acquisition was already proved above from current unsnapped contact.
        // Remember cosmetic selection BEFORE waiting for deployed Compose geometry.
        // Otherwise a failed first pose publication forgets C and folds it again
        // next tick, preventing the geometry needed for that very publication.
        // This enum/raw anchor supplies no skin pose, Grip edge or action lease.
        // An approach can acquire C at the edge of its volume. Once the
        // current unsnapped hand actually reaches either the folded contact
        // centre or the visible crossbar, recenter the stationary-hover
        // anchor there. Do not trail a slowly moving hand across the whole
        // retention volume, which would trap it instead of allowing support.
        const bool at_contact_centre=api::recenter_starter_anchor(distance(c.closed_grasp),distance(c.grasp));
        if(!same||nearby_kind!=api::Kind::starter||!nearby_anchor_known||at_contact_centre){
            nearby_physical_anchor=f.controllers.hands[0].grip.position;nearby_anchor_known=true;
        }
        nearby_kind=selected;nearby_source=f;
        if(!part_pose(f,selected,goal,&c,&actual))return false;
    }
    proximity_gate=28;const auto call=lancersaw_visual::function<api::Publish>(api::publish_export);
    if(!call||!nativeequipment::current(nullptr,f))return false;
    proximity_gate=29;api::Request q{};q.source=f;q.kind=selected;q.grip=goal;q.proximity=1;
    const bool published=selected==api::Kind::support?publish_support(q,actual.model_scale):
        (publish_starter_socket(f,c,actual.model_scale,units)||publish_part_profile(q));
    if(published){
        if(selected==api::Kind::starter&&(!same||nearby_kind!=api::Kind::starter||!nearby_anchor_known)){
            nearby_physical_anchor=f.controllers.hands[0].grip.position;nearby_anchor_known=true;
        }else if(selected!=api::Kind::starter)nearby_anchor_known=false;
        nearby_kind=selected;nearby_source=f;
        lancersaw_starter::cosmetic_nearby=selected==api::Kind::starter;
        lancersaw_starter::cosmetic_identity=lancerreload_inventory::identity(f);}
    proximity_gate=published?30u:31u;
    report_grasp_attempt(f,selected,published,published?0u:34u,selected==api::Kind::starter?part_pose_gate:0,&game,&decision);
    return published;
}
void tick(hand::Frame const& f,hand::GripGameState const& game,hand::Decision const& decision) noexcept try {
    // Boolean source witnesses only. No model/equipment action or pose is
    // cached, and every recorded byte is freshly revalidated each guard.
    std::optional<weaponhand::source_validation::Scope> source_reads;
    if(!weaponhand::source_validation::active&&nativelancerfollow::bound_key_known&&nativelancerfollow::bound_is_lancer&&
       hand::same(nativelancerfollow::bound_key.weapon,f.weapon)&&
       nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation())source_reads.emplace(f);

    (void)publish_main(f,game);
    struct Attempt {
        hand::Frame const& frame;hand::GripGameState const& game;hand::Decision const& decision;api::Kind kind{};bool published{};unsigned gate{1},part_gate{};
        ~Attempt(){report_grasp_attempt(frame,kind,published,gate,part_gate,&game,&decision);}
    } attempt{f,game,decision};
    if(proximity_tick(f,game,decision)){
        attempt.kind=nearby_kind;attempt.published=true;attempt.gate=0;
        attempt.part_gate=nearby_kind==api::Kind::starter?part_pose_gate:0;
        magazine_known=false;return;
    }
    if(!f.left_tracked||!f.left_grip_known||!f.left_grip_down||decision.release_required||
       (decision.status!=hand::DecisionStatus::granted&&decision.status!=hand::DecisionStatus::retained)||
       !hand::lease_valid(decision.lease,f.key.lifecycle)||decision.lease.id!=f.press_token||
       (decision.lease.kind!=hand::LeaseKind::magazine&&decision.lease.kind!=hand::LeaseKind::ammo&&
        decision.lease.kind!=hand::LeaseKind::support)){
        magazine_known=false;part_fit.reset();return;
    }
    if(decision.lease.kind==hand::LeaseKind::support){
        attempt.kind=api::Kind::support;attempt.gate=2;
        magazine_known=false;part_fit.reset();P geometry{};
        if(!nativelancerfollow::support_geometry(f,geometry))return;
        attempt.gate=3;if(!nativeequipment::current(nullptr,f))return;
        api::Request q{};q.source=f;q.kind=api::Kind::support;q.grip=geometry;
        attempt.published=publish_support(q);attempt.gate=attempt.published?0u:33u;return;
    }
    api::Kind kind{};
    if(lancersaw_starter::same_hold(f))kind=api::Kind::starter;
    else if(lancer_charging::current_geometry(f)&&lancer_charging::required(f)&&
        lancer_charging::pending_identity==lancerreload_inventory::identity(f)&&
        hand::same(lancer_charging::held,decision.lease))kind=api::Kind::loading;
    else kind=api::Kind::magazine;
    attempt.kind=kind;attempt.gate=10;
    P goal{};
    if(kind==api::Kind::magazine){
        part_fit.reset();
        gears_committed_interaction::Frame actual{};
        const bool hb=playground::instance(f.weapon.address,L"Weap_LocustAssaultRifle");
        if(hb){if(!hammerburst_magazine_interaction(f,actual))return;}
        else if(!lancerreload_visual::interaction(f,actual))return;
        const auto& physical=actual.physical_left_grip;
        // Re-evaluate the actual item's Grip Point on every fresh frame.
        // Insertion assistance and the seated extraction stop can move the
        // visible magazine relative to the raw controller. A cached offset
        // on the controller would leave its hand behind during that motion.
        const auto component=playground::pointer(f.weapon.address,L"MagazineMesh");
        std::array<float,16> matrix{},after{};P magazine{};double size{};gears_vr::Viewpoint wrist{};Q wrist_q{};
        if(!playground::instance(component,L"StaticMeshComponent")||
           !copy(matrix.data(),component+0xa0,sizeof matrix)||!hammerburst_runtime::matrix(matrix,magazine,size))return;
        if(hb){
            if(!lancerreload_visual::active||!hand::same(lancerreload_visual::saved.weapon,f.weapon)||
               !lancerreload_visual::alive()||lancerreload_visual::saved.component.address!=component||
               !hammerburst_runtime::old_or_new_magazine_stage())return;
            P authored{};
            if(!gears_weapon_hammerburst_owner_magazine::authored_wrist_world_from_static(magazine,size,authored)||
               !copy(after.data(),component+0xa0,sizeof after)||matrix!=after||
               playground::pointer(f.weapon.address,L"MagazineMesh")!=component||
               !gears_weapon_hammerburst_owner_magazine::grip_from_wrist(authored,actual.model_scale,
                   actual.right.units_per_metre,goal)||!nativeequipment::current(nullptr,f))return;
        }else{
            if(!(natural_magazine_current(f)?gears_lancer_natural_magazine::grip.wrist_for_item(
                    {magazine.position,gears_vr::ue_angles(gears_vr::rotation_matrix(magazine.orientation))},wrist,actual.model_scale):
                  gears_weapon::lancer_left_for_magazine(
                    {magazine.position,gears_vr::ue_angles(gears_vr::rotation_matrix(magazine.orientation))},wrist,actual.model_scale))||
               !copy(after.data(),component+0xa0,sizeof after)||matrix!=after||
               playground::pointer(f.weapon.address,L"MagazineMesh")!=component||
               !gears_vr::rig_tracking::detail::quaternion(gears_vr::ue_matrix(wrist.rotation),wrist_q))return;
            if(!grip_for_wrist({wrist.position,wrist_q},actual.right.units_per_metre,actual.model_scale,goal))return;
        }
        if(!same_magazine_capture(f,actual)){
            magazine_known=false;
            // A newly granted ammo fetch can precede creation/attachment of
            // the replacement magazine. Do not permanently cache the distant
            // old component's pose. Retry only on a later fresh actual pair.
            gears_hand_authored_mount::Mount model{};P bounded_visual{};
            if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
               gears_hand_authored_mount::Side::left,actual.model_scale,actual.right.units_per_metre,model)||
               !api::apply({kind,goal,1},physical,model.calibration,actual.right.units_per_metre,bounded_visual))return;
            magazine_token=f.press_token;magazine_weapon=f.weapon;magazine_known=true;
            magazine_key=f.key;magazine_reference=f.reference_generation;magazine_session=f.stereo.session;
            magazine_space=f.stereo.space_epoch;magazine_input=f.controllers.input_epoch;
            magazine_profile=f.controllers.profile_generation;magazine_hold=actual.right.hold_generation;
            magazine_stream=actual.right.producer.stream;magazine_tracking=actual.right.producer.tracking[0];
        }
    }else if(!part_pose(f,kind,goal)){attempt.gate=20;attempt.part_gate=part_pose_gate;return;}
    const auto call=lancersaw_visual::function<api::Publish>(api::publish_export);
    attempt.gate=3;if(!call||!nativeequipment::current(nullptr,f))return;
    api::Request q{};q.source=f;q.kind=kind;q.grip=goal;const bool published=publish_part_profile(q);
    attempt.published=published;attempt.gate=published?0u:33u;
}catch(...){}
}
