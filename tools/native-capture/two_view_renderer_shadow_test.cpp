#include <windows.h>
#include <cstdio>
#include "two_view_renderer_shadow.h"
namespace {
unsigned constructed{},destroyed{};bool fail_allocate{},bad_matrix{},alias_array{},bad_index{};
void* allocated_array{};void* allocated_storage{};void* allocated_root{};
bool read(void* out,const void* from,size_t size) {
    SIZE_T count{};return from && ReadProcessMemory(GetCurrentProcess(),from,out,size,&count) && count==size;
}
void* __fastcall allocate(unsigned size,unsigned alignment) {
    if(size!=0x140 || alignment!=8 || fail_allocate)return nullptr;
    allocated_root=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,size);return allocated_root;
}
void* __fastcall construct(void* root,void* family,void* canvas,void* transform) {
    if(canvas!=reinterpret_cast<void*>(0x1111) || transform!=reinterpret_cast<void*>(0x2222))ExitProcess(80);
    ++constructed;
    auto input=gears_shadow::field<gears_shadow::Header>(static_cast<unsigned char*>(family),0);
    if(input.count!=2 || input.capacity!=2)ExitProcess(81);
    uintptr_t sources[2]{};memcpy(sources,reinterpret_cast<void*>(input.data),sizeof(sources));
    allocated_storage=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,2*0xd30);
    allocated_array=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,16);
    if(!allocated_storage || !allocated_array)ExitProcess(82);
    uintptr_t pointers[2]{};
    for(unsigned i=0;i<2;++i) {
        auto view=static_cast<unsigned char*>(allocated_storage)+i*0xd30;
        pointers[i]=reinterpret_cast<uintptr_t>(view);
        uintptr_t owner=reinterpret_cast<uintptr_t>(root)+8;
        memcpy(view,&owner,8);memcpy(view+8,reinterpret_cast<void*>(sources[i]+8),8);
        memcpy(view+0x70,reinterpret_cast<void*>(sources[i]+0x70),128);
        unsigned index=bad_index?9:i;memcpy(view+0xd24,&index,4);
    }
    memcpy(allocated_array,pointers,sizeof(pointers));
    gears_shadow::Header copied{alias_array?input.data:reinterpret_cast<uintptr_t>(allocated_array),2,2};
    gears_shadow::Header storage{reinterpret_cast<uintptr_t>(allocated_storage),2,2};
    memcpy(static_cast<unsigned char*>(root)+8,&copied,sizeof(copied));
    memcpy(static_cast<unsigned char*>(root)+0x68,&storage,sizeof(storage));
    if(bad_matrix){float value=42;memcpy(static_cast<unsigned char*>(allocated_storage)+0x70,&value,4);}
    return root;
}
void* __fastcall destroy(void* root) {
    ++destroyed;HeapFree(GetProcessHeap(),0,allocated_array);HeapFree(GetProcessHeap(),0,allocated_storage);
    HeapFree(GetProcessHeap(),0,root);allocated_array=allocated_storage=allocated_root=nullptr;return root;
}
}
int main() {
    alignas(8) unsigned char family[0x60]{},primary[0x430]{},secondary[0x430]{};
    uintptr_t first=reinterpret_cast<uintptr_t>(primary);
    gears_shadow::Header header{reinterpret_cast<uintptr_t>(&first),1,17};memcpy(family,&header,sizeof(header));
    uintptr_t a=0x1000,b=0x2000;memcpy(primary+8,&a,8);memcpy(secondary+8,&b,8);
    for(unsigned i=0;i<32;++i){float value=float(i)+.5f;memcpy(primary+0x70+4*i,&value,4);value+=1;memcpy(secondary+0x70+4*i,&value,4);}
    gears_pair_shadow::API api{allocate,construct,destroy,read};
    auto run=[&](){return gears_pair_shadow::run(api,family,secondary,reinterpret_cast<void*>(0x1111),reinterpret_cast<void*>(0x2222));};
    auto result=run();
    if(result.checks!=255 || result.retain_inputs || constructed!=1 || destroyed!=1)return 1;
    bad_matrix=true;result=run();bad_matrix=false;
    if(result.checks==255 || (result.checks&32) || !(result.checks&64) || result.retain_inputs || destroyed!=2)return 2;
    alias_array=true;result=run();alias_array=false;
    if(!result.retain_inputs || (result.checks&64) || destroyed!=2)return 3;
    // Explicit mock ownership allows the harness to reclaim quarantine. The
    // native experiment does not attempt this on structurally unknown objects.
    destroy(allocated_root);
    bad_index=true;result=run();bad_index=false;
    if(!result.retain_inputs || (result.checks&16) || (result.checks&64))return 4;
    destroy(allocated_root);
    fail_allocate=true;result=run();fail_allocate=false;
    if(result.checks!=1 || result.retain_inputs)return 5;
    memcpy(secondary+8,&a,8);result=run();
    if(result.checks || result.renderer || result.retain_inputs)return 6;
    printf("Two-view renderer: two owned copies, state/index/owner matching, native cleanup and quarantine guards passed\n");return 0;
}
