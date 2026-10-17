#include "gpu_viewport_stats.h"
#include <limits>
#include <cstdio>
#include <memory>
static int strict_lifecycle_tests(){
    using namespace gears_viewport;
    auto storage=std::make_unique<Tracker>(true);auto& t=*storage;
    const Target target{123,1920,1080,28,0};const Rect rectangle{0,0,1920,1080};
    t.create(32,target);
    if(t.set(1,full,false,rectangle)||t.set(1,full,true,rectangle)||t.bind(1,1,32)||t.draw_state(1).tracked)return 40;
    if(t.retire(1)||t.retire(0)||t.reset(0)||t.snapshot().overflow)return 41;
    // Keep an unrelated recording alive throughout 1024 submit/retire cycles.
    if(!t.reset(7)||!t.set(7,full,false,rectangle,1,true,.1f,.9f)||!t.set(7,full,true,rectangle)||!t.bind(7,1,32))return 42;
    for(uintptr_t list=100;list<1124;++list){
        if(!t.reset(list)||!t.set(list,full,false,rectangle)||!t.bind(list,1,32)||!t.retire(list))return 43;
        if(t.draw_state(list).tracked||t.set(list,full,false,rectangle)||t.bind(list,1,32))return 44;
        auto other=t.draw_state(7);
        if(!other.tracked||!(other.viewport==rectangle)||!other.depth_known||other.min_depth!=.1f||other.max_depth!=.9f||!(t.binding(7).target==target))return 45;
    }
    if(t.snapshot().overflow||!(t.lookup(32)==target))return 46;
    if(!t.retire(7)||t.draw_state(7).tracked||t.binding(7).targets||t.set(7,full,true,rectangle)||t.bind(7,1,32))return 47;
    if(!t.reset(7)||!t.set(7,full,false,rectangle)||!t.bind(7,1,32)||!(t.binding(7).target==target))return 48;
    uintptr_t dest=64,src=32;t.copy(&dest,&src,1);
    if(!t.retire(7)||!t.reset(8)||!t.bind(8,1,64)||!(t.binding(8).target==target))return 49;
    if(!t.retire(8))return 50;
    // Capacity concerns simultaneous recordings, not historical list identities.
    for(uintptr_t list=1;list<=Tracker::list_capacity;++list)if(!t.reset(list))return 51;
    const uintptr_t rejected=Tracker::list_capacity+1;
    if(t.reset(rejected)||t.snapshot().overflow!=1||t.draw_state(rejected).tracked)return 52;
    if(t.set(rejected,full,false,rectangle)||t.bind(rejected,1,32)||t.snapshot().overflow!=1)return 53;
    auto capacity=t.snapshot();
    if(capacity.active_lists!=Tracker::list_capacity||capacity.list_highwater!=Tracker::list_capacity||capacity.first_failed_list!=rejected)return 56;
    if(!t.retire(128)||!t.reset(rejected)||!t.set(rejected,full,false,rectangle)||!t.bind(rejected,1,32))return 54;
    if(!t.draw_state(129).tracked||t.draw_state(128).tracked||!(t.lookup(64)==target))return 55;
    if(t.retire(128)||t.snapshot().active_lists!=Tracker::list_capacity)return 57;
    // Retirement/replacement cannot erase an overflow awaiting the caller's
    // fail-closed window boundary. Clearing reports it once, preserving history.
    if(!t.clear_bindings()||t.clear_bindings()||t.snapshot().active_lists||t.binding(rejected).targets)return 58;
    if(t.snapshot().list_highwater!=Tracker::list_capacity||t.snapshot().first_failed_list!=rejected||t.snapshot().overflow!=1)return 59;
    if(!t.reset(rejected)||!t.bind(rejected,1,32)||!(t.binding(rejected).target==target)||t.snapshot().active_lists!=1)return 60;
    return 0;
}
int main(){
    if(int error=strict_lifecycle_tests())return error;
    using namespace gears_viewport;
    if(classify(0,0,1920,1080)!=full||classify(0,0,960,1080)!=left||classify(960,0,960,1080)!=right)return 1;
    if(classify(0,0,640,360)!=other||classify(0,0,0,1080)!=unknown||
       classify(std::numeric_limits<float>::quiet_NaN(),0,960,1080)!=unknown)return 2;
    auto tracker_storage=std::make_unique<Tracker>();auto& tracker=*tracker_storage;
    tracker.draw(42,1);tracker.set(42,right,false);tracker.set(42,left,true);tracker.draw(42,1);
    auto sample=tracker.snapshot();
    // A right viewport with a left scissor is distinct from a matching right draw.
    if(sample.untracked_draws!=1||sample.draws[1][right][left]!=1||sample.draws[1][right][right])return 3;
    tracker.reset(42);tracker.draw(42,2);
    if(tracker.snapshot().draws[2][unknown][unknown]!=1)return 4;
    for(uintptr_t i=100;i<100+Tracker::list_capacity;++i)tracker.reset(i);
    if(tracker.snapshot().overflow!=1)return 5;
    auto rectangle_storage=std::make_unique<Tracker>();auto& rectangles=*rectangle_storage;
    Rect right_rect{960,0,960,1080},wide_scissor{0,0,32767,32767};
    rectangles.set(1,right,false,right_rect);rectangles.set(1,other,true,wide_scissor);
    auto observed=rectangles.draw_state(1);
    if(!observed.tracked||observed.viewport_count!=1||observed.scissor_count!=1||!(observed.viewport==right_rect)||!(observed.scissor==wide_scissor))return 25;
    rectangles.set(2,other,true,{},2);
    if(rectangles.draw_state(2).scissor_count!=2||rectangles.draw_state(999).tracked)return 26;
    rectangles.draw(1,0);rectangles.draw(1,1);rectangles.draw(1,1);
    auto pairs=rectangles.snapshot();
    if(pairs.trial_rects[0].calls!=2||!(pairs.trial_rects[0].viewport==right_rect)||
       !(pairs.trial_rects[0].scissor==wide_scissor))return 6;
    rectangles.reset(1);rectangles.draw(1,1);
    if(rectangles.snapshot().trial_rects[1].calls!=1||rectangles.snapshot().trial_rects[1].viewport.width!=0)return 7;
    for(unsigned i=0;i<64;++i){rectangles.set(1,other,false,{float(i),0,10,10});rectangles.draw(1,1);}
    if(rectangles.snapshot().rect_overflow!=2)return 8;
    auto target_storage=std::make_unique<Tracker>();auto& targets=*target_storage;
    Target small{123,960,1080,28,0},large{456,1920,1080,28,0};
    targets.create(32,small);targets.create(64,large);
    uintptr_t destinations[]{96,128},sources[]{32,64};targets.copy(destinations,sources,2);
    targets.bind(1,1,96);targets.set(1,right,false,right_rect);targets.draw(1,1);
    targets.bind(1,1,128);targets.draw(1,1);targets.draw(1,1);
    auto binding=targets.binding(1);
    if(binding.viewport!=right||binding.targets!=1||!(binding.target==large)||targets.binding(999).target.resource)return 16;
    auto output=targets.snapshot();
    if(output.trial_targets[0].calls!=1||!(output.trial_targets[0].target==small)||
       output.trial_targets[1].calls!=2||!(output.trial_targets[1].target==large))return 9;
    uintptr_t unknown_source=999;targets.copy(destinations,&unknown_source,1);
    if(targets.lookup(96).resource||targets.lookup(128).resource!=large.resource)return 10;
    targets.create(32,large);if(!(targets.lookup(32)==large))return 11;
    // CPU descriptors may be overwritten or copied after OM binding. The
    // recorded resource remains frozen until a fresh bind, including unknowns.
    targets.bind(2,1,32);targets.create(32,small);
    if(!(targets.binding(2).target==large))return 18;
    uintptr_t overwrite_destination=32,overwrite_source=96;
    targets.copy(&overwrite_destination,&overwrite_source,1);
    if(targets.lookup(32).resource||!(targets.binding(2).target==large))return 19;
    targets.bind(2,1,32);targets.create(32,large);
    if(targets.binding(2).target.resource)return 20;
    targets.bind(2,1,32);if(!(targets.binding(2).target==large))return 21;
    targets.reset(1);targets.draw(1,1);output=targets.snapshot();
    if(targets.binding(1).target.resource||targets.binding(1).targets)return 17;
    if(output.trial_targets[2].targets||output.trial_targets[2].target.resource)return 12;
    targets.reject_copy();if(targets.lookup(128).resource||targets.snapshot().copy_rejected!=1)return 13;
    targets.copy(destinations,sources,129);
    if(targets.snapshot().copy_rejected!=2)return 14;
    for(uintptr_t i=1;i<=5000;++i)targets.create(i*32,large);
    if(!targets.snapshot().descriptor_evictions||!(targets.lookup(5000*32)==large))return 15;
    if(!targets.descriptor_capacity_lost())return 22;
    targets.clear_bindings();targets.reset(2);targets.create(32,large);
    if(!targets.descriptor_capacity_lost())return 23;
    if(has_draw_work(0,1)||has_draw_work(3,0)||has_draw_work(0,0)||!has_draw_work(3,1))return 24;
    DrawState saved{true,{0,0,3840,2160},{0,0,16384,16384},1,1,true,.2f,.8f};
    auto plan=overlay_plan(saved,3840,2160);
    if(!plan.valid||!(plan.viewports[1]==Rect{1920,0,1920,2160})||!(plan.scissors[1]==Rect{1920,0,1920,2160}))return 25;
    auto clipped=saved;clipped.scissor={-4,-2,9,7};auto clip=overlay_plan(clipped,3840,2160);
    if(!clip.valid||!(clip.scissors[0]==Rect{0,0,3,5})||!(clip.scissors[1]==Rect{1920,0,3,5}))return 26;
    for(unsigned fault=0;fault<6;++fault){auto bad=saved;
        if(fault==0)bad.depth_known=false;if(fault==1)bad.max_depth=2;
        if(fault==2)bad.viewport_count=2;if(fault==3)bad.scissor_count=0;
        if(fault==4)bad.scissor.x=.5f;if(fault==5)bad.scissor.width=-1;
        if(overlay_plan(bad,3840,2160).valid)return 27;}
    if(overlay_plan(saved,3839,2160).valid)return 28;
    auto empty=saved;empty.scissor={1,0,0,100};if(overlay_plan(empty,3840,2160).valid)return 36;
    empty.scissor={0,1,100,0};if(overlay_plan(empty,3840,2160).valid)return 37;
    empty.scissor={4000,0,100,100};if(overlay_plan(empty,3840,2160).valid)return 38;
    Rect current_view{},current_scissor{};float minimum=0,maximum=0;unsigned draws=0;
    auto view=[&](Rect r,float mn,float mx){current_view=r;minimum=mn;maximum=mx;};
    auto scissor=[&](Rect r){current_scissor=r;};
    auto draw=[&](unsigned eye){if(!(current_view==plan.viewports[eye])||!(current_scissor==plan.scissors[eye]))throw 1;++draws;};
    if(replay_overlay(saved,plan,view,scissor,draw)!=3||draws!=2||!(current_view==saved.viewport)||
        !(current_scissor==saved.scissor)||minimum!=saved.min_depth||maximum!=saved.max_depth)return 29;
    try {replay_overlay(saved,plan,view,scissor,[](unsigned){throw 1;});return 30;}catch(int){}
    if(!(current_view==saved.viewport)||!(current_scissor==saved.scissor)||minimum!=.2f||maximum!=.8f)return 31;
    uintptr_t stack[]{0x140172741,0x14019e753,0x14019f2f3};
    if(!overlay_caller_chain(stack,3,0x140000000)||overlay_caller_chain(stack,2,0x140000000))return 32;
    stack[1]++;if(overlay_caller_chain(stack,3,0x140000000))return 33;
    uintptr_t observed_stack[]{0x140172741,0x14019e753,0x70000080,0x14019f2f3};
    auto observed_ok=[&](uintptr_t entry){return overlay_observed_caller_chain(observed_stack,4,0x140000000,entry,0x70000000,0x70000100);};
    if(!observed_ok(0x14019f2f3)||observed_ok(0x14019f2f4)||overlay_caller_chain(observed_stack,4,0x140000000))return 36;
    observed_stack[2]=0x70000100;if(observed_ok(0x14019f2f3))return 37;
    observed_stack[2]=0x70000080;observed_stack[3]++;if(observed_ok(0x14019f2f3))return 38;
    observed_stack[3]--;observed_stack[1]++;if(observed_ok(0x14019f2f3))return 39;
    auto depth_storage=std::make_unique<Tracker>();auto& depths=*depth_storage;
    depths.reset(123);depths.set(123,other,false,saved.viewport,1,true,.2f,.8f);
    depths.set(123,other,true,saved.scissor);auto state=depths.draw_state(123);
    if(!state.depth_known||state.min_depth!=.2f||state.max_depth!=.8f)return 34;
    depths.reset(123);if(depths.draw_state(123).depth_known)return 35;
    // Provenance snapshots survive descriptor overwrites; target equality and
    // tracking policy remain unchanged. Copy sources must be captured together.
    auto provenance_storage=std::make_unique<Tracker>();auto& provenance=*provenance_storage;
    provenance.create(32,small,7,0x111);provenance.bind(1,1,32);
    const auto first=provenance.binding_provenance(1);
    if(first.handle!=32||first.trace.write_serial!=1||first.trace.origin_create_serial!=1||
       first.trace.create_epoch!=7||first.trace.last_write_epoch!=7||first.trace.creation_live!=0x111)return 40;
    provenance.create(32,large,8,0x222);
    if(!(provenance.binding_provenance(1)==first)||!(provenance.binding(1).target==small))return 41;
    provenance.create(64,small,9,0x333);
    uintptr_t overlap_dest[]{64,96},overlap_src[]{32,64};provenance.copy(overlap_dest,overlap_src,2,10);
    provenance.bind(2,1,64);provenance.bind(3,1,96);
    auto copy1=provenance.binding_provenance(2),copy2=provenance.binding_provenance(3);
    if(!(provenance.binding(2).target==large)||!(provenance.binding(3).target==small)||
       copy1.trace.write_serial!=4||copy1.trace.origin_create_serial!=2||copy1.trace.create_epoch!=8||copy1.trace.last_write_epoch!=10||
       copy1.trace.copy_source_handle!=32||copy1.trace.source_write_serial!=2||copy1.trace.creation_live!=0x222||
       copy2.trace.write_serial!=5||copy2.trace.origin_create_serial!=3||copy2.trace.source_write_serial!=3||
       copy2.trace.copy_source_handle!=64||copy2.trace.creation_live!=0x333)return 42;
    uintptr_t absent=999;provenance.copy(overlap_dest,&absent,1,11);provenance.bind(4,1,64);
    if(provenance.binding(4).target.resource||provenance.binding_provenance(4).trace.write_serial||
       provenance.binding_provenance(4).handle!=64||!(provenance.binding_provenance(2)==copy1))return 43;
    provenance.reset(2);if(provenance.binding_provenance(2).handle||provenance.binding_provenance(2).trace.write_serial)return 44;
    provenance.reject_copy();provenance.bind(2,1,96);
    if(provenance.binding_provenance(2).trace.write_serial||!(provenance.binding_provenance(3)==copy2))return 45;
    provenance.clear_bindings();if(provenance.binding_provenance(3).handle)return 46;
    provenance.create(32,large,12);provenance.bind(3,1,32);
    if(provenance.binding_provenance(3).trace.write_serial!=7)return 47;
    provenance.retire(3);if(provenance.binding_provenance(3).handle)return 48;
    provenance.bind(4,0,64);if(provenance.binding_provenance(4).handle)return 49;
    puts("GPU stats and overlay bounds/depth/state restoration/stack/provenance checks passed");return 0;
}
