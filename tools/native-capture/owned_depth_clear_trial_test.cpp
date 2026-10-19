#include "owned_depth_clear_trial.h"
#include <cassert>
#include <stdexcept>
#include <vector>
using namespace gears_owned_depth_clear;
Input valid(unsigned width=4608,unsigned height=2704,unsigned desktop_width=1920,unsigned desktop_height=1200){
    Input in{};in.enabled=in.attested=in.owned_current=in.owned_active=true;
    in.module_base=0x140000000;in.caller=in.module_base+0x1af345;in.ticket=503;in.world=2;
    in.module_size=0x3000000;in.observer_base=0x7ff978900000;in.observer_size=0x100000;
    in.vertices=4;in.instances=1;in.width=in.plan_width=width;in.height=in.plan_height=height;in.plan_valid=true;
    in.desktop_valid=true;in.desktop_chain=0x900;in.desktop_queue=in.owned_queue=0xa00;
    in.desktop_epoch=3;in.desktop_generation=in.matched_generation=2;in.inventory_qpc=123;
    in.desktop_count=in.binding_count=2;in.inventory_resources=in.binding_resources={0xb00,0xc00};
    in.desktop_width=desktop_width;in.desktop_height=desktop_height;
    const std::array<int,4> source_rectangle{0,0,int(desktop_width),int(desktop_height)};
    in.source_output=gears_source_output::freeze(in.ticket,in.world,0xd00,0xe00,0xf00,
        source_rectangle,source_rectangle,source_rectangle);
    in.state={true,{0,0,float(desktop_width),float(desktop_height)},{0,0,16384,16384},1,1,true,0,1};
    in.om.observed=in.om.count_valid=in.om.depth_present=in.om.depth_read=true;in.om.serial=4;in.om.depth_handle=0x800;
    in.stack_count=3;in.stack={in.caller,in.module_base+0x1ade10,in.module_base+0x27e752};return in;
}
int main(){
    const std::array<int,4> engine_rectangle{0,0,1280,720};
    auto freeze=[&](std::array<int,4> original,std::array<int,4> left,std::array<int,4> right){
        return gears_source_output::freeze(503,2,0xd00,0xe00,0xf00,original,left,right);};
    assert(freeze(engine_rectangle,engine_rectangle,engine_rectangle).matches(503,2));
    auto wrong=engine_rectangle;wrong[0]=1;assert(!freeze(wrong,wrong,wrong).valid());
    wrong=engine_rectangle;wrong[1]=1;assert(!freeze(wrong,wrong,wrong).valid());
    wrong=engine_rectangle;wrong[2]++;assert(!freeze(engine_rectangle,wrong,engine_rectangle).valid());
    assert(!freeze(engine_rectangle,engine_rectangle,wrong).valid());
    wrong=engine_rectangle;wrong[2]=8193;assert(!freeze(wrong,wrong,wrong).valid());
    wrong=engine_rectangle;wrong[3]=63;assert(!freeze(wrong,wrong,wrong).valid());
    assert(!gears_source_output::freeze(0,2,0xd00,0xe00,0xf00,engine_rectangle,engine_rectangle,engine_rectangle).valid());
    assert(!gears_source_output::freeze(503,0,0xd00,0xe00,0xf00,engine_rectangle,engine_rectangle,engine_rectangle).valid());
    assert(!gears_source_output::freeze(503,2,0,0xe00,0xf00,engine_rectangle,engine_rectangle,engine_rectangle).valid());
    auto in=valid();auto p=plan(in);assert(p.apply());
    in.stack_count=6;in.stack={0x7ff97896b582,0x7ff978977703,0x1401af345,0x7ff97898564d,0x1401ade10,0x14027e752};
    assert(plan(in).apply()); // Observed injected capture-module frame.
    in=valid();
    unsigned draws{};std::vector<Viewport> changed;
    auto set=[&](Viewport v){changed.push_back(v);};
    forward(p,set,[&]{++draws;assert(changed.size()==1&&changed[0].rectangle==gears_viewport::Rect({0,0,float(in.width),float(in.height)}));});
    assert(draws==1&&changed.size()==2&&changed[1].rectangle==in.state.viewport&&changed[1].minimum==0&&changed[1].maximum==1);
    changed.clear();draws=0;
    try{forward(p,set,[&]{++draws;throw std::runtime_error("draw");});assert(false);}catch(std::runtime_error const&){}
    assert(draws==1&&changed.size()==2&&changed.back().rectangle==in.state.viewport);
    auto reject=[&](Input bad,Reason expected){auto rejected=plan(bad);assert(rejected.reason==expected);changed.clear();draws=0;
        forward(rejected,set,[&]{++draws;});assert(draws==1&&changed.empty());};
    in=valid(1818,994,1280,720);in.desktop_width=in.desktop_height=8;
    auto tiny=plan(in);assert(tiny.apply());changed.clear();draws=0;
    forward(tiny,set,[&]{++draws;assert(changed.size()==1&&changed.front().rectangle==gears_viewport::Rect({0,0,1818,994}));});
    assert(draws==1&&changed.size()==2&&changed.back().rectangle==gears_viewport::Rect({0,0,1280,720}));
    in.source_output.ticket++;reject(in,source_extent);
    in=valid();in.source_output.world++;reject(in,source_extent);
    in=valid();in.source_output={};reject(in,source_extent);
    in=valid();in.source_output.source=0;reject(in,source_extent);
    in=valid();in.source_output.family=0;reject(in,source_extent);
    in=valid();in.source_output.target=0;reject(in,source_extent);
    in=valid();in.source_output.rectangle[0]=1;reject(in,source_extent);
    in=valid();in.source_output.rectangle[1]=1;reject(in,source_extent);
    in=valid();in.source_output.rectangle[2]++;reject(in,source_extent);
    in=valid();in.source_output.rectangle[3]++;reject(in,source_extent);
    in=valid();in.state.viewport={0,0,80,56};in.stack[2]=in.module_base+0x16e132;reject(in,source_extent);
    in=valid(1818,994,1280,720);in.desktop_width=in.desktop_height=8;
    in.stack[2]=in.module_base+0x16e132;reject(in,route); // Matching extent never admits the unrelated clear route.
    for(auto dimensions:std::array<std::array<unsigned,4>,5>{{{2560,1400,3840,2160},{5120,2800,1920,1080},
        {3456,1728,1280,1024},{2688,3024,2560,1440},{640,350,3840,2160}}}){
        in=valid(dimensions[0],dimensions[1],dimensions[2],dimensions[3]);auto dynamic=plan(in);assert(dynamic.apply());
        changed.clear();draws=0;forward(dynamic,set,[&]{++draws;});
        assert(draws==1&&changed.size()==2&&changed.front().rectangle.width==float(dimensions[0])&&
            changed.front().rectangle.height==float(dimensions[1])&&changed.back().rectangle==in.state.viewport);
    }
    in=valid();in.state.viewport={0,0,float(in.width),float(in.height)};in.desktop_valid=false;
    reject(in,already_correct); // Exact plan viewport stays untouched, still requiring the native route.
    in.stack[2]++;reject(in,route);
    in=valid();in.enabled=false;reject(in,disabled);
    in=valid();in.attested=false;reject(in,identity);
    in=valid();in.vertices=3;reject(in,arguments);
    in=valid();in.instances=2;reject(in,arguments);
    in=valid();in.start=1;reject(in,arguments);
    in=valid();in.first_instance=1;reject(in,arguments);
    in=valid();in.caller++;reject(in,arguments);
    in=valid();in.owned_current=false;reject(in,owner);
    in=valid();in.ticket=0;reject(in,owner);
    in=valid();in.world=0;reject(in,owner);
    in=valid();in.owned_active=false;reject(in,output);
    in=valid();in.plan_valid=false;reject(in,output);
    in=valid();in.plan_width+=2;reject(in,output);
    in=valid();in.plan_height++;reject(in,output);
    in=valid(8194,2704);reject(in,output);
    in=valid(4609,2704);reject(in,output);
    in=valid();in.om.depth_read=false;reject(in,attachments);
    in=valid();in.om.depth_present=false;reject(in,attachments);
    in=valid();in.om.depth_handle=0;reject(in,attachments);
    in=valid();in.om.count=1;reject(in,attachments);
    in=valid();in.om.serial=0;reject(in,attachments);
    in=valid();in.state.scissor.width=3840;reject(in,rectangles);
    in=valid();in.state.viewport_count=2;reject(in,rectangles);
    in=valid();in.state.depth_known=false;reject(in,rectangles);
    in=valid();in.state.min_depth=0.1f;reject(in,rectangles);
    in=valid();in.state.viewport.width=1000;reject(in,source_extent);
    in=valid();in.state.viewport.x=4;reject(in,source_extent);
    in=valid();in.desktop_valid=false;reject(in,desktop);
    in=valid();in.desktop_width=0;reject(in,desktop);
    in=valid();in.desktop_height=0;reject(in,desktop);
    in=valid();in.desktop_queue++;reject(in,desktop);
    in=valid();in.desktop_generation++;reject(in,desktop);
    in=valid();in.matched_generation=0;reject(in,desktop);
    in=valid();in.inventory_qpc=0;reject(in,desktop);
    in=valid();in.binding_resources[0]++;reject(in,desktop); // Stale Present after a resize, even on the first sample.
    in=valid();in.binding_count=1;reject(in,desktop);
    in=valid();in.inventory_resources[0]=in.binding_resources[0]=0;reject(in,desktop);
    in=valid();in.stack[2]++;reject(in,route);
    in=valid();std::swap(in.stack[1],in.stack[2]);reject(in,route);
    in=valid();in.stack_count=0;reject(in,route);
    in=valid();in.stack_count=4;in.stack={in.caller,in.module_base+0x123456,in.module_base+0x1ade10,in.module_base+0x27e752};reject(in,route);
    in=valid();in.stack_count=4;in.stack={in.caller,0x7ff900001000,in.module_base+0x1ade10,in.module_base+0x27e752};reject(in,route);
    in=valid();in.module_size=0;reject(in,route);
    in=valid();in.observer_base=in.module_base;reject(in,route);
}
