#pragma once
#include <array>
#include <cstdint>
#include <cmath>
#include <mutex>

namespace gears_viewport {
// Categories describe API rectangles, not final pixels or engine eye identity.
enum Category:unsigned {unknown,full,left,right,other};
struct Rect {
    float x{},y{},width{},height{};
    bool operator==(const Rect& b) const {return x==b.x&&y==b.y&&width==b.width&&height==b.height;}
};
struct DrawRect {Rect viewport{},scissor{};uint64_t calls{};};
struct Target {
    uintptr_t resource{};
    uint64_t width{};
    unsigned height{},format{},mip{};
    bool operator==(const Target& b) const {return resource==b.resource&&width==b.width&&height==b.height&&format==b.format&&mip==b.mip;}
};
struct TargetDraw {unsigned viewport{},targets{};Target target{};uint64_t calls{};};
inline unsigned classify(float x,float y,float width,float height) {
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(width)||!std::isfinite(height)||width<=0||height<=0)return unknown;
    auto equal=[](float a,float b){return std::abs(a-b)<.01f;};
    if(equal(y,0)&&equal(height,1080)) {
        if(equal(x,0)&&equal(width,1920))return full;
        if(equal(x,0)&&equal(width,960))return left;
        if(equal(x,960)&&equal(width,960))return right;
    }
    return other;
}
struct Snapshot {
    uint64_t resets{},viewports{},scissors{},overflow{},untracked_draws{};
    std::array<std::array<std::array<uint64_t,5>,5>,3> draws{};
    // Exact paired rectangles only inside the trial consumer, bounded storage.
    std::array<DrawRect,64> trial_rects{};
    uint64_t rect_overflow{};
    uint64_t target_binds{},descriptor_creates{},descriptor_copies{},descriptor_evictions{},copy_rejected{},target_overflow{};
    std::array<TargetDraw,128> trial_targets{};
};
class Tracker {
    struct Slot {uintptr_t list{};unsigned viewport{},scissor{};Rect viewport_rect{},scissor_rect{};uintptr_t target_handle{};unsigned targets{};};
    struct Descriptor {uintptr_t handle{};Target target{};};
    std::array<Slot,256> slots{};
    std::array<Descriptor,4096> descriptors{};
    Snapshot data{};
    std::mutex mutex;
    Descriptor* descriptor(uintptr_t handle,bool insert) {
        if(!handle)return nullptr;
        unsigned bucket=static_cast<unsigned>(((handle>>5)^(handle>>17))%1024)*4;
        Descriptor* empty{};
        for(unsigned i=0;i<4;++i){auto& entry=descriptors[bucket+i];
            if(entry.handle==handle)return &entry;if(!entry.handle&&!empty)empty=&entry;}
        if(!insert)return nullptr;
        if(!empty){empty=&descriptors[bucket];++data.descriptor_evictions;}
        *empty={handle,{}};return empty;
    }
    Slot* find(uintptr_t list,bool insert) {
        if(!list)return nullptr;
        Slot* empty{};
        for(auto& slot:slots){if(slot.list==list)return &slot;if(!slot.list&&!empty)empty=&slot;}
        if(insert&&empty){empty->list=list;return empty;}
        if(insert)++data.overflow;
        return nullptr;
    }
public:
    void reset(uintptr_t list) {std::lock_guard<std::mutex> lock(mutex);++data.resets;
        if(auto slot=find(list,true)){slot->viewport=slot->scissor=unknown;slot->viewport_rect=slot->scissor_rect={};slot->target_handle=0;slot->targets=0;}}
    void create(uintptr_t handle,Target target) {std::lock_guard<std::mutex> lock(mutex);++data.descriptor_creates;
        if(auto entry=descriptor(handle,true))entry->target=target;}
    // Snapshot all sources before assigning destinations. Unknown sources clear old metadata.
    void copy(const uintptr_t* destinations,const uintptr_t* sources,unsigned count) {
        std::lock_guard<std::mutex> lock(mutex);std::array<Target,128> values{};
        if(count>values.size()){++data.copy_rejected;for(auto& entry:descriptors)entry={};return;}
        data.descriptor_copies+=count;
        for(unsigned i=0;i<count;++i)if(auto entry=descriptor(sources[i],false))values[i]=entry->target;
        for(unsigned i=0;i<count;++i)if(auto entry=descriptor(destinations[i],true))entry->target=values[i];
    }
    void reject_copy(){std::lock_guard<std::mutex> lock(mutex);++data.copy_rejected;for(auto& entry:descriptors)entry={};}
    void bind(uintptr_t list,unsigned count,uintptr_t first) {std::lock_guard<std::mutex> lock(mutex);++data.target_binds;
        if(auto slot=find(list,true)){slot->targets=count;slot->target_handle=first;}}
    Target lookup(uintptr_t handle){std::lock_guard<std::mutex> lock(mutex);
        auto entry=descriptor(handle,false);return entry?entry->target:Target{};}
    void set(uintptr_t list,unsigned category,bool scissor,Rect rect={}) {std::lock_guard<std::mutex> lock(mutex);
        if(scissor)++data.scissors;else ++data.viewports;
        if(auto slot=find(list,true)){(scissor?slot->scissor:slot->viewport)=category<5?category:unknown;
            (scissor?slot->scissor_rect:slot->viewport_rect)=rect;}}
    void draw(uintptr_t list,unsigned phase) {std::lock_guard<std::mutex> lock(mutex);
        auto slot=find(list,false);if(!slot){++data.untracked_draws;return;}
        ++data.draws[phase<3?phase:0][slot->viewport][slot->scissor];
        if(phase==1){
            Target target{};if(auto entry=descriptor(slot->target_handle,false))target=entry->target;
            bool saved=false;
            for(auto& entry:data.trial_targets){
                if(!entry.calls){entry={slot->viewport,slot->targets,target,1};saved=true;break;}
                if(entry.viewport==slot->viewport&&entry.targets==slot->targets&&entry.target==target){++entry.calls;saved=true;break;}
            }
            if(!saved)++data.target_overflow;
            for(auto& entry:data.trial_rects){
                if(!entry.calls){entry={slot->viewport_rect,slot->scissor_rect,1};return;}
                if(entry.viewport==slot->viewport_rect&&entry.scissor==slot->scissor_rect){++entry.calls;return;}
            }
            ++data.rect_overflow;
        }}
    Snapshot snapshot(){std::lock_guard<std::mutex> lock(mutex);return data;}
};
}
