#pragma once
#include "../vr-common/stereo_frame.h"
#include <cstdint>

// Unconnected renderer candidate. The game-thread adapter must authenticate
// identities and read the original weapon, then supply its actual mesh pose in
// the SAME XR LOCAL frame as the eyes. Controller grip alone is not that pose.
namespace weapon_ammo_card {
using gears_vr::Pose;
using gears_vr::Vector;
struct FrameKey {
    std::uint64_t session{},sequence{},space_epoch{};
    std::int64_t predicted_time{};
};
inline bool valid(FrameKey k) noexcept {
    return k.session&&k.sequence&&k.space_epoch&&k.predicted_time!=0;
}
inline bool same(FrameKey a,FrameKey b) noexcept {
    return a.session==b.session&&a.sequence==b.sequence&&
        a.space_epoch==b.space_epoch&&a.predicted_time==b.predicted_time;
}
inline FrameKey key(gears_vr::StereoSample const& s) noexcept {
    return {s.session,s.sequence,s.space_epoch,s.predicted_time};
}
enum class AmmoKind : unsigned {unknown,firearm,throwable};
struct Counts {
    AmmoKind kind{AmmoKind::unknown};
    std::uint32_t in_gun{},spare{};
    bool infinite_spare{};
};
inline bool valid(Counts const& c) noexcept {
    return (c.kind==AmmoKind::firearm&&c.in_gun<=10000&&c.spare<=1000000)||
        (c.kind==AmmoKind::throwable&&c.in_gun<=1000000&&!c.spare);
}
// Mirrors the saved WarHUD.DrawWeaponInfo data flow: original GetMagazineSize
// minus AmmoUsedCount, with SpareAmmoCount kept separate for the VR card.
// Do not infer ammunition from shots, reload gestures or a display timer.
// Getter/property identity, type and game-thread admission belong to the caller.
inline bool original_firearm_counts(double magazine_size,std::int32_t ammo_used,
    std::int32_t spare,bool infinite_spare,Counts& out) noexcept {
    if(!std::isfinite(magazine_size)||magazine_size<1||magazine_size>10000||
       std::floor(magazine_size)!=magazine_size||ammo_used<0||ammo_used>magazine_size||
       spare<0||spare>1000000)return false;
    out={AmmoKind::firearm,std::uint32_t(magazine_size)-std::uint32_t(ammo_used),
        std::uint32_t(spare),infinite_spare};return true;
}
// Human grenade availability uses the original SpareAmmoCount path. It must
// not inherit the firearm magazine expression or invent a chambered grenade.
inline bool original_throwable_counts(std::int32_t spare,bool infinite_spare,Counts& out) noexcept {
    if(spare<0||spare>1000000)return false;
    out={AmmoKind::throwable,std::uint32_t(spare),0,infinite_spare};return true;
}
struct Identity {
    std::uint64_t world{},pawn{},weapon{},profile{};
};
inline bool valid(Identity id) noexcept {return id.world&&id.pawn&&id.weapon&&id.profile;}
inline bool same(Identity a,Identity b) noexcept {
    return a.world==b.world&&a.pawn==b.pawn&&a.weapon==b.weapon&&a.profile==b.profile;
}
struct AmmoFrame {
    Identity identity{};
    FrameKey frame{};
    std::int64_t received_ns{};
    Counts counts{};
    bool authenticated{};
};
struct WeaponFrame {
    Identity identity{};
    FrameKey frame{};
    std::int64_t received_ns{};
    Pose pose_in_local_m{};
    // Includes current-weapon, right-hand-held and actual component admission.
    // Holster, pickup animation, mesh loss or a weapon switch clears held.
    bool authenticated{},held{};
};
struct Mount {
    std::uint64_t profile{};
    // +X is text-right, +Y text-up, +Z is the readable front normal. This
    // complete pose is fixed in the verified weapon frame, never billboarded.
    Pose card_in_weapon_m{};
    double width_m{},height_m{};
    bool verified{}; // No generic mount, Lancer offset or mesh-axis guess.
};
struct Options {bool enabled{},allow_simulated{};};
struct Plan {
    Identity identity{};
    FrameKey frame{};
    Counts counts{};
    Pose pose_in_local_m{};
    double width_m{},height_m{};
    // Diagnostic geometry only; not a claim about headset text readability.
    double front_cosine[2]{},distance_m[2]{};
};
inline bool fresh(std::int64_t received,std::int64_t now) noexcept {
    return now>0&&received>0&&received<=now&&now-received<=100000000;
}
inline bool prepare(Options options,AmmoFrame const& ammo,WeaponFrame const& weapon,
    Mount const& mount,gears_vr::StereoSample const& eyes,std::int64_t now_ns,Plan& out) {
    if(!options.enabled||!ammo.authenticated||!weapon.authenticated||!weapon.held||
       !valid(ammo.identity)||!same(ammo.identity,weapon.identity)||
       !valid(ammo.frame)||!same(ammo.frame,weapon.frame)||!same(ammo.frame,key(eyes))||
       !fresh(ammo.received_ns,now_ns)||!fresh(weapon.received_ns,now_ns)||
       !fresh(eyes.received_ns,now_ns)||!valid(ammo.counts)||
       !gears_vr::stereo_source_allowed(eyes.source,options.allow_simulated)||
       !gears_vr::stereo_render_active(eyes)||!gears_vr::stereo_tracking_valid(eyes,options.allow_simulated)||
       !gears_vr::stereo_geometry_valid(eyes)||!gears_vr::stereo_pose_valid(weapon.pose_in_local_m)||
       !mount.verified||!mount.profile||mount.profile!=weapon.identity.profile||
       !gears_vr::stereo_pose_valid(mount.card_in_weapon_m)||
       !std::isfinite(mount.width_m)||!std::isfinite(mount.height_m)||
       mount.width_m<.04||mount.width_m>.4||mount.height_m<.02||mount.height_m>.2||
       // Matches the texture's 3:1 aspect; never stretch the numbers.
       std::abs(mount.width_m/mount.height_m-3)>1e-9)return false;
    const auto rotation=gears_vr::rotation_matrix(weapon.pose_in_local_m.orientation);
    const auto offset=gears_vr::transform(rotation,mount.card_in_weapon_m.position);
    Plan candidate{};candidate.identity=ammo.identity;candidate.frame=ammo.frame;candidate.counts=ammo.counts;
    candidate.pose_in_local_m.position={weapon.pose_in_local_m.position.x+offset.x,
        weapon.pose_in_local_m.position.y+offset.y,weapon.pose_in_local_m.position.z+offset.z};
    candidate.pose_in_local_m.orientation=gears_vr::normalized(gears_vr::multiply(
        weapon.pose_in_local_m.orientation,mount.card_in_weapon_m.orientation));
    if(!gears_vr::stereo_pose_valid(candidate.pose_in_local_m))return false;
    const auto normal=gears_vr::transform(gears_vr::rotation_matrix(candidate.pose_in_local_m.orientation),{0,0,1});
    for(unsigned i=0;i<2;++i){const auto eye=eyes.eyes[i].pose.position,card=candidate.pose_in_local_m.position;
        const Vector to_eye{eye.x-card.x,eye.y-card.y,eye.z-card.z};
        const auto distance=std::hypot(std::hypot(to_eye.x,to_eye.y),to_eye.z);
        if(!std::isfinite(distance)||distance<.05||distance>5)return false;
        const auto cosine=(to_eye.x*normal.x+to_eye.y*normal.y+to_eye.z*normal.z)/distance;
        // Reject a reversed/edge-on mount instead of rotating it toward HMD.
        if(!std::isfinite(cosine)||cosine<=.05)return false;
        candidate.distance_m[i]=distance;candidate.front_cosine[i]=cosine;
    }
    candidate.width_m=mount.width_m;candidate.height_m=mount.height_m;
    out=candidate;return true;
}
}
