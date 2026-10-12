#include <windows.h>
#include <cstdio>
#include "source_view_shadow.h"
namespace {
unsigned allocated{},copied{},destroyed{},freed{},references=1;
bool fail_allocate{},corrupt_matrix{},shallow_array{};
bool altered_plane{},nonfinite_plane{};
void* freed_address{};
uintptr_t replacement_state{};
bool read(void* out,const void* from,size_t size) {
    if(from==freed_address)return false;
    SIZE_T got{};return from && ReadProcessMemory(GetCurrentProcess(),from,out,size,&got) && got==size;
}
void* __fastcall allocate(unsigned size,unsigned align) {
    if(size!=0x430 || align!=8 || fail_allocate)return nullptr;
    ++allocated;freed_address=nullptr;return HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,size);
}
void* __fastcall copy(void* out,void* source) {
    ++copied;memcpy(out,source,0x430);
    if(replacement_state)memcpy(static_cast<unsigned char*>(out)+8,&replacement_state,8);
    constexpr size_t offsets[]={0x380,0x390,0x3ec,0x400};
    constexpr size_t strides[]={16,16,8,8};
    for(unsigned i=0;i<4;++i) {
        auto h=gears_shadow::field<gears_shadow::Header>(static_cast<unsigned char*>(source),offsets[i]);
        if(h.count && !shallow_array) {
            void* elements=HeapAlloc(GetProcessHeap(),0,size_t(h.count)*strides[i]);
            if(!elements)ExitProcess(90);
            memcpy(elements,reinterpret_cast<void*>(h.data),size_t(h.count)*strides[i]);
            if(i==0 && (altered_plane || nonfinite_plane)) {
                float coefficient=nonfinite_plane?NAN:2.f;memcpy(elements,&coefficient,4);
            }
            h.data=reinterpret_cast<uintptr_t>(elements);h.capacity=h.count;
            memcpy(static_cast<unsigned char*>(out)+offsets[i],&h,sizeof(h));
        }
    }
    if(corrupt_matrix) {float value=42;memcpy(static_cast<unsigned char*>(out)+0x70,&value,4);}
    ++references;return out;
}
void __fastcall destroy(void* view) {
    ++destroyed;--references;
    if(!shallow_array)for(size_t offset:{size_t(0x380),size_t(0x390),size_t(0x3ec),size_t(0x400)}) {
        auto h=gears_shadow::field<gears_shadow::Header>(static_cast<unsigned char*>(view),offset);
        if(h.data)HeapFree(GetProcessHeap(),0,reinterpret_cast<void*>(h.data));
    }
}
void __fastcall free_view(void* view,unsigned size) {
    if(size!=0x430)ExitProcess(91);
    ++freed;freed_address=view;HeapFree(GetProcessHeap(),0,view);
}
}
int main() {
    alignas(8) unsigned char source[0x430]{};
    uintptr_t first=reinterpret_cast<uintptr_t>(source);
    gears_shadow::Header family{reinterpret_cast<uintptr_t>(&first),1,17};
    uintptr_t owner=reinterpret_cast<uintptr_t>(&family);memcpy(source,&owner,8);
    uintptr_t shared=0x12345;memcpy(source+0x418,&shared,8);
    for(unsigned i=0;i<32;++i){float f=float(i)+.5f;memcpy(source+0x70+4*i,&f,4);}
    unsigned char elements[32];for(unsigned i=0;i<32;++i)elements[i]=static_cast<unsigned char>(i);
    for(size_t offset:{size_t(0x380),size_t(0x390),size_t(0x3ec),size_t(0x400)}) {
        gears_shadow::Header h{reinterpret_cast<uintptr_t>(elements),2,4};memcpy(source+offset,&h,sizeof(h));
    }
    gears_shadow::API api{allocate,copy,destroy,free_view,read};
    auto result=gears_shadow::run(api,&family,source);
    if(result.stage!=6 || result.checks!=255 || result.nonempty_arrays!=4 ||
       allocated!=1 || copied!=1 || destroyed!=1 || freed!=1 || references!=1)return 1;
    // A bad comparison still has to release the constructed object's resources.
    corrupt_matrix=true;result=gears_shadow::run(api,&family,source);corrupt_matrix=false;
    if(result.stage!=7 || (result.checks&4) || !(result.checks&32) || references!=1 || freed!=2)return 2;
    shallow_array=true;result=gears_shadow::run(api,&family,source);shallow_array=false;
    if(result.stage!=7 || (result.checks&16) || (result.checks&32) || references!=2 || freed!=2)return 3;
    // The guard deliberately quarantines aliases. The mock knows exactly what
    // it shared, so the test harness can reclaim this rejected allocation.
    shallow_array=true;destroy(reinterpret_cast<void*>(result.temporary));
    free_view(reinterpret_cast<void*>(result.temporary),0x430);shallow_array=false;
    fail_allocate=true;result=gears_shadow::run(api,&family,source);fail_allocate=false;
    if(result.stage!=5 || copied!=3 || destroyed!=3 || freed!=3)return 4;
    family.count=2;result=gears_shadow::run(api,&family,source);family.count=1;
    if(result.stage!=1 || allocated!=3)return 5;
    uintptr_t unsupported=1;memcpy(source+0x140,&unsupported,8);
    result=gears_shadow::run(api,&family,source);
    if(result.stage!=3 || allocated!=3)return 6;
    unsupported=0;memcpy(source+0x140,&unsupported,8);
    gears_shadow::Header invalid{1,257,257};memcpy(source+0x380,&invalid,sizeof(invalid));
    result=gears_shadow::run(api,&family,source);
    if(result.stage!=4 || allocated!=3)return 7;
    gears_shadow::Header valid{reinterpret_cast<uintptr_t>(elements),2,4};memcpy(source+0x380,&valid,sizeof(valid));
    replacement_state=0x3000;
    result=gears_shadow::run(api,&family,source,replacement_state);
    if(result.stage!=6 || result.checks!=255 || references!=1 || freed!=4 || gears_shadow::field<uintptr_t>(source,8)!=0)return 8;
    // A replacement pointer is accepted only when explicitly expected.
    result=gears_shadow::run(api,&family,source);
    if(result.stage!=7 || (result.checks&8) || !(result.checks&32) || references!=1 || freed!=5)return 9;
    replacement_state=0;
    float expected_matrices[32];memcpy(expected_matrices,source+0x70,sizeof(expected_matrices));expected_matrices[0]=42;
    corrupt_matrix=true;altered_plane=true;
    result=gears_shadow::run(api,&family,source,0,expected_matrices,true);
    if(result.stage!=6 || result.checks!=255 || references!=1)return 10;
    // Nonfinite shifted geometry and finite-but-unexpected original geometry
    // must both fail, while retaining the normal cleanup guarantee.
    nonfinite_plane=true;
    result=gears_shadow::run(api,&family,source,0,expected_matrices,true);
    if(result.stage!=7 || (result.checks&16) || !(result.checks&32) || references!=1)return 11;
    nonfinite_plane=false;
    result=gears_shadow::run(api,&family,source,0,expected_matrices,false);
    if(result.stage!=7 || (result.checks&16) || !(result.checks&32) || references!=1)return 12;
    printf("Source shadow: deep copy, balanced resources, mismatch cleanup, allocation failure and guards passed\n");return 0;
}
