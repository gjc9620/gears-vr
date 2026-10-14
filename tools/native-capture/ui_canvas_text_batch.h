#pragma once
#include "ui_canvas_batch.h"
#include <vector>
#include <new>
#include <utility>

namespace gears_ui_text_batch {
using Config=gears_ui_batch::Config;
using Vertex=gears_ui_batch::Vertex;
inline constexpr unsigned max_items=64,max_sort_elements=64,max_added_vertices=1024;
struct Item {std::uintptr_t item{},payload{};std::uint32_t vertex_count{};};
struct Snapshot {
    std::uintptr_t canvas{},fcanvas{}; // canvas==0 for the direct native adapter.
    std::int32_t sort_key{};unsigned item_count{};
    std::array<Item,max_items> items{};
};
struct Range {
    std::uintptr_t item{},payload{};
    std::uint32_t begin{},end{},vertex_offset{};
};
struct Delta {
    std::uintptr_t canvas{},fcanvas{};std::int32_t sort_key{};
    std::vector<Range> ranges;
    // Four-byte padding is excluded. Each source vertex has a 48-byte stride.
    std::vector<Vertex> vertices;
};
namespace detail {
using Array=gears_ui_batch::detail::Array;
using gears_ui_batch::detail::same;
using gears_ui_batch::detail::valid;
using gears_ui_batch::detail::add;
template<class Read> using Reader=gears_ui_batch::detail::Reader<Read>;
inline bool same(Item const& a,Item const& b){return a.item==b.item&&a.payload==b.payload&&a.vertex_count==b.vertex_count;}
inline bool same(Snapshot const& a,Snapshot const& b){
    if(a.canvas!=b.canvas||a.fcanvas!=b.fcanvas||a.sort_key!=b.sort_key||a.item_count!=b.item_count)return false;
    for(unsigned i=0;i<a.item_count;++i)if(!same(a.items[i],b.items[i]))return false;return true;
}
struct Trace {
    Array keys{},elements{},items{};std::uintptr_t element{};
    std::array<unsigned char,20> element_bytes{};
    std::array<Array,max_items> vertices{};
};
inline bool same(Trace const& a,Trace const& b,unsigned count){
    if(!same(a.keys,b.keys)||!same(a.elements,b.elements)||!same(a.items,b.items)||
       a.element!=b.element||a.element_bytes!=b.element_bytes)return false;
    for(unsigned i=0;i<count;++i)if(!same(a.vertices[i],b.vertices[i]))return false;return true;
}
template<class Read> bool capture(Config const& c,Read read,std::uintptr_t fcanvas,Snapshot& out,Trace& trace_out){
    Reader<Read> r{read};Snapshot s{};Trace t{};s.fcanvas=fcanvas;
    if(!gears_ui_batch::validate_binding(c,read)||!fcanvas||
       !r.field(t.keys,fcanvas,0x28)||!valid(t.keys,4096,4)||!t.keys.count||
       !r.field(s.sort_key,t.keys.data,std::size_t(t.keys.count-1)*4)||
       !r.field(t.elements,fcanvas,0x48)||!valid(t.elements,4096,20)||
       unsigned(t.elements.count)>max_sort_elements)return false;
    bool found=false;
    for(int i=0;i<t.elements.count;++i){std::array<unsigned char,20> raw{};std::uintptr_t address{};std::int32_t key{};
        if(!add(t.elements.data,std::size_t(i)*20,address)||!r.bytes(raw.data(),address,raw.size()))return false;
        std::memcpy(&key,raw.data(),4);if(key!=s.sort_key)continue;
        if(found)return false;found=true;t.element=address;t.element_bytes=raw;std::memcpy(&t.items,raw.data()+4,16);
        if(!valid(t.items,4096,8)||unsigned(t.items.count)>max_items)return false;
    }
    s.item_count=unsigned(t.items.count);
    std::array<std::uintptr_t,max_items> pointers{};std::uintptr_t expected{};
    if(!add(c.base,0x133cc98,expected)||
       (s.item_count&&!r.bytes(pointers.data(),t.items.data,s.item_count*sizeof(pointers[0]))))return false;
    for(unsigned i=0;i<s.item_count;++i){auto& item=s.items[i];item.item=pointers[i];std::uintptr_t table{};
        if(!item.item||!r.field(table,item.item,0)||table!=expected||!r.field(item.payload,item.item,8)||!item.payload||
           !r.field(t.vertices[i],item.payload,0x48)||!valid(t.vertices[i],1048576,gears_ui_batch::vertex_stride))return false;
        item.vertex_count=unsigned(t.vertices[i].count);
        for(unsigned j=0;j<i;++j)if(item.item==s.items[j].item||item.payload==s.items[j].payload)return false;
    }
    // Verify every selected item and allocation header, not just the final atlas.
    unsigned matches=0;
    for(int i=0;i<t.elements.count;++i){std::int32_t key{};std::uintptr_t address{};
        if(!add(t.elements.data,std::size_t(i)*20,address)||!r.bytes(&key,address,4))return false;
        if(key==s.sort_key){++matches;if(address!=t.element)return false;}
    }
    if(matches!=(found?1u:0u))return false;
    if(found){std::array<unsigned char,20> raw{};
        if(!r.bytes(raw.data(),t.element,raw.size())||raw!=t.element_bytes)return false;}
    for(unsigned i=0;i<s.item_count;++i){auto const& item=s.items[i];std::uintptr_t pointer{},table{},payload{};Array vertices{};
        if(!r.field(pointer,t.items.data,std::size_t(i)*8)||pointer!=item.item||
           !r.field(table,item.item,0)||table!=expected||!r.field(payload,item.item,8)||payload!=item.payload||
           !r.field(vertices,item.payload,0x48)||!same(vertices,t.vertices[i]))return false;
    }
    Array keys{},elements{};std::int32_t key{};
    if(!r.field(keys,fcanvas,0x28)||!same(keys,t.keys)||!r.field(elements,fcanvas,0x48)||!same(elements,t.elements)||
       !r.field(key,t.keys.data,std::size_t(t.keys.count-1)*4)||key!=s.sort_key)return false;
    out=s;trace_out=t;return true;
}
inline bool valid_before(Snapshot const& s){
    if(!s.fcanvas||s.item_count>max_items)return false;
    for(unsigned i=0;i<s.item_count;++i){auto const& item=s.items[i];
        if(!item.item||!item.payload||item.vertex_count>1048576)return false;
        for(unsigned j=0;j<i;++j)if(item.item==s.items[j].item||item.payload==s.items[j].payload)return false;}
    return true;
}
}
template<class Read> bool snapshot_fcanvas(Config const& c,Read read,std::uintptr_t fcanvas,Snapshot& out){
    Snapshot s{};detail::Trace t{};if(!detail::capture(c,read,fcanvas,s,t))return false;out=s;return true;
}
template<class Read> bool snapshot_canvas(Config const& c,Read read,std::uintptr_t canvas,Snapshot& out){
    detail::Reader<Read> r{read};std::uintptr_t fcanvas{},checked{};Snapshot s{};
    if(!r.field(fcanvas,canvas,0x9c)||!snapshot_fcanvas(c,read,fcanvas,s)||
       !r.field(checked,canvas,0x9c)||checked!=fcanvas)return false;
    s.canvas=canvas;out=s;return true;
}
template<class Read> bool delta_fcanvas(Config const& c,Read read,Snapshot const& before,std::uintptr_t fcanvas,Delta& out){
    if(!detail::valid_before(before)||fcanvas!=before.fcanvas)return false;
    Snapshot after{};detail::Trace post{};
    if(!detail::capture(c,read,fcanvas,after,post)||after.sort_key!=before.sort_key||after.item_count<before.item_count)return false;
    unsigned total=0;
    for(unsigned i=0;i<after.item_count;++i){auto const& item=after.items[i];unsigned begin=0;
        if(i<before.item_count){auto const& old=before.items[i];
            if(item.item!=old.item||item.payload!=old.payload||item.vertex_count<old.vertex_count)return false;begin=old.vertex_count;}
        const unsigned added=item.vertex_count-begin;
        if(added%4||added>max_added_vertices-total)return false;total+=added;
    }
    Delta candidate{};candidate.canvas=before.canvas;candidate.fcanvas=fcanvas;candidate.sort_key=after.sort_key;
    try {
        if(total){candidate.ranges.reserve(after.item_count);candidate.vertices.resize(total);}
        detail::Reader<Read> r{read};unsigned offset=0;
        for(unsigned i=0;i<after.item_count;++i){auto const& item=after.items[i];
            const unsigned begin=i<before.item_count?before.items[i].vertex_count:0;
            if(begin==item.vertex_count)continue;
            candidate.ranges.push_back({item.item,item.payload,begin,item.vertex_count,offset});
            for(unsigned j=begin;j<item.vertex_count;++j){std::uintptr_t address{};
                if(!detail::add(post.vertices[i].data,std::size_t(j)*gears_ui_batch::vertex_stride,address)||
                   !r.bytes(candidate.vertices[offset].data(),address,candidate.vertices[offset].size()))return false;
                ++offset;
            }
        }
        Snapshot checked{};detail::Trace final{};
        if(!detail::capture(c,read,fcanvas,checked,final)||!detail::same(after,checked)||
           !detail::same(post,final,after.item_count))return false;
        out=std::move(candidate);return true;
    }catch(std::bad_alloc const&){return false;}
}
template<class Read> bool delta_canvas(Config const& c,Read read,Snapshot const& before,std::uintptr_t canvas,Delta& out){
    if(!canvas||canvas!=before.canvas)return false;
    detail::Reader<Read> r{read};std::uintptr_t fcanvas{},checked{};Delta candidate{};
    if(!r.field(fcanvas,canvas,0x9c)||fcanvas!=before.fcanvas||!delta_fcanvas(c,read,before,fcanvas,candidate)||
       !r.field(checked,canvas,0x9c)||checked!=fcanvas)return false;
    out=std::move(candidate);return true;
}
// Repeat-read consistency is not an atomic snapshot or allocation/ABA identity.
// No old vertex-data pointer is kept. Unknown render-item types conservatively
// reject the whole observation. This helper does not prove draw membership,
// scene-ticket ownership, complete string rendering or subtitle visibility;
// exceeding any bound rejects observation without modifying the original text.
}
