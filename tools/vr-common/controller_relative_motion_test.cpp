#include "controller_relative_motion.h"
#include <cassert>
#include <cstdio>
#include <limits>
namespace c=gears_hand_continuity;namespace r=gears_controller_relative_motion;
c::TrackedInput input(unsigned sequence){
    c::TrackedInput value{};value.stream_epoch=1;value.tracking_epoch[0]=value.tracking_epoch[1]=1;
    auto& p=value.input;p.source=gears_vr::StereoSource::RuntimeTracked;p.session=1;p.sequence=sequence;
    p.space_epoch=p.input_epoch=p.profile_generation=1;p.sync_ns=1000000000+std::int64_t(sequence-1)*22000000;
    p.received_ns=p.sync_ns+1000;p.predicted_time=p.sync_ns+10000000;p.focused=1;
    for(auto& h:p.hands){h.grip_active=h.aim_active=h.trigger_active=h.squeeze_active=1;h.grip_flags=h.aim_flags=15;}
    p.hands[0].squeeze=1;p.hands[0].pressed[gears_vr::control_squeeze]=1;
    p.hands[0].grip.position.z=-.02*(sequence-1);
    assert(c::tracked_input_shape_valid(value));return value;
}
int main(){
    unsigned cases{};const auto previous=input(1),current=input(6);
    c::MotionHistory history{};history.count=5;for(unsigned i=0;i<5;++i)history.frames[i]=input(i+2);
    r::Segment segment{};assert(r::build(previous,current,history,gears_vr::control_squeeze,{},1,segment));
    assert(segment.count==6);
    for(unsigned i=0;i<segment.count;++i){assert(segment.points[i].sequence==i+1);
        assert(segment.points[i].sync_ns==input(i+1).input.sync_ns&&std::abs(segment.points[i].position.x-.02*i)<1e-12);}
    ++cases;
    for(unsigned failure=0;failure<19;++failure){
        auto h=history;auto a=previous,b=current;gears_vr::Pose part{};double scale=1;unsigned control=gears_vr::control_squeeze;
        switch(failure){
        case 0:control=gears_vr::control_count;break;
        case 1:h.frames[0]=input(3);break;
        case 2:std::swap(h.frames[1],h.frames[2]);break;
        case 3:++h.frames[2].tracking_epoch[0];break;
        case 4:++h.frames[2].tracking_epoch[1];break;
        case 5:++h.frames[2].input.space_epoch;break;
        case 6:++h.frames[2].input.input_epoch;break;
        case 7:h.frames[2].input.focused=0;break;
        case 8:h.frames[2].input.display_mode=1;break;
        case 9:h.frames[2].input.hands[0].grip_flags=0;break;
        case 10:h.frames[2].input.hands[0].pressed[control]=2;h.frames[2].input.hands[0].released[control]=1;break;
        case 11:h.frames[2].input.sync_ns=h.frames[1].input.sync_ns;break;
        case 12:b.input.hands[0].grip.position.x+=.01;break;
        case 13:h.reserved=1;break;
        case 14:h.count=c::motion_history_capacity+1;break;
        case 15:scale=0;break;
        case 16:part.orientation.x=std::numeric_limits<double>::quiet_NaN();break;
        case 17:a.stream_epoch=0;break;
        case 18:h.frames[2].input.hands[1].grip_active=0;break;
        }
        r::Segment unchanged{};unchanged.count=99;
        assert(!r::build(a,b,h,control,part,scale,unchanged)&&unchanged.count==99);++cases;
    }
    // The cached genuine anchor may precede the oldest available ring row.
    history.count=8;for(unsigned i=0;i<8;++i)history.frames[i]=input(i+2);
    assert(r::build(previous,input(9),history,gears_vr::control_squeeze,{},1,segment)&&segment.count==9);++cases;
    auto raw=input(3);raw.input.hands[1].grip={{.1,1.5,-.2},{0,std::sin(.2),0,std::cos(.2)}};
    raw.input.hands[0].grip.position={.3,1.4,-.4};
    gears_vr::Pose part{{.05,.02,-.03},{std::sin(.15),0,0,std::cos(.15)}};
    gears_vr::Vector before{},after{};assert(r::project(raw,part,1.25,before));
    const gears_vr::Quaternion q{0,0,std::sin(.275),std::cos(.275)};
    for(auto& hand:raw.input.hands){auto p=gears_vr::transform(gears_vr::rotation_matrix(q),hand.grip.position);
        hand.grip.position={p.x+.6,p.y+.2,p.z-.1};hand.grip.orientation=gears_vr::multiply(q,hand.grip.orientation);}
    assert(r::project(raw,part,1.25,after));
    assert(std::hypot(std::hypot(before.x-after.x,before.y-after.y),before.z-after.z)<1e-12);++cases;
    std::printf("ControllerRelativeMotionTest passed:%u cases; original timestamps, exact source chain, both-hand tracking, held edge, atomic failure and relative6DoF invariance\n",cases);
}
