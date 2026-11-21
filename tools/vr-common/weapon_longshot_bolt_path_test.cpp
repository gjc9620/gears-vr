#include "weapon_longshot_bolt_path.h"
#include <cassert>
#include <cstdio>
#include <limits>
namespace tracks=gears_weapon::longshot_tracks;
tracks::Samples fixture(){
    tracks::Samples samples{};samples.count=3;samples.length=1;
    samples.bones[3][1].position.y=5;return samples;
}
int main(){
    auto samples=fixture();tracks::BoltPath path;
    assert(path.bind(samples,{},100));float time{};double progress{};
    assert(path.time_for_progress(0,time)&&time==0);
    assert(path.time_for_progress(1,time)&&std::abs(time-1.f/3)<1e-7);
    assert(path.time_for_progress(.5,time)&&std::abs(time-1.f/6)<1e-7);
    assert(path.fit({0,.025,0},0,.6,.005,progress)&&std::abs(progress-.5)<1e-10);
    assert(!path.fit({0,.05,0},0,.25,.005,progress));
    assert(!path.fit({.05,.025,0},.5,.25,.005,progress));
    assert(path.fit({0,.05,0},.8,.25,.005,progress)&&std::abs(progress-1)<1e-10);
    assert(path.fit({0,0,0},.2,.25,.005,progress)&&progress==0);
    assert(!path.time_for_progress(1.01,time));
    auto bad=samples;bad.bones[0][1].position.x=1;
    assert(!path.bind(bad,{},100)&&!path.bound());
    bad=samples;bad.bones[3][2].position.y=1;
    assert(!path.bind(bad,{},100));
    bad=samples;bad.bones[3][1].orientation.x=std::numeric_limits<double>::quiet_NaN();
    assert(!path.bind(bad,{},100));
    bad=samples;bad.bones[3][2].orientation={0,0,1,0};
    assert(!path.bind(bad,{},100));
    assert(!path.bind(samples,{},1));
    // A rotating handle produces an arc even with no bolt translation.
    samples=fixture();samples.bones[3][1].position={};
    samples.bones[3][1].orientation={0,0,std::sqrt(.5),std::sqrt(.5)};
    assert(path.bind(samples,{5,0,0},100));
    const double diagonal=.05*std::sqrt(.5);
    assert(path.fit({diagonal,diagonal,0},.5,.1,.005,progress)&&std::abs(progress-.5)<.01);
    assert(path.fit({0,.05,0},.8,.25,.005,progress)&&std::abs(progress-1)<1e-10);
    // The same source can combine rotation with rearward travel.
    samples.bones[3][1].position.y=3;
    assert(path.bind(samples,{5,0,0},100));
    assert(path.fit({0,.08,0},.8,.25,.005,progress)&&std::abs(progress-1)<1e-10);
    std::puts("Longshot bolt path: translation, rotation, compound travel, close endpoints and discontinuity checks passed");
}
