#include "owned_depth_clear_trial.h"
#include <cassert>
#include <stdexcept>
#include <vector>
using namespace gears_owned_depth_clear;
Input valid(){
    Input in{};in.enabled=in.attested=in.owned_current=in.owned_active=true;
    in.module_base=0x140000000;in.caller=in.module_base+0x1af345;in.ticket=503;in.world=2;
    in.module_size=0x3000000;in.observer_base=0x7ff978900000;in.observer_size=0x100000;
    in.vertices=4;in.instances=1;in.width=5120;in.height=2800;
    in.state={true,{0,0,3840,2160},{0,0,16384,16384},1,1,true,0,1};
    in.om.observed=in.om.count_valid=in.om.depth_present=in.om.depth_read=true;in.om.serial=4;in.om.depth_handle=0x800;
    in.stack_count=3;in.stack={in.caller,in.module_base+0x1ade10,in.module_base+0x27e752};return in;
}
int main(){
    auto in=valid();auto p=plan(in);assert(p.apply());
    in.stack_count=6;in.stack={0x7ff97896b582,0x7ff978977703,0x1401af345,0x7ff97898564d,0x1401ade10,0x14027e752};
    assert(plan(in).apply()); // Observed injected capture-module frame.
    in=valid();
    unsigned draws{};std::vector<Viewport> changed;
    auto set=[&](Viewport v){changed.push_back(v);};
    forward(p,set,[&]{++draws;assert(changed.size()==1&&changed[0].rectangle==gears_viewport::Rect({0,0,5120,2800}));});
    assert(draws==1&&changed.size()==2&&changed[1].rectangle==in.state.viewport&&changed[1].minimum==0&&changed[1].maximum==1);
    changed.clear();draws=0;
    try{forward(p,set,[&]{++draws;throw std::runtime_error("draw");});assert(false);}catch(std::runtime_error const&){}
    assert(draws==1&&changed.size()==2&&changed.back().rectangle==in.state.viewport);
    auto reject=[&](Input bad,Reason expected){auto rejected=plan(bad);assert(rejected.reason==expected);changed.clear();draws=0;
        forward(rejected,set,[&]{++draws;});assert(draws==1&&changed.empty());};
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
    in=valid();in.width=4096;reject(in,output);
    in=valid();in.height=2160;reject(in,output);
    in=valid();in.om.depth_read=false;reject(in,attachments);
    in=valid();in.om.depth_present=false;reject(in,attachments);
    in=valid();in.om.depth_handle=0;reject(in,attachments);
    in=valid();in.om.count=1;reject(in,attachments);
    in=valid();in.om.serial=0;reject(in,attachments);
    in=valid();in.state.scissor.width=3840;reject(in,rectangles);
    in=valid();in.state.viewport_count=2;reject(in,rectangles);
    in=valid();in.state.depth_known=false;reject(in,rectangles);
    in=valid();in.state.min_depth=0.1f;reject(in,rectangles);
    in=valid();in.state.viewport.width=5120;reject(in,rectangles);
    in=valid();in.stack[2]++;reject(in,route);
    in=valid();std::swap(in.stack[1],in.stack[2]);reject(in,route);
    in=valid();in.stack_count=0;reject(in,route);
    in=valid();in.stack_count=4;in.stack={in.caller,in.module_base+0x123456,in.module_base+0x1ade10,in.module_base+0x27e752};reject(in,route);
    in=valid();in.stack_count=4;in.stack={in.caller,0x7ff900001000,in.module_base+0x1ade10,in.module_base+0x27e752};reject(in,route);
    in=valid();in.module_size=0;reject(in,route);
    in=valid();in.observer_base=in.module_base;reject(in,route);
}
