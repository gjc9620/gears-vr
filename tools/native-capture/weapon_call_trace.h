#pragma once
#include "ui_call_boundary.h"

// Synchronous, bounded FFrame ancestry checks for read-only shot observation.
// These addresses are not persisted as authority and never cause a new call.
namespace gears_weapon_call_trace {
using gears_ui_call_boundary::Frame;
inline bool script_pc(std::uintptr_t pc,std::uintptr_t script,unsigned size){
    return script&&size&&size<=131072&&pc>=script&&pc-script<size;
}
template<class Read> bool tick_ancestry(Read read,std::uintptr_t frame,
    std::uintptr_t tick_frame,std::uintptr_t tick_fn,std::uintptr_t controller,
    std::uintptr_t tick_script,unsigned tick_size){
    if(!frame||!tick_frame||!tick_fn||!controller)return false;
    std::array<std::uintptr_t,gears_ui_call_boundary::frame_limit> visited{};
    for(unsigned depth=0;depth<visited.size();++depth){
        for(unsigned i=0;i<depth;++i)if(visited[i]==frame)return false;
        visited[depth]=frame;Frame current{};
        if(!gears_ui_call_boundary::frame(read,frame,current))return false;
        if(frame==tick_frame)return current.node==tick_fn&&current.object==controller&&
            script_pc(current.code,tick_script,tick_size);
        frame=current.previous;
    }
    return false;
}
template<class Read> bool fire_body_ancestry(Read read,std::uintptr_t caller,
    std::uintptr_t fire_caller,std::uintptr_t fire_fn,std::uintptr_t weapon,
    std::uintptr_t fire_script,unsigned fire_size,
    std::uintptr_t tick_frame,std::uintptr_t tick_fn,std::uintptr_t controller,
    std::uintptr_t tick_script,unsigned tick_size){
    if(!caller||!fire_caller||caller==fire_caller||!fire_fn||!weapon)return false;
    std::array<std::uintptr_t,gears_ui_call_boundary::frame_limit> visited{};
    for(unsigned depth=0;depth<visited.size();++depth){
        if(caller==fire_caller)return false; // Still evaluating the call's arguments.
        for(unsigned i=0;i<depth;++i)if(visited[i]==caller)return false;
        visited[depth]=caller;Frame current{};
        if(!gears_ui_call_boundary::frame(read,caller,current))return false;
        if(current.node==fire_fn&&current.object==weapon&&
           script_pc(current.code,fire_script,fire_size)&&current.previous==fire_caller)
            return tick_ancestry(read,fire_caller,tick_frame,tick_fn,controller,tick_script,tick_size);
        caller=current.previous;
    }
    return false;
}
}
