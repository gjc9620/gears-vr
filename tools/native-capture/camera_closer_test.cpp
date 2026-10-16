#include "camera_closer.h"
#include <cassert>
#include <limits>
int main(){
    using namespace gears_camera_closer;
    gears_vr::StereoMatrix v{1,0,0,0,0,1,0,0,0,0,1,0,0,0,-100,1};Result r{};
    assert(prepare(v,0,100,true,{0,0,260},r)&&r.view==v&&r.applied_units==0);
    assert(prepare(v,.5,100,false,{0,0,260},r)&&r.view==v);
    assert(prepare(v,.3,100,true,{0,0,260},r)&&r.applied_units==30&&r.view[14]==-130);
    assert(prepare(v,.5,100,true,{0,0,160},r)&&r.applied_units==20); // Preserve 0.4m clearance.
    assert(prepare(v,.5,100,true,{0,0,120},r)&&r.view==v); // Already close/occluded.
    assert(prepare(v,.5,100,true,{0,0,80},r)&&r.view==v); // Pawn behind camera.
    assert(prepare(v,.3,200,true,{0,0,300},r)&&r.applied_units==60);
    for(double bad:{-.1,.501,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()})
        assert(!prepare(v,bad,100,true,{0,0,260},r));
    auto yaw=v;yaw[0]=0;yaw[2]=-1;yaw[8]=1;yaw[10]=0;
    assert(prepare(yaw,.3,100,true,{-260,0,0},r));
    const auto before=gears_vr::stereo_camera_position(yaw),after=gears_vr::stereo_camera_position(r.view);
    assert(std::abs(after.x-before.x+30)<1e-6&&after.y==before.y&&after.z==before.z);
    for(unsigned i=0;i<16;++i)if(i!=14)assert(r.view[i]==yaw[i]);
    auto invalid=v;invalid[0]=2;assert(!prepare(invalid,.3,100,true,{0,0,260},r));
    gears_vr::StereoMatrix projection{1.2f,0,0,0,0,1.5f,0,0,.02f,-.01f,.0001f,1,0,0,5,0};
    for(double angle:{0.,.7,-1.2}){
        gears_vr::StereoSample sample{};sample.head.orientation={0,std::sin(angle/2),0,std::cos(angle/2)};
        sample.head.position={.1,.2,.3};
        for(unsigned eye=0;eye<2;++eye){auto local=gears_vr::transform(gears_vr::rotation_matrix(sample.head.orientation),{eye?.032:-.032,0,0});
            sample.eyes[eye].pose={ {sample.head.position.x+local.x,sample.head.position.y+local.y,sample.head.position.z+local.z},sample.head.orientation};
            sample.eyes[eye].fov={-.8,.7,.75,-.65};}
        assert(prepare(yaw,.3,100,true,{-260,0,0},r));
        gears_vr::StereoMatrices baseline{},closer{};gears_vr::Pose reference{};
        assert(gears_vr::stereo_matrices(yaw,projection,reference,sample,100,baseline));
        assert(gears_vr::stereo_matrices(r.view,projection,reference,sample,100,closer));
        for(unsigned eye=0;eye<2;++eye){
            auto a=gears_vr::stereo_camera_position(baseline.view[eye]),b=gears_vr::stereo_camera_position(closer.view[eye]);
            assert(std::abs(b.x-a.x+30)<.001&&std::abs(b.y-a.y)<.001&&std::abs(b.z-a.z)<.001);
            assert(baseline.projection[eye]==closer.projection[eye]);
            for(unsigned i=0;i<12;++i)assert(baseline.view[eye][i]==closer.view[eye][i]);
        }
    }
}
