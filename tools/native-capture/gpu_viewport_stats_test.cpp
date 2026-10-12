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
    puts("GPU viewport stats: rectangles, mismatched scissor, reset/reuse, unknown draw and capacity passed");return 0;
}
