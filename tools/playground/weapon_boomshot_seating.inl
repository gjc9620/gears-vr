// Normal-GT original source certification + callback-safe geometry read.
// Original Notify_AmmoReload only detaches M0 and shows the embedded mesh;
// it supplies no seated static-component transform to reuse.
namespace boomshot_seating {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
namespace physical=gears_weapon::boomshot_insertion;namespace vr=gears_vr;
using P=vr::Pose;using V=vr::Vector;
struct Binding {
    inv::Identity identity{};boomshot_pose::Source pose{};boomshot_original::Source original{};
    boomshot_socket::Source socket{};P seated{},root_in_right{};double root_scale{},units{};std::uint64_t generation{};bool known{},root_grip_known{}; P observed_right{};std::uint64_t observed_release{};unsigned neutral_samples{};
};
Binding binding{};std::uint64_t next_generation{1};
// Authored original-surface fit, NOT a claimed engine-seated transform.
// Original GPU exports a3b70678 /3f0f49cb; fixed Magazine pivot(0,30,2).
// Axial rim-plane centre residual X180=.026722raw, Y180=.754200raw.
// No fitted translation/scale. Original-body footprint clears after13.474raw
// downward travel;15.5raw adds2.026raw of initial separation. Nine orthographic
// source views reviewed; surface/all-angle/game/headset acceptance remains open.
constexpr V authored_pivot_raw{0,30,2};constexpr vr::Quaternion authored_rotation{1,0,0,0};
constexpr double travel_raw=15.5,approach_raw=1.,capture_raw=1.2,retain_raw=1.8,seat_raw=.2;
V subtract(V a,V b) noexcept{return {a.x-b.x,a.y-b.y,a.z-b.z};}
double distance(V a,V b) noexcept {const auto d=subtract(a,b);return std::hypot(std::hypot(d.x,d.y),d.z);}
bool aligned(vr::Quaternion a,vr::Quaternion b,double degrees) noexcept {
    return std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w)>=std::cos(degrees*3.14159265358979323846/360.);
}
bool socket_fields(boomshot_socket::Source const& s) noexcept {
    // Check actual reflected values; absent serialized tags were not zero proof.
    return s.relative.x==0&&s.relative.y==0&&s.relative.z==0&&s.rotation.pitch==0&&s.rotation.yaw==0&&s.rotation.roll==0&&
        s.scale.x==1&&s.scale.y==1&&s.scale.z==1;
}
bool current_socket(hand::Frame const& f,boomshot_pose::Source const& pose) noexcept {
    namespace meta=gripdamage_metadata;lancermain_asset::Evidence asset{};asset.component=pose.component;asset.asset=pose.asset;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};boomshot_socket::Reads reads{};
    boomshot_socket::Source first{},last{};
    if(!meta::tables(base,schema.objects,schema.names)||!boomshot_socket::metadata(f,asset,r,reads,first))return false;
    reads.repeat=true;
    return boomshot_socket::metadata(f,asset,r,reads,last)&&reads.cursor==reads.count&&reads.field_cursor==reads.field_count&&
        reads.name_cursor==reads.name_count&&socket_fields(first)&&socket_fields(last)&&
        hand::same(first.socket,binding.socket.socket)&&hand::same(last.socket,binding.socket.socket)&&
        first.name.index==binding.socket.name.index&&first.name.number==binding.socket.name.number&&
        first.bone.index==binding.socket.bone.index&&first.bone.number==binding.socket.bone.number;
}
bool read(hand::Frame const& f,physical::Geometry& out) noexcept {
    out={};
    if(!weaponhand::current_gt(nullptr)||!hand::frame_active(f,weaponhand::clock_ns(nullptr))||
       !binding.known||!(binding.identity==inv::identity(f))||!binding.generation||
       !nativelancerfollow::bound_key_known||!nativelancerfollow::mount.bound()||
       !hand::same(f.weapon,nativelancerfollow::bound_key.weapon)||
       !hand::same(binding.pose.component,nativelancerfollow::bound_key.component)||
       !hand::same(binding.pose.asset,nativelancerfollow::bound_key.asset)||
       nativelancerfollow::bound_units!=binding.units)return false;
    physical::Geometry geometry{};geometry.id=binding.generation;geometry.seated_in_root_m=binding.seated;geometry.outward={0,0,-1};
    geometry.travel_m=travel_raw/binding.units;geometry.approach_margin_m=approach_raw/binding.units;
    geometry.capture_radius_m=capture_raw/binding.units;geometry.retain_radius_m=retain_raw/binding.units;
    geometry.capture_angle_degrees=12;geometry.retain_angle_degrees=18;geometry.seat_tolerance_m=seat_raw/binding.units;
    geometry.verified=true; // Original model + authored snap geometry, not headset-fit acceptance.
    if(!physical::valid(geometry))return false;out=geometry;return true;
}
bool observe_root(hand::Frame const& f,hammerburst_animation::Parts const& parts)noexcept{
    gears_rig_tracking::Frame rig{};P root{};double scale{};
    if(handrig::read(rig,0,unsigned(f.stereo.source)).reply!=gears_rig_tracking::Reply::accepted||
       gears_equipment_rig::match(f,rig,weaponhand::clock_ns(nullptr),unsigned(f.stereo.source))!=gears_equipment_rig::Status::matched||
       !hammerburst_runtime::matrix(parts.world[0],root,scale))return false;
    const auto right=rig.world_grip[1];const auto p=gears_weapon::rotate(vr::inverse(right.orientation),subtract(root.position,right.position));
    binding.root_in_right={{p.x/rig.units_per_metre,p.y/rig.units_per_metre,p.z/rig.units_per_metre},
        vr::normalized(vr::multiply(vr::inverse(right.orientation),root.orientation))};
    binding.root_scale=scale;binding.root_grip_known=true;return true;
}
bool grip_distance(hand::Frame const& f,physical::Geometry const& geometry,double& metres)noexcept{
    // Native candidate selection uses this frame's raw controllers. The
    // normal-GT observer already bound the original root to the right grip;
    // reading a second Rig publication here would compare different frames.
    if(!physical::valid(geometry)||!binding.root_grip_known||!hand::frame_active(f,weaponhand::clock_ns(nullptr)))return false;
    const auto p=geometry.seated_in_root_m.position;
    const auto offset=gears_weapon::rotate(binding.root_in_right.orientation,{p.x*binding.root_scale,p.y*binding.root_scale,p.z*binding.root_scale});
    const auto root=binding.root_in_right.position;
    const V in_right{root.x+offset.x,root.y+offset.y,root.z+offset.z};
    const auto& right=f.controllers.hands[1].grip;
    const auto raw=gears_weapon::rotate(right.orientation,{in_right.y,in_right.z,-in_right.x});
    const V target{right.position.x+raw.x,right.position.y+raw.y,right.position.z+raw.z};
    metres=distance(f.controllers.hands[0].grip.position,target)/binding.root_scale;
    return std::isfinite(metres);
}
bool certify(hand::Frame const& f) noexcept {
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::current(nullptr,f))return false;
    physical::Geometry existing{};if(read(f,existing)){
        hammerburst_animation::Parts parts{};
        if(!f.left_grip_down){
            const auto right=f.controllers.hands[1].grip;
            const auto released=f.controllers.hands[0].released[gears_vr::control_squeeze];
            const bool stationary=gears_vr::stereo_same_pose(right,binding.observed_right)&&released==binding.observed_release;
            if(!stationary)binding.neutral_samples=0;
            if(!stationary||binding.neutral_samples<3){
                if(boomshot_parts::read(f,parts)&&observe_root(f,parts)){
                    binding.observed_right=right;binding.observed_release=released;++binding.neutral_samples;
                }
            }
        }
        return true;
    }
    const auto identity=inv::identity(f);const auto units=nativelancerfollow::bound_units;
    if(!identity.valid()||!std::isfinite(units)||units<10||units>500||!next_generation||next_generation==UINT64_MAX)return false;
    boomshot_original::Source original_model{};
    if(!boomshot_original::prepare(f,original_model)||!hand::same(original_model.parts.component,nativelancerfollow::bound_key.component)||
       !hand::same(original_model.parts.asset,nativelancerfollow::bound_key.asset))return false;
    P root{};double scale{};
    if(!hammerburst_runtime::matrix(original_model.parts.world[0],root,scale)||!nativeequipment::current(nullptr,f))return false;
    Binding value{};value.identity=identity;value.original=original_model;
    value.pose.component=original_model.parts.component;value.pose.asset=original_model.parts.asset;value.pose.world=original_model.parts.world;
    value.seated={{authored_pivot_raw.x/units,authored_pivot_raw.y/units,authored_pivot_raw.z/units},authored_rotation};
    value.root_scale=scale;value.units=units;value.generation=next_generation++;value.known=true;
    const auto previous=binding;binding=value;
    if(!read(f,existing)||!observe_root(f,original_model.parts)){binding=previous;return false;}return true;
}
}
