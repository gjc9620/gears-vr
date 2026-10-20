// Included after ngx_route_observer.inl and the existing typed GPU trackers.
// Default OFF. No SDK argument, global handle, resource state, copy region,
// submission or fence is changed. This host closes specific missing evidence
// edges before the independent-feature adapter can be activated.
namespace ngx_stereo_host {
#if defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER
#if !defined(GEARS_NGX_ROUTE_OBSERVER) || !GEARS_NGX_ROUTE_OBSERVER
#error NGX stereo host requires authenticated native wrapper observation
#endif
#if !defined(GEARS_OWNED_OUTPUT_TRACKING) || !GEARS_OWNED_OUTPUT_TRACKING
#error NGX stereo host requires the existing typed ResourceBarrier hook
#endif
#if defined(GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT) && GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT
#error Read-only NGX host does not admit suppressed native copies
#endif
using CreateFn=uint32_t(__fastcall*)(uintptr_t,uint32_t,uintptr_t,uintptr_t*);
using EvalFn=uint32_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
using ReleaseFn=uint32_t(__fastcall*)(uintptr_t);
CreateFn original_create{};EvalFn original_evaluate{};ReleaseFn original_release{};
struct List {uintptr_t raw{},canonical{};uint64_t generation{};};
using Typed=gears_ngx_stereo::TypedResourceLease;
struct Transition {Typed resource{};List list{};uint64_t serial{};uintptr_t caller{};unsigned before{},after{},subresource{},flags{};};
struct Draw {uintptr_t resource{},caller{},view{};List list{};gears_viewport::DrawState rectangles{};uint64_t first{},last{},count{};};
struct Bind {List list{};std::array<uintptr_t,8> resources{};unsigned count{};uintptr_t depth_handle{};bool complete{};};
struct TicketBank {
    uint64_t world{},ticket{},serial{};unsigned transitions{},draws{},binds{},dropped{};
    std::array<Transition,64> resources{};std::array<Draw,64> producers{};std::array<Bind,16> bindings{};
};
struct RendererView {uintptr_t address{};unsigned mask{};std::array<float,4> source{},sampling{};std::array<int32_t,4> target{};};
struct Row {
    bool evaluate{},returned{},unwinding{},nested{},concurrent{},dispatcher_returned{},dispatcher_match{},copy_returned{};
    uint64_t epoch{},sequence{},wrapper_result{};unsigned dispatches{},copies{},thread{},typed_mask{},producer_mask{},bank_dropped{};
    uintptr_t caller{},structure{},command{},consumed_handle{},consumed_parameters{},callback{},handle_output{};
    uintptr_t vendor{},vendor_module{};bool vendor_module_named{};uint32_t dispatch_result{};
    gears_ngx_route::Sample before{},after{};List command_list{},copy_list{};
    std::array<Transition,4> inputs{};std::array<Draw,6> producers{};std::array<RendererView,2> views{};
    std::array<uint32_t,2> renderer_extent{},buffer_extent{};unsigned renderer_mask{};
    std::array<Typed,2> copied{};unsigned copy_source_type{},copy_destination_type{},copy_source_subresource{},copy_destination_subresource{};
    std::array<unsigned,3> copy_destination{};bool copy_box{};uintptr_t copy_caller{};
    long long entry{},exit{},dispatcher_entry{},dispatcher_exit{};
    uintptr_t queue{};uint64_t execute_order{};unsigned execute_member{},execute_occurrences{};bool execute_returned{},execute_generation_match{};
    unsigned retained_mask{};bool typed_set_ready{};
    std::array<gears_ngx_stereo::Resource,4> adapter_resources{};
    uintptr_t eval_queue{};uint64_t eval_execute_order{};unsigned eval_execute_member{},eval_execute_occurrences{};
    bool eval_execute_returned{},eval_execute_generation_match{};
};
struct ReleaseRow {uintptr_t handle{},caller{},vendor{},module{};uint32_t result{};bool returned{},vendor_module_named{};long long entry{},exit{};};
bool attested{},printed{};TicketBank bank;std::array<Row,12> rows{};unsigned row_count{},epoch_evals{},epochs{},in_wrappers{},release_count{};
unsigned sampled_tickets{},typed_observations{};
unsigned total_sampled_tickets{},total_typed_observations{};
std::array<ReleaseRow,8> releases{};uint64_t sequence{},execute_sequence{},limited{},foreign_dispatches{},read_failures{};
thread_local int current_row=-1,pending_row=-1;
NativeCreateSource latest_native_create{};uint64_t native_create_revision{};
thread_local WrapperScope* source_create_scope{};
#if GEARS_NGX_MV_TARGET_CAPTURE
thread_local WrapperScope* target_wrapper{};
#endif
std::shared_ptr<ControlOwner> control_owner;
bool attach_control(std::shared_ptr<ControlOwner> const& owner){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!attested||!recording.load()||in_wrappers||control_owner||!owner||!owner->configured())return false;
    control_owner=owner;return true;
}
bool detach_control(std::shared_ptr<ControlOwner> const& owner){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!owner||control_owner!=owner||in_wrappers||active.load()||!owner->can_detach())return false;
#if defined(GEARS_NGX_CAPTURE_CONTROL_WIRING) && GEARS_NGX_CAPTURE_CONTROL_WIRING
    ngx_capture_control::detached(owner); // Only after callback/GPU/feature drain above.
#endif
    control_owner.reset();return true;
}
std::shared_ptr<ControlOwner> current_control(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);return control_owner;}
gears_ngx_stereo::NativeOptimalQuery native_optimal_query(){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    return attested?reinterpret_cast<gears_ngx_stereo::NativeOptimalQuery>(base+0xbe61bc):nullptr;
}
bool read_create_fields(std::array<uint32_t,7>& fields,uintptr_t structure);
bool pending_create_current(NativeCreateSource const& source){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);uintptr_t handle{},parameters{};
    std::array<uint32_t,7> fields{};
    return attested&&recording.load()&&in_wrappers==1&&source_create_scope&&!source_create_scope->source_bad&&
        !source.handle&&source.revision==native_create_revision&&source.world==committed_world_generation.load()&&
        source_create_scope->source.revision==source.revision&&source_create_scope->source.fields==source.fields&&
        read_create_fields(fields,source_create_scope->source_structure)&&fields==source.fields&&
        ngx_route_observer::read(handle,base,gears_ngx_route::handle_cell_rva)&&!handle&&
        ngx_route_observer::read(parameters,base,gears_ngx_route::parameter_cell_rva)&&parameters==source.parameters;
}
uint64_t ControlOwner::create(NativeCreateSource const& family,uintptr_t command,
    gears_ngx_stereo::CreateShadow const& source,void const* original_pointer,std::function<uint64_t(void const*)> const& original){
    using namespace gears_ngx_stereo;ControlCreateBinding binding;
    if(!configured_||!callbacks_.create(callbacks_.lifetime.get(),family,command,binding)||
       binding.identity.world!=family.world||binding.identity.parameters!=family.parameters||
       binding.command.raw!=command||!valid_create_layout(binding.layout)||family.fields[0]!=2*binding.layout.input.width||
       family.fields[1]!=binding.layout.input.height||family.fields[2]!=binding.layout.full_output.width||
       family.fields[3]!=binding.layout.full_output.height){invalidate();return dispatch_unavailable;}
    // Query the actual per-eye output/Quality, never divide family min/max.
    // The adapter gate serializes parameter-consuming owned wrappers. It is
    // deliberately released before bridge_.create_wrapper takes its own gate;
    // the original Create wrapper then overwrites its normal Create parameters.
    {
        auto entry=adapter_.enter();if(!entry)return dispatch_unavailable;
        if(!pending_create_current(family)||source_create_scope->source_command!=command||
           source_create_scope->source_structure!=reinterpret_cast<uintptr_t>(original_pointer)){
            adapter_.invalidate(entry);return dispatch_unavailable;}
        try{binding.optimal=backend_.optimal(binding.layout.eye_output,family.fields[4]);}
        catch(...){adapter_.invalidate(entry);throw;}
        if(!pending_create_current(family)||!binding.optimal.supports(binding.layout.input)){
            adapter_.invalidate(entry);
            return binding.optimal.called&&!native_result_accepted(binding.optimal.result)?binding.optimal.result:dispatch_unavailable;
        }
    }
    {std::lock_guard<std::mutex> lock(metadata_mutex_);creation_=binding;}
    return bridge_.create_wrapper(binding.identity,binding.layout,binding.command,
        reinterpret_cast<uint64_t*>(base+gears_ngx_route::handle_cell_rva),source,original_pointer,[&](void const* owned){
            return pending_create_current(family)?original(owned):uint64_t(dispatch_unavailable);
        }).raw;
}
uint64_t ControlOwner::evaluate(ControlConsumption consumed,uintptr_t command,uintptr_t handle,uintptr_t parameters,
    gears_ngx_stereo::EvaluateShadow const& source,void const* original_pointer,std::function<uint64_t(void const*)> const& original){
    using namespace gears_ngx_stereo;ControlCreateBinding created;
    {std::lock_guard<std::mutex> lock(metadata_mutex_);created=creation_;}
    ControlInputTicket ticket;unsigned eye{};std::array<Point,3> bases{};
    for(unsigned i=0;i<3;++i)std::memcpy(&bases[i],source.bytes.data()+0x60+8*i,8);
    if(!configured_||!consumed.world||!consumed.ticket||!consumed.renderer||created.identity.world!=consumed.world||
       created.identity.parameters!=parameters||!select_input_eye(created.layout,created.layout.input,bases,eye)||
       !callbacks_.admit(callbacks_.lifetime.get(),consumed)||
       !callbacks_.input(callbacks_.lifetime.get(),consumed,source,command,ticket)||
       ticket.consumption.world!=consumed.world||ticket.consumption.ticket!=consumed.ticket||ticket.consumption.renderer!=consumed.renderer||
       !ticket.original_views[0]||!ticket.original_views[1]||ticket.original_views[0]==ticket.original_views[1]||ticket.command.raw!=command){
        invalidate();return dispatch_unavailable;}
    auto const& producer=ticket.producers[eye];auto const& use=producer.use;
    if(!same(use.identity,created.identity)||use.ticket!=consumed.ticket||use.eye!=eye||
       use.list!=ticket.command.list||use.generation!=ticket.command.generation||producer.view!=ticket.original_views[eye]){
        invalidate();return dispatch_unavailable;}
    EvaluationBinding binding{producer,ticket.command,handle,ticket.raw_resources};
    return bridge_.evaluate_wrapper(binding,source,original_pointer,original).raw;
}
uint64_t create_passthrough(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,void const* structure,CreateWrapperFn forward){
    auto owner=current_control();if(!owner)return forward(a,b,c,d,e,structure);
    gears_ngx_stereo::CreateShadow source;auto* observed=source_create_scope;
    if(!observed||observed->source_bad||!safe_copy(source.bytes.data(),structure,source.bytes.size())){
        owner->invalidate();return gears_ngx_stereo::dispatch_unavailable;}
    return owner->create(observed->source,a,source,structure,[&](void const* p){return forward(a,b,c,d,e,p);});
}
uint64_t evaluate_passthrough(bool owned,uintptr_t a,uintptr_t b,uintptr_t c,void const* structure,EvaluateWrapperFn forward){
    auto owner=current_control();if(!owner)return forward(a,b,c,structure);
    gears_ngx_stereo::EvaluateShadow source;uintptr_t handle{},parameters{};
    ControlConsumption consumed{committed_world_generation.load(),consuming_frame_ticket,reinterpret_cast<uintptr_t>(consuming_renderer)};
    if(!owned||!consuming_frozen_frame.enabled||consumed.ticket!=consuming_frozen_frame.id||
       consumed.world!=consuming_frozen_frame.world_generation||!safe_copy(source.bytes.data(),structure,source.bytes.size())||
       !ngx_route_observer::read(handle,base,gears_ngx_route::handle_cell_rva)||!ngx_route_observer::read(parameters,base,gears_ngx_route::parameter_cell_rva)){
        owner->invalidate();return gears_ngx_stereo::dispatch_unavailable;}
    return owner->evaluate(consumed,a,handle,parameters,source,structure,[&](void const* p){return forward(a,b,c,p);});
}
template<class T> bool read(T& value,uintptr_t address,uintptr_t offset=0){return ngx_route_observer::read(value,address,offset);}
bool read_create_fields(std::array<uint32_t,7>& fields,uintptr_t structure){
    std::array<uint32_t,6> words{};uint8_t subrect{};
    if(!read(words,structure)||!read(subrect,structure,0x18))return false;
    for(unsigned i=0;i<6;++i)fields[i]=words[i];fields[6]=subrect;return true;
}
NativeCreateAvailability native_create_state(uint64_t world,NativeCreateSource& out){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);out={};uintptr_t handle{},parameters{};
    using State=NativeCreateAvailability;
    if(!attested||!recording.load()||in_wrappers)return State::unavailable;
    const auto current_world=committed_world_generation.load();
    if(latest_native_create.revision&&latest_native_create.world!=current_world){latest_native_create={};++native_create_revision;}
    if(!world||world!=current_world)return State::unavailable;
    if(!read(handle,base,gears_ngx_route::handle_cell_rva)){
        latest_native_create={};++native_create_revision;return State::unavailable;}
    // A readable zero native handle is distinct from missing Create evidence.
    if(!handle){latest_native_create={};++native_create_revision;return State::inactive;}
    if(!latest_native_create.revision)return State::unavailable;
    if(!read(parameters,base,gears_ngx_route::parameter_cell_rva)||
       handle!=latest_native_create.handle||parameters!=latest_native_create.parameters){
        latest_native_create={};++native_create_revision;return State::unavailable;}
    out=latest_native_create;return State::ready;
}
bool native_create_source(uint64_t world,NativeCreateSource& out){
    return native_create_state(world,out)==NativeCreateAvailability::ready;
}
bool native_create_matches(NativeCreateSource const& expected){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);uintptr_t handle{},parameters{};
    if(!attested||!recording.load()||in_wrappers>1)return false;
    // Invalidate observed loss rather than accepting an old token after a
    // later handle/parameter ABA. A stale caller must not invalidate a newer Create.
    if(latest_native_create.revision&&(latest_native_create.world!=committed_world_generation.load()||
       !read(handle,base,gears_ngx_route::handle_cell_rva)||
       !read(parameters,base,gears_ngx_route::parameter_cell_rva)||
       handle!=latest_native_create.handle||parameters!=latest_native_create.parameters)){
        latest_native_create={};++native_create_revision;
    }
    return expected.world==committed_world_generation.load()&&expected.matches(latest_native_create);
}
bool same(List a,List b){return a.raw==b.raw&&a.canonical==b.canonical&&a.generation==b.generation;}
bool valid(List a){return a.raw&&a.canonical&&a.generation;}
List list(ID3D12CommandList* value){if(!value)return {};auto id=canonical_identity(value);auto ref=frame_link.current(id);
    return {reinterpret_cast<uintptr_t>(value),id,ref.identity==id?ref.generation:0};}
#if !defined(GEARS_NGX_STEREO_HOST_TEST)
Typed typed(ID3D12Resource* resource){return Typed::observe(resource);}
bool vendor_owner(uintptr_t pointer,uintptr_t& module){HMODULE owner{};wchar_t path[MAX_PATH]{};
    if(!pointer||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(pointer),&owner))return false;
    module=reinterpret_cast<uintptr_t>(owner);const auto length=GetModuleFileNameW(owner,path,MAX_PATH);if(!length||length>=MAX_PATH)return false;
    auto* name=wcsrchr(path,L'\\');return name&&!_wcsicmp(name+1,L"_nvngx.dll");
}
UINT descriptor_stride(ID3D12GraphicsCommandList* command){ID3D12Device* device{};UINT stride{};
    if(SUCCEEDED(command->GetDevice(IID_PPV_ARGS(&device)))&&device){stride=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);device->Release();}return stride;}
#else
Typed typed(ID3D12Resource*);bool vendor_owner(uintptr_t,uintptr_t&);UINT descriptor_stride(ID3D12GraphicsCommandList*);
#endif
bool available(){return attested&&recording.load()&&frame_link_diagnostics&&epoch_evals<2&&epochs<4;}
bool select_bank(){if(!available())return false;auto owner=ngx_route_observer::ownership(true);if(!owner.associated)return false;
    if(bank.world!=owner.world||bank.ticket!=owner.ticket){if(sampled_tickets==8)return false;++sampled_tickets;++total_sampled_tickets;bank={};bank.world=owner.world;bank.ticket=owner.ticket;}
    return true;
}
void barriers(ID3D12GraphicsCommandList* command,UINT count,D3D12_RESOURCE_BARRIER const* values,uintptr_t caller){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!select_bank())return;auto ref=list(command);
    if(!valid(ref)||count>64||(count&&!values)){++bank.dropped;return;}
    for(unsigned i=0;i<count;++i){D3D12_RESOURCE_BARRIER b{};if(!safe_copy(&b,values+i,sizeof(b))){++bank.dropped;return;}
        ++bank.serial;if(b.Type!=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION){++bank.dropped;continue;}
        const auto raw=reinterpret_cast<uintptr_t>(b.Transition.pResource);unsigned slot=bank.transitions;
        for(unsigned j=0;j<bank.transitions;++j)if(bank.resources[j].resource.raw==raw){slot=j;break;}
        if(slot==bank.resources.size()){++bank.dropped;continue;}if(slot==bank.transitions)++bank.transitions;
        // pResource is typed by the intercepted D3D12 API. NGX raw parameters
        // never enter typed()/QueryInterface. This records a transition request,
        // not a globally complete resource-state tracker or GPU completion.
        if(typed_observations==512){++bank.dropped;continue;}++typed_observations;++total_typed_observations;
        bank.resources[slot]={typed(b.Transition.pResource),ref,bank.serial,caller,unsigned(b.Transition.StateBefore),
            unsigned(b.Transition.StateAfter),b.Transition.Subresource,unsigned(b.Flags)};
    }
}
void targets(ID3D12GraphicsCommandList* command,UINT count,D3D12_CPU_DESCRIPTOR_HANDLE const* handles,BOOL contiguous,D3D12_CPU_DESCRIPTOR_HANDLE const* depth){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!select_bank())return;auto ref=list(command);
    if(!valid(ref)||count>8||frame_target_stats.descriptor_capacity_lost()){++bank.dropped;return;}
    unsigned slot=bank.binds;for(unsigned i=0;i<bank.binds;++i)if(same(bank.bindings[i].list,ref)){slot=i;break;}
    if(slot==bank.bindings.size()){++bank.dropped;return;}if(slot==bank.binds)++bank.binds;
    Bind out;out.list=ref;out.count=count;out.complete=true;D3D12_CPU_DESCRIPTOR_HANDLE first{};
    const auto rtv_stride=contiguous&&count>1?descriptor_stride(command):1u;
    if(count&&(!handles||!safe_copy(&first,handles,sizeof(first))))out.complete=false;
    for(unsigned i=0;out.complete&&i<count;++i){D3D12_CPU_DESCRIPTOR_HANDLE handle{};
        if(contiguous){if(!rtv_stride||uint64_t(i)*rtv_stride>UINTPTR_MAX-first.ptr){out.complete=false;break;}handle.ptr=first.ptr+SIZE_T(i)*rtv_stride;}
        else if(!safe_copy(&handle,handles+i,sizeof(handle))){out.complete=false;break;}
        auto target=frame_target_stats.lookup(handle.ptr);out.resources[i]=target.resource;
        if(!target.resource||target.mip)out.complete=false;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE depth_value{};if(depth&&safe_copy(&depth_value,depth,sizeof(depth_value)))out.depth_handle=depth_value.ptr;
    bank.bindings[slot]=out;if(!out.complete)++bank.dropped;
}
void draw(ID3D12GraphicsCommandList* command,uintptr_t caller,unsigned elements,unsigned instances){
    if(!elements||!instances)return;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!select_bank())return;
    auto ref=list(command);Bind const* bound{};for(unsigned i=0;i<bank.binds;++i)if(same(bank.bindings[i].list,ref)){bound=&bank.bindings[i];break;}
    if(!bound||!bound->complete){++bank.dropped;return;}auto rect=frame_target_stats.draw_state(ref.raw);
    const auto owner=ngx_route_observer::ownership(true);const auto serial=++bank.serial;
    for(unsigned j=0;j<bound->count;++j){unsigned slot=bank.draws;
        for(unsigned i=0;i<bank.draws;++i){auto const& previous=bank.producers[i];if(previous.resource==bound->resources[j]&&
            same(previous.list,ref)&&previous.view==owner.view&&previous.rectangles.viewport==rect.viewport&&previous.rectangles.scissor==rect.scissor){slot=i;break;}}
        if(slot==bank.producers.size()){++bank.dropped;continue;}
        if(slot==bank.draws){++bank.draws;bank.producers[slot]={bound->resources[j],caller,owner.view,ref,rect,serial,serial,1};}
        else{auto& saved=bank.producers[slot];saved.last=serial;++saved.count;saved.caller=caller;}
    }
}
void sample_renderer(Row& row){auto const& owner=row.before.ownership;if(!owner.associated||owner.count!=2||!owner.views)return;
    if(read(row.renderer_extent,owner.renderer,0xd0))row.renderer_mask|=1;
    if(read(row.buffer_extent[0],base,0x1c3b3b8)&&read(row.buffer_extent[1],base,0x1c3b3bc))row.renderer_mask|=2;
    for(unsigned i=0;i<2;++i){auto& view=row.views[i];if(owner.views>UINTPTR_MAX-uintptr_t(i)*0xd30)continue;view.address=owner.views+uintptr_t(i)*0xd30;
        if(read(view.source,view.address,0x48))view.mask|=1;if(read(view.target,view.address,0x58))view.mask|=2;if(read(view.sampling,view.address,0x3d0))view.mask|=4;}
}
void join_inputs(Row& row){if(!row.evaluate||bank.world!=row.before.ownership.world||bank.ticket!=row.before.ownership.ticket)return;
    row.bank_dropped=bank.dropped;
    std::array<Typed,4> retained{};
    for(unsigned i=0;i<4;++i){for(unsigned j=0;j<bank.transitions;++j){auto const& t=bank.resources[j];
        if(t.resource.raw!=row.before.resources[i])continue;row.inputs[i]=t;
        if(t.resource.canonical&&t.resource.device&&SUCCEEDED(t.resource.hr))row.typed_mask|=1u<<i;
        retained[i]=t.resource;if(t.resource.retained())row.retained_mask|=1u<<i;
        if(t.list.raw==row.command)row.command_list=t.list;break;}}
    row.typed_set_ready=gears_ngx_stereo::resolve_typed_resources(retained,row.before.resources,row.adapter_resources);
    // Sample order follows native structure: Color,Output,Depth,MV. Producer
    // order is Color,Depth,MV, joined to actual RTV binding/draw observations.
    constexpr unsigned input_indices[]{0,2,3};
    for(unsigned i=0;i<3;++i){auto canonical=row.inputs[input_indices[i]].resource.canonical;
        for(unsigned j=0;canonical&&j<bank.draws;++j){auto const& d=bank.producers[j];if(d.resource!=canonical)continue;
            auto& latest=row.producers[2*i];auto& previous=row.producers[2*i+1];
            if(d.last>latest.last){previous=latest;latest=d;}else if(d.last>previous.last)previous=d;row.producer_mask|=1u<<i;}}
}
WrapperScope::WrapperScope(bool eval,bool owned,uintptr_t caller,uintptr_t command,void const* structure)
    :previous(current_row),exceptions(std::uncaught_exceptions()),evaluate(eval){
    current_row=-1;pending_row=-1;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!attested||!recording.load())return;
    const bool concurrent=in_wrappers++!=0;counted=true;if(previous>=0)rows[unsigned(previous)].nested=true;
#if GEARS_NGX_MV_TARGET_CAPTURE
    if(eval){target_previous=target_wrapper;target_wrapper=this;target_linked=true;}
    if(concurrent)ngx_mv_target_capture::invalidate(5);
    else if(!eval)ngx_mv_target_capture::invalidate(1);
#endif
    if(concurrent){latest_native_create={};++native_create_revision;}
#if defined(GEARS_NGX_INPUT_RECT_EXPERIMENT) && GEARS_NGX_INPUT_RECT_EXPERIMENT
    // A late Create or changed feature cannot certify a previously constructed
    // reduced-input ticket. Stop the experiment; original native work still
    // runs so registered renderers retain their ordinary destruction path.
    if(consuming_frozen_frame.ngx_constructor_source.revision&&
       (!owned||!eval||!native_create_matches(consuming_frozen_frame.ngx_constructor_source))){
        request_session_stop(SessionStop::proof_failure);ngx_epoch_pair::changed();}
#endif
    if(!eval){
        latest_native_create={};source_seen=true;previous_source=source_create_scope;source_create_scope=this;
        source.revision=++native_create_revision;source.world=committed_world_generation.load();source_command=command;source_structure=reinterpret_cast<uintptr_t>(structure);
        source_bad=concurrent||previous_source||caller!=base+0xbe66bc||!source.world||
            !read(source.parameters,base,gears_ngx_route::parameter_cell_rva)||!source.parameters||
            !read_create_fields(source.fields,reinterpret_cast<uintptr_t>(structure));
    }
    if(!eval){++epochs;epoch_evals=0;bank={};
        // A menu-created DLSS feature may follow many ordinary frames. Their
        // exhausted bank must not permanently suppress the new feature's typed
        // inputs. Initial epoch plus at most three Creates bounds total work.
        if(epochs<4)sampled_tickets=typed_observations=0;}
#if defined(GEARS_NGX_EPOCH_PAIR_WIRING) && GEARS_NGX_EPOCH_PAIR_WIRING
    if(eval)ngx_epoch_pair::eval_enter(reinterpret_cast<uintptr_t>(this),owned,caller,command,structure,concurrent);
#endif
#if GEARS_NGX_MV_TARGET_CAPTURE
    if(eval)ngx_mv_target_capture::eval_enter(*this,owned,caller,command,structure,concurrent);
#endif
    if(epochs>=4||(eval&&epoch_evals>=2)||row_count==rows.size()){++limited;return;}
    gears_ngx_route::Row native;native.evaluate=eval;native.structure=reinterpret_cast<uintptr_t>(structure);
    auto before=ngx_route_observer::sample(native,owned);
    if(eval&&(!before.ownership.associated||caller!=base+0x9a35d3)){++limited;return;}
    row=int(row_count++);current_row=row;auto& value=rows[unsigned(row)];value.evaluate=eval;value.epoch=epochs;value.sequence=++sequence;
    value.caller=caller;value.command=command;value.structure=native.structure;value.before=before;value.entry=now();value.thread=GetCurrentThreadId();
    value.nested=previous>=0;value.concurrent=concurrent;if(eval){++epoch_evals;sample_renderer(value);join_inputs(value);}
}
WrapperScope::~WrapperScope(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(source_seen){
        uintptr_t handle{},parameters{};std::array<uint32_t,7> fields{};
        if(!source_bad&&source_returned&&returned&&std::uncaught_exceptions()==exceptions&&source_dispatches==1&&
           in_wrappers==1&&source.revision==native_create_revision&&source.world==committed_world_generation.load()&&
           (source_result&0xfff00000u)!=0xbad00000u&&uint32_t(result)==source_result&&
           read(handle,base,gears_ngx_route::handle_cell_rva)&&handle&&handle==source.handle&&
           read(parameters,base,gears_ngx_route::parameter_cell_rva)&&parameters==source.parameters&&
           read_create_fields(fields,source_structure)&&fields==source.fields){latest_native_create=source;
#if defined(GEARS_NGX_EPOCH_PAIR_WIRING) && GEARS_NGX_EPOCH_PAIR_WIRING
            ngx_epoch_pair::created(source);
#endif
        }else{
#if defined(GEARS_NGX_EPOCH_PAIR_WIRING) && GEARS_NGX_EPOCH_PAIR_WIRING
            ngx_epoch_pair::changed();
#endif
        }
        source_create_scope=previous_source;
    }
#if defined(GEARS_NGX_EPOCH_PAIR_WIRING) && GEARS_NGX_EPOCH_PAIR_WIRING
    if(evaluate)ngx_epoch_pair::eval_leave(reinterpret_cast<uintptr_t>(this),returned&&std::uncaught_exceptions()==exceptions,result);
#endif
#if GEARS_NGX_MV_TARGET_CAPTURE
    if(evaluate)ngx_mv_target_capture::eval_leave(*this,returned,std::uncaught_exceptions()!=exceptions,result);
    if(target_linked)target_wrapper=target_previous;
#endif
    if(counted&&in_wrappers)--in_wrappers;
    if(row>=0){auto& value=rows[unsigned(row)];gears_ngx_route::Row native;native.evaluate=evaluate;native.structure=value.structure;
        value.after=ngx_route_observer::sample(native,true);value.returned=returned;value.wrapper_result=result;
        value.unwinding=std::uncaught_exceptions()!=exceptions;value.exit=now();if(evaluate&&returned&&!value.unwinding)pending_row=row;}
    current_row=previous;
}
uint32_t __fastcall hook_create(uintptr_t command,uint32_t feature,uintptr_t parameters,uintptr_t* output){
    Scope active_scope;int selected=-1;WrapperScope* source_owner{};{std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        source_owner=source_create_scope;
        if(attested&&recording.load()&&source_owner){
            if(++source_owner->source_dispatches!=1||command!=source_owner->source_command||feature!=1||
               parameters!=source_owner->source.parameters||reinterpret_cast<uintptr_t>(output)!=base+gears_ngx_route::handle_cell_rva)
                source_owner->source_bad=true;
        }else{source_owner=nullptr;latest_native_create={};++native_create_revision;}
        if(attested&&recording.load()&&current_row>=0&&!rows[unsigned(current_row)].evaluate){selected=current_row;auto& row=rows[unsigned(selected)];++row.dispatches;
            row.dispatcher_entry=now();row.consumed_parameters=parameters;row.dispatcher_match=row.dispatches==1&&row.command==command&&
                row.before.slots_mask==3&&row.before.fields_mask==127&&row.before.parameters==parameters&&feature==1&&reinterpret_cast<uintptr_t>(output)==base+gears_ngx_route::handle_cell_rva;}
        else ++foreign_dispatches;}
    auto owner=current_control();const auto result=owner?owner->bridge().create_dispatch(command,feature,parameters,reinterpret_cast<uint64_t*>(output),original_create):
        original_create(command,feature,parameters,output);
    if(source_owner){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        source_owner->source_result=result;source_owner->source_returned=true;
        if(!read(source_owner->source.handle,reinterpret_cast<uintptr_t>(output)))source_owner->source_bad=true;}
    if(selected>=0){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto& row=rows[unsigned(selected)];row.dispatch_result=result;row.dispatcher_returned=true;row.dispatcher_exit=now();
        if(!read(row.handle_output,reinterpret_cast<uintptr_t>(output)))++read_failures;read(row.vendor,base,0x1b55d78);row.vendor_module_named=vendor_owner(row.vendor,row.vendor_module);}
    return result;
}
uint32_t __fastcall hook_evaluate(uintptr_t command,uintptr_t handle,uintptr_t parameters,uintptr_t callback){
    Scope active_scope;int selected=-1;
#if GEARS_NGX_MV_TARGET_CAPTURE
    WrapperScope* target{};
#endif
#if defined(GEARS_NGX_EPOCH_PAIR_WIRING) && GEARS_NGX_EPOCH_PAIR_WIRING
    int pair_dispatch=-1;
#endif
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
#if defined(GEARS_NGX_EPOCH_PAIR_WIRING) && GEARS_NGX_EPOCH_PAIR_WIRING
        pair_dispatch=ngx_epoch_pair::dispatch(command,handle,parameters,callback);
#endif
#if GEARS_NGX_MV_TARGET_CAPTURE
        target=target_wrapper;ngx_mv_target_capture::sdk_enter(target,command,handle,parameters,callback);
#endif
        if(attested&&recording.load()&&current_row>=0&&rows[unsigned(current_row)].evaluate){selected=current_row;auto& row=rows[unsigned(selected)];++row.dispatches;
            row.dispatcher_entry=now();row.consumed_handle=handle;row.consumed_parameters=parameters;row.callback=callback;
            row.dispatcher_match=row.dispatches==1&&row.command==command&&row.before.slots_mask==3&&row.before.fields_mask==0x7ffff&&row.before.handle==handle&&row.before.parameters==parameters&&!callback;
            read(row.vendor,base,0x1b55d80);row.vendor_module_named=vendor_owner(row.vendor,row.vendor_module);}
        else ++foreign_dispatches;}
    auto owner=current_control();const auto result=owner?owner->bridge().evaluate_dispatch(command,handle,parameters,callback,original_evaluate):
        original_evaluate(command,handle,parameters,callback);
#if GEARS_NGX_MV_TARGET_CAPTURE
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);ngx_mv_target_capture::sdk_leave(target,result);}
#endif
    if(selected>=0){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto& row=rows[unsigned(selected)];row.dispatch_result=result;row.dispatcher_returned=true;row.dispatcher_exit=now();}
#if defined(GEARS_NGX_EPOCH_PAIR_WIRING) && GEARS_NGX_EPOCH_PAIR_WIRING
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);ngx_epoch_pair::dispatched(pair_dispatch,result);}
#endif
    return result;
}
uint32_t __fastcall hook_release(uintptr_t handle){Scope active_scope;int selected=-1;
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
#if GEARS_NGX_MV_TARGET_CAPTURE
        if(handle==latest_native_create.handle||in_wrappers)ngx_mv_target_capture::invalidate(2);
#endif
#if defined(GEARS_NGX_EPOCH_PAIR_WIRING) && GEARS_NGX_EPOCH_PAIR_WIRING
        ngx_epoch_pair::released(handle);
#endif
        if(handle==latest_native_create.handle||in_wrappers){latest_native_create={};++native_create_revision;}
        if(attested&&recording.load()&&release_count<releases.size()){
        selected=int(release_count++);auto& row=releases[unsigned(selected)];row.handle=handle;row.caller=reinterpret_cast<uintptr_t>(_ReturnAddress());row.entry=now();
        read(row.vendor,base,0x1b55d88);row.vendor_module_named=vendor_owner(row.vendor,row.module);}}
    auto owner=current_control();const auto result=owner?owner->release(handle,original_release):original_release(handle);
    if(selected>=0){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto& row=releases[unsigned(selected)];row.result=result;row.returned=true;row.exit=now();}return result;
}
void copy_passthrough(ID3D12GraphicsCommandList* command,D3D12_TEXTURE_COPY_LOCATION const* destination,UINT x,UINT y,UINT z,
    D3D12_TEXTURE_COPY_LOCATION const* source,D3D12_BOX const* box,uintptr_t caller,CopyFn forward){
#if defined(GEARS_NGX_EPOCH_PAIR_OBSERVER) && GEARS_NGX_EPOCH_PAIR_OBSERVER
    ngx_epoch_pair::CopyScope epoch_copy(command,destination,x,y,z,source,box,caller);
#endif
    int selected=-1;D3D12_TEXTURE_COPY_LOCATION src{},dst{};
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);int candidate=current_row>=0?current_row:pending_row;
        if(attested&&recording.load()&&candidate>=0&&rows[unsigned(candidate)].evaluate&&safe_copy(&src,source,sizeof(src))&&safe_copy(&dst,destination,sizeof(dst))){
            auto& row=rows[unsigned(candidate)];if(reinterpret_cast<uintptr_t>(src.pResource)==row.before.resources[1]&&reinterpret_cast<uintptr_t>(dst.pResource)==row.before.resources[0]){
                selected=candidate;++row.copies;row.copy_caller=caller;row.copy_list=list(command);row.copied={typed(src.pResource),typed(dst.pResource)};
                row.copy_source_type=src.Type;row.copy_destination_type=dst.Type;row.copy_source_subresource=src.SubresourceIndex;row.copy_destination_subresource=dst.SubresourceIndex;
                row.copy_destination={x,y,z};row.copy_box=box!=nullptr;}}}
#if defined(GEARS_NGX_CAPTURE_CONTROL_WIRING) && GEARS_NGX_CAPTURE_CONTROL_WIRING
    if(!ngx_capture_control::copy(command,destination,x,y,z,source,box,caller,forward))
#endif
        forward(command,destination,x,y,z,source,box);
#if defined(GEARS_NGX_EPOCH_PAIR_OBSERVER) && GEARS_NGX_EPOCH_PAIR_OBSERVER
    epoch_copy.returned(command,destination,source,box);
#endif
    if(selected>=0){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto& row=rows[unsigned(selected)];D3D12_TEXTURE_COPY_LOCATION fresh_source{},fresh_destination{};
        row.copy_returned=safe_copy(&fresh_source,source,sizeof(fresh_source))&&safe_copy(&fresh_destination,destination,sizeof(fresh_destination))&&
            fresh_source.pResource==src.pResource&&fresh_destination.pResource==dst.pResource&&fresh_source.Type==src.Type&&fresh_destination.Type==dst.Type&&same(list(command),row.copy_list);}
}
void execute_passthrough(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* commands,ExecuteFn forward){
#if defined(GEARS_NGX_EPOCH_PAIR_OBSERVER) && GEARS_NGX_EPOCH_PAIR_OBSERVER
    ngx_epoch_pair::ExecuteScope epoch_execute(queue,count,commands);
#endif
    std::array<List,64> members{};bool readable=false;uint64_t order{};
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(attested&&recording.load()){
        order=++execute_sequence;std::array<ID3D12CommandList*,64> raw{};readable=count<=raw.size()&&(!count||safe_copy(raw.data(),commands,count*sizeof(raw[0])));
        if(readable)for(unsigned i=0;i<count;++i)members[i]=list(raw[i]);}}
#if defined(GEARS_NGX_CAPTURE_CONTROL_WIRING) && GEARS_NGX_CAPTURE_CONTROL_WIRING
    ngx_capture_control::execute(queue,count,commands,forward);
#else
    forward(queue,count,commands);
#endif
#if defined(GEARS_NGX_EPOCH_PAIR_OBSERVER) && GEARS_NGX_EPOCH_PAIR_OBSERVER
    epoch_execute.returned();
#endif
    if(!order)return;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);const auto queue_id=canonical_identity(queue);
    for(unsigned i=0;readable&&i<row_count;++i){auto& row=rows[i];
        // Eval and native Output->Color may be different physical recordings.
        // Never let observing Copy's Execute stand in for Eval's Execute.
        if(row.command_list.raw)for(unsigned j=0;j<count;++j)if(members[j].raw==row.command_list.raw){
            ++row.eval_execute_occurrences;if(!row.eval_execute_order){row.eval_execute_order=order;row.eval_execute_member=j;row.eval_queue=queue_id;
                row.eval_execute_generation_match=valid(members[j])&&same(members[j],row.command_list);row.eval_execute_returned=true;}
            if(row.eval_execute_occurrences!=1)row.eval_execute_generation_match=false;}
        List wanted=row.copy_list;if(!wanted.raw)continue;
        for(unsigned j=0;j<count;++j)if(members[j].raw==wanted.raw){++row.execute_occurrences;if(!row.execute_order){row.execute_order=order;row.execute_member=j;row.queue=queue_id;
            row.execute_generation_match=valid(members[j])&&same(members[j],wanted);row.execute_returned=true;}}}
}
struct Contract {uintptr_t rva;unsigned size;char const* hash;};
// Artifact 1 historical runtime section (base 0x140000000), code identity
// 911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6.
// Full function chunks from the saved wrapper/release contracts, not disk-EXE
// identity. enable() hashes every complete current chunk before any host hook.
constexpr Contract contracts[]{
#if GEARS_NGX_MV_TARGET_CAPTURE
    {0x17be48,159,"e2d30368522c5974dde25b589ada9148abd7519b90b9465540f51c6e81fcfcaf"},
    {0x8b8d30,55,"870433ad9e5b23eae335b1dd87be9e945750aa2576d64320e23bc0c7abf8936e"},
    {0x1ad081,372,"ba02432bf4774e29809c5c929b1af1cf539ac67d60e82f53f2d650ef05ce2c78"},
    {0x1ad280,107,"93bd5488a41306c3d29eef626af0f4f20ae779d22ab8dfc10b4833d01d4a0005"},
    // Complete setup/initialization chunks. Admission uses the typed root
    // callback's current return address, never a recovered native ABI.
    {0x138020,438,"5f60a7a5281df647b8f6b37deb0fe5a725fee6a240d43712157baa5958d32f0c"},
    {0x2200a8,140,"4e6733c8ab5e39dead5557842ad1d0c86eece582d424ce54b73a8192084c1783"},
    {0x220134,136,"70b6ffde6ae8a12e5da60db390a0ca02cc7ed0ec2d79a765b814cbb66e1bb572"},
    {0x8d786e,35,"710e19e7af3527d0a64ee51849aa3336ed2b0b9305d90386f31a238b32c4b289"},
    // Complete hot/cold chunks from artifact1 compute-chunks.json. These
    // authenticate the historical root-table/Dispatch caller candidates,
    // without asserting that the shader writes the selected MV resource.
    {0x1ac6e4,554,"f0d3877d9829b76031efa5fbd57aaffc087f85cc6dff5e119d10a70b5ce9cf6a"},
    {0x8c2ad8,178,"e3c537f27a6eae88cea5c7aba7ed9661be5b1088bce04008c7b56ad306d1e0e1"},
    {0x17b420,2554,"942d8c3413c5b4e3d076e2e0515992a4071db3a0167abfda1d2584a47a1d7e2e"},
    {0x8b8b96,383,"12e23d12bb5c5579487b624c0bb61345dd709b6f066baa6cfa7b7f0f1cb3331d"},
#endif
    // Native zero-offset/null-box Copy route and its state setup/restore.
    {0x1c5dd8,0x315,"038bef1d04e09846166ed3dd5479a71f6a9cc548469d5b8f1626553f1f137bf4"},
    {0x897f50,0x239,"ce26c00b6cd9d2148ec2f94f53e3ca11fab8127b943f08386d6d2a7c199e71c7"},
    {0x1c60f0,0x8c,"0c9f3023bb7afd53bf933be7ea725a591e548ba5ec2b2e44e8eab2d790ccc61b"},
    {0x1c617c,0x43,"421a79a0fc79095b9082dae3e29d7ea221705decb0da2fce586c99d23ce5231e"},
    {0x17c0b4,0x10e,"16a728d1d2df0385da04fe63c8bc43d01941c3d34384445489820180e076ae94"},
    {0x17ab94,0x327,"dd18f288f7976969f3be83427e41455328568263995755e1aced455707db0a86"},
    {0xbe61bc,0x187,"1ac9bd946037e33c6b06f2b6b8211eec72ee7096bec92912742c41150279500d"},
    {0xbe5b5c,0xf9,"53709f650d84bd1ea4cc1ae5e25eb8ef7f3462102fe51d07b790d96c48734879"},
    {0xbe5c58,0x561,"21868e6f8203578f5c4ef689fc4f66a5c79e13fdb243eebe2eb81cc7d6309441"},
    {0xbe65cc,0x446,"a00d6a1971ceef0a066f350562c2c4033ab7beedb83e34568dd9e72f637fe727"},
    {0x9a33ec,0x251,"3b871bb6c564fa06c42ce08d805791fb3450447e6fd98f7ff49359fc9766878f"},
    {0x7c82f0,0x76,"25cb57848839e9bf6d5da04fefca109a4a30326b74fcb7f90d5340aa84f969a9"},
    {0x7c8440,0x35,"ac6de83a5c7a3c8451f842391785c00e492980557c84328d55509537874d82e7"},
    {0x7c7bd0,0x90,"3c77ed054021542de9bb7589142e0f595166fd567a6b0e339661fffc6b9b941e"},
    {0x7c87e0,0x45,"ba5c32ea310653f741ee1d10b735249162778259755de6dcb0e8a673b02fff63"},
    {0xbe6aac,0x6c,"45d9331f52def2e0d92fc6339bebc53a169c04413a20265f9c6992a981479b48"},
    {0x7c80d0,0x152,"f42ccc68bce5664a9d28f2219904a8671ab47975f4a455d9a4f19ede83d16689"}
};
bool enable(FILE* file){if(current_control())return false; // Never reset a retained live SDK owner.
#if defined(GEARS_NGX_EPOCH_PAIR_WIRING) && GEARS_NGX_EPOCH_PAIR_WIRING
    ngx_epoch_pair::reset_capture(false);
#endif
    attested=false;printed=false;bank={};rows={};releases={};row_count=epoch_evals=epochs=in_wrappers=release_count=0;
    latest_native_create={};native_create_revision=0;source_create_scope=nullptr;
    sampled_tickets=typed_observations=total_sampled_tickets=total_typed_observations=0;
    sequence=execute_sequence=limited=foreign_dispatches=read_failures=0;current_row=pending_row=-1;
    if(self_testing||!continuous_session||!frame_link_diagnostics||!color_copy_diagnostics)return false;
    bool matched=true;for(auto const& contract:contracts)matched=ngx_route_observer::verify(file,contract.rva,contract.size,contract.hash)&&matched;
    attested=matched;
#if defined(GEARS_NGX_EPOCH_PAIR_WIRING) && GEARS_NGX_EPOCH_PAIR_WIRING
    ngx_epoch_pair::reset_capture(matched);
#endif
    fprintf(file,"ngx_stereo_host_identity enabled=1 attested=%u chunks=%zu mode=readonly sdk_mutation=0 gpu_commands_added=0\n",unsigned(attested),sizeof(contracts)/sizeof(contracts[0]));return attested;
}
bool install_hooks(){return attested&&install(reinterpret_cast<void*>(base+0x7c82f0),reinterpret_cast<void*>(hook_create),reinterpret_cast<void**>(&original_create))&&
    install(reinterpret_cast<void*>(base+0x7c8440),reinterpret_cast<void*>(hook_evaluate),reinterpret_cast<void**>(&original_evaluate))&&
    install(reinterpret_cast<void*>(base+0x7c87e0),reinterpret_cast<void*>(hook_release),reinterpret_cast<void**>(&original_release));}
void print(FILE* file){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
#if defined(GEARS_NGX_EPOCH_PAIR_WIRING) && GEARS_NGX_EPOCH_PAIR_WIRING
    ngx_epoch_pair::print(file);
#endif
    if(printed)return;printed=true;
    const bool drained=!recording.load()&&active.load()==0;
    fprintf(file,"ngx_stereo_host_budget sampled_tickets=%u ticket_limit_per_epoch=8 typed_observations=%u typed_limit_per_epoch=512 total_sampled_tickets=%u total_ticket_limit=32 total_typed_observations=%u total_typed_limit=2048 per_ticket_resources=64 per_ticket_draw_groups=64 draw_candidates_per_input=2\n",sampled_tickets,typed_observations,total_sampled_tickets,total_typed_observations);
    fprintf(file,"ngx_stereo_host enabled=1 attested=%u rows=%u epochs=%u limited=%llu foreign_dispatches=%llu read_failures=%llu rows_unavailable=%u gpu_completion_verified=0 producer_draw_is_write_proof=0 typed_resource_leases=1 depth_stencil_descriptor_resolved=0\n",
        unsigned(attested),row_count,epochs,limited,foreign_dispatches,read_failures,unsigned(!drained));if(!drained)return;
    for(unsigned i=0;i<row_count;++i){auto const& row=rows[i];auto const& owner=row.before.ownership;
        fprintf(file,"ngx_stereo_host_typed row=%u retained_mask=%x typed_set_ready=%u eval_queue=%llx eval_execute_order=%llu eval_member=%u eval_occurrences=%u eval_returned=%u eval_generation_match=%u\n",
            i,row.retained_mask,unsigned(row.typed_set_ready),row.eval_queue,row.eval_execute_order,row.eval_execute_member,row.eval_execute_occurrences,unsigned(row.eval_execute_returned),unsigned(row.eval_execute_generation_match));
        fprintf(file,"ngx_stereo_host_call row=%u kind=%s epoch=%llu world=%llu ticket=%llu thread=%u wrapper_command=%llx wrapper_caller=%llx wrapper_returned=%u unwinding=%u nested=%u concurrent=%u wrapper_result=%llx dispatches=%u dispatch_match=%u dispatch_returned=%u dispatch_result=%x consumed_handle=%llx consumed_parameters=%llx callback=%llx handle_output=%llx vendor=%llx vendor_module=%llx vendor_named=%u entry=%lld exit=%lld dispatch_entry=%lld dispatch_exit=%lld typed_mask=%x producer_mask=%x bank_dropped=%u command_list=%llx,%llx,%llu render=%u,%u bases=%u,%u;%u,%u;%u,%u reset=%u\n",
            i,row.evaluate?"evaluate":"create",row.epoch,owner.world,owner.ticket,row.thread,row.command,row.caller,unsigned(row.returned),unsigned(row.unwinding),unsigned(row.nested),unsigned(row.concurrent),row.wrapper_result,row.dispatches,unsigned(row.dispatcher_match),unsigned(row.dispatcher_returned),row.dispatch_result,row.consumed_handle,row.consumed_parameters,row.callback,row.handle_output,row.vendor,row.vendor_module,unsigned(row.vendor_module_named),row.entry,row.exit,row.dispatcher_entry,row.dispatcher_exit,row.typed_mask,row.producer_mask,row.bank_dropped,row.command_list.raw,row.command_list.canonical,row.command_list.generation,row.before.values[0],row.before.values[1],row.before.values[7],row.before.values[8],row.before.values[9],row.before.values[10],row.before.values[11],row.before.values[12],row.before.values[2]);
        for(unsigned j=0;j<4;++j){auto const& t=row.inputs[j];auto const& r=t.resource;fprintf(file,"ngx_stereo_host_resource row=%u input=%u ngx_raw=%llx typed_raw=%llx canonical=%llx device=%llx hr=%lx size=%llu,%u format=%u dimension=%u mips=%u layers=%u samples=%u transition_list=%llx,%llx,%llu transition=%x,%x subresource=%u flags=%u serial=%llu caller=%llx state_coverage_complete=0\n",
            i,j,row.before.resources[j],r.raw,r.canonical,r.device,static_cast<unsigned long>(r.hr),r.desc.Width,r.desc.Height,unsigned(r.desc.Format),unsigned(r.desc.Dimension),unsigned(r.desc.MipLevels),unsigned(r.desc.DepthOrArraySize),r.desc.SampleDesc.Count,t.list.raw,t.list.canonical,t.list.generation,t.before,t.after,t.subresource,t.flags,t.serial,t.caller);}
        for(unsigned j=0;j<6;++j){auto const& d=row.producers[j];auto const& r=d.rectangles;fprintf(file,"ngx_stereo_host_producer row=%u input=%u candidate=%u resource=%llx view=%llx list=%llx,%llx,%llu caller=%llx draws=%llu serial=%llu,%llu tracked=%u viewport_count=%u scissor_count=%u viewport=%.9g,%.9g,%.9g,%.9g scissor=%.9g,%.9g,%.9g,%.9g\n",i,j/2,j%2,d.resource,d.view,d.list.raw,d.list.canonical,d.list.generation,d.caller,d.count,d.first,d.last,unsigned(r.tracked),r.viewport_count,r.scissor_count,r.viewport.x,r.viewport.y,r.viewport.width,r.viewport.height,r.scissor.x,r.scissor.y,r.scissor.width,r.scissor.height);}
        for(unsigned j=0;j<2;++j){auto const& v=row.views[j];fprintf(file,"ngx_stereo_host_view row=%u eye=%u address=%llx mask=%x source=%.9g,%.9g,%.9g,%.9g target=%d,%d,%d,%d sampling=%.9g,%.9g,%.9g,%.9g renderer_mask=%x union=%u,%u buffers=%u,%u\n",i,j,v.address,v.mask,v.source[0],v.source[1],v.source[2],v.source[3],v.target[0],v.target[1],v.target[2],v.target[3],v.sampling[0],v.sampling[1],v.sampling[2],v.sampling[3],row.renderer_mask,row.renderer_extent[0],row.renderer_extent[1],row.buffer_extent[0],row.buffer_extent[1]);}
        fprintf(file,"ngx_stereo_host_copy_execute row=%u copies=%u copy_returned=%u copy_caller=%llx copy_list=%llx,%llx,%llu copy_source=%llx,%llx copy_dest=%llx,%llx destination=%u,%u,%u box=%u source_type=%u dest_type=%u source_sub=%u dest_sub=%u queue=%llx execute_order=%llu member=%u occurrences=%u execute_returned=%u generation_match=%u signal_observed=0 fence_completion_observed=0\n",i,row.copies,unsigned(row.copy_returned),row.copy_caller,row.copy_list.raw,row.copy_list.canonical,row.copy_list.generation,row.copied[0].raw,row.copied[0].canonical,row.copied[1].raw,row.copied[1].canonical,row.copy_destination[0],row.copy_destination[1],row.copy_destination[2],unsigned(row.copy_box),row.copy_source_type,row.copy_destination_type,row.copy_source_subresource,row.copy_destination_subresource,row.queue,row.execute_order,row.execute_member,row.execute_occurrences,unsigned(row.execute_returned),unsigned(row.execute_generation_match));
    }
    for(unsigned i=0;i<release_count;++i){auto const& row=releases[i];fprintf(file,"ngx_stereo_host_release row=%u handle=%llx caller=%llx result=%x returned=%u vendor=%llx module=%llx vendor_named=%u entry=%lld exit=%lld safe_retirement_verified=0\n",i,row.handle,row.caller,row.result,unsigned(row.returned),row.vendor,row.module,unsigned(row.vendor_module_named),row.entry,row.exit);}
    for(auto& transition:bank.resources)transition.resource.release();
    for(auto& row:rows){for(auto& input:row.inputs)input.resource.release();for(auto& copied:row.copied)copied.release();}
}
#endif
}
