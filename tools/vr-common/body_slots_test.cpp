#include "body_slots.h"
#include <cassert>
#include <cstdio>
using namespace gears_vr;
using namespace gears_weapon;
int main(){
    ArmspanCalibrator calibrator;
    StereoSample stereo{};stereo.session=4;stereo.space_epoch=1;stereo.focused=1;
    stereo.head={{0,1.65,0},{0,0,0,1}};
    ControllerInputFrame input{};input.session=4;input.space_epoch=1;
    input.profile_generation=1;input.focused=1;input.predicted_time=123;
    for(unsigned hand=0;hand<2;++hand){
        auto& value=input.hands[hand];value.grip_active=1;value.grip_flags=15;
        value.grip={{hand?.85:-.85,1.5,0},{0,0,0,1}};
        value.squeeze_active=1;value.squeeze=.95f;
    }
    for(int i=0;i<=200;++i){
        const auto ns=1000000000LL+i*10000000LL;
        stereo.sequence=input.sequence=std::uint64_t(i+1);
        stereo.predicted_time=input.predicted_time=123+i;
        input.sync_ns=ns-1000;input.received_ns=ns;
        assert(calibrator.observe(input,stereo,ns)==(i==200));
    }
    const auto measured=*calibrator.result();
    assert(std::abs(measured.armspan_metres-1.7)<.001);
    assert(std::abs(measured.hands_below_head_metres+.15)<.001);
    const Pose body{{0,1.65,0},{0,0,0,1}};
    assert(body_slot(measured,body,{-1.7*.135,1.65-.15+.035,.05},0)==BodySlot::shoulder_ammo);
    assert(body_slot(measured,body,{-1.7*.135,1.65-.15+.035,.05},1)==BodySlot::weapon_2);
    assert(body_slot(measured,body,{1.7*.135,1.65-.15+.035,.05},1)==BodySlot::weapon_1);
    assert(body_slot(measured,body,{-1.7*.135,1.65-.15+.035-.55,.02},1)==BodySlot::pistol);
    assert(body_slot(measured,body,{1.7*.135,1.65-.15+.035-.55,.02},1)==BodySlot::grenade);
    assert(body_slot(measured,body,{3,3,3},1)==BodySlot::none);
    calibrator.reset();
    for(int i=0;i<100;++i){
        const auto ns=4000000000LL+i*10000000LL;
        stereo.sequence=input.sequence=std::uint64_t(300+i);
        stereo.predicted_time=input.predicted_time=500+i;
        input.sync_ns=ns-1000;input.received_ns=ns;
        assert(!calibrator.observe(input,stereo,ns));
    }
    input.hands[0].squeeze=0;input.received_ns+=10000000;input.sync_ns=input.received_ns-1000;
    stereo.sequence=input.sequence=400;stereo.predicted_time=input.predicted_time=600;
    assert(!calibrator.observe(input,stereo,input.received_ns));
    assert(calibrator.progress()==0&&!calibrator.result());
    std::puts("armspan hold, reset, hand-specific shoulder and hip slot checks passed");
}
