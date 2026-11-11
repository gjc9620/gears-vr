#pragma once
#include "weapon_reload_geometry.h"

namespace gears_weapon {
// Extraction witness for HB's independently observed OPEN feed port. A grip
// release alone never proves that the original magazine cleared the gun.
class HammerburstMagazineExtraction {
    FeedSample previous_{};
    double depth_{};
    bool started_{},complete_{};
public:
    void reset() noexcept {*this={};}
    bool complete() const noexcept {return complete_;}
    FeedSample const* previous_sample() const noexcept {return started_?&previous_:nullptr;}
    bool observe(FeedSample const& s,FeedPortMount const& m) noexcept {
        if(!m.valid()||!s.weapon||!s.item||!s.world||!s.space_epoch||!s.sequence||
           s.monotonic_ns<=0||!s.tracked||!s.item_held||
           !gears_vr::stereo_pose_valid(s.weapon_pose)||!gears_vr::stereo_pose_valid(s.left_grip)){
            reset();return false;
        }
        if(started_&&(s.weapon!=previous_.weapon||s.item!=previous_.item||s.world!=previous_.world||
           s.space_epoch!=previous_.space_epoch||s.sequence<=previous_.sequence||
           s.monotonic_ns<=previous_.monotonic_ns||s.monotonic_ns-previous_.monotonic_ns>m.maximum_sample_gap_ns)){
            reset();return false;
        }
        const auto item=sum(s.left_grip.position,rotate(s.left_grip.orientation,m.item_in_left_grip.position));
        const auto port=sum(s.weapon_pose.position,rotate(s.weapon_pose.orientation,m.port_in_weapon.position));
        const auto axis=rotate(s.weapon_pose.orientation,m.outward_axis_in_weapon);
        const auto delta=difference(item,port);const auto depth=dot(delta,axis);
        const auto lateral=length(difference(delta,scaled(axis,depth)));
        const auto iq=gears_vr::multiply(s.left_grip.orientation,m.item_in_left_grip.orientation);
        const auto pq=gears_vr::multiply(s.weapon_pose.orientation,m.port_in_weapon.orientation);
        const double alignment=std::abs(iq.x*pq.x+iq.y*pq.y+iq.z*pq.z+iq.w*pq.w);
        if(lateral>m.lateral_tolerance_metres||alignment<std::cos(m.maximum_orientation_error_radians*.5)||
           depth<-.02||(!started_&&depth>m.seated_depth_metres)||
           (started_&&std::abs(depth-depth_)>m.maximum_relative_speed_metres_per_second*
               (double(s.monotonic_ns-previous_.monotonic_ns)/1e9))){reset();return false;}
        previous_=s;depth_=depth;started_=true;
        complete_|=depth>=m.approach_depth_metres;
        return true;
    }
};
}
