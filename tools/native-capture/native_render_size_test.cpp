#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define GEARS_RUNTIME_RENDER_SIZE 1
#define GEARS_NATIVE_RESIZE_PROBE 1
#include "resource_resize_probe.h"
#include <cassert>
#include <cstdio>
int main(){
 using namespace gears_vr;
 assert(!gears_native_render_size::initialize()&&!gears_native_render_size::width());
 RenderSizeLimits limits{{3072,3072},{3264,3264},{8192,8192},{8192,8192},8192,8192};RenderSizePlan p{};
 assert(make_render_size_plan(limits,.7,p)==RenderSizeError::none);
 RenderSizeBridge bridge;assert(bridge.publish(GetCurrentProcessId(),123,p));
 assert(gears_native_render_size::initialize()&&gears_native_render_size::width()==4302&&gears_native_render_size::height()==2285);
 assert(gears_native_render_size::matches_session(123)&&!gears_native_render_size::matches_session(124));
 gears_resize_probe::PinnedCapacity capacity;gears_resize_probe::Snapshot old{};old.width=3840;old.height=2160;old.valid=255;
 uint32_t width=p.source_width,height=p.source_height;
 assert(capacity.choose(11,22,true,old,width,height)==1&&width==4302&&height==2285);
 old.width=4302;old.height=2285;width=3840;height=2160;
 assert(capacity.choose(11,22,false,old,width,height)==2&&width==4302&&height==2285);
 width=4096;height=2048;assert(capacity.choose(11,22,true,old,width,height)==5);
 width=4302;height=2285;assert(capacity.choose(11,23,true,old,width,height)==4);
 puts("runtime native plan control, session and rectangular resource capacity tests passed");
}
