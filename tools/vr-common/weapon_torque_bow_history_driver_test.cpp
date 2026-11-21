#include "weapon_torque_bow_driver_fixture.h"
namespace c=gears_torque_bow_controls;namespace hc=gears_hand_continuity;
struct HistoryBackend:Backend {
    c::Continuity companion{};bool available{true};unsigned history_reads{};
    void append(h::Frame const& f){
        hc::TrackedInput tracked{};tracked.input=f.controllers;tracked.stream_epoch=3;
        tracked.tracking_epoch[0]=5;tracked.tracking_epoch[1]=7;
        auto& ring=companion.history;
        if(ring.count==hc::motion_history_capacity){for(unsigned i=1;i<ring.count;++i)ring.frames[i-1]=ring.frames[i];--ring.count;}
        ring.frames[ring.count++]=tracked;companion.current=tracked;companion.publisher={17,29};
    }
    bool read_continuity(h::Frame const& f,c::Continuity& out){
        ++history_reads;out={};if(!available||!hc::same_input(f.controllers,companion.current.input))return false;
        out=companion;return true;
    }
};
using HF=DriverFixture<HistoryBackend>;
void setup(HF& f){
    f.frame.controllers.predicted_time=f.frame.stereo.predicted_time=f.backend.time;
    f.backend.append(f.frame);assert(f.run()==d::Status::baseline);
}
void raw(HF& f,unsigned index,bool release=false,bool cancel=false){
    f.next(20000000);f.frame.controllers.predicted_time=f.frame.stereo.predicted_time=f.backend.time;
    if(index==4)f.trigger(!release);
    if(cancel&&index==4){++f.frame.controllers.hands[1].pressed[v::control_stick];f.frame.controllers.hands[1].buttons=v::controller_stick_click;}
    if(cancel&&index==5){++f.frame.controllers.hands[1].released[v::control_stick];f.frame.controllers.hands[1].buttons=0;}
    f.backend.append(f.frame);
}
int main(){
    {HF f;setup(f);for(unsigned i=1;i<=7;++i)raw(f,i);f.run();
        assert(f.backend.presses==1&&f.backend.owner&&f.driver.stage()==w::TorqueBowStage::charging);
        assert(f.driver.diagnostic().history_verified_decodes==1);
        for(unsigned i=1;i<=7;++i)raw(f,i,true);f.run();
        assert(f.backend.releases==1&&f.backend.recorded==1&&f.backend.actual.loaded==0&&!f.backend.owner);
        const auto shots=f.backend.recorded;f.run();assert(f.backend.recorded==shots);}
    for(unsigned bad=0;bad<4;++bad){HF f;setup(f);for(unsigned i=1;i<=7;++i)raw(f,i);
        if(bad==0)f.backend.available=false;
        if(bad==1)++f.backend.companion.publisher.start;
        if(bad==2)++f.backend.companion.history.frames[3].input.sequence;
        if(bad==3)++f.backend.companion.history.frames[3].tracking_epoch[1];
        f.run();assert(!f.backend.presses&&!f.backend.releases&&!f.backend.recorded);}
    {HF f;setup(f);for(unsigned i=1;i<=7;++i)raw(f,i);f.run();assert(f.backend.owner);
        for(unsigned i=1;i<=7;++i)raw(f,i,true,true);f.run();
        assert(f.backend.cancels==1&&!f.backend.releases&&!f.backend.recorded&&f.backend.actual.loaded==1&&!f.backend.owner);}
    {HF f;setup(f);for(unsigned i=1;i<=7;++i)raw(f,i);f.run();assert(f.backend.owner);
        for(unsigned i=1;i<=7;++i)raw(f,i==1?4:0,true);f.run();
        assert(f.backend.cancels==1&&!f.backend.releases&&!f.backend.recorded&&f.backend.actual.loaded==1&&!f.backend.owner);}
    {HF f;setup(f);for(unsigned i=1;i<=7;++i)raw(f,i);f.run();f.backend.deferred=true;
        for(unsigned i=1;i<=7;++i)raw(f,i,true);f.run();
        assert(f.backend.releases==1&&f.backend.owner&&!f.backend.recorded&&f.backend.actual.loaded==1);
        f.backend.arrow(true);raw(f,0,true);f.run();
        assert(f.backend.recorded==1&&f.backend.actual.loaded==0&&!f.backend.owner);}
    std::puts("Bow driver history: original press/release+one deduction, replay, missing proof, publisher/tracking loss and R3 priority passed; synthetic backend only.");
}
