#include "weapon_torque_bow_controls.h"
#include <cassert>
#include <cstdio>
namespace c=gears_torque_bow_controls;namespace hc=gears_hand_continuity;namespace v=gears_vr;
v::ControllerInputFrame input(unsigned sequence,std::int64_t ns,float trigger,bool stick){
    v::ControllerInputFrame f{};f.session=f.space_epoch=f.profile_generation=f.input_epoch=1;
    f.sequence=sequence;f.focused=1;f.predicted_time=f.sync_ns=f.received_ns=ns;
    for(auto& h:f.hands){h.grip_active=1;h.grip_flags=15;}
    f.hands[1].trigger_active=1;f.hands[1].buttons_active=v::controller_stick_click;
    f.hands[1].trigger=trigger;f.hands[1].buttons=stick?v::controller_stick_click:0;return f;
}
v::StereoSample stereo(v::ControllerInputFrame const& f){v::StereoSample s{};s.source=f.source;s.session=f.session;s.sequence=f.sequence;s.space_epoch=f.space_epoch;s.focused=1;s.predicted_time=f.predicted_time;s.received_ns=f.received_ns;return s;}
int main(){unsigned checked=0;unsigned state=0x32a7;
    constexpr float values[]={0,.2f,.35f,.350001f,.5f,.649999f,.65f,.8f,1};
    for(bool held:{false,true})for(unsigned trial=0;trial<512;++trial){
        v::ControllerEdgeCounter counter;c::Decoder decoder;
        auto boot=input(99,1020000000,held?.8f:.2f,false);counter.update(boot);
        assert(decoder.advance(boot,stereo(boot),1020000000,false).status==c::Status::baseline);
        c::Continuity proof{};proof.publisher={17,29};proof.history.count=8;
        for(unsigned i=0;i<8;++i){state^=state<<13;state^=state>>17;state^=state<<5;
            auto& t=proof.history.frames[i];t.stream_epoch=3;t.tracking_epoch[0]=5;t.tracking_epoch[1]=7;
            const float value=i<3?.5f:values[state%9];const bool stick=i>=3&&(state&16)!=0;
            t.input=input(100+i,1040000000+i*20000000,value,stick);counter.update(t.input);
        }
        proof.current=proof.history.frames[7];assert(hc::motion_history_valid(proof.history,proof.current,1));
        auto base=proof;base.current=proof.history.frames[0];base.history.count=1;
        auto b=decoder.advance(base.current.input,stereo(base.current.input),1040000000,false,false,&base);
        assert(b.status==c::Status::observed); // .5 preserves prior up OR down.
        const auto result=decoder.advance(proof.current.input,stereo(proof.current.input),1200000000,true,false,&proof);
        assert(result.status!=c::Status::invalid);
        if(result.status!=c::Status::ambiguous)assert(result.history_verified);
        ++checked;
    }
    std::printf("%u real-producer analog histories preserved both saved .5 hysteresis states and exact .35/.65 thresholds; passed.\n",checked);
}
