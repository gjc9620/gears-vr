#pragma once
#include "source_eye_args.h"
#include "source_view_shadow.h"
#include "projection_control.h"
#include "../vr-common/stereo_frame.h"
#include "source_map_snapshot.h"
#include <memory>
#include <new>

namespace gears_split_sources {
struct API {gears_shadow::Allocate allocate;gears_source_args::Constructor construct;
    gears_shadow::Destroy destroy;gears_shadow::Free free;gears_shadow::Read read;};
// Constructor geometry only. Selecting/allocating an independent engine output
// target belongs to the caller; this does not modify the family or source.
struct FixedOutputBounds {std::uint32_t width{},height{};};
// Optional per-eye render dimensions, independent of the final SBS output.
// The caller supplies actual native SR dimensions, never a quality multiplier.
// Input slots remain padded at output_width/2 so Copy0 cannot overwrite Input1.
struct RenderInputBounds {std::uint32_t width{},height{};};
enum InputFailure:unsigned {read_failed=1,family_mismatch=2,missing_original_state=4,
    missing_right_state=8,aliased_state=16,field140_nonzero=32,field418_nonzero=64};
struct InputEvidence {bool read_succeeded{};unsigned failure_mask{};
    std::uintptr_t family{},source{},observed_family{},original_state{},right_state{},field140{},field418{};};
struct Result {void* views[2]{};unsigned checks{},rejection{};bool cleanup_safe{},quarantined{};
    gears_source_args::Snapshot inputs[2]{};InputEvidence input_evidence{};
    unsigned map_checks{},map_nodes{};};
struct MapEvidence {gears_source_map::Snapshot before{},eyes[2]{},after{};};
inline bool map_outside_view(gears_source_map::Snapshot const& map,std::uintptr_t view) {
    if(gears_source_map::overlap(map.header.buckets,std::size_t(map.bucket_count)*8,view,0x430))return false;
    for(unsigned i=0;i<map.node_count;++i)
        if(gears_source_map::overlap(map.nodes[i].address,gears_source_map::node_size,view,0x430))return false;
    return true;
}
inline Result create(API api,void* family,void* source,void* right_state,bool output_bounds=false,bool preserve_geometry=false,
    bool projection_control=false,gears_projection::Phase phase=gears_projection::Phase::a1,
    const gears_vr::StereoMatrices* stereo=nullptr,const FixedOutputBounds* fixed_output=nullptr,
    const RenderInputBounds* render_input=nullptr,void* constructor_family=nullptr) {
    // family authenticates the immutable original source. The caller-owned
    // constructor family must remain alive through renderer construction and clone release.
    // Its target may differ; never substitute it into the original-source guard.
    void* const output_family=constructor_family?constructor_family:family;
    Result result{};gears_source_args::Snapshot original{};
    const auto fixed=fixed_output?*fixed_output:FixedOutputBounds{};
    if(fixed_output&&(!stereo||(fixed.width<128||fixed.width>8192||fixed.width%2||fixed.height<64||fixed.height>8192))){
        result.rejection=13;return result;}
    if(stereo && (projection_control || preserve_geometry)){result.rejection=11;return result;}
    const auto render=render_input?*render_input:RenderInputBounds{};
    if(render_input&&(!fixed_output||!stereo||render.width<64||render.height<64||
        render.width>fixed.width/2||render.height>fixed.height)){
        result.rejection=14;return result;}
    if(stereo)for(unsigned i=0;i<2;++i)for(unsigned j=0;j<16;++j)
        if(!std::isfinite(stereo->view[i][j]) || !std::isfinite(stereo->projection[i][j])){result.rejection=12;return result;}
    auto& evidence=result.input_evidence;
    evidence.family=reinterpret_cast<std::uintptr_t>(family);evidence.source=reinterpret_cast<std::uintptr_t>(source);
    evidence.right_state=reinterpret_cast<std::uintptr_t>(right_state);
    evidence.read_succeeded=api.read(original.bytes.data(),source,original.bytes.size());
    if(!evidence.read_succeeded)evidence.failure_mask=read_failed;
    else {
        // Snapshot fields are meaningful only after the full read succeeded.
        // These additional reads touch the local copy, never the game again.
        evidence.observed_family=reinterpret_cast<std::uintptr_t>(original.pointer(0));
        evidence.original_state=reinterpret_cast<std::uintptr_t>(original.pointer(8));
        evidence.field140=reinterpret_cast<std::uintptr_t>(original.pointer(0x140));
        evidence.field418=reinterpret_cast<std::uintptr_t>(original.pointer(0x418));
        if(original.pointer(0)!=family)evidence.failure_mask|=family_mismatch;
        if(!evidence.original_state)evidence.failure_mask|=missing_original_state;
        if(!right_state)evidence.failure_mask|=missing_right_state;
        // Two missing states are absence, not an alias between live states.
        if(right_state&&evidence.original_state&&right_state==original.pointer(8))evidence.failure_mask|=aliased_state;
        if(evidence.field140)evidence.failure_mask|=field140_nonzero;
        if(evidence.field418)evidence.failure_mask|=field418_nonzero;
    }
    // Nonempty maps are copied by the native constructor, one opaque payload
    // per independently allocated node. Keep every other admission guard.
    // Heap snapshots avoid adding large bounded arrays to the game's stack.
    std::unique_ptr<MapEvidence> maps;
    if(evidence.failure_mask==field140_nonzero){
        maps.reset(new(std::nothrow) MapEvidence);
        const auto source_address=reinterpret_cast<std::uintptr_t>(source);
        if(maps&&source_address<=(std::numeric_limits<std::uintptr_t>::max)()-0x430&&
           gears_source_map::read(api.read,source_address+0x140,maps->before,original.bytes.data()+0x140)&&
           map_outside_view(maps->before,source_address)){
            evidence.failure_mask=0;result.map_checks=1;result.map_nodes=maps->before.node_count;
        }else maps.reset();
    }
    if(evidence.failure_mask){result.rejection=1;return result;}
    // Full construction does not accept these post-construction resource lists.
    // Keep this one-frame test in the previously observed empty-list scope.
    if(original.value<int>(0x3f4)!=0 || original.value<int>(0x408)!=0){result.rejection=2;return result;}
    float x=original.value<float>(0x48),y=original.value<float>(0x4c);
    float width=original.value<float>(0x50),height=original.value<float>(0x54);
    if(output_bounds) {
        x=static_cast<float>(original.value<int>(0x58));y=static_cast<float>(original.value<int>(0x5c));
        width=static_cast<float>(original.value<int>(0x60));height=static_cast<float>(original.value<int>(0x64));
    }
    // A fixed stereo profile does not inherit game/window origins or dimensions.
    // Only the local constructor arguments change; every source byte is retained.
    if(fixed_output){x=y=0;width=float(fixed.width);height=float(fixed.height);}
    if(!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(width) || !std::isfinite(height) ||
       x<0 || y<0 || x+width>8192 || y+height>8192 || std::floor(x)!=x || std::floor(y)!=y ||
       width<64 || width>8192 || height<64 || height>8192 || std::floor(width)!=width ||
       std::floor(height)!=height || std::fmod(width,2.f)!=0){result.rejection=3;return result;}
    result.inputs[0]=original;result.inputs[1]=original;
    if(stereo)for(unsigned i=0;i<2;++i){memcpy(result.inputs[i].bytes.data()+0x70,stereo->view[i].data(),64);
        memcpy(result.inputs[i].bytes.data()+0xb0,stereo->projection[i].data(),64);}
    if(projection_control)for(auto& input:result.inputs)if(!gears_projection::apply(input,phase)){result.rejection=10;return result;}
    for(unsigned i=0;i<2;++i) {
        if(preserve_geometry)continue; // Controls isolate reconstruction/multi-view processing from geometry.
        gears_eye_args::Result shift{};
        if(!stereo && !gears_eye_args::shift_right(result.inputs[i],i?1.f:-1.f,shift)){result.rejection=4;return result;}
        float half=width*.5f,left=x+(i?half:0.f);
        float rectangle[]={left,y,render_input?float(render.width):half,render_input?float(render.height):height};
        memcpy(result.inputs[i].bytes.data()+0x48,rectangle,16);
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
        returned=returned && result.inputs[i].invoke(api.construct,result.views[i],output_family,state)==result.views[i];
        if(!returned || !api.read(outputs[i].bytes.data(),result.views[i],0x430)){result.rejection=6;return result;}
    }
    result.checks|=4;
    bool matching=true,safe=true;
    constexpr size_t arrays[]={0x380,0x390,0x3ec,0x400};
    for(unsigned i=0;i<2;++i) {
        auto& output=outputs[i];auto& input=result.inputs[i];
        matching=matching && output.pointer(0)==output_family && output.pointer(8)==(i?right_state:original.pointer(8)) &&
            !memcmp(output.bytes.data()+0x48,input.bytes.data()+0x48,16) &&
            !memcmp(output.bytes.data()+0x70,input.bytes.data()+0x70,128);
        if((!maps&&output.pointer(0x140)) || output.pointer(0x418))safe=false;
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
    if(maps){
        bool map_safe=true,map_matching=true;
        for(unsigned i=0;i<2;++i){
            auto address=reinterpret_cast<std::uintptr_t>(result.views[i]);
            bool read_ok=gears_source_map::read(api.read,address+0x140,maps->eyes[i],outputs[i].bytes.data()+0x140);
            if(read_ok)result.map_checks|=2u<<i;
            map_safe=read_ok&&map_safe;
            map_matching=read_ok&&map_matching;
            if(read_ok){
                map_matching=gears_source_map::same_contents(maps->before,maps->eyes[i])&&map_matching;
                map_safe=gears_source_map::independent(maps->before,maps->eyes[i])&&map_safe;
                for(auto view:{reinterpret_cast<std::uintptr_t>(source),
                    reinterpret_cast<std::uintptr_t>(result.views[0]),reinterpret_cast<std::uintptr_t>(result.views[1])})
                    map_safe=map_outside_view(maps->eyes[i],view)&&map_safe;
                map_safe=map_outside_view(maps->before,address)&&map_safe;
            }
        }
        map_safe=gears_source_map::independent(maps->eyes[0],maps->eyes[1])&&map_safe;
        if(map_safe)result.map_checks|=8;
        if(map_matching)result.map_checks|=16;
        safe=safe&&map_safe;matching=matching&&map_matching;
    }
    if(matching)result.checks|=8;
    gears_source_args::Snapshot after{};
    bool original_unchanged=api.read(after.bytes.data(),source,0x430) && after.bytes==original.bytes;
    if(maps){
        const bool after_read=gears_source_map::read(api.read,reinterpret_cast<std::uintptr_t>(source)+0x140,maps->after);
        bool after_safe=after_read;
        if(after_read){
            for(unsigned i=0;i<2;++i)
                after_safe=gears_source_map::independent(maps->after,maps->eyes[i])&&after_safe;
            for(auto view:{reinterpret_cast<std::uintptr_t>(source),
                reinterpret_cast<std::uintptr_t>(result.views[0]),reinterpret_cast<std::uintptr_t>(result.views[1])})
                after_safe=map_outside_view(maps->after,view)&&after_safe;
        }
        // Source ownership may have changed during native construction. Do not
        // release eye storage if current source ownership is unreadable/shared.
        safe=safe&&after_safe;
        if(!after_safe)result.map_checks&=~8u;
        bool map_unchanged=after_read&&gears_source_map::unchanged(maps->before,maps->after);
        if(map_unchanged)result.map_checks|=32;
        original_unchanged=original_unchanged&&map_unchanged;
    }
    if(original_unchanged)result.checks|=32;
    if(safe){result.checks|=16;result.cleanup_safe=true;result.quarantined=false;}
    if(result.checks!=63)result.rejection=!safe?7:!matching?8:9;
    return result;
}
inline bool release(API api,Result& result) {
    if(!result.cleanup_safe)return false;
    for(void*& view:result.views)if(view){api.destroy(view);api.free(view,0x430);view=nullptr;}
    result.cleanup_safe=false;return true;
}
}
