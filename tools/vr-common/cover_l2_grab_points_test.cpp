#include "cover_l2_grab_points.h"
#include <cstdio>
using namespace gears_vr;
int checks=0,failures=0;
#define CHECK(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
int main(){
    CoverL2GrabPoints p{2,true,true,true,true,{{{{-70,0,19},true},{{70,0,19},true},{{0,0,70},true}}}};
    CHECK(cover_l2_action(CoverEdgeKind::left)==3);
    CHECK(cover_l2_action(CoverEdgeKind::right)==4);
    CHECK(cover_l2_action(CoverEdgeKind::top)==7);
    CHECK(cover_l2_action(CoverEdgeKind::none)==0);
    CHECK(cover_l2_select(p,{-70,0,19},100,.18).kind==CoverEdgeKind::left);
    CHECK(cover_l2_select(p,{70,0,19},100,.18).kind==CoverEdgeKind::right);
    CHECK(cover_l2_select(p,{0,0,70},100,.18).kind==CoverEdgeKind::top);
    CHECK(!cover_l2_select(p,{0,0,0},100,.18).allowed);
    // Latch survives hand travel, then permission loss ends it immediately.
    auto held=cover_l2_select(p,{70,0,19},100,.18,1);
    CHECK(held.id==1&&held.allowed&&!held.within_acquisition_reach);
    p.lean_left=false;CHECK(!cover_l2_select(p,{70,0,19},100,.18,1).allowed);p.lean_left=true;
    p.cover_type=1;
    CHECK(!cover_l2_select(p,{0,0,70},100,.18).allowed);
    CHECK(!cover_l2_select(p,{0,0,70},100,.18,3).allowed);
    CHECK(cover_l2_select(p,{-70,0,19},100,.18).allowed);
    p.cover_type=2;p.can_popup=false;CHECK(!cover_l2_select(p,{0,0,70},100,.18).allowed);p.can_popup=true;
    p.enabled=false;CHECK(!cover_l2_select(p,{-70,0,19},100,.18).allowed);p.enabled=true;
    p.points[1].available=false;CHECK(!cover_l2_select(p,{70,0,19},100,.18).allowed);p.points[1].available=true;
    p.points[1].world=p.points[0].world;
    CHECK(!cover_l2_select(p,{-70,0,19},100,.18).allowed);
    CHECK(cover_l2_select(p,{-70,0,19},100,.18,1).id==1);
    CHECK(!cover_l2_select(p,{},0,.18).allowed);
    CHECK(!cover_l2_select(p,{},100,-1).allowed);
    CHECK(!cover_l2_select(p,{},100,.18,4).allowed);
    CHECK(!cover_l2_select(p,{NAN,0,0},100,.18,1).allowed);
    p.points[0].world.x=NAN;CHECK(!cover_l2_select(p,{},100,.18,1).allowed);
    p.cover_type=3;CHECK(!cover_l2_select(p,{},100,.18,2).allowed);
    std::printf("CoverL2GrabPointsTest checks=%d failures=%d\n",checks,failures);return failures?1:0;
}
