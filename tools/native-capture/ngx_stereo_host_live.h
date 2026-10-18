#pragma once
#include <d3d12.h>
#include <cstdio>
#include <cstdint>
#include "ngx_typed_resources.h"
namespace ngx_stereo_host {
using CopyFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,const D3D12_TEXTURE_COPY_LOCATION*,UINT,UINT,UINT,const D3D12_TEXTURE_COPY_LOCATION*,const D3D12_BOX*);
using ExecuteFn=void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*);
#if defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER
struct WrapperScope {
    int row{-1},previous{-1};int exceptions{};bool evaluate{},returned{},counted{};uint64_t result{};
    WrapperScope(bool,bool,uintptr_t,uintptr_t,void const*);~WrapperScope();
    void normal_return(uint64_t raw){returned=true;result=raw;}
    WrapperScope(WrapperScope const&)=delete;WrapperScope& operator=(WrapperScope const&)=delete;
};
bool enable(FILE*);bool install_hooks();void print(FILE*);
void barriers(ID3D12GraphicsCommandList*,UINT,D3D12_RESOURCE_BARRIER const*,uintptr_t);
void targets(ID3D12GraphicsCommandList*,UINT,D3D12_CPU_DESCRIPTOR_HANDLE const*,BOOL,D3D12_CPU_DESCRIPTOR_HANDLE const*);
void draw(ID3D12GraphicsCommandList*,uintptr_t,unsigned,unsigned);
void copy_passthrough(ID3D12GraphicsCommandList*,D3D12_TEXTURE_COPY_LOCATION const*,UINT,UINT,UINT,D3D12_TEXTURE_COPY_LOCATION const*,D3D12_BOX const*,uintptr_t,CopyFn);
void execute_passthrough(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*,ExecuteFn);
#else
struct WrapperScope {WrapperScope(bool,bool,uintptr_t,uintptr_t,void const*){}void normal_return(uint64_t){}};
inline bool enable(FILE*){return true;}inline bool install_hooks(){return true;}inline void print(FILE*){}
inline void barriers(ID3D12GraphicsCommandList*,UINT,D3D12_RESOURCE_BARRIER const*,uintptr_t){}
inline void targets(ID3D12GraphicsCommandList*,UINT,D3D12_CPU_DESCRIPTOR_HANDLE const*,BOOL,D3D12_CPU_DESCRIPTOR_HANDLE const*){}
inline void draw(ID3D12GraphicsCommandList*,uintptr_t,unsigned,unsigned){}
inline void copy_passthrough(ID3D12GraphicsCommandList* l,D3D12_TEXTURE_COPY_LOCATION const* d,UINT x,UINT y,UINT z,D3D12_TEXTURE_COPY_LOCATION const* s,D3D12_BOX const* b,uintptr_t,CopyFn f){f(l,d,x,y,z,s,b);}
inline void execute_passthrough(ID3D12CommandQueue* q,UINT n,ID3D12CommandList* const* l,ExecuteFn f){f(q,n,l);}
#endif
}
