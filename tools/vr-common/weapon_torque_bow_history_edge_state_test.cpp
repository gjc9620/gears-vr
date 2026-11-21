#include "weapon_torque_bow_controls.h"
#include <cassert>
#include <cstdio>
namespace c=gears_torque_bow_controls;namespace hc=gears_hand_continuity;namespace v=gears_vr;
v::StereoSample stereo(v::ControllerInputFrame const& f){v::StereoSample s{};s.source=f.source;s.session=f.session;s.sequence=f.sequence;s.space_epoch=f.space_epoch;s.focused=1;s.predicted_time=f.predicted_time;s.received_ns=f.received_ns;return s;}
c::Continuity generate(unsigned kind){
    c::Continuity out{};out.publisher={17,29};out.history.count=8;v::ControllerEdgeCounter counter;
    for(unsigned i=0;i<8;++i){auto& t=out.history.frames[i];t.stream_epoch=3;t.tracking_epoch[0]=5;t.tracking_epoch[1]=7;auto& f=t.input;
        f.session=f.space_epoch=f.profile_generation=f.input_epoch=1;f.sequence=100+i;f.focused=1;
        f.predicted_time=f.sync_ns=f.received_ns=1040000000+i*20000000;
        for(auto& h:f.hands){h.grip_active=1;h.grip_flags=15;}
        auto& h=f.hands[1];h.trigger_active=1;h.buttons_active=v::controller_stick_click;
        if(kind==0)h.trigger=i>=4?1.f:0.f;
        if(kind==1)h.trigger=i<4?1.f:0.f;
        if(kind==2)h.trigger=1;
        if(kind==3)h.buttons=i>=4?v::controller_stick_click:0;
        if(kind==4)h.buttons=i==3?v::controller_stick_click:0;
        counter.update(f);
    }
    // Contradict just one original sample's state without changing its valid
    // shape/counters/epoch/sequence. Final endpoint stays coherent. This is
    // malformed evidence, not a claim that the real producer emitted it.
    auto& h=out.history.frames[4].input.hands[1];
    if(kind==0)h.trigger=0;
    if(kind==1)h.trigger=1;
    if(kind==2)h.trigger=0;
    if(kind==3)h.buttons=0;
    if(kind==4||kind==5)h.buttons=v::controller_stick_click;
    if(kind==6)h.trigger=1;
    out.current=out.history.frames[7];return out;
}
int main(){unsigned checked=0;
    for(unsigned kind=0;kind<7;++kind){auto proof=generate(kind);assert(hc::motion_history_valid(proof.history,proof.current,1));
        c::Decoder decoder;auto base=proof;base.current=proof.history.frames[0];base.history.count=1;
        auto first=decoder.advance(base.current.input,stereo(base.current.input),1040000000,false,false,&base);assert(first.status==c::Status::baseline);
        auto result=decoder.advance(proof.current.input,stereo(proof.current.input),1200000000,true,false,&proof);
        assert(result.status==c::Status::invalid&&result.rejection==c::Rejection::observation_gap&&!result.count&&!result.release_observed);
        ++checked;
    }
    std::printf("%u malformed interior state/counter histories %s; endpoint and existing motion validation remain valid.\n",checked,
        "rejected"
    );
}
