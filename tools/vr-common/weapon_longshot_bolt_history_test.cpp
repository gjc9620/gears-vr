#include "weapon_longshot_bolt_stroke.h"
#include <cassert>
#include <cstdio>
#include <memory>
using namespace gears_weapon;using namespace gears_weapon::longshot_tracks;
struct HistoryFixture {
    Samples curve{};BoltPath path;BoltStroke stroke;BoltSample sample{};
    gears_controller_relative_motion::Segment motion{};
    explicit HistoryFixture(double travel=.1){
        curve.count=3;curve.length=1;
        for(auto& bone:curve.bones)for(auto& p:bone)p.orientation.w=1;
        curve.bones[3][1].position={travel*100,0,0};assert(path.bind(curve,{},100));
        sample.stamp={{1,2,3,4,5,6,7,8,9},10,1000000000,1000,true,true,true};
        sample.frame=1;sample.lease=11;sample.path_generation=12;
        sample.grip_down=sample.lease_current=sample.new_grant=sample.bone_measured=true;
        assert(stroke.begin(path,sample,BoltDirection::opening,{},sample.stamp.observed_ns));
        motion.count=6;
        for(unsigned i=0;i<6;++i)motion.points[i]={10+i,1000000000+std::int64_t(i)*22000000,{.02*i,0,0}};
        sample.frame=2;sample.stamp.sequence=15;sample.stamp.observed_ns=1110000000;
        sample.hand_in_root_m={.1,0,0};sample.new_grant=false;
    }
    BoltResult step(){return stroke.observe_continuous(path,sample,motion,sample.stamp.observed_ns+65000000);}
};
int main(){
    unsigned cases{};
    {auto f=std::make_unique<HistoryFixture>();auto old=f->stroke;
        assert(old.observe(f->path,f->sample,f->sample.stamp.observed_ns+65000000)==BoltResult::interrupted);
        assert(f->step()==BoltResult::moved&&!f->stroke.completed()&&f->stroke.wanted_progress()>.99);
        f->sample.stamp.observed_ns+=20000000;++f->sample.stamp.sequence;f->sample.actual_progress=1;
        assert(f->stroke.observe(f->path,f->sample,f->sample.stamp.observed_ns)==BoltResult::waiting);
        ++f->sample.frame;
        assert(f->stroke.observe(f->path,f->sample,f->sample.stamp.observed_ns)==BoltResult::complete);++cases;}
    for(unsigned failure=0;failure<15;++failure){auto f=std::make_unique<HistoryFixture>();
        switch(failure){
        case 0:f->sample.lease_current=false;break;
        case 1:++f->sample.lease;break;
        case 2:++f->sample.path_generation;break;
        case 3:++f->sample.stamp.identity.space_epoch;break;
        case 4:f->sample.stamp.left_tracked=false;break;
        case 5:f->sample.stamp.right_tracked=false;break;
        case 6:f->sample.grip_down=false;break;
        case 7:f->sample.frame=1;break;
        case 8:f->motion.count=1;break;
        case 9:++f->motion.points[2].sequence;break;
        case 10:f->motion.points[2].sync_ns=f->motion.points[1].sync_ns;break;
        case 11:f->motion.points[2].position.y=.03;break;
        case 12:f->motion.points[0].position.x=.001;break;
        case 13:f->motion.points[5].position.x=.101;break;
        case 14:f->motion.points[1].position.x=.08;break;
        }
        assert(f->step()==BoltResult::interrupted&&!f->stroke.active());++cases;
    }
    {auto f=std::make_unique<HistoryFixture>();
        assert(f->stroke.observe_continuous(f->path,f->sample,f->motion,f->sample.stamp.observed_ns+100000001)==BoltResult::interrupted);++cases;}
    {auto f=std::make_unique<HistoryFixture>(.3);
        for(unsigned i=0;i<f->motion.count;++i)f->motion.points[i].position.x=.035*i;
        f->sample.hand_in_root_m={.175,0,0};
        // Each22ms point fits1.5m/s+2mm, but the complete110ms displacement
        // exceeds the unchanged net1.5m/s+2mm allowance.
        assert(f->step()==BoltResult::interrupted);++cases;}
    {auto f=std::make_unique<HistoryFixture>();
        for(auto& p:f->motion.points)p.position={};f->sample.hand_in_root_m={};f->sample.actual_progress=1;
        assert(f->step()==BoltResult::waiting&&!f->stroke.completed()&&f->stroke.wanted_progress()==0);++cases;}
    std::printf("LongshotBoltHistoryTest passed:%u cases; old110ms rejection reproduced, exact intermediate motion accepted, current deadline/lease/path/speed and later real-Frame bone acknowledgement retained\n",cases);
}
