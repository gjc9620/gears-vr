#include "gpu_viewport_stats.h"
#include <limits>
#include <cstdio>
int main(){
    using namespace gears_viewport;
    if(classify(0,0,1920,1080)!=full||classify(0,0,960,1080)!=left||classify(960,0,960,1080)!=right)return 1;
    if(classify(0,0,640,360)!=other||classify(0,0,0,1080)!=unknown||
       classify(std::numeric_limits<float>::quiet_NaN(),0,960,1080)!=unknown)return 2;
    Tracker tracker;tracker.draw(42,1);tracker.set(42,right,false);tracker.set(42,left,true);tracker.draw(42,1);
    auto sample=tracker.snapshot();
    // A right viewport with a left scissor is distinct from a matching right draw.
    if(sample.untracked_draws!=1||sample.draws[1][right][left]!=1||sample.draws[1][right][right])return 3;
    tracker.reset(42);tracker.draw(42,2);
    if(tracker.snapshot().draws[2][unknown][unknown]!=1)return 4;
    for(uintptr_t i=100;i<356;++i)tracker.reset(i);
    if(tracker.snapshot().overflow!=1)return 5;
    Tracker rectangles;
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
    Tracker targets;Target small{123,960,1080,28,0},large{456,1920,1080,28,0};
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
    Tracker depths;depths.reset(123);depths.set(123,other,false,saved.viewport,1,true,.2f,.8f);
    depths.set(123,other,true,saved.scissor);auto state=depths.draw_state(123);
    if(!state.depth_known||state.min_depth!=.2f||state.max_depth!=.8f)return 34;
    depths.reset(123);if(depths.draw_state(123).depth_known)return 35;
    puts("GPU stats and overlay bounds/depth/state restoration/stack checks passed");return 0;
}
