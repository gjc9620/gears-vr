#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <cmath>

// This experiment owns only an unsubmitted 0x430 source view. It never changes
// a family pointer/count or calls the 0xD30 rendered-view destructor.
namespace gears_shadow {
constexpr size_t view_size=0x430;
using Allocate=void*(__fastcall*)(unsigned,unsigned);
using Copy=void*(__fastcall*)(void*,void*);
using Destroy=void(__fastcall*)(void*);
using Free=void(__fastcall*)(void*,unsigned);
using Read=bool(*)(void*,const void*,size_t);
using Probe=bool(*)(void*,void*);
struct API {Allocate allocate;Copy copy;Destroy destroy;Free free;Read read;};
struct Header {uintptr_t data;int count,capacity;};
static_assert(sizeof(Header)==16);
struct Result {
    unsigned stage{},checks{},nonempty_arrays{};
    uintptr_t source{},temporary{};
};
template<class T> T field(const unsigned char* bytes,size_t offset) {
    T value{};memcpy(&value,bytes+offset,sizeof(value));return value;
}
inline Result run(API api,void* family,void* source,uintptr_t expected_state=0,
                  const void* expected_matrices=nullptr,bool shifted_planes=false,Probe probe=nullptr) {
    Result r{};r.source=reinterpret_cast<uintptr_t>(source);
    std::array<unsigned char,view_size> before{},copied{},after{};
    Header original{};
    if(!api.read(&original,family,sizeof(original)) || original.count!=1 ||
       original.capacity<1 || original.capacity>1024 || !original.data ||
       !api.read(before.data(),source,before.size()) ||
       field<uintptr_t>(before.data(),0)!=reinterpret_cast<uintptr_t>(family)) {r.stage=1;return r;}
    uintptr_t first{};
    if(!api.read(&first,reinterpret_cast<void*>(original.data),8) || first!=r.source) {r.stage=1;return r;}
    for(unsigned i=0;i<32;++i)if(!std::isfinite(field<float>(before.data(),0x70+4*i))){r.stage=2;return r;}
    // The linked map copy path has not been independently exercised yet.
    // Reject a nonempty map rather than silently treating it as an array.
    if(field<uintptr_t>(before.data(),0x140)) {r.stage=3;return r;}
    constexpr size_t offsets[]={0x380,0x390,0x3ec,0x400};
    constexpr size_t strides[]={16,16,8,8};
    for(unsigned i=0;i<4;++i) {
        Header h=field<Header>(before.data(),offsets[i]);
        if(h.count<0 || h.count>256 || h.capacity<h.count || h.capacity>65536 ||
           (h.count && !h.data)) {r.stage=4;return r;}
    }
    void* destination=api.allocate(unsigned(view_size),8);
    if(!destination){r.stage=5;return r;}
    r.temporary=reinterpret_cast<uintptr_t>(destination);r.checks|=1;
    // This is the version-gated native COPY CONSTRUCTOR, not memcpy/assignment.
    void* returned=api.copy(destination,source);r.checks|=2;
    bool cleanup_safe=returned==destination && api.read(copied.data(),destination,copied.size());
    if(cleanup_safe) {
        if(!memcmp(expected_matrices?expected_matrices:before.data()+0x70,copied.data()+0x70,128))r.checks|=4;
        const uintptr_t state=expected_state?expected_state:field<uintptr_t>(before.data(),8);
        if(field<uintptr_t>(before.data(),0)==field<uintptr_t>(copied.data(),0) &&
           field<uintptr_t>(copied.data(),8)==state &&
           field<uintptr_t>(before.data(),0x418)==field<uintptr_t>(copied.data(),0x418))r.checks|=8;
        if(field<uintptr_t>(before.data(),0x418)!=field<uintptr_t>(copied.data(),0x418))cleanup_safe=false;
        uintptr_t source_buckets=field<uintptr_t>(before.data(),0x14c);
        uintptr_t copy_buckets=field<uintptr_t>(copied.data(),0x14c);
        if(copy_buckets && source_buckets==copy_buckets)cleanup_safe=false;
        bool independent=true;
        std::array<unsigned char,4096> a{},b{};
        for(unsigned i=0;i<4;++i) {
            Header s=field<Header>(before.data(),offsets[i]);
            Header d=field<Header>(copied.data(),offsets[i]);
            if(d.count!=s.count || d.capacity<d.count || d.capacity>65536 ||
               (d.data && d.data==s.data))independent=false;
            if(d.count<0 || d.count>256 || d.capacity<d.count || d.capacity>65536 ||
               (d.count && !d.data) || (d.data && d.data==s.data))cleanup_safe=false;
            if(s.count) {
                ++r.nonempty_arrays;
                size_t size=size_t(s.count)*strides[i];
                if(!d.data || !api.read(a.data(),reinterpret_cast<void*>(s.data),size) ||
                   !api.read(b.data(),reinterpret_cast<void*>(d.data),size))independent=false;
                else if(shifted_planes && i<2) {
                    // A translated view has different geometry coefficients.
                    // Check finite four-float elements, not original equality.
                    for(size_t j=0;j<size;j+=4)if(!std::isfinite(field<float>(b.data(),j)))independent=false;
                } else if(memcmp(a.data(),b.data(),size))independent=false;
            }
        }
        if(independent)r.checks|=16;
    }
    // A numeric/content mismatch can be cleaned up. An unexpected owned alias
    // cannot: destroying it could free the original view's storage. Quarantine
    // that one allocation until process exit, report failure and never retry.
    // Never read destination after free: pooled pages may remain readable.
    if(cleanup_safe && probe && (r.checks&31)==31)cleanup_safe=probe(family,destination);
    if(cleanup_safe){api.destroy(destination);api.free(destination,unsigned(view_size));r.checks|=32;}
    if(api.read(after.data(),source,after.size()) && before==after)r.checks|=64;
    Header final{};uintptr_t final_first{};
    if(api.read(&final,family,sizeof(final)) && !memcmp(&original,&final,sizeof(final)) &&
       api.read(&final_first,reinterpret_cast<void*>(final.data),8) && final_first==first)r.checks|=128;
    r.stage=r.checks==255?6:7;return r;
}
}
