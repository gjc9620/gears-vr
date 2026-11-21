#include "weapon_torque_bow_controls.h"
#include <cassert>
#include <cstdio>
namespace c=gears_torque_bow_controls;namespace v=gears_vr;
namespace {
struct F {
    c::Decoder decoder;v::ControllerInputFrame input{};v::StereoSample stereo{};std::int64_t now{1000000000};
    F(){
        input.session=stereo.session=1;input.space_epoch=stereo.space_epoch=1;input.sequence=stereo.sequence=20;
        input.focused=stereo.focused=1;input.predicted_time=stereo.predicted_time=100;
        auto& h=input.hands[1];h.grip_active=h.trigger_active=1;h.grip_flags=15;
        h.buttons_active=v::controller_stick_click;stamp();
    }
    void stamp(){input.sync_ns=input.received_ns=stereo.received_ns=now;}
    void next(std::int64_t delta=10000000){now+=delta;++input.sequence;stereo.sequence=input.sequence;stamp();}
    void trigger(bool down){auto& h=input.hands[1];++(down?h.pressed[v::control_trigger]:h.released[v::control_trigger]);h.trigger=down?1.f:0.f;}
    void stick(bool down){auto& h=input.hands[1];++(down?h.pressed[v::control_stick]:h.released[v::control_stick]);h.buttons=down?v::controller_stick_click:0;}
    c::Result run(bool charged=false){return decoder.advance(input,stereo,now,charged);}
    void baseline(){auto r=run();assert(r.status==c::Status::baseline&&!r.count);}
};
void edge(c::Result r,c::Action a){assert(r.count==1&&r.actions[0]==a);}
}
int main(){
    {F f;f.baseline();f.next();f.trigger(true);edge(f.run(),c::Action::press);
     f.next();assert(f.run().status==c::Status::observed);f.next();f.trigger(false);edge(f.run(true),c::Action::release);}
    // A complete short trigger tap between GT publications preserves order.
    {F f;f.baseline();f.next();f.trigger(true);f.trigger(false);auto r=f.run();
     assert(r.count==2&&r.actions[0]==c::Action::press&&r.actions[1]==c::Action::release&&r.release_observed);
     r=f.run();assert(!r.count&&!r.release_observed&&r.status==c::Status::invalid);}
    {F f;f.trigger(true);f.baseline();f.next();f.trigger(false);f.trigger(true);auto r=f.run(true);
     assert(r.count==2&&r.actions[0]==c::Action::release&&r.actions[1]==c::Action::press);}
    // A skipped complete R3 tap cancels once. It never also releases/shoots.
    for(bool simultaneous_release:{false,true}){F f;f.trigger(true);f.baseline();f.next();
     f.stick(true);f.stick(false);if(simultaneous_release)f.trigger(false);
     auto r=f.run(true);assert(r.status==c::Status::cancel);edge(r,c::Action::cancel);
     f.next();assert(!f.run(true).count);}
    {F f;f.trigger(true);f.baseline();f.next();f.stick(true);f.trigger(false);
     edge(f.run(false),c::Action::release);} // Outside Charge, no R3 reload intent.
    // More than one cycle has no reliable original call order: quarantine it.
    for(bool use_stick:{false,true}){F f;f.baseline();f.next();for(unsigned i=0;i<2;++i){
      if(use_stick){f.stick(true);f.stick(false);}else{f.trigger(true);f.trigger(false);}}
     auto r=f.run(true);assert(r.status==c::Status::ambiguous&&!r.count&&!r.release_observed);
     f.next();assert(!f.run(true).count);}
    // Cohort changes and tracking recovery are baselines, never held presses.
    for(unsigned boundary=0;boundary<5;++boundary){F f;f.baseline();f.next();f.trigger(true);
     if(boundary==0)++f.input.session,++f.stereo.session;
     if(boundary==1)++f.input.space_epoch,++f.stereo.space_epoch;
     if(boundary==2)++f.input.profile_generation;
     if(boundary==3)++f.input.input_epoch;
     if(boundary==4){f.input.hands[1].grip_flags=3;assert(f.run(true).status==c::Status::invalid);f.input.hands[1].grip_flags=15;f.next();}
     auto r=f.run(true);assert(r.status==c::Status::baseline&&!r.count&&!r.release_observed);}
    {F f;f.trigger(true);f.baseline();f.next();f.input.hands[1].trigger=.5f;assert(!f.run(true).count);
     f.next();f.input.hands[1].trigger=.35f;f.trigger(false);auto r=f.run(true);assert(r.release_observed);edge(r,c::Action::release);}
    for(unsigned bad=0;bad<5;++bad){F f;f.baseline();f.next();f.trigger(true);
     if(bad==0)f.input.sync_ns=f.now-100000001;
     if(bad==1)f.input.sync_ns=f.now+1;
     if(bad==2)f.input.hands[1].buttons_active=0;
     if(bad==3)f.input.hands[1].trigger_active=0,f.input.hands[1].trigger=0;
     if(bad==4)f.input.focused=0;
     auto r=f.run(true);assert(r.status==c::Status::invalid&&!r.count&&!r.release_observed);}
    {F f;f.baseline();f.next(100000001);f.trigger(true);assert(f.run(true).status==c::Status::invalid);
     f.next();assert(f.run(true).status==c::Status::baseline);}
    {F f;f.trigger(true);f.baseline();f.next();f.input.hands[1].pressed[v::control_trigger]=0;
     assert(f.run(true).status==c::Status::invalid);}
    {F f;f.baseline();f.next();f.trigger(true);
     f.input.source=f.stereo.source=v::StereoSource::MetaOperatorSimulated;
     const auto r=f.decoder.advance(f.input,f.stereo,f.now,true,true);
     assert(r.status==c::Status::baseline&&!r.count&&!r.release_observed);} // Source changes never become physical edges.
    std::puts("Torque Bow controller edges: skipped taps, R3 cancel priority, replay, ambiguity, cohorts and tracking loss passed; synthetic XR only.");
}
