#pragma once
#include "../native-capture/weapon_call_trace.h"
#include <cstring>

// Read-only invocation locals for the observed retail FireAllPellets body.
// The caller separately closes live typed metadata, binding and shot cohort.
namespace gears_gnasher_trace_input {
struct Layout {
    std::uintptr_t function{},weapon{},parent{},script{};
    unsigned script_bytes{},locals_bytes{},code_offset{},calc_name_index{};
    std::array<std::uintptr_t,3> arguments{}; // pushed start, end, output array
    bool valid() const noexcept {
        return function&&weapon&&parent&&script&&script_bytes<=131072&&script_bytes>=2548&&
            script<=UINTPTR_MAX-script_bytes&&locals_bytes==320&&
            (code_offset==2471||code_offset==2520)&&calc_name_index&&
            arguments[0]&&arguments[1]&&arguments[2]&&arguments[0]!=arguments[1]&&
            arguments[0]!=arguments[2]&&arguments[1]!=arguments[2];
    }
};
template<class Read>bool direct(Read read,std::uintptr_t caller,Layout const& layout,
                               gears_weapon_call_trace::FireInput& out) noexcept {
    out={};if(!layout.valid()||!caller||caller>UINTPTR_MAX-0x38)return false;
    gears_ui_call_boundary::Frame before{},after{};std::uintptr_t locals{},again{};
    if(!gears_ui_call_boundary::frame(read,caller,before)||before.node!=layout.function||
       before.object!=layout.weapon||before.previous!=layout.parent||before.code!=layout.script+layout.code_offset||
       !read(&locals,reinterpret_cast<void const*>(caller+0x30),8)||!locals||(locals&3)||
       locals>UINTPTR_MAX-layout.locals_bytes)return false;
    // EX_VirtualFunction + exact CalcWeaponFire FName, three EX_LocalVariable
    // expressions and EX_EndFunctionParms. Addresses come from live metadata.
    std::array<unsigned char,37> expected{},actual{},closing{};unsigned at{};
    expected[at++]=0x1b;std::memcpy(expected.data()+at,&layout.calc_name_index,4);at+=8;
    for(auto field:layout.arguments){expected[at++]=0;std::memcpy(expected.data()+at,&field,8);at+=8;}
    expected[at++]=0x16;
    const auto call_code=layout.script+layout.code_offset-9;
    if(at!=expected.size()||!read(actual.data(),reinterpret_cast<void const*>(call_code),actual.size())||actual!=expected)return false;
    gears_weapon_call_trace::FireInput value{};
    if(!read(value.start.data(),reinterpret_cast<void const*>(locals),12)||
       !read(value.end.data(),reinterpret_cast<void const*>(locals+112),12)||
       !read(value.pushed.data(),reinterpret_cast<void const*>(locals+160),12))return false;
    for(auto const& vector:{value.start,value.end,value.pushed})for(float x:vector)
        if(!std::isfinite(x)||std::abs(x)>1e8f)return false;
    gears_weapon_call_trace::FireInput second{};
    if(!read(second.start.data(),reinterpret_cast<void const*>(locals),12)||
       !read(second.end.data(),reinterpret_cast<void const*>(locals+112),12)||
       !read(second.pushed.data(),reinterpret_cast<void const*>(locals+160),12)||
       second.start!=value.start||second.end!=value.end||second.pushed!=value.pushed||
       !read(closing.data(),reinterpret_cast<void const*>(call_code),closing.size())||closing!=actual||
       !gears_ui_call_boundary::frame(read,caller,after)||
       !read(&again,reinterpret_cast<void const*>(caller+0x30),8)||again!=locals||
       before.node!=after.node||before.object!=after.object||before.code!=after.code||before.previous!=after.previous)return false;
    out=value;return true;
}
}
