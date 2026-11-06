#pragma once
#include <array>
#include <cstdint>

namespace gears_lancer_instance_factory {
// Verified retail DrawDynamicElements compares GPU.vtable[5] with GetVF and
// inlines the original implementation when equal. Patching GetVF's entry
// cannot intercept that path. An INSTANCE table with a different slot5 routes
// through the existing wrapper; all other slots and shared tables stay intact.
inline constexpr std::uint32_t table_rva=0x133a318;
inline constexpr std::array<std::uint32_t,13> slot_rvas{{
    0x154230,0x15424c,0x154484,0x282798,0xdfb50,0x35b9a0,0xcfa300,
    0x19f860,0x13b14c,0x13b160,0x1ea9e0,0xc3f20,0xc3f20}};
inline constexpr unsigned primary_slot=5,secondary_slot=6;
class Lease {
    alignas(8) std::array<std::uintptr_t,14> private_{};
    std::uintptr_t gpu_{},original_{};
    bool active_{};
public:
    Lease()=default;
    Lease(Lease const&)=delete;Lease& operator=(Lease const&)=delete;
    bool active() const noexcept{return active_;}
    std::uintptr_t gpu() const noexcept{return gpu_;}
    std::uintptr_t table() const noexcept{return reinterpret_cast<std::uintptr_t>(private_.data()+1);}
    std::array<std::uintptr_t,14> const& entries() const noexcept{return private_;}
    template<class Read,class Compare>
    bool install(std::uintptr_t base,std::uintptr_t gpu,std::uintptr_t primary,std::uintptr_t secondary,
        Read&& read,Compare&& compare) noexcept {
        if(!base||!gpu||(gpu&7)||!primary||!secondary||primary==secondary||
           primary==base+slot_rvas[primary_slot]||secondary==base+slot_rvas[secondary_slot])return false;
        std::uintptr_t actual{};
        if(active_)return gpu==gpu_&&read(gpu,actual)&&actual==table();
        if(!read(gpu,actual)||actual!=base+table_rva)return false;
        std::array<std::uintptr_t,14> copied{},again{};
        for(unsigned i=0;i<copied.size();++i)
            if(!read(actual+(std::intptr_t(i)-1)*sizeof(std::uintptr_t),copied[i]))return false;
        for(unsigned i=0;i<slot_rvas.size();++i)if(copied[i+1]!=base+slot_rvas[i])return false;
        for(unsigned i=0;i<again.size();++i)
            if(!read(actual+(std::intptr_t(i)-1)*sizeof(std::uintptr_t),again[i])||again[i]!=copied[i])return false;
        if(!read(gpu,actual)||actual!=base+table_rva)return false;
        private_=copied;private_[primary_slot+1]=primary;private_[secondary_slot+1]=secondary;
        // Adopt before dispatch. A fault/ambiguous CAS keeps this table and
        // native owner pinned until restoration is positively established.
        gpu_=gpu;original_=actual;active_=true;bool exchanged{};
        if(!compare(gpu,original_,table(),exchanged))return false;
        if(!exchanged){active_=false;gpu_=original_=0;return false;}
        return read(gpu,actual)&&actual==table();
    }
    // Caller must first prove actual no-morph upload and FIFO retirement.
    // Compare only the pointer we wrote. A foreign replacement is never set.
    template<class Read,class Compare>
    bool restore(Read&& read,Compare&& compare) noexcept {
        if(!active_)return true;std::uintptr_t actual{};
        if(!read(gpu_,actual))return false;
        if(actual==table()){
            bool exchanged{};
            if(!compare(gpu_,table(),original_,exchanged)||!exchanged||
               !read(gpu_,actual)||actual!=original_)return false;
        }
        active_=false;gpu_=original_=0;return true;
    }
};
}
