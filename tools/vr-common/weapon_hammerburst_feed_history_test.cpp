#include "weapon_hammerburst_feed_history.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <limits>
namespace h=gears_hammerburst_feed_history;
using namespace gears_weapon;
FeedSample sample(std::uint64_t seq,std::int64_t ns,double depth){
    return {1,2,3,4,seq,ns,{},{{0,0,depth},{0,0,0,1}},true,true};
}
FeedInsertionStroke seeded(FeedSample const& prior,FeedPortMount const& mount){
    FeedInsertionStroke s;assert(s.arm(1,2,3,4));
    auto first=prior;--first.sequence;first.monotonic_ns-=20000000;first.left_grip.position.z=.06;
    assert(s.observe(first,mount)==FeedResult::approaching);
    assert(s.observe(prior,mount)==FeedResult::approaching);return s;
}
int main(){
    unsigned cases{};FeedPortMount mount;
    // Recorded53560 depths and110.739ms gap, with synthetic producer points.
    // These points are a local fixture, never claimed as recorded game history.
    auto prior=sample(2,1020000000,.0441288882);
    std::array<FeedSample,6> points{};points[0]=prior;
    for(unsigned i=1;i<6;++i)points[i]=sample(2+i,prior.monotonic_ns+110739000*i/5,
        prior.left_grip.position.z+(.0419230863-prior.left_grip.position.z)*i/5);
    auto current=points.back();auto s=seeded(prior,mount);FeedResult result{};
    assert(h::advance(s,prior,current,points.data(),6,mount,result)&&result==FeedResult::approaching);
    auto seated=sample(8,current.monotonic_ns+20000000,.004);
    assert(s.observe(seated,mount)==FeedResult::inserted);++cases;
    auto old=seeded(prior,mount);assert(old.arm(1,2,3,4));
    assert(old.observe(current,mount)==FeedResult::rejected);
    assert(old.observe(seated,mount)==FeedResult::rejected);++cases;
    for(unsigned failure=0;failure<15;++failure){
        auto bad=points;auto before=prior,end=current;auto m=mount;unsigned count=6;
        switch(failure){
        case 0:++bad[2].item;break;
        case 1:++bad[2].world;break;
        case 2:++bad[2].weapon;break;
        case 3:++bad[2].space_epoch;break;
        case 4:++bad[2].sequence;break;
        case 5:bad[2].monotonic_ns=bad[1].monotonic_ns;break;
        case 6:bad[2].tracked=false;break;
        case 7:bad[2].item_held=false;break;
        case 8:bad[2].left_grip.orientation.w=std::numeric_limits<double>::quiet_NaN();break;
        case 9:end.left_grip.position.x+=.01;break;
        case 10:before.left_grip.position.x+=.01;break;
        case 11:bad[2].weapon_pose.position.x=.01;break;
        case 12:count=2;break;
        case 13:m.maximum_sample_gap_ns=1000;break;
        case 14:bad[2].left_grip.position.z=1;break;
        }
        auto actual=seeded(prior,mount),unchanged=actual;FeedResult out=FeedResult::inserted;
        assert(!h::advance(actual,before,end,bad.data(),count,m,out));
        assert(out==FeedResult::inserted);
        // Failed history leaves stroke untouched; direct fresh continuation still inserts.
        auto direct=sample(3,prior.monotonic_ns+20000000,.004);
        assert(actual.observe(direct,mount)==unchanged.observe(direct,mount));++cases;
    }
    // Crossing inside a ring is accepted only while all later samples stay seated.
    auto p=sample(2,1020000000,.05);std::array<FeedSample,4> terminal{p,
        sample(3,1040000000,.004),sample(4,1060000000,.003),sample(5,1080000000,.002)};
    auto terminal_stroke=seeded(p,mount);result=FeedResult::rejected;
    assert(h::advance(terminal_stroke,p,terminal.back(),terminal.data(),4,mount,result)&&result==FeedResult::inserted);++cases;
    for(unsigned failure=0;failure<3;++failure){auto bad=terminal;
        if(failure==0)bad[2].left_grip.position.z=.02;
        if(failure==1)bad[2].left_grip.position.x=.1;
        if(failure==2)bad[2].left_grip.position.z=-1;
        auto a=seeded(p,mount);assert(!h::advance(a,p,bad.back(),bad.data(),4,mount,result));++cases;
    }
    std::printf("Hammerburst feed-history: %u scenarios; real kernel, synthetic original-point fixtures; no game\n",cases);
}
