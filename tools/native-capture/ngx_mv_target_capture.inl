// One subsequent consumption window, independent of the frozen epoch-pair
// diagnostic quota. Included after the host implementation. All state access
// uses frame_hook_mutex, followed by the descriptor observer's mutex if needed.
namespace ngx_mv_target_capture {
#if GEARS_NGX_MV_TARGET_CAPTURE
using gears_ngx_mv_target::Context;
using gears_ngx_mv_target::Phase;
gears_ngx_mv_target::Window window;
ngx_stereo_host::NativeCreateSource selected_source{};
gears_ngx_stereo::TypedResourceLease selected_mv;
bool closed{},leases_released{};unsigned stop_cause{};
struct Result {uint64_t wrapper{};uint32_t sdk{};bool returned{},sdk_returned{},accepted{};};
std::array<Result,2> results{};
enum Stop:unsigned {create_changed=1,feature_released,world_changed,identity_lost,nested_wrapper,closed_capture,sdk_mismatch,consume_unwound,selection_invalid};
bool pending(){const auto p=window.snapshot().phase;return p==Phase::waiting||p==Phase::capturing;}
void terminal(){
    const auto p=window.snapshot().phase;
    if(p==Phase::frozen||p==Phase::failed){ngx_mv_compute_capture::stop();ngx_mv_descriptor_capture::stop_target();}
}
Context identity(){
    Context out;auto const& s=window.snapshot().selection;
    out.world=committed_world_generation.load();out.mv=selected_mv.retained()?selected_mv.canonical:0;
    const bool matched=ngx_stereo_host::native_create_matches(selected_source);
    auto const& current=ngx_stereo_host::latest_native_create;
    out.revision=current.revision?current.revision:ngx_stereo_host::native_create_revision;
    out.handle=current.handle;out.parameters=current.parameters;
    out.current=!closed&&recording.load()&&matched&&out.world==s.world&&!submitted_lifetime.stopping();
    out.enabled=true;return out;
}
void invalidate(unsigned cause){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!pending())return;
    if(!stop_cause)stop_cause=cause;
    auto c=identity();c.current=false;c.nested=cause==nested_wrapper;
    window.observe_identity(c,GetTickCount64());terminal();
}
void select_seed(ngx_stereo_host::NativeCreateSource const& source){
    if(closed||window.snapshot().phase!=Phase::idle)return;
    uint64_t observation{};gears_ngx_mv_census::Use use;gears_ngx_stereo::TypedResourceLease mv;
    if(!ngx_mv_descriptor_capture::snapshot_target_seed(observation,use,mv))return;
    const auto selection_tick=GetTickCount64();
    if(!observation||!mv.retained()||source.revision!=use.revision||source.world!=use.world||
       source.handle!=use.handle||source.parameters!=use.parameters||!ngx_stereo_host::native_create_matches(source)){
        window.select({},selection_tick,5000);stop_cause=selection_invalid;terminal();return;
    }
    selected_source=source;selected_mv=std::move(mv);
    window.select({observation,use.revision,use.world,use.ticket,use.handle,use.parameters,selected_mv.canonical},selection_tick,5000);
    terminal();
}
Context consumption(gears_submitted::FrameTicket const& frame,void* renderer,bool nested,bool entering){
    auto c=identity();c.ticket=frame.id;c.renderer=reinterpret_cast<uintptr_t>(renderer);c.enabled=frame.enabled;c.nested=nested;
    c.current=c.current&&frame.world_generation==c.world;
    // Only entry asks the current submitted inventory. The original consumer
    // may retire and free renderer before end; the closing tuple is local.
    if(entering){gears_submitted::FrameTicket live;
        c.current=c.current&&renderer&&submitted_lifetime.frame(c.renderer,live)&&live.enabled&&
            live.id==frame.id&&live.world_generation==frame.world_generation;
    }
    return c;
}
struct ConsumeScope {
    gears_submitted::FrameTicket frame;void* renderer{};bool nested{},finished{};
    ConsumeScope(gears_submitted::FrameTicket const& f,void* r,bool n):frame(f),renderer(r),nested(n){
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        if(pending()){window.consume_begin(consumption(frame,renderer,nested,true),GetTickCount64());terminal();}
    }
    void finish(){
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);finished=true;
        if(pending()){window.consume_end(consumption(frame,renderer,nested,false),GetTickCount64());terminal();}
    }
    ~ConsumeScope(){if(!finished)invalidate(consume_unwound);}
};
void eval_enter(ngx_stereo_host::WrapperScope& scope,bool owned,uintptr_t caller,uintptr_t command,void const* structure,bool concurrent){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    select_seed(ngx_stereo_host::latest_native_create);
    if(!pending())return;
    auto c=consumption(consuming_frozen_frame,consuming_renderer,concurrent,true);
    c.current=c.current&&owned&&caller==base+0x9a35d3&&consuming_frame_ticket==c.ticket;
    uintptr_t raw{};const auto address=reinterpret_cast<uintptr_t>(structure);
    if(!address||address>UINTPTR_MAX-0x28||!safe_copy(&raw,reinterpret_cast<void const*>(address+0x20),8)||raw!=selected_mv.raw)c.mv=0;
    if(window.eval_enter(c,reinterpret_cast<uintptr_t>(&scope),GetTickCount64())){
        scope.target_admitted=true;scope.target_context=c;scope.target_structure=address;
        scope.target_ordinal=window.snapshot().eval_count-1;
        scope.target_receipt.begin(command,c.handle,c.parameters);
    }
    terminal();
}
void sdk_enter(ngx_stereo_host::WrapperScope* scope,uintptr_t command,uintptr_t handle,uintptr_t parameters,uintptr_t callback){
    if(scope&&scope->target_admitted&&!scope->target_receipt.dispatch_enter(command,handle,parameters,callback))invalidate(sdk_mismatch);
}
void sdk_leave(ngx_stereo_host::WrapperScope* scope,uint32_t raw){
    if(!scope||!scope->target_admitted)return;
    scope->target_sdk_result=raw;scope->target_sdk_returned=true;scope->target_receipt.dispatch_leave(true,raw);
}
void eval_leave(ngx_stereo_host::WrapperScope& scope,bool returned,bool unwinding,uint64_t raw){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!scope.target_admitted)return;
    auto c=consumption(consuming_frozen_frame,consuming_renderer,false,true);
    uintptr_t mv{};
    if(!safe_copy(&mv,reinterpret_cast<void const*>(scope.target_structure+0x20),8)||mv!=selected_mv.raw)c.mv=0;
    const bool normal=returned&&!unwinding;
    const bool accepted=scope.target_receipt.finish(returned,unwinding,raw);
    if(scope.target_ordinal<results.size())results[scope.target_ordinal]={raw,scope.target_sdk_result,normal,scope.target_sdk_returned,accepted};
    window.eval_leave(c,reinterpret_cast<uintptr_t>(&scope),normal,accepted,GetTickCount64());terminal();
}
void poll(){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!pending())return;
    if(committed_world_generation.load()!=window.snapshot().selection.world){invalidate(world_changed);return;}
    // A worker can run while the original SDK call is in flight. Its Create
    // identity checks are deliberately deferred until that wrapper exits.
    if(!ngx_stereo_host::in_wrappers)window.observe_identity(identity(),GetTickCount64());
    else window.poll(GetTickCount64());
    terminal();
}
void close(){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);closed=true;
    invalidate(closed_capture);ngx_mv_compute_capture::stop();ngx_mv_descriptor_capture::stop_target();
}
void release_drained(){
    gears_ngx_stereo::TypedResourceLease retired;
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        if(!closed||active.load()||recording.load())return;
        retired=std::move(selected_mv);leases_released=true;}
    // No observer lock across the final native Release.
    retired.release();
    ngx_mv_compute_capture::release_drained();
}
void print(FILE* file){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!file)return;auto const& s=window.snapshot();
    ngx_mv_compute_capture::print(file);
    fprintf(file,"ngx_mv_target_window enabled=1 phase=%u reason=%u stop_cause=%u observation=%llu revision=%llu world=%llu selection_ticket=%llu consume_ticket=%llu renderer=%llx handle=%llx parameters=%llx mv=%llx selected_tick_ms=%llu deadline_tick_ms=%llu last_tick_ms=%llu evals=%u completed_evals=%u eval_open=%u closed=%u leases_released=%u producer_domain_verified=0 gpu_completion_verified=0\n",
        unsigned(s.phase),unsigned(s.reason),stop_cause,s.selection.observation_id,s.selection.revision,s.selection.world,s.selection.ticket,
        s.consume.ticket,s.consume.renderer,s.selection.handle,s.selection.parameters,s.selection.mv,s.selected_at,s.deadline,s.last_now,
        s.eval_count,s.completed_evals,unsigned(s.eval_open),unsigned(closed),unsigned(leases_released));
    for(unsigned i=0;i<s.eval_count;++i){auto const& e=s.evals[i];auto const& r=results[i];
        fprintf(file,"ngx_mv_target_eval row=%u scope=%llx entered_tick_ms=%llu returned_tick_ms=%llu returned=%u accepted=%u sdk_returned=%u sdk_result=%x wrapper_result=%llx named_success=%u\n",
            i,e.scope,e.entered_at,e.returned_at,unsigned(r.returned),unsigned(r.accepted),unsigned(r.sdk_returned),r.sdk,r.wrapper,unsigned(r.sdk_returned&&r.sdk==1));}
}
#else
struct ConsumeScope {ConsumeScope(gears_submitted::FrameTicket const&,void*,bool){}void finish(){}};
void poll(){}void close(){}void release_drained(){}void print(FILE*){}
#endif
}
