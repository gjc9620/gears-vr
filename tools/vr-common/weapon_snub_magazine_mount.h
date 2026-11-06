#pragma once
#include "weapon_pose_solver.h"
#include <cstddef>
#include <limits>
namespace gears_weapon {
// Original Snub vertices in engine mesh axes, with object scale already baked.
// Geometry describes contact/feed only; it confers no hand or inventory grant.
struct SnubMagazineGeometry {
    gears_vr::Vector centre{},feed{},outward{};
    double length{},radius{};bool valid{};
};
template<class Position>bool snub_magazine_geometry(std::size_t count,Position position,
    gears_vr::Vector closed_feed,gears_vr::Vector outward,bool current_reference_verified,SnubMagazineGeometry& out) noexcept {
    out={};if(!current_reference_verified||count<12||count>32767||!gears_vr::finite(closed_feed)||
        !gears_vr::finite(outward)||std::abs(length(outward)-1)>1e-5)return false;
    using V=gears_vr::Vector;
    const auto dot=[](V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;};
    V centre{};for(std::size_t i=0;i<count;++i){const auto p=position(i);
        if(!gears_vr::finite(p))return false;centre.x+=p.x;centre.y+=p.y;centre.z+=p.z;}
    centre={centre.x/double(count),centre.y/double(count),centre.z/double(count)};
    // Original geometry is asymmetric. Its longest statistical axis is NOT
    // the magazine-well direction. Use the actual closed original bone's feed
    // point/axis supplied by the owning adapter, never a PCA guess.
    const V axis=outward;
    double low=std::numeric_limits<double>::max(),high=-low,radius=0;
    for(std::size_t i=0;i<count;++i){const auto p=position(i);const V d{p.x-centre.x,p.y-centre.y,p.z-centre.z};
        const double along=dot(d,axis);low=(std::min)(low,along);high=(std::max)(high,along);
        radius=(std::max)(radius,std::sqrt((std::max)(0.,dot(d,d)-along*along)));}
    if(!std::isfinite(high-low)||high-low<=0||radius<=0||radius>=high-low)return false;
    const V feed_delta{closed_feed.x-centre.x,closed_feed.y-centre.y,closed_feed.z-centre.z};
    const double feed_depth=dot(feed_delta,axis);
    if(feed_depth<low-(high-low)*.25||feed_depth>high+(high-low)*.25||
       dot(feed_delta,feed_delta)-feed_depth*feed_depth>radius*radius*1.5625)return false;
    out={centre,closed_feed,axis,high-low,radius,true};return true;
}
class SnubMagazineMount {
    gears_vr::Pose relative_{};bool known_{};
public:
    void reset() noexcept {*this={};}
    bool known() const noexcept {return known_;}
    gears_vr::Pose relative() const noexcept {return relative_;}
    bool capture(gears_vr::Pose const& wrist,gears_vr::Pose const& seated_item,
        gears_vr::Vector contact,double maximum_distance) noexcept {
        if(known_||!gears_vr::stereo_pose_valid(wrist)||!gears_vr::stereo_pose_valid(seated_item)||
           !gears_vr::finite(contact)||!std::isfinite(maximum_distance)||maximum_distance<=0||
           length(difference(wrist.position,contact))>maximum_distance)return false;
        const auto inverse=gears_vr::inverse(wrist.orientation);
        relative_={rotate(inverse,difference(seated_item.position,wrist.position)),
            gears_vr::normalized(gears_vr::multiply(inverse,seated_item.orientation))};
        known_=true;return true;
    }
    bool apply(gears_vr::Pose const& wrist,gears_vr::Pose& item) const noexcept {
        if(!known_||!gears_vr::stereo_pose_valid(wrist))return false;
        item={sum(wrist.position,rotate(wrist.orientation,relative_.position)),
            gears_vr::normalized(gears_vr::multiply(wrist.orientation,relative_.orientation))};
        return gears_vr::stereo_pose_valid(item);
    }
};
} // namespace gears_weapon
