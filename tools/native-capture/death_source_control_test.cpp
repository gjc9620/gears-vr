#include "death_source_control.h"
#include <cassert>
#include <climits>
#include <cstdio>
#include <limits>

int main(){
    using namespace gears_death_source;
    const Args args{{1920,0,1920,1080,1920,0,1920,1080},{3840,1080,3840,1080}};
    Gate good{};good.enabled=good.attested=good.recording=good.diagnostics=good.frozen_enabled=good.unpossessed=good.renderer=true;
    good.ticket=good.frozen_ticket=7;good.caller=0xd80a7b;good.views=0x10000;good.current=0x10d30;good.count=good.capacity=2;
    good.native_read=good.stable=good.source_valid=true;good.source_canonical=99;good.source_width=3840;good.source_height=1080;
    good.source_format=26;good.source_dimension=3;good.source_array=good.source_mips=good.source_samples=1;
    auto unchanged=[&](Decision const& d,Args const& original){
        assert(!d.modified&&d.forwarded.rectangle==original.rectangle&&d.forwarded.dimensions==original.dimensions);
    };
    long long origin{};Gate off{};unchanged(decide(args,off,10000,1000,origin),args);assert(!origin);
    const long long ticks[]={10000,10999,11000,12499,12500,1000000};
    const unsigned phases[]={a1,a1,b,b,a2,a2};
    for(unsigned i=0;i<6;++i){auto d=decide(args,good,ticks[i],1000,origin);
        assert(d.rejection==accepted&&d.phase==phases[i]&&origin==10000);
        auto expected=args;if(phases[i]==b)expected.rectangle[4]=0;
        assert(d.modified==(phases[i]==b)&&d.forwarded.rectangle==expected.rectangle&&d.forwarded.dimensions==args.dimensions);
    }
    // Every independent guard keeps all twelve arguments and the clock intact.
    for(unsigned i=0;i<31;++i){auto g=good;
        switch(i){case 0:g.enabled=false;break;case 1:g.attested=false;break;case 2:g.recording=false;break;
        case 3:g.diagnostics=false;break;case 4:g.frozen_enabled=false;break;case 5:g.unpossessed=false;break;
        case 6:g.renderer=false;break;case 7:g.other_control=true;break;case 8:g.ticket=0;break;case 9:++g.frozen_ticket;break;
        case 10:++g.caller;break;case 11:g.native_read=false;break;case 12:g.views=0;break;case 13:++g.views;break;
        case 14:g.current=g.views;break;case 15:++g.current;break;case 16:g.current=g.views-8;break;
        case 17:g.current=g.views+2*0xd30;break;case 18:g.count=1;break;case 19:g.capacity=1;break;case 20:g.capacity=1025;break;
        case 21:g.stable=false;break;case 22:g.source_valid=false;break;case 23:g.source_canonical=0;break;
        case 24:g.source_dimension=2;break;case 25:g.source_array=2;break;case 26:g.source_mips=0;break;
        case 27:g.source_samples=2;break;case 28:++g.source_format;break;case 29:++g.source_width;break;case 30:++g.source_height;break;}
        for(unsigned started=0;started<2;++started){long long clock_origin=started?10000:0;
            auto d=decide(args,g,11500,1000,clock_origin);assert(d.rejection!=accepted);unchanged(d,args);
            assert(clock_origin==(started?10000:0));}
    }
    for(unsigned i=0;i<12;++i){auto changed=args;if(i<8)changed.rectangle[i]+=.25f;else ++changed.dimensions[i-8];
        auto d=decide(changed,good,11500,1000,origin);assert(d.rejection==shape);unchanged(d,changed);}
    for(const auto value:{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
        auto changed=args;changed.rectangle[4]=value;auto d=decide(changed,good,11500,1000,origin);assert(d.rejection==shape&&!d.modified);}
    auto d=decide(args,good,9999,1000,origin);assert(d.rejection==clock);unchanged(d,args);assert(origin==10000);
    d=decide(args,good,11500,0,origin);assert(d.rejection==clock);unchanged(d,args);
    long long fresh{};d=decide(args,good,0,1000,fresh);assert(d.rejection==clock&&!fresh);
    assert(phase_at(0,10000,1000)==waiting&&phase_at(10000,9999,1000)==invalid);
    assert(phase_at(1,8,3)==b&&phase_at(1,9,3)==a2); // Fractional threshold, odd frequency.
    assert(phase_at(1,LLONG_MAX,LLONG_MAX)==a1&&phase_at(1,LLONG_MAX,1)==a2);
    auto smaller=args;smaller.rectangle={960,0,960,720,960,0,960,720};smaller.dimensions={1920,720,1920,720};
    auto smaller_gate=good;smaller_gate.source_width=1920;smaller_gate.source_height=720;
    d=decide(smaller,smaller_gate,11500,1000,origin);auto expected_smaller=smaller;expected_smaller.rectangle[4]=0;
    assert(d.rejection==accepted&&d.modified&&d.forwarded.rectangle==expected_smaller.rectangle&&d.forwarded.dimensions==smaller.dimensions);
    for(const unsigned width:{0u,3u,8194u}){auto bad=args;bad.dimensions[0]=bad.dimensions[2]=width;
        d=decide(bad,good,11500,1000,origin);assert(d.rejection==shape);unchanged(d,bad);}
    for(const unsigned height:{0u,8193u}){auto bad=args;bad.dimensions[1]=bad.dimensions[3]=height;
        d=decide(bad,good,11500,1000,origin);assert(d.rejection==shape);unchanged(d,bad);}
    Recorder records;
    for(unsigned i=0;i<514;++i)records.completed({i,2,10000+i,i%3,1920,i%3==b?0.f:1920.f,99});
    assert(records.size==512&&records.dropped==2&&records.last.ticket==513&&records.modified==171);
    assert(records.counts[0]+records.counts[1]+records.counts[2]==514);
    std::puts("DeathSourceControlTest passed");
}
