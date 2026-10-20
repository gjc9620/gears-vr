#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>

// Independent policy only: no hooks, native memory writer, or enabled default.
// Evidence: analysis/graphics-lock/{static-dlss-findings.json,
// startup-menu1-34900.json,lock-parameter-thread-ida.json}.
namespace gears_graphics_lock {
inline constexpr std::uintptr_t singleton_rva=0x1da57f0,settings_vtable_rva=0x14dd920;
inline constexpr std::uintptr_t menu_rva=0x280da4,collector_rva=0x2ac894,commit_rva=0xff6f4;
inline constexpr std::uintptr_t command_vtable_rva=0x1343588;
inline constexpr std::size_t normal_dlss_offset=0x3c8,alternate_dlss_offset=0x56c;
inline constexpr std::size_t command_dlss_offset=0x78,command_size=0xc0;
inline constexpr std::int32_t dlss_menu_mode=3;
inline constexpr std::uint32_t menu_success_eax=1,commit_stride_eax=0xc0;
inline constexpr char game_sha256[]="bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4";
inline constexpr char runtime_code_sha256[]="911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6";
struct Entry {std::uintptr_t rva;std::array<unsigned char,32> bytes;};
inline constexpr std::array<Entry,3> entries{{
    {menu_rva,{0x33,0xc0,0x41,0xb8,0x01,0x00,0x00,0x00,0x44,0x89,0x81,0x14,0x03,0x00,0x00,0x89,
               0x81,0xd4,0x02,0x00,0x00,0x89,0x81,0x80,0x03,0x00,0x00,0x48,0x89,0x81,0xc8,0x03}},
    {collector_rva,{0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x70,0x18,0x48,0x89,0x78,0x20,0x55,
                    0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8d,0xa8,0xf8,0xfe,0xff,0xff,0x48}},
    {commit_rva,{0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x68,0x18,0x48,0x89,0x70,0x20,0x57,
                 0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x30,0x8b,0x59,0x0c,0x48}}
}};

// Windows x64 register facts, not claimed recovered C++ prototypes:
// menu: RCX=settings, EDX=mode, EAX=1 on the successful setter paths.
// collector: RCX=settings (both retained callers pass singleton getter RAX);
// its callers do not consume a return value. Commit: RCX=command, EAX=192.
// invoke_* below preserve the integrator's original callable return type/value.
enum class Reason {disabled,applied,other_mode,identity_unverified,address_overflow,
    read_failed,entry_mismatch,singleton_mismatch,vtable_mismatch,thread_unverified,
    thread_mismatch,write_unauthorized,command_layout_unverified,command_identity,
    invalid_boolean,compare_exchange_failed,verification_failed};
struct Result {
    Reason reason{Reason::disabled};bool modified{},bypass_original{};
    bool protected_now()const{return reason==Reason::applied;}
    bool fault()const{return reason!=Reason::disabled&&reason!=Reason::applied&&reason!=Reason::other_mode;}
};
struct Admission {
    Reason reason{Reason::identity_unverified};std::uintptr_t base{},singleton{},vtable{},command_vtable{};
    bool valid()const{return reason==Reason::applied;}
};
struct Context {
    bool enabled{};Admission admission{};
    // Must come from actual game hook observations; launcher PID/thread is not evidence.
    std::uint32_t owner_thread{},consumer_thread{};
    bool owner_thread_verified{},consumer_thread_verified{};
    // Reader/writer safety and command layout must be independently admitted.
    bool normal_write_authorized{},command_write_authorized{},command_layout_verified{};
};
inline bool add(std::uintptr_t base,std::size_t offset,std::uintptr_t& result){
    if(!base||offset>std::numeric_limits<std::uintptr_t>::max()-base)return false;
    result=base+offset;return true;
}
template<class Reader,class T> bool read_value(Reader& read,std::uintptr_t address,T& value){
    return read(address,&value,sizeof(value));
}
// Run before enabling/changing entry bytes. executable_identity_verified is an
// external SHA256/module identity proof, never inferred from these 32-byte prefixes.
template<class Reader> Admission admit(std::uintptr_t base,bool executable_identity_verified,Reader& read){
    Admission a{};a.base=base;
    if(!executable_identity_verified||sizeof(std::uintptr_t)!=8)return a;
    if(!add(base,singleton_rva,a.singleton)||!add(base,settings_vtable_rva,a.vtable)||
       !add(base,command_vtable_rva,a.command_vtable)){a.reason=Reason::address_overflow;return a;}
    for(const auto& entry:entries){
        std::uintptr_t address{};if(!add(base,entry.rva,address)){a.reason=Reason::address_overflow;return a;}
        std::array<unsigned char,32> bytes{};
        if(!read(address,bytes.data(),bytes.size())){a.reason=Reason::read_failed;return a;}
        if(bytes!=entry.bytes){a.reason=Reason::entry_mismatch;return a;}
    }
    std::uintptr_t value{};
    if(!read_value(read,a.singleton,value)){a.reason=Reason::read_failed;return a;}
    if(value!=a.vtable){a.reason=Reason::vtable_mismatch;return a;}
    constexpr std::array<std::pair<std::size_t,std::uintptr_t>,3> slots{{{0x18,0x26a9dc},{0x20,0x269168},{0x190,menu_rva}}};
    for(const auto& slot:slots){
        std::uintptr_t address{},expected{};
        if(!add(a.vtable,slot.first,address)||!add(base,slot.second,expected)){a.reason=Reason::address_overflow;return a;}
        if(!read_value(read,address,value)){a.reason=Reason::read_failed;return a;}
        if(value!=expected){a.reason=Reason::vtable_mismatch;return a;}
    }
    std::uintptr_t address{},expected{};
    if(!add(a.command_vtable,8,address)||!add(base,commit_rva,expected)){a.reason=Reason::address_overflow;return a;}
    if(!read_value(read,address,value)){a.reason=Reason::read_failed;return a;}
    if(value!=expected){a.reason=Reason::command_identity;return a;}
    a.reason=Reason::applied;return a;
}
template<class Reader> Result settings_gate(const Context& c,std::uintptr_t settings,std::uint32_t thread,Reader& read){
    if(!c.enabled)return {};
    if(!c.admission.valid())return {c.admission.reason};
    if(settings!=c.admission.singleton)return {Reason::singleton_mismatch};
    if(!c.owner_thread_verified||!c.owner_thread||!thread)return {Reason::thread_unverified};
    if(thread!=c.owner_thread)return {Reason::thread_mismatch};
    std::uintptr_t value{};
    if(!read_value(read,settings,value))return {Reason::read_failed};
    if(value!=c.admission.vtable)return {Reason::vtable_mismatch};
    return {Reason::applied};
}
template<class Reader> Result menu(const Context& c,std::uintptr_t settings,std::int32_t mode,std::uint32_t thread,Reader& read){
    if(!c.enabled)return {};
    if(mode!=dlss_menu_mode)return {Reason::other_mode};
    auto result=settings_gate(c,settings,thread,read);
    if(result.protected_now())result.bypass_original=true;
    return result;
}
// CAS contract: an authorized, naturally aligned, all-or-nothing 32-bit
// compare-exchange at exactly address. It must not change protection or write
// adjacent bytes. No implementation is provided or assumed by this policy.
template<class Reader,class CompareExchange> Result clear_boolean(std::uintptr_t address,Reader& read,CompareExchange& cas){
    if(address%alignof(std::uint32_t))return {Reason::address_overflow};
    std::uint32_t value{};
    if(!read_value(read,address,value))return {Reason::read_failed};
    if(value>1)return {Reason::invalid_boolean};
    if(!value)return {Reason::applied};
    if(!cas(address,value,std::uint32_t{0}))return {Reason::compare_exchange_failed};
    Result result{Reason::applied,true};
    if(!read_value(read,address,value)||value!=0)result.reason=Reason::verification_failed;
    return result;
}
template<class Reader,class CompareExchange> Result collector_before(const Context& c,std::uintptr_t settings,
    std::uint32_t thread,Reader& read,CompareExchange& cas){
    auto result=settings_gate(c,settings,thread,read);
    if(!result.protected_now())return result;
    if(!c.normal_write_authorized)return {Reason::write_unauthorized};
    std::uintptr_t address{};if(!add(settings,normal_dlss_offset,address))return {Reason::address_overflow};
    return clear_boolean(address,read,cas);
}
template<class Reader,class CompareExchange> Result commit_before(const Context& c,std::uintptr_t command,
    std::uint32_t thread,Reader& read,CompareExchange& cas){
    if(!c.enabled)return {};
    if(!c.admission.valid())return {c.admission.reason};
    if(!c.consumer_thread_verified||!c.consumer_thread||!thread)return {Reason::thread_unverified};
    if(thread!=c.consumer_thread)return {Reason::thread_mismatch};
    if(!c.command_layout_verified)return {Reason::command_layout_unverified};
    if(!c.command_write_authorized)return {Reason::write_unauthorized};
    if(!command||command%alignof(std::uintptr_t))return {Reason::command_identity};
    std::uintptr_t vtable{},address{},end{};
    if(!add(command,command_size-1,end)||!add(command,command_dlss_offset,address))return {Reason::address_overflow};
    if(!read_value(read,command,vtable))return {Reason::read_failed};
    if(vtable!=c.admission.command_vtable)return {Reason::command_identity};
    // Preserve original command address/lifetime/opaque payload. A local copy
    // would require extra proof of address independence and non-retention.
    return clear_boolean(address,read,cas);
}
template<class Reader,class Original> auto invoke_menu(const Context& c,std::uintptr_t settings,std::int32_t mode,
    std::uint32_t thread,Reader& read,Original&& original,Result& result)
    -> decltype(original(settings,mode)){
    using Return=decltype(original(settings,mode));
    static_assert(std::is_integral<Return>::value,"Menu callable must preserve the observed integer success ABI");
    result=menu(c,settings,mode,thread,read);
    if(result.bypass_original)return static_cast<Return>(menu_success_eax);
    return std::forward<Original>(original)(settings,mode);
}
template<class Reader,class CompareExchange,class Original> decltype(auto) invoke_collector(const Context& c,
    std::uintptr_t settings,std::uint32_t thread,Reader& read,CompareExchange& cas,Original&& original,Result& result){
    result=collector_before(c,settings,thread,read,cas);
    return std::forward<Original>(original)(settings);
}
template<class Reader,class CompareExchange,class Original> decltype(auto) invoke_commit(const Context& c,
    std::uintptr_t command,std::uint32_t thread,Reader& read,CompareExchange& cas,Original&& original,Result& result){
    result=commit_before(c,command,thread,read,cas);
    return std::forward<Original>(original)(command);
}
// Failures deliberately preserve ordinary engine invocation. Integration must
// record result.fault(), withdraw the release lock claim, and stop VR safely;
// forwarding alone must never be reported as an effective runtime lock.
}
