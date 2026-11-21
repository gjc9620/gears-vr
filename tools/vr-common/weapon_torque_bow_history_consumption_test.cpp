#include "weapon_torque_bow_controls.h"
#include "weapon_torque_bow_interaction.h"
#include <cassert>
#include <cstdio>
using namespace gears_weapon;
using E=TorqueBowEvent;using A=TorqueBowAction;using S=TorqueBowStage;using O=TorqueBowOutcome;
namespace {
// Synthetic adapter receipts. No fixture is retail ABI or physical evidence.
constexpr TorqueBowIdentity id{1,2,3,4,5,6,7,8};
constexpr TorqueBowOriginalState idle{true,false,false,false,false};
constexpr TorqueBowOriginalState charged{false,true,false,true,false};
constexpr TorqueBowOriginalState deferred{false,true,true,false,false};
TorqueBowConfig config(){
    TorqueBowConfig c{};c.identity=id;c.instance_config_serial=17;c.capacity=1;
    c.game_ticks_per_second=1000;
    c.original_class_verified=c.original_charge_entry_verified=c.original_end_fire_verified=true;
    c.non_firing_abort_verified=c.manual_reload_phase_verified=c.reload_plan_verified=true;
    c.reload_step_count=1;c.reload_steps[0]={TorqueBowReloadAction::insert_arrow,201};return c;
}
struct F {
    TorqueBowInteraction m;std::uint64_t seq{},events{};std::int64_t ns{1000000},tick{};
    explicit F(unsigned loaded=0,unsigned reserve=1){assert(m.bind(config(),{1,loaded,reserve}));}
    TorqueBowStamp next(std::int64_t at=-1){++seq;ns+=1000000;if(at>=0)tick=at;return {id,seq,ns,tick};}
    E sample(bool down=false,TorqueBowOriginalState state=idle,bool right=true,bool left=true){
        const auto stamp=next();return m.observe({stamp,m.inventory(),state,right,left,true,down,stamp.monotonic_ns});
    }
    E begin(){const auto s=next(tick<1000?1000:tick);return m.begin_left_shoulder({s,1,1,true,true,true,true,s.game_tick},
        {11,TorqueBowAmmoSource::left_shoulder,true,true,1,true,true});}
    E seat(std::int64_t at){auto start=seq;return m.complete_step(next(at),{17,1,201,start,11,true,true,true,true,true});}
    E edge(std::uint64_t n,bool down){return m.trigger(next(),{n,true,true,down});}
    E cancel(std::uint64_t n=1){return m.cancel_charge(next(),{n,true,true,true});}
    TorqueBowRequest take(A a){TorqueBowRequest r{};assert(m.take_request(r,ns));assert(r.action==a);
        TorqueBowRequest twice{};assert(!m.take_request(twice,ns));return r;}
    E ack(TorqueBowRequest const& r,TorqueBowOriginalState state=idle){
        auto i=m.inventory();if(r.action==A::complete_original_reload){i=r.after;++i.revision;}
        return m.acknowledge({next(),r.token,++events,true,i,state,r.outcome});
    }
    void charge(){assert(sample()==E::observed);assert(edge(1,true)==E::request_ready);
        auto r=take(A::begin_original_charge);assert(ack(r,charged)==E::charge_started);}
};
}
namespace hc=gears_hand_continuity;namespace vr=gears_vr;namespace b=bow_history_evidence;
struct Fixture {
    hc::MotionHistory history{};hc::Publisher publisher{17,29};std::int64_t now{1200000000};
    Fixture(){
        history.count=8;
        for(unsigned i=0;i<8;++i){auto& s=history.frames[i];auto& f=s.input;
            s.stream_epoch=3;s.tracking_epoch[0]=5;s.tracking_epoch[1]=7;
            f.session=f.space_epoch=f.profile_generation=f.input_epoch=1;f.sequence=100+i;f.focused=1;
            f.sync_ns=f.received_ns=f.predicted_time=1040000000+i*20000000;
            for(auto& hand:f.hands){hand.grip_active=1;hand.grip_flags=15;}
            f.hands[1].trigger_active=1;f.hands[1].buttons_active=vr::controller_stick_click;
        }
    }
    void press(unsigned first,unsigned control=vr::control_trigger){
        for(unsigned i=first;i<8;++i){auto& h=history.frames[i].input.hands[1];h.pressed[control]=1;
            if(control==vr::control_trigger)h.trigger=1;else h.buttons=vr::controller_stick_click;}
    }
    void release(unsigned first,unsigned control=vr::control_trigger){
        for(unsigned i=first;i<8;++i){auto& h=history.frames[i].input.hands[1];h.released[control]=1;
            if(control==vr::control_trigger)h.trigger=0;else h.buttons=0;}
    }
    b::Receipt run(){return b::inspect(history.frames[0],publisher,history.frames[7],publisher,history,now,history.frames[0].input.hands[1].trigger>.35f);}
};

namespace c=gears_torque_bow_controls;
c::Continuity companion(Fixture const& f,unsigned index){
    c::Continuity out{};out.current=f.history.frames[index];out.publisher=f.publisher;
    out.history=f.history;out.history.count=index+1;return out;
}
c::Result step(c::Decoder& decoder,Fixture const& f,unsigned index,std::int64_t now,bool owned,
               c::Continuity const* proof){
    const auto& input=f.history.frames[index].input;vr::StereoSample stereo{};
    stereo.source=input.source;stereo.session=input.session;stereo.space_epoch=input.space_epoch;
    stereo.sequence=input.sequence;stereo.predicted_time=input.predicted_time;stereo.received_ns=input.received_ns;stereo.focused=1;
    return decoder.advance(input,stereo,now,owned,false,proof);
}
int main(){
    unsigned cases=0;
    for(unsigned pattern=0;pattern<5;++pattern){Fixture f;c::Decoder decoder;
        if(pattern==0)f.press(4);
        if(pattern==1){f.press(3);f.release(5);}
        if(pattern==2){f.press(3,vr::control_stick);f.release(5,vr::control_stick);f.press(4);f.release(6);}
        if(pattern==3){f.press(1);}
        auto base=companion(f,0);assert(step(decoder,f,0,f.history.frames[0].input.sync_ns,false,&base).status==c::Status::baseline);
        auto current=companion(f,7);auto result=step(decoder,f,7,f.now,pattern==2,&current);
        if(pattern==0){assert(result.history_verified&&result.count==1&&result.actions[0]==c::Action::press);
            assert(result.original_action_ns[0]==1120000000);}
        if(pattern==1){assert(result.history_verified&&result.count==2&&result.actions[0]==c::Action::press&&result.actions[1]==c::Action::release);
            assert(result.original_action_ns[0]==1100000000&&result.original_action_ns[1]==1140000000);}
        if(pattern==2){assert(result.history_verified&&result.status==c::Status::cancel&&result.count==1);
            assert(result.original_action_ns[0]==1100000000);}
        if(pattern==3)assert(result.rejection==c::Rejection::observation_gap&&!result.count);
        if(pattern==4)assert(result.history_verified&&result.status==c::Status::observed&&!result.count);
        auto replay=step(decoder,f,7,f.now,false,&current);
        if(pattern==3)assert(replay.status==c::Status::baseline&&!replay.count);
        else assert(replay.status==c::Status::invalid&&!replay.count&&!replay.release_observed);++cases;
    }
    for(unsigned bad=0;bad<9;++bad){Fixture f;f.press(4);c::Decoder decoder;auto base=companion(f,0);
        assert(step(decoder,f,0,f.history.frames[0].input.sync_ns,false,bad==0?nullptr:&base).status==c::Status::baseline);
        auto current=companion(f,7);
        if(bad==1)++current.publisher.start;
        if(bad==2)++current.publisher.pid;
        if(bad==3)++current.history.frames[3].input.sequence;
        if(bad==4)++current.history.frames[3].tracking_epoch[1];
        if(bad==5)++current.history.frames[0].input.received_ns;
        if(bad==6)current.history.frames[3].input.hands[1].trigger_active=0;
        if(bad==7)decoder.lost();
        if(bad==8)++current.current.input.received_ns;
        auto result=step(decoder,f,7,f.now,false,&current);
        if(bad==7)assert(result.status==c::Status::baseline&&!result.count);
        else assert(result.status==c::Status::invalid&&!result.count&&!result.history_verified);
        ++cases;
    }
    // Current input shape/stereo/freshness stay mandatory despite good history.
    {Fixture f;f.press(4);c::Decoder decoder;auto base=companion(f,0);step(decoder,f,0,1040000000,false,&base);
        auto proof=companion(f,7);auto r=step(decoder,f,7,1280000001,false,&proof);
        assert(r.status==c::Status::invalid&&r.rejection==c::Rejection::input_shape&&!r.count);++cases;}
    // A history path cannot relax the original aggregate multi-edge ambiguity.
    {Fixture f;f.press(3);f.release(4);for(unsigned i=5;i<8;++i){auto& h=f.history.frames[i].input.hands[1];h.pressed[vr::control_trigger]=2;h.trigger=1;}
        c::Decoder decoder;auto base=companion(f,0);step(decoder,f,0,1040000000,false,&base);
        auto proof=companion(f,7);auto r=step(decoder,f,7,1180000000,false,&proof);
        assert(r.status==c::Status::ambiguous&&!r.count&&!r.release_observed);++cases;}
    for(auto age:{0LL,80000000LL,100000000LL}){F f(1,5);f.ns=1000000000;assert(f.sample()==E::observed);
        auto s=f.next();s.original_control_ns=s.monotonic_ns-age;
        assert(f.m.trigger(s,{1,true,true,true})==E::request_ready);auto request=f.take(A::begin_original_charge);
        assert(request.authored_ns==s.monotonic_ns&&request.valid_until_ns==s.original_control_ns+100000000);++cases;}
    for(auto age:{100000001LL,-1LL}){F f(1,5);f.ns=1000000000;f.sample();auto s=f.next();s.original_control_ns=s.monotonic_ns-age;
        auto event=f.m.trigger(s,{1,true,true,true});assert(event!=E::request_ready);TorqueBowRequest request{};assert(!f.m.take_request(request,f.ns));++cases;}
    // A deferred release retains its own original raw deadline across begin ACK.
    {F f(1,5);f.ns=1000000000;f.sample();assert(f.edge(1,true)==E::request_ready);auto begin=f.take(A::begin_original_charge);
        auto release=f.next();release.original_control_ns=release.monotonic_ns-30000000;
        assert(f.m.trigger(release,{2,true,true,false})==E::release_buffered);
        assert(f.ack(begin,charged)==E::request_ready);auto end=f.take(A::end_original_fire);
        assert(end.valid_until_ns==release.original_control_ns+100000000);++cases;}
    for(unsigned kind=0;kind<4;++kind){F f(1,5);f.ns=1000000000;f.charge();
        const auto previous_pose=1001000000LL;f.ns+=140000000;const auto stamp=f.next();
        auto sync=stamp.monotonic_ns;auto proof=previous_pose;
        if(kind==1)++proof;
        if(kind==2)sync=stamp.monotonic_ns-100000001;
        if(kind==3)sync=stamp.monotonic_ns+1;
        auto event=f.m.observe({stamp,f.m.inventory(),charged,true,true,true,true,sync,proof});
        if(kind==0)assert(event==E::observed&&f.m.stage()==S::charging);
        else assert(event!=E::observed||f.m.stage()!=S::charging);++cases;
    }
    {F f(1,5);f.ns=1000000000;f.sample();f.edge(1,true);auto begin=f.take(A::begin_original_charge);
        auto release=f.next();release.original_control_ns=release.monotonic_ns-80000000;
        assert(f.m.trigger(release,{2,true,true,false})==E::release_buffered);f.ns+=30000000;
        f.ack(begin,charged);TorqueBowRequest expired{};assert(!f.m.take_request(expired,f.ns));++cases;
    }
    std::printf("Bow raw history consumption and original deadlines: %u cases passed; prototype only.\n",cases);
}
