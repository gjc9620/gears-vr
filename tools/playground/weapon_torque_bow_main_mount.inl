// Bow-only fixed original main contact. Called from the existing main mesh
// binder; no Native/C0/input writer, alternate hand target or animation read.
namespace torquebow_main_mount {
namespace hand=gears_weapon_hand;namespace grip=gears_torque_bow_main_grip;
bool bind(hand::Frame const& f,gears_vr::Viewpoint const& right,std::array<float,16> const& matrix,
    double units,gears_weapon::WeaponMeshFollow& out) noexcept {
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::current(nullptr,f)||
       !playground::instance(f.weapon.address,L"Weap_Bow_base"))return false;
    std::array<float,16> avatar{},closing{};hand::ObjectIdentity mesh{};double avatar_scale{};
    if(!nativeequipment::identify(nullptr,f.mesh.address,mesh)||!hand::same(f.mesh,mesh)||
       !copy(avatar.data(),f.mesh.address+0xa0,sizeof avatar)||!grip::uniform_scale(avatar,avatar_scale))return false;
    // This is the unchanged held-wrist calibration required by the existing
    // Native body-main-mount protocol for its admitted held_visual weapons.
    // It is not certification of a Lancer asset or a new Bow physical fit.
    gears_weapon_lancer_wrist_mount::Profile native{};gears_weapon::WeaponMeshFollow candidate;
    if(!gears_weapon_lancer_wrist_mount::marcus115(gears_hand_authored_mount::marcus115_signature,avatar_scale,units,native)||
       !grip::bind(candidate,right,matrix,avatar_scale,units,native.mount.calibration)||
       !copy(closing.data(),f.mesh.address+0xa0,sizeof closing)||avatar!=closing||
       !nativeequipment::identify(nullptr,f.mesh.address,mesh)||!hand::same(f.mesh,mesh)||
       !nativeequipment::current(nullptr,f)||out.bound())return false;
    out=candidate;
    static unsigned rows{};
    if(log_file&&rows++<8){fwprintf(log_file,L"torquebow_main_contact frame=%llu avatar_scale=%.9g units=%.9g fixed_original_wrist=1 pickup_offset=0 native_calibration_unchanged=1 skin_fit_accepted=0\n",
        f.key.frame,avatar_scale,units);fflush(log_file);}
    return true;
}
}
