#pragma once
#include "weapon_lancer_feed_anchor.h"
#include "weapon_lancer_front_handle.h"

namespace gears_weapon::lancer_guide {
using V=gears_vr::Vector;
struct Line {V start{},end{};};
struct Shape {std::array<Line,64> lines{};unsigned count{};};
inline constexpr unsigned border_rails=7;
enum class Step {none,remove_magazine,insert_magazine,pull_charging_handle,
    discard_magazine,take_magazine,release_charging,wait_return};
// The guide follows unfinished mechanics. It cannot finish a transaction,
// change inventory or turn a UI timeout into charging completion.
inline Step charging_step(unsigned phase,bool full_pull) noexcept {
    switch(phase){
    case 0:return Step::pull_charging_handle;
    case 1:return full_pull?Step::release_charging:Step::pull_charging_handle;
    case 2:case 3:return Step::wait_return;
    default:return Step::none;
    }
}
inline bool empty_magazine_hint(int capacity,int used,int spare,bool infinite) noexcept {
    // An empty weapon needs a discoverable first step before the first grab.
    // Loaded/finished has no idle removal prompt; invalid counts stay unknown.
    return capacity>0&&capacity<=10000&&used==capacity&&spare>=0&&spare<=1000000&&
        (spare>0||infinite);
}
// Owner-marked front handle. The B receiver has no player interaction.
// Contact, original vertices and guide share this one profile.
// Hash-verified original LOD0 b_AR_Magazine rigid faces (156 triangles).
// A cosmetic empty-port outline, not an enlarged interaction/collision region.
inline constexpr V magazine_min_raw{-2.2875463963,22.3767566681,-6.9506421089};
inline constexpr V magazine_max_raw{2.2875463963,38.7670059204,4.3254747391};
inline V add(V a,V b) noexcept {return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V scale(V a,double s) noexcept {return {a.x*s,a.y*s,a.z*s};}
inline V point(std::array<float,16> const& m,V p) noexcept {
    return {m[12]+p.x*m[0]+p.y*m[4]+p.z*m[8],
            m[13]+p.x*m[1]+p.y*m[5]+p.z*m[9],
            m[14]+p.x*m[2]+p.y*m[6]+p.z*m[10]};
}
// Closely spaced rails form a readable border at the runtime's full scale1
// view size. The previous three thin rails disappeared in the recorded view.
// The frame is cosmetic; it never enlarges the real contact volume.
inline bool marker(V center,V right,V up,double half,Shape& out) noexcept {
    const auto norm=[](V p){return std::hypot(std::hypot(p.x,p.y),p.z);};
    if(!gears_vr::finite(center)||!gears_vr::finite(right)||!gears_vr::finite(up)||
       std::abs(norm(right)-1)>.001||std::abs(norm(up)-1)>.001||
       std::abs(right.x*up.x+right.y*up.y+right.z*up.z)>.001||
       !std::isfinite(half)||half<.1||half>100)return false;
    Shape s{};
    for(unsigned rail=0;rail<border_rails;++rail){
        const double radius=half*(1.+double(rail)*.02);std::array<V,4> p{};
        for(unsigned i=0;i<4;++i)p[i]=add(center,add(scale(right,i&1?radius:-radius),scale(up,i&2?radius:-radius)));
        for(auto const edge:{std::array<unsigned,2>{0,1},{1,3},{3,2},{2,0}})
            s.lines[s.count++]={p[edge[0]],p[edge[1]]};
    }
    out=s;return true;
}
inline bool magazine(std::array<float,16> const& matrix,Step step,Shape& out) noexcept {
    LancerFeedWorld feed{};
    if((step!=Step::remove_magazine&&step!=Step::insert_magazine)||!lancer_feed_world(matrix,feed))return false;
    Shape shape{};
    // Bold side-face frames, rather than a one-pixel wire cage through the
    // receiver. A close rail band forms a physical-width border at scale1;
    // both actual sides remain depth-tested. No enlarged contact volume.
    for(double side:{-1.,1.})for(unsigned rail=0;rail<border_rails;++rail){
        const double pad=.55+.10*rail,x=side*(magazine_max_raw.x+.85);
        std::array<V,4> corners{};
        for(unsigned i=0;i<4;++i)corners[i]=point(matrix,{x,
            i&1?magazine_max_raw.y+pad:magazine_min_raw.y-pad,
            i&2?magazine_max_raw.z+pad:magazine_min_raw.z-pad});
        for(auto const edge:{std::array<unsigned,2>{0,1},{1,3},{3,2},{2,0}})
            shape.lines[shape.count++]={corners[edge[0]],corners[edge[1]]};
    }
    // Original fixed port and extraction axis, never the detached magazine.
    const auto center=scale(add(magazine_min_raw,magazine_max_raw),.5);
    const double sign=step==Step::remove_magazine?-1.:1.;
    const auto from=add(center,{0,0,sign<0?-7.: -13.});
    const auto to=add(from,{0,0,sign*6.});
    // Keep the direction arrow on those visible side faces too; the former
    // centre-line arrow was easily hidden behind the gun or carrying hand.
    for(double face:{-1.,1.}){const V offset{face*(magazine_max_raw.x+.85),0,0};
        shape.lines[shape.count++]={point(matrix,add(from,offset)),point(matrix,add(to,offset))};
        for(double side:{-1.,1.})shape.lines[shape.count++]={point(matrix,add(to,offset)),
            point(matrix,add(add(to,offset),{0,side*1.6,-sign*1.8}))};
    }
    for(unsigned i=0;i<shape.count;++i)if(!gears_vr::finite(shape.lines[i].start)||!gears_vr::finite(shape.lines[i].end))return false;
    out=shape;return true;
}
inline bool charging(std::array<float,16> const& matrix,double distance_raw,Step step,Shape& out) noexcept {
    LancerFeedWorld feed{};
    if(!lancer_feed_world(matrix,feed)||!std::isfinite(distance_raw)||distance_raw<0||
       distance_raw>lancer_front::travel_raw+.001||
       (step!=Step::pull_charging_handle&&step!=Step::release_charging&&step!=Step::wait_return))return false;
    Shape shape{};
    // Thick side-face outline around the actual small front handle, following
    // its observed translation. B and the old cap are never outlined.
    for(unsigned rail=0;rail<border_rails;++rail){
        const double pad=.55+.10*rail;std::array<V,4> p{};
        for(unsigned i=0;i<4;++i)p[i]=point(matrix,{lancer_front::maximum_raw.x+.6,
            (i&1?lancer_front::maximum_raw.y+pad:lancer_front::minimum_raw.y-pad)-distance_raw,
            i&2?lancer_front::maximum_raw.z+pad:lancer_front::minimum_raw.z-pad});
        for(auto const edge:{std::array<unsigned,2>{0,1},{1,3},{3,2},{2,0}})
            shape.lines[shape.count++]={p[edge[0]],p[edge[1]]};
    }
    const auto contact=add(lancer_front::contact_raw,{.8,-distance_raw,0});
    if(step==Step::release_charging){
        // Outward arrows mean release at full travel, not another pull.
        for(double direction:{-1.,1.}){
            const auto end=add(contact,{0,0,direction*3.});
            shape.lines[shape.count++]={point(matrix,contact),point(matrix,end)};
            for(double side:{-1.,1.})shape.lines[shape.count++]={point(matrix,end),point(matrix,add(end,{0,side*.8,-direction*1.}))};
        }
    }else{
        const double direction=step==Step::pull_charging_handle?-1.:1.;
        const double remaining=step==Step::pull_charging_handle?
            lancer_front::travel_raw-distance_raw:distance_raw;
        const auto end=add(contact,{0,direction*std::max(.5,remaining),0});
        shape.lines[shape.count++]={point(matrix,contact),point(matrix,end)};
        for(double side:{-1.,1.})shape.lines[shape.count++]={point(matrix,end),point(matrix,add(end,{0,-direction*1.2,side*.8}))};
    }
    for(unsigned i=0;i<shape.count;++i)if(!gears_vr::finite(shape.lines[i].start)||!gears_vr::finite(shape.lines[i].end))return false;
    out=shape;return true;
}
}
