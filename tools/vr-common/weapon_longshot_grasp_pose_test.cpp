#include "weapon_longshot_grasp_pose.h"
#include "hand_authored_mount_profile.h"
#include <cassert>
#include <cstdio>
#include <limits>
namespace g=gears_weapon_longshot_grasp;
namespace v=gears_vr;
v::Vector rotated(v::Quaternion q,v::Vector p){return v::transform(v::rotation_matrix(q),p);}
void close(v::Vector a,v::Vector b){assert(std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z)<1e-9);}
int main(){
    for(double units:{10.,100.,500.}){
        gears_hand_authored_mount::Mount anatomy{};
        assert(gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
            gears_hand_authored_mount::Side::left,1,units,anatomy));
        const auto offset=anatomy.calibration.grip_to_wrist_metres;
        const auto calibration=anatomy.calibration.grip_to_wrist_rotation;
        v::Pose round{};assert(g::cartridge_mount(offset,calibration,units,round));
        // Independent landmark reconstruction: round case +Y and wrist pad
        // must coincide under several arbitrary world rotations/translations.
        for(v::Quaternion q:{v::Quaternion{},v::normalized({.3,.2,-.4,.7}),v::Quaternion{1,0,0,0}}){
            const v::Vector origin{101,-205,33};
            const auto case_local=g::add(round.position,rotated(round.orientation,{0,g::cartridge_contact_from_primer_m*units,0}));
            const auto pad_local=g::scale(g::add(offset,rotated(calibration,g::precision_contact_in_wrist_m)),units);
            close(g::add(origin,rotated(q,case_local)),g::add(origin,rotated(q,pad_local)));
            close(rotated(round.orientation,{0,1,0}),rotated(calibration,{1,0,0}));
            for(v::Vector axis:{v::Vector{1,0,0},v::Vector{0,0,1},v::Vector{.2,-.8,.1}}){
                const v::Vector contact{14,53,-7};const v::Pose physical{{31,2,-3},q};v::Pose goal{};
                assert(g::cylinder_grip(contact,axis,physical,offset,calibration,units,goal));
                const auto wrist=v::normalized(v::multiply(goal.orientation,calibration));
                const auto wrist_position=g::add(goal.position,rotated(goal.orientation,g::scale(offset,units)));
                close(g::add(wrist_position,rotated(wrist,g::scale(g::cylinder_contact_in_wrist_m,units))),contact);
                assert(g::normalize(axis));const auto row=rotated(wrist,{0,0,1});
                assert(std::abs(std::abs(g::dot(row,axis))-1)<1e-10);
                // Solving a skin goal never changes the physical source pose.
                close(physical.position,{31,2,-3});
            }
        }
    }
    v::Pose out{{9,8,7},{}};const auto nan=std::numeric_limits<double>::quiet_NaN();
    assert(!g::cartridge_mount({}, {},9,out));assert(!g::cartridge_mount({}, {},501,out));
    assert(!g::cartridge_mount({nan,0,0},{},100,out));assert(!g::cartridge_mount({}, {0,0,0,0},100,out));
    assert(!g::cylinder_grip({}, {},{}, {},{},100,out));
    assert(!g::cylinder_grip({nan,0,0},{1,0,0},{}, {},{},100,out));
    assert(!g::cylinder_grip({}, {1,0,0},{{},{nan,0,0,1}}, {},{},100,out));
    close(out.position,{9,8,7});
    std::puts("Longshot grasp geometry: shared rendered/physical cartridge pad, cylinder axis/contact, calibrated world invariance and invalid-input rejection passed; no runtime/skin acceptance");
}
