#pragma once
#include "weapon_longshot_reload.h"
#include "stereo_frame.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace gears_weapon::longshot_scope_control {
// Positions are metres in the same current world frame. Ocular axes match
// weapon_longshot_scope: +Z behind the ocular, -Z toward the objective.
struct EyeContact {double distance_m{},lateral_m{},angle_degrees{};bool tracked{};};
inline EyeContact contact(gears_vr::Pose const& ocular,gears_vr::Pose const& eye,bool tracked) noexcept {
    if(!tracked||!gears_vr::stereo_pose_valid(ocular)||!gears_vr::stereo_pose_valid(eye))return {};
    const auto inverse=gears_vr::transpose(gears_vr::rotation_matrix(ocular.orientation));
    const auto p=gears_vr::transform(inverse,{eye.position.x-ocular.position.x,
        eye.position.y-ocular.position.y,eye.position.z-ocular.position.z});
    const auto forward=gears_vr::transform(inverse,gears_vr::transform(
        gears_vr::rotation_matrix(eye.orientation),{0,0,-1}));
    const double cosine=(std::max)(-1.,(std::min)(1.,-forward.z));
    return {p.z,std::hypot(p.x,p.y),std::acos(cosine)*180/3.14159265358979323846,true};
}
inline bool in_eye_box(EyeContact e,bool retained) noexcept {
    if(!e.tracked||!std::isfinite(e.distance_m)||!std::isfinite(e.lateral_m)||
       !std::isfinite(e.angle_degrees)||e.lateral_m<0||e.angle_degrees<0)return false;
    return retained?e.distance_m>=.03&&e.distance_m<=.13&&e.lateral_m<=.04&&e.angle_degrees<=22:
        e.distance_m>=.04&&e.distance_m<=.10&&e.lateral_m<=.025&&e.angle_degrees<=15;
}
struct Frame {
    LongshotReloadStamp stamp{};
    std::array<EyeContact,2> eyes{};
    std::uint32_t pressed{},released{}; // Cumulative right-stick counters.
    bool click_down{},click_active{},scope_ready{},menu{},reloading{};
};
struct Result {
    bool near_eye{},consume_click{},switch_magnification{};
    int eye{-1};
};
class Control {
    LongshotReloadIdentity identity_{};
    std::uint64_t sequence_{};
    std::int64_t observed_ns_{},enter_ns_{};
    std::uint32_t presses_{},releases_{};
    int eye_{-1},candidate_{-1};
    bool initialized_{},consumed_{},near_{};
public:
    Result observe(Frame const& f,std::int64_t now_ns) noexcept {
        const auto& s=f.stamp;
        if(!s.valid(now_ns)||!s.weapon_owned||!s.right_tracked||!f.click_active){
            initialized_=false;near_=false;eye_=candidate_=-1;enter_ns_=0;
            return {false,consumed_,false,-1};
        }
        const bool same=initialized_&&identity_==s.identity;
        if(same&&(s.sequence<=sequence_||s.observed_ns<observed_ns_)){
            // A cached producer sample cannot advance dwell or dispatch, but
            // current menu/reload/eye facts must still withdraw an old scope.
            // Reject regression/colliding counters without lowering the input
            // high-water marks or releasing an already consumed press.
            const bool duplicate=s.sequence==sequence_&&s.observed_ns==observed_ns_&&
                f.pressed==presses_&&f.released==releases_&&f.pressed>=f.released&&
                f.pressed-f.released<=1&&f.click_down==(f.pressed!=f.released);
            const bool lost_eye=near_?!in_eye_box(f.eyes[static_cast<unsigned>(eye_)],true):
                candidate_>=0&&!in_eye_box(f.eyes[static_cast<unsigned>(candidate_)],false);
            if(!duplicate||f.menu||f.reloading||lost_eye){
                near_=false;eye_=candidate_=-1;enter_ns_=0;
            }
            return {near_,consumed_,false,eye_};
        }
        const bool counters_valid=f.pressed>=f.released&&f.pressed-f.released<=1&&
            f.click_down==(f.pressed!=f.released);
        if(!same||!counters_valid||s.observed_ns-observed_ns_>100000000||
           f.pressed<presses_||f.released<releases_){
            identity_=s.identity;sequence_=s.sequence;observed_ns_=s.observed_ns;
            presses_=f.pressed;releases_=f.released;initialized_=true;
            consumed_=f.click_down;near_=false;eye_=candidate_=-1;enter_ns_=0;
            return {false,consumed_,false,-1};
        }
        const auto press_delta=f.pressed-presses_;
        const auto release_delta=f.released-releases_;
        sequence_=s.sequence;observed_ns_=s.observed_ns;
        presses_=f.pressed;releases_=f.released;
        if(!f.click_down)consumed_=false;
        if(f.menu||f.reloading){
            near_=false;eye_=candidate_=-1;enter_ns_=0;
            return {false,consumed_,false,-1};
        }
        if(near_&&!in_eye_box(f.eyes[static_cast<unsigned>(eye_)],true)){
            near_=false;eye_=-1;
        }
        if(!near_){
            int candidate=-1;
            for(unsigned i=0;i<2;++i)if(in_eye_box(f.eyes[i],false)){
                candidate=static_cast<int>(i);break;
            }
            if(candidate!=candidate_){candidate_=candidate;enter_ns_=s.observed_ns;}
            if(candidate>=0&&s.observed_ns-enter_ns_>=80000000){near_=true;eye_=candidate;}
        }else{candidate_=eye_;enter_ns_=s.observed_ns;}
        // Only one newly observed press with no skipped release can execute.
        // Ownership remains latched through exit, pause and tracking loss.
        const bool new_press=press_delta==1&&release_delta==0&&f.click_down;
        const bool dispatch=new_press&&near_&&!consumed_&&f.scope_ready;
        if(new_press&&near_)consumed_=true;
        return {near_,consumed_,dispatch,eye_};
    }
};
} // namespace gears_weapon::longshot_scope_control
