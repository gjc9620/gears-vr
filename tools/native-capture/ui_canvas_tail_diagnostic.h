#pragma once
#include "ui_canvas_batch.h"

// Observation only. This is not a batch Snapshot or publication authority.
// A foreign tail's vtable is copied as an opaque value; its payload is never read.
namespace gears_ui_batch_diagnostic {
struct Tail {
    std::uintptr_t canvas{},fcanvas{},element{},item{},vtable{};
    std::int32_t sort_key{};
    gears_ui_batch::detail::Array keys{},elements{},items{};
    bool empty{};
};
inline bool same(Tail const& a,Tail const& b){
    using gears_ui_batch::detail::same;
    return a.canvas==b.canvas&&a.fcanvas==b.fcanvas&&a.element==b.element&&a.item==b.item&&
        a.vtable==b.vtable&&a.sort_key==b.sort_key&&a.empty==b.empty&&
        same(a.keys,b.keys)&&same(a.elements,b.elements)&&same(a.items,b.items);
}
namespace detail {
template<class Read> bool capture(Read read,std::uintptr_t canvas,Tail& out){
    using namespace gears_ui_batch::detail;
    Reader<Read> r{read};Tail t{};t.canvas=canvas;
    if(!r.field(t.fcanvas,canvas,0x9c)||!t.fcanvas||
       !r.field(t.keys,t.fcanvas,0x28)||!valid(t.keys,4096,4)||!t.keys.count||
       !r.field(t.sort_key,t.keys.data,std::size_t(t.keys.count-1)*4)||
       !r.field(t.elements,t.fcanvas,0x48)||!valid(t.elements,4096,20))return false;
    bool found=false;
    for(int i=0;i<t.elements.count;++i){
        std::array<unsigned char,20> raw{};std::uintptr_t address{};std::int32_t key{};
        if(!add(t.elements.data,std::size_t(i)*20,address)||!r.bytes(raw.data(),address,raw.size()))return false;
        std::memcpy(&key,raw.data(),4);if(key!=t.sort_key)continue;
        if(found)return false;found=true;t.element=address;
        std::memcpy(&t.items,raw.data()+4,sizeof(t.items));
        if(!valid(t.items,65536,8))return false;
    }
    t.empty=!found||!t.items.count;
    if(!t.empty&&(!r.field(t.item,t.items.data,std::size_t(t.items.count-1)*8)||!t.item||
       !r.field(t.vtable,t.item,0)))return false;
    out=t;return true;
}
}
template<class Read> bool observe_tail(Read read,std::uintptr_t canvas,Tail& out){
    Tail first{},second{};
    if(!detail::capture(read,canvas,first)||!detail::capture(read,canvas,second)||!same(first,second))return false;
    out=first;return true;
}
}
