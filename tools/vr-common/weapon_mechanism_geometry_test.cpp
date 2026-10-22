#include "weapon_mechanism_geometry.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace gears_vr;
using namespace gears_weapon;

static bool close(double a,double b){return std::abs(a-b)<1e-6;}

int main(){
    // Synthetic geometry exercises coordinate transforms; these values are not
    // asserted to be the original Gnasher mount or hinge axis.
    LeverMount mount{};
    mount.left_front_grip_in_main.position={0,0,-.2};
    mount.pivot_in_main={0,0,0};
    mount.right_handle_home_in_main={0,-.2,0};
    mount.hinge_axis_in_main={1,0,0};
    assert(mount.valid());
    StereoSample stereo{};stereo.session=7;stereo.sequence=2;
    stereo.space_epoch=3;stereo.predicted_time=1000;stereo.focused=1;
    stereo.head={{0,1.6,0},{0,0,0,1}};
    ControllerInputFrame hands{};hands.session=7;hands.sequence=2;
    hands.space_epoch=3;hands.predicted_time=1000;hands.focused=1;
    hands.sync_ns=999999900;hands.received_ns=1000000000;
    for(auto& hand:hands.hands){hand.grip_active=1;hand.grip_flags=15;}
    hands.hands[0].grip.position={0,1,-.2};
    hands.hands[1].grip.position={0,.8,0};
    LeverPoseSample sample{};
    assert(gnasher_lever_sample(hands,stereo,mount,10,20,1000000100,true,true,sample));
    assert(close(sample.lever_angle_radians,0)&&sample.weapon==10&&sample.world==20);
    hands.hands[1].grip.position={0,1,-.2};
    assert(gnasher_lever_sample(hands,stereo,mount,10,20,1000000100,true,true,sample));
    assert(close(sample.lever_angle_radians,1.5707963267948966));
    hands.hands[1].grip.position={0,1-.2*std::cos(2.5),-.2*std::sin(2.5)};
    assert(gnasher_lever_sample(hands,stereo,mount,10,20,1000000100,true,true,sample));
    assert(close(sample.lever_angle_radians,2.5));
    hands.hands[1].grip.position={0,1,-.3}; // wrong radius
    assert(!gnasher_lever_sample(hands,stereo,mount,10,20,1000000100,true,true,sample));
    hands.hands[1].grip.position={.1,1,-.2}; // wrong axial offset
    assert(!gnasher_lever_sample(hands,stereo,mount,10,20,1000000100,true,true,sample));
    hands.hands[1].grip.position={0,1-.2*std::cos(2.5),-.2*std::sin(2.5)};
    assert(!gnasher_lever_sample(hands,stereo,mount,10,20,1000000100,false,true,sample));
    stereo.head.position={3,1.8,4}; // HMD motion cannot change the hinge.
    assert(gnasher_lever_sample(hands,stereo,mount,10,20,1000000100,true,true,sample));
    assert(close(sample.lever_angle_radians,2.5));
    hands.hands[1].grip_flags=3;
    assert(!gnasher_lever_sample(hands,stereo,mount,10,20,1000000100,true,true,sample));
    hands.hands[1].grip_flags=15;stereo.sequence=3;
    assert(!gnasher_lever_sample(hands,stereo,mount,10,20,1000000100,true,true,sample));
    // Feed real hand positions through the geometric adapter and then the
    // already ordered down-and-return cycle recognizer.
    constexpr LeverTravel travel{.1,.8,15.,80000000,1000000000,100000000};
    GnasherLever lever;
    assert(lever.arm(10,20,3));
    auto tick=1000000100LL;
    const double stroke[]={0,.2,.4,.6,.85,.6,.4,.2,0};
    bool completed=false;
    for(double angle:stroke){
        ++hands.sequence;stereo.sequence=hands.sequence;
        tick+=20000000;
        hands.received_ns=tick-100;hands.sync_ns=tick-200;
        hands.hands[1].grip.position={0,1-.2*std::cos(angle),-.2*std::sin(angle)};
        assert(gnasher_lever_sample(hands,stereo,mount,10,20,tick,true,true,sample));
        completed=lever.observe(sample,travel);
    }
    assert(completed&&lever.phase()==LeverPhase::unarmed);
    // The same stroke is measured relative to the supporting hand when the
    // whole weapon has changed world orientation and position.
    const Quaternion yaw{0,std::sin(.5),0,std::cos(.5)};
    const Vector base{.3,1.2,-.4};
    hands.hands[0].grip.orientation=yaw;
    hands.hands[0].grip.position=sum(base,rotate(yaw,mount.left_front_grip_in_main.position));
    hands.hands[1].grip.position=sum(base,rotate(yaw,{0,-.2*std::cos(.5),-.2*std::sin(.5)}));
    ++hands.sequence;stereo.sequence=hands.sequence;tick+=20000000;
    hands.received_ns=tick-100;hands.sync_ns=tick-200;
    assert(gnasher_lever_sample(hands,stereo,mount,10,20,tick,true,true,sample));
    assert(close(sample.lever_angle_radians,.5));
    // The purchased game's lever moves about -X through 32.4 degrees. Use
    // its measured pivot and resting pawn hand-bone origins for a synthetic
    // controller arc. True controller offsets and mesh contact are unknown.
    LeverMount original_axis_mount=mount;
    original_axis_mount.hinge_axis_in_main=gnasher_lever_reference_axis;
    const auto pivot_metres=scaled(gnasher_lever_reference_pivot_raw,.01);
    assert(std::abs(pivot_metres.y-.2062220742)<1e-8);
    original_axis_mount.left_front_grip_in_main.position=
        scaled(gnasher_idle_left_hand_bone_raw,.01);
    original_axis_mount.pivot_in_main=pivot_metres;
    original_axis_mount.right_handle_home_in_main=
        scaled(gnasher_idle_right_hand_bone_raw,.01);
    assert(original_axis_mount.valid());
    const auto arm=difference(original_axis_mount.right_handle_home_in_main,pivot_metres);
    assert(std::hypot(arm.y,arm.z)>.38&&std::hypot(arm.y,arm.z)<.40);
    hands.hands[0].grip.orientation={0,0,0,1};
    hands.hands[0].grip.position=sum({0,1,0},
        original_axis_mount.left_front_grip_in_main.position);
    GnasherLever original_cycle;
    assert(original_cycle.arm(10,20,3));
    const double original_stroke[]={0,.1,.2,.34,.48,.34,.2,.1,0};
    bool original_completed=false;
    for(double angle:original_stroke){
        ++hands.sequence;stereo.sequence=hands.sequence;tick+=20000000;
        hands.received_ns=tick-100;hands.sync_ns=tick-200;
        const Vector bent{arm.x,
            arm.y*std::cos(angle)+arm.z*std::sin(angle),
            -arm.y*std::sin(angle)+arm.z*std::cos(angle)};
        hands.hands[1].grip.position=sum({0,1,0},sum(pivot_metres,bent));
        assert(gnasher_lever_sample(hands,stereo,original_axis_mount,10,20,tick,
            true,true,sample));
        original_completed=original_cycle.observe(sample,original_gnasher_lever_travel);
    }
    assert(original_completed);
    std::puts("Gnasher lever hand geometry, radius, tracking and HMD independence passed");
}
