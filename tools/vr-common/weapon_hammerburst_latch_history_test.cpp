#include "weapon_hammerburst_latch_history.h"
#include <cassert>
#include <cstdio>
using namespace gears_weapon;
HammerburstLatchSample sample(double angle,std::uint64_t sequence,std::uint64_t ms){
    return {{.05*std::cos(angle),0,-.05*std::sin(angle)},sequence,ms,7,true,true};
}
int main(){
    HammerburstLatchGeometry g{{0,0,0},{0,1,0},{.05,0,0},.8,.02,.02,.02,true};
    std::array<HammerburstLatchSample,9> points{};
    points[0]=sample(.0242935013,1,1000);
    points[1]=sample(.012,2,1056);points[2]=sample(0,3,1112);
    HammerburstLatchStroke direct;assert(direct.begin(g,points[0],.0242935013));
    assert(direct.observe(points[2])==HammerburstLatchMotion::rejected);
    for(unsigned failure=0;failure<9;++failure){
        auto p=points;auto current=p[2];unsigned count=3;
        if(failure==1)count=2;
        if(failure==2)p[1].milliseconds=1101;
        if(failure==3)p[1].sequence=1;
        if(failure==4)p[1].grip_token=8;
        if(failure==5)p[1].tracked=false;
        if(failure==6)p[0].hand.x+=.001;
        if(failure==7)p[2].hand.x+=.001;
        if(failure==8)p[1].hand={1,0,0};
        HammerburstLatchStroke stroke;assert(stroke.begin(g,points[0],.0242935013));
        const auto result=hammerburst_latch_history(stroke,current,p,count);
        if(!failure){assert(result==HammerburstLatchMotion::closed&&stroke.active()&&stroke.angle()==0);}
        else assert(result==HammerburstLatchMotion::rejected&&!stroke.active());
    }
    std::puts("HB original-point mechanical history: 112ms direct gap rejected; segmented original samples accepted;8 malformed paths rejected. Synthetic points, no live producer/Frame/engine proof.");
}
