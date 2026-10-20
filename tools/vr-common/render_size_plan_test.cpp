#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "render_size_bridge.h"
#include "mirror_geometry.h"
#include <cassert>
#include <cstdio>
#include <limits>
int main(){
    using namespace gears_vr;
    RenderSizeLimits limits{{3072,3072},{3264,3264},{8192,8192},{8192,8192},8192,8192};RenderSizePlan p{};
    assert(make_render_size_plan(limits,1.,p)==RenderSizeError::none&&p.eye_width==3072&&p.eye_height==3264&&p.source_width==6144&&p.source_height==3264);
    assert(make_render_size_plan(limits,.7,p)==RenderSizeError::none&&p.eye_width==2151&&p.eye_height==2285&&render_size_valid(p));
    const auto before=p;
    assert(make_render_size_plan(limits,1.34,p)==RenderSizeError::engine_limit&&p.source_width==before.source_width);
    for(double invalid:{0.,.249,2.001,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()})
        assert(make_render_size_plan(limits,invalid,p)==RenderSizeError::scale);
    limits.recommended_width[1]=3000;limits.recommended_height[1]=3300;
    assert(make_render_size_plan(limits,1.,p)==RenderSizeError::none&&p.eye_width==3072&&p.eye_height==3300);
    limits.maximum_height[0]=3299;assert(make_render_size_plan(limits,1.,p)==RenderSizeError::runtime_limit);
    limits.maximum_height[0]=8192;limits.recommended_width[1]=0;assert(make_render_size_plan(limits,1.,p)==RenderSizeError::recommendation);
    assert(render_size_valid(before));auto wrong=before;++wrong.source_width;assert(!render_size_valid(wrong));
    wrong=before;wrong.recommended_width=std::numeric_limits<unsigned>::max();wrong.scale=2.;assert(!render_size_valid(wrong));
    const auto rectangle=mirror_rectangle(1920,1080,3072,3264);
    assert(rectangle.height==1080&&rectangle.width==1016&&rectangle.x==452&&rectangle.y==0);
    const auto square=mirror_rectangle(1920,1080,2048,2048);
    assert(square.width==1080&&square.height==1080&&square.x==420);
    const auto wide=mirror_rectangle(1000,2000,2000,1000);
    assert(wide.width==1000&&wide.height==500&&wide.y==750);
    assert(!mirror_rectangle(0,2000,2000,1000).width);
    {
        RenderSizeBridge absent;RenderSizePacket packet{};assert(!absent.read_current(packet));
        RenderSizeBridge producer;assert(producer.publish(GetCurrentProcessId(),123,before));
        RenderSizeBridge duplicate;assert(!duplicate.publish(GetCurrentProcessId(),124,before));
        RenderSizeBridge reader;assert(reader.read_current(packet)&&packet.session==123&&packet.plan.source_width==4302);
        wchar_t name[96]{};swprintf_s(name,L"Local\\GearsVR.RenderSize.%lu",GetCurrentProcessId());
        HANDLE mapping=OpenFileMappingW(FILE_MAP_WRITE,FALSE,name);assert(mapping);
        auto* live=static_cast<RenderSizePacket*>(MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,sizeof(RenderSizePacket)));assert(live);
        const auto original=*live;
        ++live->game_creation;assert(!reader.read_current(packet));*live=original;
        ++live->target_pid;assert(!reader.read_current(packet));*live=original;
        ++live->version;assert(!reader.read_current(packet));*live=original;
        ++live->plan.source_height;assert(!reader.read_current(packet));*live=original;
        assert(reader.read_current(packet));UnmapViewOfFile(live);CloseHandle(mapping);
    }
    puts("render-size planning, engine/runtime limits, rectangle mirror and control identity tests passed");
}
