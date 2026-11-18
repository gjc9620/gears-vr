#pragma once
#include "hand_authored_mount_profile.h"
#include <array>
#include <cstring>
#include <type_traits>

// Local authoring data only. Never a tracking sample, permission or timestamp.
namespace gears_pose_editor {
namespace vr=gears_vr;
struct Config {
    std::uint64_t revision{};
    std::uint32_t enabled{},reserved{}; // low8: part0..4; bit8 volumes; bit9 poses
    gears_hand_authored_mount::Signature geometry{gears_hand_authored_mount::marcus115_signature};
    // Delta to the palm contact in the original main-grip axes, metres.
    vr::Pose palm{};
    // Postmultiply CURRENT admitted local finger rotations, index..thumb.
    std::array<vr::Quaternion,15> fingers{};
};
static_assert(std::is_trivially_copyable_v<Config> && sizeof(Config)==584);
inline constexpr std::size_t wire_size=600;
using Wire=std::array<unsigned char,wire_size>;
inline std::uint64_t checksum(unsigned char const* p,std::size_t n) noexcept {
    std::uint64_t h=14695981039346656037ull;
    for(std::size_t i=0;i<n;++i){h^=p[i];h*=1099511628211ull;}return h;
}
inline bool bounded(vr::Quaternion q,double degrees) noexcept {
    const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return std::isfinite(n)&&std::abs(n-1.)<1e-6&&
        std::abs(q.w)>=std::cos(degrees*3.14159265358979323846/360.);
}
inline unsigned part(Config const& c) noexcept {return c.reserved&255u;}
inline bool volumes(Config const& c) noexcept {return (c.reserved&256u)!=0;}
inline bool poses(Config const& c) noexcept {return (c.reserved&512u)!=0;}
inline vr::Pose adjusted_grip(Config const& c,vr::Pose const& grip,double units) noexcept {
    if(!c.enabled)return grip;
    const auto d=vr::transform(vr::rotation_matrix(grip.orientation),c.palm.position);
    return {{grip.position.x+d.x*units,grip.position.y+d.y*units,grip.position.z+d.z*units},
        vr::normalized(vr::multiply(grip.orientation,c.palm.orientation))};
}
inline bool valid(Config const& c) noexcept {
    if(!c.revision||c.enabled>1||part(c)>4||(c.reserved&~0x307u)||c.geometry!=gears_hand_authored_mount::marcus115_signature||
       !vr::finite(c.palm.position)||std::hypot(std::hypot(c.palm.position.x,c.palm.position.y),c.palm.position.z)>.05||
       !bounded(c.palm.orientation,45))return false;
    for(auto const& q:c.fingers)if(!bounded(q,90))return false;
    return true;
}
inline Wire encode(Config const& c) noexcept {
    Wire w{};std::memcpy(w.data(),"GVRPOSE2",8);std::memcpy(w.data()+8,&c,sizeof c);
    const auto h=checksum(w.data(),592);std::memcpy(w.data()+592,&h,8);return w;
}
inline bool decode(Wire const& w,Config& out) noexcept {
    std::uint64_t h{};std::memcpy(&h,w.data()+592,8);
    if(std::memcmp(w.data(),"GVRPOSE2",8)||h!=checksum(w.data(),592))return false;
    Config c{};std::memcpy(&c,w.data()+8,sizeof c);
    if(!valid(c))return false;out=c;return true;
}
inline vr::Quaternion finger(Config const& c,unsigned joint,vr::Quaternion current) noexcept {
    return c.enabled&&joint<15?vr::normalized(vr::multiply(current,c.fingers[joint])):current;
}
#if defined(_WIN32)
using Read=bool(__cdecl*)(unsigned,Config*) noexcept;
using Report=void(__cdecl*)(std::uint64_t,unsigned,std::uint64_t) noexcept;
inline constexpr char read_export[]="GearsVrReadPoseEditorV2";
inline constexpr char report_export[]="GearsVrPoseEditorReceiptV2";
#endif
}
