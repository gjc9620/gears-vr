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
};
class Tracker {
    struct Slot {uintptr_t list{};unsigned viewport{},scissor{};Rect viewport_rect{},scissor_rect{};};
    std::array<Slot,256> slots{};
    Snapshot data{};
    std::mutex mutex;
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
        if(auto slot=find(list,true)){slot->viewport=slot->scissor=unknown;slot->viewport_rect=slot->scissor_rect={};}}
    void set(uintptr_t list,unsigned category,bool scissor,Rect rect={}) {std::lock_guard<std::mutex> lock(mutex);
        if(scissor)++data.scissors;else ++data.viewports;
        if(auto slot=find(list,true)){(scissor?slot->scissor:slot->viewport)=category<5?category:unknown;
            (scissor?slot->scissor_rect:slot->viewport_rect)=rect;}}
    void draw(uintptr_t list,unsigned phase) {std::lock_guard<std::mutex> lock(mutex);
        auto slot=find(list,false);if(!slot){++data.untracked_draws;return;}
        ++data.draws[phase<3?phase:0][slot->viewport][slot->scissor];
        if(phase==1){
            for(auto& entry:data.trial_rects){
                if(!entry.calls){entry={slot->viewport_rect,slot->scissor_rect,1};return;}
                if(entry.viewport==slot->viewport_rect&&entry.scissor==slot->scissor_rect){++entry.calls;return;}
            }
            ++data.rect_overflow;
        }}
    Snapshot snapshot(){std::lock_guard<std::mutex> lock(mutex);return data;}
};
}
