#pragma once
#include <d3d12.h>
#include <cstdio>
#include <cstdint>
#include <array>
#include <functional>
#include <mutex>
#include "ngx_typed_resources.h"
#include "ngx_stereo_native_bridge.h"
#include "ngx_stereo_native_api.h"
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
using CreateWrapperFn=uint64_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,void const*);
using EvaluateWrapperFn=uint64_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,void const*);
using CopyFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,const D3D12_TEXTURE_COPY_LOCATION*,UINT,UINT,UINT,const D3D12_TEXTURE_COPY_LOCATION*,const D3D12_BOX*);
using ExecuteFn=void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*);
#if defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER
struct ControlConsumption {uint64_t world{},ticket{};uintptr_t renderer{};};
struct ControlCreateBinding {
    gears_ngx_stereo::Identity identity{};gears_ngx_stereo::Layout layout{};gears_ngx_stereo::Command command{};
};
struct ControlInputTicket {
    ControlConsumption consumption{};std::array<uintptr_t,2> original_views{};
    std::array<gears_ngx_stereo::ProducerObservation,2> producers{};
    // Adapter order Color,Depth,MV,Output; must come from retained typed bindings.
    std::array<uint64_t,4> raw_resources{};gears_ngx_stereo::Command command{};
};
struct ControlCallbacks {
    // Retains the actual ticket/typed-resource/fence owner until drained detach.
    std::shared_ptr<void> lifetime;
    bool (*create)(void*,NativeCreateSource const&,uintptr_t,ControlCreateBinding&){};
    bool (*input)(void*,ControlConsumption,gears_ngx_stereo::EvaluateShadow const&,uintptr_t,ControlInputTicket&){};
    gears_ngx_stereo::Completion (*completion)(void*,uint64_t){};
};
// Live SDK owner. No automatic Release in the destructor. The host pins this
// owner; detach is rejected until actual creation/Eval/Copy retirement drains.
class ControlOwner {
    gears_ngx_stereo::NativeDispatchBackend backend_;
    gears_ngx_stereo::StereoAdapter adapter_;
    gears_ngx_stereo::NativeWrapperBridge bridge_;
    ControlCallbacks callbacks_;bool configured_{};
    std::mutex metadata_mutex_;ControlCreateBinding creation_{};
public:
    ControlOwner(gears_ngx_stereo::NativeDispatchFunctions functions,gears_ngx_stereo::AdapterPrerequisites prerequisites,ControlCallbacks callbacks)
        :backend_(functions),adapter_(backend_,prerequisites),bridge_(adapter_,true),callbacks_(std::move(callbacks)),
         configured_(functions.ready()&&prerequisites.ready()&&callbacks_.lifetime&&callbacks_.create&&callbacks_.input&&callbacks_.completion){}
    ControlOwner(ControlOwner const&)=delete;
    bool configured()const{return configured_;}
    bool can_detach(){auto e=adapter_.enter();return e&&(!backend_.create_attempts()||adapter_.drained(e));}
    void invalidate(){auto e=adapter_.enter();if(e)adapter_.invalidate(e);}
    // Root feeds ONLY real typed Copy/Execute/Signal/completion observations.
    gears_ngx_stereo::StereoAdapter& adapter(){return adapter_;}
    gears_ngx_stereo::NativeWrapperBridge& bridge(){return bridge_;}
    uint64_t create(NativeCreateSource const&,uintptr_t,gears_ngx_stereo::CreateShadow const&,void const*,std::function<uint64_t(void const*)> const&);
    uint64_t evaluate(ControlConsumption,uintptr_t,uintptr_t,uintptr_t,gears_ngx_stereo::EvaluateShadow const&,void const*,std::function<uint64_t(void const*)> const&);
    uint32_t release(uint64_t handle,std::function<uint32_t(uint64_t)> const& original){
        return bridge_.release_dispatch(handle,callbacks_.completion(callbacks_.lifetime.get(),handle),original);}
};
bool attach_control(std::shared_ptr<ControlOwner> const&);
bool detach_control(std::shared_ptr<ControlOwner> const&);
uint64_t create_passthrough(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,void const*,CreateWrapperFn);
uint64_t evaluate_passthrough(bool,uintptr_t,uintptr_t,uintptr_t,void const*,EvaluateWrapperFn);
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
inline uint64_t create_passthrough(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,void const* p,CreateWrapperFn f){return f(a,b,c,d,e,p);}
inline uint64_t evaluate_passthrough(bool,uintptr_t a,uintptr_t b,uintptr_t c,void const* p,EvaluateWrapperFn f){return f(a,b,c,p);}
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
