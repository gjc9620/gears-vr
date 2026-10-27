#include "stereo_frame.h"
#include <cassert>
#include <cstdio>
#include <limits>

using namespace gears_vr;
static StereoSample sample(std::uint64_t sequence=1) {
    StereoSample s{};
    s.session=7;s.sequence=sequence;s.received_ns=1000000000+sequence*1000000;
    s.predicted_time=1000+sequence;s.focused=s.should_render=1;
    s.head_flags=s.view_flags=15;s.head.orientation.w=1;
    for(unsigned eye=0;eye<2;++eye){
        s.eyes[eye].pose.orientation.w=1;
        s.eyes[eye].pose.position.x=eye?.032:-.032;
        s.eyes[eye].fov={-.7,.7,.7,-.7};
    }
    return s;
}
static void stable(ReferenceContinuity const& c) {
    assert(c.epoch&&c.stable&&!c.pending&&!c.exhausted);
}
int main() {
    {
        ReferenceContinuityCounter c;
        assert(!c.snapshot(true,false).stable);
        c.accepted();stable(c.snapshot(true,false));
        auto e=c.snapshot(true,false).epoch;
        c.break_observed();assert(c.snapshot(true,false).epoch==e+1);
        c.break_observed();assert(c.snapshot(true,false).epoch==e+1);
        assert(!c.snapshot(true,false).stable);
        c.accepted();stable(c.snapshot(true,false));
        assert(!c.snapshot(false,false).stable&&!c.snapshot(true,true).stable);
        c.boundary();c.boundary();assert(c.snapshot(true,false).epoch==e+3);
    }
    {
        ReferenceContinuityCounter c((std::numeric_limits<std::uint64_t>::max)()-1);
        c.accepted();c.boundary();c.accepted();stable(c.snapshot(true,false));
        c.boundary();c.accepted();auto s=c.snapshot(true,false);
        assert(s.exhausted&&!s.stable&&s.epoch==(std::numeric_limits<std::uint64_t>::max)());
        c.boundary();assert(c.snapshot(true,false).epoch==s.epoch);
        ReferenceContinuityCounter zero(0);zero.accepted();assert(zero.snapshot(true,false).exhausted);
    }
    {
        StereoGate gate;auto s=sample();
        assert(!gate.continuity().stable);
        auto r=gate.update(s,s.received_ns);assert(r);stable(gate.continuity());
        const auto e=gate.continuity().epoch;
        // Reusing an identical accepted eye packet is not itself a boundary.
        assert(gate.update(s,s.received_ns));assert(gate.continuity().epoch==e);
        // A pure old-age receipt and no receipt preserve the reference certificate.
        assert(gate.update(s,s.received_ns+100000001).status==StereoStatus::stale);
        stable(gate.continuity());assert(gate.continuity().epoch==e);
        assert(gate.update(sample(2),sample(2).received_ns));assert(gate.continuity().epoch==e);
        // Even reset + a fresh generation1 cannot resurrect the prior epoch.
        gate.reset();assert(!gate.continuity().stable&&gate.continuity().epoch>e);
        r=gate.update(s,s.received_ns);assert(r&&r.reference_generation==1);
        stable(gate.continuity());assert(gate.continuity().epoch>e);
    }
    // Each observed non-age failure latches one break, including failures that
    // leave the old reference pose/generation untouched. Recovery exposes it.
    for(unsigned failure=0;failure<10;++failure){
        StereoGate gate;auto first=sample();auto initial=gate.update(first,first.received_ns);
        assert(initial);const auto e=gate.continuity().epoch;
        auto bad=sample(2);auto now=bad.received_ns;
        switch(failure){
        case 0:bad.focused=bad.visible=bad.should_render=0;break;
        case 1:bad.head_flags=0;break;
        case 2:bad.eyes[0].pose.orientation.w=0;break;
        case 3:bad.session=8;break;
        case 4:bad.sequence=0;break;
        case 5:bad=first;bad.head.position.x=.1;break;
        case 6:bad.source=StereoSource::MetaOperatorSimulated;break;
        case 7:now=bad.received_ns-1;break;
        case 8:bad.received_ns=0;break;
        case 9:now=0;break;
        }
        assert(!gate.update(bad,now));auto broken=gate.continuity();
        assert(!broken.stable&&broken.epoch==e+1);
        assert(!gate.update(bad,now));assert(gate.continuity().epoch==broken.epoch);
        auto recovered=sample(3);auto result=gate.update(recovered,recovered.received_ns);
        assert(result&&result.reference_generation==initial.reference_generation);
        assert(stereo_same_pose(result.reference,initial.reference));
        stable(gate.continuity());assert(gate.continuity().epoch==broken.epoch);
    }
    for(unsigned mixed=0;mixed<2;++mixed){
        StereoGate gate;auto first=sample();assert(gate.update(first,first.received_ns));
        const auto e=gate.continuity().epoch;auto old=sample(2);
        if(mixed==0)old.eyes[0].pose.orientation.w=0;else ++old.space_epoch;
        assert(gate.update(old,old.received_ns+100000001).status==StereoStatus::stale);
        assert(!gate.continuity().stable&&gate.continuity().epoch==e+1);
        auto next=sample(3);assert(gate.update(next,next.received_ns));
        stable(gate.continuity());assert(gate.continuity().epoch==e+1);
    }
    {
        StereoGate gate;auto s=sample();assert(gate.update(s,s.received_ns));
        auto e=gate.continuity().epoch;
        gate.request_recenter();auto pending=gate.continuity();
        assert(pending.pending&&!pending.stable&&pending.epoch>e);
        auto vertical=sample(2);vertical.head.orientation={std::sqrt(.5),0,0,std::sqrt(.5)};
        for(auto& eye:vertical.eyes)eye.pose.orientation=vertical.head.orientation;
        auto deferred=gate.update(vertical,vertical.received_ns);
        assert(deferred&&deferred.heading_deferred&&deferred.reference_generation==1);
        assert(gate.continuity().pending&&!gate.continuity().stable);
        auto next=sample(3);auto result=gate.update(next,next.received_ns);
        assert(result&&result.reference_generation==2);stable(gate.continuity());
        assert(gate.continuity().epoch>pending.epoch);
        e=gate.continuity().epoch;next=sample(4);++next.space_epoch;
        result=gate.update(next,next.received_ns);assert(result&&result.reference_generation==3);
        stable(gate.continuity());assert(gate.continuity().epoch>e);
    }
    {
        StereoGate gate;auto vertical=sample();
        vertical.head.orientation={std::sqrt(.5),0,0,std::sqrt(.5)};
        for(auto& eye:vertical.eyes)eye.pose.orientation=vertical.head.orientation;
        assert(gate.update(vertical,vertical.received_ns).status==StereoStatus::heading_unavailable);
        assert(!gate.continuity().stable);
        auto next=sample(2);assert(gate.update(next,next.received_ns));stable(gate.continuity());
    }
    std::puts("reference continuity: age-only suspension, hidden failures, resets, pending heading and exhaustion passed");
}
