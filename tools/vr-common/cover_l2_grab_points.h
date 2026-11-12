#pragma once
#include "cover_interaction.h"
#include <array>

namespace gears_vr {
// Original L2 action numbers: left=3, right=4, low-cover popup=7.
// The caller supplies CURRENT game-reference world positions. This policy
// selects points only: it does not dispatch L2 or claim collision clearance.
struct CoverL2GrabPoint {
    Vector world{};
    bool available=false;
};
struct CoverL2GrabPoints {
    unsigned cover_type=0; // Original CT_Standing=1, CT_MidLevel=2.
    bool enabled=false,lean_left=false,lean_right=false,can_popup=false;
    std::array<CoverL2GrabPoint,3> points{}; // left, right, top
};
inline bool cover_l2_finite(Vector p){
    return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);
}
inline bool cover_l2_allowed(CoverL2GrabPoints const& p,unsigned index){
    if(index>=3||!p.enabled||(p.cover_type!=1&&p.cover_type!=2)||
       !p.points[index].available||!cover_l2_finite(p.points[index].world))return false;
    return index==0?p.lean_left:index==1?p.lean_right:p.cover_type==2&&p.can_popup;
}
inline unsigned cover_l2_action(CoverEdgeKind kind){
    return kind==CoverEdgeKind::left?3:kind==CoverEdgeKind::right?4:kind==CoverEdgeKind::top?7:0;
}
// ids 1/2/3 are local to the current cover/slot epochs, never object pointers.
// A latched edge remains selected while dragging beyond acquisition reach.
// Losing its original permission ends it; it cannot silently switch edges.
inline CoverEdgeObservation cover_l2_select(CoverL2GrabPoints const& points,Vector hand_world,
    double units_per_metre,double reach_metres,std::uint64_t latched_id=0,
    double ambiguity_metres=.01){
    if(!cover_l2_finite(hand_world)||!std::isfinite(units_per_metre)||units_per_metre<10||units_per_metre>500||
       !std::isfinite(reach_metres)||reach_metres<=0||!std::isfinite(ambiguity_metres)||
       ambiguity_metres<0||ambiguity_metres>=reach_metres||latched_id>3)return {};
    constexpr CoverEdgeKind kinds[]={CoverEdgeKind::left,CoverEdgeKind::right,CoverEdgeKind::top};
    auto distance=[&](unsigned i){const auto p=points.points[i].world;
        return std::hypot(std::hypot(p.x-hand_world.x,p.y-hand_world.y),p.z-hand_world.z)/units_per_metre;};
    if(latched_id){const unsigned i=unsigned(latched_id-1);
        return cover_l2_allowed(points,i)?CoverEdgeObservation{latched_id,kinds[i],true,distance(i)<=reach_metres}:CoverEdgeObservation{};}
    unsigned best=3;double first=std::numeric_limits<double>::infinity(),second=first;
    for(unsigned i=0;i<3;++i)if(cover_l2_allowed(points,i)){
        const auto d=distance(i);
        if(d<first){second=first;first=d;best=i;}else if(d<second)second=d;
    }
    if(best==3||first>reach_metres||second-first<=ambiguity_metres)return {};
    return {best+1,kinds[best],true,true};
}
} // namespace gears_vr
