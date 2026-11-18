// Actual original bones; read-only geometry, not a tracking or action permit.
namespace gnasher_parts {
gears_equipment_names::Lookup names({L"b_Shotgun_Root",L"b_Shotgun_Main",L"b_Shotgun_Lever"});
bool read(gears_weapon_hand::Frame const& f,hammerburst_animation::Parts& out) noexcept {
    return hammerburst_animation::read_part_group(f,out,lancermain_asset::HeldAsset::gnasher,names,
        {L"b_Shotgun_Root",L"b_Shotgun_Main",L"b_Shotgun_Lever"},{0,1,2});
}
// Original gun-bone geometry only. Units are supplied by this Frame's rig;
// component scale is retained, never interpreted as a controller correction.
struct Measured {
    hammerburst_animation::Parts source{};gears_vr::Pose lever_in_main{};
};
struct FrameGeometry {
    hammerburst_animation::Parts source{};
    gears_vr::Pose main_in_component{},lever_in_main{};
    double component_scale{};
};
bool frame_geometry(gears_weapon_hand::Frame const& f,double units,FrameGeometry& out) noexcept {
    out={};namespace hand=gears_weapon_hand;
    if(!nativeequipment::current(nullptr,f)||!nativelancerfollow::bound_key_known)return false;
    const auto key=nativelancerfollow::bound_key;
    if(!hand::same(key.weapon,f.weapon)||key.hold_generation!=vrequip::model.hold_generation())return false;
    vrequip::MeshPoseProbe before{},after{};FrameGeometry value{};
    if(!nativelancerfollow::read_key(key,before)||!read(f,value.source)||
       !hand::same(value.source.key,f.key)||!hand::same(value.source.component,key.component)||
       !hand::same(value.source.asset,key.asset)||!nativelancerfollow::read_key(key,after)||
       std::memcmp(before.local_to_world,after.local_to_world,sizeof before.local_to_world)||
       !nativelancerfollow::bound_key_known||!(nativelancerfollow::bound_key==key)||
       !nativeequipment::current(nullptr,f))return false;
    std::array<float,16> component_matrix{};
    std::memcpy(component_matrix.data(),after.local_to_world,sizeof after.local_to_world);
    gears_vr::Pose component{},main{},lever{};double main_scale{},lever_scale{};
    if(!hammerburst_runtime::matrix(component_matrix,component,value.component_scale)||
       !hammerburst_runtime::matrix(value.source.world[1],main,main_scale)||
       !hammerburst_runtime::matrix(value.source.world[2],lever,lever_scale)||
       !gears_weapon::gnasher_live_basis::relation(component,value.component_scale,main,main_scale,units,value.main_in_component)||
       !gears_weapon::gnasher_live_basis::relation(main,main_scale,lever,lever_scale,units,value.lever_in_main)||
       !nativeequipment::current(nullptr,f))return false;
    out=value;return true;
}
bool measure(gears_weapon_hand::Frame const& f,double units,Measured& out) noexcept {
    out={};FrameGeometry value{};
    // Final closure must use the same live component/bone/scale bracket as
    // the down endpoint. Sequential world-bone getters alone do not prove
    // that the component stayed fixed while their relative pose was read.
    if(!frame_geometry(f,units,value))return false;
    out={value.source,value.lever_in_main};return true;
}
}
