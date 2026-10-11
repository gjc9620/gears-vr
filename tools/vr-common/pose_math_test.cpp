#include "pose_math.h"
#include <cstdio>
#include <limits>
#include <random>
using namespace gears_vr;
constexpr double pi=3.141592653589793;
bool close(double a,double b,double epsilon=1e-8) {return std::abs(a-b)<epsilon;}
bool vector_close(Vector a,Vector b) {return close(a.x,b.x)&&close(a.y,b.y)&&close(a.z,b.z);}
Quaternion axis(int index,double angle) {Quaternion q{};q.w=std::cos(angle/2);if(index==0)q.x=std::sin(angle/2);if(index==1)q.y=std::sin(angle/2);if(index==2)q.z=std::sin(angle/2);return q;}
int main() {
    Viewpoint clean{{10,20,30},{}};Pose neutral{};Pose current{};
    if(!vector_close(xr_to_ue(Vector{1,2,3}),{-3,1,2}))return 1;
    for(int i=0;i<3;++i) {
        current.orientation=axis(i,pi/6);auto result=relative_view(clean,neutral,current);
        if(!close(result.rotation.pitch,i==0?pi/6:0)||!close(result.rotation.yaw,i==1?-pi/6:0)||!close(result.rotation.roll,i==2?-pi/6:0))return 2;
    }
    current={{.1,.2,-.3},{}};auto translated=relative_view(clean,neutral,current);
    if(!vector_close(translated.position,{40,30,50}))return 3;
    clean.rotation.yaw=pi/2;translated=relative_view(clean,neutral,current);
    if(!vector_close(translated.position,{0,50,50}))return 4;
    // A recenter must produce the clean view, including nonzero position and orientation.
    neutral={{1,2,3},multiply(axis(1,.4),axis(0,.2))};current=neutral;
    auto recentered=relative_view(clean,neutral,current);
    if(!vector_close(recentered.position,clean.position)||!close(recentered.rotation.yaw,clean.rotation.yaw))return 5;
    // An eye at +/- 35.5 mm should yield +/- 3.55 units along camera right.
    clean.rotation={};neutral={};current={{-.0355,0,0},{}};
    auto left=relative_view(clean,neutral,current);current.position.x=.0355;auto right=relative_view(clean,neutral,current);
    if(!vector_close(left.position,{10,16.45,30})||!vector_close(right.position,{10,23.55,30}))return 6;
    // Random rotations: matrix->angles->matrix should retain orientation, including poles.
    std::mt19937 random(0x6d0f);std::uniform_real_distribution<double> angle(-pi,pi);
    for(int sample=0;sample<10000;++sample) {
        Rotator r{angle(random),angle(random),angle(random)};
        if(sample==0)r.pitch=pi/2;if(sample==1)r.pitch=-pi/2;
        auto a=ue_matrix(r),b=ue_matrix(ue_angles(a));
        for(int i=0;i<3;++i)for(int j=0;j<3;++j)if(!close(a[i][j],b[i][j],1e-7))return 7;
        // Quaternion sign ambiguity must not change the rotation matrix.
        auto q=normalized(multiply(axis(0,angle(random)),multiply(axis(1,angle(random)),axis(2,angle(random)))));
        auto plus=rotation_matrix(q),minus=rotation_matrix({-q.x,-q.y,-q.z,-q.w});
        for(int i=0;i<3;++i)for(int j=0;j<3;++j)if(!close(plus[i][j],minus[i][j]))return 8;
    }
    try{normalized({0,0,0,0});return 9;}catch(std::invalid_argument const&){}
    try{to_rotator_units(std::numeric_limits<double>::quiet_NaN());return 10;}catch(std::invalid_argument const&){}
    if(to_rotator_units(pi/2)!=16384 || to_rotator_units(-pi/2)!=-16384)return 11;
    puts("Pose math: axis signs, units, recenter, eye separation, 10000 rotations and invalid inputs PASS");
    return 0;
}
