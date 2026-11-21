#include "weapon_gnasher_producer_motion.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace gears_weapon;
using namespace gears_vr;
static void check(bool condition,char const* expression,int line){
    if(!condition){std::fprintf(stderr,"line %d: %s\n",line,expression);std::exit(1);}
}
#define CHECK(expression) check(bool(expression),#expression,__LINE__)

static GnasherMechanicalConfig synthetic_config(){
    GnasherMechanicalConfig c{};
    c.calibration_id=1;c.lever_calibrated=c.feed_calibrated=true;
    c.capacity=8;c.game_ticks_per_second=1000;
    c.lever.left_front_grip_in_main.position={0,0,.08};
    c.lever.pivot_in_main={0,0,0};
    c.lever.right_handle_home_in_main={0,-.2,0};
    c.lever.hinge_axis_in_main={1,0,0};
    return c;
}

static gears_hand_continuity::TrackedInput sample(std::uint64_t sequence){
    gears_hand_continuity::TrackedInput t{};t.stream_epoch=t.tracking_epoch[0]=t.tracking_epoch[1]=1;
    auto& input=t.input;input.source=gears_vr::StereoSource::MetaOperatorSimulated;input.session=3;input.sequence=sequence;
    input.space_epoch=5;input.input_epoch=7;input.profile_generation=9;
    input.received_ns=1000000000+sequence*20000000;input.sync_ns=input.received_ns;input.predicted_time=input.sync_ns;
    input.focused=1;input.display_mode=2;input.display_observed_ns=input.sync_ns;
    for(unsigned i=0;i<2;++i){
        auto& h=input.hands[i];h.grip.position={0.,.1,0.};h.grip.orientation={0.,0.,0.,1.};
        h.aim=h.grip;h.grip_flags=h.aim_flags=15;h.grip_active=h.aim_active=1;
        h.squeeze_active=h.trigger_active=1;h.squeeze=h.trigger=.9f;
        for(unsigned n=0;n<gears_vr::control_count;++n){h.pressed[n]=0;h.released[n]=0;}
        h.last_squeeze_press_grip=h.grip;h.last_squeeze_press_flags=1;h.last_squeeze_press_sequence=0;
    }
    // In UE relative coordinates: right=(0,-.2,-.08), matching the calibrated home.
    input.hands[0].grip.position={0.,.1,0.};
    input.hands[1].grip.position={-.2,.02,0.};
    return t;
}

static void test_case(unsigned count,bool expected){
    const auto config=synthetic_config();CHECK(config.valid());
    const GnasherIdentity identity{0x11,0x22,0x33,0x44,3,5,7,9};
    CHECK(identity.valid());
    gears_gnasher_producer_segment::Segment source{};source.count=count;
    for(unsigned i=0;i<count;++i)source.points[i]=sample(100+i);
    GnasherLeverMotion out{};out.count=77;out.points[0].lever_angle_radians=91.;
    const bool accepted=gears_gnasher_producer_segment::lever_motion(source,config,identity,out);
    CHECK(accepted==expected);
    if(accepted){
        CHECK(out.count==count&&out.points[count-1].sequence==100+count-1);
        CHECK(out.points[0].weapon==identity.weapon&&out.points[0].world==identity.world);
    }else{
        CHECK(out.count==0);
        for(unsigned i=0;i<out.points.size();++i){
            CHECK(out.points[i].weapon==0&&out.points[i].world==0&&out.points[i].sequence==0);
        }
    }
}

int main(){
    test_case(9,true);
    test_case(10,false);
    test_case(33,false);
    std::printf("Gnasher producer motion: 9-point success; 10/33 reject before destination write\n");
    return 0;
}