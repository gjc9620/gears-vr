#include "crosshair_tick_evidence.h"
#include <cassert>
#include <iostream>
using namespace gears_crosshair_tick;
static Binding binding(){return {1,2,3,4,5};}
static gears_crosshair_assignment::CompletedAssignmentReceipt receipt(){
    gears_crosshair_assignment::CompletedAssignmentReceipt r{};
    r.controller=1;r.pawn=2;r.weapon=3;r.point={10,20,30};return r;
}
static void seal(Evidence& e){auto t=e.begin(binding(),10);e.assignment(t,receipt(),11);assert(e.finish(t,true,12));}
int main(){
    {Evidence e;seal(e);auto r=e.consume(4,13);assert(r&&r->tick==1&&r->point==receipt().point&&r->binding.root_frame==5);
        assert(r->begin_qpc==10&&r->assignment_qpc==11&&r->finish_qpc==12);assert(!e.consume(4,14));
        seal(e);auto again=e.consume(4,13);assert(again&&again->tick==2&&again->point==r->point);}
    {Evidence e;seal(e);auto t=e.begin(binding(),20);assert(!e.finish(t,true,21));assert(!e.consume(4,22));assert(e.counters.discarded==1);}
    {Evidence e;auto t=e.begin(binding(),10);e.assignment(t,receipt(),11);e.assignment(t,receipt(),12);
        assert(!e.finish(t,true,13)&&!e.consume(4,14));assert(e.counters.duplicate_assignment==1);}
    {Evidence e;auto t=e.begin(binding(),10);e.assignment(t,receipt(),11);auto nested=e.begin(binding(),12);assert(nested==0);
        e.assignment(nested,receipt(),13);assert(!e.finish(nested,true,14));assert(!e.finish(t,true,15));assert(!e.consume(4,16));
        seal(e);assert(e.consume(4,20));}
    {Evidence e;auto t=e.begin(binding(),10);auto nested=e.begin(binding(),11);auto deeper=e.begin(binding(),12);
        assert(!e.finish(deeper,true,13));assert(!e.finish(nested,true,14));assert(!e.finish(t,true,15));}
    for(unsigned kind=0;kind<3;++kind){Evidence e;auto t=e.begin(binding(),10);auto r=receipt();
        if(kind==0)r.controller=9;if(kind==1)r.pawn=9;if(kind==2)r.weapon=9;
        e.assignment(t,r,11);assert(!e.finish(t,true,12));assert(!e.consume(4,13));}
    {Evidence e;auto t=e.begin(binding(),10);e.assignment(t+1,receipt(),11);e.assignment(t,receipt(),12);
        assert(!e.finish(t,true,13));}
    {Evidence e;auto t=e.begin(binding(),10);e.assignment(t,receipt(),11);assert(!e.finish(t+1,true,12));assert(!e.finish(t,true,13));}
    {Evidence e;auto t=e.begin(binding(),10);e.assignment(t,receipt(),11);assert(!e.finish(t,false,12));}
    {Evidence e;seal(e);assert(!e.consume(9,13));assert(!e.consume(4,14));}
    {Evidence e;seal(e);assert(!e.consume(4,12));assert(!e.consume(4,13));}
    for(unsigned kind=0;kind<3;++kind){Evidence e;auto t=e.begin(binding(),kind==0?0:10);
        e.assignment(t,receipt(),kind==1?9:11);assert(!e.finish(t,true,kind==2?10:12));assert(!e.consume(4,13));}
    {Evidence e;auto b=binding();b.world=0;auto t=e.begin(b,10);e.assignment(t,receipt(),11);assert(!e.finish(t,true,12));}
    {Evidence e;auto t=e.begin(binding(),10);auto r=receipt();r.point[0]=(std::numeric_limits<float>::quiet_NaN)();
        e.assignment(t,r,11);assert(!e.finish(t,true,12));}
    {Evidence e;seal(e);e.assignment(1,receipt(),13);assert(!e.consume(4,14));}
    {Evidence e;seal(e);e.invalidate();assert(!e.consume(4,13));assert(e.counters.discarded==1);
        auto t=e.begin(binding(),20);assert(t==2);e.assignment(t,receipt(),21);
        assert(e.finish(t,true,22));auto r=e.consume(4,23);assert(r&&r->tick==2);}
    {Evidence e;auto t=e.begin(binding(),10);e.invalidate();e.assignment(t,receipt(),11);
        assert(!e.finish(t,true,12));assert(!e.consume(4,13));
        auto next=e.begin(binding(),20);assert(next>t);e.assignment(next,receipt(),21);
        assert(e.finish(next,true,22));assert(e.consume(4,23));}
    {Evidence e;auto outer=e.begin(binding(),10);auto inner=e.begin(binding(),11);
        e.invalidate();assert(!e.finish(inner,true,12));assert(!e.finish(outer,true,13));
        auto next=e.begin(binding(),20);assert(next==outer+1);e.assignment(next,receipt(),21);
        assert(e.finish(next,true,22));assert(e.consume(4,23));}
    std::cout<<"crosshair tick evidence tests passed\n";
}
