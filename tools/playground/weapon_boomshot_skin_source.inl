#include "../vr-common/weapon_boomshot_hand_mount.h"
// Read-only current gun matrices + exact Scene Rig. C0 supplies ONLY its
// authenticated model scale here; fitted palms never become physical input.
namespace boomshot_skin_source {
namespace hand=gears_weapon_hand;namespace vr=gears_vr;namespace mount=gears_boomshot_hand_mount;
struct Source {
    boomshot_pose::Source pose{};gears_rig_tracking::Frame rig{};
    vr::Pose root{},barrel{};double root_scale{},barrel_scale{},hand_scale{},units{};
};
struct SupportBinding {
    lancerreload_inventory::Identity identity{};hand::ObjectIdentity asset{};
    std::uint64_t hold{},reference{},frame{};vr::Pose geometry{},candidate{},right{};
    double units{};unsigned stable{};bool known{};
};
SupportBinding support_binding{};
bool support_scope(hand::Frame const& f)noexcept{
    return support_binding.identity==lancerreload_inventory::identity(f)&&
        support_binding.hold==vrequip::model.hold_generation()&&support_binding.reference==f.reference_generation&&
        hand::same(support_binding.asset,nativelancerfollow::bound_key.asset)&&support_binding.units==nativelancerfollow::bound_units;
}
void observe_support(hand::Frame const& f,Source const& source)noexcept{
    if(f.left_grip_down)return;
    if(!support_scope(f)){
        support_binding={};support_binding.identity=lancerreload_inventory::identity(f);support_binding.asset=source.pose.asset;
        support_binding.hold=vrequip::model.hold_generation();support_binding.reference=f.reference_generation;support_binding.units=source.units;
    }
    // Refresh while released; shoulder-draw frames are not the settled firing pose.
    vr::Pose world{},relative{};
    if(!mount::support(source.root,source.root_scale,source.hand_scale,source.units,world)||
       !mount::relative(source.rig.world_grip[1],world,source.units,relative)||!gears_weapon_two_hand::geometry(relative))return;
    const auto right=f.controllers.hands[1].grip;
    const auto distance=[](vr::Vector a,vr::Vector b){return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z);};
    const auto aligned=[](vr::Quaternion a,vr::Quaternion b){return std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w)>.99999;};
    if(!support_binding.frame||distance(right.position,support_binding.right.position)>.001||!aligned(right.orientation,support_binding.right.orientation)||
       distance(relative.position,support_binding.candidate.position)>.003||!aligned(relative.orientation,support_binding.candidate.orientation)){
        support_binding.candidate=relative;support_binding.right=right;support_binding.frame=f.key.frame;support_binding.stable=0;return;
    }
    if(f.key.frame>support_binding.frame){
        support_binding.frame=f.key.frame;
        if(++support_binding.stable>=2){support_binding.geometry=relative;support_binding.known=true;}
    }
}
bool scale_source(gears_committed_interaction::Frame const& actual,hand::Frame const& f,
    gears_rig_tracking::Frame const& rig,std::int64_t current_ns)noexcept{
    const auto& r=actual.right;
    return gears_committed_interaction::matches(actual,f,vrequip::model.hold_generation(),current_ns,unsigned(f.stereo.source)==1)&&
        actual.model_scale>=.01&&actual.model_scale<=10&&r.units_per_metre==rig.units_per_metre&&
        r.binding_generation==rig.binding_generation&&r.native_reference.epoch==rig.native_reference.epoch&&
        r.producer.stream==rig.producer.stream&&r.producer.tracking[0]==rig.producer.tracking[0]&&r.producer.tracking[1]==rig.producer.tracking[1]&&
        r.controllers.sequence<=f.controllers.sequence&&r.controllers.sync_ns<=f.controllers.sync_ns;
}
bool model_scale(hand::Frame const& f,double& out)noexcept{
    namespace meta=gripdamage_metadata;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::final_check};
    weaponhand::BoolField field{};std::array<float,16> matrix{};vr::Pose root{};double scale{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(reader,f.mesh)||
       !weaponhand::field(reader,f.mesh.address,L"LocalToWorld",L"StructProperty",64,field)||
       !reader.at(f.mesh.address,field.property.offset,matrix)||!hammerburst_runtime::matrix(matrix,root,scale)||
       scale<.01||scale>10||!unified_weapon_fire::retained_metadata(reader))return false;
    out=scale;return true;
}
bool read(hand::Frame const& f,Source& out)noexcept{
    out={};const auto observed_now=weaponhand::clock_ns(nullptr);const auto identity=lancerreload_inventory::identity(f);
    static Source cached{};static hand::Frame cached_frame{};static bool cached_known{};
    // Geometry only, scoped to the identical Scene/controller frame. Never
    // reuse an inventory observation or extend physical input timestamps.
    if(cached_known&&hand::same_frame(cached_frame,f)&&hand::frame_active(f,observed_now)){
        out=cached;return true;
    }
    if(!weaponhand::current_gt(nullptr)||!hand::frame_active(f,observed_now)||f.recentered||f.heading_deferred||
       !identity.valid()||!nativelancerfollow::bound_key_known||!nativelancerfollow::mount.bound()||
       !hand::same(f.weapon,nativelancerfollow::bound_key.weapon)||
       nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation())return false;
    Source value{};hammerburst_animation::Parts parts{};
    if(!boomshot_parts::read(f,parts)||
       handrig::read(value.rig,0,unsigned(f.stereo.source)).reply!=gears_rig_tracking::Reply::accepted||
       gears_equipment_rig::match(f,value.rig,weaponhand::clock_ns(nullptr),unsigned(f.stereo.source))!=gears_equipment_rig::Status::matched||
       (value.rig.world_pose_mask&(gears_rig_tracking::left_grip_pose|gears_rig_tracking::right_grip_pose))!=
           (gears_rig_tracking::left_grip_pose|gears_rig_tracking::right_grip_pose)||
       !model_scale(f,value.hand_scale)||
       !hammerburst_runtime::matrix(parts.world[0],value.root,value.root_scale)||
       !hammerburst_runtime::matrix(parts.world[2],value.barrel,value.barrel_scale)||
       std::abs(value.root_scale-value.barrel_scale)>.001||
       !hand::same(parts.component,nativelancerfollow::bound_key.component)||!hand::same(parts.asset,nativelancerfollow::bound_key.asset)||
       !(identity==lancerreload_inventory::identity(f))||
       gears_equipment_rig::match(f,value.rig,weaponhand::clock_ns(nullptr),unsigned(f.stereo.source))!=gears_equipment_rig::Status::matched)return false;
    value.pose.component=parts.component;value.pose.asset=parts.asset;value.pose.world=parts.world;
    value.units=value.rig.units_per_metre;observe_support(f,value);cached=value;cached_frame=f;cached_known=true;out=value;return true;
}
bool support_world(Source const& source,vr::Pose& out)noexcept{
    return mount::support(source.root,source.root_scale,source.hand_scale,source.units,out);
}
bool mechanism_world(Source const& source,bool closing,vr::Pose& out)noexcept{
    return mount::mechanism(source.barrel,source.barrel_scale,source.hand_scale,source.units,closing,out);
}
bool relative_support(Source const& source,vr::Pose& out)noexcept{
    // This is the existing bound right-grip-to-model GEOMETRY at identity,
    // not a claimed live Root pose or a cached tracking/contact observation.
    // Keeping that geometry independent of last frame's two-hand correction
    // avoids feeding already steered gun poses back into the steering solve.
    std::array<float,16> matrix{};vr::Pose root{},relative{};double scale{};
    if(!nativelancerfollow::mount.model_matrix({{},{}},matrix)||!hammerburst_runtime::matrix(matrix,root,scale)||
       std::abs(scale-source.root_scale)>.001||!mount::support(root,scale,source.hand_scale,source.units,relative))return false;
    relative.position=mount::mul(relative.position,1/source.units);
    if(!gears_weapon_two_hand::geometry(relative))return false;out=relative;return true;
}
bool support_relative(hand::Frame const& f,vr::Pose& out)noexcept{
    // Support arbitration needs the bound grip-to-model geometry, not an
    // independently sampled animation/socket certificate. This path is data-only.
    if(hand::index_pinch(f)||!nativelancerfollow::bound_key_known||
       !nativelancerfollow::mount.bound()||!hand::same(f.weapon,nativelancerfollow::bound_key.weapon)||
       nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation()||
       !hand::frame_active(f,weaponhand::clock_ns(nullptr)))return false;
    if(!support_binding.known||!support_scope(f))return false;
    out=support_binding.geometry;return true;
}
}
