#include "weapon_longshot_foregrip.h"
#include <cassert>
#include <cstdio>
int main(){
    namespace fg=gears_weapon_longshot_foregrip;
    namespace vr=gears_vr;
    gears_hand_authored_mount::Mount left{};vr::Vector grip{};
    assert(fg::grip_in_mesh(grip));
    assert(gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
        gears_hand_authored_mount::Side::left,1,100,left));
    const auto rotation=vr::rotation_matrix(fg::wrist_in_mesh());
    const auto shift=vr::transform(rotation,vr::transform(vr::transpose(vr::rotation_matrix(left.calibration.grip_to_wrist_rotation)),
        {-100*left.calibration.grip_to_wrist_metres.x,-100*left.calibration.grip_to_wrist_metres.y,-100*left.calibration.grip_to_wrist_metres.z}));
    const auto palm=vr::transform(rotation,fg::palm_surface_raw);
    assert(std::abs(grip.x-shift.x+palm.x-fg::surface_raw.x)<1e-10);
    assert(std::abs(grip.y-shift.y+palm.y-fg::surface_raw.y)<1e-10);
    assert(std::abs(grip.z-shift.z+palm.z-fg::surface_raw.z)<1e-10);
    const auto q=fg::wrist_in_mesh();
    assert(std::abs(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w-1)<1e-10);
    const auto box=fg::contact_box({{{1,0,0},{0,1,0},{0,0,1}}});
    assert(box.valid());
    for(auto s:{-1.,1.}){
        assert(box.contains({s*.08,s*.06,-s*.06},false));
        assert(!box.contains({s*.080001,0,0},false));
        assert(!box.contains({0,s*.060001,0},false));
        assert(!box.contains({0,0,s*.060001},false));
        assert(box.contains({s*.10,s*.08,-s*.08},true));
        assert(!box.contains({s*.100001,0,0},true));
    }
    auto bad=box;bad.axes[0][0]=2;assert(!bad.contains({},false));
    bad=box;bad.retain_half.x=.01;assert(!bad.valid());
    const vr::Matrix turn{{{0,-1,0},{1,0,0},{0,0,1}}};
    const auto rotated=fg::contact_box(turn);
    assert(rotated.valid()&&rotated.contains({0,0,.08},false)&&!rotated.contains({.080001,0,0},false));
    std::puts("Longshot foregrip authored palm/contact transform checks passed; live fit pending");
}
