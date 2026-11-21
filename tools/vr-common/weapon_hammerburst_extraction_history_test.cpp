#include "weapon_hammerburst_extraction_history.h"
#include <cassert>
#include <cstdio>
#include <limits>
namespace h=gears_hammerburst_extraction_history;
using namespace gears_weapon;
int main(){
    FeedPortMount mount;
    std::array<FeedSample,gears_hand_continuity::motion_history_capacity+1> points{};
    for(unsigned i=0;i<points.size();++i)points[i]={1,2,3,4,1+i,
        1000000000+20000000ll*i,{},{{0,0,.06*i/double(points.size()-1)},{0,0,0,1}},true,true};
    const auto prior=points.front();
    HammerburstMagazineExtraction seed;assert(seed.observe(prior,mount));
    auto direct=seed;
    assert(!direct.observe(points.back(),mount)&&!direct.complete());
    auto stroke=seed;assert(h::advance(stroke,prior,points.back(),points,unsigned(points.size()),mount));
    assert(stroke.complete());unsigned cases=2;
    for(unsigned fault=0;fault<10;++fault){
        auto bad=points;unsigned count=unsigned(bad.size());
        switch(fault){
        case 0:++bad[2].item;break;
        case 1:++bad[2].world;break;
        case 2:++bad[2].sequence;break;
        case 3:bad[2].monotonic_ns=bad[1].monotonic_ns;break;
        case 4:bad[2].tracked=false;break;
        case 5:bad[2].item_held=false;break;
        case 6:bad[2].left_grip.orientation.w=std::numeric_limits<double>::quiet_NaN();break;
        case 7:bad[2].left_grip.position.x=1.;break;
        case 8:bad[2].left_grip.position.z=-1.;break;
        case 9:++count;break;
        }
        auto actual=seed;
        assert(!h::advance(actual,prior,bad.back(),bad,count,mount));
        assert(!actual.complete()&&actual.previous_sample()&&
            gears_hammerburst_feed_history::same_sample(*actual.previous_sample(),prior));++cases;
    }
    std::printf("HB extraction history: %u scenarios; actual kernel, synthetic points; no game/grant proof\n",cases);
}
