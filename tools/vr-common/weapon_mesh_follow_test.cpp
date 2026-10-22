#include "weapon_mesh_follow.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace gears_vr;
using namespace gears_weapon;

static bool near(double a,double b){return std::abs(a-b)<1e-4;}
static std::array<float,16> mesh_matrix(Rotator angle,Vector origin,double scale=1){
    const auto b=ue_matrix(angle);
    return {float(b[0][0]*scale),float(b[1][0]*scale),float(b[2][0]*scale),0,
            float(b[0][1]*scale),float(b[1][1]*scale),float(b[2][1]*scale),0,
            float(b[0][2]*scale),float(b[1][2]*scale),float(b[2][2]*scale),0,
            float(origin.x),float(origin.y),float(origin.z),1};
}
int main(){
    WeaponMeshFollow follow;
    const Viewpoint first{{100,200,300},{.12,.3,-.1}};
    const Vector mesh_origin{140,225,287};
    const Rotator mesh_rotation{-.09,.48,.16};
    auto matrix=mesh_matrix(mesh_rotation,mesh_origin,1.25);
    assert(follow.bind(first,matrix));
    Viewpoint actual{};
    assert(follow.solve(first,actual));
    assert(near(actual.position.x,mesh_origin.x)&&near(actual.position.y,mesh_origin.y)&&
           near(actual.position.z,mesh_origin.z));
    const auto expected=ue_matrix(mesh_rotation),initial=ue_matrix(actual.rotation);
    for(int row=0;row<3;++row)for(int col=0;col<3;++col)
        assert(near(initial[row][col],expected[row][col]));
    const Viewpoint moved{{110,210,320},{.12,.3+1.5707963267948966,-.1}};
    assert(follow.solve(moved,actual));
    assert(std::hypot(std::hypot(actual.position.x-mesh_origin.x,
        actual.position.y-mesh_origin.y),actual.position.z-mesh_origin.z)>10);
    const auto saved=actual;
    Viewpoint impossible=moved;impossible.position.x=INFINITY;
    assert(!follow.solve(impossible,actual)&&near(actual.position.x,saved.position.x));
    follow.reset();assert(!follow.solve(first,actual));
    matrix=mesh_matrix(mesh_rotation,mesh_origin);
    matrix[0]*=-1;matrix[1]*=-1;matrix[2]*=-1; // Mirror/reflection is not a safe rigid mount.
    assert(!follow.bind(first,matrix));
    matrix=mesh_matrix(mesh_rotation,mesh_origin);matrix[4]*=2;
    assert(!follow.bind(first,matrix));
    matrix=mesh_matrix(mesh_rotation,{1000,2000,3000});
    assert(!follow.bind(first,matrix));
    std::puts("weapon mesh follow calibration and invalid geometry gates passed");
}
