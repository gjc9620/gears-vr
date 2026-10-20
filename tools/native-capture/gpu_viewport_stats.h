#pragma once
#include <array>
#include <cstdint>
#include <cmath>
#include <mutex>
#include "recording_capacity.h"

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
// Orthogonal CPU descriptor provenance. These fields never affect Target
// equality, draw classification or acceptance.
struct DescriptorTrace {
    uint64_t write_serial{},origin_create_serial{},create_epoch{},last_write_epoch{};
    uintptr_t copy_source_handle{},creation_live{};uint64_t source_write_serial{};
    bool operator==(DescriptorTrace const& b)const{
        return write_serial==b.write_serial&&origin_create_serial==b.origin_create_serial&&create_epoch==b.create_epoch&&
            last_write_epoch==b.last_write_epoch&&copy_source_handle==b.copy_source_handle&&creation_live==b.creation_live&&
            source_write_serial==b.source_write_serial;
    }
};
struct BindingProvenance {
    uintptr_t handle{};DescriptorTrace trace{};
    bool operator==(BindingProvenance const& b)const{return handle==b.handle&&trace==b.trace;}
};
struct DescriptorSnapshot {Target target{};BindingProvenance provenance{};};
struct DrawState {bool tracked{};Rect viewport{},scissor{};unsigned viewport_count{},scissor_count{};
    bool depth_known{};float min_depth{},max_depth{};};
struct OverlayPlan {bool valid{};std::array<Rect,2> viewports{},scissors{};};
inline OverlayPlan overlay_plan(DrawState const& s,unsigned width,unsigned height) {
    OverlayPlan out{};
    if(!s.tracked||s.viewport_count!=1||s.scissor_count!=1||!s.depth_known||!width||width%2||!height||width>8192||height>8192||
       !std::isfinite(s.min_depth)||!std::isfinite(s.max_depth)||s.min_depth<0||s.max_depth>1||s.min_depth>s.max_depth||
       !(s.viewport==Rect{0,0,float(width),float(height)}))return out;
    const float edges[]={s.scissor.x,s.scissor.y,s.scissor.x+s.scissor.width,s.scissor.y+s.scissor.height};
    for(float edge:edges)if(!std::isfinite(edge)||edge<-32768||edge>32768||std::floor(edge)!=edge)return out;
    if(s.scissor.width<0||s.scissor.height<0)return out;
    auto clamp=[](float x,float maximum){return x<0?0:x>maximum?maximum:x;};
    const float left=clamp(edges[0],float(width)),right=clamp(edges[2],float(width));
    const float top=clamp(edges[1],float(height)),bottom=clamp(edges[3],float(height));
    if(right<=left||bottom<=top)return out; // Never expand an empty clip through rounding.
    for(unsigned eye=0;eye<2;++eye){float x=float(eye*width/2);
        out.viewports[eye]={x,0,float(width/2),float(height)};
        float l=std::floor(left/2),r=std::ceil(right/2);
        out.scissors[eye]={x+l,top,r-l,bottom-top};}
    out.valid=true;return out;
}
inline bool overlay_caller_chain(const std::uintptr_t* frames,std::size_t count,std::uintptr_t base) {
    if(!frames||!base)return false;
    for(std::size_t i=0;i+2<count;++i)if(frames[i]-base==0x172741&&
        frames[i+1]-base==0x19e753&&frames[i+2]-base==0x19f2f3)return true;
    return false;
}
// The opt-in Execute observer inserts exactly one owned function frame between
// the renderer and the original dispatcher. The caller supplies its authenticated
// current TLS entry return and the observer's PE unwind-function bounds.
inline bool overlay_observed_caller_chain(const std::uintptr_t* frames,std::size_t count,std::uintptr_t base,
        std::uintptr_t entry_return,std::uintptr_t observer_begin,std::uintptr_t observer_end) {
    if(!frames||!base||entry_return-base!=0x19f2f3||!observer_begin||observer_end<=observer_begin)return false;
    for(std::size_t i=0;i+3<count;++i)if(frames[i]-base==0x172741&&frames[i+1]-base==0x19e753&&
        frames[i+2]>=observer_begin&&frames[i+2]<observer_end&&frames[i+3]==entry_return)return true;
    return false;
}
template<class Viewport,class Scissor,class Draw>
unsigned replay_overlay(DrawState const& saved,OverlayPlan const& plan,Viewport viewport,Scissor scissor,Draw draw) {
    if(!plan.valid)return 0;
    struct Restore {DrawState const& state;Viewport& viewport;Scissor& scissor;
        ~Restore(){viewport(state.viewport,state.min_depth,state.max_depth);scissor(state.scissor);}} restore{saved,viewport,scissor};
    unsigned mask=0;
    for(unsigned eye=0;eye<2;++eye){viewport(plan.viewports[eye],saved.min_depth,saved.max_depth);
        scissor(plan.scissors[eye]);draw(eye);mask|=1u<<eye;}
    return mask;
}
inline bool has_draw_work(unsigned elements,unsigned instances){return elements!=0&&instances!=0;}
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
    std::size_t active_lists{},list_highwater{};
    uintptr_t first_failed_list{}; // First capacity rejection; diagnostic only.
    std::array<std::array<std::array<uint64_t,5>,5>,3> draws{};
    // Exact paired rectangles only inside the trial consumer, bounded storage.
    std::array<DrawRect,64> trial_rects{};
    uint64_t rect_overflow{};
    uint64_t target_binds{},descriptor_creates{},descriptor_copies{},descriptor_evictions{},copy_rejected{},target_overflow{};
    std::array<TargetDraw,128> trial_targets{};
};
class Tracker {
public:
    static constexpr std::size_t list_capacity=gears_recording_capacity::command_lists;
private:
    struct Slot {uintptr_t list{};unsigned viewport{},scissor{};Rect viewport_rect{},scissor_rect{};Target target{};unsigned targets{},viewport_count{},scissor_count{};
        bool depth_known{};float min_depth{},max_depth{};BindingProvenance provenance{};};
    struct Descriptor {uintptr_t handle{};Target target{};DescriptorTrace trace{};};
    std::array<Slot,list_capacity> slots{};
    std::array<Descriptor,4096> descriptors{};
    Snapshot data{};
    uint64_t cleared_binding_overflow{};
    bool descriptor_capacity_lost_{};
    uint64_t descriptor_write_serial_{};
    const bool strict_lifecycle_;
    std::mutex mutex;
    Descriptor* descriptor(uintptr_t handle,bool insert) {
        if(!handle)return nullptr;
        unsigned bucket=static_cast<unsigned>(((handle>>5)^(handle>>17))%1024)*4;
        Descriptor* empty{};
        for(unsigned i=0;i<4;++i){auto& entry=descriptors[bucket+i];
            if(entry.handle==handle)return &entry;if(!entry.handle&&!empty)empty=&entry;}
        if(!insert)return nullptr;
        if(!empty){empty=&descriptors[bucket];++data.descriptor_evictions;descriptor_capacity_lost_=true;}
        *empty={handle,{},{}};return empty;
    }
    Slot* find(uintptr_t list,bool insert) {
        if(!list)return nullptr;
        Slot* empty{};
        for(auto& slot:slots){if(slot.list==list)return &slot;if(!slot.list&&!empty)empty=&slot;}
        if(insert&&empty){empty->list=list;++data.active_lists;
            if(data.active_lists>data.list_highwater)data.list_highwater=data.active_lists;
            return empty;}
        if(insert){++data.overflow;if(!data.first_failed_list)data.first_failed_list=list;}
        return nullptr;
    }
public:
    // Strict callers announce a successful Reset (or observed initially open
    // creation) with reset(). RS/OM calls cannot create a recording implicitly.
    explicit Tracker(bool strict_lifecycle=false):strict_lifecycle_(strict_lifecycle){}
    // CPU recording state only. Submitted proof/GPU lifetimes belong to the
    // caller; descriptors are independent and survive list retirement.
    bool retire(uintptr_t list){std::lock_guard<std::mutex> lock(mutex);
        if(auto slot=find(list,false)){*slot={};--data.active_lists;return true;}return false;}
    // A consuming observer may retire a whole recording window while keeping
    // descriptor provenance from startup priming. The legacy tracker never calls this.
    bool clear_bindings(){std::lock_guard<std::mutex> lock(mutex);slots={};data.active_lists=0;
        bool overflowed=data.overflow!=cleared_binding_overflow;cleared_binding_overflow=data.overflow;return overflowed;}
    // Provenance loss is sticky for this tracker lifetime. Present/anchor cannot
    // recover evicted startup metadata merely by clearing command-list bindings.
    bool descriptor_capacity_lost(){std::lock_guard<std::mutex> lock(mutex);return descriptor_capacity_lost_;}
    bool reset(uintptr_t list) {std::lock_guard<std::mutex> lock(mutex);++data.resets;
        if(auto slot=find(list,true)){slot->viewport=slot->scissor=unknown;slot->viewport_rect=slot->scissor_rect={};slot->target={};slot->provenance={};slot->targets=slot->viewport_count=slot->scissor_count=0;slot->depth_known=false;slot->min_depth=slot->max_depth=0;return true;}return false;}
    void create(uintptr_t handle,Target target,uint64_t epoch=0,uintptr_t creation_live=0) {
        std::lock_guard<std::mutex> lock(mutex);++data.descriptor_creates;const auto serial=++descriptor_write_serial_;
        if(auto entry=descriptor(handle,true)){entry->target=target;entry->trace={serial,serial,epoch,epoch,0,creation_live,0};}}
    // Snapshot all sources before assigning destinations. Unknown sources clear old metadata.
    void copy(const uintptr_t* destinations,const uintptr_t* sources,unsigned count,uint64_t epoch=0) {
        std::lock_guard<std::mutex> lock(mutex);std::array<Descriptor,128> values{};
        if(count>values.size()){++data.copy_rejected;for(auto& entry:descriptors)entry={};return;}
        data.descriptor_copies+=count;
        for(unsigned i=0;i<count;++i)if(auto entry=descriptor(sources[i],false))values[i]=*entry;
        for(unsigned i=0;i<count;++i){const auto serial=++descriptor_write_serial_;
            if(auto entry=descriptor(destinations[i],true)){entry->target=values[i].target;entry->trace={};
                if(values[i].trace.write_serial){entry->trace=values[i].trace;entry->trace.write_serial=serial;
                    entry->trace.last_write_epoch=epoch;entry->trace.copy_source_handle=sources[i];
                    entry->trace.source_write_serial=values[i].trace.write_serial;}}}
    }
    void reject_copy(){std::lock_guard<std::mutex> lock(mutex);++data.copy_rejected;for(auto& entry:descriptors)entry={};}
    bool bind(uintptr_t list,unsigned count,uintptr_t first) {std::lock_guard<std::mutex> lock(mutex);++data.target_binds;
        if(auto slot=find(list,!strict_lifecycle_)){slot->targets=count;slot->target={};slot->provenance={count?first:0,{}};
            // OMSetRenderTargets consumes CPU RTV descriptors at the bind call.
            // Later descriptor overwrites do not change this recorded binding.
            if(count)if(auto entry=descriptor(first,false)){slot->target=entry->target;slot->provenance.trace=entry->trace;}
            return true;}return false;}
    Target lookup(uintptr_t handle){std::lock_guard<std::mutex> lock(mutex);
        auto entry=descriptor(handle,false);return entry?entry->target:Target{};}
    DescriptorSnapshot descriptor_snapshot(uintptr_t handle){std::lock_guard<std::mutex> lock(mutex);
        auto entry=descriptor(handle,false);return entry?DescriptorSnapshot{entry->target,{handle,entry->trace}}:DescriptorSnapshot{};}
    TargetDraw binding(uintptr_t list){std::lock_guard<std::mutex> lock(mutex);
        auto slot=find(list,false);if(!slot)return {};
        return {slot->viewport,slot->targets,slot->target,0};}
    BindingProvenance binding_provenance(uintptr_t list){std::lock_guard<std::mutex> lock(mutex);
        auto slot=find(list,false);return slot?slot->provenance:BindingProvenance{};}
    DrawState draw_state(uintptr_t list){std::lock_guard<std::mutex> lock(mutex);auto slot=find(list,false);
        return slot?DrawState{true,slot->viewport_rect,slot->scissor_rect,slot->viewport_count,slot->scissor_count,slot->depth_known,slot->min_depth,slot->max_depth}:DrawState{};}
    bool set(uintptr_t list,unsigned category,bool scissor,Rect rect={},unsigned count=1,bool depth_known=false,float min_depth=0,float max_depth=0) {std::lock_guard<std::mutex> lock(mutex);
        if(scissor)++data.scissors;else ++data.viewports;
        if(auto slot=find(list,!strict_lifecycle_)){(scissor?slot->scissor:slot->viewport)=category<5?category:unknown;
            (scissor?slot->scissor_count:slot->viewport_count)=count;
            (scissor?slot->scissor_rect:slot->viewport_rect)=rect;
            if(!scissor){slot->depth_known=depth_known;slot->min_depth=min_depth;slot->max_depth=max_depth;}return true;}return false;}
    void draw(uintptr_t list,unsigned phase) {std::lock_guard<std::mutex> lock(mutex);
        auto slot=find(list,false);if(!slot){++data.untracked_draws;return;}
        ++data.draws[phase<3?phase:0][slot->viewport][slot->scissor];
        if(phase==1){
            Target target=slot->target;
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
