#include <windows.h>
#include <cstdio>
#include "split_source_views.h"
namespace {
unsigned allocations{},constructs{},destroys{},frees{},allocation_limit=100;
bool bad_matrix{},alias_array{},fail_constructor{};
uintptr_t aliased{};
void* allocated[2]{};
bool read(void* out,const void* from,size_t size){SIZE_T count{};return from && ReadProcessMemory(GetCurrentProcess(),from,out,size,&count) && count==size;}
void* __fastcall allocate(unsigned size,unsigned alignment) {
    if(size!=0x430 || alignment!=8 || allocations>=allocation_limit)return nullptr;
    void* view=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,size);allocated[allocations%2]=view;++allocations;return view;
}
void __fastcall destroy(void*){++destroys;}
void __fastcall free_view(void* view,unsigned size){if(size!=0x430)ExitProcess(80);++frees;HeapFree(GetProcessHeap(),0,view);}
void* __fastcall construct(void* destination,void* owner,void* state,void*,void*,void*,void*,void*,void*,void*,
    float x,float y,float width,float height,const void* view,const void* projection,
    const void*,const void*,const void*,const void*,float,unsigned) {
    ++constructs;if(fail_constructor && constructs%2==0)return nullptr;
    auto bytes=static_cast<unsigned char*>(destination);
    memcpy(bytes,&owner,8);memcpy(bytes+8,&state,8);
    float rectangle[]={x,y,width,height};memcpy(bytes+0x48,rectangle,16);
    memcpy(bytes+0x70,view,64);memcpy(bytes+0xb0,projection,64);
    if(bad_matrix){float value=42;memcpy(bytes+0x70,&value,4);}
    if(alias_array){gears_shadow::Header header{aliased,1,1};memcpy(bytes+0x380,&header,sizeof(header));}
    return destination;
}
}
int main() {
    alignas(8) unsigned char family[0x60]{},source[0x430]{};unsigned char planes[16]{};
    uintptr_t owner=reinterpret_cast<uintptr_t>(family),state=0x1000;aliased=reinterpret_cast<uintptr_t>(planes);
    memcpy(source,&owner,8);memcpy(source+8,&state,8);
    float rectangle[]={0,0,1920,1080};memcpy(source+0x48,rectangle,16);
    float matrix[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    memcpy(source+0x70,matrix,64);memcpy(source+0xb0,matrix,64);
    gears_split_sources::API api{allocate,construct,destroy,free_view,read};
    auto run=[&](){return gears_split_sources::create(api,family,source,reinterpret_cast<void*>(0x2000));};
    auto result=run();
    if(result.checks!=63 || result.quarantined || !result.cleanup_safe ||
       result.inputs[0].value<float>(0x50)!=960 || result.inputs[1].value<float>(0x48)!=960 ||
       result.inputs[0].value<float>(0xa0)!=1 || result.inputs[1].value<float>(0xa0)!=-1)return 1;
    if(!gears_split_sources::release(api,result) || destroys!=2 || frees!=2 || result.views[0] || result.views[1])return 2;
    float inset[]={640,360,640,360};memcpy(source+0x48,inset,16);result=run();
    if(result.checks!=63 || result.inputs[0].value<float>(0x48)!=640 ||
       result.inputs[1].value<float>(0x48)!=960 || result.inputs[0].value<float>(0x50)!=320 ||
       result.inputs[1].value<float>(0x4c)!=360 || !gears_split_sources::release(api,result))return 8;
    int output_rectangle[]={0,0,1920,1080};memcpy(source+0x58,output_rectangle,16);
    result=gears_split_sources::create(api,family,source,reinterpret_cast<void*>(0x2000),true);
    if(result.checks!=63 || result.inputs[0].value<float>(0x48)!=0 ||
       result.inputs[1].value<float>(0x48)!=960 || result.inputs[0].value<float>(0x50)!=960 ||
       result.inputs[1].value<float>(0x4c)!=0 || result.inputs[1].value<float>(0x54)!=1080 ||
       !gears_split_sources::release(api,result))return 10;
    memcpy(source+0x48,rectangle,16);
    result=gears_split_sources::create(api,family,source,reinterpret_cast<void*>(0x2000),false,true);
    if(result.checks!=63 || memcmp(result.inputs[0].bytes.data(),source,sizeof(source)) ||
       memcmp(result.inputs[1].bytes.data(),source,sizeof(source)) || !gears_split_sources::release(api,result))return 11;
    unsigned freed_before_partial=frees;
    allocation_limit=allocations+1;result=run();allocation_limit=100;
    if(result.views[0] || result.views[1] || result.cleanup_safe || frees!=freed_before_partial+1)return 3;
    bad_matrix=true;result=run();bad_matrix=false;
    if((result.checks&8) || !result.cleanup_safe || !gears_split_sources::release(api,result))return 4;
    gears_shadow::Header original_array{aliased,1,1};memcpy(source+0x380,&original_array,sizeof(original_array));
    alias_array=true;result=run();alias_array=false;
    if(result.cleanup_safe || !result.quarantined || gears_split_sources::release(api,result))return 5;
    for(void* pointer:result.views)free_view(pointer,0x430); // Mock-known quarantine, never native cleanup.
    fail_constructor=true;result=run();fail_constructor=false;
    if(result.cleanup_safe || !result.quarantined)return 6;
    for(void* pointer:result.views)free_view(pointer,0x430);
    float odd=1919;memcpy(source+0x50,&odd,4);result=run();
    if(result.checks || result.views[0] || result.views[1])return 7;
    if(result.rejection!=3)return 9;
    printf("Split sources: native argument rectangles/offsets, cleanup ordering inputs, partial allocation and alias quarantine passed\n");return 0;
}
