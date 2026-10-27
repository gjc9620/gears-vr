#pragma once
#include "ui_call_boundary.h"
#include <cmath>

// Synchronous, bounded FFrame ancestry checks for read-only shot observation.
// These addresses are not persisted as authority and never cause a new call.
namespace gears_weapon_call_trace {
using gears_ui_call_boundary::Frame;
inline bool script_pc(std::uintptr_t pc,std::uintptr_t script,unsigned size){
    return script&&size&&size<=131072&&pc>=script&&pc-script<size;
}
template<class Read> bool direct_caller(Read read,std::uintptr_t caller,
    std::uintptr_t expected_node,std::uintptr_t expected_object,
    std::uintptr_t expected_previous,std::uintptr_t script,unsigned size){
    Frame value{};
    return caller&&expected_previous&&
        gears_ui_call_boundary::frame(read,caller,value)&&
        value.node==expected_node&&value.object==expected_object&&
        value.previous==expected_previous&&script_pc(value.code,script,size);
}
struct FireInput {std::array<float,3> start{},end{},pushed{};};
// The entry frame belongs to Engine.Weapon.InstantFire. Capture only the
// three fixed locals already computed for its direct CalcWeaponFire call.
// Never inspect callee parameters, ImpactList or a frame after this call ends.
template<class Read> bool direct_fire_input(Read read,std::uintptr_t caller,
    std::uintptr_t fire_function,std::uintptr_t weapon,std::uintptr_t parent,
    std::uintptr_t script,unsigned script_size,unsigned locals_size,
    std::array<unsigned,3> offsets,FireInput& out){
    if(!caller||caller>UINTPTR_MAX-0x38||!fire_function||!weapon||!parent||
       !script_pc(script+0x9e,script,script_size)||
       locals_size<52||locals_size>65536||offsets!=std::array<unsigned,3>{0,12,24})return false;
    Frame before{},after{};std::uintptr_t locals{},after_locals{};
    if(!gears_ui_call_boundary::frame(read,caller,before)||
       before.node!=fire_function||before.object!=weapon||
       before.previous!=parent||before.code!=script+0x9e||
       !read(&locals,reinterpret_cast<void const*>(caller+0x30),8)||
       !locals||(locals&3)||locals>UINTPTR_MAX-locals_size)return false;
    FireInput value{};
    if(!read(value.start.data(),reinterpret_cast<void const*>(locals+offsets[0]),12)||
       !read(value.end.data(),reinterpret_cast<void const*>(locals+offsets[1]),12)||
       !read(value.pushed.data(),reinterpret_cast<void const*>(locals+offsets[2]),12)||
       !gears_ui_call_boundary::frame(read,caller,after)||
       !read(&after_locals,reinterpret_cast<void const*>(caller+0x30),8)||
       after.node!=before.node||after.object!=before.object||
       after.code!=before.code||after.previous!=before.previous||
       after_locals!=locals)return false;
    for(float component:value.start)if(!std::isfinite(component)||std::abs(component)>1e8f)return false;
    for(float component:value.end)if(!std::isfinite(component)||std::abs(component)>1e8f)return false;
    for(float component:value.pushed)if(!std::isfinite(component)||std::abs(component)>1e8f)return false;
    out=value;return true;
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
