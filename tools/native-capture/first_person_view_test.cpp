#include "first_person_view.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace gears_first_person;
    Toggle toggle;
    assert(!toggle.update(true,true,false,100)); // No neutral admission yet.
    assert(!toggle.update(true,false,true,101));
    assert(!toggle.update(true,true,false,110));
    assert(!toggle.update(true,true,false,189));
    assert(toggle.update(true,true,false,190)&&toggle.requested());
    for(unsigned i=0;i<300;++i)assert(!toggle.update(true,true,false,200+i));
    assert(!toggle.update(false,false,true,600));
    assert(!toggle.update(true,true,false,700));
    assert(!toggle.update(true,false,true,701));
    assert(!toggle.update(true,true,false,710));
    assert(toggle.update(true,true,false,790)&&!toggle.requested());
    assert(toggle.update(true,false,true,800,true)&&toggle.requested());
    assert(!toggle.update(false,false,true,801,true)&&toggle.requested());
    // User sequence: rifle preview -> unzoomed sniper -> native scope ->
    // unzoomed sniper. Weapon switches/zoom never erase the requested mode.
    assert(camera_mode(toggle.requested(),true,true,false,false)==CameraMode::preview);
    assert(camera_mode(toggle.requested(),true,true,true,false)==CameraMode::preview);
    assert(camera_mode(toggle.requested(),true,true,true,true)==CameraMode::native_scope);
    assert(camera_mode(toggle.requested(),true,true,true,false)==CameraMode::preview&&toggle.requested());
    assert(camera_mode(toggle.requested(),true,false,true,false)==CameraMode::unchanged&&toggle.requested());
    assert(camera_mode(toggle.requested(),false,true,true,false)==CameraMode::unchanged&&toggle.requested());
    assert(toggle.update(true,false,true,900,true)&&!toggle.requested());
    assert(camera_mode(toggle.requested(),true,true,true,false)==CameraMode::unchanged);
    // E+R while holding a sniper takes the same neutral/80ms edge as a rifle.
    assert(!toggle.update(true,false,true,1000));assert(!toggle.update(true,true,false,1010));
    assert(toggle.update(true,true,false,1090)&&toggle.requested());
    assert(camera_mode(toggle.requested(),true,true,true,false)==CameraMode::preview);
    using namespace gears_vr;
    Vector eye{1000,-2000,60};Result result{};
    for(double yaw:{-3.,-.7,0.,1.,3.})for(double pitch:{-.8,0.,.8})for(double roll:{-.4,0.,.4}){
        const auto r=ue_matrix({pitch,yaw,roll});
        StereoMatrix view{float(r[0][1]),float(r[0][2]),float(r[0][0]),0,
                          float(r[1][1]),float(r[1][2]),float(r[1][0]),0,
                          float(r[2][1]),float(r[2][2]),float(r[2][0]),0,0,0,0,1};
        assert(view_with_origin(view,{eye.x-160,eye.y+48,eye.z+16},view));
        assert(prepare(view,eye,100,result)&&stereo_rigid(result.view));
        assert(std::abs(result.origin.z-70)<1e-8); // Pitch never produces peek.
        assert(std::abs(result.origin.x-eye.x-35*std::cos(yaw))<1e-4);
        assert(std::abs(result.origin.y-eye.y-35*std::sin(yaw))<1e-4);
        auto recovered=stereo_camera_position(result.view);
        assert(std::hypot(std::hypot(recovered.x-result.origin.x,recovered.y-result.origin.y),recovered.z-result.origin.z)<.02);
        assert(std::abs(result.view[8])<1e-7); // No roll in the base camera.
        for(unsigned i:{2u,6u,10u})assert(result.view[i]==view[i]); // Keep native aim direction.
    }
    auto invalid=result.view;invalid[0]=10;assert(!prepare(invalid,eye,100,result));
    std::puts("first-person: chord debounce/rearm/context, 45 pitch/yaw/roll views, no pitch-height coupling, origin bounds passed");
}
