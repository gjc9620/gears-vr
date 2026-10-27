#include "body_slots.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
using namespace gears_vr;
using namespace gears_weapon;
namespace {
unsigned checks{};
void check(bool condition){
    ++checks;
    if(!condition){std::fprintf(stderr,"armspan regression check %u failed\n",checks);std::exit(1);}
}
struct Input {
    StereoSample stereo{};
    ControllerInputFrame hands{};
    Input(){
        stereo.session=hands.session=4;stereo.space_epoch=hands.space_epoch=1;
        stereo.focused=hands.focused=1;stereo.head={{0,1.65,0},{0,0,0,1}};
        for(unsigned i=0;i<2;++i){
            auto& hand=hands.hands[i];hand.grip_active=1;hand.grip_flags=15;
            hand.grip={{i?.85:-.85,1.5,0},{0,0,0,1}};
            hand.squeeze_active=1;hand.squeeze=.95f;
        }
    }
    void stamp(std::uint64_t sequence,std::int64_t ns){
        stereo.sequence=hands.sequence=sequence;
        stereo.predicted_time=hands.predicted_time=std::int64_t(sequence)+100;
        stereo.received_ns=hands.received_ns=ns;hands.sync_ns=ns-1000;
    }
    bool observe(ArmspanCalibrator& calibrator,std::uint64_t sequence,std::int64_t ns){
        stamp(sequence,ns);return calibrator.observe(hands,stereo,ns);
    }
};
void duplicate_holds(){
    // 60 unique samples span exactly two seconds, with repeated render polls.
    ArmspanCalibrator calibrator;Input input;
    for(int i=0;i<60;++i){
        const auto ns=1000000000LL+2000000000LL*i/59;
        check(input.observe(calibrator,std::uint64_t(i+1),ns)==(i==59));
        if(i==59)break;
        const auto progress=calibrator.progress();
        for(auto delay:{5000000LL,10000000LL}){
            check(!calibrator.observe(input.hands,input.stereo,ns+delay));
            check(calibrator.progress()==progress&&!calibrator.result());
        }
    }
    check(calibrator.result()->valid());
    check(std::abs(calibrator.result()->armspan_metres-1.7)<.001);
    check(std::abs(calibrator.result()->hands_below_head_metres+.15)<.001);

    // Elapsed time alone cannot promote duplicate polls into the 60th sample.
    calibrator.reset();
    for(int i=0;i<59;++i)check(!input.observe(calibrator,std::uint64_t(i+1),4000000000LL+i*40000000LL));
    const auto progress=calibrator.progress();
    for(int i=1;i<=3;++i){
        check(!calibrator.observe(input.hands,input.stereo,input.hands.received_ns+i*5000000LL));
        check(calibrator.progress()==progress&&!calibrator.result());
    }
    check(input.observe(calibrator,60,6360000000LL));

    // Enough unique samples, but only a later unique sample advances hold time.
    calibrator.reset();
    for(int i=0;i<100;++i)check(!input.observe(calibrator,std::uint64_t(i+1),7000000000LL+i*20000000LL));
    const auto before=calibrator.progress();
    check(!calibrator.observe(input.hands,input.stereo,9000000000LL));
    check(calibrator.progress()==before&&!calibrator.result());
    check(input.observe(calibrator,101,9020000000LL));
}
void changed_or_rollback(){
    // Every mutation remains otherwise valid: same-sequence changes must be
    // rejected without using the disputed sample to begin another hold.
    for(unsigned mutation=0;mutation<13;++mutation){
        ArmspanCalibrator calibrator;Input input;
        check(!input.observe(calibrator,100,1000000000LL));
        check(!input.observe(calibrator,101,1040000000LL));
        check(calibrator.progress()>0);
        switch(mutation){
            case 0:input.stereo.head.position.x+=.001;break;
            case 1:input.stereo.head.orientation={0,std::sin(.01),0,std::cos(.01)};break;
            case 2:input.hands.hands[0].grip.position.x+=.001;break;
            case 3:input.hands.hands[1].grip.position.x+=.001;break;
            case 4:input.hands.hands[0].grip.orientation={std::sin(.01),0,0,std::cos(.01)};break;
            case 5:input.hands.hands[1].grip.orientation={0,0,std::sin(.01),std::cos(.01)};break;
            case 6:input.hands.hands[0].squeeze=.9f;break;
            case 7:input.hands.hands[1].grip_flags|=16;break;
            case 8:++input.hands.predicted_time;++input.stereo.predicted_time;break;
            case 9:++input.hands.received_ns;break;
            case 10:++input.hands.sync_ns;break;
            case 11:++input.stereo.received_ns;break;
            case 12:--input.hands.sequence;--input.stereo.sequence;break;
        }
        check(!calibrator.observe(input.hands,input.stereo,1041000000LL));
        check(calibrator.progress()==0&&!calibrator.result());
        Input next;
        for(int i=0;i<60;++i)
            check(next.observe(calibrator,std::uint64_t(i+200),1060000000LL+i*40000000LL)==(i==59));
    }
}
void source_boundaries(){
    for(unsigned boundary=0;boundary<4;++boundary){
        ArmspanCalibrator calibrator;Input input;
        check(!input.observe(calibrator,100,1000000000LL));
        check(!input.observe(calibrator,101,1040000000LL));
        switch(boundary){
            case 0:++input.hands.session;++input.stereo.session;break;
            case 1:++input.hands.space_epoch;++input.stereo.space_epoch;break;
            case 2:++input.hands.profile_generation;break;
            case 3:++input.hands.input_epoch;break;
        }
        // A new key may legitimately restart its sequence at one.
        check(!input.observe(calibrator,1,1080000000LL));
        check(calibrator.progress()==0&&!calibrator.result());
        for(int i=1;i<60;++i)
            check(input.observe(calibrator,std::uint64_t(i+1),1080000000LL+i*40000000LL)==(i==59));
    }
    ArmspanCalibrator calibrator;Input input;
    check(!input.observe(calibrator,1,2000000000LL));
    check(!input.observe(calibrator,2,2020000000LL));
    // Unique input with a rolled-back observer clock starts a fresh window.
    check(!input.observe(calibrator,3,2010000000LL));
    check(calibrator.progress()==0&&!calibrator.result());
    check(!input.observe(calibrator,4,2050000000LL));
    check(calibrator.progress()>.019&&calibrator.progress()<.021);
    // Duplicate polls cannot conceal a gap between actual accepted samples.
    check(!calibrator.observe(input.hands,input.stereo,2140000000LL));
    check(!input.observe(calibrator,5,2160000000LL));
    check(calibrator.progress()==0&&!calibrator.result());
}
void invalid_duplicates(){
    for(unsigned invalid=0;invalid<9;++invalid){
        ArmspanCalibrator calibrator;Input input;
        check(!input.observe(calibrator,1,1000000000LL));
        check(!input.observe(calibrator,2,1020000000LL));
        auto now=1021000000LL;
        switch(invalid){
            case 0:input.hands.hands[0].grip_flags=3;break;
            case 1:input.hands.hands[1].grip_active=0;break;
            case 2:input.hands.hands[0].squeeze=0;break;
            case 3:input.hands.hands[1].squeeze_active=0;input.hands.hands[1].squeeze=0;break;
            case 4:input.hands.focused=input.stereo.focused=0;break;
            case 5:now=input.hands.received_ns+100000001;break;
            case 6:now=input.hands.received_ns-1;break;
            case 7:input.stereo.head.orientation={0,0,0,0};break;
            case 8:input.hands.hands[1].grip.position.x=.1;break;
        }
        check(!calibrator.observe(input.hands,input.stereo,now));
        check(calibrator.progress()==0&&!calibrator.result());
    }
    ArmspanCalibrator calibrator;Input input;
    check(!input.observe(calibrator,1,2000000000LL));
    check(!input.observe(calibrator,2,2020000000LL));
    const auto progress=calibrator.progress();
    check(!calibrator.observe(input.hands,input.stereo,2120000000LL));
    check(calibrator.progress()==progress);
    check(!calibrator.observe(input.hands,input.stereo,2120000001LL));
    check(calibrator.progress()==0&&!calibrator.result());
}
}
int main(){
    assert(!original_inventory_slot(BodySlot::none));
    assert(!original_inventory_slot(BodySlot::shoulder_ammo));
    assert(original_inventory_slot(BodySlot::pistol)==0);
    assert(original_inventory_slot(BodySlot::grenade)==1);
    assert(original_inventory_slot(BodySlot::weapon_2)==2);
    assert(original_inventory_slot(BodySlot::weapon_1)==3);
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
    duplicate_holds();changed_or_rollback();source_boundaries();invalid_duplicates();
    std::puts("armspan hold, reset, hand-specific shoulder and hip slot checks passed");
    std::printf("armspan immutable duplicates, 60 unique/2s, rollback, mutation, source boundaries and invalidation: %u checks passed\n",checks);
    std::printf("ArmspanCalibrator persistent state: %zu bytes; no full frame copy in observe\n",sizeof(ArmspanCalibrator));
}
