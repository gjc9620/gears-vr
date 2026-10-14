#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

namespace gears_ui_batch {
static_assert(sizeof(std::uintptr_t)==8,"Certified x64 Canvas layout");
struct Config {std::uintptr_t base{};};
inline constexpr std::size_t vertex_stride=48;
// Only the first 44 bytes are certified fields; the remaining four are padding.
using Vertex=std::array<unsigned char,44>;
struct Snapshot {
    std::uintptr_t canvas{},fcanvas{},item{},payload{};
    std::int32_t sort_key{};
    std::uint32_t vertex_count{};
    bool empty{};
};
struct Receipt {
    std::uintptr_t canvas{},fcanvas{},item{},payload{};
    std::int32_t sort_key{};
    std::uint32_t begin{},end{}; // Half-open range in the POST-call vertex array.
    std::array<Vertex,4> vertices{};
};
namespace detail {
struct Array {std::uintptr_t data{};std::int32_t count{},capacity{};};
static_assert(sizeof(Array)==16,"Certified TArray header");
inline bool same(Array const& a,Array const& b){return a.data==b.data&&a.count==b.count&&a.capacity==b.capacity;}
inline bool same(Snapshot const& a,Snapshot const& b){return a.canvas==b.canvas&&a.fcanvas==b.fcanvas&&
    a.item==b.item&&a.payload==b.payload&&a.sort_key==b.sort_key&&a.vertex_count==b.vertex_count&&a.empty==b.empty;}
inline bool add(std::uintptr_t address,std::size_t offset,std::uintptr_t& out){
    if(!address||address>(std::numeric_limits<std::uintptr_t>::max)()-offset)return false;out=address+offset;return true;
}
template<class Read> struct Reader {
    Read read;
    bool bytes(void* out,std::uintptr_t address,std::size_t size) const {
        return address&&size&&address<=(std::numeric_limits<std::uintptr_t>::max)()-size&&read(out,reinterpret_cast<const void*>(address),size);
    }
    template<class T> bool field(T& out,std::uintptr_t address,std::size_t offset) const {
        std::uintptr_t target{};return add(address,offset,target)&&bytes(&out,target,sizeof(out));
    }
};
inline bool valid(Array const& a,unsigned bound,std::size_t stride){
    if(a.count<0||a.capacity<0||a.count>a.capacity||unsigned(a.capacity)>bound)return false;
    if(!a.data)return a.count==0&&a.capacity==0;
    const auto bytes=std::size_t(a.capacity)*stride;
    return a.data<=(std::numeric_limits<std::uintptr_t>::max)()-bytes;
}
struct Trace {
    Array keys{},elements{},items{},vertices{};
    std::uintptr_t element{};
    std::array<unsigned char,20> element_bytes{};
};
inline bool same(Trace const& a,Trace const& b){return same(a.keys,b.keys)&&same(a.elements,b.elements)&&
    same(a.items,b.items)&&same(a.vertices,b.vertices)&&a.element==b.element&&a.element_bytes==b.element_bytes;}
}
// Admission or per-snapshot type proof. Never invokes the cast virtual function.
template<class Read> bool validate_binding(Config const& c,Read read){
    detail::Reader<Read> r{read};std::uintptr_t table{},cast{},actual{};
    std::array<unsigned char,4> bytes{};
    return detail::add(c.base,0x133cc98,table)&&detail::add(c.base,0x1a44e0,cast)&&
        r.field(actual,table,8)&&actual==cast&&r.bytes(bytes.data(),cast,bytes.size())&&
        bytes==std::array<unsigned char,4>{0x48,0x8b,0xc1,0xc3};
}
namespace detail {
template<class Read> bool capture(Config const& c,Read read,std::uintptr_t canvas,Snapshot& out,Trace& trace_out){
    Reader<Read> r{read};Snapshot s{};Trace t{};s.canvas=canvas;
    if(!validate_binding(c,read)||!r.field(s.fcanvas,canvas,0x9c)||!s.fcanvas||
       !r.field(t.keys,s.fcanvas,0x28)||!valid(t.keys,4096,4)||!t.keys.count||
       !r.field(s.sort_key,t.keys.data,std::size_t(t.keys.count-1)*4)||
       !r.field(t.elements,s.fcanvas,0x48)||!valid(t.elements,4096,20))return false;
    bool found=false;
    for(int i=0;i<t.elements.count;++i){std::array<unsigned char,20> raw{};std::uintptr_t address{};std::int32_t key{};
        if(!add(t.elements.data,std::size_t(i)*20,address)||!r.bytes(raw.data(),address,raw.size()))return false;
        std::memcpy(&key,raw.data(),4);if(key!=s.sort_key)continue;
        if(found)return false;found=true;t.element=address;t.element_bytes=raw;std::memcpy(&t.items,raw.data()+4,16);
        if(!valid(t.items,65536,8))return false;
    }
    s.empty=!found||!t.items.count;
    if(!s.empty){std::uintptr_t table{},expected{};
        if(!r.field(s.item,t.items.data,std::size_t(t.items.count-1)*8)||!s.item||
           !add(c.base,0x133cc98,expected)||!r.field(table,s.item,0)||table!=expected||
           !r.field(s.payload,s.item,8)||!s.payload||!r.field(t.vertices,s.payload,0x48)||!valid(t.vertices,1048576,vertex_stride))return false;
        s.vertex_count=unsigned(t.vertices.count);
    }
    // All indices/pointers were obtained from these current headers. Verify
    // the selected chain did not change while being traversed; failed reads
    // are rejection, never a manufactured empty-before observation.
    Array keys{},elements{},items{},vertices{};std::uintptr_t fcanvas{},item{},payload{},table{},expected{};std::int32_t key{};
    if(!r.field(fcanvas,canvas,0x9c)||fcanvas!=s.fcanvas||!r.field(keys,s.fcanvas,0x28)||!same(keys,t.keys)||
       !r.field(key,t.keys.data,std::size_t(t.keys.count-1)*4)||key!=s.sort_key||
       !r.field(elements,s.fcanvas,0x48)||!same(elements,t.elements))return false;
    if(found){std::array<unsigned char,20> raw{};
        if(!r.bytes(raw.data(),t.element,raw.size())||raw!=t.element_bytes)return false;
        std::memcpy(&items,raw.data()+4,16);if(!same(items,t.items))return false;
    }
    // Element keys can change without a TArray header change. Confirm that the
    // selected key is still absent, or still uniquely selects the SAME element.
    unsigned matches=0;
    for(int i=0;i<t.elements.count;++i){std::int32_t current{};std::uintptr_t address{};
        if(!add(t.elements.data,std::size_t(i)*20,address)||!r.bytes(&current,address,4))return false;
        if(current==s.sort_key){++matches;if(address!=t.element)return false;}
    }
    if(matches!=(found?1u:0u))return false;
    if(!s.empty){if(!r.field(item,t.items.data,std::size_t(t.items.count-1)*8)||item!=s.item||
        !add(c.base,0x133cc98,expected)||!r.field(table,s.item,0)||table!=expected||
        !r.field(payload,s.item,8)||payload!=s.payload||!r.field(vertices,s.payload,0x48)||!same(vertices,t.vertices))return false;}
    if(!r.field(keys,s.fcanvas,0x28)||!same(keys,t.keys)||!r.field(elements,s.fcanvas,0x48)||!same(elements,t.elements)||
       !r.field(fcanvas,canvas,0x9c)||fcanvas!=s.fcanvas||
       !r.field(key,t.keys.data,std::size_t(t.keys.count-1)*4)||key!=s.sort_key)return false;
    out=s;trace_out=t;return true;
}
}
// The authenticated UObject Canvas is supplied by the existing wrapper hook.
// A successful empty snapshot still has a verified current sort key/FCanvas.
template<class Read> bool snapshot(Config const& c,Read read,std::uintptr_t canvas,Snapshot& out){
    Snapshot candidate{};detail::Trace trace{};if(!detail::capture(c,read,canvas,candidate,trace))return false;out=candidate;return true;
}
template<class Read> bool appended_quad(Config const& c,Read read,Snapshot const& before,std::uintptr_t canvas,Receipt& out){
    if(!before.canvas||!before.fcanvas||canvas!=before.canvas||
       (before.empty&&(before.item||before.payload||before.vertex_count))||
       (!before.empty&&(!before.item||!before.payload||before.vertex_count>1048576)))return false;
    Snapshot after{};detail::Trace post{};if(!detail::capture(c,read,canvas,after,post)||after.empty||
        after.fcanvas!=before.fcanvas||after.sort_key!=before.sort_key)return false;
    if(!before.empty&&after.payload==before.payload&&after.item!=before.item)return false;
    const unsigned begin=!before.empty&&after.payload==before.payload?before.vertex_count:0;
    if(begin>1048576-4||after.vertex_count!=begin+4)return false;
    Receipt receipt{};receipt.canvas=canvas;receipt.fcanvas=after.fcanvas;receipt.item=after.item;receipt.payload=after.payload;
    receipt.sort_key=after.sort_key;receipt.begin=begin;receipt.end=begin+4;
    detail::Reader<Read> r{read};std::uintptr_t address{};
    // Use only the newly captured POST header, including its current allocation
    // after a realloc. No previous vertex-data pointer is retained or read.
    for(unsigned i=0;i<receipt.vertices.size();++i)
        if(!detail::add(post.vertices.data,std::size_t(begin+i)*vertex_stride,address)||
           !r.bytes(receipt.vertices[i].data(),address,receipt.vertices[i].size()))return false;
    Snapshot checked{};detail::Trace final{};
    if(!detail::capture(c,read,canvas,checked,final)||!detail::same(after,checked)||!detail::same(post,final))return false;
    out=receipt;return true;
}
// A receipt copies 176 known bytes from four 48-byte-stride vertices, excluding
// padding, and identifies a synchronously observed interval.
// It supplies no persistent allocation identity, ownership epoch, purity of a
// whole batch, queued-command transfer proof, scene ticket, or target freshness.
}
