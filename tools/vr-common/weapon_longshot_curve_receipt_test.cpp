#include "weapon_longshot_curve_receipt.h"
#include <cassert>
#include <cstdio>
#include <functional>
#include <limits>
#include <vector>
namespace h=gears_weapon_hand;namespace receipt=gears_longshot_curve;
struct Observation {h::ObjectIdentity weapon{},component{},asset{},node{},sequence{};float reload_start{1},length{.8f},time{.4f},rate{};};
h::ObjectIdentity identity(unsigned address){return {address,0,4096,address/8,address,0,1};}
struct Fixture {
    h::Frame f{};Observation sample{};
    Fixture(){
        f.key={1,2,3,4,5,6,17,19};f.flags=15;f.now_ns=2000;
        f.world=identity(64);f.controller=identity(80);f.pawn=identity(96);f.mesh=identity(112);f.weapon=identity(128);
        f.reference_generation=7;f.press_token=8;f.interaction_control=1;f.left_grip_known=f.left_grip_down=f.left_tracked=1;
        auto& input=f.controllers;input.source=gears_vr::StereoSource::MetaOperatorSimulated;
        input.session=11;input.sequence=12;input.space_epoch=13;input.profile_generation=14;input.input_epoch=15;
        input.predicted_time=10000;input.received_ns=1001;input.sync_ns=1000;input.focused=1;
        sample.weapon=f.weapon;sample.component=identity(144);sample.asset=identity(160);sample.node=identity(176);sample.sequence=identity(192);
    }
};
int main(){
    unsigned cases{};Fixture v;const auto accepted=receipt::capture(v.f,v.sample);
    assert(accepted.complete&&receipt::matches(accepted,v.f,v.sample));++cases;
    using Change=std::function<void(h::Frame&)>;
    const std::vector<Change> changes={
      [](auto& f){++f.key.game_start;},[](auto& f){++f.key.native_started_qpc;},[](auto& f){++f.key.registration_cookie;},
      [](auto& f){++f.key.lifecycle;},[](auto& f){++f.key.frame;},[](auto& f){++f.key.world_generation;},
      [](auto& f){++f.key.game_pid;},[](auto& f){++f.key.thread_id;},
      [](auto& f){f.world.address+=8;},[](auto& f){++f.controller.registry_index;},[](auto& f){f.pawn.klass+=8;},
      [](auto& f){++f.mesh.name_number;},[](auto& f){f.weapon.address+=8;},
      [](auto& f){++f.reference_generation;},[](auto& f){++f.press_token;},[](auto& f){f.flags^=1;},
      [](auto& f){++f.interaction_control;},[](auto& f){++f.now_ns;},[](auto& f){++f.size;},[](auto& f){++f.abi_version;},
      [](auto& f){f.left_grip_known=0;},[](auto& f){f.left_grip_down=0;},[](auto& f){f.left_tracked=0;},
      [](auto& f){f.controllers.source=gears_vr::StereoSource::RuntimeTracked;},
      [](auto& f){++f.controllers.session;},[](auto& f){++f.controllers.sequence;},[](auto& f){++f.controllers.space_epoch;},
      [](auto& f){++f.controllers.profile_generation;},[](auto& f){++f.controllers.input_epoch;},
      [](auto& f){++f.controllers.predicted_time;},[](auto& f){++f.controllers.received_ns;},[](auto& f){++f.controllers.sync_ns;},
      [](auto& f){f.controllers.focused=0;},[](auto& f){++f.controllers.hands[0].pressed[gears_vr::control_squeeze];},
      [](auto& f){++f.controllers.hands[0].released[gears_vr::control_squeeze];},
      [](auto& f){f.controllers.hands[0].grip.position.x+=.01;},[](auto& f){f.controllers.hands[1].grip.orientation.x+=.01;},
      [](auto& f){f.controllers.hands[0].squeeze=.5f;},[](auto& f){f.controllers.hands[1].grip_flags^=1;}
    };
    for(auto const& change:changes){auto f=v.f;change(f);assert(!receipt::matches(accepted,f,v.sample));++cases;}
    using ObjectChange=std::function<void(Observation&)>;
    for(auto const& change:std::vector<ObjectChange>{
      [](auto& o){o.weapon.address+=8;},[](auto& o){o.component.address+=8;},[](auto& o){++o.asset.registry_index;},
      [](auto& o){o.node.address+=8;},[](auto& o){++o.sequence.name_index;},[](auto& o){o.reload_start+=1;},[](auto& o){o.length+=1;}}){
        auto o=v.sample;change(o);assert(!receipt::matches(accepted,v.f,o));++cases;
    }
    for(unsigned invalid=0;invalid<8;++invalid){auto f=v.f;auto o=v.sample;
        switch(invalid){case 0:f.key.frame=0;break;case 1:f.key.game_pid=0;break;case 2:f.key.thread_id=0;break;
        case 3:f.controllers.sync_ns=0;break;case 4:o.node.known=0;break;case 5:o.sequence.known=0;break;
        case 6:o.reload_start=std::numeric_limits<float>::infinity();break;case 7:o.length=0;break;}
        const auto rejected=receipt::capture(f,o);assert(!rejected.complete&&!receipt::matches(rejected,f,o));++cases;
    }
    auto missing=accepted;missing.complete=false;assert(!receipt::matches(missing,v.f,v.sample));++cases;
    // This receipt does not certify current time/rate or freshness. Those
    // remain the ORIGINAL reconcile/current-source checks in seek_impl.
    auto current=v.sample;current.time=.5f;current.rate=1;
    assert(receipt::matches(accepted,v.f,current));++cases;
    std::printf("Longshot same-Frame curve receipt: %u context/input/actor cases; data only, no fresh or dispatch permission\n",cases);
}
