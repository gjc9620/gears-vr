#include "weapon_longshot_eye_contact.h"
#include <cassert>
#include <cstdio>
namespace eye=gears_weapon::longshot_eye_contact;
namespace scope=gears_weapon::longshot_scope_control;
using P=gears_vr::Pose;
int main(){
    P ocular{};assert(eye::ocular_world_m({},1,100,ocular));
    assert(std::abs(ocular.position.x-eye::ocular_center_raw.x/100)<1e-10);
    P head=ocular;head.position.y-=.06;
    auto c=eye::contact_ue(ocular,head,true);assert(c.tracked&&std::abs(c.distance_m-.06)<1e-10&&c.lateral_m<1e-10&&c.angle_degrees<1e-5&&scope::in_eye_box(c,false));
    head.position.x+=.026;assert(!scope::in_eye_box(eye::contact_ue(ocular,head,true),false));
    assert(scope::in_eye_box(eye::contact_ue(ocular,head,true),true));
    head=ocular;head.position.y+=.06;assert(!scope::in_eye_box(eye::contact_ue(ocular,head,true),false));
    head=ocular;head.position.y-=.06;head.orientation={};assert(!scope::in_eye_box(eye::contact_ue(ocular,head,true),false));
    assert(!eye::contact_ue(ocular,head,false).tracked);
    // A translated/rotated gun and eye give identical relative contact.
    P mesh{{1000,-2000,3000},{0,0,.7071067811865475,.7071067811865475}},moved{};
    assert(eye::ocular_world_m(mesh,1,100,moved));
    P moved_head=moved;const auto forward=gears_vr::transform(gears_vr::rotation_matrix(moved.orientation),{1,0,0});
    moved_head.position={moved.position.x-.06*forward.x,moved.position.y-.06*forward.y,moved.position.z-.06*forward.z};
    c=eye::contact_ue(moved,moved_head,true);assert(std::abs(c.distance_m-.06)<1e-10&&c.lateral_m<1e-10&&c.angle_degrees<1e-5);
    P untouched{{1,2,3},{}};auto before=untouched;
    assert(!eye::ocular_world_m(mesh,0,100,untouched)&&gears_vr::stereo_same_pose(before,untouched));
    assert(!eye::ocular_world_m(mesh,1,1,untouched));
    std::puts("Longshot original ocular mount: UE/XR axis distinction, scale, behind-lens relief, lateral/angle and world invariance passed");
}
