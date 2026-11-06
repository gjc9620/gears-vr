#include "weapon_two_hand_hold.h"
#include <cassert>
#include <cstdio>
int main(){
    namespace h=gears_weapon_two_hand;namespace v=gears_vr;
    const v::Pose mount{{.35,0,0},{1,0,0,0}},right{{10,20,30},{0,0,0,1}};
    v::Pose left{{45,20,30},{0,0,0,1}};h::Solution s{};
    assert(h::solve(right,left,mount,100,s));assert(h::length(h::sub(s.right.position,right.position))==0);
    assert(h::length(h::sub(s.left.position,left.position))<1e-9);
    left.position={40,30,35};assert(h::solve(right,left,mount,100,s));
    const auto actual=h::sub(left.position,right.position),expected=h::sub(s.left.position,s.right.position);
    const auto cross=v::Vector{actual.y*expected.z-actual.z*expected.y,actual.z*expected.x-actual.x*expected.z,actual.x*expected.y-actual.y*expected.x};
    assert(h::length(cross)<1e-8&&std::abs(h::length(expected)-35)<1e-8);
    // Moving the right hand translates both contacts; the head is no input.
    auto moved=right;moved.position.z+=8;left.position.z+=8;h::Solution translated{};
    assert(h::solve(moved,left,mount,100,translated));assert(std::abs(translated.left.position.z-s.left.position.z-8)<1e-9);
    auto invalid=mount;invalid.position={};assert(!h::solve(right,left,invalid,100,s));
    left.position={-25,20,30};assert(!h::solve(right,left,mount,100,s));
    std::puts("Two-hand geometry: fixed right contact, actual off-hand steering, translation, opposite/invalid rejection passed; CPU only");
}
