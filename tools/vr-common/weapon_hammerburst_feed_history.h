#pragma once
#include "weapon_reload_geometry.h"
#include "controller_continuity_frame.h"
#include <array>

// Geometry observations only. The caller must still own the current real
// Frame/grant and validate the producer, controls and fixed weapon mapping.
namespace gears_hammerburst_feed_history {
using namespace gears_weapon;
inline bool close_pose(Pose const& a,Pose const& b,double epsilon=1e-5) noexcept {
    if(!gears_vr::stereo_pose_valid(a)||!gears_vr::stereo_pose_valid(b))return false;
    const auto x=gears_vr::normalized(a.orientation),y=gears_vr::normalized(b.orientation);
    return length(difference(a.position,b.position))<=epsilon&&
        std::abs(x.x*y.x+x.y*y.y+x.z*y.z+x.w*y.w)>=1-epsilon*epsilon;
}
inline bool same_sample(FeedSample const& a,FeedSample const& b) noexcept {
    return a.weapon==b.weapon&&a.item==b.item&&a.world==b.world&&a.space_epoch==b.space_epoch&&
        a.sequence==b.sequence&&a.monotonic_ns==b.monotonic_ns&&a.tracked==b.tracked&&a.item_held==b.item_held&&
        close_pose(a.weapon_pose,b.weapon_pose)&&close_pose(a.left_grip,b.left_grip);
}
inline bool seated(FeedSample const& s,FeedPortMount const& m) noexcept {
    const auto axis=rotate(s.weapon_pose.orientation,m.outward_axis_in_weapon);
    const auto p=sum(s.left_grip.position,rotate(s.left_grip.orientation,m.item_in_left_grip.position));
    const auto port=sum(s.weapon_pose.position,rotate(s.weapon_pose.orientation,m.port_in_weapon.position));
    const auto delta=difference(p,port);const double depth=dot(delta,axis);
    const auto q=gears_vr::normalized(gears_vr::multiply(s.left_grip.orientation,m.item_in_left_grip.orientation));
    const auto t=gears_vr::normalized(gears_vr::multiply(s.weapon_pose.orientation,m.port_in_weapon.orientation));
    return std::isfinite(depth)&&depth<=m.seated_depth_metres&&
        length(difference(delta,scaled(axis,depth)))<=m.lateral_tolerance_metres&&
        std::abs(q.x*t.x+q.y*t.y+q.z*t.z+q.w*t.w)>=std::cos(m.maximum_orientation_error_radians*.5);
}
namespace detail {
inline bool advance(FeedInsertionStroke& stroke,FeedSample const& previous,FeedSample const& current,
                    FeedSample const* points,unsigned capacity,unsigned count,FeedPortMount const& mount,FeedResult& out) noexcept {
    if(!mount.valid()||!points||count<3||count>capacity||capacity>gears_hand_continuity::motion_history_capacity+1||!same_sample(points[0],previous)||
       !same_sample(points[count-1],current))return false;
    for(unsigned i=0;i<count;++i){const auto& s=points[i];
        if(!s.weapon||!s.item||!s.world||!s.space_epoch||!s.sequence||s.monotonic_ns<=0||!s.tracked||!s.item_held||
           s.weapon!=current.weapon||s.item!=current.item||s.world!=current.world||s.space_epoch!=current.space_epoch||
           !close_pose(s.weapon_pose,current.weapon_pose)||!gears_vr::stereo_pose_valid(s.left_grip))return false;
        if(i){const auto& p=points[i-1];
            if(p.sequence==UINT64_MAX||s.sequence!=p.sequence+1||s.monotonic_ns<=p.monotonic_ns||
               s.monotonic_ns-p.monotonic_ns>mount.maximum_sample_gap_ns)return false;
            const auto depth=[&](FeedSample const& v){
                const auto point=sum(v.left_grip.position,rotate(v.left_grip.orientation,mount.item_in_left_grip.position));
                const auto port=sum(v.weapon_pose.position,rotate(v.weapon_pose.orientation,mount.port_in_weapon.position));
                return dot(difference(point,port),rotate(v.weapon_pose.orientation,mount.outward_axis_in_weapon));
            };
            if(std::abs(depth(s)-depth(p))>mount.maximum_relative_speed_metres_per_second*
               (double(s.monotonic_ns-p.monotonic_ns)/1e9))return false;
        }
    }
    auto trial=stroke;FeedResult result=FeedResult::rejected;bool inserted=false;
    for(unsigned i=1;i<count;++i){
        // Once crossed, all remaining actual points must still be seated.
        // There is no completion from a historical touch followed by withdrawal.
        if(inserted){if(!seated(points[i],mount))return false;}
        else {result=trial.observe(points[i],mount);inserted=result==FeedResult::inserted;}
    }
    stroke=trial;out=inserted?FeedResult::inserted:result;return true;
}
}
// Preserve the original pointer API's9-point bound. The live expanded path
// passes an owned array, so its real compile-time storage bounds the count.
inline bool advance(FeedInsertionStroke& stroke,FeedSample const& previous,FeedSample const& current,
                    FeedSample const* points,unsigned count,FeedPortMount const& mount,FeedResult& out) noexcept {
    return detail::advance(stroke,previous,current,points,9,count,mount,out);
}
template<std::size_t N>
inline bool advance(FeedInsertionStroke& stroke,FeedSample const& previous,FeedSample const& current,
                    std::array<FeedSample,N> const& points,unsigned count,FeedPortMount const& mount,FeedResult& out) noexcept {
    static_assert(N<=gears_hand_continuity::motion_history_capacity+1,"Actual producer bound");
    return detail::advance(stroke,previous,current,points.data(),unsigned(N),count,mount,out);
}
}
