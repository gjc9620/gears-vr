#pragma once
#include "ui_canvas_batch.h"

namespace gears_ui_tile_batch {
// A foreign tail is a distinct typed state, never an empty simple batch.
// Prefix pointer values are copied, not retained item-array allocations.
inline constexpr unsigned prefix_capacity=64;
struct ForeignTail {
    std::uintptr_t canvas{},fcanvas{},element{},tail{},vtable{};
    std::int32_t sort_key{};
    gears_ui_batch::detail::Array keys{},elements{},items{};
    unsigned prefix_count{};
    std::array<std::uintptr_t,prefix_capacity> prefix{};
};
struct Before {
    bool strict_valid{},foreign_valid{};
    gears_ui_batch::Snapshot strict{};
    ForeignTail foreign{};
};
namespace detail {
inline bool same(ForeignTail const& a,ForeignTail const& b){
    return a.canvas==b.canvas&&a.fcanvas==b.fcanvas&&a.element==b.element&&a.tail==b.tail&&a.vtable==b.vtable&&
        a.sort_key==b.sort_key&&gears_ui_batch::detail::same(a.keys,b.keys)&&
        gears_ui_batch::detail::same(a.elements,b.elements)&&gears_ui_batch::detail::same(a.items,b.items)&&
        a.prefix_count==b.prefix_count&&a.prefix==b.prefix;
}
// after_prefix=0 copies a nonempty before prefix of at most64 pointers.
// Otherwise copy exactly that many old pointers from the POST array, plus its
// separate new tail; the old array may already have been reallocated/freed.
template<class Read> bool chain(Read read,std::uintptr_t canvas,unsigned after_prefix,ForeignTail& out){
    using namespace gears_ui_batch::detail;
    Reader<Read> r{read};ForeignTail t{};t.canvas=canvas;
    if(after_prefix>prefix_capacity||!r.field(t.fcanvas,canvas,0x9c)||!t.fcanvas||
       !r.field(t.keys,t.fcanvas,0x28)||!valid(t.keys,4096,4)||!t.keys.count||
       !r.field(t.sort_key,t.keys.data,std::size_t(t.keys.count-1)*4)||
       !r.field(t.elements,t.fcanvas,0x48)||!valid(t.elements,4096,20))return false;
    bool found=false;
    for(int i=0;i<t.elements.count;++i){
        std::array<unsigned char,20> raw{};std::uintptr_t address{};std::int32_t key{};
        if(!add(t.elements.data,std::size_t(i)*20,address)||!r.bytes(raw.data(),address,raw.size()))return false;
        std::memcpy(&key,raw.data(),4);if(key!=t.sort_key)continue;
        if(found)return false;found=true;t.element=address;std::memcpy(&t.items,raw.data()+4,sizeof(t.items));
        if(!valid(t.items,65536,8))return false;
    }
    if(!found||!t.items.count)return false;
    if(after_prefix){if(unsigned(t.items.count)!=after_prefix+1)return false;t.prefix_count=after_prefix;}
    else {if(unsigned(t.items.count)>prefix_capacity)return false;t.prefix_count=unsigned(t.items.count);}
    for(unsigned i=0;i<t.prefix_count;++i){
        if(!r.field(t.prefix[i],t.items.data,std::size_t(i)*8)||!t.prefix[i])return false;
        for(unsigned j=0;j<i;++j)if(t.prefix[i]==t.prefix[j])return false;
    }
    if(!r.field(t.tail,t.items.data,std::size_t(t.items.count-1)*8)||!t.tail||!r.field(t.vtable,t.tail,0)||!t.vtable)return false;
    if(!after_prefix&&t.tail!=t.prefix[t.prefix_count-1])return false;
    out=t;return true;
}
template<class Read> bool stable_chain(Read read,std::uintptr_t canvas,unsigned after_prefix,ForeignTail& out){
    ForeignTail a{},b{};
    if(!chain(read,canvas,after_prefix,a)||!chain(read,canvas,after_prefix,b)||!same(a,b))return false;
    out=a;return true;
}
}
template<class Read> bool capture_before(gears_ui_batch::Config const& c,Read read,std::uintptr_t canvas,Before& out){
    Before candidate{};
    if(gears_ui_batch::snapshot(c,read,canvas,candidate.strict))candidate.strict_valid=true;
    else {
        std::uintptr_t simple{};
        if(!gears_ui_batch::validate_binding(c,read)||!gears_ui_batch::detail::add(c.base,0x133cc98,simple)||
           !detail::stable_chain(read,canvas,0,candidate.foreign)||candidate.foreign.vtable==simple||
           !gears_ui_batch::validate_binding(c,read))return false;
        candidate.foreign_valid=true;
    }
    out=candidate;return true;
}
template<class Read> bool appended_quad(gears_ui_batch::Config const& c,Read read,Before const& before,
                                       std::uintptr_t canvas,gears_ui_batch::Receipt& out){
    if(before.strict_valid)return !before.foreign_valid&&gears_ui_batch::appended_quad(c,read,before.strict,canvas,out);
    auto const& old=before.foreign;
    std::uintptr_t simple{};
    if(!before.foreign_valid||!old.canvas||canvas!=old.canvas||!old.fcanvas||!old.tail||!old.vtable||
       !old.prefix_count||old.prefix_count>prefix_capacity||unsigned(old.items.count)!=old.prefix_count||
       old.prefix[old.prefix_count-1]!=old.tail||!gears_ui_batch::detail::add(c.base,0x133cc98,simple)||old.vtable==simple)return false;
    gears_ui_batch::Snapshot after{};gears_ui_batch::detail::Trace post{};
    if(!gears_ui_batch::detail::capture(c,read,canvas,after,post)||after.empty||after.fcanvas!=old.fcanvas||
       after.sort_key!=old.sort_key||after.vertex_count!=4)return false;
    ForeignTail current{};
    if(!detail::stable_chain(read,canvas,old.prefix_count,current)||current.fcanvas!=old.fcanvas||
       current.sort_key!=old.sort_key||current.prefix!=old.prefix||current.tail!=after.item||current.vtable!=simple||
       !gears_ui_batch::detail::same(current.items,post.items))return false;
    for(unsigned i=0;i<old.prefix_count;++i)if(current.tail==old.prefix[i])return false;
    gears_ui_batch::detail::Reader<Read> r{read};std::uintptr_t old_type{};
    if(!r.field(old_type,old.tail,0)||old_type!=old.vtable)return false;
    gears_ui_batch::Receipt receipt{};receipt.canvas=canvas;receipt.fcanvas=after.fcanvas;receipt.item=after.item;
    receipt.payload=after.payload;receipt.sort_key=after.sort_key;receipt.begin=0;receipt.end=4;
    for(unsigned i=0;i<receipt.vertices.size();++i){std::uintptr_t address{};
        if(!gears_ui_batch::detail::add(post.vertices.data,std::size_t(i)*gears_ui_batch::vertex_stride,address)||
           !r.bytes(receipt.vertices[i].data(),address,receipt.vertices[i].size()))return false;
    }
    gears_ui_batch::Snapshot checked{};gears_ui_batch::detail::Trace final{};ForeignTail final_chain{};
    if(!gears_ui_batch::detail::capture(c,read,canvas,checked,final)||!gears_ui_batch::detail::same(after,checked)||
       !gears_ui_batch::detail::same(post,final)||!detail::stable_chain(read,canvas,old.prefix_count,final_chain)||
       !detail::same(current,final_chain)||!r.field(old_type,old.tail,0)||old_type!=old.vtable)return false;
    out=receipt;return true;
}
// This certifies one synchronous item-prefix transition and copied quad bytes.
// It does not establish allocation identity or read any foreign item's payload.
}
