#include <windows.h>
#include <cstdio>
#include "native_source_view_args.h"
namespace {
const gears_source_args::Snapshot* expected{};
unsigned calls{};bool good{};
__declspec(noinline) void* __fastcall receive(void* destination,void* owner,void* state,
    void* a4,void* a5,void* a6,void* a7,void* a8,void* a9,void* a10,
    float x,float y,float width,float height,const void* view,const void* projection,
    const void* p17,const void* p18,const void* p19,const void* map,float p21,unsigned p22) {
    ++calls;void* pointers[]={a4,a5,a6,a7,a8,a9,a10};
    good=destination==reinterpret_cast<void*>(0x1000) && owner==reinterpret_cast<void*>(0x2000) && state==reinterpret_cast<void*>(0x3000);
    for(unsigned i=0;i<7;++i)good=good && pointers[i]==reinterpret_cast<void*>(uintptr_t(0x4000+0x100*i));
    good=good && x==17.25f && y==-8.5f && width==1920.f && height==1080.f && p21==.625f && p22==0x89abcdef;
    good=good && view==expected->bytes.data()+0x70 && projection==expected->bytes.data()+0xb0 &&
        p17==expected->bytes.data()+0xf0 && p18==expected->bytes.data()+0x100 &&
        p19==expected->bytes.data()+0x110 && map==expected->bytes.data()+0x140;
    float view_value{},projection_value{};memcpy(&view_value,view,4);memcpy(&projection_value,projection,4);
    good=good && view_value==1.25f && projection_value==2.5f;
    return destination;
}
}
int main() {
    gears_source_args::Snapshot source{};
    for(unsigned i=0;i<7;++i){uintptr_t value=0x4000+0x100*i;memcpy(source.bytes.data()+0x10+8*i,&value,8);}
    float values[]={17.25f,-8.5f,1920.f,1080.f};memcpy(source.bytes.data()+0x48,values,sizeof(values));
    float v=1.25f,p=2.5f,factor=.625f;unsigned flags=0x89abcdef;
    memcpy(source.bytes.data()+0x70,&v,4);memcpy(source.bytes.data()+0xb0,&p,4);
    memcpy(source.bytes.data()+0x3e0,&factor,4);memcpy(source.bytes.data()+0x3e4,&flags,4);
    auto original=source.bytes;expected=&source;
    gears_source_args::Constructor volatile fn=receive;
    void* result=source.invoke(fn,reinterpret_cast<void*>(0x1000),reinterpret_cast<void*>(0x2000),reinterpret_cast<void*>(0x3000));
    if(result!=reinterpret_cast<void*>(0x1000) || !good || calls!=1 || original!=source.bytes)return 1;
    // After an owning snapshot moves, every input buffer must point to its new
    // storage, rather than to the old snapshot or a temporary argument buffer.
    auto relocated=source;expected=&relocated;
    relocated.invoke(fn,reinterpret_cast<void*>(0x1000),reinterpret_cast<void*>(0x2000),reinterpret_cast<void*>(0x3000));
    if(!good || calls!=2 || original!=relocated.bytes)return 2;
    printf("Source constructor scaffold: 22-argument pointer/float ABI, immutable inputs and relocated storage passed\n");return 0;
}
