#include "weapon_pose_solver.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace gears_vr;
using namespace gears_weapon;

static bool close(double a,double b){return std::abs(a-b)<1e-6;}
static bool close(Vector a,Vector b){return close(a.x,b.x)&&close(a.y,b.y)&&close(a.z,b.z);}

int main(){
    WeaponMount mount{};mount.support_grip_in_weapon={0,0,-.5};
    mount.muzzle_in_weapon={0,0,-1};
    assert(mount.valid());
    StereoSample stereo{};stereo.session=7;stereo.sequence=2;
    stereo.space_epoch=3;stereo.predicted_time=1000;stereo.focused=1;
    stereo.head={{0,1.6,0},{0,0,0,1}};
    ControllerInputFrame hands{};hands.session=7;hands.sequence=2;
    hands.space_epoch=3;hands.predicted_time=1000;hands.focused=1;
    hands.sync_ns=999999900;hands.received_ns=1000000000;
    for(auto& hand:hands.hands){hand.grip_active=1;hand.grip_flags=15;}
    hands.hands[1].grip.position={0,1,0};
    hands.hands[0].grip.position={0,1,-.5};
    WeaponPoseSolution one{},two{};
    assert(solve_weapon_pose(hands,stereo,mount,false,1000000100,one));
    assert(solve_weapon_pose(hands,stereo,mount,true,1000000100,two));
    assert(!one.support_used&&two.support_used);
    assert(close(one.gun.position,{0,1,0})&&close(one.muzzle,{0,1,-1}));
    assert(close(two.direction,{0,0,-1}));
    hands.hands[0].grip.position={.25,1,-.4330127018922193};
    assert(solve_weapon_pose(hands,stereo,mount,true,1000000100,two)&&two.support_used);
    assert(close(two.gun.position,hands.hands[1].grip.position));
    assert(close(two.direction,{.5,0,-.8660254037844386}));
    // The HMD may move independently while both hands and the gun remain still.
    stereo.head.position={2,1.8,1};
    WeaponPoseSolution head_moved{};
    assert(solve_weapon_pose(hands,stereo,mount,true,1000000100,head_moved));
    assert(close(head_moved.muzzle,two.muzzle)&&close(head_moved.direction,two.direction));
    const Pose reference{{0,0,0},{0,0,0,1}};
    const Viewpoint base{{10,20,30},{0,0,0}};
    WeaponAimLine aim{};
    assert(weapon_world_aim(one,reference,base,100,aim));
    assert(close(aim.start,{110,20,130})&&close(aim.direction,{1,0,0}));
    WeaponAimLine moved{};
    assert(weapon_world_aim(head_moved,reference,base,100,moved));
    stereo.sequence=3; // A different stereo ticket cannot consume this hand pose.
    WeaponPoseSolution unchanged=one;
    assert(!solve_weapon_pose(hands,stereo,mount,true,1000000100,unchanged));
    assert(close(unchanged.muzzle,one.muzzle));
    stereo.sequence=2;hands.hands[1].grip_flags=3;
    assert(!solve_weapon_pose(hands,stereo,mount,true,1000000100,unchanged));
    hands.hands[1].grip_flags=15;hands.hands[0].grip_flags=3;
    assert(solve_weapon_pose(hands,stereo,mount,true,1000000100,unchanged)&&!unchanged.support_used);
    assert(close(unchanged.direction,{0,0,-1}));
    std::puts("weapon hand mount, two-hand muzzle, head independence and ticket gates passed");
}
