#pragma once
#include <d3d12.h>
#include <cstdio>
#include <cstdint>
#include <array>
#include "ngx_typed_resources.h"
namespace ngx_stereo_host {
// Actual completed native family Create fields. This is a CPU parameter source,
// not proof of allocation, production, safe GPU retirement or SR admission.
struct NativeCreateSource {
    uint64_t revision{},world{};uintptr_t handle{},parameters{};
    std::array<uint32_t,7> fields{}; // Width,Height,OutWidth,OutHeight,quality,flags,subrect byte.
    bool split_input(uint32_t output_width,uint32_t output_height,uint32_t& width,uint32_t& height)const{
        if(!revision||!world||!handle||!parameters||fields[2]!=output_width||fields[3]!=output_height||
           output_width<128||output_width>8192||output_width%2||output_height<64||output_height>8192||
           fields[0]<128||fields[0]%2||fields[0]>output_width||fields[1]<64||fields[1]>output_height)return false;
        width=fields[0]/2;height=fields[1];return true;
    }
};
enum class NativeCreateAvailability {unavailable,inactive,ready};
using CopyFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,const D3D12_TEXTURE_COPY_LOCATION*,UINT,UINT,UINT,const D3D12_TEXTURE_COPY_LOCATION*,const D3D12_BOX*);
using ExecuteFn=void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*);
#if defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER
struct WrapperScope {
    int row{-1},previous{-1};int exceptions{};bool evaluate{},returned{},counted{};uint64_t result{};
    WrapperScope(bool,bool,uintptr_t,uintptr_t,void const*);~WrapperScope();
    void normal_return(uint64_t raw){returned=true;result=raw;}
    NativeCreateSource source{};WrapperScope* previous_source{};uintptr_t source_command{},source_structure{};
    unsigned source_dispatches{};uint32_t source_result{};bool source_seen{},source_bad{},source_returned{};
    WrapperScope(WrapperScope const&)=delete;WrapperScope& operator=(WrapperScope const&)=delete;
};
bool enable(FILE*);bool install_hooks();void print(FILE*);
// Fresh handle/parameter/world checks. No diagnostic-row quota is involved.
bool native_create_source(uint64_t world,NativeCreateSource&);
NativeCreateAvailability native_create_state(uint64_t world,NativeCreateSource&);
void barriers(ID3D12GraphicsCommandList*,UINT,D3D12_RESOURCE_BARRIER const*,uintptr_t);
void targets(ID3D12GraphicsCommandList*,UINT,D3D12_CPU_DESCRIPTOR_HANDLE const*,BOOL,D3D12_CPU_DESCRIPTOR_HANDLE const*);
void draw(ID3D12GraphicsCommandList*,uintptr_t,unsigned,unsigned);
void copy_passthrough(ID3D12GraphicsCommandList*,D3D12_TEXTURE_COPY_LOCATION const*,UINT,UINT,UINT,D3D12_TEXTURE_COPY_LOCATION const*,D3D12_BOX const*,uintptr_t,CopyFn);
void execute_passthrough(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*,ExecuteFn);
#else
struct WrapperScope {WrapperScope(bool,bool,uintptr_t,uintptr_t,void const*){}void normal_return(uint64_t){}};
inline bool enable(FILE*){return true;}inline bool install_hooks(){return true;}inline void print(FILE*){}
inline bool native_create_source(uint64_t,NativeCreateSource& out){out={};return false;}
inline NativeCreateAvailability native_create_state(uint64_t,NativeCreateSource& out){out={};return NativeCreateAvailability::unavailable;}
inline void barriers(ID3D12GraphicsCommandList*,UINT,D3D12_RESOURCE_BARRIER const*,uintptr_t){}
inline void targets(ID3D12GraphicsCommandList*,UINT,D3D12_CPU_DESCRIPTOR_HANDLE const*,BOOL,D3D12_CPU_DESCRIPTOR_HANDLE const*){}
inline void draw(ID3D12GraphicsCommandList*,uintptr_t,unsigned,unsigned){}
inline void copy_passthrough(ID3D12GraphicsCommandList* l,D3D12_TEXTURE_COPY_LOCATION const* d,UINT x,UINT y,UINT z,D3D12_TEXTURE_COPY_LOCATION const* s,D3D12_BOX const* b,uintptr_t,CopyFn f){f(l,d,x,y,z,s,b);}
inline void execute_passthrough(ID3D12CommandQueue* q,UINT n,ID3D12CommandList* const* l,ExecuteFn f){f(q,n,l);}
#endif
}
