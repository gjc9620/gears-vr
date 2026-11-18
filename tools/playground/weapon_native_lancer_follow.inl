// Held Lancer's existing mesh follows the authenticated Native right grip.
// The component lease is restored on the next owning game-thread Tick before
// the original PlayerTick runs. No tracking sample or game object is borrowed
// across that original action; only the fixed grip-to-mesh mount survives.
#include "../vr-common/hand_contact_equivalence.h"
#include "../vr-common/weapon_boomshot_direction.h"
#include "../vr-common/weapon_boomshot_owner_grabposes.h"
#include "../vr-common/weapon_lancer_authored_support.h"
#include "../vr-common/weapon_two_hand_hold.h"
#include "../vr-common/weapon_lancer_foregrip.h"
#include "../vr-common/weapon_hammerburst_foregrip.h"
#include "../vr-common/weapon_longshot_foregrip.h"
#include "../vr-common/weapon_lancer_wrist_mount.h"
#include "../vr-common/weapon_lancer_sight_zero.h"
#include "../vr-common/weapon_frag_grasp.h"
#include "../vr-common/weapon_smoke_grasp.h"
#include "../vr-common/weapon_boltok_main_grip.h"
#include "../vr-common/weapon_boltok_aim_grip.h"
#include "weapon_boltok_main_mount.inl"
namespace unified_weapon_fire {
bool held_socket_basis(gears_weapon_hand::Frame const&,uptr,
    std::array<float,16> const&,gears_vr::Matrix&) noexcept;
}
namespace nativelancerfollow {
namespace hand=gears_weapon_hand;
namespace rig=gears_rig_tracking;
using Value=gears_weapon::WeaponMeshComponentValue;
using Bytes=gears_weapon::MeshComponentBytes12;
using Absolute=gears_weapon::MeshComponentAbsolute;
struct Key {
    hand::ObjectIdentity weapon{},component{},asset{};
    std::uint64_t hold_generation{};
    bool operator==(Key const& other) const noexcept {
        return hand::same(weapon,other.weapon)&&hand::same(component,other.component)&&
            hand::same(asset,other.asset)&&hold_generation==other.hold_generation;
    }
};
using State=gears_weapon::WeaponMeshComponentState<Key>;
gears_weapon::WeaponMeshComponentLease<Key> lease;
std::atomic<bool> visible{};
bool faulted{};
Key bound_key{};bool bound_key_known{},bound_is_lancer{},bound_is_frag{},bound_is_smoke{},bound_is_hammerburst{},bound_is_longshot{},bound_is_boltok{},bound_is_torque_bow{},bound_is_hod{},bound_is_boomshot{};
std::uint64_t bound_session{},bound_space{},bound_input{},bound_reference{},bound_world{};
double bound_units{};
gears_weapon::WeaponMeshFollow mount;
bool bind_boomshot_main(gears_vr::Viewpoint const& right,std::array<float,16> const& observed_mesh,double units) noexcept {
    const auto scale=std::hypot(std::hypot(double(observed_mesh[0]),double(observed_mesh[1])),double(observed_mesh[2]));
    gears_vr::Quaternion q{};gears_vr::Pose gun{};
    if(!gears_vr::rig_tracking::detail::quaternion(gears_vr::ue_matrix(right.rotation),q)||
       !gears_boomshot_owner_grabposes::main_gun_from_grip({right.position,q},scale,units,1.,gun))return false;
    const auto basis=gears_vr::rotation_matrix(gun.orientation);std::array<float,16> captured{};
    for(unsigned axis=0;axis<3;++axis)for(unsigned world=0;world<3;++world)captured[axis*4+world]=float(basis[world][axis]*scale);
    captured[12]=float(gun.position.x);captured[13]=float(gun.position.y);captured[14]=float(gun.position.z);captured[15]=1;
    return mount.bind(right,captured);
}
gears_weapon::WeaponMeshFollow editor_baseline;
std::uint64_t editor_mount_revision{};
bool refresh_editor_mount() noexcept {
    editor_mount_revision=0;
    if(!bound_is_lancer)return true;
    mount=editor_baseline;gears_pose_editor::Config edit{};
    if(!pose_editor_client::read(edit))return true;
    if(edit.enabled&&!editor_baseline.adjusted_contact(edit.palm,bound_units,mount))return false;
    editor_mount_revision=edit.revision;return true;
}
bool refresh_boltok_aim_mount(rig::Frame const& prepared) noexcept {
    auto const& input=prepared.controllers.hands[1];
    if(!(prepared.world_pose_mask&rig::right_grip_pose)||
       !gears_vr::controller_pose_usable(input.grip,input.grip_flags,input.grip_active)||
       !gears_vr::controller_pose_usable(input.aim,input.aim_flags,input.aim_active))return false;
    gears_vr::Pose aim{};
    if(!gears_vr::rig_tracking::pose(prepared.base_view,prepared.reference,input.aim,prepared.units_per_metre,aim))return false;
    const auto relative=gears_vr::normalized(gears_vr::multiply(
        gears_vr::inverse(prepared.world_grip[1].orientation),aim.orientation));
    const gears_vr::Pose delta{{},gears_vr::inverse(relative)};
    // Recompute only a current mathematical relation from the immutable
    // authored contact. No input, permission or previous world pose is cached.
    return editor_baseline.adjusted_contact(delta,prepared.units_per_metre,mount);
}
// Committed endpoint positions are actual palm centres, but their axes are
// evaluated Wrist axes. Every Lancer consumer reconstructing the held mesh
// uses this same conversion, including reload contact/initial magazine pose.
bool endpoint_matrix(gears_vr::Pose const& palm,std::array<float,16>& out) noexcept {
    gears_vr::Pose grip{};
    if(!bound_key_known||!bound_is_lancer||!mount.bound()||
       !gears_weapon_lancer_wrist_mount::grip_from_palm_endpoint(palm,grip))return false;
    return mount.model_matrix({grip.position,gears_vr::ue_angles(gears_vr::rotation_matrix(grip.orientation))},out);
}
bool support_geometry_with_box(hand::Frame const& f,gears_vr::Pose& out,gears_weapon::SupportBox* box,
    gears_weapon::SupportRegion* region=nullptr) noexcept try {
    if(!bound_key_known||(!bound_is_lancer&&!bound_is_hammerburst&&!bound_is_longshot)||!mount.bound()||!hand::same(bound_key.weapon,f.weapon)||
       bound_key.hold_generation!=vrequip::model.hold_generation()||!std::isfinite(bound_units)||bound_units<10||bound_units>500||
       bound_session!=f.stereo.session||bound_space!=f.stereo.space_epoch||bound_input!=f.controllers.input_epoch||
       bound_reference!=f.reference_generation||bound_world!=f.key.world_generation)return false;
    hand::ObjectIdentity component{},asset{};
    if(playground::pointer(f.weapon.address,L"Mesh")!=bound_key.component.address||
       playground::pointer(bound_key.component.address,L"SkeletalMesh")!=bound_key.asset.address||
       !nativeequipment::identify(nullptr,bound_key.component.address,component)||!hand::same(component,bound_key.component)||
       !nativeequipment::identify(nullptr,bound_key.asset.address,asset)||!hand::same(asset,bound_key.asset))return false;
    std::array<float,16> gun{};if(!mount.model_matrix({{},{}},gun))return false;
    gears_vr::Matrix basis{};
    for(unsigned i=0;i<3;++i){const double n=std::hypot(std::hypot(double(gun[i*4]),double(gun[i*4+1])),double(gun[i*4+2]));
        if(!std::isfinite(n)||n<.1||n>10)return false;
        for(unsigned j=0;j<3;++j)basis[j][i]=gun[i*4+j]/n;}
    gears_vr::Quaternion gun_q{};if(!gears_vr::rig_tracking::detail::quaternion(basis,gun_q))return false;
    gears_hand_authored_mount::Mount left{};
    if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
        gears_hand_authored_mount::Side::left,1,100,left))return false;
    const auto wrist_in_gun=bound_is_longshot?gears_weapon_longshot_foregrip::wrist_in_mesh():
        bound_is_hammerburst?gears_weapon_hammerburst_foregrip::wrist_in_mesh():gears_weapon_lancer_foregrip::wrist_in_mesh();
    gears_vr::Vector p{};
    if(!(bound_is_longshot?gears_weapon_longshot_foregrip::grip_in_mesh(p):
         bound_is_hammerburst?gears_weapon_hammerburst_foregrip::grip_in_mesh(p):gears_weapon_lancer_foregrip::grip_in_mesh(p)))return false;
    gears_vr::Pose pose{};
    pose.position={(gun[12]+p.x*gun[0]+p.y*gun[4]+p.z*gun[8])/bound_units,
        (gun[13]+p.x*gun[1]+p.y*gun[5]+p.z*gun[9])/bound_units,
        (gun[14]+p.x*gun[2]+p.y*gun[6]+p.z*gun[10])/bound_units};
    pose.orientation=gears_vr::normalized(gears_vr::multiply(gears_vr::multiply(gun_q,wrist_in_gun),
        gears_vr::inverse(left.calibration.grip_to_wrist_rotation)));
    if(!gears_weapon_two_hand::geometry(pose))return false;
    if(box){*box=bound_is_longshot?gears_weapon_longshot_foregrip::contact_box(basis):gears_weapon::SupportBox{};
        if(box->enabled&&!box->valid())return false;}
    if(region){
        *region={};
        if(bound_is_lancer)if(const auto authored=gears_lancer_workbench_profile::support()){
            const gears_vr::Vector socket{pose.position.y,pose.position.z,-pose.position.x};
            if(!gears_lancer_authored_support::from_asset(*authored,gun,bound_units,socket,*region))return false;
            constexpr gears_vr::Matrix ue_to_xr{{{0,1,0},{0,0,1},{-1,0,0}}};
            const auto axes=gears_vr::multiply(gears_vr::multiply(ue_to_xr,
                gears_vr::rotation_matrix(pose.orientation)),gears_vr::transpose(ue_to_xr));
            gears_vr::Quaternion grip_rotation{};
            if(!gears_vr::rig_tracking::detail::quaternion(axes,grip_rotation))return false;
            region->target_grip_in_mount={socket,grip_rotation};
            const auto t=left.calibration.grip_to_wrist_metres;
            region->wrist_offset_in_grip={t.y,t.z,-t.x};
            region->snap_limits_enabled=gears_lancer_workbench_profile::support_snap_limits_enabled;
            if(!region->valid())return false;
        }
    }
    out=pose;return true;
}catch(...){return false;}
bool support_geometry(hand::Frame const& f,gears_vr::Pose& out) noexcept {
    if(playground::instance(f.weapon.address,L"Weap_Boomer_base"))return boomshot_runtime::support_geometry(f,out);
    return support_geometry_with_box(f,out,nullptr);
}
bool fill_support_mount(hand::Frame const& f,hand::GripGameState& game) noexcept {
    if(playground::instance(f.weapon.address,L"WEAP_HammerOfDawn"))return false;

    // Owner contract: neutral proximity fits the hand; only large-object
    // Grip, not an index-trigger pinch, may steer this secondary handguard.
    if(hand::index_pinch(f))return false;
    gears_vr::Pose g{};gears_weapon::SupportBox box{};gears_weapon::SupportRegion region{};
    if(!support_geometry_with_box(f,g,&box,&region))return false;
    game.mount={};game.mount.support_grip_in_weapon={g.position.y,g.position.z,-g.position.x};
    game.mount.support_length_tolerance_metres=.22;game.mount.maximum_support_correction_radians=.8;
    game.acquire_metres=gears_weapon::lancer_contact::support_radius;game.release_metres=.22;game.mount_asset=bound_key.asset;
    game.support_box=box;
    game.support_region=region;
    game.mount_revision=bound_key.hold_generation;game.mount_known=game.mount.valid();return game.mount_known;
}
// Geometry only, never a retained action grant or live pose. Returning to an
// existing weapon must not recapture the transient switch-animation offset.
struct SavedMount {
    Key key{};
    gears_weapon::WeaponMeshFollow geometry;
    std::uint64_t session{},space{},input{},reference{},world{},cookie{},native_start{};
    double units{};bool known{};
};
std::array<SavedMount,4> saved_mounts{};
unsigned next_saved_mount{};
std::uint64_t attempts{},commits{},restores{},rejections{},last_input_sequence{};
unsigned sample_logs{};
unsigned endpoint_logs{};
unsigned endpoint_stage_logs{};
unsigned last_apply_stage{},render_rejection_rows{};
unsigned reload_motion_logs{};
unsigned support_withdrawal_rows{};std::uint64_t support_withdrawal_frame{};
unsigned departing_logs{};
std::int64_t last_sample_ns{};
gears_vr::Pose last_sample_grip{};
bool last_sample_grip_known{};
struct Plan {
    hand::Frame source{};
    Key key{};
    gears_vr::Viewpoint target{};
    gears_vr::Pose right_grip{};
    std::uint64_t publication{};
    bool valid{};
    rig::Frame prepared{};
    bool applied{};
    std::uint64_t editor_revision{};
    std::uint64_t endpoint_serial{},endpoint_input{};
    gears_committed_hand::Frame endpoint_result{};
    bool render_owned{};
    bool departing{};
};
Plan plan;
unsigned muzzle_stage{};
std::uint64_t muzzle_reprepared{},muzzle_reprepared_tracking{};
void endpoint_stage(unsigned stage,Plan const& p) noexcept {
    last_apply_stage=stage;
    if(!log_file||!p.endpoint_serial||endpoint_stage_logs>=12)return;
    fwprintf(log_file,L"native_lancer_endpoint_apply stage=%u valid=%u applied=%u serial=%llu input=%llu frame=%llu grip=%.9g,%.9g,%.9g target=%.9g,%.9g,%.9g\n",
        stage,unsigned(p.valid),unsigned(p.applied),p.endpoint_serial,p.endpoint_input,p.source.key.frame,p.right_grip.position.x,p.right_grip.position.y,p.right_grip.position.z,
        p.target.position.x,p.target.position.y,p.target.position.z);fflush(log_file);++endpoint_stage_logs;
}
bool prepared_for(hand::Frame const& source,rig::Frame& out) noexcept {
    if(!plan.valid||plan.departing||!hand::same_frame(source,plan.source)||
       !nativeequipment::current(nullptr,source))return false;
    out=plan.prepared;return true;
}
struct DeferredReadback {
    Key key{};
    gears_vr::Vector target{};
    std::uint64_t frame{},input_sequence{};
    std::int64_t applied_ns{};
    bool valid{};
    hand::ObjectIdentity hand_mesh{};
    std::uint64_t endpoint_serial{};
    gears_vr::Vector selected_grip{};
};
DeferredReadback deferred_readback;
// Observation only at the existing pre-PlayerTick readback. These matrices
// are never input to a writer or a tracking permit. Retain the phase label:
// a coherent GT bracket is not proof of a later rendered eye image.
bool observed_palm(hand::ObjectIdentity const& mesh,gears_vr::Vector& out) noexcept {
    hand::ObjectIdentity before{},after{};Array bases{},again{};
    std::array<float,16> component{},wrist{},closing_component{},closing_wrist{};
    if(!nativeequipment::identify(nullptr,mesh.address,before)||!hand::same(before,mesh)||
       !copy(&bases,mesh.address+0x26c,sizeof bases)||bases.count!=115||bases.capacity<bases.count||bases.capacity>2000||
       bases.data>UINTPTR_MAX-115*64||!bases.data||
       !copy(component.data(),mesh.address+0xa0,64)||!copy(wrist.data(),bases.data+35*64,64))return false;
    gears_vr::Vector marker{};if(!gears_weapon_lancer_contact::palm_marker_in_wrist(marker))return false;
    const auto transform=[](std::array<float,16> const& m,gears_vr::Vector p) noexcept {
        return gears_vr::Vector{m[12]+m[0]*p.x+m[4]*p.y+m[8]*p.z,
            m[13]+m[1]*p.x+m[5]*p.y+m[9]*p.z,m[14]+m[2]*p.x+m[6]*p.y+m[10]*p.z};
    };
    const auto palm=transform(component,transform(wrist,marker));
    if(!gears_vr::finite(palm)||!copy(&again,mesh.address+0x26c,sizeof again)||
       bases.data!=again.data||bases.count!=again.count||bases.capacity!=again.capacity||
       !copy(closing_component.data(),mesh.address+0xa0,64)||component!=closing_component||
       !copy(closing_wrist.data(),bases.data+35*64,64)||wrist!=closing_wrist||
       !nativeequipment::identify(nullptr,mesh.address,after)||!hand::same(before,after))return false;
    out=palm;return true;
}
bool asset_rotation_origin(Key const& key,std::array<std::int32_t,3>& result) noexcept {
    hand::ObjectIdentity before{},after{};
    if(!nativeequipment::identify(nullptr,key.asset.address,before)||!hand::same(before,key.asset))return false;
    const auto property=playground::property(key.asset.address,L"RotOrigin");
    if(!property||!playground::named(get<uptr>(property+0x50),L"StructProperty")||
       get<unsigned>(property+0x70)!=1||get<unsigned>(property+0x74)!=12)return false;
    const auto structure=get<uptr>(property+0xd0);
    if(!structure||!playground::named(structure,L"Rotator")||get<unsigned>(structure+0x88)!=12)return false;
    const auto offset=get<unsigned>(property+0x94),size=get<unsigned>(key.asset.klass+0x88);
    if(offset>size||12>size-offset||key.asset.address>UINTPTR_MAX-offset-12)return false;
    std::array<std::int32_t,3> again{};
    return copy(result.data(),key.asset.address+offset,12)&&
        playground::property(key.asset.address,L"RotOrigin")==property&&get<unsigned>(property+0x94)==offset&&
        copy(again.data(),key.asset.address+offset,12)&&result==again&&
        nativeequipment::identify(nullptr,key.asset.address,after)&&hand::same(before,after);
}
bool sample_target(hand::Frame const& source,gears_vr::Vector& grip,gears_vr::Vector& mesh,
    std::int64_t& render_acquisition_ns,double& position_difference,double& orientation_difference) noexcept {
    if(!plan.valid||plan.departing||!hand::same_frame(source,plan.source))return false;
    grip=plan.right_grip.position;mesh=plan.target.position;
    render_acquisition_ns=plan.endpoint_serial?plan.endpoint_result.acquisition_begin_ns:0;
    const auto& raw=plan.prepared.world_grip[1];
    position_difference=std::hypot(std::hypot(grip.x-raw.position.x,grip.y-raw.position.y),grip.z-raw.position.z);
    const auto a=gears_vr::normalized(raw.orientation),b=gears_vr::normalized(plan.right_grip.orientation);
    const double sign=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0?-1.:1.;
    orientation_difference=(std::max)({std::abs(a.x-sign*b.x),std::abs(a.y-sign*b.y),std::abs(a.z-sign*b.z),std::abs(a.w-sign*b.w)});
    return true;
}

bool read_key(Key const& key,vrequip::MeshPoseProbe& pose) noexcept {
    if(!weaponhand::current_gt(nullptr)||GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire)||
       !hand::identity_valid(key.weapon)||!hand::identity_valid(key.component)||!hand::identity_valid(key.asset)||
       !key.hold_generation)return false;
    hand::ObjectIdentity weapon{},component{},asset{};
    return nativeequipment::identify(nullptr,key.weapon.address,weapon)&&hand::same(weapon,key.weapon)&&
        nativeequipment::identify(nullptr,key.component.address,component)&&hand::same(component,key.component)&&
        nativeequipment::identify(nullptr,key.asset.address,asset)&&hand::same(asset,key.asset)&&
        vrequip::mesh_pose_probe(key.weapon.address,pose)&&pose.mesh==key.component.address&&
        pose.asset==key.asset.address&&!pose.hidden;
}
bool value_of(Key const& key,State& out,vrequip::MeshPoseProbe* diagnostic=nullptr) noexcept {
    out={};vrequip::MeshPoseProbe pose{};
    if(!read_key(key,pose))return false;
    out.key=key;out.known=true;
    out.value.absolute={pose.absolute_translation,pose.absolute_rotation,pose.absolute_scale};
    std::memcpy(out.value.translation.data(),&pose.translation,12);
    std::memcpy(out.value.rotation.data(),&pose.rotation,12);
    if(diagnostic)*diagnostic=pose;
    return true;
}
bool bool_arg(playground::Args& call,const wchar_t* field,bool value) noexcept {
    const auto p=call.prop(field);
    if(!call.valid||!p||get<unsigned>(p+0x74)!=4||
       !playground::named(get<uptr>(p+0x50),L"BoolProperty"))return false;
    const auto offset=get<unsigned>(p+0x94),mask=get<unsigned>(p+0xd0);
    if(!mask||(mask&(mask-1))||offset>call.r.size||4>call.r.size-offset)return false;
    unsigned bits{};std::memcpy(&bits,call.r.params+offset,4);
    bits=value?bits|mask:bits&~mask;
    std::memcpy(call.r.params+offset,&bits,4);return true;
}
// Existing readback observes the first actual restore read, not an extra
// complete component/metadata pass before it. This stack-local output is
// never an input to the lease, its comparisons or any writer/permission.
struct RestoreReadback {
    vrequip::MeshPoseProbe pose{};
    std::int64_t read_ns{};
    bool known{};
};
struct Backend {
    Key key{};
    RestoreReadback* first_read{};
    bool read(State& out) noexcept {
        const bool capture=first_read&&!first_read->known;
        const bool known=value_of(key,out,capture?&first_read->pose:nullptr);
        if(known&&capture){first_read->read_ns=nativeequipment::clock_ns(nullptr);first_read->known=true;}
        return known;
    }
    bool set_absolute(Key const& wanted_key,Value const& expected,Absolute const& wanted) noexcept {
        State before{};if(!(wanted_key==key)||!read(before)||!gears_weapon::mesh_component_exact(before.value,expected))return false;
        playground::Args call(key.component.address,L"SetAbsolute");
        return bool_arg(call,L"NewAbsoluteTranslation",wanted[0])&&
            bool_arg(call,L"NewAbsoluteRotation",wanted[1])&&
            bool_arg(call,L"NewAbsoluteScale",wanted[2])&&call.run();
    }
    bool set_translation(Key const& wanted_key,Value const& expected,Bytes const& wanted) noexcept {
        State before{};if(!(wanted_key==key)||!read(before)||!gears_weapon::mesh_component_exact(before.value,expected))return false;
        playground::Args call(key.component.address,L"SetTranslation");const auto p=call.prop(L"NewTranslation");
        return call.valid&&p&&get<unsigned>(p+0x74)==12&&
            playground::named(get<uptr>(p+0x50),L"StructProperty")&&
            call.bytes(L"NewTranslation",wanted.data(),12)&&call.run();
    }
    bool set_rotation(Key const& wanted_key,Value const& expected,Bytes const& wanted) noexcept {
        State before{};if(!(wanted_key==key)||!read(before)||!gears_weapon::mesh_component_exact(before.value,expected))return false;
        playground::Args call(key.component.address,L"SetRotation");const auto p=call.prop(L"NewRotation");
        return call.valid&&p&&get<unsigned>(p+0x74)==12&&
            playground::named(get<uptr>(p+0x50),L"StructProperty")&&
            call.bytes(L"NewRotation",wanted.data(),12)&&call.run();
    }
};
bool active() noexcept {return visible.load(std::memory_order_acquire);}
void pre_original_tick(wchar_t const* observation_phase) noexcept {
    // A skipped or rejected PostTick may never recycle the previous plan.
    // Only a freshly prepared plan from this PlayerTick may feed R2 or display.
    plan={};
    if(!lease.dirty()){
        deferred_readback={};
        if(lease.has_snapshot())lease.reset();
        visible.store(false,std::memory_order_release);return;
    }
    // SetTranslation updates component fields immediately, but UE3 may not
    // refresh LocalToWorld until a later scene update. Read the selected
    // previous pose before restoring the lease, then compare it by its own
    // frame/sequence rather than treating the immediate matrix as final.
    const auto prior=deferred_readback;
    deferred_readback={};
    RestoreReadback restore_readback{};
    Backend backend{lease.key(),prior.valid&&prior.key==lease.key()?&restore_readback:nullptr};
    gears_committed_hand::Frame latest{};gears_vr::Vector palm{},contact{};
    // These two extra observations feed ONLY the diagnostic contact row.
    // Keep the actual restore/read/compare/identity transaction on EVERY tick;
    // do not spend every new input Frame on unrelated mesh/bone cross-checks.
    // Four early samples and one per128 valid readbacks retain bounded review.
    static std::uint64_t contact_observations{};
    const bool observe_contact=prior.valid&&log_file&&
        (contact_observations++<4||contact_observations%128==0);
    const bool endpoint_known=observe_contact&&handrig::read_endpoint(latest)&&
        hand::same(latest.weapon,prior.key.weapon)&&hand::same(latest.mesh,prior.hand_mesh)&&
        latest.hold_generation==prior.key.hold_generation;
    const bool palm_observed=observe_contact&&prior.endpoint_serial&&observed_palm(prior.hand_mesh,palm);
    const bool clean=lease.restore(backend);
    const bool delayed_known=restore_readback.known;
    const auto& delayed=restore_readback.pose;
    const bool palm_known=delayed_known&&palm_observed;
    gears_vr::Vector local_contact{};gears_vr::Matrix contact_basis{};
    const bool contact_known=observe_contact&&delayed_known&&prior.endpoint_serial&&gears_weapon_lancer_contact::authored(local_contact,contact_basis);
    if(contact_known){const auto& m=delayed.local_to_world;
        contact={m[12]+m[0]*local_contact.x+m[4]*local_contact.y+m[8]*local_contact.z,
            m[13]+m[1]*local_contact.x+m[5]*local_contact.y+m[9]*local_contact.z,
            m[14]+m[2]*local_contact.x+m[6]*local_contact.y+m[10]*local_contact.z};}
    const auto readback_ns=restore_readback.known?restore_readback.read_ns:nativeequipment::clock_ns(nullptr);
    if(prior.valid&&log_file){
        fwprintf(log_file,L"native_lancer_follow_readback frame=%llu source_sequence=%llu age_ns=%lld known=%u target_world=%.9g,%.9g,%.9g component_translation=%.9g,%.9g,%.9g gun_world=%.9g,%.9g,%.9g gun_basis=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g restored=%u component_rot=%d,%d,%d absolute=%u,%u,%u\n",
            prior.frame,prior.input_sequence,readback_ns-prior.applied_ns,unsigned(delayed_known),
            prior.target.x,prior.target.y,prior.target.z,
            double(delayed.translation.x),double(delayed.translation.y),double(delayed.translation.z),
            double(delayed.local_to_world[12]),double(delayed.local_to_world[13]),double(delayed.local_to_world[14]),
            double(delayed.local_to_world[0]),double(delayed.local_to_world[1]),double(delayed.local_to_world[2]),
            double(delayed.local_to_world[4]),double(delayed.local_to_world[5]),double(delayed.local_to_world[6]),
            double(delayed.local_to_world[8]),double(delayed.local_to_world[9]),double(delayed.local_to_world[10]),
            unsigned(clean),delayed.rotation.pitch,delayed.rotation.yaw,delayed.rotation.roll,
            unsigned(delayed.absolute_translation),unsigned(delayed.absolute_rotation),unsigned(delayed.absolute_scale));
        fflush(log_file);
        if(observe_contact){fwprintf(log_file,L"native_lancer_contact_readback phase=%ls frame=%llu source_sequence=%llu selected_serial=%llu latest_serial=%llu latest_known=%u palm_known=%u contact_known=%u selected_grip=%.9g,%.9g,%.9g latest_grip=%.9g,%.9g,%.9g observed_palm=%.9g,%.9g,%.9g observed_contact=%.9g,%.9g,%.9g\n",
            observation_phase,prior.frame,prior.input_sequence,prior.endpoint_serial,latest.serial,unsigned(endpoint_known),unsigned(palm_known),unsigned(contact_known),
            prior.selected_grip.x,prior.selected_grip.y,prior.selected_grip.z,latest.right_grip.position.x,latest.right_grip.position.y,latest.right_grip.position.z,
            palm.x,palm.y,palm.z,contact.x,contact.y,contact.z);fflush(log_file);}
    }
    if(clean){++restores;visible.store(false,std::memory_order_release);
        if(lease.faulted())faulted=true;
        lease.reset();
    }else{faulted=true;visible.store(true,std::memory_order_release);}
}
void tick(hand::Frame const& source,Phase phase) noexcept {
    plan={};
    ++attempts;
    // An original RefireCheckTimer can run between PlayerTicks while our
    // current visual component lease is still installed. Preparing an action
    // ray is read-only and must not require first replacing that visible gun
    // with the stock attachment. Permit only the SAME bound Lancer/hold; the
    // complete current rig/basis check below still precedes its mount solve.
    const bool read_owned_visual=phase==Phase::prepare_only&&(bound_is_lancer||bound_is_hammerburst||bound_is_boltok||bound_is_torque_bow||bound_is_hod||bound_is_boomshot)&&bound_key_known&&
        lease.dirty()&&lease.key()==bound_key&&hand::same(bound_key.weapon,source.weapon)&&
        bound_key.hold_generation==vrequip::model.hold_generation();
    if(faulted||(lease.dirty()&&!read_owned_visual)||!nativeequipment::current(nullptr,source)){
        ++rejections;return;
    }
    const auto held=vrequip::model.hold_generation();
    if(!held||!vrequip::model.held_for(source.world.address,source.pawn.address,source.weapon.address)){
        // Keep fixed geometry, NOT the old pose or action permit, until the
        // original switch has removed this still-attached departing Lancer.
        // Only a new successful C0 free-hand result may drive its mesh.
        if(bound_is_lancer&&bound_key_known&&
           vrequip::model.departing_for(source.world.address,source.pawn.address,bound_key.weapon.address))return;
        mount.reset();bound_key_known=false;return;
    }
    rig::Frame prepared{};const auto read=nativeequipment_rig::acquire(source,prepared);
    if(read.status!=nativeequipment_rig::Status::ready||!(prepared.world_pose_mask&rig::right_grip_pose)||
       !gears_vr::stereo_pose_valid(prepared.world_grip[1])){++rejections;return;}
    auto grip=prepared.world_grip[1];
    if(playground::instance(source.weapon.address,L"Weap_Boomer_base")){
        const auto& aim_input=prepared.controllers.hands[1];gears_vr::Pose aim_pose{};
        if(gears_vr::controller_pose_usable(aim_input.aim,aim_input.aim_flags,aim_input.aim_active)){
            if(!gears_vr::rig_tracking::pose(prepared.base_view,prepared.reference,aim_input.aim,prepared.units_per_metre,aim_pose)){++rejections;return;}
            grip.orientation=aim_pose.orientation;
        }
    }

    gears_vr::Viewpoint right{grip.position,gears_vr::ue_angles(gears_vr::rotation_matrix(grip.orientation))};
    const bool same_basis=bound_key_known&&hand::same(bound_key.weapon,source.weapon)&&
        bound_key.hold_generation==held&&bound_session==prepared.stereo.session&&
        bound_space==prepared.stereo.space_epoch&&bound_input==prepared.controllers.input_epoch&&
        bound_reference==prepared.reference_generation&&bound_world==prepared.world_generation;
    if(read_owned_visual&&!same_basis){++rejections;return;}
    Key key=bound_key;vrequip::MeshPoseProbe mesh{};
    if(!same_basis){
        lancermain_asset::Evidence asset{};
        const auto basic=lancermain_asset::read(source,asset,lancermain_asset::AssetScope::basic_hold);
        const bool is_frag=basic!=lancermain_asset::Status::ready&&
            lancermain_asset::read(source,asset,lancermain_asset::AssetScope::frag_hold)==lancermain_asset::Status::ready;
        const bool is_smoke=basic!=lancermain_asset::Status::ready&&!is_frag&&
            lancermain_asset::read(source,asset,lancermain_asset::AssetScope::smoke_hold)==lancermain_asset::Status::ready;
        if((basic!=lancermain_asset::Status::ready&&!is_frag&&!is_smoke)||
           !nativeequipment::attached_frame(source,asset.component.address)){
            ++rejections;return;
        }
        key={source.weapon,asset.component,asset.asset,held};
        State initial{};
        if(!value_of(key,initial,&mesh)){++rejections;return;}
        mount.reset();std::array<float,16> matrix{};
        std::memcpy(matrix.data(),mesh.local_to_world,sizeof(mesh.local_to_world));
        // Preserve the measured original attachment orientation for the new
        // basic-held assets. Only the certified Lancer uses its authored bore
        // orientation; no pistol/Hammerburst muzzle eligibility is implied.
        lancermain_asset::Evidence lancer{};
        const bool is_lancer=lancermain_asset::read(source,lancer)==lancermain_asset::Status::ready;
        bound_is_lancer=is_lancer;bound_is_frag=is_frag;bound_is_smoke=is_smoke;
        bound_is_hammerburst=basic==lancermain_asset::Status::ready&&
            playground::instance(source.weapon.address,L"Weap_LocustAssaultRifle");
        bound_is_longshot=basic==lancermain_asset::Status::ready&&
            playground::instance(source.weapon.address,L"Weap_SniperRifle");
        bound_is_boltok=basic==lancermain_asset::Status::ready&&
            playground::instance(source.weapon.address,L"Weap_LocustPistol");
        bound_is_torque_bow=basic==lancermain_asset::Status::ready&&
            playground::instance(source.weapon.address,L"Weap_Bow_base");
        bound_is_hod=basic==lancermain_asset::Status::ready&&
            playground::instance(source.weapon.address,L"WEAP_HammerOfDawn");
        bound_is_boomshot=basic==lancermain_asset::Status::ready&&
            playground::instance(source.weapon.address,L"Weap_Boomer_base");
        bool restored_geometry=false;
        for(const auto& saved:saved_mounts){
            if(saved.known&&hand::same(saved.key.weapon,key.weapon)&&
               hand::same(saved.key.component,key.component)&&hand::same(saved.key.asset,key.asset)&&
               saved.session==prepared.stereo.session&&saved.space==prepared.stereo.space_epoch&&
               saved.input==prepared.controllers.input_epoch&&saved.reference==prepared.reference_generation&&
               saved.world==prepared.world_generation&&saved.units==prepared.units_per_metre&&
               saved.cookie==source.key.registration_cookie&&saved.native_start==source.key.native_started_qpc){
                mount=saved.geometry;restored_geometry=true;break;
            }
        }
        gears_vr::Matrix measured_basis{},wrist_in_mesh{};
        gears_vr::Vector palm_in_mesh{};
        gears_weapon_frag_grasp::Profile frag{};gears_weapon_smoke_grasp::Profile smoke{};
        const double frag_scale=std::hypot(std::hypot(double(matrix[0]),double(matrix[1])),double(matrix[2]));
        // Never fit the Lancer contact to the tracked grip at pickup: the
        // original gun may still be in the switching animation. Preserve the
        // authored palm marker used by Native, independently of that distance.
        const bool is_hod=basic==lancermain_asset::Status::ready&&playground::instance(source.weapon.address,L"WEAP_HammerOfDawn");
        const bool is_gnasher=basic==lancermain_asset::Status::ready&&playground::instance(source.weapon.address,L"Weap_Shotgun");
        const bool bound=restored_geometry||(is_frag?
            (gears_weapon_frag_grasp::marcus115(gears_hand_authored_mount::marcus115_signature,1,prepared.units_per_metre,frag,frag_scale)&&
             mount.bind_mount(right,matrix,frag.palm_in_mesh,frag.wrist_in_mesh,
                 gears_vr::rotation_matrix(frag.hand.calibration.grip_to_wrist_rotation))):is_smoke?
            (gears_weapon_smoke_grasp::marcus115(gears_hand_authored_mount::marcus115_signature,1,prepared.units_per_metre,smoke,frag_scale)&&
             mount.bind_mount(right,matrix,smoke.palm_in_mesh,smoke.wrist_in_mesh,
                 gears_vr::rotation_matrix(smoke.hand.calibration.grip_to_wrist_rotation))):is_lancer?
            (gears_weapon_lancer_contact::authored(palm_in_mesh,wrist_in_mesh)&&
             unified_weapon_fire::held_socket_basis(source,key.component.address,matrix,measured_basis)&&
             mount.bind_mount(right,matrix,palm_in_mesh,gears_vr::transpose(measured_basis),
                 gears_vr::rotation_matrix(gears_weapon_lancer_wrist_mount::gun_pitch_down))):
            bound_is_boltok?
            boltok_main_mount::bind(source,right,matrix,prepared.units_per_metre,mount):
            bound_is_torque_bow?
            torquebow_main_mount::bind(source,right,matrix,prepared.units_per_metre,mount):
            is_gnasher?
            // Same source-derived raw grip used by the Gnasher recognizer;
            // no transient original hand-bone or switching-pose recapture.
            mount.bind_mount(right,matrix,gears_weapon_gnasher_hold::right_grip_mesh_raw.position,
                gears_vr::rotation_matrix(gears_weapon_gnasher_hold::right_grip_mesh_raw.orientation),
                gears_vr::Matrix{{{1,0,0},{0,1,0},{0,0,1}}}):
            playground::instance(source.weapon.address,L"Weap_Boomer_base")?
            bind_boomshot_main(right,matrix,prepared.units_per_metre):
            (unified_weapon_fire::held_socket_basis(source,key.component.address,matrix,measured_basis)&&
             mount.bind_oriented(right,matrix,is_hod?gears_weapon_hod_geometry::mesh_in_hand(measured_basis):measured_basis)));
        if(!bound){++rejections;return;}
        if(!restored_geometry){
            auto& saved=saved_mounts[next_saved_mount++%saved_mounts.size()];
            saved={key,mount,prepared.stereo.session,prepared.stereo.space_epoch,
                prepared.controllers.input_epoch,prepared.reference_generation,
                prepared.world_generation,source.key.registration_cookie,source.key.native_started_qpc,
                prepared.units_per_metre,true};
        }
        bound_key=key;bound_key_known=true;bound_session=prepared.stereo.session;
        bound_space=prepared.stereo.space_epoch;bound_input=prepared.controllers.input_epoch;
        bound_reference=prepared.reference_generation;bound_world=prepared.world_generation;
        bound_units=prepared.units_per_metre;
        editor_baseline=mount;
    }
    // Always derive from the saved authored baseline, never accumulate edits.
    // All subsequent contact, model_matrix and muzzle users share this mount.
    if(!refresh_editor_mount())return;
    if(bound_is_boltok&&!refresh_boltok_aim_mount(prepared)){++rejections;return;}
    // Frag is visual-only here. Its sole component writer consumes the actual
    // current C0 result below; a raw-controller PostPlayerTick never moves it
    // independently from the rendered reach-resolved hand or grants a shot.
    if(bound_is_frag||bound_is_smoke)return;
    gears_committed_hand::Frame endpoint{};
    // When a current reach-resolved hand result exists, both the shot and
    // visual mesh use that same palm. A shot must not REQUIRE a later render
    // result: the current authenticated controller remains the fallback when
    // no such result exists. prepare_only never acquires a component lease.
    if(bound_is_lancer){
        const bool received=handrig::read_endpoint(endpoint);const auto endpoint_now=nativeequipment::clock_ns(nullptr);
        const bool sim=source.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated;
        const bool matched=received&&gears_committed_hand::matches(endpoint,source,prepared,held,endpoint_now,sim);
        if(log_file&&endpoint_logs<8){
            fwprintf(log_file,L"native_lancer_endpoint received=%u matched=%u tracking_fresh_now=%u result_current=%u serial=%llu endpoint_input=%llu action_input=%llu endpoint_frame=%llu action_frame=%llu endpoint_hold=%llu action_hold=%llu endpoint_lifecycle=%llu action_lifecycle=%llu endpoint_binding=%llu tracking_binding=%llu endpoint_ref=%llu tracking_ref=%llu age_ns=%lld\n",
                unsigned(received),unsigned(matched),unsigned(received&&gears_committed_hand::fresh(endpoint,endpoint_now,sim)),
                unsigned(received&&gears_committed_hand::current_result(endpoint,endpoint_now,sim)),
                endpoint.serial,endpoint.controllers.sequence,source.controllers.sequence,endpoint.parent.frame,source.key.frame,
                endpoint.hold_generation,held,endpoint.parent.lifecycle,source.key.lifecycle,endpoint.binding_generation,prepared.binding_generation,
                endpoint.reference_generation,prepared.reference_generation,endpoint_now-endpoint.acquisition_begin_ns);
            fflush(log_file);++endpoint_logs;
        }
        if(!matched&&phase==Phase::post_original){
            ++rejections;return;
        }
        gears_vr::Pose endpoint_grip{};
        if(matched&&!gears_weapon_lancer_wrist_mount::grip_from_palm_endpoint(endpoint.right_grip,endpoint_grip))return;
        if(matched&&phase==Phase::prepare_only&&
           gears_hand_contact_equivalence::same_contact(grip,endpoint_grip,prepared.units_per_metre)){
            // Independently current tracking already describes this exact
            // reachable palm. Solve the action mount from THAT tracking pose,
            // as in the existing no-render-result fallback; do not make its
            // lifetime depend on an older, geometrically identical C0 result.
            // No render clock is renewed and no projected/unreachable palm
            // can use this equivalence path. Visual writes still require C0.
            endpoint={};
        }else if(matched){
            grip=endpoint_grip;
            right={grip.position,gears_vr::ue_angles(gears_vr::rotation_matrix(grip.orientation))};
        }else endpoint={};
    }
    if(!endpoint.serial&&(bound_is_lancer||bound_is_hammerburst||bound_is_longshot||playground::instance(source.weapon.address,L"Weap_Boomer_base"))&&(prepared.world_pose_mask&rig::left_grip_pose)){
        hand::GripSnapshot snapshot{};gears_vr::Pose geometry{};gears_weapon_two_hand::Solution supported{};
        const bool snapshot_known=weaponhand::client.snapshot_on_gt(snapshot);
        if(!snapshot_known&&source.left_grip_down&&log_file&&support_withdrawal_rows<8){
            const auto failure=weaponhand::client.withdrawal_on_gt();
            if(failure.frame&&failure.frame!=support_withdrawal_frame&&failure.press_token==source.press_token){
                support_withdrawal_frame=failure.frame;++support_withdrawal_rows;
                fwprintf(log_file,L"native_lancer_support_source_withdrawal frame=%llu previous_frame=%llu token=%llu reason=%u game_difference=%u age_ns=%lld held=%u\n",
                    source.key.frame,failure.frame,failure.press_token,unsigned(failure.failure),failure.game_difference,
                    failure.input_age_ns,failure.left_grip_down);fflush(log_file);}
        }
        if(snapshot_known&&snapshot.grip==hand::Grip::two&&
           snapshot.current(source.key,source.weapon,nativeequipment::clock_ns(nullptr))&&support_geometry(source,geometry)&&
           gears_weapon_two_hand::solve(grip,prepared.world_grip[0],geometry,bound_units,supported)){
            grip=supported.right;right={grip.position,gears_vr::ue_angles(gears_vr::rotation_matrix(grip.orientation))};
        }
    }
    gears_vr::Viewpoint target{};
    if(!mount.solve(right,target)||!nativeequipment::current(nullptr,source)){
        if(endpoint.serial&&log_file&&endpoint_stage_logs<12){fwprintf(log_file,L"native_lancer_endpoint_plan_failed serial=%llu grip=%.9g,%.9g,%.9g quat=%.9g,%.9g,%.9g,%.9g\n",endpoint.serial,grip.position.x,grip.position.y,grip.position.z,grip.orientation.x,grip.orientation.y,grip.orientation.z,grip.orientation.w);fflush(log_file);++endpoint_stage_logs;}
        ++rejections;return;
    }
    if(gnasher_runtime::correct_main(source,prepared,target)==gnasher_runtime::MainResult::rejected){
        ++rejections;return;
    }
    if(!gears_vr::finite(target.position)){
        ++rejections;return;
    }
    plan={source,key,target,grip,prepared.publication_sequence,true};
    plan.prepared=prepared;
    plan.editor_revision=editor_mount_revision;
    plan.endpoint_serial=endpoint.serial;plan.endpoint_input=endpoint.controllers.sequence;
    plan.endpoint_result=endpoint;
    endpoint_stage(9,plan);
    // The committed endpoint has its own original acquisition clock. Consume
    // it immediately on this same ordinary PostPlayerTick. Later callers see
    // applied=true and retain the existing at-most-once component lease.
    if(endpoint.serial&&phase==Phase::post_original)apply();
}
bool adjust_muzzle(hand::Frame const& source,lancermain_asset::Evidence const& asset,
    playground::Vector& origin,playground::Rotation& aim,gears_weapon_lancer_sight::Result* sight_zero=nullptr) noexcept {
    muzzle_stage=1;
    const auto current_plan=[&]() noexcept {return plan.valid&&!plan.departing&&hand::same_frame(source,plan.source)&&
       hand::same(asset.component,plan.key.component)&&hand::same(asset.asset,plan.key.asset)&&
       nativeequipment::current(nullptr,source);};
    if(!current_plan())return false;
    muzzle_stage=2;
    const auto current_endpoint=[&]() noexcept {
        if(!plan.endpoint_serial)return true;
        gears_committed_hand::Frame latest{};
        return handrig::read_endpoint(latest)&&latest.serial==plan.endpoint_serial&&
           gears_committed_hand::matches(plan.endpoint_result,source,plan.prepared,
               vrequip::model.hold_generation(),nativeequipment::clock_ns(nullptr),
               source.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated);
    };
    // GetSocketWorldLocationAndRotation may retire/replace the preceding C0
    // result. Do not use that revoked hand or drop a real held trigger solely
    // because the earlier render result changed during the getter. Rebuild
    // ONCE from this same authenticated action Frame: a new matching C0 palm,
    // or the already-supported current-controller prepare_only plan. All
    // original clocks, rig, world, hold and current-ray checks still apply.
    if(!current_endpoint()){
        tick(source,Phase::prepare_only);
        if(!current_plan()||!current_endpoint())return false;
        ++muzzle_reprepared;if(!plan.endpoint_serial)++muzzle_reprepared_tracking;
    }
    if(!plan.valid||plan.departing||!hand::same_frame(source,plan.source)||
       !hand::same(asset.component,plan.key.component)||!hand::same(asset.asset,plan.key.asset)||
       !nativeequipment::current(nullptr,source))return false;
    muzzle_stage=3;
    vrequip::MeshPoseProbe fresh{};
    if(!vrequip::mesh_pose_probe(source.weapon.address,fresh)||
       fresh.mesh!=asset.component.address||fresh.asset!=asset.asset.address||fresh.hidden)return false;
    muzzle_stage=4;
    const auto& m=fresh.local_to_world;
    gears_vr::Matrix old_basis{};
    for(int axis=0;axis<3;++axis){
        const double scale=std::hypot(std::hypot(double(m[axis*4]),double(m[axis*4+1])),double(m[axis*4+2]));
        if(!std::isfinite(scale)||scale<.1||scale>10.)return false;
        for(int world=0;world<3;++world)old_basis[world][axis]=double(m[axis*4+world])/scale;
    }
    const auto new_basis=gears_vr::ue_matrix(plan.target.rotation);
    muzzle_stage=5;
    const auto inverse=gears_vr::transpose(old_basis);
    const gears_vr::Vector from{double(origin.x)-m[12],double(origin.y)-m[13],double(origin.z)-m[14]};
    const auto moved=gears_vr::transform(new_basis,gears_vr::transform(inverse,from));
    const gears_vr::Vector result{plan.target.position.x+moved.x,plan.target.position.y+moved.y,
        plan.target.position.z+moved.z};
    constexpr double radians=3.14159265358979323846/32768.;
    const double pitch=double(aim.pitch)*radians,yaw=double(aim.yaw)*radians;
    const gears_vr::Vector forward{std::cos(pitch)*std::cos(yaw),std::cos(pitch)*std::sin(yaw),std::sin(pitch)};
    auto rotated=gears_vr::transform(new_basis,gears_vr::transform(inverse,forward));
    if(!gears_vr::finite(result)||!gears_vr::finite(rotated)||
       std::hypot(std::hypot(rotated.x,rotated.y),rotated.z)<.99)return false;
    if(bound_is_lancer){
        double scale[3]{};
        for(unsigned axis=0;axis<3;++axis)scale[axis]=std::hypot(std::hypot(double(m[axis*4]),double(m[axis*4+1])),double(m[axis*4+2]));
        if((std::max)({scale[0],scale[1],scale[2]})-(std::min)({scale[0],scale[1],scale[2]})>.001)return false;
        gears_weapon_lancer_sight::Result zero{};
        if(!gears_weapon_lancer_sight::solve(new_basis,plan.target.position,scale[0],plan.prepared.units_per_metre,
            result,rotated,zero))return false;
        rotated=zero.direction;
        if(sight_zero)*sight_zero=zero;
    }
    const playground::Vector corrected_origin{float(result.x),float(result.y),float(result.z)};
    const playground::Rotation corrected_aim{int(std::lround(std::atan2(rotated.z,
        std::hypot(rotated.x,rotated.y))/radians)),int(std::lround(std::atan2(rotated.y,rotated.x)/radians)),0};
    if(!std::isfinite(corrected_origin.x)||!std::isfinite(corrected_origin.y)||
       !std::isfinite(corrected_origin.z))return false;
    origin=corrected_origin;aim=corrected_aim;muzzle_stage=6;return true;
}
bool render_current(Plan const& p,unsigned* diagnostic=nullptr) noexcept {
    if(diagnostic)*diagnostic=0;
    const auto reject=[&](unsigned reason) noexcept {if(diagnostic)*diagnostic=reason;return false;};
    const auto& f=p.endpoint_result;
    const bool eligible=p.departing?
        (f.purpose==gears_committed_hand::Purpose::free_hand_visual&&!f.hold_generation&&
         vrequip::model.departing_for(f.world.address,f.pawn.address,p.key.weapon.address)):
        (((!bound_is_frag&&f.purpose==gears_committed_hand::Purpose::held_lancer)||
          f.purpose==gears_committed_hand::Purpose::observed_held_visual)&&
         vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
         vrequip::model.hold_generation()==f.hold_generation&&p.key.hold_generation==f.hold_generation);
    if(!p.render_owned||(!bound_is_lancer&&!bound_is_frag&&!bound_is_smoke&&!bound_is_hammerburst&&!bound_is_boltok&&!bound_is_torque_bow&&!bound_is_hod&&!bound_is_boomshot)||!bound_key_known||!(p.key==bound_key)||
       !enabled.load(std::memory_order_acquire)||!weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready()||
       !weaponhand::client.registration_alive(f.parent.registration_cookie,f.parent.native_started_qpc)||
       !eligible||
       (!p.departing&&!hand::same(p.key.weapon,f.weapon))||bound_session!=f.stereo.session||bound_space!=f.stereo.space_epoch||
       bound_input!=f.controllers.input_epoch||bound_reference!=f.reference_generation||bound_world!=f.parent.world_generation||
       bound_units!=f.units_per_metre)return reject(1);
    gears_committed_hand::Frame latest{};
    if(!handrig::read_endpoint(latest)||latest.serial!=f.serial)return reject(2);
    if(!gears_committed_hand::current_result(f,weaponhand::clock_ns(nullptr),f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated))return reject(3);
    for(auto const& identity:{f.world,f.controller,f.pawn,f.mesh,f.weapon}){
        hand::ObjectIdentity actual{};if(!nativeequipment::identify(nullptr,identity.address,actual)||!hand::same(actual,identity))return reject(4);}
    // Current live links, not a reconstructed action Frame. The identity-only
    // attachment reader grants no action permission; this writer is bound to
    // the actual Native C0 result and is never consumed by shooting/equipment.
    const bool attached=playground::pointer(f.controller.address,L"Pawn")==f.pawn.address&&
        playground::pointer(f.controller.address,L"WorldInfo")==f.world.address&&
        playground::pointer(f.pawn.address,L"Controller")==f.controller.address&&
        playground::pointer(f.pawn.address,L"Mesh")==f.mesh.address&&
        playground::pointer(f.pawn.address,L"Weapon")==f.weapon.address&&
        nativeequipment::read_attached_frame(p.source,p.key.component.address);
    return attached?true:reject(5);
}
void apply() noexcept {
    endpoint_stage(8,plan);
    if(!plan.valid||plan.applied)return;
    // Certified Lancer visual writes belong to the actual C0 result callback.
    // PostPlayerTick still prepares authoritative shots and binds geometry.
    if((bound_is_lancer||bound_is_frag||bound_is_smoke||bound_is_hammerburst||bound_is_boltok||bound_is_torque_bow||bound_is_hod||bound_is_boomshot)&&!plan.render_owned)return;
    const Plan wanted_plan=plan;
    const auto& source=wanted_plan.source;
    const auto& key=wanted_plan.key;
    const auto source_current=[&]() noexcept {
        return wanted_plan.render_owned?render_current(wanted_plan):nativeequipment::current(nullptr,source);
    };
    const auto endpoint_current=[&]() noexcept {
        if(wanted_plan.render_owned)return render_current(wanted_plan);
        if(!wanted_plan.endpoint_serial)return true;
        gears_committed_hand::Frame latest{};
        return handrig::read_endpoint(latest)&&latest.serial==wanted_plan.endpoint_serial&&
            gears_committed_hand::matches(wanted_plan.endpoint_result,source,wanted_plan.prepared,
                vrequip::model.hold_generation(),nativeequipment::clock_ns(nullptr),
                source.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated);
    };
    if(faulted||lease.dirty()||!source_current()){
        endpoint_stage(1,wanted_plan);
        ++rejections;return;
    }
    if(!(wanted_plan.render_owned?render_current(wanted_plan):nativeequipment::attached_frame(source,key.component.address))){
        endpoint_stage(2,wanted_plan);
        ++rejections;return;
    }
    const auto& target=wanted_plan.target;
    const playground::Vector position{float(target.position.x),float(target.position.y),float(target.position.z)};
    // LocalToWorld includes the SkeletalMesh asset's RotOrigin. Convert the
    // desired MODEL basis to the component control basis, as in the existing
    // component writer; writing model Euler angles directly double-applies it.
    std::array<std::int32_t,3> asset_origin{};gears_vr::Rotator control_rotation{};
    if(!asset_rotation_origin(key,asset_origin)||
       !gears_weapon::model_to_component_rotation(target.rotation,asset_origin,control_rotation)){
        endpoint_stage(3,wanted_plan);
        ++rejections;return;
    }
    const playground::Rotation rotation{gears_vr::to_rotator_units(control_rotation.pitch),
        gears_vr::to_rotator_units(control_rotation.yaw),gears_vr::to_rotator_units(control_rotation.roll)};
    const playground::Rotation model_rotation{gears_vr::to_rotator_units(target.rotation.pitch),
        gears_vr::to_rotator_units(target.rotation.yaw),gears_vr::to_rotator_units(target.rotation.roll)};
    if(!std::isfinite(position.x)||!std::isfinite(position.y)||!std::isfinite(position.z)){
        ++rejections;return;
    }
    Backend backend{key};
    if(!endpoint_current()){endpoint_stage(5,wanted_plan);++rejections;return;}
    const bool written=lease.acquire_from_current(key,backend,[&](Value const& current){
        Value wanted=current;wanted.absolute[0]=wanted.absolute[1]=true;
        std::memcpy(wanted.translation.data(),&position,12);
        std::memcpy(wanted.rotation.data(),&rotation,12);
        return wanted;
    });
    if(!written||!source_current()||!endpoint_current()){
        endpoint_stage(4,wanted_plan);
        if(lease.dirty())pre_original_tick();
        if(lease.faulted())faulted=true;
        ++rejections;return;
    }
    plan.applied=true;
    if(bound_is_lancer)pose_editor_client::receipt(plan.editor_revision,source.key.frame);
    endpoint_stage(0,wanted_plan);
    ++commits;visible.store(lease.dirty(),std::memory_order_release);
    if(wanted_plan.departing&&log_file&&departing_logs<4){
        ++departing_logs;
        fwprintf(log_file,L"native_departing_weapon_visual at_ns=%lld serial=%llu old_weapon=%llx current_weapon=%llx attached=1 hold_permission=0\n",
            nativeequipment::clock_ns(nullptr),wanted_plan.endpoint_serial,key.weapon.address,wanted_plan.endpoint_result.weapon.address);
        fflush(log_file);
    }
    // Bounded same-frame trajectory samples: keep a sparse time series and
    // always sample a substantial hand move/turn. Logging only the first few
    // frames missed every actual sweep in the previous private check.
    const auto observed_ns=nativeequipment::clock_ns(nullptr);
    const auto& grip=wanted_plan.right_grip;
    const gears_vr::Vector displacement=last_sample_grip_known?
        gears_vr::Vector{grip.position.x-last_sample_grip.position.x,
            grip.position.y-last_sample_grip.position.y,
            grip.position.z-last_sample_grip.position.z}:gears_vr::Vector{};
    const double turn=last_sample_grip_known?std::abs(
        grip.orientation.x*last_sample_grip.orientation.x+
        grip.orientation.y*last_sample_grip.orientation.y+
        grip.orientation.z*last_sample_grip.orientation.z+
        grip.orientation.w*last_sample_grip.orientation.w):1.;
    const bool moved=last_sample_grip_known&&
        std::hypot(std::hypot(displacement.x,displacement.y),displacement.z)>=5.;
    const bool rotated=last_sample_grip_known&&turn<.999;
    const bool interval=last_sample_ns<=0||observed_ns-last_sample_ns>=750000000;
    const bool reload_motion=(moved||rotated)&&lancerreload_runtime::needs_priority()&&reload_motion_logs<8;
    if(log_file&&(sample_logs<64||reload_motion)&&source.controllers.sequence!=last_input_sequence&&
       (sample_logs<4||moved||rotated||interval)){
        State actual{};vrequip::MeshPoseProbe pose{};
        const bool sampled=value_of(key,actual,&pose);
        fwprintf(log_file,L"native_lancer_follow frame=%llu publication=%llu source_sequence=%llu sync_ns=%lld observed_ns=%lld hold=%llu accepted=1 mesh_read=%u grip_world=%.9g,%.9g,%.9g grip_quat=%.9g,%.9g,%.9g,%.9g target_world=%.9g,%.9g,%.9g target_rot=%d,%d,%d asset_origin=%d,%d,%d component_translation=%.9g,%.9g,%.9g gun_world=%.9g,%.9g,%.9g gun_rot=%d,%d,%d dirty=%u\n",
            source.key.frame,wanted_plan.publication,source.controllers.sequence,source.controllers.sync_ns,
            observed_ns,key.hold_generation,unsigned(sampled),
            wanted_plan.right_grip.position.x,wanted_plan.right_grip.position.y,wanted_plan.right_grip.position.z,
            wanted_plan.right_grip.orientation.x,wanted_plan.right_grip.orientation.y,
            wanted_plan.right_grip.orientation.z,wanted_plan.right_grip.orientation.w,
            target.position.x,target.position.y,target.position.z,
            model_rotation.pitch,model_rotation.yaw,model_rotation.roll,
            asset_origin[0],asset_origin[1],asset_origin[2],
            double(pose.translation.x),double(pose.translation.y),double(pose.translation.z),
            double(pose.local_to_world[12]),double(pose.local_to_world[13]),double(pose.local_to_world[14]),
            pose.rotation.pitch,pose.rotation.yaw,pose.rotation.roll,lease.dirty_mask());
        fflush(log_file);if(sample_logs>=64)++reload_motion_logs;
        ++sample_logs;last_input_sequence=source.controllers.sequence;
        deferred_readback={key,target.position,source.key.frame,source.controllers.sequence,observed_ns,true};
        deferred_readback.hand_mesh=source.mesh;deferred_readback.endpoint_serial=wanted_plan.endpoint_serial;
        deferred_readback.selected_grip=wanted_plan.right_grip.position;
        last_sample_ns=observed_ns;last_sample_grip=grip;last_sample_grip_known=true;
    }
}
bool render_rejected(gears_committed_hand::Frame const& f,unsigned stage,unsigned detail=0) noexcept {
    // Failure-only extension of the existing limited endpoint receipts. It
    // performs no new game reads and is private-simulator-only. Startup
    // successes cannot exhaust it before the observed C transition.
    if(log_file&&render_rejection_rows<16&&(bound_is_lancer||bound_is_hammerburst||bound_is_boltok||bound_is_torque_bow||bound_is_hod||bound_is_boomshot)&&bound_key_known&&
       hand::same(bound_key.weapon,f.weapon)&&f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated){
        ++render_rejection_rows;const auto now=weaponhand::clock_ns(nullptr);
        fwprintf(log_file,L"native_lancer_render_rejected stage=%u detail=%u serial=%llu tick=%llu compose=%llu input=%llu sync_ns=%lld observed_ns=%lld age_ns=%lld purpose=%u dirty=%u faulted=%u bound_hold=%llu result_hold=%llu enabled=%u\n",
            stage,detail,f.serial,f.tick,f.compose,f.controllers.sequence,f.controllers.sync_ns,now,now-f.controllers.sync_ns,
            unsigned(f.purpose),unsigned(lease.dirty()),unsigned(faulted),bound_key.hold_generation,f.hold_generation,unsigned(enabled.load()));fflush(log_file);}
    return false;
}
bool on_render_result(gears_committed_hand::Frame const& f) noexcept {
    if(faulted||(!bound_is_lancer&&!bound_is_frag&&!bound_is_smoke&&!bound_is_hammerburst&&!bound_is_boltok&&!bound_is_torque_bow&&!bound_is_hod&&!bound_is_boomshot)||!bound_key_known||!mount.bound())return render_rejected(f,1);
    // Revoke/normal pre-PlayerTick must have settled the sole old lease.
    if(lease.dirty())return render_rejected(f,2);
    gears_vr::Pose endpoint_grip{};
    const bool converted=bound_is_boltok?
        (endpoint_grip=f.right_grip,gears_vr::stereo_pose_valid(endpoint_grip)):
        bound_is_frag?gears_weapon_frag_grasp::grip_from_palm_endpoint(f.right_grip,endpoint_grip):
        gears_weapon_lancer_wrist_mount::grip_from_palm_endpoint(f.right_grip,endpoint_grip);
    if(!converted)return render_rejected(f,3);
    if(!refresh_editor_mount())return render_rejected(f,7);
    Plan next{};next.key=bound_key;next.right_grip=endpoint_grip;next.endpoint_result=f;
    next.editor_revision=editor_mount_revision;
    next.endpoint_serial=f.serial;next.endpoint_input=f.controllers.sequence;next.render_owned=true;
    next.departing=f.purpose==gears_committed_hand::Purpose::free_hand_visual;
    // Metadata/log carrier ONLY. No timestamp is synthesized and this carrier
    // never reaches nativeequipment::current, fire, inventory or Rig admission.
    next.source.key=f.parent;next.source.world=f.world;next.source.controller=f.controller;
    next.source.pawn=f.pawn;next.source.mesh=f.mesh;next.source.weapon=next.departing?bound_key.weapon:f.weapon;
    next.source.stereo=f.stereo;next.source.controllers=f.controllers;
    const gears_vr::Viewpoint grip{endpoint_grip.position,gears_vr::ue_angles(gears_vr::rotation_matrix(endpoint_grip.orientation))};
    unsigned gate{};
    if(!render_current(next,&gate))return render_rejected(f,4,gate);
    // Native's Boltok endpoint already has Aim-oriented wrist axes and the
    // actual palm centre. The neutral baseline avoids applying Aim twice.
    const auto& render_mount=bound_is_boltok?editor_baseline:mount;
    if(!render_mount.solve(grip,next.target))return render_rejected(f,5);
    next.valid=true;plan=next;apply();return plan.applied?true:render_rejected(f,6,last_apply_stage);
}
} // namespace nativelancerfollow
