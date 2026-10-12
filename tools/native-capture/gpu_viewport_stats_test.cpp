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
    targets.reset(1);targets.draw(1,1);output=targets.snapshot();
    if(targets.binding(1).target.resource||targets.binding(1).targets)return 17;
    if(output.trial_targets[2].targets||output.trial_targets[2].target.resource)return 12;
    targets.reject_copy();if(targets.lookup(128).resource||targets.snapshot().copy_rejected!=1)return 13;
    targets.copy(destinations,sources,129);
    if(targets.snapshot().copy_rejected!=2)return 14;
    for(uintptr_t i=1;i<=5000;++i)targets.create(i*32,large);
    if(!targets.snapshot().descriptor_evictions||!(targets.lookup(5000*32)==large))return 15;
    puts("GPU stats: rectangles, resets, first RTV binding, descriptor copy/overwrite/unknown and bounded capacity passed");return 0;
}
