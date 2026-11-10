#include "../vr-common/weapon_hammerburst_supply_publication.h"
// One GT consumer, sharing the existing native reload lease, inventory writer,
// component restoration and stop/drain boundary with Lancer. No second runner.
namespace hammerburst_runtime {
namespace hand=gears_weapon_hand;
namespace inv=lancerreload_inventory;
namespace anim=hammerburst_animation;
namespace visual=lancerreload_visual;
std::atomic<std::uint64_t> configured_policy{gears_weapon::LancerReloadPolicy{}.packed()};
bool manual_enabled() noexcept {return weapon_part_inputs::original_source_enabled.load(std::memory_order_acquire);}
gears_weapon::LancerReloadPolicy opening_policy;
bool ui_pending{};
gears_weapon::ReloadSupplyRejection empty_supply;
using V=gears_vr::Vector;using Q=gears_vr::Quaternion;using M=gears_vr::Matrix;using P=gears_vr::Pose;
enum class Stage {idle,acquiring,opening,opened,old_magazine,waiting_ammo,new_magazine,closing,completing,recovering,parking,parked,restoring};
Stage stage{};
#include "weapon_hammerburst_reload_park_ledger.inl"
park_ledger::Plan parked_plan;
lancerreload_state::Lease parked_native_lease;
namespace restoration_admission {void cancel() noexcept;}
struct Geometry {
    nativelancerfollow::Key key{};
    inv::Identity identity{};
    gears_weapon::HammerburstLatchGeometry latch{};
    Q closed_orientation{};
    bool known{};
};
Geometry geometry;
inv::ObservedInventory observer;
inv::Transaction transaction;
inv::InsertionWriter writer;
inv::Inventory opening;
inv::Request completion;
lancerreload_state::RewardReceipt reward;
#include "weapon_hammerburst_reload_begin.inl"
#include "weapon_hammerburst_reload_cleanup.inl"
gears_weapon::HammerburstLatchStroke stroke;
gears_weapon::FeedInsertionStroke insertion;
gears_weapon::HammerburstMagazineExtraction extraction;
gears_weapon::FeedPortMount feed;
hand::Lease held{};
gears_hammerburst_supply_publication::Snapshot supply_publication,latch_publication;
std::uint64_t serial{},old_item{},new_item{};
inv::Identity last_started_identity{};
std::uint64_t last_started_grip{};
std::uint64_t feed_item{},feed_sequence{};
std::int64_t feed_ns{};
unsigned feed_logs{};V last_feed_position{};bool last_feed_position_known{};
unsigned latch_rejection_logs{};
unsigned latch_stationary_holds{};
std::int64_t last_latch_seek_elapsed_ns{};
double angle{};
bool released{},switch_requested{},native_exited{};
bool restore_display_ready{};
unsigned restore_display_step{};
bool restoration_failed{};
unsigned restore_resume_step{};
std::uint64_t restore_resume_intent{};
bool resume_grant_ready{};
bool restore_native_uncertain{};
unsigned open_pose_reads{},open_stable_reads{},feed_failure{};
double last_open_observed{};
std::int64_t geometry_retry_ns{};
unsigned geometry_failure{},last_geometry_failure{~0u};
#include "weapon_hammerburst_restore_log.inl"

V add(V a,V b) noexcept {return {a.x+b.x,a.y+b.y,a.z+b.z};}
V sub(V a,V b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
V scale(V a,double s) noexcept {return {a.x*s,a.y*s,a.z*s};}
double length(V a) noexcept {return std::hypot(std::hypot(a.x,a.y),a.z);}
bool matrix(std::array<float,16> const& raw,P& pose,double& uniform) noexcept {
    for(auto v:raw)if(!std::isfinite(v))return false;
    if(std::abs(raw[3])>1e-4||std::abs(raw[7])>1e-4||std::abs(raw[11])>1e-4||std::abs(raw[15]-1)>1e-4)return false;
    M basis{};double sizes[3]{};
    for(unsigned i=0;i<3;++i){
        sizes[i]=std::hypot(std::hypot(double(raw[i*4]),double(raw[i*4+1])),double(raw[i*4+2]));
        if(sizes[i]<.1||sizes[i]>10)return false;
        for(unsigned j=0;j<3;++j)basis[j][i]=raw[i*4+j]/sizes[i];
    }
    uniform=(sizes[0]+sizes[1]+sizes[2])/3;
    if(std::abs(sizes[0]-uniform)>1e-3||std::abs(sizes[1]-uniform)>1e-3||std::abs(sizes[2]-uniform)>1e-3)return false;
    for(unsigned i=0;i<3;++i)for(unsigned j=i;j<3;++j){
        double dot=0;for(unsigned k=0;k<3;++k)dot+=basis[k][i]*basis[k][j];
        if(std::abs(dot-(i==j?1.:0.))>1e-3)return false;
    }
    // Reflection is not a rotation. The quaternion helper alone does not
    // authenticate handedness of a malformed component/bone matrix.
    const V x{basis[0][0],basis[1][0],basis[2][0]},y{basis[0][1],basis[1][1],basis[2][1]},z{basis[0][2],basis[1][2],basis[2][2]};
    const V cross{x.y*y.z-x.z*y.y,x.z*y.x-x.x*y.z,x.x*y.y-x.y*y.x};
    if(cross.x*z.x+cross.y*z.y+cross.z*z.z<.999)return false;
    pose.position={raw[12],raw[13],raw[14]};
    return gears_vr::rig_tracking::detail::quaternion(basis,pose.orientation)&&gears_vr::stereo_pose_valid(pose);
}
bool eligible(hand::Frame const& f) noexcept {
    return playground::instance(f.weapon.address,L"Weap_LocustAssaultRifle")&&
        nativeequipment::current(nullptr,f)&&nativelancerfollow::bound_key_known&&
        hand::same(f.weapon,nativelancerfollow::bound_key.weapon)&&nativelancerfollow::mount.bound();
}
bool geometry_current(hand::Frame const& f) noexcept {
    return geometry.known&&geometry.identity==inv::identity(f)&&
        geometry.key==nativelancerfollow::bound_key&&
        nativelancerfollow::bound_session==f.stereo.session&&nativelancerfollow::bound_space==f.stereo.space_epoch&&
        nativelancerfollow::bound_input==f.controllers.input_epoch&&
        nativelancerfollow::bound_reference==f.reference_generation&&nativelancerfollow::bound_world==f.key.world_generation;
}
bool fresh_restore_main(hand::Frame const& f) noexcept {
    const auto now=weaponhand::clock_ns(nullptr);
    return now>=f.now_ns&&now-f.now_ns<=100000000&&f.controllers.sync_ns>0&&
        now>=f.controllers.sync_ns&&now-f.controllers.sync_ns<=100000000&&
        gears_vr::controller_pose_usable(f.controllers.hands[1].grip,f.controllers.hands[1].grip_flags,f.controllers.hands[1].grip_active)&&
        hand::frame_active(f,now)&&nativeequipment::current(nullptr,f);
}
// All relative measurements use the same two raw tracked grips and the same
// fixed gun mount. They do not depend on a later render/Scene publication.
bool local_hand(hand::Frame const& f,P& left) noexcept {
    if(!geometry_current(f)||!f.left_tracked||!f.controllers.sequence||f.controllers.sync_ns<=0)return false;
    const double units=nativelancerfollow::bound_units;
    if(!std::isfinite(units)||units<10||units>500)return false;
    for(const auto& h:f.controllers.hands)if(!gears_vr::controller_pose_usable(h.grip,h.grip_flags,h.grip_active))return false;
    const auto convert=[&](P const& p){
        const auto basis=gears_vr::xr_to_ue(gears_vr::rotation_matrix(p.orientation));
        return gears_vr::Viewpoint{scale(gears_vr::xr_to_ue(p.position),units),gears_vr::ue_angles(basis)};
    };
    const auto l=convert(f.controllers.hands[0].grip),r=convert(f.controllers.hands[1].grip);
    std::array<float,16> raw{};P gun{};double size{};
    if(!nativelancerfollow::mount.model_matrix(r,raw)||!matrix(raw,gun,size))return false;
    const auto inverse=gears_vr::inverse(gun.orientation);
    left.position=scale(gears_weapon::rotate(inverse,sub(l.position,gun.position)),1/(size*units));
    Q lq{};if(!gears_vr::rig_tracking::detail::quaternion(gears_vr::ue_matrix(l.rotation),lq))return false;
    left.orientation=gears_vr::normalized(gears_vr::multiply(inverse,lq));
    return gears_vr::stereo_pose_valid(left);
}
bool parts_local(hand::Frame const& f,std::array<P,3>& poses) noexcept {
    anim::Parts parts{};vrequip::MeshPoseProbe mesh{};P gun{};double size{};
    if(!anim::read_parts(f,parts)||!vrequip::mesh_pose_probe(f.weapon.address,mesh)||
       mesh.mesh!=parts.component.address||mesh.asset!=parts.asset.address)return false;
    std::array<float,16> raw{};std::memcpy(raw.data(),mesh.local_to_world,sizeof(mesh.local_to_world));
    if(!matrix(raw,gun,size))return false;
    const auto inverse=gears_vr::inverse(gun.orientation);const double units=nativelancerfollow::bound_units;
    if(!std::isfinite(units)||units<10||units>500)return false;
    for(unsigned i=0;i<3;++i){P world{};double bone_size{};
        if(!matrix(parts.world[i],world,bone_size)||std::abs(bone_size-size)>1e-3)return false;
        poses[i]={scale(gears_weapon::rotate(inverse,sub(world.position,gun.position)),1/(size*units)),
            gears_vr::normalized(gears_vr::multiply(inverse,world.orientation))};
    }
    return nativeequipment::current(nullptr,f);
}
bool discover(hand::Frame const& f) noexcept {
    geometry_failure=1;
    if(!eligible(f))return false;
    // The original sequence node need not exist until PlayWeaponAnimation.
    // Authenticate the closed parent-relative bone pose itself, rather than
    // making a possibly lazy idle animation node a prerequisite to grabbing.
    geometry_failure=2;bool reloading{};
    if(!lancerreload_state::bool_call(f.weapon.address,L"IsReloading",reloading)||reloading)return false;
    geometry_failure=4;
    std::array<P,3> bones{};if(!parts_local(f,bones))return false;
    const double units=nativelancerfollow::bound_units;
    const auto latch_in_root=scale(gears_weapon::rotate(gears_vr::inverse(bones[0].orientation),sub(bones[1].position,bones[0].position)),units);
    const auto mag_in_latch=scale(gears_weapon::rotate(gears_vr::inverse(bones[1].orientation),sub(bones[2].position,bones[1].position)),units);
    const auto latch_rotation=gears_vr::multiply(gears_vr::inverse(bones[0].orientation),bones[1].orientation);
    const auto mag_rotation=gears_vr::multiply(gears_vr::inverse(bones[1].orientation),bones[2].orientation);
    geometry_failure=5;
    if(length(sub(latch_in_root,{31.3649845123,0,2.1253657341}))>.05||
       length(sub(mag_in_latch,{-5.5591144562,0,-1.2809873819}))>.05||
       std::abs(latch_rotation.w)<.99999||std::abs(mag_rotation.w)<.99999)return false;
    Geometry g{};g.key=nativelancerfollow::bound_key;g.identity=inv::identity(f);
    g.latch.pivot=bones[1].position;
    g.closed_orientation=bones[1].orientation;
    // Original stored localY quaternion rotates the live UE bone with the
    // opposite sign. Confirmed by the first actual stopped-node bone sample.
    g.latch.axis=gears_weapon::rotate(bones[1].orientation,{0,-1,0});
    // Nearest actual original latch surface to the authored top grasp, plus
    // 1 mm outward. Prior candidate was ~3 cm off that surface. This refines
    // geometric placement, not physical palm calibration or comfort proof.
    g.latch.closed_contact=add(g.latch.pivot,gears_weapon::rotate(bones[1].orientation,
        {-7.09357487/units,2.10061312/units,1.88769577/units}));
    g.latch.open_angle=0;g.latch.current_instance_verified=true;g.known=true;
    if(!nativeequipment::current(nullptr,f))return false;
    geometry=g;geometry_failure=0;
    if(log_file){fwprintf(log_file,L"hammerburst_reload_geometry weapon=%llx pivot=%.9g,%.9g,%.9g axis=%.9g,%.9g,%.9g contact=%.9g,%.9g,%.9g units=%.9g\n",
        f.weapon.address,g.latch.pivot.x,g.latch.pivot.y,g.latch.pivot.z,g.latch.axis.x,g.latch.axis.y,g.latch.axis.z,
        g.latch.closed_contact.x,g.latch.closed_contact.y,g.latch.closed_contact.z,units);fflush(log_file);}
    return geometry_current(f);
}
V contact() noexcept {
    const auto& g=geometry.latch;const auto v=sub(g.closed_contact,g.pivot);
    const double dot=v.x*g.axis.x+v.y*g.axis.y+v.z*g.axis.z;
    const V cross{g.axis.y*v.z-g.axis.z*v.y,g.axis.z*v.x-g.axis.x*v.z,g.axis.x*v.y-g.axis.y*v.x};
    return add(g.pivot,add(add(scale(v,std::cos(angle)),scale(cross,std::sin(angle))),scale(g.axis,dot*(1-std::cos(angle)))));
}
bool at_latch(hand::Frame const& f) noexcept {P left{};return local_hand(f,left)&&length(sub(left.position,contact()))<=geometry.latch.contact_radius;}
bool granted(hand::Frame const& f,hand::Decision const& d,hand::LeaseKind kind,bool fresh=false) noexcept {
    return !d.release_required&&hand::same(d.key,f.key)&&
        (d.status==hand::DecisionStatus::granted||d.status==hand::DecisionStatus::retained)&&
        d.lease.kind==kind&&d.lease.id==f.press_token&&hand::lease_valid(d.lease,f.key.lifecycle)&&
        (!fresh||(d.new_grant&&d.status==hand::DecisionStatus::granted));
}
void propose(hand::Frame const& f,hand::GripGameState& game) noexcept {
    if(!game.main_hand_held||game.busy||game.animation_owns_hands||!f.left_tracked||!f.left_grip_down||
       !geometry_current(f)||game.kind!=hand::WeaponKind::firearm)return;
    if(stage==Stage::recovering||stage==Stage::completing||(stage==Stage::restoring&&!restore_display_ready))return;
    const auto interaction_phase=stage==Stage::restoring?parked_plan.resume_stage:stage;
    // Preserve ONLY this callback's original supply proposal through
    // its acquisition Frame. The new incumbent enters NEXT Frame prepare/
    // commit normally. Never relax Client comparison or first-grant checks.
    if(stage==Stage::new_magazine&&supply_publication.same_acquisition_source(f,held,new_item)){
        const P body{f.stereo.head.position,f.reference.orientation};
        game.request_ammo=gears_weapon::body_slot(nativeequipment::calibration,body,f.controllers.hands[0].grip.position,0)==gears_weapon::BodySlot::shoulder_ammo;
        static unsigned rows{};
        if(log_file&&rows<8){fwprintf(log_file,L"hammerburst_supply_frame frame=%llu sequence=%llu token=%llu request_ammo=%u defer_incumbent=1\n",
            f.key.frame,f.controllers.sequence,f.press_token,unsigned(game.request_ammo));fflush(log_file);++rows;}
        return;
    }
    if((stage==Stage::acquiring||stage==Stage::opening)&&latch_publication.same_acquisition_source(f,held,old_item)){
        game.request_magazine=at_latch(f);
        static unsigned rows{};
        if(log_file&&rows<8){fwprintf(log_file,L"hammerburst_latch_frame frame=%llu sequence=%llu token=%llu request_magazine=%u defer_incumbent=1\n",
            f.key.frame,f.controllers.sequence,f.press_token,unsigned(game.request_magazine));fflush(log_file);++rows;}
        return;
    }
    if(held.id==f.press_token&&hand::lease_valid(held,f.key.lifecycle)){
        for(auto& slot:game.object_incumbents){if(hand::same(slot,held))return;
            if(slot.kind==hand::LeaseKind::none){slot=held;return;}}
        return;
    }
    if(interaction_phase==Stage::waiting_ammo){
        if(empty_supply.rejected(f.key.lifecycle,f.press_token))return;
        const P body{f.stereo.head.position,f.reference.orientation};
        game.request_ammo=gears_weapon::body_slot(nativeequipment::calibration,body,f.controllers.hands[0].grip.position,0)==gears_weapon::BodySlot::shoulder_ammo;
    }else if(interaction_phase==Stage::old_magazine&&visual::active&&visual::last_grab_known){
        gears_rig_tracking::Frame rig{};const auto expected=static_cast<std::uint32_t>(f.stereo.source);
        if(handrig::read(rig,0,expected).reply!=gears_rig_tracking::Reply::accepted||
           gears_equipment_rig::match(f,rig,weaponhand::clock_ns(nullptr),expected)!=gears_equipment_rig::Status::matched||
           !(rig.world_pose_mask&gears_rig_tracking::left_grip_pose)||
           !gears_vr::stereo_pose_valid(rig.world_grip[0])||!std::isfinite(rig.units_per_metre))return;
        game.request_magazine=length(sub(rig.world_grip[0].position,visual::last_grab_world))<=.08*rig.units_per_metre;
    }else if(interaction_phase==Stage::idle||interaction_phase==Stage::acquiring||interaction_phase==Stage::opening||interaction_phase==Stage::opened||interaction_phase==Stage::closing)
        game.request_magazine=at_latch(f);
}
bool animation_owned(hand::Frame const& f) noexcept {
    if(serial&&native_exited&&parked_plan.valid&&anim::ownership.active&&anim::ownership.visual_only&&
       anim::ownership.serial==serial&&anim::ownership.inventory==opening.identity)
        return fresh_restore_main(f)&&geometry_current(f)&&inv::identity(f)==opening.identity;
    return serial?lancerreload_state::owns(f,serial):
        !lancerreload_state::lease.acquired&&nativeequipment::current(nullptr,f)&&
        opening.identity==inv::identity(f)&&(!anim::ownership.active||
        (anim::ownership.visual_only&&anim::ownership.inventory==opening.identity));
}
std::uint64_t animation_serial() noexcept {return serial?serial:old_item;}
bool restore_animation() noexcept {return anim::ownership.visual_only?hammerburst_visual_clip::restore():anim::hand_back_after_native_exit();}
bool blocks_fire(hand::ObjectIdentity const& weapon) noexcept {
    return stage!=Stage::idle&&hand::same(weapon,opening.identity.weapon);
}
void switch_dispatched(hand::Frame const& f) noexcept {
    if(stage!=Stage::idle&&opening.identity==inv::identity(f))switch_requested=true;
}
void reset(bool preserve_geometry=false) noexcept {
    restoration_admission::cancel();
    restore_display_ready=false;
    restore_display_step=0;restoration_failed=false;
    restore_resume_step=0;restore_resume_intent=0;resume_grant_ready=restore_native_uncertain=false;
    const bool release_outstanding=stage!=Stage::parked;
    stage=Stage::idle;observer={};transaction={};writer={};opening={};completion={};reward={};
    supply_publication.reset();latch_publication.reset();held={};serial=old_item=new_item=0;angle=0;released=switch_requested=native_exited=ui_pending=false;
    feed_item=feed_sequence=0;feed_ns=0;
    open_pose_reads=open_stable_reads=feed_failure=0;last_open_observed=0;
    stroke.reset();insertion.reset();extraction.reset();feed={};if(!preserve_geometry)geometry.known=false;
    last_latch_seek_elapsed_ns=0;
    latch_stationary_holds=0;
    if(release_outstanding)lancerreload_runtime::outstanding.store(false,std::memory_order_release);
    parked_plan={};parked_native_lease={};
}
bool cleanup(unsigned line=0) noexcept {
    if(stage!=Stage::recovering&&log_file){
        fwprintf(log_file,L"hammerburst_reload_cleanup line=%u stage=%u serial=%llu angle=%.9g animation=%u stopped=%u pending=%u stationary_holds=%u\n",
            line,unsigned(stage),serial,angle,unsigned(anim::ownership.active),unsigned(anim::ownership.stopped),unsigned(transaction.pending()),latch_stationary_holds);fflush(log_file);
    }
    stage=Stage::recovering;held={};stroke.reset();insertion.reset();
    // Never write into a recycled weapon. Only positive registry retirement
    // permits dropping receipts without restoring the old object's component.
    if(lancerreload_runtime::weapon_retired(opening.identity.weapon)){
        visual::active=false;visual::saved={};visual::last_grab_known=false;
        lancerreload_state::lease={};anim::ownership={};reset();return true;
    }
    if(restore_native_uncertain)return false; // Unknown original dispatch cannot be acknowledged as cleaned.
    inv::Request pending_request{};
    if(transaction.dispatched_request(pending_request)&&!lancerreload_state::restore_partial_insert(pending_request,observer))return false;
    if(!lancerreload_state::recover_owned()||!restore_animation()||!visual::restore())return false;
    if(!cleanup_completion::settle(transaction,observer,opening,reward))return false;
    reset();return true;
}
#include "weapon_hammerburst_reload_park.inl"
#include "weapon_hammerburst_restore_admission.inl"
#include "weapon_hammerburst_restore_rebind.inl"
#include "weapon_hammerburst_restore_display.inl"
#include "weapon_hammerburst_restore_resume.inl"
bool restored_game_time(hand::Frame const& source,float& value) noexcept {
    hand::ObjectIdentity world{};
    return nativeequipment::identify(nullptr,source.world.address,world)&&hand::same(world,source.world)&&
        playground::pointer(source.pawn.address,L"WorldInfo")==world.address&&
        lancerreload_state::time_field(world.address,L"TimeSeconds",value)&&nativeequipment::current(nullptr,source);
}
#include "weapon_hammerburst_restore_clock.inl"
#include "weapon_hammerburst_latch_history.inl"
bool hand_sample(hand::Frame const& f,gears_weapon::HammerburstLatchSample& out) noexcept {
    P left{};if(!local_hand(f,left))return false;
    out={left.position,f.controllers.sequence,std::uint64_t(f.controllers.sync_ns/1000000),f.press_token,
        f.left_tracked!=0,f.left_grip_known&&f.left_grip_down};
    latch_history::capture(f,out);return true;
}
bool move_magazine(hand::Frame const& f) noexcept {
    gears_rig_tracking::Frame rig{};
    if(!(nativelancerfollow::prepared_for(f,rig)||nativeequipment_rig::acquire(f,rig).status==nativeequipment_rig::Status::ready)||
       !(rig.world_pose_mask&gears_rig_tracking::left_grip_pose)||!gears_vr::stereo_pose_valid(rig.world_grip[0]))return false;
    const auto& left=rig.world_grip[0];
    const auto position=add(left.position,scale(gears_weapon::rotate(left.orientation,feed.item_in_left_grip.position),rig.units_per_metre));
    const auto orientation=gears_vr::multiply(left.orientation,feed.item_in_left_grip.orientation);
    return visual::move_target(f,{position,gears_vr::ue_angles(gears_vr::rotation_matrix(orientation))},left.position);
}
bool prepare_feed(hand::Frame const& f) noexcept {
    feed_failure=1;
    std::array<P,3> bones{};P left{};
    if(!parts_local(f,bones)||!local_hand(f,left))return false;
    const auto relative=gears_vr::multiply(bones[1].orientation,gears_vr::inverse(geometry.closed_orientation));
    const double signed_sine=relative.x*geometry.latch.axis.x+relative.y*geometry.latch.axis.y+relative.z*geometry.latch.axis.z;
    const double observed_angle=2*std::atan2(relative.w<0?-signed_sine:signed_sine,std::abs(relative.w));
    if(log_file){fwprintf(log_file,L"hammerburst_reload_bone_angle expected=%.9g observed=%.9g\n",angle,observed_angle);fflush(log_file);}
    if(std::abs(observed_angle-angle)>.03){
        feed_failure=2;++open_pose_reads;
        if(std::abs(observed_angle-last_open_observed)<.002)++open_stable_reads;else open_stable_reads=0;
        last_open_observed=observed_angle;
        // Only multiple stable NEW-frame native bone receipts can select the
        // endpoint sampling law. The reference decoder is not runtime proof.
        auto& latch=anim::ownership.latch;double predicted{};
        if(open_stable_reads>=2&&latch.sampling==gears_weapon::HammerburstLatchCurve::Sampling::reference_keys&&
           anim::reconcile_seek()&&latch.curve.angle_at_time(anim::ownership.last.time,predicted,
               gears_weapon::HammerburstLatchCurve::Sampling::endpoint_intervals)&&
           std::abs(predicted-observed_angle)<.01&&std::abs(predicted-angle)>.03){
            latch.sampling=gears_weapon::HammerburstLatchCurve::Sampling::endpoint_intervals;
            open_pose_reads=open_stable_reads=0;
            if(log_file){fwprintf(log_file,L"hammerburst_reload_sampling endpoint_intervals=1 observed=%.9g predicted=%.9g time=%.9g\n",
                observed_angle,predicted,anim::ownership.last.time);fflush(log_file);}
        }
        // Correct only the still-authenticated hand's requested endpoint. No
        // animation clock/elapsed reward window or native inventory is changed.
        if(latch.sampling==gears_weapon::HammerburstLatchCurve::Sampling::endpoint_intervals&&
           !anim::seek_latch_angle(f,animation_serial(),angle)&&anim::ownership.seek_pending)feed_failure=3;
        return false;
    }
    feed_failure=3;
    anim::remember_sampling(f); // Only reached after actual open-bone agreement.
    const auto outward=sub(bones[2].position,bones[1].position);const auto distance=length(outward);
    if(distance<.01||distance>.3)return false;
    feed={};feed.port_in_weapon=bones[2];feed.outward_axis_in_weapon=scale(outward,1/distance);
    feed.item_in_left_grip={gears_weapon::rotate(gears_vr::inverse(left.orientation),sub(bones[2].position,left.position)),
        gears_vr::multiply(gears_vr::inverse(left.orientation),bones[2].orientation)};
    // Geometry is specific to the current open native bone chain. These
    // tolerances are authored interaction limits, pending physical feedback.
    if(!feed.valid())return false;
    if(log_file){fwprintf(log_file,L"hammerburst_reload_feed port=%.9g,%.9g,%.9g outward=%.9g,%.9g,%.9g item_offset=%.9g,%.9g,%.9g\n",
        feed.port_in_weapon.position.x,feed.port_in_weapon.position.y,feed.port_in_weapon.position.z,
        feed.outward_axis_in_weapon.x,feed.outward_axis_in_weapon.y,feed.outward_axis_in_weapon.z,
        feed.item_in_left_grip.position.x,feed.item_in_left_grip.position.y,feed.item_in_left_grip.position.z);fflush(log_file);}
    feed_failure=0;return true;
}
gears_weapon::FeedResult insertion_step(hand::Frame const& f,std::uint64_t item) noexcept {
    P left{};if(!local_hand(f,left)){insertion.reset();return gears_weapon::FeedResult::rejected;}
    if(feed_item!=item||!feed_ns||f.controllers.sync_ns-feed_ns>feed.maximum_sample_gap_ns){
        // A gap cannot carry approach progress. Start a new stroke at this
        // current authenticated held-item pose; it must approach again from
        // outside before seating. Tracking-loss regrab is separately gated.
        if(!insertion.arm(f.weapon.address,item,f.world.address,f.stereo.space_epoch))return gears_weapon::FeedResult::rejected;
        feed_sequence=0;
    }
    if(f.controllers.sequence<=feed_sequence||f.controllers.sync_ns<=feed_ns)return gears_weapon::FeedResult::rejected;
    feed_item=item;feed_sequence=f.controllers.sequence;feed_ns=f.controllers.sync_ns;
    const auto position=add(left.position,gears_weapon::rotate(left.orientation,feed.item_in_left_grip.position));
    const auto delta=sub(position,feed.port_in_weapon.position);
    const auto axis=feed.outward_axis_in_weapon;
    const double depth=delta.x*axis.x+delta.y*axis.y+delta.z*axis.z;
    const double lateral=length(sub(delta,scale(axis,depth)));
    const auto orientation=gears_vr::multiply(left.orientation,feed.item_in_left_grip.orientation);
    const auto target=feed.port_in_weapon.orientation;
    const double aligned=std::abs(orientation.x*target.x+orientation.y*target.y+orientation.z*target.z+orientation.w*target.w);
    if(log_file&&feed_logs<24&&(!last_feed_position_known||length(sub(left.position,last_feed_position))>.01)){
        fwprintf(log_file,L"hammerburst_feed_sample stage=%u sequence=%llu item=%llu hand=%.9g,%.9g,%.9g hand_quat=%.9g,%.9g,%.9g,%.9g depth=%.9g lateral=%.9g orientation_dot=%.9g\n",
            unsigned(stage),f.controllers.sequence,item,left.position.x,left.position.y,left.position.z,
            left.orientation.x,left.orientation.y,left.orientation.z,left.orientation.w,depth,lateral,aligned);fflush(log_file);
        ++feed_logs;last_feed_position=left.position;last_feed_position_known=true;
    }
    gears_weapon::FeedSample sample{f.weapon.address,item,f.world.address,f.stereo.space_epoch,f.controllers.sequence,
        f.controllers.sync_ns,{{0,0,0},{0,0,0,1}},left,f.left_tracked!=0,true};
    if(stage==Stage::old_magazine&&!extraction.complete())extraction.observe(sample,feed);
    return insertion.observe(sample,feed);
}
void step(hand::Frame const& f,hand::Decision const& decision) noexcept {
    if(stage==Stage::acquiring){
        if(!animation_owned(f)||!f.left_grip_known)return;
        // Acquisition may span frames. A real known UP is still required
        // after tracking/ownership loss before any fresh grant can regrab.
        // It neither acquires the animation nor advances the latch.
        if(!f.left_grip_down){released=true;held={};return;}
        if(!granted(f,decision,hand::LeaseKind::magazine))return;
        if(held.id!=f.press_token){
            if(!released||!granted(f,decision,hand::LeaseKind::magazine,true))return;
            held=decision.lease;released=false;
        }
        if(!anim::ownership.active){
            const auto result=(serial?anim::acquire(f,serial):hammerburst_visual_clip::start(f,old_item));
            if(result==anim::Acquire::recovery_required)cleanup(__LINE__);
            return; // Stop and the first position change use distinct frames.
        }
        geometry.latch.open_angle=anim::ownership.latch.curve.open_angle();
        if(log_file){fwprintf(log_file,L"hammerburst_reload_latch open_angle=%.9g\n",geometry.latch.open_angle);fflush(log_file);}
        if(!anim::seek_latch_angle(f,animation_serial(),0)){
            if(anim::ownership.seek_pending)cleanup(__LINE__);
            return; // A pre-dispatch expiry has no setter to replay.
        }
        stage=Stage::opening;stroke.reset();
        // The authenticated grab already supplies this closed-angle sample.
        // Initialize the pure stroke now instead of wasting another GT frame
        // with no motion. The first changed animation pose is still later.
        gears_weapon::HammerburstLatchSample initial{};
        if(hand_sample(f,initial)&&stroke.begin(geometry.latch,initial,0))latch_history::previous=latch_history::sampled;
        return;
    }
    if(stage==Stage::opened){
        // Bone matrices may be updated after the seek's own GT dispatch.
        // Read on the next tick; do not mistake immediate stale matrices for
        // a verified open feed point.
        if(!f.left_grip_down){released=true;held={};return;}
        if(!granted(f,decision,hand::LeaseKind::magazine))return;
        held=decision.lease;
        if(!prepare_feed(f)){
            if(feed_failure==3||open_pose_reads>=8)cleanup(__LINE__);
            return;
        }
        if(!visual::begin(f,visual::Profile::hammerburst)){cleanup(__LINE__);return;}
        if(!insertion.arm(f.weapon.address,old_item,f.world.address,f.stereo.space_epoch)){cleanup(__LINE__);return;}
        extraction.reset();stage=Stage::old_magazine;insertion_step(f,old_item);return;
    }
    if(!f.left_grip_known)return;
    if(!f.left_grip_down){
        released=true;held={};stroke.reset();resume_grant_ready=false;
        if(stage==Stage::old_magazine){
            // A premature release cancels and restores the owned component;
            // it cannot expose shoulder supply without actual extraction.
            if(!extraction.complete()){cleanup(__LINE__);return;}
            if(!transaction.stow_original(old_item)||!visual::stow()){cleanup(__LINE__);return;}
            stage=Stage::waiting_ammo;
        }else if(stage==Stage::new_magazine){
            if(!transaction.release_item(new_item)||!visual::stow()){cleanup(__LINE__);return;}
            new_item=0;stage=Stage::waiting_ammo;
        }
        return; // Opening/closing retain the actual mechanical angle.
    }
    if(stage==Stage::opening||stage==Stage::closing){
        if(!granted(f,decision,hand::LeaseKind::magazine))return;
        gears_weapon::HammerburstLatchSample sample{};if(!hand_sample(f,sample))return;
        if(!stroke.active()){
            if(held.id!=f.press_token&&(!released||!granted(f,decision,hand::LeaseKind::magazine,true)))return;
            if(stroke.begin(geometry.latch,sample,angle)){held=decision.lease;released=false;latch_history::previous=latch_history::sampled;}
            return;
        }
        const auto previous_ms=stroke.last_observation_ms();
        const bool retained_contact=f.left_tracked&&decision.status==hand::DecisionStatus::retained&&
            !decision.new_grant&&hand::same(held,decision.lease);
        gears_weapon::HammerburstLatchMotion moved{};
        const bool history_needed=sample.milliseconds>previous_ms&&sample.milliseconds-previous_ms>geometry.latch.observation_interval_ms;
        const bool used_history=history_needed&&latch_history::advance(f,sample,retained_contact,moved);
        if(!used_history)moved=stroke.observe(sample,retained_contact);
        if(moved!=gears_weapon::HammerburstLatchMotion::rejected)latch_history::previous=latch_history::sampled;
        if(history_needed&&log_file){static unsigned rows{};if(rows++<12){
            fwprintf(log_file,L"hammerburst_latch_history gate=%u used=%u result=%u previous_capture=%u current_capture=%u serial=%llu frame=%llu previous_ms=%llu sample_ms=%llu\n",
                latch_history::gate,unsigned(used_history),unsigned(moved),latch_history::previous.capture_gate,latch_history::sampled.capture_gate,serial,f.key.frame,previous_ms,sample.milliseconds);fflush(log_file);}}

        if(moved==gears_weapon::HammerburstLatchMotion::held_stationary){
            if(latch_stationary_holds!=~0u)++latch_stationary_holds;
            return; // No seek, mechanical progress, completion or new grab.
        }
        if(moved==gears_weapon::HammerburstLatchMotion::rejected){
            if(log_file&&latch_rejection_logs++<8){
                const auto previous_hand=stroke.last_hand_in_weapon();
                const double hand_delta=std::hypot(std::hypot(sample.hand.x-previous_hand.x,sample.hand.y-previous_hand.y),sample.hand.z-previous_hand.z);
                fwprintf(log_file,L"hammerburst_latch_rejected reason=%u stage=%u serial=%llu angle=%.9g sequence=%llu previous_ms=%llu sample_ms=%llu interval_limit_ms=%llu previous_seek_ns=%lld stationary_holds=%u retained_same=%u hand_delta_m=%.9g\n",
                    unsigned(stroke.last_rejection()),unsigned(stage),serial,angle,sample.sequence,
                    previous_ms,sample.milliseconds,geometry.latch.observation_interval_ms,last_latch_seek_elapsed_ns,latch_stationary_holds,unsigned(retained_contact),hand_delta);fflush(log_file);
            }
            held={};released=false;return;
        }
        // A fresh stationary sample still advances the continuity witness,
        // but must not reparse the whole animation and redispatch the same
        // native setter. Those redundant calls can stall the next sample.
        // Changed angles retain the full original ownership/content checks.
        if(std::abs(stroke.angle()-angle)>1e-7){
            const auto began=weaponhand::clock_ns(nullptr);
            const bool sought=anim::seek_latch_angle(f,animation_serial(),stroke.angle());
            last_latch_seek_elapsed_ns=weaponhand::clock_ns(nullptr)-began;
            if(!sought){cleanup(__LINE__);return;}
        }
        angle=stroke.angle();
        if(stage==Stage::opening&&moved==gears_weapon::HammerburstLatchMotion::open){stage=Stage::opened;stroke.reset();}
        else if(stage==Stage::closing&&moved==gears_weapon::HammerburstLatchMotion::closed){
            std::int64_t tick=f.controllers.sync_ns/1000000;
            std::int64_t accepted_begin{},window{};
            if(!closure_clock(f,tick,accepted_begin,window))window=0;
            if(log_file){fwprintf(log_file,L"hammerburst_reload_closure serial=%llu elapsed_game_ms=%lld perfect_end_ms=%lld inserted=%u target=%u\n",
                serial,tick-accepted_begin,window,
                transaction.inserted(),transaction.target());fflush(log_file);}
            if(!transaction.confirm_closed(tick,window)||(serial&&!transaction.take_request(completion))){cleanup(__LINE__);return;}
            stage=Stage::completing;held={};
        }
        return;
    }
    if(stage==Stage::waiting_ammo){
        const bool resumed_grant=resume_grant_ready&&granted(f,decision,hand::LeaseKind::ammo)&&hand::same(held,decision.lease);
        if(!resumed_grant&&(!released||!granted(f,decision,hand::LeaseKind::ammo,true)))return;
        resume_grant_ready=false;
        inv::Inventory current{};
        if(!observer.sample(f,current))return;
        const auto expected=transaction.inventory();
        if(!(current.identity==expected.identity)||current.loaded!=expected.loaded||current.reserve!=expected.reserve||
           current.capacity!=expected.capacity||current.infinite_reserve!=expected.infinite_reserve){cleanup(__LINE__);return;}
        if(!current.loaded&&!current.reserve&&!current.infinite_reserve){
            if(!nativeequipment::current(nullptr,f))return;
            if(empty_supply.reject(f.key.lifecycle,f.press_token)){
                gears_weapon::EquipmentSnapshot cue{};cue.world=f.world.address;cue.pawn=f.pawn.address;cue.current_weapon=f.weapon.address;
                bool played{};try{played=vrequip::play_empty_slot_sound(cue,4);}catch(...){}
                if(log_file){fwprintf(log_file,L"hammerburst_empty_supply token=%llu sound=%u replacement_created=0\n",f.press_token,unsigned(played));fflush(log_file);}
            }
            return;
        }
        new_item=decision.lease.id;
        if(!transaction.reserve_magazine_reusing_loaded(new_item)||!visual::take_replacement(f)||
           !insertion.arm(f.weapon.address,new_item,f.world.address,f.stereo.space_epoch)){cleanup(__LINE__);return;}
        if(log_file){fwprintf(log_file,L"hammerburst_supply token=%llu rounds=%u spare_reserved=%u loaded=%u reserve=%u\n",
            new_item,transaction.held_magazine_rounds(),transaction.reserved(),current.loaded,current.reserve);fflush(log_file);}
        supply_publication.remember(f,decision);
        held=decision.lease;released=false;stage=Stage::new_magazine;insertion_step(f,new_item);return;
    }
    if(stage!=Stage::old_magazine&&stage!=Stage::new_magazine)return;
    const auto kind=stage==Stage::old_magazine?hand::LeaseKind::magazine:hand::LeaseKind::ammo;
    if(stage==Stage::old_magazine&&held.id!=f.press_token){
        if(!released||!granted(f,decision,kind,true))return;
        if(!insertion.arm(f.weapon.address,old_item,f.world.address,f.stereo.space_epoch)){cleanup(__LINE__);return;}
        held=decision.lease;released=false;insertion_step(f,old_item);return;
    }
    if(!granted(f,decision,kind)||!move_magazine(f))return;
    const auto item=stage==Stage::old_magazine?old_item:new_item;
    if(insertion_step(f,item)!=gears_weapon::FeedResult::inserted)return;
    if(stage==Stage::old_magazine){if(!transaction.restore_original(old_item)){cleanup(__LINE__);return;}}
    else if(!transaction.target()){
        if(!transaction.seat_reused_rounds(item)){cleanup(__LINE__);return;}
    }else{
        inv::Request request{};inv::Receipt receipt{};
        if(!transaction.request_insertion(item)||!transaction.take_request(request)){cleanup(__LINE__);return;}
        const auto result=writer.commit(f,request,observer,lancerreload_state::owns,receipt);
        if(!receipt.token){receipt.token=request.token;observer.sample(f,receipt.observed);}
        if(result==inv::CommitResult::reconciliation_required||!transaction.acknowledge(receipt)||result!=inv::CommitResult::committed){cleanup(__LINE__);return;}
    }
    if(!visual::stow()||!visual::flag(f.weapon.address,L"SetWeaponAmmoBoneDisplay",L"bShow",true)){cleanup(__LINE__);return;}
    stage=Stage::closing;held={};released=false;stroke.reset();
}
bool tick(uptr pc) noexcept {
    if(stage==Stage::idle&&lancerreload_runtime::stage!=lancerreload_runtime::Stage::idle)return false;
    if(stage==Stage::recovering){cleanup(__LINE__);return true;}
    if(stage==Stage::parked&&lancerreload_runtime::stop_requested.load(std::memory_order_acquire)){
        reset();return false; // No live resources; do not touch another owner's services.
    }
    if(lancerreload_runtime::stop_requested.load(std::memory_order_acquire)){
        if(stage!=Stage::idle){cleanup(__LINE__);return true;}return false;
    }
    if(stage==Stage::parking){
        if(lancerreload_runtime::weapon_retired(opening.identity.weapon))cleanup(__LINE__);
        else park_control::finish();
        return true;
    }
    if(stage==Stage::parked){
        if(lancerreload_runtime::weapon_retired(opening.identity.weapon)){reset();return false;}
        if(uptr(vrequip::model.held_weapon())!=opening.identity.weapon.address)return false;
        hand::Frame resumed{};hand::GripGameState resumed_game{};hand::Decision resumed_decision{};
        if(weaponhand::client.copy_frame_on_gt(resumed,resumed_game,resumed_decision)&&resumed.controller.address==pc&&
           resumed_game.known&&resumed_game.main_hand_held&&!resumed_game.busy&&!resumed_game.animation_owns_hands)
            restore_control::rebind(resumed);
        return true; // No hand action or display mutation on the rebind frame.
    }
    if(stage==Stage::restoring){
        if(lancerreload_runtime::weapon_retired(opening.identity.weapon)){cleanup(__LINE__);return true;}
        if(switch_requested){park_control::begin();return true;}
        hand::Frame resumed{};hand::GripGameState resumed_game{};hand::Decision resumed_decision{};
        if(weaponhand::client.copy_frame_on_gt(resumed,resumed_game,resumed_decision)&&resumed.controller.address==pc&&
           resumed_game.known&&resumed_game.main_hand_held&&!resumed_game.busy&&!resumed_game.animation_owns_hands){
            if(restoration_failed){park_control::begin();return true;}
            const bool failed=restore_display_ready?
                restore_control::resume(resumed,resumed_decision)==restore_control::ResumeResult::recovery_required:
                restore_control::display(resumed)==restore_control::DisplayResult::recovery_required;
            if(failed){
                restoration_failed=true;park_control::begin();
            }
        }else released=false;
        // A restored picture is not permission to reuse an old hand action.
        return true;
    }
    if(switch_requested){
        if(stage!=Stage::idle&&(!park_control::enabled()||(!park_control::begin()&&stage!=Stage::parking)))cleanup(__LINE__);
        return stage!=Stage::idle;
    }
    if(stage==Stage::completing){
        if(!serial){
            inv::Inventory counts{};
            if(!observer.sample_owned(opening.identity,counts))return true;
            const auto expected=transaction.inventory();
            if(counts.loaded!=expected.loaded||counts.reserve!=expected.reserve||counts.capacity!=expected.capacity||
               counts.infinite_reserve!=expected.infinite_reserve){cleanup(__LINE__);return true;}
            if(!transaction.mechanical_ready()||!restore_animation()||!visual::restore()){cleanup(__LINE__);return true;}
            if(log_file){fwprintf(log_file,L"hammerburst_inspection_complete loaded=%u reserve=%u inventory_changed=0\n",counts.loaded,counts.reserve);fflush(log_file);}
            reset(true);return true;
        }
        if(!native_exited){
            if(!lancerreload_state::recover_owned()||!restore_animation()){cleanup(__LINE__);return true;}
            native_exited=true;return true;
        }
        if(anim::ownership.active){
            if(!anim::ownership.visual_only||!(anim::ownership.inventory==opening.identity)||!restore_animation())return true;
        }
        inv::Inventory counts{};
        if(completion.result==gears_weapon::ReloadTxnResult::perfect&&
           !lancerreload_state::perfect_reward(completion,opening,observer,reward))return true;
        if(!observer.sample_owned(completion.identity,counts))return true;
        const auto applied=completion.result==gears_weapon::ReloadTxnResult::inspection?gears_weapon::ReloadTxnResult::none:completion.result;
        if(!transaction.acknowledge({completion.token,true,counts,applied})){cleanup(__LINE__);return true;}
        if(visual::restore()){
            if(log_file){fwprintf(log_file,L"hammerburst_reload_complete serial=%llu result=%u loaded=%u reserve=%u\n",
                serial,unsigned(completion.result),counts.loaded,counts.reserve);fflush(log_file);}
            reset(true); // Immutable same-instance closed geometry, not a grant.
        }else stage=Stage::recovering;
        return true;
    }
    hand::Frame f{};hand::GripGameState game{};hand::Decision decision{};
    if(!weaponhand::client.copy_frame_on_gt(f,game,decision)||f.controller.address!=pc||
       !game.known||!game.main_hand_held||game.busy||game.animation_owns_hands){
        if(stage!=Stage::idle){held={};stroke.reset();insertion.reset();extraction.reset();released=false;
            if(lancerreload_runtime::weapon_retired(opening.identity.weapon))cleanup(__LINE__);return true;}
        return false;
    }
    if(stage!=Stage::idle){
        const auto actual_identity=inv::identity(f);
        if(!(actual_identity==opening.identity)){
            const auto& old=opening.identity;const auto& now=actual_identity;
            const unsigned difference=unsigned(!hand::same(old.world,now.world))|
                (unsigned(!hand::same(old.pawn,now.pawn))<<1)|(unsigned(!hand::same(old.weapon,now.weapon))<<2)|
                (unsigned(old.game_start!=now.game_start)<<3)|(unsigned(old.native_start!=now.native_start)<<4)|
                (unsigned(old.registration!=now.registration)<<5)|(unsigned(old.lifecycle!=now.lifecycle)<<6)|
                (unsigned(old.world_generation!=now.world_generation)<<7)|(unsigned(old.hold_generation!=now.hold_generation)<<8)|
                (unsigned(old.session!=now.session)<<9)|(unsigned(old.space!=now.space)<<10)|(unsigned(old.input!=now.input)<<11);
            static unsigned rows{};
            if(log_file&&rows<8){fwprintf(log_file,L"hammerburst_identity_lost difference=%u frame=%llu sequence=%llu stage=%u old_hold=%llu now_hold=%llu old_lifecycle=%llu now_lifecycle=%llu old_world_generation=%llu now_world_generation=%llu old_input=%llu now_input=%llu old_space=%llu now_space=%llu\n",
                difference,f.key.frame,f.controllers.sequence,unsigned(stage),old.hold_generation,now.hold_generation,
                old.lifecycle,now.lifecycle,old.world_generation,now.world_generation,old.input,now.input,old.space,now.space);
                fflush(log_file);++rows;}
            cleanup(__LINE__);return true;
        }
    }
    if(stage!=Stage::idle&&serial&&!native_exited){
        if(ui_pending){
            const auto display=opening.loaded?opening_policy.tactical_ms:opening_policy.empty_ms;
            const auto window=opening_policy.deadline(opening.loaded==0,lancerreload_state::lease.perfect_end_ms);
            if(!lancerreload_state::set_ui_timing(f,serial,display,window,opening_policy.mode!=gears_weapon::LancerPerfectMode::original)){
                if(!(lancerreload_state::lease.ui_owned&&lancerreload_state::lease.attempts_owned&&lancerreload_state::ui_setup_gate>=6)){
                    cleanup(__LINE__);return true;
                }
            }
            lancerreload_state::lease.perfect_end_ms=window;ui_pending=false;return true;
        }
        if(!lancerreload_state::tick_ui_timing()){cleanup(__LINE__);return true;}
    }
    if(stage==Stage::idle){
        if(!eligible(f))return false;
        if(!geometry_current(f)){
            const auto now=weaponhand::clock_ns(nullptr);
            if(now>=geometry_retry_ns){
                discover(f);geometry_retry_ns=now+(anim::parts_failure==2?50000000:1000000000);
                if(geometry_failure!=last_geometry_failure&&log_file){
                    fwprintf(log_file,L"hammerburst_reload_geometry_status gate=%u parts_gate=%u\n",geometry_failure,anim::parts_failure);fflush(log_file);
                    last_geometry_failure=geometry_failure;
                }
            }return true;
        }
        if(!f.left_tracked||!f.left_grip_down||!granted(f,decision,hand::LeaseKind::magazine,true)||!at_latch(f))return true;
        const auto starting=inv::identity(f);
        if((starting==last_started_identity||inv::same_instance_across_hold(last_started_identity,starting))&&
           f.press_token<=last_started_grip)return true;
        inv::Inventory before{};
        if(!observer.sample(f,before))return true;
        const bool needs_reload=before.loaded<before.capacity&&(before.infinite_reserve||before.reserve);
        if(!gears_weapon::LancerReloadPolicy::unpack(configured_policy.load(std::memory_order_acquire),opening_policy))return true;
        const auto result=reload_begin::acquire(f,before,serial,reward);
        if(result==lancerreload_state::BeginResult::rejected)return true;
        last_started_identity=starting;last_started_grip=f.press_token;
        latch_publication.remember(f,decision,hand::LeaseKind::magazine);
        opening=before;old_item=f.press_token;held=decision.lease;ui_pending=needs_reload;
        if(log_file){fwprintf(log_file,L"hammerburst_reload_begin serial=%llu accepted_ms=%lld perfect_window_ms=%lld game_start=%.9g\n",
            serial,lancerreload_state::lease.accepted_begin_ms,lancerreload_state::lease.perfect_end_ms,
            double(lancerreload_state::lease.reward_start));fflush(log_file);}
        lancerreload_runtime::outstanding.store(true,std::memory_order_release);stage=Stage::acquiring;
        if(result!=lancerreload_state::BeginResult::acquired||
           !transaction.bind(before)||!transaction.begin(before,old_item,gears_weapon::ReloadIntentSource::mechanism_open,serial,
               gears_weapon::ReloadTxnMode::magazine,before.capacity,serial?lancerreload_state::lease.accepted_begin_ms:f.controllers.sync_ns/1000000,old_item)){
            cleanup(__LINE__);return true;
        }
        // Original acceptance and ownership acquisition may share this fresh
        // grab. acquire independently authenticates the live sequence, native
        // state, original functions and unchanged action grant; if not ready,
        // the normal acquiring stage observes again on a later frame. Keep
        // StopAnim and the first SetPosition on distinct frames as before.
        if((serial?anim::acquire(f,serial):hammerburst_visual_clip::start(f,old_item))==anim::Acquire::recovery_required)cleanup(__LINE__);
        return true;
    }
    if(!f.left_tracked){
        held={};stroke.reset();insertion.reset();extraction.reset();released=false;
        if(stage==Stage::new_magazine){
            if(!transaction.release_item(new_item)||!visual::stow())cleanup(__LINE__);
            else{stage=Stage::waiting_ammo;new_item=0;}
        }
        return true;
    }
    const auto before=stage;step(f,decision);
    if(before!=stage&&log_file){
        const auto counts=transaction.inventory();
        fwprintf(log_file,L"hammerburst_reload_transition from=%u to=%u serial=%llu angle=%.9g loaded=%u reserve=%u pending=%u stationary_holds=%u\n",
            unsigned(before),unsigned(stage),serial,angle,counts.loaded,counts.reserve,unsigned(transaction.pending()),latch_stationary_holds);fflush(log_file);
    }
    return true;
}

} // namespace hammerburst_runtime
