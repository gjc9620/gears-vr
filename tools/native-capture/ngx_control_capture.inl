// Capture-local integration. No startup call is added by this patch.
namespace ngx_capture_control {
#if defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER
using namespace gears_ngx_stereo;
#include "ngx_packed_stage_capture.inl"
struct Context {
    struct Construction {uint64_t world{},ticket{};uintptr_t renderer{};std::array<uintptr_t,2> views{};};
    NativeGpuSubmission gpu;std::weak_ptr<ngx_stereo_host::ControlOwner> owner;
    Completion last_native_completion{};unsigned adapter_failure_line{},adapter_phase{};
    std::array<Construction,64> constructions{};
    std::unique_ptr<TypedQuartet> quartet{std::make_unique<TypedQuartet>()};
    std::shared_ptr<void> producer_lifetime;
    // Actual producer evidence remains required; no observer row/bank can
    // implement it. Quartet retention is separate from producer proof.
    bool (*producer)(void*,ngx_stereo_host::ControlCreateBinding const&,ngx_stereo_host::ControlConsumption,
        EvaluateShadow const&,Command,std::array<Resource,4> const&,ngx_stereo_host::ControlInputTicket&){};
    ngx_stereo_host::ControlCreateBinding created{};ngx_stereo_host::ControlInputTicket last_input{};
    TypedResourceLease pixel_output,pixel_color;
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    PackedBuffers packed_buffers;PackedStage packed_stage;
    std::optional<RecordedOutput> final_output_snapshot;
    std::array<TypedResourceLease,16> final_targets{};unsigned final_target_count{};
    std::array<std::unique_ptr<TypedQuartet>,8> retired_quartets{};unsigned recreations{};
    std::array<uint32_t,7> native_create_fields{};bool epoch_retired{};
    gears_ngx_source::Activity source_activity;uint64_t constructor_source_suppressed{};
    uint64_t control_mv_epoch{};
    unsigned owned_draw_traces{},owned_unknown_draw_traces{},owned_other_draw_traces{},owned_gap_draw_traces{},owned_rebind_draw_traces{},owned_copy_traces{};
    uint64_t owned_rebind_trace_epoch{};
#endif
    unsigned last_eye{},failure_stage{};TypedCopyBoundary* copy_boundary{};bool failed{},copy_pending{},transport_accepted{};
};
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
std::shared_ptr<Context>& context=*new std::shared_ptr<Context>;
FILE* trial_events{};unsigned trial_event_count{};
unsigned trial_failure_event_count{};
unsigned trial_lifecycle_event_count{};
bool trial_preprocess_pixels{};
bool trial_scene_color_pixels{};
bool trial_output_target_rects{},trial_preprocess_preserve{};
bool trial_packed_input{};
bool trial_right_draw_pixels{};
bool trial_pre_eval_pixels{};
bool trial_baseline_pixels{};
bool trial_input_union_mask{};
bool trial_color_after_pixels{};
bool trial_final_output_join{};
bool trial_final_target_pixels{};
uint64_t trial_preprocess_boxed{},trial_preprocess_unresolved{};unsigned trial_preprocess_traces{};
unsigned trial_copy_stack_traces{};
void trial_event(void* opaque,char const* stage,uint64_t command_value,uint64_t value,NativeResult result){
    auto* c=static_cast<Context*>(opaque);if(!trial_events)return;
    const bool failure_event=!strcmp(stage,"context_rejected")||!strcmp(stage,"context_invalidated");
    const bool lifecycle_event=!strncmp(stage,"release_",8)||!strncmp(stage,"create_wrapper",14)||!strncmp(stage,"mv_",3)||!strcmp(stage,"create_binding_rejected")||!strcmp(stage,"create_leave");
    if(trial_event_count>=128&&(!failure_event||trial_failure_event_count>=8)&&(!lifecycle_event||trial_lifecycle_event_count>=32))return;
    if(failure_event)++trial_failure_event_count;
    if(lifecycle_event)++trial_lifecycle_event_count;
    fprintf(trial_events,"event=%u stage=%s thread=%lu ticket=%llu cmd=%llx value=%llx result=%x context_failure=%u context_failed=%u\n",
        ++trial_event_count,stage,GetCurrentThreadId(),consuming_frame_ticket,command_value,value,result,c?c->failure_stage:0,unsigned(c&&c->failed));fflush(trial_events);
    if(c&&(failure_event||lifecycle_event)){auto const& done=c->last_native_completion;uintptr_t public_handle{};
        ngx_route_observer::read(public_handle,base,gears_ngx_route::handle_cell_rva);
        fprintf(trial_events,"lifecycle cached_adapter_failure=%u cached_adapter_phase=%u epoch=%llu world=%llu public_handle=%llx completion_device=%llx completion_queue=%llx completion_fence=%llx completion_value=%llu copy_pending=%u\n",
            c->adapter_failure_line,c->adapter_phase,c->created.identity.epoch,c->created.identity.world,public_handle,done.device,done.queue,done.fence,done.value,unsigned(c->copy_pending));fflush(trial_events);}
    fprintf(trial_events,"producer_failure=%u surfaces=%u depth_copies=%u mv_dispatches=%u pso_queries=%llu pso_verified=%llu\n",
        ngx_trial_producer::last_failure,ngx_trial_producer::state?ngx_trial_producer::state->surface_count:0,ngx_trial_producer::state?ngx_trial_producer::state->depth_count:0,
        ngx_trial_producer::state?ngx_trial_producer::state->motion_count:0,ngx_trial_producer::pso_queries,ngx_trial_producer::pso_verified);fflush(trial_events);
    if(ngx_trial_producer::state){auto const& s=*ngx_trial_producer::state;fprintf(trial_events,"producer_scope valid=%u failure=%u world=%llu ticket=%llu renderer=%llx current_world=%llu current_renderer=%llx typed_candidates=%u transition=%u\n",
        unsigned(s.valid),s.failure,s.world,s.ticket,s.renderer,committed_world_generation.load(),reinterpret_cast<uintptr_t>(consuming_renderer),s.candidate_count,unsigned(c&&c->last_input.native_transition));fflush(trial_events);}
    if(ngx_trial_producer::state){auto const& s=*ngx_trial_producer::state;auto const& m=s.motion_stages;auto const& v=s.last_motion_values;
        fprintf(trial_events,"motion_stages=%u,%u,%u,%u,%u,%u,%u last_values=%.9g,%.9g,%.9g,%.9g source=%u,%u,%u,%u constants_tables=%llu constant_uploads=%llu constant_copies=%llu\n",
            m[0],m[1],m[2],m[3],m[4],m[5],m[6],v[0],v[1],v[2],v[3],s.rectangles[0].x,s.rectangles[0].y,s.rectangles[0].width,s.rectangles[0].height,
            ngx_mv_constants_capture::table_attempts,ngx_mv_constants_capture::matches,ngx_mv_constants_capture::copies);fflush(trial_events);}
    if(ngx_trial_producer::state){auto const& s=*ngx_trial_producer::state;
        for(unsigned eye=0;eye<2;++eye){auto const& t=s.target_rectangles[eye];auto const& a=s.sampling[eye];
            fprintf(trial_events,"constructed_view eye=%u field_mask=%u target=%d,%d,%d,%d sampling=%.9g,%.9g,%.9g,%.9g\n",eye,s.view_field_mask,t[0],t[1],t[2],t[3],a[0],a[1],a[2],a[3]);}fflush(trial_events);}
}
#else
std::shared_ptr<Context> context;
#endif
bool refuse(Context& c,unsigned stage){c.failure_stage=stage;
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    trial_event(&c,"context_rejected",0,stage,dispatch_unavailable);
#endif
    return false;
}
uint64_t generation(ID3D12CommandList*,uintptr_t canonical){auto ref=frame_link.current(canonical);return ref.identity==canonical?ref.generation:0;}
bool read(void* out,void const* in,size_t size){return safe_copy(out,in,size);}
void reject(Context& c,unsigned site){
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    trial_event(&c,"context_invalidated",0,site,dispatch_unavailable);
#endif
    c.failed=true;if(auto owner=c.owner.lock()){owner->invalidate();request_session_stop(SessionStop::proof_failure);}}
// Called by successful detach_control while frame_hook_mutex is held. No stop,
// print, world change, failed recording or uncertain Release reaches this path.
void detached(std::shared_ptr<ngx_stereo_host::ControlOwner> const& owner){
    if(context&&context->owner.lock()==owner){
        context->failed=true;context->quartet.reset();context.reset();
    }
}
bool acquisition_scope(Context& c,uint64_t& ticket){
    gears_submitted::FrameTicket live;ticket=consuming_frame_ticket;
    return !c.failed&&c.quartet&&!c.owner.expired()&&valid(c.created.identity)&&recording.load()&&
        c.created.identity.world==committed_world_generation.load()&&consuming_renderer&&ticket&&
        consuming_frozen_frame.enabled&&consuming_frozen_frame.id==ticket&&
        consuming_frozen_frame.world_generation==c.created.identity.world&&
        submitted_lifetime.frame(reinterpret_cast<uintptr_t>(consuming_renderer),live)&&
        live.enabled&&live.id==ticket&&live.world_generation==c.created.identity.world;
}
void acquire_barriers(UINT count,D3D12_RESOURCE_BARRIER const* values){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);uint64_t ticket{};
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    ngx_trial_producer::barriers(count,values);
#endif
    if(!context||!acquisition_scope(*context,ticket))return;auto& c=*context;
    if(count>1024||(count&&!values)){c.quartet->uncertain();reject(c,__LINE__);return;}
    for(UINT i=0;i<count;++i){D3D12_RESOURCE_BARRIER value{};
        if(!safe_copy(&value,values+i,sizeof(value))){c.quartet->uncertain();reject(c,__LINE__);return;}
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
        // This trial requires full-buffer CDMO allocations; SDK-private scratch
        // textures cannot consume the quartet's finite candidate lease table.
        auto observe=[&](ID3D12Resource* resource){if(!resource)return true;const auto d=resource->GetDesc();
            if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||d.Width!=c.created.layout.full_output.width||d.Height!=c.created.layout.full_output.height)return true;
            return c.quartet->observe_resource(c.created.identity,ticket,resource);};
        bool acquired{};switch(value.Type){
        case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:acquired=observe(value.Transition.pResource);break;
        case D3D12_RESOURCE_BARRIER_TYPE_UAV:acquired=observe(value.UAV.pResource);break;
        case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:acquired=observe(value.Aliasing.pResourceBefore)&&observe(value.Aliasing.pResourceAfter);break;
        default:acquired=false;break;}
#else
        const bool acquired=acquire_quartet_barrier(*c.quartet,c.created.identity,ticket,value);
#endif
        if(!acquired){
            c.quartet->uncertain();reject(c,__LINE__);return;}
    }
}
void acquire_copy(D3D12_TEXTURE_COPY_LOCATION const* source,D3D12_TEXTURE_COPY_LOCATION const* destination){
    uint64_t ticket{};if(!context||!acquisition_scope(*context,ticket))return;auto& c=*context;
    D3D12_TEXTURE_COPY_LOCATION s{},d{};
    if(!safe_copy(&s,source,sizeof(s))||!safe_copy(&d,destination,sizeof(d))||
       !c.quartet->observe_resource(c.created.identity,ticket,s.pResource)||
       !c.quartet->observe_resource(c.created.identity,ticket,d.pResource)){
        c.quartet->uncertain();reject(c,__LINE__);
    }
}
void reset_command(ID3D12CommandList* list){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!context||context->failed||!list)return;
    auto& c=*context;auto owner=c.owner.lock();if(!owner)return;
    const auto id=canonical_identity(list);const auto gen=generation(list,id);
    ngx_final_xr::reset(id);
    if(!c.gpu.reset_success(owner->adapter(),list,id,gen))reject(c,__LINE__);
}
void created_command(HRESULT hr,void** out,bool open){
    if(ngx_final_xr::enabled()&&stereo_publish_bypass)return;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    const bool observing=ngx_stereo_host::attested&&recording.load()&&frame_link_diagnostics;
    const bool controlling=context&&!context->failed;
    if(FAILED(hr)){
#if defined(GEARS_NGX_EPOCH_PAIR_OBSERVER) && GEARS_NGX_EPOCH_PAIR_OBSERVER
        ngx_epoch_pair::creation_failed();
#endif
        return;
    }
    if(!observing&&!controlling)return;
    auto failed=[&](){if(context)reject(*context,__LINE__);
#if defined(GEARS_NGX_EPOCH_PAIR_OBSERVER) && GEARS_NGX_EPOCH_PAIR_OBSERVER
        ngx_epoch_pair::creation_failed();ngx_epoch_pair::probe.gap();
#endif
    };
    IUnknown* returned{};Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;
    // The successful typed API result is held during registration. Birth makes
    // a fresh generation even on address reuse; no permanent list COM cache.
    if(!safe_copy(&returned,out,sizeof(returned))||!returned||FAILED(returned->QueryInterface(IID_PPV_ARGS(&list)))){failed();return;}
    if(list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT)return;
    const auto id=canonical_identity(list.Get());const auto born=frame_link.create_success(id,open);
#if defined(GEARS_NGX_EPOCH_PAIR_OBSERVER) && GEARS_NGX_EPOCH_PAIR_OBSERVER
    ngx_epoch_pair::created_list(list.Get(),open,id&&born.identity);
#endif
    if(!id||!born.identity){failed();return;}
    // CreateCommandList1 is closed: only the actual Reset enables recording.
}
bool command(Context& c,uintptr_t raw,Command& out){
    out={};auto owner=c.owner.lock();if(c.failed||!owner||!raw){c.failure_stage=1;return false;}
    // raw is the preserved first argument of the authenticated complete native
    // Create/Evaluate wrapper: the SDK command-list ABI, not a resource field.
    // Copy uses its actual typed D3D list. Retain only at these owned call sites.
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;
    Microsoft::WRL::ComPtr<ID3D12Device> device;Microsoft::WRL::ComPtr<IUnknown> identity,device_id;
    if(FAILED(reinterpret_cast<IUnknown*>(raw)->QueryInterface(IID_PPV_ARGS(&list)))||
       list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT||FAILED(list.As(&identity))||
       FAILED(list->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device.As(&device_id))){c.failure_stage=2;return false;}
    const auto id=reinterpret_cast<uintptr_t>(identity.Get());const auto gen=generation(list.Get(),id);
    if(!gen||!c.gpu.retain_recording(owner->adapter(),list.Get(),id,gen)){c.failure_stage=3;return false;}
    out={raw,id,gen,reinterpret_cast<uintptr_t>(device_id.Get()),true};return true;
}
bool constructed(void* renderer,gears_submitted::FrameTicket const& frame,Record const& built,bool verified){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!context)return true;auto& c=*context;
    if(!frame.enabled||!frame.id)return true;
    if(c.failed)return c.owner.expired();
    if(!verified||!frame.enabled||!frame.id||built.family_count!=2){reject(c,__LINE__);return c.owner.expired();}
    Context::Construction* slot{};
    for(auto& old:c.constructions){gears_submitted::FrameTicket live;
        if(!old.renderer||!submitted_lifetime.frame(old.renderer,live)||live.id!=old.ticket||live.world_generation!=old.world){slot=&old;break;}}
    if(!slot){reject(c,__LINE__);return c.owner.expired();}
    *slot={frame.world_generation,frame.id,reinterpret_cast<uintptr_t>(renderer),{built.family_views[0],built.family_views[1]}};return true;
}
bool create(void* opaque,ngx_stereo_host::NativeCreateSource const& source,uintptr_t raw,ngx_stereo_host::ControlCreateBinding& out){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto& c=*static_cast<Context*>(opaque);out={};
    if(c.failed||source.world!=committed_world_generation.load()||!source.create_layout(out.layout)){c.failure_stage=20;return false;}
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    if(trial_packed_input){out.layout.packed_input=true;out.layout.input_base[1]={out.layout.input.width,0};}
#endif
    if(!command(c,raw,out.command))return false;
    out.identity={source.revision,source.world,out.command.device,source.parameters,source.revision};
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    if(valid(c.created.identity)){
        auto owner=c.owner.lock();if(!owner)return refuse(c,200);
        auto entry=owner->adapter().enter();
        if(!entry||!owner->adapter().drained(entry)||c.copy_pending||c.recreations==c.retired_quartets.size()||
           source.revision<=c.created.identity.epoch||source.world!=c.created.identity.world||out.identity.device!=c.created.identity.device||
           source.fields!=c.native_create_fields||!same(out.layout,c.created.layout)||
           !ngx_final_xr::rebind(c.created.identity,out.identity,out.layout.full_output))return refuse(c,201);
        // Keep all old typed leases in this pinned trial until process exit.
        // A new feature epoch receives an empty quartet and fresh producer
        // bindings, even when the native pool reuses the same allocation.
        c.retired_quartets[c.recreations++]=std::move(c.quartet);c.quartet=std::make_unique<TypedQuartet>();
        c.final_output_snapshot.reset();c.last_input={};c.last_eye=0;c.transport_accepted=false;c.epoch_retired=false;
        c.source_activity.hold(consuming_frame_ticket);
        trial_event(&c,"create_wrapper_rebind",raw,out.identity.epoch,named_result_success);
    }
#endif
    if(!c.quartet||!c.quartet->begin(out.identity)){c.failure_stage=21;return false;}
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    if(!ngx_mv_descriptor_capture::set_control_extent(out.layout.full_output))return refuse(c,202);
#endif
    out.creation_ticket=consuming_frame_ticket;c.created=out;
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    c.native_create_fields=source.fields;
    if(!ngx_trial_producer::active())return refuse(c,22);
    // These leases were acquired earlier in this same consume at typed API
    // barriers, before the first Create. No integer field is cast to COM.
    for(unsigned i=0;i<ngx_trial_producer::state->candidate_count;++i)
        if(!c.quartet->observe_resource(out.identity,consuming_frame_ticket,ngx_trial_producer::state->candidates[i].get()))return refuse(c,23);
    trial_event(&c,"create_binding",raw,uint64_t(out.layout.input.width)<<32|out.layout.input.height,1);
#endif
    return true;
}
bool input(void* opaque,ngx_stereo_host::ControlConsumption used,EvaluateShadow const& source,uintptr_t raw,ngx_stereo_host::ControlInputTicket& out){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto& c=*static_cast<Context*>(opaque);out={};Command cmd;gears_submitted::FrameTicket frame;
    c.failure_stage=30;
    if(c.failed||!c.producer||!c.producer_lifetime||used.renderer!=reinterpret_cast<uintptr_t>(consuming_renderer)||
       used.ticket!=consuming_frame_ticket||!consuming_frozen_frame.enabled||used.ticket!=consuming_frozen_frame.id||
       used.world!=consuming_frozen_frame.world_generation||used.world!=committed_world_generation.load()||
       !submitted_lifetime.frame(used.renderer,frame)||!frame.enabled||frame.id!=used.ticket||frame.world_generation!=used.world||!command(c,raw,cmd))return refuse(c,c.failure_stage);
    Context::Construction const* original{};for(auto const& built:c.constructions)
        if(built.world==used.world&&built.ticket==used.ticket&&built.renderer==used.renderer){original=&built;break;}
    if(!original)return refuse(c,31);Record live;inspect_completed_renderer(live,consuming_renderer);
    if(live.family_count!=2||live.renderer_count!=2||live.family_views[0]!=original->views[0]||live.family_views[1]!=original->views[1])return refuse(c,32);
    std::array<uintptr_t,4> native{};constexpr size_t offsets[]{0,8,0x18,0x20};
    for(unsigned i=0;i<4;++i)std::memcpy(&native[i],source.bytes.data()+offsets[i],sizeof(uintptr_t));
    std::array<Resource,4> retained{};
    if(!c.quartet||!c.quartet->bind(c.created.identity,used.ticket,native,retained))return refuse(c,33);
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    // The bounded diagnostic skips the first native pair. Control must bind
    // the actual first pair independently, from the retained typed quartet.
    TypedResourceLease actual_mv;
    if(!c.quartet->retained_native(c.created.identity,used.ticket,3,actual_mv))return refuse(c,37);
    const bool replacement_mv=c.recreations&&c.control_mv_epoch&&c.control_mv_epoch!=c.created.identity.epoch;
    if(!ngx_mv_descriptor_capture::seed_control_mv(actual_mv,replacement_mv)){
        if(trial_events)ngx_mv_descriptor_capture::print_control_seed(trial_events,actual_mv,replacement_mv);return refuse(c,37);}
    if(replacement_mv){if(trial_events)ngx_mv_descriptor_capture::print_control_seed(trial_events,actual_mv,true);
        trial_event(&c,"mv_epoch_rebind",actual_mv.raw,c.created.identity.epoch,named_result_success);}
    c.control_mv_epoch=c.created.identity.epoch;
    if(!c.quartet->retained_native(c.created.identity,used.ticket,1,c.pixel_output))return refuse(c,38);
    if(!c.quartet->retained_native(c.created.identity,used.ticket,0,c.pixel_color))return refuse(c,38);
    if(!ngx_final_xr::ensure(c.pixel_color.get(),c.created.identity,c.created.layout.full_output))return refuse(c,41);
#endif
    if(!c.producer(c.producer_lifetime.get(),c.created,used,source,cmd,retained,out)||out.original_views!=original->views)return refuse(c,34);
    std::array<Point,3> bases;for(unsigned i=0;i<3;++i)std::memcpy(&bases[i],source.bytes.data()+0x60+8*i,8);
    const Extent rendered{out.producers[0].rendered[0].width,out.producers[0].rendered[0].height};
    if(!select_input_eye(c.created.layout,c.created.layout.packed_input?rendered:c.created.layout.input,bases,c.last_eye))return false;
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    if(!c.last_eye)c.final_output_snapshot.reset();
    if(c.created.layout.packed_input&&c.last_eye==1&&!c.packed_stage.right_ready(used.ticket))return refuse(c,39);
#endif
    constexpr unsigned order[]{0,2,3,1};
    for(unsigned i=0;i<4;++i)if(out.raw_resources[i]!=native[order[i]]||
        !same_resource(out.producers[c.last_eye].resources[i],retained[i]))return false;
    c.last_input=out;c.failure_stage=0;c.copy_pending=true;
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    if(trial_pre_eval_pixels&&c.transport_accepted&&!out.native_transition&&ngx_trial_producer::inputs_accepted>=64){
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> native_list;
        // Same authenticated SDK command ABI already verified and retained by
        // command() above; resource pointers come only from typed epoch leases.
        if(FAILED(reinterpret_cast<IUnknown*>(raw)->QueryInterface(IID_PPV_ARGS(&native_list))))return refuse(c,40);
        auto const& r=out.producers[c.last_eye].rendered[0];
        ngx_output_pixels::pre_evaluate_capture(native_list.Get(),c.pixel_output.get(),c.pixel_color.get(),c.last_eye,{r.x,r.y,r.width,r.height});
    }
#endif
    return true; // Adapter still requires actual successful SDK return before Copy.
}
Completion completion(void* opaque,uint64_t handle){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto& c=*static_cast<Context*>(opaque);auto owner=c.owner.lock();
    c.last_native_completion={};
    if(!owner)return {};auto entry=owner->adapter().enter();if(!entry||!owner->adapter().owns(entry,handle))return {};
    c.last_native_completion=c.gpu.completion();return c.last_native_completion;
}
// Called by ControlOwner before its producer callback and adapter gate. The
// wrapper's caller still owns the native renderer; never return/defer/replay it.
// All queue Signals needed below have ALREADY returned. Sleep holds neither
// frame_hook_mutex nor an adapter Entry, so submission/stop hooks can progress.
bool admit(void* opaque,ngx_stereo_host::ControlConsumption used){
    auto& c=*static_cast<Context*>(opaque);const auto start=GetTickCount64();
    constexpr ULONGLONG first_pair_timeout_ms=2000;
    for(;;){
        {
            std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto owner=c.owner.lock();
            if(!owner||c.failed||!recording.load()||submitted_lifetime.stopping()||
               used.world!=committed_world_generation.load()||used.world!=c.created.identity.world){
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
                if(trial_events){fprintf(trial_events,"ticket_admission_guard ticket=%llu owner=%u failed=%u recording=%u stopping=%u world=%llu current_world=%llu created_world=%llu stop_source_file=%s stop_source_line=%u\n",
                    used.ticket,unsigned(bool(owner)),unsigned(c.failed),unsigned(recording.load()),unsigned(submitted_lifetime.stopping()),used.world,committed_world_generation.load(),c.created.identity.world,session_stop_source_file.load(),session_stop_source_line.load());
#if defined(GEARS_NGX_OWNED_SCENE_TRIAL) && GEARS_NGX_OWNED_SCENE_TRIAL
                    owned_ui_live::progress(trial_events);fprintf(trial_events,"\n");
                    dlss_output_control::print(trial_events);
#endif
                    fflush(trial_events);}
#endif
                reject(c,__LINE__);return false;}
            TicketAdmission admission;[[maybe_unused]] AcceptancePhase phase_before{},phase_after{};Completion observed_completion{};
            {auto entry=owner->adapter().enter();if(!entry){reject(c,__LINE__);return false;}
                phase_before=owner->adapter().phase(entry);observed_completion=c.gpu.completion();
                admission=owner->adapter().admit_ticket(entry,used.world,used.ticket,observed_completion);
                phase_after=owner->adapter().phase(entry);}
            if(admission==TicketAdmission::admitted){
                // admit_ticket polls the same actual fence obligations as
                // Execute, and may be the callback that accepts the candidate.
                // Keep the outer POD gate synchronized before this frame draws.
                if(phase_after==AcceptancePhase::accepted&&!c.transport_accepted){
                    c.transport_accepted=true;
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
                    if(trial_events){fprintf(trial_events,"ticket_transport_accepted ticket=%llu phase_before=%u phase_after=%u completion=%llu device=%llx queue=%llx fence=%llx qpc=%lld\n",
                        used.ticket,unsigned(phase_before),unsigned(phase_after),observed_completion.value,observed_completion.device,observed_completion.queue,observed_completion.fence,now());fflush(trial_events);}
#endif
                }
                return true;
            }
            if(admission==TicketAdmission::rejected||GetTickCount64()-start>=first_pair_timeout_ms){
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
                if(trial_events){fprintf(trial_events,"ticket_admission_failure ticket=%llu admission=%u phase_before=%u phase_after=%u elapsed_ms=%llu completion=%llu device=%llx queue=%llx fence=%llx gpu_failed=%u gpu_failure_line=%u adapter_failure_line=%u last_input_ticket=%llu last_eye=%u copy_pending=%u\n",
                    used.ticket,unsigned(admission),unsigned(phase_before),unsigned(phase_after),GetTickCount64()-start,observed_completion.value,
                    observed_completion.device,observed_completion.queue,observed_completion.fence,unsigned(c.gpu.failed()),c.gpu.failure_line(),owner->adapter().failure_line(),c.last_input.consumption.ticket,c.last_eye,unsigned(c.copy_pending));fflush(trial_events);}
#endif
                reject(c,__LINE__);return false;}
        }
        Sleep(10); // Only normal pending first-pair GPU completion reaches here.
    }
}
void boxed_dispatch(void* opaque,CopyPlan const& plan){auto& c=*static_cast<Context*>(opaque);
    if(c.failed||!c.copy_boundary||!c.copy_boundary->copy(plan))throw std::runtime_error("NGX actual copy boundary rejected");
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    if(c.created.layout.packed_input){PackedBackend backend{c.packed_buffers,*c.copy_boundary};
        if(!(plan.ordinal?c.packed_stage.restore_output(backend,plan.ticket):c.packed_stage.restore_input(backend,plan.ticket)))throw std::runtime_error("NGX packed restoration rejected");}
#endif
}
bool current(void*,ID3D12GraphicsCommandList* list,uintptr_t id,uint64_t gen){return canonical_identity(list)==id&&generation(list,id)==gen;}
bool copy(ID3D12GraphicsCommandList* list,D3D12_TEXTURE_COPY_LOCATION const* dst,UINT x,UINT y,UINT z,
    D3D12_TEXTURE_COPY_LOCATION const* src,D3D12_BOX const* box,uintptr_t caller,ngx_stereo_host::CopyFn forward){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto owner=ngx_stereo_host::current_control();if(!owner)return false;
    if(!context||context->owner.lock()!=owner){owner->invalidate();request_session_stop(SessionStop::proof_failure);return true;}
    acquire_copy(src,dst);
    auto& c=*context;D3D12_TEXTURE_COPY_LOCATION s{},d{};
    if(!safe_copy(&s,src,sizeof(s))||!safe_copy(&d,dst,sizeof(d))){reject(c,__LINE__);return true;}
    if(reinterpret_cast<uintptr_t>(s.pResource)!=c.last_input.raw_resources[3]||reinterpret_cast<uintptr_t>(d.pResource)!=c.last_input.raw_resources[0])return false;
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    if(trial_events&&trial_copy_stack_traces++<16){void* frames[12]{};const auto captured=RtlCaptureStackBackTrace(0,12,frames,nullptr);
        fprintf(trial_events,"native_color_copy_stack ticket=%llu previous_input_ticket=%llu pending_eval_copy=%u source=%llx target=%llx captured=%u addresses=",
            consuming_frame_ticket,c.last_input.consumption.ticket,unsigned(c.copy_pending),reinterpret_cast<uintptr_t>(s.pResource),reinterpret_cast<uintptr_t>(d.pResource),unsigned(captured));
        for(unsigned i=0;i<captured;++i)fprintf(trial_events,"%s%llx",i?",":"",reinterpret_cast<uintptr_t>(frames[i]));fprintf(trial_events,"\n");fflush(trial_events);}
#endif
    // The native graph also uses these scratch textures for full-buffer
    // preprocessing copies before DLSS. Resource equality and caller RVA alone
    // do not identify the one post-Evaluate copy. Only this ticket's pending
    // eye can enter the boxed-copy adapter.
    if(!c.copy_pending||c.last_input.consumption.ticket!=consuming_frame_ticket||
       c.last_input.consumption.renderer!=reinterpret_cast<uintptr_t>(consuming_renderer)){
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
        uint64_t current_ticket{};auto const& raw=c.last_input.raw_resources;
        if(!trial_preprocess_preserve)return false; // The unsuccessful region hypothesis is opt-in only.
        if(c.failed||!acquisition_scope(c,current_ticket)||!c.quartet->bound()||
            !preserves_native_copy(ngx_stereo_host::attested,caller,d,x,y,z,s,box,uintptr_t(raw[3]),uintptr_t(raw[0])))return false;
        auto source=TypedResourceLease::observe(s.pResource),target=TypedResourceLease::observe(d.pResource);Resource a{},b{};Rect region{};
        if(!source.materialize(a)||!target.materialize(b)||a.device!=c.created.identity.device||b.device!=a.device||
            !same(a.extent,c.created.layout.full_output)||!same(b.extent,a.extent)||a.format!=26||b.format!=26)return false;
        const bool selected=ngx_trial_producer::native_preprocess_region(a.identity,region);
        if(trial_events&&trial_preprocess_traces++<32){fprintf(trial_events,"native_preprocess_copy ticket=%llu source=%llx target=%llx selected=%u region=%u,%u,%u,%u\n",
            current_ticket,a.identity,b.identity,unsigned(selected),region.x,region.y,region.width,region.height);fflush(trial_events);}
        if(!selected){++trial_preprocess_unresolved;return false;}
        // State transitions and flush stay with the authenticated native copy.
        // Preserve the untouched eye in Color while copying this current draw.
        const D3D12_BOX region_box{region.x,region.y,0,region.x+region.width,region.y+region.height,1};
        forward(list,&d,region.x,region.y,0,&s,&region_box);++trial_preprocess_boxed;return true;
#else
        return false;
#endif
    }
    Command cmd;uint64_t ticket{};std::array<Resource,4> retained{};
    auto const& raw=c.last_input.raw_resources;
    const std::array<uintptr_t,4> native{uintptr_t(raw[0]),uintptr_t(raw[3]),uintptr_t(raw[1]),uintptr_t(raw[2])};
    if(c.failed||c.copy_boundary||c.last_eye>1||!acquisition_scope(c,ticket)||
       c.last_input.consumption.ticket!=ticket||c.last_input.consumption.world!=c.created.identity.world||
       c.last_input.consumption.renderer!=reinterpret_cast<uintptr_t>(consuming_renderer)||
       !c.quartet->bound()||!c.quartet->bind(c.created.identity,ticket,native,retained)||
       !preserves_native_copy(ngx_stereo_host::attested,caller,d,x,y,z,s,box,uintptr_t(raw[3]),uintptr_t(raw[0]))||
       !command(c,reinterpret_cast<uintptr_t>(list),cmd)||cmd.device!=c.created.identity.device){reject(c,__LINE__);return true;}
    auto const& producer=c.last_input.producers[c.last_eye];
    if(!same(producer.use.identity,c.created.identity)||producer.use.ticket!=ticket||producer.use.eye!=c.last_eye){reject(c,__LINE__);return true;}
    for(unsigned i=0;i<4;++i)if(!same_resource(retained[i],producer.resources[i])){reject(c,__LINE__);return true;}
    CopyFacts facts;facts.identity=c.created.identity;facts.ticket=c.last_input.consumption.ticket;facts.caller=caller;
    facts.list=cmd.list;facts.generation=cmd.generation;facts.list_device=cmd.device;facts.direct=cmd.direct;
    // Legal-state requirement is preserved from the attested native call,
    // not reconstructed from last-observed barriers. No barriers are added.
    facts.copy_source=facts.copy_dest=true;facts.resources=retained;
    for(unsigned eye=0;eye<2;++eye){auto const& r=c.last_input.producers[eye].rendered[0];facts.input_base[eye]={r.x,r.y};}
    facts.feature_output=c.created.layout.eye_output;facts.evaluated_ordinal=c.last_eye;
    TypedCopyBoundary boundary(list,cmd.list,cmd.generation,facts.ticket,c.last_eye,d,s,&c,current,forward);
    struct Scope {Context& c;Scope(Context& value,TypedCopyBoundary& b):c(value){c.copy_boundary=&b;}~Scope(){c.copy_boundary=nullptr;}} scope(c,boundary);
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    if(c.created.layout.packed_input&&!c.last_eye){
        if(!c.packed_buffers.ensure(boundary,c.created.layout.eye_output)){reject(c,__LINE__);return true;}
        PackedBackend backend{c.packed_buffers,boundary};auto const& right=c.last_input.producers[1].rendered[0];
        PackedRequest request{c.created.identity,ticket,cmd.list,cmd.generation,{},right,c.created.layout.eye_output};
        for(unsigned i=0;i<4;++i)request.resources[i]=backend.metadata(PackedRole(i));
        if(!c.packed_stage.backup(backend,request)){reject(c,__LINE__);return true;}
        facts.input_backup=c.packed_stage.input_backup();facts.output_backup=c.packed_stage.output_backup();
    }
#endif
    bool ok{};try{auto entry=owner->adapter().enter();ok=entry&&owner->adapter().copy(entry,facts,c.last_eye);
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
        // Freeze the receipt using this already-owned Entry at the actual copy
        // seam. Native final-output observers must never acquire the SDK API
        // order while their caller may already hold a frame/renderer lock.
        if(ok&&c.last_eye==1){RecordedOutput output;
            if(owner->adapter().recorded_output(entry,output)){
                if(trial_final_output_join)c.final_output_snapshot=output;
                if(trial_input_union_mask&&!trial_packed_input)c.source_activity.accepted_pair(output.ticket);}}
#endif
    }catch(...){reject(c,__LINE__);throw;}
    if(!ok)reject(c,__LINE__);else c.copy_pending=false;return true; // Selected owned copy never falls back to full Output->Color.
}
void execute(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* lists,ngx_stereo_host::ExecuteFn forward){
    if(ngx_final_xr::enabled()&&stereo_publish_bypass){forward(queue,count,lists);return;}
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto owner=ngx_stereo_host::current_control();
    if(!owner){forward(queue,count,lists);return;}
    if(!context||context->owner.lock()!=owner){owner->invalidate();request_session_stop(SessionStop::proof_failure);forward(queue,count,lists);return;}
    if(!context->gpu.execute(owner->adapter(),queue,count,lists,forward,read,generation)){context->failure_stage=1000+context->gpu.failure_line();reject(*context,__LINE__);return;}
    TransportCompletion transport;
    {auto entry=owner->adapter().enter();if(!entry){reject(*context,__LINE__);return;}
        transport=owner->adapter().complete_candidate_transport(entry,context->gpu.completion());
        context->adapter_failure_line=owner->adapter().failure_line();context->adapter_phase=unsigned(owner->adapter().phase(entry));}
    if(transport==TransportCompletion::accepted)context->transport_accepted=true;
    if(transport==TransportCompletion::retired){context->transport_accepted=false;
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
        context->final_output_snapshot.reset();context->epoch_retired=true;
#endif
        return;}
    if(transport==TransportCompletion::rejected)reject(*context,__LINE__);
    if(!context->failed)ngx_final_xr::executed(queue,count,lists);
}
// Prepare before the native Create/Eval wrappers. Activate only when
// the actual producer source and full output/retirement gates are wired.
// No call is inserted here; this is a host installation seam, not a user option.
bool activate(AdapterPrerequisites prerequisites){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!context||context->failed||!context->producer||!context->producer_lifetime||!context->owner.expired())return false;
    auto& c=*context;NativeDispatchFunctions functions{ngx_stereo_host::original_create,ngx_stereo_host::original_evaluate,ngx_stereo_host::original_release,&c,boxed_dispatch};
    functions.optimal=ngx_stereo_host::native_optimal_query();
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
    functions.trace_context=&c;functions.trace=trial_event;
#endif
    ngx_stereo_host::ControlCallbacks callbacks{context,create,input,completion,admit};
    auto owner=std::make_shared<ngx_stereo_host::ControlOwner>(functions,prerequisites,callbacks);
    c.owner=owner;if(ngx_stereo_host::attach_control(owner))return true;c.owner.reset();return false;
}
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
bool prepare_trial(FILE* file){
    // Background diagnostics and explicit user-operated owned-scene testing
    // share renderer proofs, but have separate startup/audio admission.
    auto test_flag=[](wchar_t const* name){wchar_t value[8]{};return GetEnvironmentVariableW(name,value,8)==1&&value[0]==L'1';};
    bool manual_compiled=false,owned_scene=false;
#if defined(GEARS_NGX_MANUAL_OWNED_TEST) && GEARS_NGX_MANUAL_OWNED_TEST
    manual_compiled=true;
#endif
#if defined(GEARS_NGX_OWNED_SCENE_TRIAL) && GEARS_NGX_OWNED_SCENE_TRIAL
    owned_scene=true;
#endif
    const auto launch_mode=gears_ngx_trial_launch::mode({ngx_stereo_host::attested,manual_compiled,
        test_flag(L"GEARSVR_TEST_NGX_MANUAL_OWNED"),owned_scene,test_flag(L"GEARSVR_TEST_NGX_FINAL_XR"),
        test_flag(L"GEARSVR_TEST_WINDOWS_BEHIND"),test_flag(L"GEARSVR_TEST_MUTE_GAME"),test_flag(L"GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY")});
    const bool allowed=launch_mode!=gears_ngx_trial_launch::Mode::rejected;
    fprintf(file,"ngx_isolated_stereo_trial prepared=%u default_off=1 private_background_test_required=%u manual_owned_test=%u\n",unsigned(allowed),
        unsigned(launch_mode!=gears_ngx_trial_launch::Mode::manual_owned),unsigned(launch_mode==gears_ngx_trial_launch::Mode::manual_owned));
    if(!allowed)return false;
    trial_preprocess_pixels=test_flag(L"GEARSVR_TEST_NGX_PREPROCESS_READBACK");
    trial_scene_color_pixels=test_flag(L"GEARSVR_TEST_NGX_SCENE_COLOR_READBACK");
    trial_output_target_rects=test_flag(L"GEARSVR_TEST_NGX_OUTPUT_TARGET_RECT");
    trial_preprocess_preserve=test_flag(L"GEARSVR_TEST_NGX_PREPROCESS_PRESERVE_REGIONS");
    trial_packed_input=test_flag(L"GEARSVR_TEST_NGX_PACKED_INPUT");if(trial_packed_input&&trial_output_target_rects)return false;
    trial_right_draw_pixels=test_flag(L"GEARSVR_TEST_NGX_RIGHT_DRAW_READBACK");
    trial_pre_eval_pixels=test_flag(L"GEARSVR_TEST_NGX_PRE_EVAL_READBACK");
    trial_baseline_pixels=test_flag(L"GEARSVR_TEST_NGX_NO_SR_READBACK");
    trial_input_union_mask=test_flag(L"GEARSVR_TEST_NGX_INPUT_UNION_MASK");
    if(trial_input_union_mask&&!trial_packed_input&&!ngx_mv_descriptor_capture::enable_control_epochs())return false;
    trial_color_after_pixels=test_flag(L"GEARSVR_TEST_NGX_POST_COPY_COLOR_READBACK");
    trial_final_target_pixels=test_flag(L"GEARSVR_TEST_NGX_FINAL_TARGET_READBACK");
#if defined(GEARS_NGX_FINAL_OUTPUT_JOIN_TRIAL) && GEARS_NGX_FINAL_OUTPUT_JOIN_TRIAL
    trial_final_output_join=test_flag(L"GEARSVR_TEST_NGX_FINAL_OUTPUT_JOIN");
#endif
    const bool final_xr_requested=test_flag(L"GEARSVR_TEST_NGX_FINAL_XR");
    const bool final_xr_enabled=ngx_final_xr::enable(final_xr_requested&&trial_final_output_join&&trial_input_union_mask&&!trial_packed_input,file);
    if(final_xr_requested&&!final_xr_enabled)return false;
#if defined(GEARS_NGX_OWNED_SCENE_TRIAL) && GEARS_NGX_OWNED_SCENE_TRIAL
    const bool owned_join=test_flag(L"GEARSVR_TEST_NGX_OWNED_SCENE")&&trial_final_output_join&&trial_input_union_mask&&
        !trial_packed_input&&!final_xr_requested&&owned_bindings_enabled&&owned_xr_probe;
    fprintf(file,"ngx_owned_scene_join enabled=%u native_owner_target_required=1 output_return_and_ui_required=1\n",unsigned(owned_join));
    if(!owned_join)return false;
#endif
    fprintf(file,"ngx_trial_preprocess_pixel_mode enabled=%u capture=first_two_pre_eval_native_sources sdk_correlation_claim=0\n",unsigned(trial_preprocess_pixels));
    HMODULE pinned{};if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&trial_events),&pinned))return false;
    fprintf(file,"ngx_isolated_stereo_trial resident_until_normal_test_process_exit=1\n");
    wchar_t path[MAX_PATH]{};const auto length=GetModuleFileNameW(own_module,path,MAX_PATH);if(!length||length>=MAX_PATH)return false;
    auto* separator=wcsrchr(path,L'\\');if(!separator)return false;wcscpy_s(separator+1,MAX_PATH-size_t(separator+1-path),L"ngx-isolated-trial-events.txt");
    if(GetFileAttributesW(path)!=INVALID_FILE_ATTRIBUTES)return false;
    trial_events=_wfsopen(path,L"w",_SH_DENYNO);if(!trial_events)return false;
    context=std::make_shared<Context>();ngx_trial_producer::state=std::make_shared<ngx_trial_producer::State>();
    ngx_trial_producer::enabled=true;context->producer_lifetime=ngx_trial_producer::state;context->producer=ngx_trial_producer::input;
    return true;
}
bool trial_before_create(){
    if(!ngx_trial_producer::enabled)return true;if(ngx_stereo_host::current_control())return true;
    if(!context||context->failed||!ngx_trial_producer::active())return false;
    // Current byte contracts cover complete wrappers, dispatcher/release ABI,
    // source construction and native transition/copy helpers. The live input
    // callback still requires each ticket's typed quartet and actual domains.
    AdapterPrerequisites prerequisites{true,true,true,true,true,true,true,true,true,true,true,true};
    prerequisites.allow_native_transition_bootstrap=true;
    prerequisites.packed_input_staging=trial_packed_input;
    return activate(prerequisites);
}
void print_trial(FILE* file){
    ngx_final_xr::print(file);
    if(context)fprintf(file,"ngx_trial_recreation count=%u retired_quartets=%u limit=8 epoch_retired=%u\n",context->recreations,context->recreations,unsigned(context->epoch_retired));
    if(context)fprintf(file,"ngx_trial_source_activity constructor_eligible=%u suppressed=%llu gap_frames=%llu recoveries=%llu cached_handle_not_activity_proof=1\n",
        unsigned(context->source_activity.eligible()),context->constructor_source_suppressed,context->source_activity.gap_frames,context->source_activity.recoveries);
    fprintf(file,"ngx_trial_preprocess boxed=%llu unresolved=%llu native_copy_state_preserved=1 tls_eye_used=0 pixel_correctness_verified=0\n",trial_preprocess_boxed,trial_preprocess_unresolved);
    ngx_trial_producer::print(file);auto owner=ngx_stereo_host::current_control();
    if(context)fprintf(file,"ngx_packed_staging enabled=%u failed=%u backups_recorded=%llu right_restores_recorded=%llu left_restores_recorded=%llu final_publication_join=0\n",unsigned(trial_packed_input),unsigned(context->packed_stage.failed()),context->packed_stage.backups,context->packed_stage.input_restores,context->packed_stage.output_restores);
    if(context)fprintf(file,"ngx_final_target_pixels enabled=%u typed_targets=%u typed_limit=16 resource_ownership_only=1\n",unsigned(trial_final_target_pixels),context->final_target_count);
    if(!owner){fprintf(file,"ngx_isolated_stereo_owner attached=0\n");return;}
    auto e=owner->adapter().enter();if(!e){fprintf(file,"ngx_isolated_stereo_owner attached=1 busy=1\n");return;}
    auto handles=owner->adapter().owned_handles(e);fprintf(file,"ngx_isolated_stereo_owner attached=1 failed=%u phase=%u handles=%llx,%llx typed_quartet_bound=%u gpu_failed=%u retained_recordings=%zu inflight_batches=%zu\n",
        unsigned(context&&context->failed),unsigned(owner->adapter().phase(e)),handles[0],handles[1],unsigned(context&&context->quartet&&context->quartet->bound()),
        unsigned(context&&context->gpu.failed()),context?context->gpu.retained_recordings():0,context?context->gpu.inflight_batches():0);
}
bool pixel_capture_ready(){
    auto owner=ngx_stereo_host::current_control();if(!owner||!context||context->failed||context->last_input.native_transition||
        ngx_trial_producer::inputs_accepted<64)return false;
    auto entry=owner->adapter().enter();return entry&&owner->adapter().phase(entry)==AcceptancePhase::accepted;
}
bool preprocess_pixel_mode(){return trial_preprocess_pixels;}
bool scene_color_pixel_mode(){return trial_scene_color_pixels;}
bool right_draw_pixel_mode(){return trial_right_draw_pixels;}
bool pre_eval_pixel_mode(){return trial_pre_eval_pixels;}
bool baseline_pixel_mode(){return trial_baseline_pixels;}
bool final_target_pixel_mode(){return trial_final_target_pixels;}
void acquire_final_target_barriers(UINT count,D3D12_RESOURCE_BARRIER const* values){
    if((!trial_final_target_pixels&&!ngx_final_xr::enabled())||!recording.load()||count>1024||(count&&!values))return;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!context||context->failed||!context->created.identity.device)return;
    auto& c=*context;for(UINT i=0;i<count;++i){D3D12_RESOURCE_BARRIER b{};
        if(!safe_copy(&b,values+i,sizeof b))return;
        if(b.Type!=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION||!b.Transition.pResource)continue;
        bool known{};for(unsigned j=0;j<c.final_target_count;++j)if(c.final_targets[j].raw==reinterpret_cast<uintptr_t>(b.Transition.pResource))known=true;
        if(known||c.final_target_count==c.final_targets.size())continue;
        auto desc=b.Transition.pResource->GetDesc();if(desc.Format!=DXGI_FORMAT_R10G10B10A2_UNORM||desc.Width!=c.created.layout.full_output.width||desc.Height!=c.created.layout.full_output.height)continue;
        auto lease=TypedResourceLease::observe(b.Transition.pResource);Resource resource;
        if(lease.materialize(resource)&&resource.device==c.created.identity.device)c.final_targets[c.final_target_count++]=std::move(lease);
    }
}
bool final_target_pixel_pair(uintptr_t target,ID3D12Resource*& source,ID3D12Resource*& color,unsigned& eye,Rect& destination){
    source=color=nullptr;eye=0;destination={};
#if defined(GEARS_DLSS_OUTPUT_CONTROL) && GEARS_DLSS_OUTPUT_CONTROL
    if((!trial_final_target_pixels&&!ngx_final_xr::enabled())||!context||!ngx_trial_producer::active()||ngx_trial_producer::inputs_accepted<64||!dlss_output_control::scoped())return false;
    auto* q=dlss_output_control::current_quad;if(!q||!q->output||!q->output->sdk_output||!q->output->proof.draw.candidate||
       !final_output_current(*q->output->sdk_output)||q->output->proof.draw.target!=target)return false;
    eye=q->output->before.eye;destination={uint32_t(q->forwarded[0]),uint32_t(q->forwarded[1]),uint32_t(q->forwarded[2]),uint32_t(q->forwarded[3])};
    for(unsigned i=0;i<context->final_target_count;++i)if(context->final_targets[i].canonical==target)source=context->final_targets[i].get();
    color=context->pixel_color.get();return source&&color&&source!=color;
#else
    (void)target;return false;
#endif
}
bool post_copy_color_pixels(){return trial_packed_input||trial_color_after_pixels;}
bool output_route_sample_ready(){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
#if defined(GEARS_NGX_OWNED_SCENE_TRIAL) && GEARS_NGX_OWNED_SCENE_TRIAL
    const bool ready=context&&!context->failed&&valid(context->created.identity)&&
        context->created.identity.world==committed_world_generation.load()&&ngx_trial_producer::active();
    static unsigned traces{};
    if(ready&&traces<8&&trial_events){auto const& c=*context;++traces;
        fprintf(trial_events,"owned_output_entry row=%u ticket=%llu epoch=%llu input_ticket=%llu eye=%u native_transition=%u transport_accepted=%u copy_pending=%u qpc=%lld\n",
            traces,consuming_frame_ticket,c.created.identity.epoch,c.last_input.consumption.ticket,c.last_eye,unsigned(c.last_input.native_transition),unsigned(c.transport_accepted),unsigned(c.copy_pending),now());fflush(trial_events);}
    return ready; // Observe even when native Output precedes this ticket's SDK calls.
#else
    constexpr unsigned minimum_inputs=64;
    return context&&!context->failed&&context->transport_accepted&&!context->last_input.native_transition&&
        ngx_trial_producer::inputs_accepted>=minimum_inputs&&ngx_trial_producer::active()&&!context->copy_pending&&
        context->last_input.consumption.ticket==consuming_frame_ticket;
#endif
}
bool final_output_current(RecordedOutput const& output){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    return trial_final_output_join&&trial_input_union_mask&&context&&!context->failed&&context->transport_accepted&&
        !context->owner.expired()&&same(output.identity,context->created.identity)&&output.ticket==consuming_frame_ticket&&
        output.identity.world==committed_world_generation.load()&&context->last_input.consumption.ticket==output.ticket&&
        context->last_input.consumption.renderer==reinterpret_cast<uintptr_t>(consuming_renderer)&&
        context->last_eye==1&&!context->copy_pending&&output.color==context->pixel_color.canonical;
}
bool final_output_ticket(uint64_t world,uint64_t ticket,uintptr_t renderer,uintptr_t view,RecordedOutput& output){
    output={};if(!trial_final_output_join)return false;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!context||!context->final_output_snapshot||context->failed||world!=committed_world_generation.load()||ticket!=consuming_frame_ticket||
       renderer!=reinterpret_cast<uintptr_t>(consuming_renderer)||!consuming_frozen_frame.enabled||
       consuming_frozen_frame.id!=ticket||consuming_frozen_frame.world_generation!=world)return false;
    auto owner=context->owner.lock();if(!owner||owner!=ngx_stereo_host::current_control())return false;
    bool constructed{};for(auto const& built:context->constructions)if(built.world==world&&built.ticket==ticket&&built.renderer==renderer&&
        (view==built.views[0]||view==built.views[1])&&built.views==context->last_input.original_views)constructed=true;
    if(!constructed||!final_output_current(*context->final_output_snapshot))return false;
    output=*context->final_output_snapshot;return true;
}

bool input_union_mask_trial(uint64_t world,uint64_t ticket,uintptr_t renderer,uintptr_t storage){
    if(!trial_input_union_mask||!context||context->failed||context->owner.expired())return false;
    for(auto const& built:context->constructions)if(built.world==world&&built.ticket==ticket&&built.renderer==renderer&&
       built.views[0]==storage&&built.views[1]==storage+0xd30)return true;
    return false;
}
bool constructor_source_enabled(){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!trial_input_union_mask||trial_packed_input||!context||!valid(context->created.identity))return true;
    const bool eligible=!context->failed&&!context->epoch_retired&&context->source_activity.eligible();
    if(!eligible)++context->constructor_source_suppressed;return eligible;
}
#if defined(GEARS_NGX_OWNED_SCENE_TRIAL) && GEARS_NGX_OWNED_SCENE_TRIAL
bool native_output_ticket(uint64_t world,uint64_t ticket,uintptr_t renderer,uintptr_t view,gears_dlss_output::NativeGap& out,bool preserve_candidate){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);out={};
    if(!context||context->failed||context->epoch_retired||context->copy_pending||context->last_eye!=1||
       !world||world!=committed_world_generation.load()||!ticket||ticket!=consuming_frame_ticket||renderer!=reinterpret_cast<uintptr_t>(consuming_renderer)||
       !consuming_frozen_frame.enabled||consuming_frozen_frame.id!=ticket||consuming_frozen_frame.world_generation!=world)return false;
    auto const& c=*context;auto owner=c.owner.lock();auto const& prior=c.last_input.consumption;
    const bool candidate=prior.ticket==ticket&&(c.last_input.native_transition||!c.transport_accepted||preserve_candidate);
    const bool gap=prior.ticket&&prior.ticket<ticket&&c.transport_accepted;
    if(!owner||owner!=ngx_stereo_host::current_control()||!valid(c.created.identity)||c.created.identity.world!=world||
       prior.world!=world||(!candidate&&!gap)||!same(c.last_input.producers[1].use.identity,c.created.identity))return false;
    Context::Construction const* built{};for(auto const& row:c.constructions)if(row.world==world&&row.ticket==ticket&&row.renderer==renderer&&
       (view==row.views[0]||view==row.views[1]))built=&row;
    if(!built)return false;auto const& l=c.created.layout;
    out.world=world;out.ticket=ticket;out.prior_sdk_ticket=prior.ticket;out.renderer=renderer;out.epoch=c.created.identity;out.views=built->views;
    out.unpublished_sdk=candidate;
    out.output={l.full_output.width,l.full_output.height};
    // Create parameters deliberately contain no physical allocations. Use the
    // retained typed Color topology; the current native binding is independently
    // re-read/checked by BindScope and the actual draw, including any new Color.
    Resource retained_color;
    if(!c.pixel_color.materialize(retained_color)||retained_color.identity!=c.last_input.producers[1].resources[0].identity||
       !gears_dlss_output::set_native_texture(out,retained_color))return false;
    auto const& f=consuming_frozen_frame;
    if(f.ngx_constructor_source.revision){
        if(f.ngx_constructor_source.world!=world||!ngx_stereo_host::native_create_matches(f.ngx_constructor_source))return false;
        for(unsigned eye=0;eye<2;++eye)for(unsigned i=0;i<4;++i){const float value=f.ngx_constructor_rectangles[eye][i];
            if(!std::isfinite(value)||value<0||value>8192||std::floor(value)!=value)return false;out.input[eye][i]=unsigned(value);}
    }else for(unsigned eye=0;eye<2;++eye)out.input[eye]={eye*(out.output[0]/2),0,out.output[0]/2,out.output[1]};
    return true;
}
bool native_output_current(gears_dlss_output::NativeGap const& expected){
    // A candidate stays unpublished even if GPU acceptance advances during its
    // native scope. Revalidate all identity/geometry; never upgrade its witness.
    gears_dlss_output::NativeGap fresh;return native_output_ticket(expected.world,expected.ticket,expected.renderer,expected.views[0],fresh,expected.unpublished_sdk)&&gears_dlss_output::same(expected,fresh);
}
void owned_copy_observe(ID3D12GraphicsCommandList* list,D3D12_TEXTURE_COPY_LOCATION const* destination,UINT x,UINT y,UINT z,
    D3D12_TEXTURE_COPY_LOCATION const* source,D3D12_BOX const* box,uintptr_t caller,unsigned api){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!context||context->failed||!valid(context->created.identity)||!trial_events||!list||context->owned_copy_traces>=16)return;
    D3D12_TEXTURE_COPY_LOCATION d{},s{};D3D12_BOX b{};
    if(!safe_copy(&d,destination,sizeof(d))||!safe_copy(&s,source,sizeof(s))||!d.pResource||!s.pResource||
       canonical_identity(d.pResource)!=gears_owned_scene_live::trial.resource)return;
    auto& c=*context;++c.owned_copy_traces;const bool box_read=!box||safe_copy(&b,box,sizeof(b));
    const auto dd=d.pResource->GetDesc(),sd=s.pResource->GetDesc();const auto ref=frame_link.current(canonical_identity(list));
    const bool game_caller=caller>=base&&caller-base<0x2000000;
    fprintf(trial_events,"owned_final_copy row=%u qpc=%lld thread=%lu consume_ticket=%llu input_ticket=%llu owned_ticket=%llu api=%u caller=%llx caller_is_game=%u original_returned=1 gpu_completion_claim=0 list=%llx generation=%llu source=%llx target=%llx source_type=%u target_type=%u source_subresource=%u target_subresource=%u source_dimensions=%llu,%u source_format=%u target_dimensions=%llu,%u target_format=%u destination=%u,%u,%u box_null=%u box_read=%u box=%u,%u,%u,%u,%u,%u transport_accepted=%u transition=%u copy_pending=%u output_snapshot=%u\n",
        c.owned_copy_traces,now(),GetCurrentThreadId(),consuming_frame_ticket,c.last_input.consumption.ticket,gears_owned_scene_live::trial.ticket,
        api,game_caller?caller-base:caller,unsigned(game_caller),ref.identity,ref.generation,canonical_identity(s.pResource),canonical_identity(d.pResource),
        unsigned(s.Type),unsigned(d.Type),s.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX?s.SubresourceIndex:UINT_MAX,
        d.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX?d.SubresourceIndex:UINT_MAX,sd.Width,sd.Height,unsigned(sd.Format),dd.Width,dd.Height,unsigned(dd.Format),
        x,y,z,unsigned(!box),unsigned(box_read),b.left,b.top,b.front,b.right,b.bottom,b.back,
        unsigned(c.transport_accepted),unsigned(c.last_input.native_transition),unsigned(c.copy_pending),unsigned(bool(c.final_output_snapshot)));
    fflush(trial_events);
}
void owned_draw_observe(ID3D12GraphicsCommandList* list,gears_viewport::TargetDraw const& binding,
    uintptr_t caller,unsigned kind,unsigned elements,unsigned instances,bool known){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!context||!trial_events||!list||!elements||!instances||
       (known&&(binding.targets!=1||(binding.target.format!=24&&binding.target.format!=28)||binding.target.width<64||binding.target.height<64)))return;
    auto& c=*context;const bool other=known&&binding.target.resource!=gears_owned_scene_live::trial.resource;
    // Reserve a separate bounded bank for draws after the last observed SDK
    // input. This is observation only: absence for the whole ticket is not yet
    // proved here, and cannot grant publication or manufacture an SDK receipt.
    const bool gap=known&&!other&&consuming_frame_ticket&&c.last_input.consumption.ticket&&
        consuming_frame_ticket>c.last_input.consumption.ticket;
    if(c.created.identity.epoch!=c.owned_rebind_trace_epoch){c.owned_rebind_trace_epoch=c.created.identity.epoch;c.owned_rebind_draw_traces=0;}
    const bool rebind=known&&!other&&consuming_frame_ticket&&c.recreations&&c.owned_rebind_draw_traces<8;
    if((!gap&&!rebind&&(c.failed||!valid(c.created.identity)||c.owned_draw_traces>=48))||
       (gap&&c.owned_gap_draw_traces>=8&&!rebind)||(!known&&c.owned_unknown_draw_traces>=8)||(other&&c.owned_other_draw_traces>=8))return;
    ++c.owned_draw_traces;if(!known)++c.owned_unknown_draw_traces;if(other)++c.owned_other_draw_traces;
    if(gap){++c.owned_gap_draw_traces;fprintf(trial_events,"owned_gap_draw row=%u input_precedes_consume=1 gap_bank_row=%u source_constructor_eligible=%u context_failed=%u epoch_retired=%u diagnostics_authorize=0\n",
        c.owned_draw_traces,c.owned_gap_draw_traces,unsigned(c.source_activity.eligible()),unsigned(c.failed),unsigned(c.epoch_retired));dlss_output_control::print_current_gap(trial_events);}
    if(rebind){++c.owned_rebind_draw_traces;fprintf(trial_events,"owned_rebind_draw row=%u rebind_bank_row=%u epoch=%llu recreation=%u diagnostics_authorize=0\n",
        c.owned_draw_traces,c.owned_rebind_draw_traces,c.created.identity.epoch,c.recreations);if(!gap)dlss_output_control::print_current_gap(trial_events);}
    const auto canonical=canonical_identity(list);const auto ref=frame_link.current(canonical);
    auto const r=owned_target_stats.draw_state(canonical);auto const& q=late_quad_candidate;
    fprintf(trial_events,"owned_final_draw row=%u qpc=%lld thread=%lu consume_ticket=%llu input_ticket=%llu owned_ticket=%llu epoch=%llu transport_accepted=%u transition=%u copy_pending=%u output_snapshot=%u caller=%llx kind=%u elements=%u instances=%u target=%llx expected_owned=%llx dimensions=%llu,%u format=%u list=%llx generation=%llu final_eye_valid=%u final_eye=%u quad_active=%u quad_caller=%llx quad_args=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g quad_dimensions=%u,%u,%u,%u viewport=%.9g,%.9g,%.9g,%.9g scissor=%.9g,%.9g,%.9g,%.9g tracked=%u\n",
        c.owned_draw_traces,now(),GetCurrentThreadId(),consuming_frame_ticket,c.last_input.consumption.ticket,gears_owned_scene_live::trial.ticket,c.created.identity.epoch,
        unsigned(c.transport_accepted),unsigned(c.last_input.native_transition),unsigned(c.copy_pending),unsigned(bool(c.final_output_snapshot)),caller>=base?caller-base:caller,
        kind,elements,instances,binding.target.resource,gears_owned_scene_live::trial.resource,binding.target.width,binding.target.height,binding.target.format,
        ref.identity,ref.generation,unsigned(final_eye.valid),final_eye.eye,unsigned(q.active),q.caller,
        q.args[0],q.args[1],q.args[2],q.args[3],q.args[4],q.args[5],q.args[6],q.args[7],q.dimensions[0],q.dimensions[1],q.dimensions[2],q.dimensions[3],
        r.viewport.x,r.viewport.y,r.viewport.width,r.viewport.height,r.scissor.x,r.scissor.y,r.scissor.width,r.scissor.height,unsigned(r.tracked));
    fprintf(trial_events,"owned_final_draw_tracking row=%u binding_known=%u descriptor_capacity_lost=%u owned_error=%u trial_error=%u prior_unknown_draws=%llu\n",
        c.owned_draw_traces,unsigned(known),unsigned(owned_target_stats.descriptor_capacity_lost()),owned_scene_gpu::error,gears_owned_scene_live::trial.error,gears_owned_scene_live::trial.unknown_draws);
    if(c.owned_draw_traces<=4||(binding.target.resource==gears_owned_scene_live::trial.resource&&c.owned_draw_traces<=16)){std::array<void*,16> stack{};
        const auto count=CaptureStackBackTrace(0,static_cast<DWORD>(stack.size()),stack.data(),nullptr);
        fprintf(trial_events,"owned_final_draw_native_stack row=%u rvas=",c.owned_draw_traces);
        for(unsigned i=0;i<count;++i){const auto a=reinterpret_cast<uintptr_t>(stack[i]);if(a>=base&&a-base<0x2000000)fprintf(trial_events,"%llx,",a-base);}
        fprintf(trial_events,"\n");}
    fflush(trial_events);
}
gears_ngx_owned::Publication owned_publication(uint64_t world,uint64_t ticket){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);gears_ngx_owned::Facts f;f.world=world;f.ticket=ticket;
    if(context){auto const& c=*context;f.owner=!c.owner.expired();f.failed=c.failed;f.epoch=c.created.identity;
        f.no_sdk_gap=!c.source_activity.eligible();f.transition=c.last_input.native_transition;
        f.render_candidate=dlss_output_control::unpublished(world,ticket,c.created.identity);
        f.copy_pending=c.copy_pending;f.transport_accepted=c.transport_accepted;f.last_eye=c.last_eye;
        f.input_world=c.last_input.consumption.world;f.input_ticket=c.last_input.consumption.ticket;
        f.input_renderer=c.last_input.consumption.renderer;f.input_epoch=c.last_input.producers[1].use.identity;
        if(c.final_output_snapshot)f.output=&*c.final_output_snapshot;}
    return gears_ngx_owned::publication(f);
}
#endif
void consumed(uint64_t world,uint64_t ticket,uintptr_t renderer,bool enabled){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!trial_input_union_mask||trial_packed_input||!context||context->failed||!recording.load()||submitted_lifetime.stopping()||
       !valid(context->created.identity)||!enabled||!ticket||world!=context->created.identity.world||world!=committed_world_generation.load())return;
    // The caller supplies the immutable registered frame saved before original
    // consume. Original may already have destroyed/reused the renderer, so no
    // live pointer lookup after its return can strengthen this gap observation.
    auto& c=*context;
    const bool used=c.last_input.consumption.ticket==ticket&&c.last_input.consumption.renderer==renderer&&c.last_eye==1&&!c.copy_pending;
    c.source_activity.consumed(ticket,used);
}
bool baseline_pixel_ready(){
    uintptr_t handle{};return trial_baseline_pixels&&ngx_trial_producer::active()&&consuming_frame_ticket>=32&&
        !ngx_stereo_host::current_control()&&ngx_route_observer::read(handle,base,gears_ngx_route::handle_cell_rva)&&!handle;
}
bool right_draw_pixel_pair(uintptr_t target,ID3D12Resource*& source,ID3D12Resource*& paired,unsigned& role,Rect& right){
    source=paired=nullptr;right={};role=0;
    if(!trial_right_draw_pixels||!context||context->failed||!context->transport_accepted||
       ngx_trial_producer::inputs_accepted<64||!ngx_trial_producer::active()||
       context->created.identity.world!=committed_world_generation.load())return false;
    auto& c=*context;
    if(target==c.pixel_color.canonical){source=c.pixel_color.get();paired=c.pixel_output.get();role=1;}
    else if(target==c.pixel_output.canonical){source=c.pixel_output.get();paired=c.pixel_color.get();role=2;}
    right=ngx_trial_producer::state->rectangles[1];
    return source&&paired&&source!=paired&&right.width&&right.height;
}
void right_draw_pixel_trace(unsigned role,float x,float y,float w,float h,float sx,float sy,float sw,float sh){
    static unsigned traces{};if(!trial_events||traces>=8)return;++traces;
    fprintf(trial_events,"right_draw_binding role=%u ticket=%llu previous_input_ticket=%llu previous_eye=%u copy_pending=%u viewport=%.9g,%.9g,%.9g,%.9g scissor=%.9g,%.9g,%.9g,%.9g\n",role,consuming_frame_ticket,context?context->last_input.consumption.ticket:0,context?context->last_eye:0,unsigned(context&&context->copy_pending),x,y,w,h,sx,sy,sw,sh);fflush(trial_events);
}
bool output_target_rect_trial(){return trial_output_target_rects;}
bool packed_input_trial(){return trial_packed_input;}
bool scene_color_pixel_pair(ID3D12Resource* source,ID3D12Resource*& other){
    other=nullptr;
    if(!trial_scene_color_pixels||!context||context->failed||!context->transport_accepted||ngx_trial_producer::inputs_accepted<64||
        context->owner.expired()||context->created.identity.world!=committed_world_generation.load()||
        reinterpret_cast<uintptr_t>(source)!=context->last_input.raw_resources[0])return false;
    // Observation only: a typed epoch lease survives outside consume TLS.
    // This supplies no producer ticket or logical-eye association.
    other=context->pixel_output.get();return other&&other!=source;
}
void scene_color_transition_trace(unsigned before,unsigned after,unsigned flags,unsigned sub,uintptr_t caller,bool associated,uint64_t ticket){
    if(!trial_events)return;fprintf(trial_events,"scene_color_transition before=%u after=%u flags=%u sub=%u caller=%llx associated=%u ticket=%llu thread=%lu\n",
        before,after,flags,sub,caller,unsigned(associated),ticket,GetCurrentThreadId());fflush(trial_events);
}
bool preprocess_pixel_resources(std::array<uintptr_t,4>& out){
    out={};uint64_t current_ticket{};
    if(!trial_preprocess_pixels||!context||context->copy_pending||!acquisition_scope(*context,current_ticket)||!ngx_trial_producer::active()||!pixel_capture_ready())return false;
    auto const& r=context->last_input.raw_resources;out={uintptr_t(r[0]),uintptr_t(r[3]),uintptr_t(r[1]),uintptr_t(r[2])};return true;
}
#endif
#else
inline void acquire_barriers(UINT,D3D12_RESOURCE_BARRIER const*){}
inline void reset_command(ID3D12CommandList*){}
inline void created_command(HRESULT,void**,bool){}
inline bool constructed(void*,gears_submitted::FrameTicket const&,Record const&,bool){return true;}
inline bool copy(ID3D12GraphicsCommandList*,D3D12_TEXTURE_COPY_LOCATION const*,UINT,UINT,UINT,D3D12_TEXTURE_COPY_LOCATION const*,D3D12_BOX const*,uintptr_t,ngx_stereo_host::CopyFn){return false;}
inline void execute(ID3D12CommandQueue* q,UINT n,ID3D12CommandList* const* lists,ngx_stereo_host::ExecuteFn f){f(q,n,lists);}
#endif
}
