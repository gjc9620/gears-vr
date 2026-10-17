#include "final_transfer_bracket.h"
#include <cassert>
#include <iostream>
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
    std::cout<<"final_transfer_bracket_test passed\n";
}
