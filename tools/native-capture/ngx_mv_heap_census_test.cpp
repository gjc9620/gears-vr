#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ngx_mv_heap_census.h"
#include <cassert>
#include <cstdio>
using namespace gears_ngx_mv_census;
int main(){
    const Event cpu{true,true,false,true,true,false,32},gpu{true,true,false,true,true,true,4096};
    Census census;census.record(cpu);assert(!census.counts().calls);census.arm();
    census.record(cpu);census.record(gpu);census.record(cpu);
    assert(census.counts().creations==3&&census.counts().nonvisible.creations==2);
    assert(census.counts().nonvisible.descriptors==64&&census.counts().nonvisible.max_descriptors==32);
    assert(census.counts().visible.descriptors==4096&&census.counts().visible.max_descriptors==4096);
    Event failure=cpu;failure.failed=true;census.record(failure);
    Event unreadable=cpu;unreadable.descriptor_readable=false;census.record(unreadable);
    Event output=cpu;output.output_readable=false;census.record(output);
    Event validation=cpu;validation.created=false;census.record(validation);
    Event rtv=cpu;rtv.cbv_srv_uav=false;census.record(rtv);
    assert(census.counts().calls==8&&census.counts().creations==3&&census.counts().failed==1);
    assert(census.counts().descriptor_unreadable==1&&census.counts().output_unreadable==1);
    assert(census.counts().non_creation==1&&census.counts().other_type==1&&!census.counts().overflow);
    const Use use{1,2,3,4,5,6};assert(!census.accepted(1,use)&&!census.accepted(0,{}));
    assert(census.accepted(0,use));census.record(gpu);assert(!census.accepted(0,use));
    assert(census.snapshots()[0].counts.creations==3&&census.counts().creations==4);
    auto wrong=use;++wrong.revision;assert(!census.accepted(1,wrong));
    wrong=use;++wrong.world;assert(!census.accepted(1,wrong));
    wrong=use;++wrong.ticket;assert(!census.accepted(1,wrong));
    wrong=use;++wrong.renderer;assert(!census.accepted(1,wrong));
    wrong=use;++wrong.handle;assert(!census.accepted(1,wrong));
    wrong=use;++wrong.parameters;assert(!census.accepted(1,wrong));
    assert(census.accepted(1,use)&&!census.accepted(2,use));census.record(cpu);
    assert(census.snapshots()[1].counts.creations==4&&census.counts().creations==5);
    census.close();census.record(cpu);census.arm();assert(!census.active()&&census.counts().creations==5);
    assert(!census.accepted(0,use));
    Counts saturated;saturated.calls=UINT64_MAX;saturated.creations=UINT64_MAX;
    saturated.visible.creations=UINT64_MAX;saturated.visible.descriptors=UINT64_MAX-4095;
    accumulate(saturated,gpu);assert(saturated.overflow&&saturated.calls==UINT64_MAX&&saturated.creations==UINT64_MAX);
    assert(saturated.visible.creations==UINT64_MAX&&saturated.visible.descriptors==UINT64_MAX&&saturated.visible.max_descriptors==4096);
    Counts exact;exact.nonvisible.descriptors=UINT64_MAX-32;accumulate(exact,cpu);
    assert(!exact.overflow&&exact.nonvisible.descriptors==UINT64_MAX);accumulate(exact,cpu);assert(exact.overflow);
    Counts failed;failed.failed=UINT64_MAX;accumulate(failed,failure);assert(failed.overflow&&failed.failed==UINT64_MAX);
    Counts unread;unread.descriptor_unreadable=UINT64_MAX;accumulate(unread,unreadable);assert(unread.overflow);
    Counts out;out.output_unreadable=UINT64_MAX;accumulate(out,output);assert(out.overflow);
    // The independent counter has no bank admission/capacity input: the ninth
    // creation and every later creation still contribute after bank capacity 8.
    Census after_capacity;after_capacity.arm();for(unsigned i=0;i<9;++i)after_capacity.record(cpu);
    assert(after_capacity.accepted(0,use)&&after_capacity.snapshots()[0].counts.creations==9);
    for(unsigned i=0;i<8;++i)after_capacity.record(gpu);
    assert(after_capacity.accepted(1,use)&&after_capacity.snapshots()[1].counts.creations==17);
    after_capacity.record(cpu);after_capacity.close();assert(after_capacity.counts().creations==18);
    assert(after_capacity.snapshots()[0].counts.creations==9&&after_capacity.snapshots()[1].counts.creations==17);
    Census closed;closed.arm();closed.close();assert(!closed.accepted(0,use));
    std::puts("MV heap census CPU tests passed; scalar counts only, no provenance/GPU proof");
}
