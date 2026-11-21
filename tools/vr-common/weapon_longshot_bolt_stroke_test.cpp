#include "weapon_longshot_bolt_stroke.h"
#include <cassert>
#include <cstdio>
#include <memory>
using namespace gears_weapon;
using namespace gears_weapon::longshot_tracks;
struct Fixture {
    Samples curve{};BoltPath path;BoltStroke stroke;BoltSample s{};
    Fixture(){
        curve.count=3;curve.length=1;
        for(auto& bone:curve.bones)for(auto& p:bone)p.orientation.w=1;
        curve.bones[3][1].position={10,0,0};assert(path.bind(curve,{},100));
        s.stamp={{1,2,3,4,5,6,7,8,9},1,1000000000,1000,true,true,true};
        s.frame=1;s.lease=11;s.path_generation=12;s.grip_down=s.lease_current=s.new_grant=s.bone_measured=true;
    }
    void next(double hand,double actual){++s.stamp.sequence;++s.frame;s.stamp.observed_ns+=20000000;
        s.hand_in_root_m={hand,0,0};s.actual_progress=actual;s.new_grant=false;}
    bool begin(BoltDirection d=BoltDirection::opening){return stroke.begin(path,s,d,{s.actual_progress*.1,0,0},s.stamp.observed_ns);}
    BoltResult step(){return stroke.observe(path,s,s.stamp.observed_ns);}
};
int main(){
    {auto owner=std::make_unique<Fixture>();auto& f=*owner;assert(f.stroke.last_observed_ns()==0);assert(f.begin());
        const auto before=f.s.stamp.observed_ns;assert(f.stroke.last_observed_ns()==before);
        f.next(0,0);assert(f.step()==BoltResult::waiting&&f.stroke.last_observed_ns()==f.s.stamp.observed_ns);
        const auto accepted=f.stroke.last_observed_ns();f.next(0,0);f.s.stamp.observed_ns+=100000001;
        assert(f.step()==BoltResult::interrupted&&f.stroke.last_observed_ns()==accepted);}
    {auto owner=std::make_unique<Fixture>();auto& f=*owner;assert(f.begin());for(unsigned i=1;i<=5;++i){f.next(.02*i,.2*(i-1));assert(f.step()==BoltResult::moved);}
        assert(!f.stroke.completed());f.next(.1,1);assert(f.step()==BoltResult::complete&&f.stroke.completed());}
    {auto owner=std::make_unique<Fixture>();auto& f=*owner;f.s.actual_progress=1;f.s.hand_in_root_m={.1,0,0};assert(f.begin(BoltDirection::closing));
        for(unsigned i=1;i<=5;++i){f.next(.1-.02*i,1-.2*(i-1));assert(f.step()==BoltResult::moved);}
        f.next(0,0);assert(f.step()==BoltResult::complete);}
    // No physical movement, even a native animation at its endpoint cannot complete.
    {auto owner=std::make_unique<Fixture>();auto& f=*owner;assert(f.begin());f.next(0,1);assert(f.step()==BoltResult::waiting&&!f.stroke.completed());}
    // A wrist acquired off-centre must still travel the complete bolt path.
    {auto owner=std::make_unique<Fixture>();auto& f=*owner;f.s.hand_in_root_m={0,.04,0};assert(f.begin());f.next(.02,0);f.s.hand_in_root_m.y=.04;
        assert(f.step()==BoltResult::moved&&std::abs(f.stroke.wanted_progress()-.2)<1e-6);}
    for(unsigned error=0;error<8;++error){auto owner=std::make_unique<Fixture>();auto& f=*owner;
        switch(error){case 0:f.s.new_grant=false;break;case 1:f.s.lease_current=false;break;case 2:f.s.grip_down=false;break;
        case 3:f.s.position_error_m=.003;break;case 4:f.s.orientation_error_degrees=3;break;
        case 5:f.s.hand_in_root_m={0,.051,0};break;case 6:f.s.bone_measured=false;break;case 7:f.s.path_generation=0;break;}
        assert(!f.begin());}
    for(unsigned error=0;error<7;++error){auto owner=std::make_unique<Fixture>();auto& f=*owner;assert(f.begin());f.next(.02,0);
        switch(error){case 0:f.s.stamp.left_tracked=false;break;case 1:f.s.lease_current=false;break;case 2:++f.s.lease;break;
        case 3:++f.s.stamp.identity.space_epoch;break;case 4:f.s.stamp.observed_ns+=100000001;break;
        case 5:f.s.grip_down=false;break;case 6:++f.s.path_generation;break;}
        assert(f.step()==BoltResult::interrupted&&!f.stroke.active());}
    {auto owner=std::make_unique<Fixture>();auto& f=*owner;assert(f.begin());f.next(.1,0);assert(f.step()==BoltResult::interrupted);}
    {auto owner=std::make_unique<Fixture>();auto& f=*owner;assert(f.begin());auto duplicate=f.s;f.next(.02,0);assert(f.step()==BoltResult::moved);
        f.s=duplicate;assert(f.step()==BoltResult::interrupted);}
    {auto owner=std::make_unique<Fixture>();auto& f=*owner;assert(f.begin());f.next(.02,0);f.s.stamp.sequence=1;assert(f.step()==BoltResult::waiting);}
    {auto owner=std::make_unique<Fixture>();auto& f=*owner;assert(f.begin());f.next(.1,0);f.s.stamp.observed_ns+=80000000;assert(f.step()==BoltResult::moved);
        f.next(.1,1);f.s.position_error_m=.003;assert(f.step()==BoltResult::waiting);
        f.next(.1,1);f.s.position_error_m=0;f.s.orientation_error_degrees=3;assert(f.step()==BoltResult::waiting);
        f.next(.1,1);f.s.orientation_error_degrees=0;assert(f.step()==BoltResult::complete);}
    std::puts("Longshot physical bolt stroke: opening/closing, native acknowledgement, contact, lease and discontinuity checks passed");
}
