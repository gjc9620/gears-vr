// Pure coordinate tests: no Game/XR, bridge publication, hooks or engine calls.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../vr-common/rig_tracking_math.h"
#include "../vr-common/cover_view.h"
#include <cstdio>
#include <cstring>
#include <limits>
using namespace gears_vr;
namespace rig=gears_vr::rig_tracking;
unsigned checks{},failures{};
#define CHECK(x) do{++checks;if(!(x)){++failures;std::printf("FAIL line %d: %s\n",__LINE__,#x);}}while(0)
constexpr double pi=3.14159265358979323846;
bool near(Vector a,Vector b,double tolerance=.002){return stereo_distance(a,b)<tolerance;}
bool same_rotation(Quaternion a,Quaternion b){
    const auto x=rotation_matrix(a),y=rotation_matrix(b);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)if(std::abs(x[i][j]-y[i][j])>2e-5)return false;
    return true;
}
Quaternion axis(unsigned index,double angle){Quaternion q{};q.w=std::cos(angle/2);
    if(index==0)q.x=std::sin(angle/2);else if(index==1)q.y=std::sin(angle/2);else q.z=std::sin(angle/2);return q;}
StereoMatrix view(Rotator rotation,Vector origin){
    const auto r=ue_matrix(rotation);StereoMatrix v{};v[15]=1;
    for(unsigned i=0;i<3;++i){v[i*4]=static_cast<float>(r[i][1]);v[i*4+1]=static_cast<float>(r[i][2]);v[i*4+2]=static_cast<float>(r[i][0]);}
    CHECK(view_with_origin(v,origin,v));return v;
}
StereoSample sample(Pose head){
    StereoSample value{};value.source=StereoSource::MetaOperatorSimulated;value.head=head;
    for(unsigned i=0;i<2;++i){auto d=transform(rotation_matrix(head.orientation),{i?.032:-.032,0,0});
        value.eyes[i].pose={{head.position.x+d.x,head.position.y+d.y,head.position.z+d.z},head.orientation};value.eyes[i].fov={-.8,.8,.8,-.8};}
    return value;
}
Vector rendered_midpoint(StereoMatrix const& base,Pose reference,Pose head,double units){
    const StereoMatrix projection{1,0,0,0,0,1,0,0,0,0,0,1,0,0,10,0};StereoMatrices result{};
    CHECK(stereo_matrices(base,projection,reference,sample(head),units,result,true));
    const auto left=stereo_camera_position(result.view[0]),right=stereo_camera_position(result.view[1]);
    return {(left.x+right.x)/2,(left.y+right.y)/2,(left.z+right.z)/2};
}
Pose rebase(Pose p,Pose space){auto d=transform(rotation_matrix(space.orientation),p.position);
    return {{d.x+space.position.x,d.y+space.position.y,d.z+space.position.z},normalized(multiply(space.orientation,p.orientation))};}
void failure_point(StereoMatrix const& base,Pose reference,Vector raw,double units){Vector out{7,8,9},before=out;
    CHECK(!rig::point(base,reference,raw,units,out));CHECK(std::memcmp(&out,&before,sizeof out)==0);}
void failure_pose(StereoMatrix const& base,Pose reference,Pose raw,double units){Pose out{{7,8,9},{.1,.2,.3,.4}},before=out;
    CHECK(!rig::pose(base,reference,raw,units,out));CHECK(std::memcmp(&out,&before,sizeof out)==0);}
int main(){
    const auto base=view({}, {100,-200,30});const Pose reference{};Pose out{};
    CHECK(rig::pose(base,reference,reference,100,out));CHECK(near(out.position,{100,-200,30}));
    CHECK(out.orientation.x==0&&out.orientation.y==0&&out.orientation.z==0&&out.orientation.w==1);
    const Pose left{{-.3,-.2,-.5},{}},right{{.4,-.1,-.6},{}};Pose world_left{},world_right{};
    CHECK(rig::pose(base,reference,left,100,world_left));CHECK(near(world_left.position,{150,-230,10}));
    CHECK(rig::pose(base,reference,right,100,world_right));CHECK(near(world_right.position,{160,-160,20}));
    for(double units:{10.,100.,500.}){Vector world{};CHECK(rig::point(base,reference,{.2,.3,-.4},units,world));
        CHECK(near(world,{100+.4*units,-200+.2*units,30+.3*units}));}
    // Full rendered-base yaw/pitch/roll, including vertical forward. These
    // independent stereo eye origins catch HMD addition twice and yaw flattening.
    for(double yaw:{-pi,-pi/2,0.,.8,pi})for(double pitch:{-pi/2,-.6,0.,.7,pi/2}){
        const Rotator angle{pitch,yaw,.23};const auto b=view(angle,{101,-203,37});
        const Pose ref{{.5,1.4,-.2},normalized(multiply(axis(1,.7),axis(0,.2)))};
        const auto d=transform(rotation_matrix(ref.orientation),{.2,.3,-.4});
        const Pose head{{ref.position.x+d.x,ref.position.y+d.y,ref.position.z+d.z},normalized(multiply(ref.orientation,axis(2,.3)))};
        Vector world{};CHECK(rig::point(b,ref,head.position,100,world));CHECK(near(world,rendered_midpoint(b,ref,head,100)));
        CHECK(rig::pose(b,ref,ref,100,out));const auto expected=ue_matrix(angle),actual=rotation_matrix(out.orientation);
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)CHECK(std::abs(actual[i][j]-expected[i][j])<2e-5);
    }
    // The existing level-cover mapping, including nonzero committed offsets.
    for(double yaw:{-2.,0.,1.3})for(CoverOffset offset:{CoverOffset{-.2,.1},CoverOffset{.35,-.1}}){
        CoverViewAnchor anchor{{200,-130,40},{std::cos(yaw),std::sin(yaw),0},{-std::sin(yaw),std::cos(yaw),0}};
        CoverViewResult committed{};CHECK(prepare_cover_view(anchor,offset,100,committed));
        const Pose ref{{.2,1.5,-.4},axis(1,.4)};
        for(Vector raw:{Vector{-.1,1.1,-.7},Vector{.6,1.2,-.8},Vector{.4,1.8,-.2}}){Vector a{},b{};
            CHECK(cover_tracking_point(anchor,committed,ref,raw,100,a));CHECK(rig::point(committed.base_view,ref,raw,100,b));CHECK(near(a,b));}
    }
    CoverViewAnchor anchor{{100,-200,30},{1,0,0},{0,1,0}};CoverViewResult held{};
    CHECK(prepare_cover_view(anchor,{-.2,0},100,held));Vector wall{},neutral{};
    CHECK(rig::point(base,reference,{0,0,-.5},100,neutral));CHECK(rig::point(held.base_view,reference,{.2,0,-.5},100,wall));CHECK(near(wall,neutral));
    // Controller rotation changes orientation, with no position or extra HMD term.
    for(unsigned a=0;a<3;++a){Pose raw=right;raw.orientation=axis(a,pi/2);CHECK(rig::pose(base,reference,raw,100,out));
        CHECK(near(out.position,world_right.position));CHECK(!same_rotation(out.orientation,world_right.orientation));}
    Pose raw=right;raw.orientation=axis(1,pi/2);CHECK(rig::pose(base,reference,raw,100,out));CHECK(near(transform(rotation_matrix(out.orientation),{1,0,0}),{0,-1,0}));
    // A rigid tracking-space rebase/recenter transforms reference and raw pose
    // together, leaving the same world's point AND proper orientation invariant.
    const Pose ref{{.3,1.4,-.2},axis(1,.8)},tracked{{-.4,1.1,-.7},normalized(multiply(axis(0,.5),axis(2,-.9)))};
    Pose before{};CHECK(rig::pose(base,ref,tracked,100,before));
    for(Pose space:{Pose{{3,-2,4},axis(0,pi/2)},Pose{{-7,5,.3},normalized(multiply(axis(1,1.7),axis(2,.4)))}}){
        CHECK(rig::pose(base,rebase(ref,space),rebase(tracked,space),100,out));CHECK(near(before.position,out.position));CHECK(same_rotation(before.orientation,out.orientation));}
    // Quaternion conversion branches at 180 degrees about all three axes.
    for(unsigned a=0;a<3;++a){Pose p{{},axis(a,pi)};CHECK(rig::pose(base,reference,p,100,out));
        CHECK(stereo_pose_valid(out));CHECK(near(transform(rotation_matrix(out.orientation),{1,0,0}),rig::map_vector(base,transform(rotation_matrix(p.orientation),{0,0,-1}))));}
    const double nan=std::numeric_limits<double>::quiet_NaN(),inf=std::numeric_limits<double>::infinity();
    for(double units:{0.,-1.,9.,501.,nan,inf}){failure_point(base,reference,{},units);failure_pose(base,reference,right,units);}
    for(unsigned index:{0u,12u,15u}){auto bad=base;bad[index]=std::numeric_limits<float>::quiet_NaN();failure_point(bad,reference,{},100);failure_pose(bad,reference,right,100);}
    for(unsigned index:{3u,7u,11u,15u}){auto bad=base;bad[index]=.2f;failure_point(bad,reference,{},100);failure_pose(bad,reference,right,100);}
    for(float factor:{-1.f,1.2f}){auto bad=base;for(unsigned i=0;i<3;++i)bad[i*4]*=factor;failure_point(bad,reference,{},100);failure_pose(bad,reference,right,100);}
    for(Quaternion bad:{Quaternion{0,0,0,0},Quaternion{0,0,0,2},Quaternion{nan,0,0,1}}){
        failure_point(base,Pose{{},bad},{},100);failure_pose(base,Pose{{},bad},right,100);failure_pose(base,reference,Pose{{},bad},100);}
    for(Vector bad:{Vector{nan,0,0},Vector{0,inf,0}}){failure_point(base,reference,bad,100);failure_point(base,Pose{bad,{}},{},100);failure_pose(base,reference,Pose{bad,{}},100);}
    const double huge=(std::numeric_limits<double>::max)();failure_point(base,Pose{{-huge,0,0},{}},{huge,0,0},100);
    std::printf("RigTrackingMath: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
