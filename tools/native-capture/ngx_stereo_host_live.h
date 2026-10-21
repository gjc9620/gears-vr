#pragma once
#include <d3d12.h>
#include <cstdio>
#include <cstdint>
#include <array>
#include <functional>
#include <mutex>
#include "ngx_typed_resources.h"
#include "ngx_native_create_source.h"
#include "ngx_stereo_native_bridge.h"
#include "ngx_stereo_native_api.h"
#include "ngx_owned_publication.h"
#include "dlss_output_control.h"
namespace ngx_capture_control {
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
bool trial_before_create();
void print_trial(FILE*);
bool pixel_capture_ready();
bool preprocess_pixel_mode();
bool preprocess_pixel_resources(std::array<uintptr_t,4>&);
bool scene_color_pixel_mode();
bool right_draw_pixel_mode();
bool pre_eval_pixel_mode();
bool baseline_pixel_mode();
bool baseline_pixel_ready();
bool input_union_mask_trial(uint64_t,uint64_t,uintptr_t,uintptr_t);
bool constructor_source_enabled();
void consumed(uint64_t,uint64_t,uintptr_t,bool);
bool post_copy_color_pixels();
bool output_route_sample_ready();
#if defined(GEARS_NGX_OWNED_SCENE_TRIAL) && GEARS_NGX_OWNED_SCENE_TRIAL
gears_ngx_owned::Publication owned_publication(uint64_t,uint64_t);
bool native_output_ticket(uint64_t,uint64_t,uintptr_t,uintptr_t,gears_dlss_output::NativeGap&,bool preserve_candidate=false);
bool native_output_current(gears_dlss_output::NativeGap const&);
void owned_draw_observe(ID3D12GraphicsCommandList*,gears_viewport::TargetDraw const&,uintptr_t,unsigned,unsigned,unsigned,bool);
void owned_copy_observe(ID3D12GraphicsCommandList*,D3D12_TEXTURE_COPY_LOCATION const*,UINT,UINT,UINT,D3D12_TEXTURE_COPY_LOCATION const*,D3D12_BOX const*,uintptr_t,unsigned);
#endif
bool final_output_ticket(uint64_t,uint64_t,uintptr_t,uintptr_t,gears_ngx_stereo::RecordedOutput&);
bool final_output_current(gears_ngx_stereo::RecordedOutput const&);
bool final_target_pixel_mode();
void acquire_final_target_barriers(UINT,D3D12_RESOURCE_BARRIER const*);
bool final_target_pixel_pair(uintptr_t,ID3D12Resource*&,ID3D12Resource*&,unsigned&,gears_ngx_stereo::Rect&);
bool right_draw_pixel_pair(uintptr_t,ID3D12Resource*&,ID3D12Resource*&,unsigned&,gears_ngx_stereo::Rect&);
void right_draw_pixel_trace(unsigned,float,float,float,float,float,float,float,float);
bool output_target_rect_trial();
bool packed_input_trial();
bool scene_color_pixel_pair(ID3D12Resource*,ID3D12Resource*&);
void scene_color_transition_trace(unsigned,unsigned,unsigned,unsigned,uintptr_t,bool,uint64_t);
#endif
}
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER && \
    defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER && \
    defined(GEARS_NGX_EPOCH_PAIR_OBSERVER) && GEARS_NGX_EPOCH_PAIR_OBSERVER
#define GEARS_NGX_MV_TARGET_CAPTURE 1
#include "ngx_mv_target_window.h"
#include "ngx_mv_sdk_receipt.h"
#else
#define GEARS_NGX_MV_TARGET_CAPTURE 0
#endif
namespace ngx_stereo_host {
using NativeCreateSource = gears_ngx_create::NativeCreateSource;
enum class NativeCreateAvailability {unavailable,inactive,ready};
using CreateWrapperFn=uint64_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,void const*);
using EvaluateWrapperFn=uint64_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,void const*);
using CopyFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,const D3D12_TEXTURE_COPY_LOCATION*,UINT,UINT,UINT,const D3D12_TEXTURE_COPY_LOCATION*,const D3D12_BOX*);
using ExecuteFn=void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*);
#if defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER
// Acquired before the wrapper's observation/frame locks. Extra GetOptimal
// calls and complete native parameter setters share this serialization scope.
// Acquired before any frame bookkeeping lock at wrapper/Execute/Copy/Reset
// entry. It orders adapter operations without holding frame_hook_mutex over SDK.
struct ApiOrderScope {std::unique_lock<std::recursive_mutex> lock;ApiOrderScope();};
struct ParameterScope {std::unique_lock<std::recursive_mutex> lock;ApiOrderScope order;ParameterScope();};
struct ControlConsumption {uint64_t world{},ticket{};uintptr_t renderer{};};
struct ControlCreateBinding {
    gears_ngx_stereo::Identity identity{};gears_ngx_stereo::Layout layout{};gears_ngx_stereo::Command command{};
    gears_ngx_stereo::NativeOptimalSettings optimal{}; // One query covers two identically sized independent features.
    uint64_t creation_ticket{};
};
struct ControlInputTicket {
    ControlConsumption consumption{};std::array<uintptr_t,2> original_views{};
    std::array<gears_ngx_stereo::ProducerObservation,2> producers{};
    // Adapter order Color,Depth,MV,Output; must come from retained typed bindings.
    std::array<uint64_t,4> raw_resources{};gears_ngx_stereo::Command command{};
    bool native_transition{}; // Private, unpublished first pair; no fresh MV claim.
};
struct ControlCallbacks {
    // Retains the actual ticket/typed-resource/fence owner until drained detach.
    std::shared_ptr<void> lifetime;
    bool (*create)(void*,NativeCreateSource const&,uintptr_t,ControlCreateBinding&){};
    bool (*input)(void*,ControlConsumption,gears_ngx_stereo::EvaluateShadow const&,uintptr_t,ControlInputTicket&){};
    gears_ngx_stereo::Completion (*completion)(void*,uint64_t){};
    // Called before the input callback/adapter gate. May hold the synchronous
    // native call until an already-submitted first pair completes; no replay.
    bool (*admit)(void*,ControlConsumption){};
};
// Live SDK owner. No automatic Release in the destructor. The host pins this
// owner; detach is rejected until actual creation/Eval/Copy retirement drains.
class ControlOwner {
    gears_ngx_stereo::NativeDispatchBackend backend_;
    gears_ngx_stereo::StereoAdapter adapter_;
    gears_ngx_stereo::NativeWrapperBridge bridge_;
    ControlCallbacks callbacks_;bool configured_{},packed_input_{};
    std::mutex metadata_mutex_;ControlCreateBinding creation_{};
public:
    ControlOwner(gears_ngx_stereo::NativeDispatchFunctions functions,gears_ngx_stereo::AdapterPrerequisites prerequisites,ControlCallbacks callbacks)
        :backend_(functions),adapter_(backend_,prerequisites),bridge_(adapter_,true),callbacks_(std::move(callbacks)),
         configured_(functions.ready()&&functions.optimal&&prerequisites.ready()&&callbacks_.lifetime&&callbacks_.create&&callbacks_.input&&callbacks_.completion&&callbacks_.admit),packed_input_(prerequisites.packed_input_staging){}
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
        backend_.trace("release_wrapper_begin",0,handle);
        const auto result=bridge_.release_dispatch(handle,callbacks_.completion(callbacks_.lifetime.get(),handle),original);
        backend_.trace("release_wrapper_return",0,handle,result);return result;}
};
gears_ngx_stereo::NativeOptimalQuery native_optimal_query();
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
#if GEARS_NGX_MV_TARGET_CAPTURE
    WrapperScope* target_previous{};bool target_linked{},target_admitted{},target_sdk_returned{};
    uintptr_t target_structure{};unsigned target_ordinal{2};uint32_t target_sdk_result{};
    gears_ngx_mv_target::Context target_context{};
    gears_ngx_mv_target::SdkReceipt target_receipt{};
#endif
    WrapperScope(WrapperScope const&)=delete;WrapperScope& operator=(WrapperScope const&)=delete;
};
bool enable(FILE*);bool install_hooks();void print(FILE*);
// Fresh handle/parameter/world checks. No diagnostic-row quota is involved.
bool native_create_source(uint64_t world,NativeCreateSource&);
bool native_create_matches(NativeCreateSource const&);
NativeCreateAvailability native_create_state(uint64_t world,NativeCreateSource&);
void barriers(ID3D12GraphicsCommandList*,UINT,D3D12_RESOURCE_BARRIER const*,uintptr_t);
void targets(ID3D12GraphicsCommandList*,UINT,D3D12_CPU_DESCRIPTOR_HANDLE const*,BOOL,D3D12_CPU_DESCRIPTOR_HANDLE const*);
void draw(ID3D12GraphicsCommandList*,uintptr_t,unsigned,unsigned);
void copy_passthrough(ID3D12GraphicsCommandList*,D3D12_TEXTURE_COPY_LOCATION const*,UINT,UINT,UINT,D3D12_TEXTURE_COPY_LOCATION const*,D3D12_BOX const*,uintptr_t,CopyFn);
void execute_passthrough(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*,ExecuteFn);
#else
struct ApiOrderScope {};
struct ParameterScope {};
inline uint64_t create_passthrough(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,void const* p,CreateWrapperFn f){return f(a,b,c,d,e,p);}
inline uint64_t evaluate_passthrough(bool,uintptr_t a,uintptr_t b,uintptr_t c,void const* p,EvaluateWrapperFn f){return f(a,b,c,p);}
struct WrapperScope {WrapperScope(bool,bool,uintptr_t,uintptr_t,void const*){}void normal_return(uint64_t){}};
inline bool enable(FILE*){return true;}inline bool install_hooks(){return true;}inline void print(FILE*){}
inline bool native_create_source(uint64_t,NativeCreateSource& out){out={};return false;}
inline bool native_create_matches(NativeCreateSource const&){return false;}
inline NativeCreateAvailability native_create_state(uint64_t,NativeCreateSource& out){out={};return NativeCreateAvailability::unavailable;}
inline void barriers(ID3D12GraphicsCommandList*,UINT,D3D12_RESOURCE_BARRIER const*,uintptr_t){}
inline void targets(ID3D12GraphicsCommandList*,UINT,D3D12_CPU_DESCRIPTOR_HANDLE const*,BOOL,D3D12_CPU_DESCRIPTOR_HANDLE const*){}
inline void draw(ID3D12GraphicsCommandList*,uintptr_t,unsigned,unsigned){}
inline void copy_passthrough(ID3D12GraphicsCommandList* l,D3D12_TEXTURE_COPY_LOCATION const* d,UINT x,UINT y,UINT z,D3D12_TEXTURE_COPY_LOCATION const* s,D3D12_BOX const* b,uintptr_t,CopyFn f){f(l,d,x,y,z,s,b);}
inline void execute_passthrough(ID3D12CommandQueue* q,UINT n,ID3D12CommandList* const* l,ExecuteFn f){f(q,n,l);}
#endif
}
