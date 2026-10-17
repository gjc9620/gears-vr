#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <limits>

// Observation-only helpers. Addresses in completed diagnostics are never an
// authorization source; only the synchronous live stack may be inspected.
namespace gears_ui_call_boundary {
inline constexpr std::uintptr_t rva=0x1d6f30;
// First 32 contiguous bytes mechanically concatenated from saved artifact 1
// analysis/ida/reticle-inline59/call-boundary.json (base 0x140000000).
// Current-process instruction comparison and limitations: docs/91.
inline constexpr std::array<unsigned char,32> entry_bytes{{
    0x40,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x81,0xec,
    0x38,0x05,0x00,0x00,0x48,0x8d,0x6c,0x24,0x30,0xc5,0xf8,0x29,0xb5,0xf0,0x04,0x00}};
inline constexpr unsigned frame_limit=12;
struct Frame {std::uintptr_t node{},object{},code{},previous{};};
template<class Read> bool frame(Read read,std::uintptr_t address,Frame& out){
    if(!address||(address&7)||address>(std::numeric_limits<std::uintptr_t>::max)()-0x40)return false;
    Frame value{};
    if(!read(&value.node,reinterpret_cast<void const*>(address+0x18),sizeof(value.node))||
       !read(&value.object,reinterpret_cast<void const*>(address+0x20),sizeof(value.object))||
       !read(&value.code,reinterpret_cast<void const*>(address+0x28),sizeof(value.code))||
       !read(&value.previous,reinterpret_cast<void const*>(address+0x38),sizeof(value.previous))||
       !value.node||!value.object||!value.code)return false;
    out=value;return true;
}
// nearest_frame is supplied only by the existing successful authenticated
// classify_canvas_scope path. A selected callee must have its own actual frame
// and PreviousFrame==entry caller: entry's RDX itself is never a callee frame.
template<class Read> bool body_ancestry(Read read,std::uintptr_t canvas_frame,
    std::uintptr_t nearest_frame,std::uintptr_t caller,std::uintptr_t callee,std::uintptr_t self){
    if(!nearest_frame||!caller||(caller&7)||!callee||!self||nearest_frame==caller)return false;
    std::array<std::uintptr_t,frame_limit> visited{};
    for(unsigned depth=0;depth<frame_limit;++depth){
        if(canvas_frame==caller)return false; // Argument expression / missing callee.
        for(unsigned i=0;i<depth;++i)if(visited[i]==canvas_frame)return false;
        visited[depth]=canvas_frame;Frame current{};
        if(!frame(read,canvas_frame,current))return false;
        if(canvas_frame==nearest_frame)return current.node==callee&&current.object==self&&current.previous==caller;
        canvas_frame=current.previous;
    }return false;
}
struct Child {std::uint64_t id{},parent_token{},parent_child{};std::uintptr_t self{},caller{},callee{};unsigned scope_index{},depth{};};
struct Nesting {Child const* current{};unsigned depth{};};
inline bool selected_depth(Nesting const& state){return state.depth&&state.depth<=frame_limit;}
// Empty calls cannot consume the later positive-body diagnostic budget.
template<class T,unsigned Capacity=32> struct Samples {
    static_assert(Capacity>0&&Capacity<=64);
    std::array<std::array<T,Capacity>,2> rows{};
    std::array<unsigned,2> counts{};
    std::array<std::uint64_t,2> dropped{};
    bool append(T const& value,bool body){
        const unsigned bucket=body?1:0;
        if(counts[bucket]>=Capacity){++dropped[bucket];return false;}
        rows[bucket][counts[bucket]++]=value;return true;
    }
};
struct Nest {
    Nesting& state;Child const* saved;unsigned saved_depth;
    explicit Nest(Nesting& value):state(value),saved(value.current),saved_depth(value.depth){++state.depth;}
    void select(Child const* child){state.current=child;}
    ~Nest(){state.current=saved;state.depth=saved_depth;}
    Nest(Nest const&)=delete;Nest& operator=(Nest const&)=delete;
};
}
