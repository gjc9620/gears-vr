// Capture-local integration. No startup call is added by this patch.
namespace ngx_capture_control {
#if defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER
using namespace gears_ngx_stereo;
struct Context {
    struct Construction {uint64_t world{},ticket{};uintptr_t renderer{};std::array<uintptr_t,2> views{};};
    NativeGpuSubmission gpu;std::weak_ptr<ngx_stereo_host::ControlOwner> owner;
    std::array<Construction,64> constructions{};
    std::unique_ptr<TypedQuartet> quartet{std::make_unique<TypedQuartet>()};
    std::shared_ptr<void> producer_lifetime;
    // Actual producer evidence remains required; no observer row/bank can
    // implement it. Quartet retention is separate from producer proof.
    bool (*producer)(void*,ngx_stereo_host::ControlCreateBinding const&,ngx_stereo_host::ControlConsumption,
        EvaluateShadow const&,Command,ngx_stereo_host::ControlInputTicket&){};
    ngx_stereo_host::ControlCreateBinding created{};ngx_stereo_host::ControlInputTicket last_input{};
    unsigned last_eye{};TypedCopyBoundary* copy_boundary{};bool failed{};
};
std::shared_ptr<Context> context;
uint64_t generation(ID3D12CommandList*,uintptr_t canonical){auto ref=frame_link.current(canonical);return ref.identity==canonical?ref.generation:0;}
bool read(void* out,void const* in,size_t size){return safe_copy(out,in,size);}
void reject(Context& c){c.failed=true;if(auto owner=c.owner.lock()){owner->invalidate();request_session_stop(SessionStop::proof_failure);}}
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
    if(!context||!acquisition_scope(*context,ticket))return;auto& c=*context;
    if(count>1024||(count&&!values)){c.quartet->uncertain();reject(c);return;}
    for(UINT i=0;i<count;++i){D3D12_RESOURCE_BARRIER value{};
        if(!safe_copy(&value,values+i,sizeof(value))||!acquire_quartet_barrier(*c.quartet,c.created.identity,ticket,value)){
            c.quartet->uncertain();reject(c);return;}
    }
}
void acquire_copy(D3D12_TEXTURE_COPY_LOCATION const* source,D3D12_TEXTURE_COPY_LOCATION const* destination){
    uint64_t ticket{};if(!context||!acquisition_scope(*context,ticket))return;auto& c=*context;
    D3D12_TEXTURE_COPY_LOCATION s{},d{};
    if(!safe_copy(&s,source,sizeof(s))||!safe_copy(&d,destination,sizeof(d))||
       !c.quartet->observe_resource(c.created.identity,ticket,s.pResource)||
       !c.quartet->observe_resource(c.created.identity,ticket,d.pResource)){
        c.quartet->uncertain();reject(c);
    }
}
void reset_command(ID3D12CommandList* list){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!context||context->failed||!list)return;
    auto& c=*context;auto owner=c.owner.lock();if(!owner)return;
    const auto id=canonical_identity(list);const auto gen=generation(list,id);
    if(!c.gpu.reset_success(owner->adapter(),list,id,gen))reject(c);
}
void created_command(HRESULT hr,void** out,bool open){
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
    auto failed=[&](){if(context)reject(*context);
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
    out={};auto owner=c.owner.lock();if(c.failed||!owner||!raw)return false;
    // raw is the preserved first argument of the authenticated complete native
    // Create/Evaluate wrapper: the SDK command-list ABI, not a resource field.
    // Copy uses its actual typed D3D list. Retain only at these owned call sites.
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;
    Microsoft::WRL::ComPtr<ID3D12Device> device;Microsoft::WRL::ComPtr<IUnknown> identity,device_id;
    if(FAILED(reinterpret_cast<IUnknown*>(raw)->QueryInterface(IID_PPV_ARGS(&list)))||
       list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT||FAILED(list.As(&identity))||
       FAILED(list->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device.As(&device_id)))return false;
    const auto id=reinterpret_cast<uintptr_t>(identity.Get());const auto gen=generation(list.Get(),id);
    if(!gen||!c.gpu.retain_recording(owner->adapter(),list.Get(),id,gen))return false;
    out={raw,id,gen,reinterpret_cast<uintptr_t>(device_id.Get()),true};return true;
}
bool constructed(void* renderer,gears_submitted::FrameTicket const& frame,Record const& built,bool verified){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!context)return true;auto& c=*context;
    if(c.failed)return c.owner.expired();
    if(!verified||!frame.enabled||!frame.id||built.family_count!=2){reject(c);return c.owner.expired();}
    Context::Construction* slot{};
    for(auto& old:c.constructions){gears_submitted::FrameTicket live;
        if(!old.renderer||!submitted_lifetime.frame(old.renderer,live)||live.id!=old.ticket||live.world_generation!=old.world){slot=&old;break;}}
    if(!slot){reject(c);return c.owner.expired();}
    *slot={frame.world_generation,frame.id,reinterpret_cast<uintptr_t>(renderer),{built.family_views[0],built.family_views[1]}};return true;
}
bool create(void* opaque,ngx_stereo_host::NativeCreateSource const& source,uintptr_t raw,ngx_stereo_host::ControlCreateBinding& out){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto& c=*static_cast<Context*>(opaque);out={};
    if(c.failed||source.world!=committed_world_generation.load()||!source.create_layout(out.layout)||!command(c,raw,out.command))return false;
    out.identity={source.revision,source.world,out.command.device,source.parameters,source.revision};
    if(!c.quartet||!c.quartet->begin(out.identity))return false;c.created=out;return true;
}
bool input(void* opaque,ngx_stereo_host::ControlConsumption used,EvaluateShadow const& source,uintptr_t raw,ngx_stereo_host::ControlInputTicket& out){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto& c=*static_cast<Context*>(opaque);out={};Command cmd;gears_submitted::FrameTicket frame;
    if(c.failed||!c.producer||!c.producer_lifetime||used.renderer!=reinterpret_cast<uintptr_t>(consuming_renderer)||
       used.ticket!=consuming_frame_ticket||!consuming_frozen_frame.enabled||used.ticket!=consuming_frozen_frame.id||
       used.world!=consuming_frozen_frame.world_generation||used.world!=committed_world_generation.load()||
       !submitted_lifetime.frame(used.renderer,frame)||!frame.enabled||frame.id!=used.ticket||frame.world_generation!=used.world||!command(c,raw,cmd))return false;
    Context::Construction const* original{};for(auto const& built:c.constructions)
        if(built.world==used.world&&built.ticket==used.ticket&&built.renderer==used.renderer){original=&built;break;}
    if(!original)return false;Record live;inspect_completed_renderer(live,consuming_renderer);
    if(live.family_count!=2||live.renderer_count!=2||live.family_views[0]!=original->views[0]||live.family_views[1]!=original->views[1])return false;
    std::array<uintptr_t,4> native{};constexpr size_t offsets[]{0,8,0x18,0x20};
    for(unsigned i=0;i<4;++i)std::memcpy(&native[i],source.bytes.data()+offsets[i],sizeof(uintptr_t));
    std::array<Resource,4> retained{};
    if(!c.quartet||!c.quartet->bind(c.created.identity,used.ticket,native,retained))return false;
    if(!c.producer(c.producer_lifetime.get(),c.created,used,source,cmd,out)||out.original_views!=original->views)return false;
    std::array<Point,3> bases;for(unsigned i=0;i<3;++i)std::memcpy(&bases[i],source.bytes.data()+0x60+8*i,8);
    if(!select_input_eye(c.created.layout,c.created.layout.input,bases,c.last_eye))return false;
    constexpr unsigned order[]{0,2,3,1};
    for(unsigned i=0;i<4;++i)if(out.raw_resources[i]!=native[order[i]]||
        !same_resource(out.producers[c.last_eye].resources[i],retained[i]))return false;
    c.last_input=out;return true; // Producer domains are still required; retention never supplies them.
}
Completion completion(void* opaque,uint64_t handle){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto& c=*static_cast<Context*>(opaque);auto owner=c.owner.lock();
    if(!owner)return {};auto entry=owner->adapter().enter();if(!entry||!owner->adapter().owns(entry,handle))return {};return c.gpu.completion();
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
               used.world!=committed_world_generation.load()||used.world!=c.created.identity.world){reject(c);return false;}
            TicketAdmission admission;
            {auto entry=owner->adapter().enter();if(!entry){reject(c);return false;}
                admission=owner->adapter().admit_ticket(entry,used.world,used.ticket,c.gpu.completion());}
            if(admission==TicketAdmission::admitted)return true;
            if(admission==TicketAdmission::rejected||GetTickCount64()-start>=first_pair_timeout_ms){reject(c);return false;}
        }
        Sleep(10); // Only normal pending first-pair GPU completion reaches here.
    }
}
void boxed_dispatch(void* opaque,CopyPlan const& plan){auto& c=*static_cast<Context*>(opaque);
    if(c.failed||!c.copy_boundary||!c.copy_boundary->copy(plan))throw std::runtime_error("NGX actual copy boundary rejected");}
bool current(void*,ID3D12GraphicsCommandList* list,uintptr_t id,uint64_t gen){return canonical_identity(list)==id&&generation(list,id)==gen;}
bool copy(ID3D12GraphicsCommandList* list,D3D12_TEXTURE_COPY_LOCATION const* dst,UINT x,UINT y,UINT z,
    D3D12_TEXTURE_COPY_LOCATION const* src,D3D12_BOX const* box,uintptr_t caller,ngx_stereo_host::CopyFn forward){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto owner=ngx_stereo_host::current_control();if(!owner)return false;
    if(!context||context->owner.lock()!=owner){owner->invalidate();request_session_stop(SessionStop::proof_failure);return true;}
    acquire_copy(src,dst);
    auto& c=*context;D3D12_TEXTURE_COPY_LOCATION s{},d{};
    if(!safe_copy(&s,src,sizeof(s))||!safe_copy(&d,dst,sizeof(d))){reject(c);return true;}
    if(reinterpret_cast<uintptr_t>(s.pResource)!=c.last_input.raw_resources[3]||reinterpret_cast<uintptr_t>(d.pResource)!=c.last_input.raw_resources[0])return false;
    Command cmd;uint64_t ticket{};std::array<Resource,4> retained{};
    auto const& raw=c.last_input.raw_resources;
    const std::array<uintptr_t,4> native{uintptr_t(raw[0]),uintptr_t(raw[3]),uintptr_t(raw[1]),uintptr_t(raw[2])};
    if(c.failed||c.copy_boundary||c.last_eye>1||!acquisition_scope(c,ticket)||
       c.last_input.consumption.ticket!=ticket||c.last_input.consumption.world!=c.created.identity.world||
       c.last_input.consumption.renderer!=reinterpret_cast<uintptr_t>(consuming_renderer)||
       !c.quartet->bound()||!c.quartet->bind(c.created.identity,ticket,native,retained)||
       !preserves_native_copy(ngx_stereo_host::attested,caller,d,x,y,z,s,box,uintptr_t(raw[3]),uintptr_t(raw[0]))||
       !command(c,reinterpret_cast<uintptr_t>(list),cmd)||cmd.device!=c.created.identity.device){reject(c);return true;}
    auto const& producer=c.last_input.producers[c.last_eye];
    if(!same(producer.use.identity,c.created.identity)||producer.use.ticket!=ticket||producer.use.eye!=c.last_eye){reject(c);return true;}
    for(unsigned i=0;i<4;++i)if(!same_resource(retained[i],producer.resources[i])){reject(c);return true;}
    CopyFacts facts;facts.identity=c.created.identity;facts.ticket=c.last_input.consumption.ticket;facts.caller=caller;
    facts.list=cmd.list;facts.generation=cmd.generation;facts.list_device=cmd.device;facts.direct=cmd.direct;
    // Legal-state requirement is preserved from the attested native call,
    // not reconstructed from last-observed barriers. No barriers are added.
    facts.copy_source=facts.copy_dest=true;facts.resources=retained;
    facts.input_base=c.created.layout.input_base;facts.feature_output=c.created.layout.eye_output;facts.evaluated_ordinal=c.last_eye;
    TypedCopyBoundary boundary(list,cmd.list,cmd.generation,facts.ticket,c.last_eye,d,s,&c,current,forward);
    struct Scope {Context& c;Scope(Context& value,TypedCopyBoundary& b):c(value){c.copy_boundary=&b;}~Scope(){c.copy_boundary=nullptr;}} scope(c,boundary);
    bool ok{};try{auto entry=owner->adapter().enter();ok=entry&&owner->adapter().copy(entry,facts,c.last_eye);}catch(...){reject(c);throw;}
    if(!ok)reject(c);return true; // Selected owned copy never falls back to full Output->Color.
}
void execute(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* lists,ngx_stereo_host::ExecuteFn forward){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto owner=ngx_stereo_host::current_control();
    if(!owner){forward(queue,count,lists);return;}
    if(!context||context->owner.lock()!=owner){owner->invalidate();request_session_stop(SessionStop::proof_failure);forward(queue,count,lists);return;}
    if(!context->gpu.execute(owner->adapter(),queue,count,lists,forward,read,generation)){reject(*context);return;}
    TransportCompletion transport;
    {auto entry=owner->adapter().enter();if(!entry){reject(*context);return;}
        transport=owner->adapter().complete_candidate_transport(entry,context->gpu.completion());}
    if(transport==TransportCompletion::rejected)reject(*context);
}
// Prepare before the native Create/Eval wrappers. Activate only when
// the actual producer source and full output/retirement gates are wired.
// No call is inserted here; this is a host installation seam, not a user option.
bool activate(AdapterPrerequisites prerequisites){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!context||context->failed||!context->producer||!context->producer_lifetime||!context->owner.expired())return false;
    auto& c=*context;NativeDispatchFunctions functions{ngx_stereo_host::original_create,ngx_stereo_host::original_evaluate,ngx_stereo_host::original_release,&c,boxed_dispatch};
    functions.optimal=ngx_stereo_host::native_optimal_query();
    ngx_stereo_host::ControlCallbacks callbacks{context,create,input,completion,admit};
    auto owner=std::make_shared<ngx_stereo_host::ControlOwner>(functions,prerequisites,callbacks);
    c.owner=owner;if(ngx_stereo_host::attach_control(owner))return true;c.owner.reset();return false;
}
#else
inline void acquire_barriers(UINT,D3D12_RESOURCE_BARRIER const*){}
inline void reset_command(ID3D12CommandList*){}
inline void created_command(HRESULT,void**,bool){}
inline bool constructed(void*,gears_submitted::FrameTicket const&,Record const&,bool){return true;}
inline bool copy(ID3D12GraphicsCommandList*,D3D12_TEXTURE_COPY_LOCATION const*,UINT,UINT,UINT,D3D12_TEXTURE_COPY_LOCATION const*,D3D12_BOX const*,uintptr_t,ngx_stereo_host::CopyFn){return false;}
inline void execute(ID3D12CommandQueue* q,UINT n,ID3D12CommandList* const* lists,ngx_stereo_host::ExecuteFn f){f(q,n,lists);}
#endif
}
