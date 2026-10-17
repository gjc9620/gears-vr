#pragma once
#include <d3d12.h>
#include <mutex>
#include "frame_link.h"
#include "ngx_route_observer.h"
#include "death_hdr_pixels.h"
namespace ngx_output_pixels {
using CopyFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,const D3D12_TEXTURE_COPY_LOCATION*,UINT,UINT,UINT,const D3D12_TEXTURE_COPY_LOCATION*,const D3D12_BOX*);
using ExecuteFn=void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*);
using ResetFn=HRESULT(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*);
#if defined(GEARS_NGX_OUTPUT_PIXELS) && GEARS_NGX_OUTPUT_PIXELS
enum Failure:unsigned {none,association_ambiguous=32,eval_changed,eval_exception,nested_eval,typed_mismatch,prepare_failed,copy_exception,append_exception,stopped_incomplete};
struct Slot {
    gears_ngx_route::Sample before{},after{};uint64_t sequence{},copy_sequence{},result{};uintptr_t caller{},command{},structure{};unsigned thread{};
    long long entry{},exit{},copy_qpc{},execute_order{};unsigned execute_member{};bool normal{},active{},copy_seen{},inside_eval{},prepared{},released{};
    ID3D12GraphicsCommandList* raw_list{};D3D12_TEXTURE_COPY_LOCATION source{},destination{};
    gears_death_hdr_pixels::Texture source_desc{},destination_desc{};gears_death_hdr_pixels::Lifetime life{};
    HRESULT hr{E_PENDING},reset_hr{E_PENDING};long long reset_qpc{};wchar_t path[MAX_PATH]{};
};
inline bool textures_supported(gears_death_hdr_pixels::Texture const& a,gears_death_hdr_pixels::Texture const& b){
    return gears_death_hdr_pixels::supported(a)&&gears_death_hdr_pixels::supported(b)&&a.canonical!=b.canonical&&a.width==b.width&&a.height==b.height;
}
// Native GPU operations are isolated from association/lifetime logic so actual
// forwarding wrappers can be exercised offline without creating a GPU device.
struct Backend {
    virtual ~Backend()=default;
    virtual bool start()=0;
    virtual bool prepare(unsigned,Slot&)=0;
    virtual void append(unsigned,Slot&,CopyFn)=0;
    virtual uintptr_t list_identity(ID3D12CommandList*)=0;
    virtual gears_frame_link::ListRef list_ref(uintptr_t)=0;
    virtual bool queue_valid(unsigned,ID3D12CommandQueue*,uintptr_t&)=0;
    virtual bool signal(unsigned,ID3D12CommandQueue*)=0;
    virtual bool poll(unsigned,Slot&)=0;
    virtual void release(unsigned)=0;
    virtual bool publish(uint64_t,uint64_t,uint64_t)=0;
    virtual void close()=0;
};
Backend& backend();
struct EvalScope {
    int index{-1},parent{-1};int exceptions{};bool returned{};uint64_t raw_result{};
    EvalScope(bool world,uintptr_t caller,uintptr_t command,const void* structure);~EvalScope();
    void normal_return(uint64_t value){returned=true;raw_result=value;}
    EvalScope(EvalScope const&)=delete;EvalScope& operator=(EvalScope const&)=delete;
};
void create_boundary(bool owned,uintptr_t caller,uintptr_t command,const void* structure);
bool enable(FILE*);void poll();void stop();void print(FILE*);
std::unique_lock<std::recursive_mutex> execution_lock();
void copy_passthrough(ID3D12GraphicsCommandList*,const D3D12_TEXTURE_COPY_LOCATION*,UINT,UINT,UINT,const D3D12_TEXTURE_COPY_LOCATION*,const D3D12_BOX*,uintptr_t,CopyFn);
unsigned prepare_execute(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*);void after_execute(unsigned,ID3D12CommandQueue*);
void execute_passthrough(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*,ExecuteFn);
void reset_after_original(ID3D12GraphicsCommandList*,HRESULT);
HRESULT reset_passthrough(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*,ResetFn);
#else
struct EvalScope {EvalScope(bool,uintptr_t,uintptr_t,const void*){}void normal_return(uint64_t){}};
inline void create_boundary(bool,uintptr_t,uintptr_t,const void*){}inline bool enable(FILE*){return true;}inline void poll(){}inline void stop(){}inline void print(FILE*){}
inline std::unique_lock<std::recursive_mutex> execution_lock(){return {};}
inline void copy_passthrough(ID3D12GraphicsCommandList* l,const D3D12_TEXTURE_COPY_LOCATION* d,UINT x,UINT y,UINT z,const D3D12_TEXTURE_COPY_LOCATION* s,const D3D12_BOX* b,uintptr_t,CopyFn f){f(l,d,x,y,z,s,b);}
inline unsigned prepare_execute(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*){return 0;}inline void after_execute(unsigned,ID3D12CommandQueue*){}
inline void execute_passthrough(ID3D12CommandQueue* q,UINT n,ID3D12CommandList* const* l,ExecuteFn f){f(q,n,l);}
inline void reset_after_original(ID3D12GraphicsCommandList*,HRESULT){}
inline HRESULT reset_passthrough(ID3D12GraphicsCommandList* l,ID3D12CommandAllocator* a,ID3D12PipelineState* p,ResetFn f){return f(l,a,p);}
#endif
}
