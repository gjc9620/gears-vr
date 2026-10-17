#include "final_transfer_bracket.h"
#include <cassert>
#include <iostream>
#include <memory>
int main(){
    using namespace gears_final_transfer_bracket;
    Snapshot a{};a.input.source={123,3,2560,1080,1,1,24,1};a.list={456,7};a.ticket=9;a.pair_linked=true;
    assert(compare(a,a)==0);
    for(unsigned bit=0;bit<8;++bit){auto b=a;
        switch(bit){
        case 0:++b.list.generation;break;
        case 1:++b.input.source.mips;break;
        case 2:++b.input.binding.target.resource;break;
        case 3:b.input.rectangles.depth_known=true;break;
        case 4:++b.world;break;
        case 5:++b.inventory;break;
        case 6:++b.ticket;break;
        case 7:++b.input.dimensions[0];break;
        }
        assert(compare(a,b)==(1u<<bit));
    }
    Budget budget;
    for(unsigned i=0;i<4;++i){assert(budget.preflight());assert(budget.retain(false)==int(i));}
    assert(budget.preflight());assert(budget.retain(false)==-1);
    for(unsigned i=0;i<12;++i){assert(budget.preflight());assert(budget.retain(true)==int(i+4));}
    assert(!budget.preflight()&&budget.attempted==17&&budget.skipped==1);
    Budget missing;
    for(unsigned i=0;i<64;++i){assert(missing.preflight());missing.retain(false);}
    assert(!missing.preflight()&&missing.attempted==64&&missing.rejected==4&&missing.complete==0);
    Draw expected{123,456,1,6,1};DrawCalls calls;
    calls.call(expected,false);assert(calls.all_draws==1&&calls.matched_draws==0&&!calls.exactly_one_routed_draw());
    calls.match(expected);assert(calls.exactly_one_routed_draw());
    calls.call(expected,false);assert(!calls.exactly_one_routed_draw()&&calls.first==expected);
    DrawCalls bypass;bypass.call(expected,true);bypass.call(expected,false);bypass.match(expected);
    assert(bypass.bypass_draws==1&&bypass.all_draws==1&&!bypass.exactly_one_routed_draw());
    DrawCalls wrong;wrong.call(expected,false);auto other=expected;++other.raw_list;wrong.match(other);
    assert(!wrong.exactly_one_routed_draw());
    DrawCalls multiple;multiple.call(expected,false);multiple.match(expected);multiple.match(expected);
    assert(!multiple.exactly_one_routed_draw());
    Pending pending{};pending.readable=pending.stable=pending.known_buffer=true;
    pending.count=pending.viewport_count=pending.scissor_count=1;pending.dirty={1,1,1};
    pending.view=0x1230;pending.handle=0x4560;pending.target={789,2560,1080,24,0};
    pending.trace={1,1,1,1,0,789,0};pending.viewport={0,0,2560,1080,0,1};pending.scissor={0,0,16384,16384};
    Snapshot actual{};actual.input.dimensions={2560,1080,2560,1080};actual.input.binding.targets=1;
    actual.input.binding.target=pending.target;actual.provenance={pending.handle,pending.trace};
    actual.input.rectangles={true,pending_viewport(pending),pending_scissor(pending),1,1,true,0,1};
    assert(pending_reason(pending,actual)==0);
    ConsumedBinds consumed;for(unsigned i=0;i<3;++i)consumed.consume(i,true,true,false);
    auto committed=pending;committed.dirty={0,0,0};
    assert(compare_pending(pending,committed,actual,consumed)==0); // Normal dirty clearing is permitted.
    for(unsigned fault=0;fault<13;++fault){auto p=pending;auto draw=committed;auto s=actual;auto binds=consumed;
        unsigned expected_reason{};
        switch(fault){
        case 0:p.viewport[2]=std::numeric_limits<float>::quiet_NaN();expected_reason=pending_rectangles;break;
        case 1:++p.target.width;expected_reason=pending_target;break;
        case 2:p.viewport[2]/=2;expected_reason=pending_rectangles;break;
        case 3:++draw.trace.write_serial;expected_reason=pending_changed;break;
        case 4:binds={};expected_reason=pending_dirty_bind;break;
        case 5:p.dirty={0,0,0};++s.input.binding.target.resource;expected_reason=pending_not_dirty;break;
        case 6:p.count=2;expected_reason=pending_invalid;break;
        case 7:p.stable=false;expected_reason=pending_invalid;break;
        case 8:p.trace.origin_create_serial=0;expected_reason=pending_provenance;break;
        case 9:binds.consume(0,false,true,false);expected_reason=pending_dirty_bind;break;
        case 10:p.scissor[2]=1280;expected_reason=pending_rectangles;break;
        case 11:p.viewport[5]=std::numeric_limits<float>::infinity();expected_reason=pending_rectangles;break;
        case 12:++s.provenance.trace.write_serial;expected_reason=pending_actual;break;
        }
        const unsigned reason=pending_reason(p,s)|compare_pending(p,draw,s,binds);
        assert(reason&expected_reason);
    }
    auto not_dirty=pending;not_dirty.dirty={0,0,0};ConsumedBinds none;
    assert(pending_reason(not_dirty,actual)==0&&compare_pending(not_dirty,not_dirty,actual,none)==0);
    assert(equal_pending(pending,pending));auto unstable=pending;unstable.scissor[2]--;
    assert(!equal_pending(pending,unstable));unstable=pending;unstable.dirty[1]=0;assert(!equal_pending(pending,unstable));
    // Atomic descriptor observation never changes the consumed OM snapshot.
    auto tracker=std::make_unique<gears_viewport::Tracker>();
    tracker->create(0x100,pending.target,4,789);auto created=tracker->descriptor_snapshot(0x100);
    assert(created.target==pending.target&&created.provenance.handle==0x100&&created.provenance.trace.write_serial);
    uintptr_t destination=0x200,origin=0x100;tracker->copy(&destination,&origin,1,5);
    auto copied=tracker->descriptor_snapshot(0x200);assert(copied.target==created.target);
    assert(copied.provenance.trace.origin_create_serial==created.provenance.trace.origin_create_serial&&
        copied.provenance.trace.copy_source_handle==0x100&&copied.provenance.trace.source_write_serial==created.provenance.trace.write_serial);
    assert(tracker->bind(0x300,1,0x200));auto bound=tracker->binding_provenance(0x300);
    auto replacement=pending.target;++replacement.resource;tracker->create(0x200,replacement,6,790);
    auto overwritten=tracker->descriptor_snapshot(0x200);
    assert(overwritten.target==replacement&&!(overwritten.provenance.trace==copied.provenance.trace));
    assert(tracker->binding(0x300).target==pending.target&&tracker->binding_provenance(0x300)==bound);
    auto stats=tracker->snapshot();assert(!tracker->descriptor_snapshot(0x999).target.resource);
    assert(tracker->snapshot().descriptor_creates==stats.descriptor_creates&&tracker->snapshot().descriptor_evictions==stats.descriptor_evictions);
    std::cout<<"final_transfer_bracket_test passed\n";
}
