#include "weapon_longshot_scope_projection.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
using namespace gears_vr;
StereoMatrices initial(){
    StereoMatrices m{};
    for(unsigned i=0;i<2;++i){
        m.view[i]={1,0,0,0,0,1,0,0,0,0,1,0,i?.032f:-.032f,0,0,1};
        m.projection[i]={1.2f,0,0,0,0,1.5f,0,0,i?.02f:-.02f,-.01f,.0001f,1,0,0,5,0};
    }return m;
}
int main(){
    namespace scope=gears_weapon::longshot_scope_projection;
    const auto normal=initial();StereoMatrices result{};
    const Vector bore{std::sin(.1),0,std::cos(.1)};
    assert(scope::select(normal,bore,20,result));
    for(unsigned i=0;i<2;++i){
        assert(result.view[i]==normal.view[i]);
        for(unsigned n=0;n<16;++n)if(n!=0&&n!=5&&n!=8&&n!=9)assert(result.projection[i][n]==normal.projection[i][n]);
        const auto& p=result.projection[i];
        assert(std::abs(bore.x/bore.z*p[0]+p[8])<1e-6);
        assert(std::abs(p[5]/p[0]-normal.projection[i][5]/normal.projection[i][0])<1e-6);
        assert(p[0]>normal.projection[i][0]);
    }
    auto low=result,high=result;
    assert(scope::select(normal,bore,10,high)&&high.projection[0][0]>low.projection[0][0]);
    const auto unchanged=result;
    assert(!scope::select(normal,{0,0,-1},20,result));
    assert(!scope::select(normal,{0,0,2},20,result));
    assert(!scope::select(normal,{0,0,1},std::numeric_limits<double>::quiet_NaN(),result));
    auto broken=normal;broken.projection[1][11]=0;
    assert(!scope::select(broken,bore,20,result));
    assert(!std::memcmp(&result,&unchanged,sizeof result)); // Both-eye transaction.
    auto rotated=normal;
    for(auto& view:rotated.view){view[0]=0;view[2]=1;view[8]=-1;view[10]=0;}
    assert(scope::select(rotated,{1,0,0},20,result));
    assert(std::abs(result.projection[0][8])<1e-6);
    std::puts("Longshot fallback projection: actual bore centring, original FOV levels, stereo view/depth preservation and atomic rejection passed; no live rendering");
}
