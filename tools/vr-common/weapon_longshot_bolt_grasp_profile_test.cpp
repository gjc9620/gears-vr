#include "weapon_longshot_bolt_grasp_profile.h"
#include "weapon_longshot_bolt_contact.h"
#include "weapon_longshot_bolt_grasp_fixture.h"
#include "weapon_longshot_round_grasp_fixture.h"
#include "hand_authored_mount_profile.h"
#include "hand_authored_grasp.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace vr=gears_vr;namespace p=gears_weapon_longshot_bolt_grasp;
namespace g=gears_weapon_longshot_grasp;namespace skin=gears_longshot_round_fixture;
namespace bolt=gears_longshot_bolt_fixture;
using V=vr::Vector;using P=vr::Pose;
V rotated(vr::Quaternion q,V v){return vr::transform(vr::rotation_matrix(q),v);}
P compose(P const& a,P const& b){return {g::add(a.position,rotated(a.orientation,b.position)),vr::normalized(vr::multiply(a.orientation,b.orientation))};}
double length(V v){return std::hypot(std::hypot(v.x,v.y),v.z);}
bool near(V a,V b,double tolerance=1e-8){return length(g::sub(a,b))<tolerance;}
V cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
bool inside(V x,std::array<V,3> const& tri){
    const auto a=g::sub(tri[1],tri[0]),b=g::sub(tri[2],tri[0]),d=g::sub(x,tri[0]);
    const double aa=g::dot(a,a),bb=g::dot(b,b),ab=g::dot(a,b),den=aa*bb-ab*ab;
    assert(den>1e-12);const double u=(g::dot(d,a)*bb-g::dot(d,b)*ab)/den;
    const double v=(g::dot(d,b)*aa-g::dot(d,a)*ab)/den;
    return u>=-1e-6&&v>=-1e-6&&u+v<=1.000001;
}
int main(){
#ifdef _WIN32
    _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
    std::array<P,16> joints{};
    for(unsigned i=1;i<16;++i){assert(skin::parent[i]<i);joints[i]=compose(joints[skin::parent[i]],{skin::translation[i],p::closed[i-1]});}
    std::array<V,2> points{},normals{};
    for(unsigned i=0;i<2;++i){double total{};
        for(auto const& a:skin::witnesses[i]){
            points[i]=g::add(points[i],g::scale(g::add(joints[a.bone].position,rotated(joints[a.bone].orientation,a.point)),a.weight));
            normals[i]=g::add(normals[i],g::scale(rotated(joints[a.bone].orientation,a.normal),a.weight));total+=a.weight;
        }
        assert(std::abs(total-1)<1e-12&&g::normalize(normals[i]));
    }
    assert(near(points[0],{9.856321742348484,-4.5002103593477685,-6.000073107751099}));
    assert(near(points[1],{7.129140464106469,-4.507608337012556,-6.006716423900926}));
    unsigned contacts{};
    for(double units:{50.,100.,200.}){
        const double model_scale=units/100.; // Same physical original model size.
        gears_hand_authored_mount::Mount anatomy{};
        assert(gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
            gears_hand_authored_mount::Side::left,1,units,anatomy));
        for(auto q:{vr::Quaternion{},vr::normalized({.3,.2,-.4,.7}),vr::Quaternion{1,0,0,0},vr::normalized({-.4,.1,.8,.2})}){
            const P mesh{{101,-205,33},q};
            const auto world=[&](V v){return g::add(mesh.position,rotated(q,g::scale(v,model_scale)));};
            const auto knob=world(gears_weapon_longshot_bolt_contact::knob_in_mesh_units);
            const auto forward=rotated(q,bolt::forward_raw),stem=g::sub(knob,world(bolt::pivot_raw));
            P grip{};
            assert(p::grip(knob,forward,stem,anatomy.calibration.grip_to_wrist_metres,
                anatomy.calibration.grip_to_wrist_rotation,units,model_scale,grip));
            const auto wrist=compose(grip,{g::scale(anatomy.calibration.grip_to_wrist_metres,units),anatomy.calibration.grip_to_wrist_rotation});
            assert(near(g::add(wrist.position,rotated(wrist.orientation,g::scale(p::contact_in_wrist_m,units))),knob));
            for(unsigned i=0;i<2;++i){
                std::array<V,3> tri{};for(unsigned j=0;j<3;++j)tri[j]=world(bolt::triangles[i][j]);
                const auto n=rotated(q,bolt::outward_normals[i]);
                const auto point=g::add(wrist.position,rotated(wrist.orientation,g::scale(points[i],units/100.)));
                const double gap=g::dot(g::sub(point,tri[0]),n);
                assert(gap/units>.0008&&gap/units<.0016);
                assert(inside(g::sub(point,g::scale(n,gap)),tri));
                assert(g::dot(rotated(wrist.orientation,normals[i]),g::scale(n,-1.))>.92);
                ++contacts;
            }
            assert(near(mesh.position,{101,-205,33}));
        }
    }
    P output{{9,8,7},{}};const auto before=output;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    for(unsigned reason=0;reason<8;++reason){
        V contact{},forward{0,1,0},stem{1,0,0},offset{};vr::Quaternion calibration{};double units=100,scale=1;
        switch(reason){case 0:forward={};break;case 1:stem=forward;break;case 2:contact.x=nan;break;
            case 3:calibration={0,0,0,0};break;case 4:units=9;break;case 5:units=501;break;
            case 6:scale=2;break;case 7:offset.y=nan;break;}
        assert(!p::grip(contact,forward,stem,offset,calibration,units,scale,output));
        assert(near(output.position,before.position)&&output.orientation.w==before.orientation.w);
    }
    gears_hand_authored_grasp::Request request{};
    request.geometry_signature=gears_hand_authored_mount::marcus115_signature;request.profile_id=p::profile_id;
    request.side=gears_hand_authored_grasp::Side::left;request.mode=gears_hand_authored_grasp::Mode::left_part;
    request.kind=gears_hand_object_grasp::Kind::magazine;request.reference=p::reference;request.closed=p::closed;
    assert(gears_hand_authored_grasp::structural(request));
    assert(!gears_weapon_hand::frame_active(request.source,1));
    std::printf("LS knob profile: %u actual triangle/pad contacts across units/world poses; invalid atomicity and shape-only V4 passed; no arm/skin/game acceptance\n",contacts);
}
