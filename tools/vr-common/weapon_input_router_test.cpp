#include "weapon_input_router.h"
#include <cassert>
#include <cstdio>
using namespace gears_vr;
using namespace gears_weapon;

int main(){
    const ArmspanCalibration calibration{1,1.7,-.15};
    const Pose body{{0,1.65,0},{0,0,0,1}};
    StereoSample stereo{};stereo.session=7;stereo.sequence=1;
    stereo.space_epoch=1;stereo.predicted_time=100;stereo.focused=1;
    ControllerInputFrame input{};input.session=7;input.sequence=1;
    input.space_epoch=1;input.predicted_time=100;input.focused=1;
    for(unsigned hand=0;hand<2;++hand){
        input.hands[hand].grip_active=1;input.hands[hand].grip_flags=15;
        input.hands[hand].squeeze_active=1;
    }
    ControllerEdgeCounter edges;WeaponInputRouter router;
    auto step=[&](std::int64_t ns){
        input.sync_ns=ns-100;input.received_ns=ns;
        edges.update(input);
        return router.advance(input,stereo,calibration,body,ns+100);
    };
    assert(step(1000000000).status==RouterStatus::baseline);
    input.sequence=stereo.sequence=2;input.predicted_time=stereo.predicted_time=101;
    input.hands[1].grip.position={-1.7*.135,1.65-.15+.035,.05};
    input.hands[1].squeeze=.9f;
    auto event=step(1010000000);
    assert(event.status==RouterStatus::accepted&&event.count==1&&event.intents[0]==WeaponIntent::weapon_2);
    input.sequence=stereo.sequence=3;input.predicted_time=stereo.predicted_time=102;
    input.hands[1].squeeze=0;event=step(1020000000);
    assert(event.count==0); // Right grip release does not drop the weapon.
    input.sequence=stereo.sequence=4;input.predicted_time=stereo.predicted_time=103;
    input.hands[1].grip.position={1.7*.135,1.65-.15+.035,.05};
    input.hands[1].squeeze=.9f;event=step(1030000000);
    assert(event.count==1&&event.intents[0]==WeaponIntent::weapon_1);
    input.sequence=stereo.sequence=5;input.predicted_time=stereo.predicted_time=104;
    input.hands[0].grip.position={-1.7*.135,1.65-.15+.035,.05};
    input.hands[0].squeeze=.9f;input.hands[1].buttons_active=controller_stick_click;
    input.hands[1].buttons=controller_stick_click;event=step(1040000000);
    assert(event.count==2&&event.intents[0]==WeaponIntent::reload&&event.intents[1]==WeaponIntent::grab_ammo);
    input.sequence=stereo.sequence=6;input.predicted_time=stereo.predicted_time=105;
    input.hands[0].squeeze=0;event=step(1050000000);
    assert(event.count==1&&event.intents[0]==WeaponIntent::release_left_grip);
    // The game sees the final frame only; cumulative counts retain a short R3 tap.
    input.sequence=stereo.sequence=7;input.predicted_time=stereo.predicted_time=106;
    input.hands[1].buttons=0;input.sync_ns=1060000000;input.received_ns=1060000100;
    edges.update(input);
    input.sequence=stereo.sequence=8;input.predicted_time=stereo.predicted_time=107;
    input.hands[1].buttons=controller_stick_click;event=step(1070000000);
    assert(event.count==1&&event.intents[0]==WeaponIntent::reload);
    input.focused=0;stereo.focused=0;input.sequence=stereo.sequence=9;
    input.predicted_time=stereo.predicted_time=108;
    assert(step(1080000000).status==RouterStatus::rejected);
    input.focused=stereo.focused=1;input.sequence=stereo.sequence=10;
    input.predicted_time=stereo.predicted_time=109;
    assert(step(1090000000).status==RouterStatus::baseline); // Held R3 cannot retrigger.
    input.sequence=stereo.sequence=11;input.predicted_time=stereo.predicted_time=110;
    auto wrong=stereo;wrong.predicted_time=999;
    input.sync_ns=1100000000;input.received_ns=1100000100;edges.update(input);
    assert(router.advance(input,wrong,calibration,body,1100000200).status==RouterStatus::rejected);
    input.sequence=stereo.sequence=12;input.predicted_time=stereo.predicted_time=111;
    input.hands[1].squeeze=0;assert(step(1110000000).status==RouterStatus::baseline);
    input.sequence=stereo.sequence=13;input.predicted_time=stereo.predicted_time=112;
    input.hands[1].grip.position={-1.7*.135,1.65-.15+.035-.55,.02};
    input.hands[1].squeeze=.9f;input.sync_ns=1120000000;input.received_ns=1120000100;
    edges.update(input); // Grip press at the pistol pocket is not read by the game yet.
    input.sequence=stereo.sequence=14;input.predicted_time=stereo.predicted_time=113;
    input.hands[1].grip.position={3,3,3};event=step(1130000000);
    assert(event.count==1&&event.intents[0]==WeaponIntent::pistol);
    std::puts("weapon intent shoulder identity, toggle grip, reload edge and focus reset checks passed");
}
