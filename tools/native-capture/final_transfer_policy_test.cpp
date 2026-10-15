#include "final_transfer_policy.h"
#include <cassert>
#include <limits>
int main(){
    using namespace gears_final_transfer;
    Input a{};a.quad_caller=0x9a7377;a.draw_caller=0x1af92b;a.kind=1;a.elements=6;a.instances=1;a.slots=1;
    a.known_buffer=true;a.source={123,3,1920,1080,1,1,24,1};
    a.binding={0,1,{456,1920,1081,24,0},0};
    a.dimensions={1920,1081,1920,1080};a.args={0,0,1920,1081,0,-.5f,1920,1081};
    a.rectangles={true,{0,0,1920,1081},{0,0,16384,16384},1,1,true,0,1};
    assert(eligible(a));
    auto reject=[&](Input b){assert(!eligible(b));};
    auto b=a;b.source.resource=456;reject(b);b=a;b.slots=2;reject(b);b=a;b.quad_caller=0x2b11a0;reject(b);
    b=a;b.known_buffer=false;reject(b);b=a;b.args[5]=0;reject(b);b=a;b.args[4]=.5f;reject(b);
    b=a;b.dimensions[3]=1081;reject(b);b=a;b.source.width=1919;reject(b);b=a;b.source.height=1079;reject(b);
    b=a;b.source.mips=2;reject(b);b=a;b.source.samples=2;reject(b);b=a;b.source.format=28;reject(b);
    b=a;b.rectangles.scissor.height=1080;reject(b);b=a;b.rectangles.viewport.height=1080;reject(b);
    b=a;b.rectangles.max_depth=.5f;reject(b);b=a;b.args[7]=std::numeric_limits<float>::quiet_NaN();reject(b);
    b=a;b.descriptor_lost=true;reject(b);b=a;b.binding.target.height=1082;reject(b);
    a.binding.target.height=1080;a.dimensions[1]=1080;a.args[3]=a.args[7]=1080;a.args[5]=0;
    a.rectangles.viewport.height=1080;assert(eligible(a));
}
