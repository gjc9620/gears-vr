#include "weapon_troika_fixed_space.h"
#include <cstdio>
#include <cstdlib>
namespace {unsigned checks{};void check(bool x,int line){++checks;if(!x){std::fprintf(stderr,"line %d\n",line);std::exit(1);}}
#define CHECK(x) check(bool(x),__LINE__)
}
int main(){
    using namespace gears_weapon;using namespace gears_vr;
    constexpr std::int64_t now=2000000000;
    TroikaIdentity owner{1,2,3,4,5,6,7,8,191,211,281,271};
    TroikaSpaceCalibration c{{1,0,0,0,0,1,0,0,0,0,1,0,4,5,6,1},{},100,111};
    StereoSample s{};s.session=191;s.sequence=201;s.space_epoch=211;s.received_ns=now-2000000;
    s.predicted_time=100000;s.head_flags=s.view_flags=15;s.focused=s.visible=s.should_render=1;s.head={};
    for(unsigned i=0;i<2;++i){s.eyes[i].pose={};s.eyes[i].pose.position.x=i?.032:-.032;s.eyes[i].fov={-.7,.7,.6,-.6};}
    ControllerInputFrame input{};input.source=s.source;input.session=s.session;input.sequence=s.sequence;input.space_epoch=s.space_epoch;
    input.profile_generation=271;input.input_epoch=281;input.predicted_time=s.predicted_time;
    input.received_ns=now-3000000;input.sync_ns=now-4000000;input.focused=1;
    for(unsigned i=0;i<2;++i){auto& h=input.hands[i];h.grip_active=h.aim_active=1;h.grip_flags=h.aim_flags=15;
        h.grip={{.1+double(i)*.2,.4,-.5},{.6,0,0,.8}};h.aim=h.grip;}
    TroikaFixedSpace mapping;CHECK(mapping.bind(owner,c));CHECK(!mapping.bind(owner,c));
    TroikaMappedHands first{};CHECK(mapping.map(owner,111,input,s,now,first));CHECK(first.tracked_mask==3);
    CHECK(first.input_sync_ns==input.sync_ns&&first.input_received_ns==input.received_ns);
    auto changed=s;changed.head.orientation={0,.6,0,.8};TroikaMappedHands head{};
    CHECK(mapping.map(owner,111,input,changed,now,head));
    for(unsigned i=0;i<2;++i)CHECK(stereo_same_pose(first.world[i],head.world[i]));
    auto sentinel=first;auto invalid=owner;++invalid.turret_generation;
    CHECK(!mapping.map(invalid,111,input,s,now,sentinel));CHECK(stereo_same_pose(sentinel.world[0],first.world[0]));
    CHECK(!mapping.map(owner,112,input,s,now,sentinel));CHECK(!mapping.map(owner,111,input,s,now+100000001,sentinel));
    auto stale=input;stale.sync_ns=now-100000001;CHECK(!mapping.map(owner,111,stale,s,now,sentinel));
    auto missing=input;missing.hands[0].grip_active=0;missing.hands[0].grip_flags=0;
    CHECK(mapping.map(owner,111,missing,s,now,sentinel));CHECK(sentinel.tracked_mask==2);
    mapping.reset();CHECK(!mapping.map(owner,111,input,s,now,sentinel));
    std::printf("Troika fixed entry-space mapping: %u checks passed; Native mounted producer and view integration pending.\n",checks);
}
