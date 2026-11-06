#include "weapon_lancer_sight_zero.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace gears_vr;
namespace s=gears_weapon_lancer_sight;
int main(){
    for(const Rotator rotation: {Rotator{},Rotator{.3,.8,.6},Rotator{-.4,-1.2,3.141592653589793}})
    for(double scale:{.5,1.,2.})for(double units:{50.,100.,200.}){
        const auto basis=ue_matrix(rotation);const Vector origin{103,-247,381};
        const auto offset=transform(basis,{.00685*scale,72.66864*scale,11.24774*scale});
        const Vector muzzle{origin.x+offset.x,origin.y+offset.y,origin.z+offset.z};
        const auto bore=transform(basis,{0,1,0});s::Result result{};
        assert(s::solve(basis,origin,scale,units,muzzle,bore,result));
        assert(std::abs(s::length(s::minus(result.zero,result.rear))-25*units)<1e-8);
        const double t=s::length(s::minus(result.zero,muzzle));
        const Vector hit{muzzle.x+result.direction.x*t,muzzle.y+result.direction.y*t,muzzle.z+result.direction.z*t};
        assert(s::length(s::minus(hit,result.zero))<1e-8);
        const auto sight=s::minus(result.front,result.rear),target=s::minus(result.zero,result.rear);
        const Vector cross{sight.y*target.z-sight.z*target.y,sight.z*target.x-sight.x*target.z,sight.x*target.y-sight.y*target.x};
        assert(s::length(cross)<1e-7);
    }
    const auto identity=ue_matrix({});const Vector muzzle{.00685,72.66864,11.24774};s::Result r{};
    assert(s::solve(identity,{},1,100,muzzle,{0,1,0},r));
    const double old_error=std::hypot(r.zero.x-muzzle.x,r.zero.z-muzzle.z)/100.;
    assert(old_error>.66&&old_error<.67&&r.correction_radians>.027&&r.correction_radians<.028);
    const auto unchanged=r;auto bad=identity;bad[0][0]=-1;
    assert(!s::solve(bad,{},1,100,muzzle,{0,1,0},r));
    assert(r.zero.x==unchanged.zero.x&&r.zero.y==unchanged.zero.y&&r.zero.z==unchanged.zero.z);
    assert(!s::solve(identity,{},1,100,{0,40,11},{0,1,0},r));
    assert(!s::solve(identity,{},1,100,muzzle,{1,0,0},r));
    assert(!s::solve(identity,{},1,100,muzzle,{0,1,0},r,.1));
    assert(!s::solve(identity,{},std::numeric_limits<double>::quiet_NaN(),100,muzzle,{0,1,0},r));
    std::puts("Lancer sight zero: original-ring convergence, translated/rotated/inverted poses, scale/units and invalid-input guards passed");
}
