#include "weapon_boomshot_guide.h"
#include <cassert>
#include <cstdio>
#include <limits>
namespace g=gears_boomshot_guide;namespace m=gears_boomshot_hand_mount;namespace vr=gears_vr;
int main(){unsigned checks{};const auto check=[&](bool value){assert(value);++checks;};
    gears_weapon::boomshot_insertion::Geometry geometry{};geometry.id=1;geometry.verified=true;
    geometry.seated_in_root_m={{0,.30,.02},{1,0,0,0}};geometry.outward={0,0,-1};geometry.travel_m=.155;
    geometry.approach_margin_m=.01;geometry.capture_radius_m=.012;geometry.retain_radius_m=.018;
    geometry.capture_angle_degrees=12;geometry.retain_angle_degrees=18;geometry.seat_tolerance_m=.002;
    const vr::Pose root{{100,200,300},{}},shoulder{{400,500,600},{}};g::Shape shape{};
    for(double scale:{.5,1.,2.})for(double units:{30.,100.,400.}){
        const vr::Pose barrel{m::point(root,gears_weapon::boomshot_barrel::closed_raw,scale),root.orientation};
        for(auto step:{g::Step::opening,g::Step::closing,g::Step::shoulder,g::Step::entry}){
            check(g::make({step,false},root,barrel,scale,units,shoulder,geometry,shape));
            check(shape.count==(step==g::Step::entry?18u:step==g::Step::shoulder?14u:17u));
            for(unsigned i=0;i<shape.count;++i)check(vr::finite(shape.lines[i].start)&&vr::finite(shape.lines[i].end));
            if(step==g::Step::opening||step==g::Step::closing){
                const auto center=m::point(root,m::mechanism_surface_closed_raw,scale);
                check(std::abs(shape.lines[12].start.y-center.y)<1e-9);
                check((shape.lines[12].end.y-shape.lines[12].start.y)*(step==g::Step::opening?-1:1)>0);
            }
        }
        check(g::make({g::Step::entry,true},root,barrel,scale,units,shoulder,geometry,shape)&&shape.count==32);
        const auto& approach=shape.lines[13];
        check(std::abs(approach.end.y-(root.position.y+.30*scale*units))<1e-9);
        check(std::abs(approach.end.z-(root.position.z+(.02-.155)*scale*units))<1e-9);
        check(approach.start.z<approach.end.z); // Upward toward the actual entry, not downward away.
        const auto radius=geometry.capture_radius_m*scale*units;
        check(std::abs(std::sqrt(m::dot(m::sub(shape.lines[0].start,approach.end),m::sub(shape.lines[0].start,approach.end)))-radius)<1e-9);
    }
    auto turned=root;turned.orientation=vr::normalized(vr::Quaternion{0,0,.7071067811865475,.7071067811865475});
    vr::Pose barrel{m::point(turned,gears_weapon::boomshot_barrel::closed_raw,1),turned.orientation};
    check(g::make({g::Step::opening},turned,barrel,1,100,shoulder,geometry,shape));
    check(shape.lines[12].end.x>shape.lines[12].start.x);
    for(double invalid:{0.,std::numeric_limits<double>::quiet_NaN(),11.})check(!g::make({g::Step::entry},root,barrel,invalid,100,shoulder,geometry,shape)&&!shape.count);
    geometry.verified=false;check(!g::make({g::Step::entry},root,barrel,1,100,shoulder,geometry,shape)&&!shape.count);
    check(!g::make({},root,barrel,1,100,shoulder,geometry,shape)&&!shape.count);
    geometry.verified=true;
    for(auto kind:{g::Result::opened,g::Result::seated,g::Result::closed}){
        check(g::make({g::Step::entry,true},root,barrel,1,100,shoulder,geometry,shape));const auto yellow=shape.count;
        check(g::result(kind,root,barrel,1,100,geometry,shape,{})&&shape.count<=40&&shape.count>yellow);
        for(unsigned i=0;i<shape.count;++i)check(i<yellow?
            shape.lines[i].red==255&&shape.lines[i].green==220&&shape.lines[i].blue==0:
            shape.lines[i].red==40&&shape.lines[i].green==235&&shape.lines[i].blue==120);
    }
    shape={};geometry.verified=false;check(!g::result(g::Result::seated,root,barrel,1,100,geometry,shape,{})&&!shape.count);
    shape.count=41;check(!g::result(g::Result::closed,root,barrel,1,100,geometry,shape,{}));
    geometry.verified=true;
    const vr::Quaternion observers[]={{},{0,0,.7071067811865475,.7071067811865475},{0,-.7071067811865475,0,.7071067811865475}};
    const auto fixed_center=g::add(m::point(root,geometry.seated_in_root_m.position,100),{0,0,5.5});
    for(const auto& observer:observers){shape={};check(g::result(g::Result::seated,root,barrel,1,100,geometry,shape,observer));
        const auto forward=m::rotate(observer,{1,0,0});vr::Vector center{};
        for(unsigned i=0;i<shape.count;++i){center=g::add(center,m::mul(shape.lines[i].start,.25));
            check(std::abs(m::dot(m::sub(shape.lines[i].start,fixed_center),forward))<1e-9);}
        check(m::dot(m::sub(center,fixed_center),m::sub(center,fixed_center))<1e-18);
    }
    std::printf("Boomshot guide geometry: %u checks passed; current barrel arrows, distinct entry/capture, exact production root-scale/units normalization and authored direction key; no Engine/pixels/action authority\n",checks);
}
