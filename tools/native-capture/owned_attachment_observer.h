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
    std::size_t used_prefix_{};
    static constexpr unsigned cache_bits=Capacity<=64?6:11;
    static constexpr std::size_t cache_capacity=std::size_t{1}<<cache_bits;
    struct CacheEntry {uintptr_t list{};std::size_t index{};};
    mutable std::array<CacheEntry,cache_capacity> cache_{};
    static std::size_t bucket(uintptr_t list){
        return static_cast<std::size_t>(((static_cast<uint64_t>(list)>>4)*11400714819323198485ull)>>(64-cache_bits));
    }
    const Slot* find(uintptr_t list)const{
        if(!list)return nullptr;
        auto& cached=cache_[bucket(list)];
        if(cached.list==list&&cached.index<used_prefix_&&slots[cached.index].list==list)
            return &slots[cached.index];
        for(std::size_t i=0;i<used_prefix_;++i)if(slots[i].list==list){cached={list,i};return &slots[i];}
        return nullptr;
    }
    Slot* find_mutable(uintptr_t list){return const_cast<Slot*>(find(list));}
public:
    void bind(uintptr_t list,Snapshot binding){
        binding.serial=++serial_;
        if(!list){++overflow_;return;}
        if(auto* slot=find_mutable(list)){slot->binding=binding;return;}
        Slot* empty{};
        for(std::size_t i=0;i<used_prefix_;++i)if(!slots[i].list){empty=&slots[i];break;}
        if(!empty&&used_prefix_<Capacity)empty=&slots[used_prefix_++];
        if(empty){*empty={list,binding};cache_[bucket(list)]={list,static_cast<std::size_t>(empty-slots.data())};}
        else ++overflow_;
    }
    Snapshot binding(uintptr_t list)const{if(auto* slot=find(list))return slot->binding;return {};}
    void retire(uintptr_t list){if(auto* slot=find_mutable(list))*slot={};}
    void clear(){slots={};used_prefix_=0;cache_={};}
    uint64_t overflow()const{return overflow_;}
};
}
