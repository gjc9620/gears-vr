#include "../vr-common/weapon_hammerburst_component_scale.h"
#include "../vr-common/weapon_hammerburst_supply_publication.h"
#include "../vr-common/weapon_hammerburst_feed_history.h"
#include "../vr-common/weapon_hammerburst_extraction_history.h"
#include "../vr-common/weapon_hammerburst_handoff.h"
#include "weapon_hammerburst_magazine_pin.inl"
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
static_assert(unsigned(Stage::old_magazine)==4&&unsigned(Stage::new_magazine)==6);
Stage stage{};
gears_weapon_hammerburst_handoff::State magazine_handoff;
struct LatentComponentScale {
    struct Field {hand::ObjectIdentity id{};unsigned offset{},bytes{};};
    Field scalar{},xyz{},absolute{},attached{};hand::ObjectIdentity vector{};
    unsigned extent{},absolute_mask{},attached_mask{};float scalar_value{};
    std::array<float,3> xyz_value{};unsigned absolute_value{},attached_value{};
    bool known{};
    bool field_current(Field const& field) const noexcept {
        hand::ObjectIdentity actual{};
        return nativeequipment::identify(nullptr,field.id.address,actual)&&hand::same(actual,field.id)&&
            get<unsigned>(field.id.address+0x70)==1&&get<unsigned>(field.id.address+0x74)==field.bytes&&
            get<unsigned>(field.id.address+0x94)==field.offset;
    }
    // Proposal callback: cached metadata identities and bounded reads ONLY.
    bool current(hand::ObjectIdentity const& component,double parent,double& size,bool attached_owned=false) const noexcept {
        if(!known||get<unsigned>(component.klass+0x88)!=extent||!field_current(scalar)||
           !field_current(xyz)||!field_current(absolute)||!field_current(attached)||
           get<uptr>(xyz.id.address+0xd0)!=vector.address||
           get<unsigned>(absolute.id.address+0xd0)!=absolute_mask||
           get<unsigned>(attached.id.address+0xd0)!=attached_mask)return false;
        hand::ObjectIdentity current_vector{};
        if(!nativeequipment::identify(nullptr,vector.address,current_vector)||!hand::same(current_vector,vector))return false;
        float value{};std::array<float,3> three{};unsigned abs{},attach{};
        if(!copy(&value,component.address+scalar.offset,4)||value!=scalar_value||
           !copy(three.data(),component.address+xyz.offset,12)||three!=xyz_value||
           !copy(&abs,component.address+absolute.offset,4)||(abs&absolute_mask)!=absolute_value||
           !copy(&attach,component.address+attached.offset,4)||(attach&attached_mask)!=(attached_owned?attached_mask:attached_value))return false;
        return gears_hammerburst_component_scale::prospective(value,three,(abs&absolute_mask)!=0,
            false,parent,size); // Attachment was separately checked against the exact expected state.
    }
    // Ordinary GT preauthentication; never invoked by a Native proposal.
    bool prepare(hand::ObjectIdentity const& component,double parent,double& size) noexcept {
        *this={};extent=get<unsigned>(component.klass+0x88);
        const auto field=[&](wchar_t const* name,wchar_t const* type,unsigned bytes,Field& out) noexcept {
            const auto property=playground::property(component.address,name);
            if(!property||!playground::named(get<uptr>(property+0x50),type)||
               get<unsigned>(property+0x70)!=1||get<unsigned>(property+0x74)!=bytes||
               !nativeequipment::identify(nullptr,property,out.id))return false;
            out.offset=get<unsigned>(property+0x94);out.bytes=bytes;
            return out.offset<=extent&&bytes<=extent-out.offset;
        };
        if(!field(L"Scale",L"FloatProperty",4,scalar)||!field(L"Scale3D",L"StructProperty",12,xyz)||
           !field(L"AbsoluteScale",L"BoolProperty",4,absolute)||!field(L"bAttached",L"BoolProperty",4,attached)||
           !nativeequipment::identify(nullptr,get<uptr>(xyz.id.address+0xd0),vector)||
           !playground::instance(vector.address,L"ScriptStruct")||!playground::named(vector.address,L"Vector"))return false;
        absolute_mask=get<unsigned>(absolute.id.address+0xd0);attached_mask=get<unsigned>(attached.id.address+0xd0);
        if(!absolute_mask||(absolute_mask&(absolute_mask-1))||!attached_mask||(attached_mask&(attached_mask-1))||
           (absolute.offset==attached.offset&&(absolute_mask&attached_mask))||
           !copy(&scalar_value,component.address+scalar.offset,4)||!copy(xyz_value.data(),component.address+xyz.offset,12)||
           !copy(&absolute_value,component.address+absolute.offset,4)||!copy(&attached_value,component.address+attached.offset,4))return false;
        absolute_value&=absolute_mask;attached_value&=attached_mask;
        if(attached_value){*this={};return false;} // Initial cache must be unattached.
        known=true;
        if(!current(component,parent,size)){*this={};return false;}return true;
    }
};
struct OpenedContactGeometry {
    inv::Identity identity{};nativelancerfollow::Key key{};
    hand::ObjectIdentity component{},asset{};
    P grip{};double gun_scale{},static_scale{},body_scale{},units{},angle{};bool known{},latent{};
    LatentComponentScale latent_scale{};
};
OpenedContactGeometry opened_contact;
namespace feed_history {void clear() noexcept;}
#include "weapon_hammerburst_reload_park_ledger.inl"
park_ledger::Plan parked_plan;
bool old_or_new_magazine_stage() noexcept {
    const auto phase=stage==Stage::restoring?parked_plan.resume_stage:stage;
    return phase==Stage::old_magazine||phase==Stage::new_magazine;
}
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
hammerburst_magazine_pin::Pin magazine_pin;
unsigned magazine_pin_bound{},magazine_pin_kept{};
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
#include "weapon_hammerburst_feed_observation.inl"

hammerburst_magazine_pin::Observation magazine_observation(hand::Frame const& parent) noexcept {
    hammerburst_magazine_pin::Observation out{};
    out.weapon=parent.weapon;out.world=parent.world;out.pawn=parent.pawn;
    out.mesh=parent.mesh;out.controller_identity=parent.controller;
    out.component=visual::saved.component;out.lease_kind=held.kind;
    out.item=stage==Stage::old_magazine?old_item:new_item;
    out.lease_id=held.id;out.lease_lifecycle=held.lifecycle;
    out.hold_generation=vrequip::model.hold_generation();out.stage=unsigned(stage);
    out.controller=parent.controller.address;return out;
}
bool magazine_lifetime(void*,hand::ExistingActionLifetime const& lifetime) noexcept {
    return weaponhand::client.existing_action_lifetime_on_gt(lifetime);
}
void remember_magazine(hand::Frame const& f,hand::Decision const& decision) noexcept {
    if((stage!=Stage::old_magazine&&stage!=Stage::new_magazine)||released||
       !visual::alive()||!hand::same(visual::saved.weapon,f.weapon)||
       !hand::same(visual::saved.pawn,f.pawn)){magazine_pin.reset();return;}
    // Existing current grants govern all motion. This optional pin only keeps
    // that same logical incumbent through a later missing render Frame.
    if(magazine_pin.known&&magazine_pin.observed.item==(stage==Stage::old_magazine?old_item:new_item)&&
       magazine_pin.observed.stage==unsigned(stage)&&magazine_pin.observed.lease_id==held.id&&
       magazine_pin.observed.lease_lifecycle==held.lifecycle&&magazine_pin.observed.lease_kind==held.kind&&
       magazine_pin.observed.hold_generation==vrequip::model.hold_generation()&&
       hand::same(magazine_pin.observed.component,visual::saved.component)&&
       hand::same(magazine_pin.parent.weapon,f.weapon)&&magazine_pin.parent.press_token==f.press_token&&
       magazine_lifetime(nullptr,magazine_pin.lifetime))return;
    magazine_pin.reset();
    if(!nativeequipment::current(nullptr,f))return;
    gears_hand_continuity::TrackedInput raw{};gears_hand_continuity::Publisher publisher{};
    hand::ExistingActionLifetime lifetime{};
    if(!nativeequipment::read_input_continuity(f,raw,publisher)||
       !weaponhand::client.capture_existing_action_lifetime_on_gt(f.key,lifetime))return;
    const auto current=magazine_observation(f);
    if(hammerburst_magazine_pin::bind(magazine_pin,f,decision,current.item,current.component,unsigned(stage),current,
        f.stereo,raw,publisher,weaponhand::clock_ns(nullptr),
        f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated,lifetime))++magazine_pin_bound;
}
bool retain_magazine(uptr pc,hand::FrameReadObservation const& failure) noexcept {
    if((failure.failure!=hand::FrameReadFailure::publication&&failure.failure!=hand::FrameReadFailure::stale_before&&
        failure.failure!=hand::FrameReadFailure::thread_or_unarmed)||
       !magazine_pin.known||!weaponhand::current_gt(nullptr)||
       pc!=magazine_pin.parent.controller.address||!nativeequipment::domain_ready()||
       !nativeequipment::restoration_quiet()||switch_requested||released||
       (stage!=Stage::old_magazine&&stage!=Stage::new_magazine)||!visual::alive()||
       !hand::same(visual::saved.weapon,magazine_pin.parent.weapon)||
       !hand::same(visual::saved.pawn,magazine_pin.parent.pawn)||
       !vrequip::model.held_for(magazine_pin.parent.world.address,magazine_pin.parent.pawn.address,
            magazine_pin.parent.weapon.address)||!magazine_lifetime(nullptr,magazine_pin.lifetime))return false;
    const auto begin=weaponhand::clock_ns(nullptr);weaponhand::HoldSourceObservation game{};
    if(begin<=0||!weaponhand::observe_hold_source(nullptr,magazine_pin.parent,begin,game)||
       !game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||
       game.kind!=hand::WeaponKind::firearm)return false;
    gears_vr::StereoSample stereo{};gears_vr::ControllerInputFrame input{};
    gears_hand_continuity::TrackedInput raw{};gears_hand_continuity::Publisher publisher{};
    const bool sim=magazine_pin.parent.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated;
    if(nativeequipment::input_continuity_reader.read(GetCurrentProcessId(),[](){return weaponhand::clock_ns(nullptr);},
       sim,stereo,input,raw,publisher)!=gears_current_tracking::Failure::none)return false;
    const auto end=weaponhand::clock_ns(nullptr);
    return end>=begin&&end-begin<=50000000&&hammerburst_magazine_pin::retain(magazine_pin,failure,
        {magazine_lifetime},nullptr,stereo,raw,publisher,end,sim,magazine_observation(magazine_pin.parent));
}

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
bool parts_local(hand::Frame const& f,std::array<P,3>& poses,vrequip::MeshPoseProbe* observed_mesh=nullptr) noexcept {
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
    if(!nativeequipment::current(nullptr,f))return false;
    if(observed_mesh)*observed_mesh=mesh;
    return true;
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
// Called inside ClientAdapter observation: local metadata and pure geometry
// ONLY. The completed GT tick authenticates the frozen open-part geometry.
bool opened_contact_current(hand::Frame const& f) noexcept {
    if(stage!=Stage::opened||!opened_contact.known||!geometry_current(f)||
       !(opened_contact.identity==inv::identity(f))||!(opened_contact.key==nativelancerfollow::bound_key)||
       opened_contact.units!=nativelancerfollow::bound_units||std::abs(opened_contact.angle-angle)>1e-7||
       !anim::ownership.active||!anim::ownership.stopped||anim::ownership.seek_pending||
       !(anim::ownership.inventory==opening.identity))return false;
    hand::ObjectIdentity component{},asset{};
    if(playground::pointer(f.weapon.address,L"MagazineMesh")!=opened_contact.component.address||
       playground::pointer(opened_contact.component.address,L"StaticMesh")!=opened_contact.asset.address||
       !nativeequipment::identify(nullptr,opened_contact.component.address,component)||!hand::same(component,opened_contact.component)||
       !nativeequipment::identify(nullptr,opened_contact.asset.address,asset)||!hand::same(asset,opened_contact.asset))return false;
    std::array<float,16> raw{};P pose{};double size{};
    if(!copy(raw.data(),opened_contact.key.component.address+0xa0,sizeof raw)||!matrix(raw,pose,size)||
       std::abs(size-opened_contact.gun_scale)>1e-4||
       !copy(raw.data(),f.mesh.address+0xa0,sizeof raw)||!matrix(raw,pose,size)||
       std::abs(size-opened_contact.body_scale)>1e-4||
       !copy(raw.data(),opened_contact.component.address+0xa0,sizeof raw))return false;
    if(opened_contact.latent){
        // The same admitted press can attach this component inside visual::begin.
        // Carry only that sole component/lease's proposal through its own setup;
        // a new press still requires the original unattached geometry witness.
        const bool own_attachment=visual::active&&hand::same(visual::saved.weapon,f.weapon)&&
            hand::same(visual::saved.component,component)&&held.id==f.press_token&&
            hand::lease_valid(held,f.key.lifecycle)&&magazine_handoff.magazine_token==f.press_token;
        double witnessed{};
        if(!opened_contact.latent_scale.current(component,opened_contact.body_scale,witnessed)&&
           !(own_attachment&&opened_contact.latent_scale.current(component,opened_contact.body_scale,witnessed,true)))return false;
        if(gears_hammerburst_component_scale::uninitialized(raw))size=witnessed;
        else if(!own_attachment||!matrix(raw,pose,size)||std::abs(size-witnessed)>1e-4)return false;
    }else if(!matrix(raw,pose,size))return false;
    return std::abs(size-opened_contact.static_scale)<=1e-4;
}
bool opened_magazine_contact(hand::Frame const& f,P& grip) noexcept {
    P physical{};
    if(!opened_contact_current(f)||!local_hand(f,physical))return false;
    grip=opened_contact.grip;
    return gears_weapon_hammerburst_handoff::in_contact(physical,grip,opened_contact.gun_scale);
}
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
    }else if(stage==Stage::opened){
        P target{};
        game.request_magazine=magazine_handoff.can_propose()&&opened_magazine_contact(f,target);
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
bool priority_frame(hand::Frame const& f) noexcept {
    return nativelancerfollow::bound_is_hammerburst&&nativelancerfollow::bound_key_known&&
        hand::same(nativelancerfollow::bound_key.weapon,f.weapon)&&
        nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation();
}
bool blocks_fire(hand::ObjectIdentity const& weapon) noexcept {
    return stage!=Stage::idle&&hand::same(weapon,opening.identity.weapon);
}
void switch_dispatched(hand::Frame const& f) noexcept {
    if(stage!=Stage::idle&&opening.identity==inv::identity(f))switch_requested=true;
}
void reset(bool preserve_geometry=false) noexcept {
    if(log_file&&(magazine_pin_bound||magazine_pin_kept)){
        fwprintf(log_file,L"hammerburst_magazine_pin bound=%u kept=%u pose_advanced=0 inventory_written=0\n",
            magazine_pin_bound,magazine_pin_kept);fflush(log_file);}
    magazine_pin_bound=magazine_pin_kept=0;
    magazine_pin.reset();
    restoration_admission::cancel();
    restore_display_ready=false;
    restore_display_step=0;restoration_failed=false;
    restore_resume_step=0;restore_resume_intent=0;resume_grant_ready=restore_native_uncertain=false;
    const bool release_outstanding=stage!=Stage::parked;
    stage=Stage::idle;observer={};transaction={};writer={};opening={};completion={};reward={};
    supply_publication.reset();latch_publication.reset();held={};serial=old_item=new_item=0;angle=0;released=switch_requested=native_exited=ui_pending=false;
    feed_item=feed_sequence=0;feed_ns=0;feed_history::clear();
    open_pose_reads=open_stable_reads=feed_failure=0;last_open_observed=0;
    magazine_handoff.reset();
    opened_contact={};
    stroke.reset();insertion.reset();extraction.reset();feed={};if(!preserve_geometry)geometry.known=false;
    last_latch_seek_elapsed_ns=0;
    latch_stationary_holds=0;
    if(release_outstanding)lancerreload_runtime::outstanding.store(false,std::memory_order_release);
    parked_plan={};parked_native_lease={};
}
bool cleanup(unsigned line=0) noexcept {
    magazine_pin.reset();
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
    if(!lancerreload_state::recover_serial(serial)||!restore_animation()||!visual::restore())return false;
    if(!cleanup_completion::settle(transaction,observer,opening,reward))return false;
    reset();return true;
}
#include "weapon_hammerburst_ui_prepare.inl"
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
#include "weapon_hammerburst_feed_history.inl"
bool hand_sample(hand::Frame const& f,gears_weapon::HammerburstLatchSample& out) noexcept {
    P left{};if(!local_hand(f,left))return false;
    out={left.position,f.controllers.sequence,std::uint64_t(f.controllers.sync_ns/1000000),f.press_token,
        f.left_tracked!=0,f.left_grip_known&&f.left_grip_down};
    latch_history::capture(f,out);return true;
}
bool move_magazine(hand::Frame const& f) noexcept {
    feed_move_gate=1;
    gears_rig_tracking::Frame rig{};
    if(!(nativelancerfollow::prepared_for(f,rig)||nativeequipment_rig::acquire(f,rig).status==nativeequipment_rig::Status::ready)||
       !(rig.world_pose_mask&gears_rig_tracking::left_grip_pose)||!gears_vr::stereo_pose_valid(rig.world_grip[0]))return false;
    const auto& left=rig.world_grip[0];
    const auto position=add(left.position,scale(gears_weapon::rotate(left.orientation,feed.item_in_left_grip.position),rig.units_per_metre));
    const auto orientation=gears_vr::multiply(left.orientation,feed.item_in_left_grip.orientation);
    // The mechanical b_HB_Mag and independent Hammerburst_Magazine use
    // different native geometry frames. Convert only this display target;
    // physical item and mechanism poses remain unchanged.
    if(!visual::active||!visual::alive()||!hand::same(visual::saved.weapon,f.weapon))return false;
    std::array<float,16> static_raw{};P static_pose{};double static_size{};
    if(!copy(static_raw.data(),visual::saved.component.address+0xa0,sizeof static_raw)||
       !matrix(static_raw,static_pose,static_size))return false;
    const P mechanical{position,orientation};
    P display{};
    if(!gears_weapon_hammerburst_owner_magazine::static_world_from_bone(mechanical,static_size,display))return false;
    feed_move_gate=2;
    const bool moved=visual::move_target(f,{display.position,gears_vr::ue_angles(gears_vr::rotation_matrix(display.orientation))},left.position);
    if(moved)feed_move_gate=0;return moved;
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
bool refresh_opened_contact(hand::Frame const& f) noexcept {
    if(opened_contact_current(f))return true;
    opened_contact={};P grip{};
    if(stage!=Stage::opened||!eligible(f)||!animation_owned(f)||!geometry_current(f))return false;
    std::array<P,3> bones{};vrequip::MeshPoseProbe mesh{};P gun{},physical{};double gun_size{};
    if(!parts_local(f,bones,&mesh)||!local_hand(f,physical)||
       mesh.mesh!=nativelancerfollow::bound_key.component.address||mesh.asset!=nativelancerfollow::bound_key.asset.address)return false;
    std::array<float,16> raw{};std::memcpy(raw.data(),mesh.local_to_world,sizeof raw);
    if(!matrix(raw,gun,gun_size))return false;
    // MagazineMesh is a different original component from the gun's skinned
    // Mesh. Authenticate its own asset/lifetime; it grants no display write.
    const auto component=playground::pointer(f.weapon.address,L"MagazineMesh");
    const auto asset=playground::pointer(component,L"StaticMesh");
    hand::ObjectIdentity component_id{},asset_id{};
    if(!playground::instance(component,L"StaticMeshComponent")||!playground::instance(asset,L"StaticMesh")||
       !playground::named(asset,L"Hammerburst_Magazine")||
       !nativeequipment::identify(nullptr,component,component_id)||!nativeequipment::identify(nullptr,asset,asset_id))return false;
    std::array<float,16> static_raw{},after{};P static_pose{};double static_size{};
    if(!copy(static_raw.data(),component+0xa0,sizeof static_raw))return false;
    gears_committed_interaction::Frame actual{};
    if(!lancer_grasp::hammerburst_magazine_interaction(f,actual)||
       std::abs(actual.right.units_per_metre-nativelancerfollow::bound_units)>1e-6)return false;
    LatentComponentScale latent_scale{};bool latent=false;
    if(!matrix(static_raw,static_pose,static_size)){
        if(!gears_hammerburst_component_scale::uninitialized(static_raw)||
           !latent_scale.prepare(component_id,actual.model_scale,static_size))return false;
        latent=true; // Geometry witness only; not an actual StaticMesh pose.
    }
    if(!gears_weapon_hammerburst_handoff::magazine_contact_in_weapon(bones[2],nativelancerfollow::bound_units,
           static_size/gun_size,actual.model_scale/gun_size,grip))return false;
    hand::ObjectIdentity after_component{},after_asset{};vrequip::MeshPoseProbe mesh_after{};
    if(!copy(after.data(),component+0xa0,sizeof after)||after!=static_raw||
       playground::pointer(f.weapon.address,L"MagazineMesh")!=component||playground::pointer(component,L"StaticMesh")!=asset||
       !nativeequipment::identify(nullptr,component,after_component)||!hand::same(component_id,after_component)||
       !nativeequipment::identify(nullptr,asset,after_asset)||!hand::same(asset_id,after_asset)||
       !vrequip::mesh_pose_probe(f.weapon.address,mesh_after)||mesh_after.mesh!=mesh.mesh||mesh_after.asset!=mesh.asset||
       std::memcmp(mesh.local_to_world,mesh_after.local_to_world,sizeof mesh.local_to_world)||!nativeequipment::current(nullptr,f))return false;
    opened_contact.identity=inv::identity(f);opened_contact.key=nativelancerfollow::bound_key;
    opened_contact.component=component_id;opened_contact.asset=asset_id;opened_contact.grip=grip;
    opened_contact.gun_scale=gun_size;opened_contact.static_scale=static_size;
    opened_contact.body_scale=actual.model_scale;opened_contact.units=nativelancerfollow::bound_units;
    if(latent&&!latent_scale.current(component_id,actual.model_scale,static_size))return false;
    opened_contact.latent=latent;opened_contact.latent_scale=latent_scale;
    opened_contact.angle=angle;opened_contact.known=true;
    return true;
}
void observe_original_extraction(hand::Frame const& f,std::uint64_t item,
    gears_weapon::FeedSample const& sample,std::int64_t gap,double depth,double lateral,double aligned) noexcept {
    const bool history_used=gap>feed.maximum_sample_gap_ns&&feed_history::advance_extraction(f,sample);
    const bool accepted=history_used||extraction.observe(sample,feed); // Original fallback exactly once.
    if(gap>feed.maximum_sample_gap_ns&&log_file){static unsigned history_rows{};if(history_rows++<16){
        fwprintf(log_file,L"hammerburst_extraction_history frame=%llu sequence=%llu previous_sequence=%llu used=%u gate=%u points=%u gap_ns=%lld complete=%u action=0\n",
            f.key.frame,f.controllers.sequence,feed_history::prior.sequence,unsigned(history_used),feed_history::gate,
            feed_history::point_count,gap,unsigned(extraction.complete()));fflush(log_file);}}
    static unsigned rows{};
    if(log_file&&rows<32){++rows;
        fwprintf(log_file,L"hammerburst_extraction_observed frame=%llu sequence=%llu item=%llu accepted=%u complete=%u gap_ns=%lld depth=%.9g lateral=%.9g aligned=%.9g sync_ns=%lld\n",
            f.key.frame,f.controllers.sequence,item,unsigned(accepted),unsigned(extraction.complete()),gap,
            depth,lateral,aligned,f.controllers.sync_ns);fflush(log_file);
    }
}
gears_weapon::FeedResult insertion_step(hand::Frame const& f,std::uint64_t item) noexcept {
    P left{};if(!local_hand(f,left)){insertion.reset();feed_history::clear();observe_feed_rejection(f,1);return gears_weapon::FeedResult::rejected;}
    const auto sample_gap_ns=feed_ns?f.controllers.sync_ns-feed_ns:0;
    const bool first_item_sample=feed_item!=item||!feed_ns;
    if(first_item_sample){
        if(!insertion.arm(f.weapon.address,item,f.world.address,f.stereo.space_epoch))return gears_weapon::FeedResult::rejected;
        feed_sequence=0;
    }
    if(f.controllers.sequence<=feed_sequence||f.controllers.sync_ns<=feed_ns){observe_feed_rejection(f,2);return gears_weapon::FeedResult::rejected;}
    gears_weapon::FeedSample sample{f.weapon.address,item,f.world.address,f.stereo.space_epoch,f.controllers.sequence,
        f.controllers.sync_ns,{{0,0,0},{0,0,0,1}},left,f.left_tracked!=0,true};
    if(stage==Stage::new_magazine||stage==Stage::old_magazine)feed_history::capture(f,left);
    gears_weapon::FeedResult result=gears_weapon::FeedResult::rejected;
    const bool gap=sample_gap_ns>feed.maximum_sample_gap_ns;
    const bool used_history=!first_item_sample&&gap&&feed_history::advance(f,sample,result);
    const bool rearmed=first_item_sample||(gap&&!used_history);
    if(!first_item_sample&&gap&&!used_history){
        if(!insertion.arm(f.weapon.address,item,f.world.address,f.stereo.space_epoch))return gears_weapon::FeedResult::rejected;
    }
    if(stage==Stage::new_magazine&&gap&&log_file){static unsigned rows{};if(rows++<16){
        fwprintf(log_file,L"hammerburst_feed_history gate=%u used=%u points=%u frame=%llu sequence=%llu gap_ns=%lld result=%u\n",
            feed_history::gate,unsigned(used_history),feed_history::point_count,f.key.frame,f.controllers.sequence,sample_gap_ns,unsigned(result));fflush(log_file);
    }}
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
    if(stage==Stage::old_magazine&&!extraction.complete())observe_original_extraction(f,item,sample,sample_gap_ns,depth,lateral,aligned);
    if(!used_history)result=insertion.observe(sample,feed);
    if(stage==Stage::new_magazine||stage==Stage::old_magazine)feed_history::remember(sample);else feed_history::clear();
    observe_feed_result(f,item,result,sample_gap_ns,rearmed,depth,lateral,aligned);return result;
}
bool expired_before_latch_dispatch(hand::Frame const& f,std::int64_t begin,std::int64_t end) noexcept {
    if(!anim::ownership.active||!anim::ownership.stopped||anim::ownership.seek_pending||
       anim::ownership.serial!=animation_serial()||!(anim::ownership.inventory==opening.identity)||
       begin<=0||end<begin||end<f.now_ns)return false;
    const auto expired=[&](std::int64_t stamp) noexcept {return stamp>0&&end>=stamp&&end-stamp>100000000;};
    return expired(f.controllers.received_ns)||expired(f.stereo.received_ns);
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
        if(!animation_owned(f))return;
        if(!f.left_grip_known){magazine_handoff.reset();opened_contact={};held={};released=false;return;}
        if(!magazine_handoff.active)magazine_handoff.opened(f.press_token);
        magazine_handoff.observe_up(f.left_grip_known,f.left_grip_down);
        // Native/part reads belong outside the broker prepare/commit callback.
        // Cached data is weapon-local geometry, never a hand sample or permit.
        refresh_opened_contact(f);
        // Bounded GT-only readiness receipt. Never runs in Native callbacks,
        // supplies no geometry/edge/grant, and does not query Native again.
        P observed_hand{};
        const bool touching=opened_contact.known&&local_hand(f,observed_hand)&&
            gears_weapon_hammerburst_handoff::in_contact(observed_hand,opened_contact.grip,opened_contact.gun_scale);
        const unsigned readiness=unsigned(magazine_handoff.active)|(unsigned(magazine_handoff.saw_up)<<1)|
            (unsigned(opened_contact.known)<<2)|(unsigned(touching)<<3)|(unsigned(f.left_grip_known)<<4)|
            (unsigned(f.left_grip_down)<<5)|(unsigned(decision.new_grant)<<6);
        static unsigned receipt_rows{},previous_readiness=~0u;static std::uint64_t previous_token{};
        if(log_file&&receipt_rows<8&&(readiness!=previous_readiness||f.press_token!=previous_token)){
            fwprintf(log_file,L"hammerburst_handoff_readiness frame=%llu sequence=%llu token=%llu lifecycle=%llu flags=%u decision=%u new_grant=%u cache_geometry_only=1 gt_only=1\n",
                f.key.frame,f.controllers.sequence,f.press_token,f.key.lifecycle,readiness,unsigned(decision.status),unsigned(decision.new_grant));
            fflush(log_file);++receipt_rows;previous_readiness=readiness;previous_token=f.press_token;
        }
        if(!f.left_grip_down){released=true;held={};return;}
        if(!granted(f,decision,hand::LeaseKind::magazine))return;
        P contact{};
        if(!magazine_handoff.can_propose()||!opened_magazine_contact(f,contact)||
           !magazine_handoff.admit(f.left_grip_known,f.left_grip_down,f.press_token,
               granted(f,decision,hand::LeaseKind::magazine,true),true))return;
        held=decision.lease;released=false;
        if(!prepare_feed(f)){
            if(feed_failure==3||open_pose_reads>=8)cleanup(__LINE__);
            return;
        }
        if(!visual::begin(f,visual::Profile::hammerburst)){cleanup(__LINE__);return;}
        if(!insertion.arm(f.weapon.address,old_item,f.world.address,f.stereo.space_epoch)){cleanup(__LINE__);return;}
        extraction.reset();stage=Stage::old_magazine;remember_magazine(f,decision);insertion_step(f,old_item);return;
    }
    if(!f.left_grip_known){magazine_pin.reset();return;}
    if(!f.left_grip_down){
        magazine_pin.reset();released=true;held={};stroke.reset();resume_grant_ready=false;
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
            if(stroke.begin(geometry.latch,sample,angle)){
                held=decision.lease;released=false;latch_history::previous=latch_history::sampled;
                // Opt-in simulator observation of an already-established baseline.
                // It changes no grant, state, source timestamp or motion outcome.
                static const bool observe=[](){wchar_t value[4]{};return
                    GetEnvironmentVariableW(L"GEARSVR_TEST_HB_LATCH_READY",value,4)==1&&value[0]==L'1';}();
                static unsigned ready_rows{};
                if(observe&&f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated&&log_file&&ready_rows<8){
                    ++ready_rows;
                    fwprintf(log_file,L"hammerburst_latch_ready serial=%llu stage=%u frame=%llu sequence=%llu token=%llu angle=%.9g baseline_ms=%llu\n",
                        serial,unsigned(stage),f.key.frame,f.controllers.sequence,f.press_token,angle,stroke.last_observation_ms());
                    fflush(log_file);
                }
            }
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
            if(std::abs(stroke.angle()-angle)<=1e-7)return;
            // A previous accepted segment reached this target but its seek
            // was never dispatched. This current stationary same-grab frame
            // may retry the normal fresh seek; no new motion is inferred.
            moved=stroke.observed_motion();
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
            const auto ended=weaponhand::clock_ns(nullptr);last_latch_seek_elapsed_ns=ended-began;
            if(!sought){
                const bool wait=expired_before_latch_dispatch(f,began,ended);
                static unsigned rows{};
                if(log_file&&rows<16){++rows;
                    fwprintf(log_file,L"hammerburst_latch_seek_unavailable frame=%llu sequence=%llu stage=%u wait=%u seek_pending=%u previous_angle=%.9g wanted_angle=%.9g begin_ns=%lld end_ns=%lld sync_ns=%lld no_dispatch=%u validated_path_kept=%u\n",
                        f.key.frame,f.controllers.sequence,unsigned(stage),unsigned(wait),unsigned(anim::ownership.seek_pending),
                        angle,stroke.angle(),began,ended,f.controllers.sync_ns,unsigned(wait),unsigned(wait));fflush(log_file);}
                if(wait){
                    // Keep ONLY the validated pure path/sample. `angle` and
                    // stage remain at their actual prior committed values.
                    // Next frame still needs the original grant/context,
                    // original history continuity and fresh owned animation seek.
                    return;
                }
                cleanup(__LINE__);return;
            }
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
        held=decision.lease;released=false;stage=Stage::new_magazine;remember_magazine(f,decision);insertion_step(f,new_item);return;
    }
    if(stage!=Stage::old_magazine&&stage!=Stage::new_magazine)return;
    const auto kind=stage==Stage::old_magazine?hand::LeaseKind::magazine:hand::LeaseKind::ammo;
    if(stage==Stage::old_magazine&&held.id!=f.press_token){
        if(!released||!granted(f,decision,kind,true))return;
        if(!insertion.arm(f.weapon.address,old_item,f.world.address,f.stereo.space_epoch)){cleanup(__LINE__);return;}
        held=decision.lease;released=false;remember_magazine(f,decision);insertion_step(f,old_item);return;
    }
    const bool part_granted=granted(f,decision,kind);
    if(part_granted)remember_magazine(f,decision);else magazine_pin.reset();
    if(!part_granted||!move_magazine(f)){observe_move_rejection(f,decision,part_granted);return;}
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
    magazine_pin.reset();stage=Stage::closing;held={};released=false;stroke.reset();
}
// A missing/expired render publication is not a physical release. Preserve
// only this exact held new magazine or observed latch for the next independent fresh
// Native prepare. No Frame, action, clock, grant or motion is produced here.
// Existing insertion/history continuity checks still govern the next sample.
void unavailable_frame(uptr pc,hand::FrameReadObservation const& failure) noexcept {
    if(stage!=Stage::idle&&failure.failure!=hand::FrameReadFailure::none&&log_file){
        static unsigned rows{};
        if(rows<8){++rows;fwprintf(log_file,L"hammerburst_frame_unavailable failure=%u game_difference=%u frame=%llu sequence=%llu press_token=%llu input_age_ns=%lld left_down=%u frame_difference=%u game_flags=%u candidate_flags=%u candidate_decision=%u candidate_lease=%u candidate_distance_m=%.9g commit_input_age_ns=%lld since_commit_check_ns=%lld stage=%u held_kind=%u held_id=%llu client_phase=%u client_armed=%u disarm_reason=%u\n",
            unsigned(failure.failure),failure.game_difference,failure.frame,failure.sequence,failure.press_token,
            failure.input_age_ns,failure.left_grip_down,failure.frame_difference,failure.game_flags,
            failure.candidate.flags,failure.candidate.decision,failure.candidate.lease,failure.candidate.support_distance_metres,
            failure.commit_check_input_age_ns,failure.since_commit_check_ns,unsigned(stage),unsigned(held.kind),held.id,
            unsigned(weaponhand::client.phase()),unsigned(weaponhand::client.armed()),weaponhand::client.last_disarm_reason());fflush(log_file);}
    }
    if(retain_magazine(pc,failure)){++magazine_pin_kept;return;}
    const bool pose_gap=failure.failure==hand::FrameReadFailure::publication||
        failure.failure==hand::FrameReadFailure::stale_before;
    const bool same_lifecycle=hand::lease_valid(held,opening.identity.lifecycle);
    // Only an already observed actual latch grab, never acquisition/hover.
    // The next original prepare independently checks current input/token,
    // tracking, identities and mount before granting any fresh Frame/action.
    const bool held_latch=(stage==Stage::opening||stage==Stage::closing)&&
        !released&&stroke.active()&&held.kind==hand::LeaseKind::magazine&&same_lifecycle;
    if(pose_gap&&held_latch){
        static unsigned rows{};
        if(log_file&&rows<8){++rows;
            fwprintf(log_file,L"hammerburst_pose_gap_incumbent failure=%u frame=%llu sequence=%llu item=%llu action=0 stage=%u latch=%u\n",
                unsigned(failure.failure),failure.frame,failure.sequence,held.id,unsigned(stage),unsigned(held_latch));fflush(log_file);}
        return;
    }
    magazine_pin.reset();held={};stroke.reset();insertion.reset();extraction.reset();released=false;
}
bool tick(uptr pc) noexcept {
    // A parked/restored hold never inherits an old part-transition witness.
    if(stage!=Stage::opened){magazine_handoff.reset();opened_contact={};}
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
            if(!lancerreload_state::recover_serial(serial)||!restore_animation()){cleanup(__LINE__);return true;}
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
    hand::Frame f{};hand::GripGameState game{};hand::Decision decision{};hand::FrameReadObservation frame_read{};
    if(!weaponhand::client.copy_frame_on_gt(f,game,decision,&frame_read)||f.controller.address!=pc||
       !game.known||!game.main_hand_held||game.busy||game.animation_owns_hands){
        if(stage!=Stage::idle){unavailable_frame(pc,frame_read);
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
            if(log_file&&rows<8){fwprintf(log_file,L"hammerburst_identity_lost difference=%u frame=%llu sequence=%llu stage=%u old_hold=%llu now_hold=%llu old_lifecycle=%llu now_lifecycle=%llu old_world_generation=%llu now_world_generation=%llu old_input=%llu now_input=%llu old_space=%llu now_space=%llu client_phase=%u client_armed=%u disarm_reason=%u\n",
                difference,f.key.frame,f.controllers.sequence,unsigned(stage),old.hold_generation,now.hold_generation,
                old.lifecycle,now.lifecycle,old.world_generation,now.world_generation,old.input,now.input,old.space,now.space,
                unsigned(weaponhand::client.phase()),unsigned(weaponhand::client.armed()),weaponhand::client.last_disarm_reason());
                fflush(log_file);++rows;}
            cleanup(__LINE__);return true;
        }
    }
    if(stage!=Stage::idle&&serial&&!native_exited){
        if(ui_pending){
            if(!apply_pending_ui(f)){cleanup(__LINE__);return true;}
            return true;
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
        magazine_handoff.reset();
        opened_contact={};
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
bool pending_work() noexcept {return park_control::pending_resources();}
// Service an existing old owner's completion/restoration only. Its pending
// result must not select or block another weapon's current candidate.
void service_unselected(uptr pc,uptr selected) noexcept {
    if(stage!=Stage::idle&&opening.identity.weapon.address!=selected)tick(pc);
}
} // namespace hammerburst_runtime
