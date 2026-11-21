#include "../vr-common/pose_editor.h"
#include "../vr-common/weapon_mesh_follow.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <limits>
namespace a=gears_pose_editor;namespace v=gears_vr;
double dist(v::Vector x,v::Vector y){return std::hypot(std::hypot(x.x-y.x,x.y-y.y),x.z-y.z);}
int main(int argc,char** argv){
    a::Config c{};c.revision=123;c.enabled=1;assert(a::valid(c));
    auto w=a::encode(c);a::Config out{};assert(a::decode(w,out));assert(out.revision==123);
    w[500]^=1;assert(!a::decode(w,out));assert(out.revision==123);
    c.palm.position={.04,.04,0};assert(!a::valid(c));
    c.palm.position={std::numeric_limits<double>::quiet_NaN(),0,0};assert(!a::valid(c));
    c.palm={};c.fingers[1]={0,0,0,0};assert(!a::valid(c));
    c.fingers[1]={};c.geometry[0]^=1;assert(!a::valid(c));c.geometry[0]^=1;
    // Independent contact oracle: new_hand * delta * adjusted_mount equals
    // old_hand * baseline_mount. Test translations, rotation and both together.
    gears_weapon::WeaponMeshFollow base;
    std::array<float,16> matrix{{1,0,0,0,0,1,0,0,0,0,1,0,8,-3,2,1}};
    assert(base.bind({{},{}},matrix));
    for(double offset:{0.,.012})for(double angle:{0.,.3}){
        v::Pose delta{{offset,-offset*.3,offset*.2},{0,0,std::sin(angle/2),std::cos(angle/2)}};
        gears_weapon::WeaponMeshFollow adjusted;assert(base.adjusted_contact(delta,100,adjusted));
        v::Viewpoint pose{{15,20,3},{.2,.5,-.3}},original{},actual{};assert(base.solve(pose,original));
        const auto r=v::ue_matrix(pose.rotation);const auto shift=v::transform(r,delta.position);
        const auto composite=v::multiply(r,v::rotation_matrix(delta.orientation));
        v::Viewpoint moved{{pose.position.x+shift.x*100,pose.position.y+shift.y*100,pose.position.z+shift.z*100},v::ue_angles(composite)};
        assert(adjusted.solve(moved,actual));assert(dist(actual.position,original.position)<1e-8);
        auto r0=v::ue_matrix(original.rotation),r1=v::ue_matrix(actual.rotation);
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)assert(std::abs(r0[i][j]-r1[i][j])<1e-9);
        gears_weapon::WeaponMeshFollow again;assert(base.adjusted_contact(delta,100,again));
        v::Viewpoint repeated{};assert(again.solve(moved,repeated));assert(dist(repeated.position,actual.position)<1e-12);
    }
    c.palm={};c.fingers[4]={0,0,std::sin(.2),std::cos(.2)};
    auto q=a::finger(c,4,{});assert(std::abs(q.z-std::sin(.2))<1e-12);
    c.enabled=0;q=a::finger(c,4,{});assert(q.z==0&&q.w==1);
    if(argc==2){
        a::Wire python{};std::ifstream f(argv[1],std::ios::binary);assert(f.read(reinterpret_cast<char*>(python.data()),python.size()));assert(f.peek()==EOF);
        assert(a::decode(python,out));assert(out.revision==123&&out.enabled==1);
        assert(out.palm.position.x==.012&&out.palm.position.y==-.003);
        assert(std::abs(out.fingers[4].z-std::sin(25.*3.14159265358979323846/360.))<1e-12);
    }
    std::puts("Pose editor protocol, Python interoperability, inverse contact transform, no accumulation and restore passed (CPU only)");
}
