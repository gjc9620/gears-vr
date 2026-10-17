#include "final_transfer_normalize.h"
#include <cassert>
#include <iostream>
using namespace gears_final_transfer_normalize;
using namespace gears_final_transfer_bracket;
int main(){
    assert(allowed_mode(true,true,true,true,false,false,false));
    for(unsigned fault=0;fault<7;++fault){bool mode[]{true,true,true,true,false,false,false};mode[fault]=!mode[fault];
        assert(!allowed_mode(mode[0],mode[1],mode[2],mode[3],mode[4],mode[5],mode[6]));}
    Input input{};input.quad_caller=0x9a7377;input.draw_caller=0x1af92b;input.kind=1;input.elements=6;input.instances=1;
    input.source={111,3,2560,1080,1,1,24,1};input.slots=1;input.known_buffer=true;
    input.binding.targets=1;input.binding.target={222,2534,1009,24,0};
    input.args={0,0,2534,1009,-76,0,2712,1080};input.dimensions={2534,1009,2560,1080};
    input.rectangles={true,{0,0,2534,1009},{0,0,16384,16384},1,1,true,0,1};
    const auto original=input;auto p=plan(input);assert(p.valid&&p.modified&&input.args==original.args);
    assert((p.effective==std::array<float,8>{0,0,2534,1009,0,0,2560,1080}));
    auto normalized=input;normalized.args=p.effective;assert(eligible_full_source(normalized));
    auto identity=normalized;identity.binding.target.width=2560;identity.binding.target.height=1080;
    identity.args={0,0,2560,1080,0,0,2560,1080};identity.dimensions={2560,1080,2560,1080};
    identity.rectangles.viewport={0,0,2560,1080};
    const auto identity_plan=plan(identity);assert(identity_plan.valid&&!identity_plan.modified&&identity_plan.original==identity_plan.effective);
    auto one_pixel=identity;one_pixel.binding.target.height=1081;one_pixel.args={0,0,2560,1081,0,-.5f,2560,1081};
    one_pixel.dimensions={2560,1081,2560,1080};one_pixel.rectangles.viewport.height=1081;
    assert(plan(one_pixel).valid&&plan(one_pixel).modified);
    assert(!gears_final_transfer::eligible(input)&&!gears_final_transfer::eligible(normalized));
    for(unsigned fault=0;fault<18;++fault){auto x=input;
        switch(fault){
        case 0:x.quad_caller=0;break;case 1:++x.binding.target.width;break;case 2:++x.source.width;break;
        case 3:x.args[6]=0;break;case 4:x.args[4]=std::numeric_limits<float>::quiet_NaN();break;
        case 5:x.source.samples=2;break;case 6:x.source.mips=2;break;case 7:x.source.depth=2;break;
        case 8:x.source.format=28;break;case 9:x.binding.target.resource=x.source.resource;break;
        case 10:x.known_buffer=false;break;case 11:x.descriptor_lost=true;break;
        case 12:x.rectangles.viewport.width/=2;break;case 13:x.rectangles.scissor.width=100;break;
        case 14:x.dimensions[0]=2560;break;case 15:x.args[2]=100;break;
        case 16:x.rectangles.max_depth=.5f;break;case 17:x.binding.target.mip=1;break;
        }
        assert(!plan(x).valid);
    }
    Snapshot entry{},draw{};draw.input=normalized;entry=draw;entry.input.args=input.args;
    entry.raw_list=draw.raw_list=333;entry.list=draw.list={444,5};entry.world=draw.world=1;
    entry.chain=draw.chain=555;entry.inventory=draw.inventory=1;entry.resize=draw.resize=1;
    entry.context=draw.context=666;entry.source_object=draw.source_object=777;
    entry.ticket=draw.ticket=8;entry.pair_linked=draw.pair_linked=true;
    Pending pending{};pending.readable=pending.stable=pending.known_buffer=true;
    pending.count=pending.viewport_count=pending.scissor_count=1;pending.dirty={1,1,0};
    pending.view=888;pending.handle=999;pending.target=normalized.binding.target;
    pending.trace={1,1,1,1,0,222,0};pending.viewport={0,0,2534,1009,0,1};pending.scissor={0,0,16384,16384};
    draw.provenance=entry.provenance={pending.handle,pending.trace};
    ConsumedBinds binds;binds.consume(0,true,true,false);binds.consume(1,true,true,false);
    auto actual_pending=pending;actual_pending.dirty={0,0,0};
    assert(draw_proof(entry,draw,p,pending,actual_pending,binds)==0);
    for(unsigned fault=0;fault<13;++fault){auto a=entry;auto b=draw;auto pd=actual_pending;auto bound=binds;
        switch(fault){
        case 0:++b.list.generation;break;case 1:++b.source_object;break;case 2:++b.input.source.resource;break;
        case 3:++b.world;break;case 4:++b.inventory;break;case 5:++b.ticket;break;
        case 6:b.input.args=input.args;break;case 7:++b.provenance.trace.write_serial;break;
        case 8:++pd.trace.write_serial;break;case 9:bound={};break;
        case 10:b.input.elements=3;break;case 11:b.input.draw_caller=0;break;case 12:b.input.kind=0;break;
        }
        assert(draw_proof(a,b,p,pending,pd,bound)!=0);
    }
    DrawCalls calls;Draw expected{333,0x1af92b,1,6,1};calls.call(expected,false);calls.match(expected);
    assert(completion(calls,1,false,false,0));
    assert(!completion(calls,0,false,false,0)&&!completion(calls,2,false,false,0));
    assert(!completion(calls,1,true,false,0)&&!completion(calls,1,false,true,0)&&!completion(calls,1,false,false,1));
    calls.call(expected,false);assert(!completion(calls,1,false,false,0));
    std::cout<<"final_transfer_normalize_test passed\n";
}
