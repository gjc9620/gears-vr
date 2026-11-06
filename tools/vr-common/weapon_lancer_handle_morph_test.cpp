#include "weapon_lancer_handle_morph.h"
#include <cstdio>
#include <limits>
namespace m=gears_lancer_handle_morph;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("failed line %d\n",__LINE__);}}while(false)
int main(){
    // The blade's original material has no Morph permutation. Only the owned
    // two-target payload/chunk1 may request a private regular-factory lookup;
    // no live payload/header may change and all other draws remain original.
    std::array<std::uint8_t,92> gpu{};std::array<std::uint8_t,68> payload{};
    std::array<std::uint8_t,152> lod{};std::array<m::Active,2> active{{{0x11000,.2f},{0x12000,.02f}}};
    const auto address=[](auto const& x){return reinterpret_cast<std::uintptr_t>(x.data());};
    m::field(gpu,44,m::Array{address(lod),1,1});m::field(gpu,80,address(payload));
    m::field(payload,40,std::int32_t(0));m::field(payload,44,m::Array{address(active),2,2});m::field(payload,60,std::int32_t(2));
    const auto read=[&](std::uintptr_t at,void* to,std::size_t n){
        const auto copy=[&](auto const& x){const auto first=address(x),bytes=sizeof x;
            if(at<first||at-first>bytes||n>bytes-(at-first))return false;
            std::memcpy(to,reinterpret_cast<void const*>(at),n);return true;};
        return copy(gpu)||copy(payload)||copy(active)||copy(lod);};
    const auto original_gpu=gpu;const auto original_payload=payload;m::RegularLookup scratch{};
    const auto route=[&](int li,int ch,std::uintptr_t owner=0,std::uintptr_t target=0x11000){return m::regular_blade_lookup(read,address(gpu),owner?owner:address(gpu),li,ch,target,0x12000,scratch);};
    C(route(0,1));C(m::field<std::int32_t>(scratch.payload,60)==0);
    C(m::field<std::uintptr_t>(scratch.gpu,80)==address(scratch.payload));C(gpu==original_gpu);C(payload==original_payload);
    C(!route(0,0));C(!route(0,2));C(!route(1,1));C(!route(-1,1));C(!route(0,1,0x101));C(!route(0,1,0,0x13000));
    m::field(payload,60,std::int32_t(0));C(!route(0,1));payload=original_payload;
    m::field(payload,44,m::Array{address(active),3,3});C(!route(0,1));payload=original_payload;
    active[1].weight=std::numeric_limits<float>::quiet_NaN();C(!route(0,1));active[1].weight=.02f;
    unsigned gpu_reads{};
    const auto changed=[&](std::uintptr_t at,void* to,std::size_t n){
        const bool okay=read(at,to,n);
        if(at==address(gpu)&&++gpu_reads==2&&okay)static_cast<std::uint8_t*>(to)[36]^=1;
        return okay;};
    C(!m::regular_blade_lookup(changed,address(gpu),address(gpu),0,1,0x11000,0x12000,scratch));
    C(gpu==original_gpu);C(payload==original_payload);
    {double observed=-1;active={{{0x11000,0.f},{0x12000,0.f}}};
        C(m::installed_distance(read,address(gpu),address(payload),0x11000,0x12000,observed)&&observed==0);
        // Re-reading a static payload changes no input bytes/serial authority.
        const auto saved_gpu=gpu;const auto saved_payload=payload;
        for(unsigned i=0;i<8;++i)C(m::installed_distance(read,address(gpu),address(payload),0x11000,0x12000,observed)&&observed==0);
        C(gpu==saved_gpu&&payload==saved_payload);
        active[0].weight=active[1].weight=.5f;
        C(m::installed_distance(read,address(gpu),address(payload),0x11000,0x12000,observed)&&observed==gears_weapon::lancer_front::travel_raw);
        active[1].target=0x13000;C(!m::installed_distance(read,address(gpu),address(payload),0x11000,0x12000,observed));
        active[1]={0x12000,std::numeric_limits<float>::quiet_NaN()};C(!m::installed_distance(read,address(gpu),address(payload),0x11000,0x12000,observed));
        active[1]={0x12000,.5f};
        m::field(payload,44,m::Array{});C(m::installed_distance(read,address(gpu),address(payload),0x11000,0x12000,observed)&&observed==0);
        unsigned reads{};const auto replaced=[&](std::uintptr_t at,void* out,std::size_t n){
            const bool okay=read(at,out,n);
            if(okay&&at==address(gpu)+80&&++reads==2){std::uintptr_t foreign=0x98700;std::memcpy(out,&foreign,8);}return okay;};
        C(!m::installed_distance(replaced,address(gpu),address(payload),0x11000,0x12000,observed));
        payload=saved_payload;active={{{0x11000,.2f},{0x12000,.02f}}};}
    std::printf("lancer_handle_morph: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
