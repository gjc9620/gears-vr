#pragma once
#include "source_eye_args.h"
#include "source_view_shadow.h"

namespace gears_split_sources {
struct API {gears_shadow::Allocate allocate;gears_source_args::Constructor construct;
    gears_shadow::Destroy destroy;gears_shadow::Free free;gears_shadow::Read read;};
struct Result {void* views[2]{};unsigned checks{},rejection{};bool cleanup_safe{},quarantined{};
    gears_source_args::Snapshot inputs[2]{};};
inline Result create(API api,void* family,void* source,void* right_state) {
    Result result{};gears_source_args::Snapshot original{};
    if(!api.read(original.bytes.data(),source,original.bytes.size()) || original.pointer(0)!=family ||
       !original.pointer(8) || !right_state || right_state==original.pointer(8) ||
       original.pointer(0x140) || original.pointer(0x418)){result.rejection=1;return result;}
    // Full construction does not accept these post-construction resource lists.
    // Keep this one-frame test in the previously observed empty-list scope.
    if(original.value<int>(0x3f4)!=0 || original.value<int>(0x408)!=0){result.rejection=2;return result;}
    float x=original.value<float>(0x48),y=original.value<float>(0x4c);
    float width=original.value<float>(0x50),height=original.value<float>(0x54);
    if(!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(width) || !std::isfinite(height) ||
       x<0 || y<0 || x+width>8192 || y+height>8192 || std::floor(x)!=x || std::floor(y)!=y ||
       width<64 || width>8192 || height<64 || height>8192 || std::floor(width)!=width ||
       std::floor(height)!=height || std::fmod(width,2.f)!=0){result.rejection=3;return result;}
    result.inputs[0]=original;result.inputs[1]=original;
    for(unsigned i=0;i<2;++i) {
        gears_eye_args::Result shift{};
        if(!gears_eye_args::shift_right(result.inputs[i],i?1.f:-1.f,shift)){result.rejection=4;return result;}
        float half=width*.5f,left=x+(i?half:0.f);
        memcpy(result.inputs[i].bytes.data()+0x48,&left,4);
        memcpy(result.inputs[i].bytes.data()+0x50,&half,4);
    }
    result.checks|=1;
    for(unsigned i=0;i<2;++i)result.views[i]=api.allocate(0x430,8);
    if(!result.views[0] || !result.views[1]) {
        // Neither object has been constructed yet, so raw allocations can free.
        for(void*& view:result.views)if(view){api.free(view,0x430);view=nullptr;}
        result.rejection=5;return result;
    }
    result.checks|=2;result.quarantined=true;
    std::array<gears_source_args::Snapshot,2> outputs{};
    bool returned=true;
    for(unsigned i=0;i<2;++i) {
        void* state=i?right_state:original.pointer(8);
        returned=returned && result.inputs[i].invoke(api.construct,result.views[i],family,state)==result.views[i];
        if(!returned || !api.read(outputs[i].bytes.data(),result.views[i],0x430)){result.rejection=6;return result;}
    }
    result.checks|=4;
    bool matching=true,safe=true;
    constexpr size_t arrays[]={0x380,0x390,0x3ec,0x400};
    for(unsigned i=0;i<2;++i) {
        auto& output=outputs[i];auto& input=result.inputs[i];
        matching=matching && output.pointer(0)==family && output.pointer(8)==(i?right_state:original.pointer(8)) &&
            !memcmp(output.bytes.data()+0x48,input.bytes.data()+0x48,16) &&
            !memcmp(output.bytes.data()+0x70,input.bytes.data()+0x70,128);
        if(output.pointer(0x140) || output.pointer(0x418))safe=false;
        if(output.pointer(0x14c) && (output.pointer(0x14c)==original.pointer(0x14c) ||
            output.pointer(0x14c)==outputs[1-i].pointer(0x14c)))safe=false;
        for(size_t offset:arrays) {
            auto h=gears_shadow::field<gears_shadow::Header>(output.bytes.data(),offset);
            auto src=gears_shadow::field<gears_shadow::Header>(original.bytes.data(),offset);
            auto other=gears_shadow::field<gears_shadow::Header>(outputs[1-i].bytes.data(),offset);
            if(h.count<0 || h.count>256 || h.capacity<h.count || h.capacity>65536 ||
               (h.count && !h.data) || (h.data && (h.data==src.data || h.data==other.data)))safe=false;
        }
        for(size_t offset:{size_t(0x70),size_t(0xb0),size_t(0x160),size_t(0x1a0),size_t(0x1e0),size_t(0x220),size_t(0x260),size_t(0x2b0),size_t(0x2f0)})
            for(unsigned j=0;j<16;++j)if(!std::isfinite(output.value<float>(offset+4*j)))matching=false;
    }
    if(matching)result.checks|=8;
    if(safe){result.checks|=16;result.cleanup_safe=true;result.quarantined=false;}
    gears_source_args::Snapshot after{};
    if(api.read(after.bytes.data(),source,0x430) && after.bytes==original.bytes)result.checks|=32;
    if(result.checks!=63)result.rejection=!safe?7:!matching?8:9;
    return result;
}
inline bool release(API api,Result& result) {
    if(!result.cleanup_safe)return false;
    for(void*& view:result.views)if(view){api.destroy(view);api.free(view,0x430);view=nullptr;}
    result.cleanup_safe=false;return true;
}
}
