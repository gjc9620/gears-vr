// Boltok-only fixed authoring contact through the existing component follower.
// This formats no C0 target and owns no separate hand, camera or bone writer.
namespace boltok_main_mount {
bool bind(gears_weapon_hand::Frame const& f,gears_vr::Viewpoint const& right,
    std::array<float,16> const& matrix,double units,
    gears_weapon::WeaponMeshFollow& out) noexcept {
    namespace hand=gears_weapon_hand;
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::current(nullptr,f)||
       !playground::instance(f.weapon.address,L"Weap_LocustPistol")||out.bound())return false;
    std::array<float,16> avatar{},after{};hand::ObjectIdentity mesh{};
    if(!nativeequipment::identify(nullptr,f.mesh.address,mesh)||!hand::same(f.mesh,mesh)||
       !copy(avatar.data(),f.mesh.address+0xa0,sizeof avatar))return false;
    double avatar_scale{};
    for(unsigned axis=0;axis<3;++axis){
        const auto size=std::hypot(std::hypot(double(avatar[axis*4]),double(avatar[axis*4+1])),double(avatar[axis*4+2]));
        if(!std::isfinite(size)||size<.1||size>10)return false;
        if(axis&&std::abs(size-avatar_scale/double(axis))>1e-5)return false;
        avatar_scale+=size;
    }
    avatar_scale/=3.;
    gears_weapon_lancer_wrist_mount::Profile native{};
    gears_weapon::WeaponMeshFollow candidate;
    if(!gears_weapon_lancer_wrist_mount::marcus115(gears_hand_authored_mount::marcus115_signature,avatar_scale,units,native)||
       !gears_boltok_main_grip::bind(candidate,right,matrix,units,native.mount.calibration)||
       !copy(after.data(),f.mesh.address+0xa0,sizeof after)||after!=avatar||
       !nativeequipment::identify(nullptr,f.mesh.address,mesh)||!hand::same(f.mesh,mesh)||
       !nativeequipment::current(nullptr,f))return false;
    out=candidate;
    if(log_file){static unsigned rows{};if(rows++<8){
        fwprintf(log_file,L"boltok_main_contact frame=%llu hold=%llu avatar_scale=%.9g units=%.9g source=owner_authored_default pickup_offset=0 native_calibration_unchanged=1 fit_accepted=0\n",
            f.key.frame,vrequip::model.hold_generation(),avatar_scale,units);fflush(log_file);}}
    return true;
}
}
