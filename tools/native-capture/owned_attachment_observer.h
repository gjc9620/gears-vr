#pragma once
#include "gpu_viewport_stats.h"

// CPU OM binding diagnostics only. Caller serializes access with the existing
// frame-hook lock. No resource/descriptor address is dereferenced here.
namespace gears_owned_attachments {
struct Snapshot {
    std::array<gears_viewport::Target,8> targets{};
    std::array<uintptr_t,8> handles{};
    uint64_t serial{};unsigned count{},read_mask{},known_mask{};
    uintptr_t depth_handle{};
    bool observed{},count_valid{},descriptor_capacity_lost{},depth_present{},depth_read{};
    bool complete()const{
        const unsigned mask=count_valid?((1u<<count)-1u):0;
        return observed&&count_valid&&read_mask==mask&&known_mask==mask&&(!count||!descriptor_capacity_lost);
    }
};
// read_handle indexes the original argument array; contiguous bindings consume
// its first handle only. lookup returns a value copied at OM bind time.
template<class ReadHandle,class Lookup>
Snapshot capture(unsigned count,bool contiguous,unsigned stride,bool capacity_lost,
    ReadHandle read_handle,Lookup lookup,bool depth_present,bool depth_read,uintptr_t depth_handle){
    Snapshot out{};out.observed=true;out.count=count;out.count_valid=count<=out.targets.size();
    out.descriptor_capacity_lost=capacity_lost;out.depth_present=depth_present;out.depth_read=depth_read;
    out.depth_handle=depth_read?depth_handle:0;
    if(!out.count_valid)return out;
    uintptr_t first{};const bool first_read=!count||!contiguous||read_handle(0,first);
    for(unsigned i=0;i<count;++i){uintptr_t handle{};
        if(contiguous){const uint64_t offset=uint64_t(i)*stride;
            if(!first_read||(i&&!stride)||offset>UINTPTR_MAX-first)continue;handle=first+static_cast<uintptr_t>(offset);
        }else if(!read_handle(i,handle))continue;
        out.read_mask|=1u<<i;out.handles[i]=handle;out.targets[i]=lookup(handle);
        if(out.targets[i].resource)out.known_mask|=1u<<i;
    }
    return out;
}
inline bool same(Snapshot const& a,Snapshot const& b){
    if(a.observed!=b.observed||a.count!=b.count||a.count_valid!=b.count_valid||a.read_mask!=b.read_mask||
       a.known_mask!=b.known_mask||a.descriptor_capacity_lost!=b.descriptor_capacity_lost||
       a.depth_present!=b.depth_present||a.depth_read!=b.depth_read||a.depth_handle!=b.depth_handle||a.handles!=b.handles)return false;
    for(unsigned i=0;i<a.targets.size();++i)if(!(a.targets[i]==b.targets[i]))return false;
    return true; // Bind serial is chronology, not attachment identity.
}
template<size_t Capacity=gears_recording_capacity::command_lists>
class Tracker {
    struct Slot {uintptr_t list{};Snapshot binding{};};
    std::array<Slot,Capacity> slots{};uint64_t serial_{},overflow_{};
public:
    void bind(uintptr_t list,Snapshot binding){
        binding.serial=++serial_;Slot* empty{};
        if(!list){++overflow_;return;}
        for(auto& slot:slots){if(slot.list==list){slot.binding=binding;return;}if(!slot.list&&!empty)empty=&slot;}
        if(empty)*empty={list,binding};else ++overflow_;
    }
    Snapshot binding(uintptr_t list)const{for(auto const& slot:slots)if(list&&slot.list==list)return slot.binding;return {};}
    void retire(uintptr_t list){for(auto& slot:slots)if(slot.list==list){slot={};return;}}
    void clear(){slots={};}
    uint64_t overflow()const{return overflow_;}
};
}
