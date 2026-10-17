#pragma once
#include "ui_canvas_batch.h"

// Structural historical-route candidates only. This does not establish UObject
// registry identity, active scene/local-player membership, GPU ownership or an
// admitted PathChoice draw. Hook reservation proof is separate from runtime.
namespace gears_ui_scene_root {
namespace d=gears_ui_batch::detail;
struct Config {std::uintptr_t base{};};
inline constexpr std::uintptr_t adapter_rva=0x296178,caller_site_rva=0x2c3dc8,caller_return_rva=0x2c3dce;
inline constexpr std::uintptr_t client_table_rva=0x1429d58,scene_table_rva=0x1382758,scene_render_rva=0x296188;
inline constexpr std::array<unsigned char,13> adapter_bytes{0x49,0x8b,0x00,0x49,0x8b,0xc8,0x48,0xff,0xa0,0xa8,0x04,0x00,0x00};
inline constexpr std::array<unsigned char,6> caller_bytes{0xff,0x90,0xb8,0x02,0x00,0x00};
struct Candidate {std::uintptr_t client{},fcanvas{},scene{},client_vtable{},scene_vtable{};};
struct MatrixSnapshot {std::uintptr_t fcanvas{},top_address{};d::Array stack{};std::array<unsigned char,80> top{};};
namespace detail {
struct Addresses {std::uintptr_t adapter{},caller_site{},caller_return{},client_table{},scene_table{},scene_render{};};
inline bool addresses(Config c,Addresses& out){Addresses a{};
    if(!d::add(c.base,adapter_rva,a.adapter)||!d::add(c.base,caller_site_rva,a.caller_site)||
       !d::add(c.base,caller_return_rva,a.caller_return)||!d::add(c.base,client_table_rva,a.client_table)||
       !d::add(c.base,scene_table_rva,a.scene_table)||!d::add(c.base,scene_render_rva,a.scene_render))return false;
    out=a;return true;}
}
// Invoke before reserving/installing the detour. Never require original entry
// bytes after installation: a correctly patched adapter would fail that check.
template<class Read> bool validate_adapter(Config c,Read read){detail::Addresses at{};d::Reader<Read> r{read};
    std::array<unsigned char,13> first{},second{};
    return detail::addresses(c,at)&&r.bytes(first.data(),at.adapter,first.size())&&first==adapter_bytes&&
        r.bytes(second.data(),at.adapter,second.size())&&second==first;
}
template<class Read> bool candidate(Config c,Read read,std::uintptr_t client,std::uintptr_t fcanvas,
    std::uintptr_t scene,std::uintptr_t caller_return,Candidate& out){
    detail::Addresses at{};d::Reader<Read> r{read};Candidate result{client,fcanvas,scene};
    if(!client||!fcanvas||!scene||!detail::addresses(c,at)||caller_return!=at.caller_return)return false;
    std::array<unsigned char,6> code{},again{};std::uintptr_t adapter{},render{};
    if(!r.bytes(code.data(),at.caller_site,code.size())||code!=caller_bytes||
       !r.field(result.client_vtable,client,0)||result.client_vtable!=at.client_table||
       !r.field(adapter,result.client_vtable,0x2b8)||adapter!=at.adapter||
       !r.field(result.scene_vtable,scene,0)||result.scene_vtable!=at.scene_table||
       !r.field(render,result.scene_vtable,0x4a8)||render!=at.scene_render)return false;
    std::uintptr_t client_table{},scene_table{},adapter_again{},render_again{};
    if(!r.field(client_table,client,0)||client_table!=result.client_vtable||
       !r.field(adapter_again,client_table,0x2b8)||adapter_again!=adapter||
       !r.field(scene_table,scene,0)||scene_table!=result.scene_vtable||
       !r.field(render_again,scene_table,0x4a8)||render_again!=render||
       !r.bytes(again.data(),at.caller_site,again.size())||again!=code)return false;
    out=result;return true;
}
// Independent optional FCanvas utility. The certified transform descriptor has
// 80-byte stride; +38/+40 is the complete x64 TArray header, not a fixed matrix.
template<class Read> bool matrix_stack(Read read,std::uintptr_t fcanvas,MatrixSnapshot& out){
    d::Reader<Read> r{read};MatrixSnapshot result{};result.fcanvas=fcanvas;
    if(!fcanvas||!r.field(result.stack,fcanvas,0x38)||!d::valid(result.stack,64,80)||!result.stack.count||
       !d::add(result.stack.data,std::size_t(result.stack.count-1)*80,result.top_address)||
       !r.bytes(result.top.data(),result.top_address,result.top.size()))return false;
    d::Array after{};std::array<unsigned char,80> top{};
    if(!r.field(after,fcanvas,0x38)||!d::same(after,result.stack)||
       !r.bytes(top.data(),result.top_address,top.size())||top!=result.top)return false;
    // Close the stack header after the final top read as well.
    if(!r.field(after,fcanvas,0x38)||!d::same(after,result.stack))return false;
    out=result;return true;
}
}
