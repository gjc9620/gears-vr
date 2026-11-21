#include "weapon_longshot_chamber_snap.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
namespace s=gears_weapon_longshot_chamber_snap;
using V=s::V;using P=s::P;using Q=s::Q;
constexpr double pi=3.14159265358979323846;
V point(P root,V p){return s::add(root.position,s::vr::transform(s::vr::rotation_matrix(root.orientation),p));}
P item_at(P root,V port,double scale,double units,double depth,double lateral){
    const Q q=s::vr::normalized(s::vr::multiply(root.orientation,{0,0,std::sin(-pi/4),std::cos(-pi/4)}));
    const V tip=point(root,s::add(s::mul(port,scale*units),{-depth*units,lateral*units,0}));
    const V axis=s::vr::transform(s::vr::rotation_matrix(q),{0,1,0});
    return {s::sub(tip,s::mul(axis,s::cartridge_length_m*units)),q};
}
int main(){
#ifdef _WIN32
    _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
    unsigned checks{};
    const P root{{98,-42,170},{0,0,std::sin(.35),std::cos(.35)}};const V port{0,.26,.105};
    for(double scale:{.5,1.,2.})for(double units:{50.,100.,200.}){
        s::Alignment out{};
        assert(s::align(item_at(root,port,scale,units,.03,.04),root,port,scale,units,false,out));
        assert(std::abs(out.raw_depth_m-.03)<1e-12&&std::abs(out.raw_lateral_m-.04)<1e-12);++checks;
        const auto tip=s::add(out.item.position,s::vr::transform(s::vr::rotation_matrix(out.item.orientation),{0,s::cartridge_length_m*units,0}));
        const auto expected=point(root,s::add(s::mul(port,scale*units),{-.03*units,0,0}));
        const auto difference=s::sub(tip,expected);assert(std::hypot(std::hypot(difference.x,difference.y),difference.z)<1e-10);++checks;
        assert(!s::align(item_at(root,port,scale,units,.03,.060001),root,port,scale,units,false,out));++checks;
        assert(!s::align(item_at(root,port,scale,units,.080001,.02),root,port,scale,units,false,out));++checks;
        assert(!s::align(item_at(root,port,scale,units,-.015001,.02),root,port,scale,units,false,out));++checks;
        assert(s::align(item_at(root,port,scale,units,.09,.07),root,port,scale,units,true,out));++checks;
        assert(!s::align(item_at(root,port,scale,units,.09,.080001),root,port,scale,units,true,out));++checks;
        assert(!s::align(item_at(root,port,scale,units,.100001,.07),root,port,scale,units,true,out));++checks;
    }
    std::printf("LongshotChamberPhysicalRangeTest: %u checks; fixed physical cm across model scale/units/root, raw depth and actual tip preserved; no game\n",checks);
}
