#pragma once
#include "../vr-common/weapon_equipment_observation.h"
#include <array>
#include <cmath>

namespace gears_weapon_component_samples {
namespace hand=gears_weapon_hand;
struct Identity {
    hand::FrameKey key{};std::array<hand::ObjectIdentity,5> objects{};std::uint64_t hold_generation{};
    std::uint64_t reference_generation{},input_session{},space_epoch{},profile_generation{},input_epoch{};
    std::uint32_t source{};
};
inline bool same(Identity const& a,Identity const& b) noexcept {
    const auto& x=a.key;const auto& y=b.key;
    if(x.game_start!=y.game_start||x.native_started_qpc!=y.native_started_qpc||x.registration_cookie!=y.registration_cookie||
       x.lifecycle!=y.lifecycle||x.world_generation!=y.world_generation||x.game_pid!=y.game_pid||x.thread_id!=y.thread_id||
       a.hold_generation!=b.hold_generation||a.reference_generation!=b.reference_generation||a.input_session!=b.input_session||
       a.space_epoch!=b.space_epoch||a.profile_generation!=b.profile_generation||a.input_epoch!=b.input_epoch||a.source!=b.source)return false;
    for(unsigned i=0;i<a.objects.size();++i)if(!hand::same(a.objects[i],b.objects[i]))return false;
    return true;
}
inline bool moved(gears_vr::Pose const& a,gears_vr::Pose const& b) noexcept {
    const auto x=a.position.x-b.position.x,y=a.position.y-b.position.y,z=a.position.z-b.position.z;
    if(x*x+y*y+z*z>=.005*.005)return true;
    const auto& q=a.orientation;const auto& p=b.orientation;
    const auto qa=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w,pa=p.x*p.x+p.y*p.y+p.z*p.z+p.w*p.w;
    const auto dot=std::abs(q.x*p.x+q.y*p.y+q.z*p.z+q.w*p.w)/std::sqrt(qa*pa);
    return dot<.9998476951563913; // two degrees; q and -q are equivalent.
}
struct Slot {Identity id{};gears_vr::Pose anchor{};unsigned used{},remaining{};bool known{};};
class Budget {
    std::array<Slot,8> slots_{};unsigned count_{};
public:
    // Sampling policy only. No metadata/pose/permission is cached as evidence.
    bool claim(Identity const& id,gears_vr::Pose const& grip,unsigned& attempt) noexcept {
        attempt=0;if(!id.hold_generation||!gears_vr::stereo_pose_valid(grip))return false;
        unsigned i=0;for(;i<count_;++i)if(same(slots_[i].id,id))break;
        if(i==count_){if(count_==slots_.size())return false;slots_[count_++].id=id;}
        auto& s=slots_[i];if(s.used==24)return false;
        if(!s.known||moved(s.anchor,grip)){s.anchor=grip;s.known=true;s.remaining=3;}
        if(!s.remaining)return false;--s.remaining;attempt=++s.used;return true;
    }
};
} // namespace gears_weapon_component_samples
