#pragma once
#include "source_view_shadow.h"

// One unsubmitted renderer. Borrowed canvas/transform and source objects stay
// live throughout the call. Never invoke consume/prepare or enqueue this root.
namespace gears_pair_shadow {
using Construct=void*(__fastcall*)(void*,void*,void*,void*);
using Destroy=void*(__fastcall*)(void*);
struct API {gears_shadow::Allocate allocate;Construct construct;Destroy destroy;gears_shadow::Read read;};
struct Result {unsigned checks{};uintptr_t renderer{};bool retain_inputs{};};
inline Result run(API api,void* family,void* secondary,void* canvas,void* transform) {
    Result result{};
    alignas(8) std::array<unsigned char,0x60> original{},input{},after{};
    if(!api.read(original.data(),family,original.size()))return result;
    auto header=gears_shadow::field<gears_shadow::Header>(original.data(),0);
    uintptr_t primary{};
    if(header.count!=1 || header.capacity<1 || header.capacity>1024 || !header.data ||
       !api.read(&primary,reinterpret_cast<void*>(header.data),8) || !primary ||
       !secondary || primary==reinterpret_cast<uintptr_t>(secondary))return result;
    std::array<std::array<unsigned char,128>,2> matrices{};
    uintptr_t source_states[2]{};
    uintptr_t views[]={primary,reinterpret_cast<uintptr_t>(secondary)};
    for(unsigned i=0;i<2;++i)
        if(!api.read(matrices[i].data(),reinterpret_cast<void*>(views[i]+0x70),128) ||
           !api.read(source_states+i,reinterpret_cast<void*>(views[i]+8),8))return result;
    if(!source_states[0] || !source_states[1] || source_states[0]==source_states[1])return result;
    result.checks|=1;
    input=original;
    gears_shadow::Header pair{reinterpret_cast<uintptr_t>(views),2,2};memcpy(input.data(),&pair,sizeof(pair));
    void* root=api.allocate(0x140,8);
    if(!root)return result;
    result.renderer=reinterpret_cast<uintptr_t>(root);result.checks|=2;
    // If construction does not produce a structurally recognized owned object,
    // quarantine the root and its inputs rather than destroying unknown aliases.
    result.retain_inputs=true;
    void* returned=api.construct(root,input.data(),canvas,transform);
    if(returned==root)result.checks|=4;
    std::array<unsigned char,0x140> built{};
    bool safe=returned==root && api.read(built.data(),root,built.size());
    if(safe) {
        auto copied=gears_shadow::field<gears_shadow::Header>(built.data(),8);
        auto storage=gears_shadow::field<gears_shadow::Header>(built.data(),0x68);
        uintptr_t owned[2]{};
        safe=copied.count==2 && copied.capacity>=2 && copied.capacity<=1024 && copied.data &&
             copied.data!=pair.data && copied.data!=header.data && storage.count==2 &&
             storage.capacity>=2 && storage.capacity<=1024 && storage.data &&
             storage.data!=primary && storage.data!=views[1] &&
             api.read(owned,reinterpret_cast<void*>(copied.data),sizeof(owned));
        if(safe)result.checks|=8;
        bool links=safe,matched=safe;
        for(unsigned i=0;safe && i<2;++i) {
            const uintptr_t expected=storage.data+uintptr_t(i)*0xd30;
            uintptr_t owner{},state{};unsigned index{};
            std::array<unsigned char,128> actual{};
            bool valid=owned[i]==expected &&
                api.read(&owner,reinterpret_cast<void*>(owned[i]),8) &&
                api.read(&state,reinterpret_cast<void*>(owned[i]+8),8) &&
                api.read(&index,reinterpret_cast<void*>(owned[i]+0xd24),4) &&
                api.read(actual.data(),reinterpret_cast<void*>(owned[i]+0x70),128);
            if(!valid){safe=false;links=false;matched=false;break;}
            if(owner!=result.renderer+8 || state!=source_states[i] || index!=i)links=false;
            if(actual!=matrices[i])matched=false;
            for(unsigned j=0;j<32;++j)if(!std::isfinite(gears_shadow::field<float>(actual.data(),4*j)))matched=false;
        }
        if(links)result.checks|=16;
        if(matched)result.checks|=32;
        // A numeric mismatch alone still uses the recognized native cleanup.
        if(safe && links){api.destroy(root);result.checks|=64;result.retain_inputs=false;}
    }
    if(api.read(after.data(),family,after.size()) && after==original)result.checks|=128;
    return result;
}
}
