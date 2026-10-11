#include "pose_gate.h"
#include <cstdio>
#include <limits>
using namespace gears_vr;
int main() {
    Viewpoint base{{12,34,56},{0,0,0}};
    PoseSample sample{{{0,0,0},{0,0,0,1}},1,1000000000,15,true};
    PoseGate gate;
    if(gate.update(base,sample,1000000000,true).status!=PoseStatus::unarmed)return 1;
    gate.arm(true);
    if(gate.update(base,sample,1000000000,true).status!=PoseStatus::recentered)return 2;
    sample.sequence=2; sample.received_ns+=10000000; sample.head.position.x=.1;
    auto moved=gate.update(base,sample,sample.received_ns,true);
    if(moved.status!=PoseStatus::applied || std::abs(moved.view.position.y-44)>1e-9)return 3;
    // Same newest sample can drive several game frames without forcing recenter.
    if(gate.update(base,sample,sample.received_ns+1000000,true).view.position.y!=44)return 4;
    auto stale=gate.update(base,sample,sample.received_ns+100000001,true);
    if(stale.status!=PoseStatus::stale || stale.view.position.y!=34)return 5;
    sample.sequence++; sample.received_ns+=200000000;
    if(gate.update(base,sample,sample.received_ns,true).status!=PoseStatus::recentered)return 6;
    sample.sequence++; sample.head.position.x=.2;
    if(std::abs(gate.update(base,sample,sample.received_ns,true).view.position.y-44)>1e-9)return 7;
    sample.location_flags=3;
    if(gate.update(base,sample,sample.received_ns,true).status!=PoseStatus::untracked)return 8;
    sample.location_flags=15; sample.sequence++;
    if(gate.update(base,sample,sample.received_ns,true).status!=PoseStatus::recentered)return 9;
    if(gate.update(base,sample,sample.received_ns,false).status!=PoseStatus::inactive)return 10;
    sample.sequence++;
    if(gate.update(base,sample,sample.received_ns,true).status!=PoseStatus::recentered)return 11;
    auto replay=sample; replay.sequence--;
    if(gate.update(base,replay,replay.received_ns,true).status!=PoseStatus::replayed)return 12;
    if(gate.update(base,sample,sample.received_ns,true).status!=PoseStatus::recentered)return 13;
    replay=sample; replay.head.position.x+=1;
    if(gate.update(base,replay,replay.received_ns,true).status!=PoseStatus::replayed)return 14;
    sample.sequence++; sample.head.orientation={0,0,0,0};
    if(gate.update(base,sample,sample.received_ns,true).status!=PoseStatus::invalid)return 15;
    sample.head.orientation={0,0,0,1};sample.sequence++;
    sample.head.position.x=std::numeric_limits<double>::quiet_NaN();
    if(gate.update(base,sample,sample.received_ns,true).status!=PoseStatus::invalid)return 16;
    sample.head.position.x=.2;sample.sequence++;
    if(gate.update(base,sample,sample.received_ns-1,true).status!=PoseStatus::stale)return 17;
    if(gate.update(base,sample,sample.received_ns,true).status!=PoseStatus::recentered)return 18;
    sample.sequence++;sample.head.position.x=.3;gate.recenter();
    if(gate.update(base,sample,sample.received_ns,true).status!=PoseStatus::recentered)return 19;
    gate.arm(false);
    if(gate.update(base,sample,sample.received_ns,true).status!=PoseStatus::unarmed)return 20;
    puts("Pose gate: arming, freshness, replay, focus, tracking loss and recenter PASS");
    return 0;
}
