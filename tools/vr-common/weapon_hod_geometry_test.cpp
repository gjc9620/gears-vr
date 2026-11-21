#include "weapon_hod_geometry.h"
#include <cassert>
#include <cmath>
#include <cstdio>
namespace g=gears_weapon_hod_geometry;namespace v=gears_vr;
int main(){
    const v::Matrix identity{{{1,0,0},{0,1,0},{0,0,1}}};
    const auto mount=g::mesh_in_hand(identity);auto forward=v::transform(mount,g::axis);
    assert(std::abs(forward.x-1)<1e-12&&std::abs(forward.y)<1e-12&&std::abs(forward.z)<1e-12);
    auto up=v::transform(mount,{0,0,1});assert(up.z==1&&up.x==0&&up.y==0);
    auto initial=g::direction({0,0,0});assert(initial.x==0&&initial.y==1&&initial.z==0);
    constexpr double pi=3.14159265358979323846;
    auto direction=g::direction({0,-pi/2,0});assert(std::abs(direction.x-1)<1e-12&&std::abs(direction.y)<1e-12);
    // A free view rotation has no input into the weapon-local relation.
    const auto gun=v::ue_matrix({.1,.3,.2});auto d=v::transform(gun,g::axis);
    assert(std::isfinite(d.x)&&std::abs(d.x*d.x+d.y*d.y+d.z*d.z-1)<1e-12);
    std::puts("HOD geometry: original +Y axis and one mesh/query mounting correction passed (offline only)");
}
