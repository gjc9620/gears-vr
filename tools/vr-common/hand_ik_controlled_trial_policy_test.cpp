#include "hand_ik_controlled_trial_policy.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace gears_hand_ik_controlled;
static TrendSummary natural_trend(){
    ReadOnlyTrend trend;
    for(unsigned i=0;i<7;++i)
        assert(!trend.observe(1000+i*40,10.f+float(i)*.04f,10.f+float(i)*.04f,true,int(i),true));
    assert(trend.observe(1280,10.28f,10.28f,true,7,true));
    return trend.summary();
}
int main(){
    constexpr std::array<int,6> wanted{{0,1,-1,-1,1,0}};
    assert(x_offsets==wanted);
    for(unsigned i=0;i<window_count;++i){
        Triple result{};assert(target(i,{{10,20,30}},result));
        assert(gears_hand_ik::exact(result,{{10.f+float(wanted[i]),20,30}}));
    }
    Triple result{};
    assert(!target(window_count,{{10,20,30}},result));
    assert(!target(0,{{std::numeric_limits<float>::quiet_NaN(),20,30}},result));
    assert(natural_trend().complete());
    assert(natural_trend().distinct_mesh_ticks==8);
    ReadOnlyTrend trend;
    for(unsigned i=0;i<20;++i)assert(!trend.observe(1000,10,10,true,int(i),true));
    assert(trend.summary().samples==1); // Repeated callbacks cannot satisfy eight samples.
    trend.reset();
    for(unsigned i=0;i<8;++i)assert(!trend.observe(1000+i*10,10,10,true,int(i),true));
    assert(!trend.summary().complete()); // Neither 250 ms nor natural rendering occurred.
    trend.reset();
    for(unsigned i=0;i<8;++i)assert(!trend.observe(1000+i*40,10,10,true,1,true));
    assert(!trend.summary().complete()); // Static bone getters cannot force tick/render evidence.
    trend.reset();
    for(unsigned i=0;i<24;++i)
        assert(!trend.observe(1000+i*40,10.f+float(i)*.04f,10.f+float(i)*.04f,true,int(i%2),true));
    assert(trend.summary().samples==24&&trend.summary().distinct_mesh_ticks==2);
    assert(trend.summary().mesh_tick_progress&&!trend.summary().complete());
    trend.reset();
    for(unsigned i=0;i<7;++i)
        assert(!trend.observe(1000+i*40,10.f+float(i)*.04f,10.f+float(i)*.04f,true,int(i),true));
    for(unsigned i=7;i<20;++i)
        assert(!trend.observe(1000+i*40,10.f+float(i)*.04f,10.f+float(i)*.04f,true,6,true));
    assert(trend.summary().distinct_mesh_ticks==7&&!trend.summary().complete());
    assert(trend.observe(1800,10.8f,10.8f,true,7,true));
    assert(trend.summary().distinct_mesh_ticks==8);
    trend.reset();
    for(unsigned i=0;i<8;++i)
        assert(!trend.observe(1000+i*40,10.f+float(i)*.04f,10.f+float(i)*.04f,i!=2,int(i),true));
    assert(!trend.summary().mesh_tick_known);
    assert(trend.summary().distinct_mesh_ticks==7); // An unknown Tick is never a distinct observed Tick.
    trend.reset();
    for(unsigned i=0;i<8;++i)
        assert(!trend.observe(1000+i*40,10.f+float(i)*.04f,10.f+float(i)*.04f,true,int(i),i!=2));
    assert(!trend.summary().valid); // A missing/unqualified baseline remains a gap.
    trend.reset();
    assert(!trend.observe(1000,10,10,true,0,true));
    assert(!trend.observe(999,10,10,true,1,true)&&!trend.summary().valid);
    trend.reset();
    assert(!trend.observe(1000,10,9,true,0,true)&&!trend.summary().valid);
    WindowEvidence evidence;
    evidence.before=evidence.after=natural_trend();
    evidence.strength_verified=evidence.restore_verified=evidence.pulse_in_time=true;
    evidence.mesh_tick_progress=evidence.render_progress=evidence.same_mesh_world=true;
    assert(evidence.complete());
    SequenceEvidence sequence;
    assert(!sequence.append(1,evidence)); // No missing, duplicate or reordered windows.
    for(unsigned i=0;i<window_count;++i){
        assert(sequence.append(i,evidence));
        assert(sequence.complete()==(i==window_count-1));
        assert(!effect_proved()); // Even a complete controlled recording remains inconclusive.
    }
    assert(!sequence.append(window_count,evidence)&&!sequence.append(0,evidence));
    for(unsigned field=0;field<8;++field){
        auto missing=evidence;
        if(field==0)missing.strength_verified=false;
        if(field==1)missing.restore_verified=false;
        if(field==2)missing.pulse_in_time=false;
        if(field==3)missing.mesh_tick_progress=false;
        if(field==4)missing.render_progress=false;
        if(field==5)missing.same_mesh_world=false;
        if(field==6)missing.before.valid=false;
        if(field==7)missing.after.valid=false;
        SequenceEvidence incomplete;
        for(unsigned i=0;i<window_count;++i)assert(incomplete.append(i,i==2?missing:evidence));
        assert(!incomplete.complete()&&!effect_proved());
    }
    assert(gears_hand_ik::pulse_in_time(1000,1400));
    assert(!gears_hand_ik::pulse_in_time(1000,1401));
    std::puts("Controlled IK sequence, read-only recovery and evidence gaps passed; effect remains inconclusive");
}
