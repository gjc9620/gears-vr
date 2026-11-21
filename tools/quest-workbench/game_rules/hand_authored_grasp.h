#pragma once
#include "hand_object_grasp.h"
#include "hand_authored_mount_profile.h"
#include <type_traits>

// Optional cosmetic publication. V2/V3 layouts and exports stay unchanged.
// No engine address, action grant, new clock or tracking substitution exists.
namespace gears_hand_authored_grasp {
namespace vr=gears_vr;namespace hand=gears_weapon_hand;
namespace object=gears_hand_object_grasp;
enum class Side:std::uint32_t {left,right};
enum class Mode:std::uint32_t {left_part,right_main_fingers};
using Joints=std::array<vr::Quaternion,15>;
using Signature=gears_hand_authored_mount::Signature;
struct Request {
    std::uint32_t size{sizeof(Request)},version{4};
    hand::Frame source{};
    Signature geometry_signature{};std::uint64_t profile_id{};
    Side side{};Mode mode{};object::Kind kind{};
    std::uint32_t proximity{},has_trigger_endpoint{},reserved{};
    vr::Pose grip{};
    Joints reference{},closed{},trigger_closed{};
};
inline constexpr char publish_export[]="GearsVrPublishAuthoredGraspV4";
// Optional finger-only command, same strict Request layout; never a wrist goal.
inline constexpr char part_fingers_export[]="GearsVrPublishAuthoredPartFingersV5";
#if defined(_WIN32)
using Publish=bool(__cdecl*)(Request const*) noexcept;
#endif
struct Binding {
    Signature geometry_signature{};
    Joints reference{}; // Taken from the already authenticated C0 model.
};
struct Output {
    bool known{};Mode mode{};hand::Frame source{};std::uint64_t profile_id{};
    Joints local{};
    // Consumer selects its generic authored preset; never the Lancer token.
    object::Target left{};
    bool finger_override{}; // Internal current finger command, not the exported Request.
    object::Kind finger_kind{}; // Current admitted part; no wrist goal.
};
inline bool unit(vr::Quaternion q) noexcept {
    const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return std::isfinite(n)&&std::abs(n-1.)<=.0001;
}
inline bool identity(vr::Pose const& p) noexcept {
    return p.position.x==0&&p.position.y==0&&p.position.z==0&&
        p.orientation.x==0&&p.orientation.y==0&&p.orientation.z==0&&p.orientation.w==1;
}
inline bool signature_known(Signature const& signature) noexcept {
    for(auto byte:signature)if(byte)return true;
    return false;
}
inline bool structural(Request const& q) noexcept {
    if(q.size!=sizeof q||q.version!=4||q.reserved||!signature_known(q.geometry_signature)||!q.profile_id||
       unsigned(q.side)>1||unsigned(q.mode)>1||q.proximity>1||q.has_trigger_endpoint>1||
       !vr::stereo_pose_valid(q.grip))return false;
    if(q.mode==Mode::right_main_fingers){
        if(q.side!=Side::right||q.kind!=object::Kind::none||q.proximity||!identity(q.grip))return false;
    }else if(q.side!=Side::left||q.kind==object::Kind::none||unsigned(q.kind)>unsigned(object::Kind::support)||q.has_trigger_endpoint)return false;
    for(unsigned i=0;i<15;++i){
        if(!unit(q.reference[i])||!unit(q.closed[i])||!unit(q.trigger_closed[i]))return false;
        if(!q.has_trigger_endpoint&&(q.trigger_closed[i].x!=0||q.trigger_closed[i].y!=0||q.trigger_closed[i].z!=0||q.trigger_closed[i].w!=1))return false;
        // A real trigger may change the index only, never the other fingers.
        if(q.has_trigger_endpoint&&i>=3){
            const auto a=q.closed[i],b=q.trigger_closed[i];
            const double sign=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0?-1.:1.;
            if(std::abs(a.x-sign*b.x)>1e-6||std::abs(a.y-sign*b.y)>1e-6||std::abs(a.z-sign*b.z)>1e-6||std::abs(a.w-sign*b.w)>1e-6)return false;
        }
    }
    return true;
}
inline bool reference_matches(Request const& q,Binding const& b) noexcept {
    if(!structural(q)||q.geometry_signature!=b.geometry_signature)return false;
    for(unsigned i=0;i<15;++i){
        const auto a=q.reference[i],r=b.reference[i];if(!unit(r))return false;
        const double sign=a.x*r.x+a.y*r.y+a.z*r.z+a.w*r.w<0?-1.:1.;
        if(std::abs(a.x-sign*r.x)>1e-6||std::abs(a.y-sign*r.y)>1e-6||std::abs(a.z-sign*r.z)>1e-6||std::abs(a.w-sign*r.w)>1e-6)return false;
    }
    return true;
}
inline bool rotations(Request const& q,Binding const& b,double trigger,Joints& out) noexcept {
    if(!reference_matches(q,b)||!std::isfinite(trigger)||trigger<0||trigger>1)return false;
    Joints candidate=q.closed;
    if(q.has_trigger_endpoint)for(unsigned i=0;i<3;++i)candidate[i]=object::blend(q.closed[i],q.trigger_closed[i],trigger);
    out=candidate;return true;
}
static_assert(std::is_trivially_copyable_v<Request>);
static_assert(sizeof(Request)==2984&&alignof(Request)==8);
static_assert(offsetof(Request,source)==8&&offsetof(Request,reference)==1544);
}
