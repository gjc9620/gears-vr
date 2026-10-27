#include "controller_recenter_request.h"
#include <cassert>
#include <cstdio>

static gears_vr::StereoSample tracked(std::uint64_t sequence,double x=0){
    gears_vr::StereoSample sample{};
    sample.session=7;sample.sequence=sequence;sample.space_epoch=2;
    sample.received_ns=1000000000+static_cast<std::int64_t>(sequence)*1000000;
    sample.predicted_time=1000+sequence;
    sample.focused=sample.should_render=1;
    sample.head_flags=sample.view_flags=15;
    sample.head.position.x=x;sample.head.orientation.w=1;
    for(unsigned eye=0;eye<2;++eye){
        sample.eyes[eye].pose.position.x=x+(eye?.032:-.032);
        sample.eyes[eye].pose.orientation.w=1;
        sample.eyes[eye].fov={-.7,.7,.7,-.7};
    }
    return sample;
}

int main(){
    gears_weapon::ControllerRecenterRequest request;
    assert(!request.submit(0,7,2));
    assert(!request.consume(1,7,2));
    assert(request.submit(1,7,2));
    assert(!request.consume(2,7,2)); // level changed
    assert(!request.consume(1,7,2)); // stale request was discarded
    assert(request.submit(2,7,2));
    assert(!request.consume(2,8,2)); // XR session changed
    assert(request.submit(2,8,2));
    assert(!request.consume(2,8,3)); // LOCAL origin changed
    assert(request.submit(2,8,3));
    assert(request.submit(2,8,3));
    assert(request.consume(2,8,3));
    assert(!request.consume(2,8,3)); // one press per accepted ticket
    assert(request.submit(3,9,4));
    request.reset();
    assert(!request.consume(3,9,4));

    gears_vr::StereoGate gate;
    auto first=tracked(1);
    auto result=gate.update(first,first.received_ns);
    assert(result&&result.status==gears_vr::StereoStatus::initial);
    auto next=tracked(2,.1);
    assert(request.submit(1,7,2));
    auto stale=gate.update(next,next.received_ns+100000001);
    assert(!gears_weapon::apply_controller_recenter(request,1,1,next,next.received_ns+100000001,
        false,gate,stale));
    result=gate.update(next,next.received_ns);
    assert(result&&result.reference_generation==1);
    assert(!gears_weapon::apply_controller_recenter(request,1,2,next,next.received_ns,
        false,gate,result)); // world changed after the game tick
    assert(gears_weapon::apply_controller_recenter(request,1,1,next,next.received_ns,
        false,gate,result));
    assert(result&&result.recentered&&result.reference_generation==2);
    assert(result.reference.position.x==.1);
    assert(!gears_weapon::apply_controller_recenter(request,1,1,next,next.received_ns,
        false,gate,result)); // one-shot even when the ticket is replayed
    auto changed=next;changed.head.position.x=.2;
    assert(request.submit(1,7,2));
    assert(!gears_weapon::apply_controller_recenter(request,1,1,changed,changed.received_ns,
        false,gate,result)); // cannot apply to an unfrozen sample
    assert(request.consume(1,7,2));
    std::puts("controller L3 recenter identity, accepted-ticket and one-shot checks passed");
}
