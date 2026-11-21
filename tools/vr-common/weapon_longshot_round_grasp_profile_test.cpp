#include "weapon_longshot_round_grasp_profile.h"
#include "weapon_longshot_round_grasp_fixture.h"
#include "hand_authored_mount_profile.h"
#include "hand_authored_grasp.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace vr=gears_vr;namespace profile=gears_weapon_longshot_round_grasp;
namespace fixture=gears_longshot_round_fixture;namespace g=gears_weapon_longshot_grasp;
using P=vr::Pose;using V=vr::Vector;
V rotated(vr::Quaternion q,V v){return vr::transform(vr::rotation_matrix(q),v);}
P compose(P const& a,P const& b){return {g::add(a.position,rotated(a.orientation,b.position)),vr::normalized(vr::multiply(a.orientation,b.orientation))};}
double length(V v){return std::hypot(std::hypot(v.x,v.y),v.z);}
bool near(V a,V b,double tolerance=1e-8){return length(g::sub(a,b))<tolerance;}
int main(){
#ifdef _WIN32
    _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
    std::array<P,16> joints{};
    for(unsigned i=1;i<16;++i){assert(fixture::parent[i]<i);
        joints[i]=compose(joints[fixture::parent[i]],{fixture::translation[i],profile::closed[i-1]});}
    std::array<V,2> points{},normals{};
    for(unsigned i=0;i<2;++i){double total{};
        for(auto const& a:fixture::witnesses[i]){assert(a.bone<16&&a.weight>=0&&a.weight<=1);
            points[i]=g::add(points[i],g::scale(g::add(joints[a.bone].position,rotated(joints[a.bone].orientation,a.point)),a.weight));
            normals[i]=g::add(normals[i],g::scale(rotated(joints[a.bone].orientation,a.normal),a.weight));total+=a.weight;}
        assert(std::abs(total-1)<1e-12&&g::normalize(normals[i]));}
    // Independent stored original-skin reconstruction from the reviewed IK.
    assert(near(points[0],{11.015976127155113,-5.000083402992075,-5.499989814017432}));
    assert(near(points[1],{8.966184134650579,-5.0138028996194635,-5.5178363529399554}));
    unsigned contacts{};
    for(double units:{50.,100.,200.}){
        gears_hand_authored_mount::Mount anatomy{};
        assert(gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
            gears_hand_authored_mount::Side::left,1,units,anatomy));
        P item_mount{};assert(profile::mount(anatomy.calibration.grip_to_wrist_metres,
            anatomy.calibration.grip_to_wrist_rotation,units,item_mount));
        for(auto rotation:{vr::Quaternion{},vr::normalized({.3,.2,-.4,.7}),vr::Quaternion{1,0,0,0},vr::normalized({-.4,.1,.8,.2})}){
            const P physical{{101,-205,33},rotation};
            const P wrist=compose(physical,{g::scale(anatomy.calibration.grip_to_wrist_metres,units),anatomy.calibration.grip_to_wrist_rotation});
            const P round=compose(physical,item_mount);
            const auto axis=rotated(round.orientation,{0,1,0});
            assert(near(axis,rotated(wrist.orientation,{0,0,-1})));
            const auto contact_axis=g::add(round.position,g::scale(axis,.025*units));
            for(unsigned i=0;i<2;++i){
                const auto skin=g::add(wrist.position,rotated(wrist.orientation,g::scale(points[i],units/100)));
                const auto delta=g::sub(skin,contact_axis);const double depth=g::dot(delta,axis);
                auto radial=g::sub(delta,g::scale(axis,depth));const double radius=length(radial)/units;
                const double gap=radius-fixture::case_radius_m;
                assert(gap>=0&&gap<=.003&&std::abs(depth)/units<.001);
                assert(g::normalize(radial));
                const auto skin_normal=rotated(wrist.orientation,normals[i]);
                assert(g::dot(skin_normal,g::scale(radial,-1))>.90);
                ++contacts;
            }
            // The authored display mount does not alter its physical input.
            assert(near(physical.position,{101,-205,33}));
        }
    }
    P output{{9,8,7},{}};const auto sentinel=output;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    assert(!profile::mount({}, {},9,output)&&!profile::mount({}, {},501,output));
    assert(!profile::mount({nan,0,0},{},100,output)&&!profile::mount({}, {0,0,0,0},100,output));
    assert(near(output.position,sentinel.position)&&output.orientation.w==sentinel.orientation.w);
    gears_hand_authored_grasp::Request q{};
    q.geometry_signature=gears_hand_authored_mount::marcus115_signature;q.profile_id=profile::profile_id;
    q.side=gears_hand_authored_grasp::Side::left;q.mode=gears_hand_authored_grasp::Mode::left_part;
    q.kind=gears_hand_object_grasp::Kind::magazine;q.reference=profile::reference;q.closed=profile::closed;
    assert(gears_hand_authored_grasp::structural(q));
    assert(!gears_weapon_hand::frame_active(q.source,1)); // Shape validity grants no source/lease authority.
    q.closed[0]={0,0,0,0};assert(!gears_hand_authored_grasp::structural(q));
    std::printf("Longshot offline pinch candidate: %u original-skin contacts, same mount/axis across units/world rotations, invalid atomicity and V4 shape-only boundary passed; no runtime/whole-skin acceptance\n",contacts);
}
