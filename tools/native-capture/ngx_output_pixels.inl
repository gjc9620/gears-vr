namespace ngx_output_pixels {
#if defined(GEARS_NGX_OUTPUT_PIXELS) && GEARS_NGX_OUTPUT_PIXELS
bool enabled{},stopped{},published{},initialized{};std::array<Slot,2> slots{};unsigned count{},failure{};
uint64_t world{},ticket{},eval_sequence{},copy_sequence{},execute_sequence{},bypassed{},unrelated{},limited{};unsigned owner_thread{};
thread_local int active_eval=-1,pending_eval=-1;
struct GapReceipt {bool boundary{},copy{};int pending{-1};long long boundary_qpc{},copy_qpc{};uintptr_t caller{},command{},structure{},copy_caller{},copy_list{},source{},destination{};unsigned thread{},copy_thread{};uint64_t world{},ticket{};};
GapReceipt gap;
#if defined(GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT) && GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT
SuppressionReceipt suppression;
#endif
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
gears_ngx_delayed::Gate delayed;
void quarantine(unsigned);
void delayed_failure(){if(delayed.selected&&!delayed.armed)quarantine(association_ambiguous);}
#endif
bool same_owner(gears_ngx_route::Ownership const& a,gears_ngx_route::Ownership const& b){
    return a.associated&&b.associated&&a.world==b.world&&a.ticket==b.ticket&&a.renderer==b.renderer&&a.context==b.context&&a.view==b.view&&a.views==b.views&&a.count==b.count;
}
gears_ngx_route::Sample eval_sample(uintptr_t structure,bool owned){gears_ngx_route::Row row;row.evaluate=true;row.structure=structure;return ngx_route_observer::sample(row,owned);}
void quarantine(unsigned reason){if(!failure)failure=reason;for(unsigned i=0;i<count;++i)slots[i].life.fail(reason);}
void ambiguous_boundary(int pending,bool owned,uintptr_t caller,uintptr_t command,const void* structure){
    if(!gap.boundary){gap.boundary=true;gap.pending=pending;gap.boundary_qpc=now();gap.caller=caller;gap.command=command;
        gap.structure=reinterpret_cast<uintptr_t>(structure);gap.thread=GetCurrentThreadId();auto const owner=ngx_route_observer::ownership(owned);gap.world=owner.world;gap.ticket=owner.ticket;}
    quarantine(association_ambiguous);
}
void foreign_boundary(bool owned,uintptr_t caller,uintptr_t command,const void* structure){
    // TLS cannot describe an Eval/Create on another thread. Invalidate the
    // global unfinished interval BEFORE any ownership/caller/quota bypass.
    if(!enabled||stopped||failure||!count||owner_thread==GetCurrentThreadId())return;
    for(unsigned i=0;i<count;++i)if(!slots[i].copy_seen){ambiguous_boundary(int(i),owned,caller,command,structure);return;}
}
bool current_owner(Slot const& s){return !stopped&&enabled&&!failure&&recording.load()&&s.thread==GetCurrentThreadId()&&
    same_owner(s.before.ownership,ngx_route_observer::ownership(true));}
void publish_pair(){
    if(published||stopped||failure||count!=2)return;
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
    if(!delayed.closed)return;
    for(auto const& s:slots)if(!s.normal||!s.copy_seen||!s.life.appended||s.life.failure)return;
#else
    for(auto const& s:slots)if(!s.normal||!s.copy_seen||!s.life.appended||s.life.failure||!current_owner(s))return;
#endif
    published=backend().publish(ticket,world,slots[1].copy_sequence);
}
EvalScope::EvalScope(bool owned,uintptr_t caller,uintptr_t command,const void* structure):parent(active_eval),exceptions(std::uncaught_exceptions()){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);active_eval=-1;
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
    int delayed_index=-1;
    if(enabled&&!stopped){observed_structure=reinterpret_cast<uintptr_t>(structure);observed=eval_sample(observed_structure,owned);gate_observed=true;
        delayed_index=delayed.begin_eval(observed,recording.load()&&ngx_route_observer::attested&&caller==base+0x9a35d3,uint64_t(now()),GetCurrentThreadId());delayed_failure();}
#endif
    foreign_boundary(owned,caller,command,structure);
    // A completed Eval is eligible only up to the NEXT Eval boundary. Never
    // assign two later copies to two old evaluations sharing the same pointers.
    if(pending_eval>=0){ambiguous_boundary(pending_eval,owned,caller,command,structure);pending_eval=-1;}
    if(parent>=0)quarantine(nested_eval);
    if(!enabled||stopped||failure||!recording.load()||!ngx_route_observer::attested||caller!=base+0x9a35d3){++bypassed;return;}
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
    if(delayed_index<0){++bypassed;return;}
    if(unsigned(delayed_index)!=count){quarantine(association_ambiguous);return;}
#endif
    ++eval_sequence;
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
    auto before=observed;
#else
    auto before=eval_sample(reinterpret_cast<uintptr_t>(structure),owned);
#endif
    if(before.fields_mask!=0x7ffff||before.slots_mask!=3||!before.ownership.associated||
#if !defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) || !GEARS_NGX_OUTPUT_DELAYED_CAPTURE
       before.ownership.ticket<32||
#endif
       !before.resources[0]||!before.resources[1]||before.resources[0]==before.resources[1]){++bypassed;return;}
    if(count==slots.size()){++limited;return;}
    if(!count){world=before.ownership.world;ticket=before.ownership.ticket;owner_thread=GetCurrentThreadId();}
    if(world!=before.ownership.world||ticket!=before.ownership.ticket||owner_thread!=GetCurrentThreadId()){quarantine(association_ambiguous);return;}
    if(count&&(!slots[count-1].normal||!slots[count-1].copy_seen||!same_owner(slots[0].before.ownership,before.ownership))){quarantine(association_ambiguous);return;}
    index=int(count++);auto& s=slots[unsigned(index)];s.before=before;s.sequence=eval_sequence;s.caller=caller;s.command=command;
    s.structure=reinterpret_cast<uintptr_t>(structure);s.thread=GetCurrentThreadId();s.entry=now();s.active=true;active_eval=index;
#if defined(GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT) && GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT
    s.epoch=delayed.epoch;
#endif
}
EvalScope::~EvalScope(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(index>=0){auto& s=slots[unsigned(index)];s.after=eval_sample(s.structure,true);s.exit=now();s.result=raw_result;s.active=false;
        const bool normal=returned&&std::uncaught_exceptions()==exceptions&&active_eval==index&&!failure&&
            s.after.fields_mask==0x7ffff&&s.after.slots_mask==3&&same_owner(s.before.ownership,s.after.ownership)&&
            s.before.resources==s.after.resources&&s.before.values==s.after.values&&s.before.handle==s.after.handle&&s.before.parameters==s.after.parameters;
        if(!normal)quarantine(std::uncaught_exceptions()!=exceptions?eval_exception:eval_changed);
        else{s.normal=true;if(!s.copy_seen)pending_eval=index;else publish_pair();}}
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
    if(gate_observed){const auto after=eval_sample(observed_structure,true);delayed.end_eval(observed,after,returned&&std::uncaught_exceptions()==exceptions,uint64_t(now()));delayed_failure();}
#endif
    active_eval=parent;
}
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
CreateScope::CreateScope(bool owned,uintptr_t caller,uintptr_t command,const void* input):structure(reinterpret_cast<uintptr_t>(input)),exceptions(std::uncaught_exceptions()){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    // Revoke first, independently of the bounded observer row allocation.
    if(enabled&&!stopped){serial=delayed.begin_create(uint64_t(now()),ngx_route_observer::ownership(owned));delayed_failure();gears_ngx_route::Row row;row.structure=structure;before=ngx_route_observer::sample(row,owned);}
    create_boundary(owned,caller,command,input);
}
CreateScope::~CreateScope(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!serial)return;
    gears_ngx_route::Row row;row.structure=structure;const auto after=ngx_route_observer::sample(row,true);
    delayed.end_create(serial,returned&&std::uncaught_exceptions()==exceptions&&recording.load()&&ngx_route_observer::attested,before,after);delayed_failure();
}
#endif
void end_ticket(uint64_t ended,bool linked,PresentContext const& present_context){
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!enabled||stopped)return;
    const gears_ngx_delayed::PresentEvidence present{present_context.known,present_context.linked,present_context.contaminated,present_context.rejection,present_context.buffer};
    if(delayed.end_ticket(ended,linked,uint64_t(now()),present))publish_pair();delayed_failure();
#else
    (void)ended;(void)linked;(void)present_context;
#endif
}
void end_ticket(uint64_t ended,bool linked){end_ticket(ended,linked,{});}
void create_boundary(bool owned,uintptr_t caller,uintptr_t command,const void* structure){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    foreign_boundary(owned,caller,command,structure);
    if(enabled&&(active_eval>=0||pending_eval>=0))quarantine(association_ambiguous);pending_eval=-1;}
std::unique_lock<FrameHookMutex> execution_lock(){return std::unique_lock<FrameHookMutex>(frame_hook_mutex);}
int prepare_copy(ID3D12GraphicsCommandList* list,const D3D12_TEXTURE_COPY_LOCATION* destination,UINT x,UINT y,UINT z,
    const D3D12_TEXTURE_COPY_LOCATION* source,const D3D12_BOX* box,uintptr_t caller){
    if(!enabled||stopped||!recording.load())return -1;
    if(failure){if(failure==association_ambiguous&&count&&!gap.copy){D3D12_TEXTURE_COPY_LOCATION src{},dst{};
            if(safe_copy(&src,source,sizeof(src))&&safe_copy(&dst,destination,sizeof(dst))&&reinterpret_cast<uintptr_t>(src.pResource)==slots[0].before.resources[1]&&reinterpret_cast<uintptr_t>(dst.pResource)==slots[0].before.resources[0]){
                gap.copy=true;gap.copy_qpc=now();gap.copy_caller=caller;gap.copy_list=reinterpret_cast<uintptr_t>(list);gap.source=reinterpret_cast<uintptr_t>(src.pResource);gap.destination=reinterpret_cast<uintptr_t>(dst.pResource);gap.copy_thread=GetCurrentThreadId();}}
        return -1;}
    const int selected=active_eval>=0?active_eval:pending_eval;if(selected<0)return -1;auto& s=slots[unsigned(selected)];
    if(!current_owner(s)){quarantine(association_ambiguous);return -1;}
    D3D12_TEXTURE_COPY_LOCATION src{},dst{};
    if(!safe_copy(&src,source,sizeof(src))||!safe_copy(&dst,destination,sizeof(dst))){quarantine(typed_mismatch);return -1;}
    if(reinterpret_cast<uintptr_t>(src.pResource)!=s.before.resources[1]&&reinterpret_cast<uintptr_t>(dst.pResource)!=s.before.resources[0]){++unrelated;return -1;}
    if(caller!=0x1c6093||reinterpret_cast<uintptr_t>(src.pResource)!=s.before.resources[1]||reinterpret_cast<uintptr_t>(dst.pResource)!=s.before.resources[0]||
       src.Type!=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX||dst.Type!=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX||src.SubresourceIndex||dst.SubresourceIndex||x||y||z||box||s.copy_seen||
       (active_eval<0&&!s.normal)){quarantine(typed_mismatch);return -1;}
    s.raw_list=list;s.source=src;s.destination=dst;s.inside_eval=active_eval>=0;s.copy_qpc=now();s.copy_sequence=++copy_sequence;
    if(!backend().prepare(unsigned(selected),s)){quarantine(prepare_failed);backend().release(unsigned(selected));s.released=true;return -1;}
    s.prepared=true;return selected;
}
#if defined(GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT) && GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT
bool suppress_first_copy(int selected,ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z,const D3D12_BOX* box,uintptr_t caller){
    if(selected!=0)return false;auto& s=slots[0];
    // prepare() already certified same-device DIRECT list, supported DEFAULT
    // heaps/resources, full extent/format, held identities and readback lifetime.
    // This experiment admits only a completed normal first Eval, never an
    // in-Eval copy whose eventual normal return is still unknown.
    const auto current=eval_sample(s.structure,true);const auto id=backend().list_identity(list);const auto ref=backend().list_ref(id);
    const bool proved=!suppression.valid&&enabled&&!stopped&&!failure&&recording.load()&&current_owner(s)&&s.prepared&&!s.released&&s.normal&&!s.active&&
        active_eval<0&&pending_eval==0&&count==1&&s.sequence==1&&s.copy_sequence==1&&!s.copy_seen&&!s.life.appended&&!s.life.failure&&
        delayed.accepted==1&&delayed.armed&&delayed.epoch_valid&&delayed.selected&&!delayed.closed&&!delayed.finished&&!delayed.first_invalid.valid&&
        delayed.stable==32&&delayed.rows==1&&!delayed.active&&s.epoch==delayed.epoch&&s.before.ownership.world==delayed.world&&delayed.last_ticket==s.before.ownership.ticket&&
        s.before.ownership.world==delayed.selected_world&&s.before.ownership.ticket==delayed.selected_ticket&&
        s.after.handle==delayed.handle&&s.after.parameters==delayed.parameters&&
        delayed.create[2]==2560&&delayed.create[3]==1080&&
        current.slots_mask==3&&current.fields_mask==0x7ffff&&same_owner(s.after.ownership,current.ownership)&&current.handle==s.after.handle&&
        current.parameters==s.after.parameters&&current.resources==s.after.resources&&current.values==s.after.values&&
        caller==0x1c6093&&!x&&!y&&!z&&!box&&s.source.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX&&s.destination.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX&&
        !s.source.SubresourceIndex&&!s.destination.SubresourceIndex&&textures_supported(s.source_desc,s.destination_desc)&&
        s.source_desc.width==2560&&s.source_desc.height==1080&&s.source_desc.format==26&&
        s.destination_desc.width==2560&&s.destination_desc.height==1080&&s.destination_desc.format==26&&
        id&&id==s.life.held_identity&&id==s.life.list.identity&&ref.identity==id&&ref.generation&&ref.generation==s.life.list.generation;
    if(!proved){quarantine(preserve_pair_unproved);return false;}
    suppression.valid=suppression.suppressed=true;s.original_suppressed=true;
    suppression.epoch=s.epoch;suppression.world=s.before.ownership.world;suppression.ticket=s.before.ownership.ticket;
    suppression.eval_sequence=s.sequence;suppression.copy_sequence=s.copy_sequence;suppression.generation=s.life.list.generation;suppression.qpc=now();
    suppression.ordinal=1;suppression.thread=GetCurrentThreadId();suppression.caller=caller;
    suppression.source_type=unsigned(s.source.Type);suppression.destination_type=unsigned(s.destination.Type);
    suppression.source_subresource=s.source.SubresourceIndex;suppression.destination_subresource=s.destination.SubresourceIndex;
    suppression.x=x;suppression.y=y;suppression.z=z;suppression.box_null=box==nullptr;
    suppression.source_raw=reinterpret_cast<uintptr_t>(s.source.pResource);suppression.destination_raw=reinterpret_cast<uintptr_t>(s.destination.pResource);
    suppression.source_canonical=s.source_desc.canonical;suppression.destination_canonical=s.destination_desc.canonical;
    suppression.list_raw=reinterpret_cast<uintptr_t>(list);suppression.list_canonical=id;
    suppression.width=s.source_desc.width;suppression.height=s.source_desc.height;suppression.format=s.source_desc.format;return true;
}
#endif
void copy_passthrough(ID3D12GraphicsCommandList* list,const D3D12_TEXTURE_COPY_LOCATION* destination,UINT x,UINT y,UINT z,
    const D3D12_TEXTURE_COPY_LOCATION* source,const D3D12_BOX* box,uintptr_t caller,CopyFn forward){
    auto lock=execution_lock();const int selected=prepare_copy(list,destination,x,y,z,source,box,caller);
    bool omitted=false;
#if defined(GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT) && GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT
    omitted=suppress_first_copy(selected,list,x,y,z,box,caller);
    if(selected>=0&&!omitted)slots[unsigned(selected)].original_forwarded=true;
#endif
    try{if(!omitted)ngx_stereo_host::copy_passthrough(list,destination,x,y,z,source,box,caller,forward);}catch(...){if(selected>=0){quarantine(copy_exception);backend().release(unsigned(selected));slots[unsigned(selected)].released=true;}throw;}
#if defined(GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT) && GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT
    if(selected>=0&&!omitted)slots[unsigned(selected)].original_returned=true;
#endif
    if(selected<0)return;auto& s=slots[unsigned(selected)];
    D3D12_TEXTURE_COPY_LOCATION src{},dst{};
    if(!current_owner(s)||!safe_copy(&src,source,sizeof(src))||!safe_copy(&dst,destination,sizeof(dst))||
       src.pResource!=s.source.pResource||dst.pResource!=s.destination.pResource||src.Type!=s.source.Type||dst.Type!=s.destination.Type||src.SubresourceIndex||dst.SubresourceIndex){
        quarantine(typed_mismatch);backend().release(unsigned(selected));s.released=true;return;}
    // Original CopySource contract still holds at this immediately adjacent
    // location. Mark appended BEFORE the extra call: an exception quarantines
    // the storage even if a driver recorded a command before unwinding.
    s.life.appended=true;s.copy_seen=true;s.prepared=false;
#if defined(GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT) && GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT
    if(omitted)suppression.readback_started=true;
#endif
    try{backend().append(unsigned(selected),s,forward);}catch(...){quarantine(append_exception);throw;}
#if defined(GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT) && GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT
    s.readback_returned=true;if(omitted)suppression.readback_returned=true;
#endif
    if(pending_eval==selected)pending_eval=-1;publish_pair();
}
unsigned prepare_execute(ID3D12CommandQueue* queue,UINT n,ID3D12CommandList* const* members){
    std::array<ID3D12CommandList*,gears_frame_link::Tracker::list_capacity> copied{};const bool readable=n<=copied.size()&&(!n||safe_copy(copied.data(),members,size_t(n)*sizeof(copied[0])));
    const auto order=++execute_sequence;unsigned token{};
    for(unsigned i=0;i<count;++i){auto& s=slots[i];if(!s.life.appended||s.life.failure||s.life.saved)continue;
        bool valid=readable,exact=true;unsigned occurrences{},member{};
        if(valid)for(unsigned k=0;k<n;++k){const auto id=backend().list_identity(copied[k]);if(!id){valid=false;break;}
            if(id==s.life.held_identity){++occurrences;member=k;const auto ref=backend().list_ref(id);exact=exact&&ref.identity==s.life.list.identity&&ref.generation==s.life.list.generation;}}
        uintptr_t identity{};if(occurrences&&!backend().queue_valid(i,queue,identity)){s.life.fail(gears_death_hdr_pixels::cross_queue);continue;}
        if(s.life.before(valid,occurrences,identity,exact)){token|=1u<<i;s.execute_member=member;s.execute_order=order;}
    }return token;
}
void after_execute(unsigned token,ID3D12CommandQueue* queue){for(unsigned i=0;i<count;++i)if(token&(1u<<i))slots[i].life.after(backend().signal(i,queue));}
void execute_passthrough(ID3D12CommandQueue* queue,UINT n,ID3D12CommandList* const* members,ExecuteFn forward){
    auto lock=execution_lock();const auto token=prepare_execute(queue,n,members);
    try{ngx_stereo_host::execute_passthrough(queue,n,members,forward);}catch(...){for(auto& s:slots)if(s.life.appended)s.life.fail(gears_death_hdr_pixels::unknown_membership);throw;}
    after_execute(token,queue);
}
void reset_after_original(ID3D12GraphicsCommandList* list,HRESULT hr){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    const auto id=backend().list_identity(list);
    for(unsigned i=0;i<count;++i){auto& s=slots[i];if(!s.life.appended||s.life.failure||s.life.saved||s.life.retired)continue;
        if(id&&id!=s.life.held_identity&&list!=s.raw_list)continue;const auto ref=backend().list_ref(s.life.held_identity);
        s.reset_hr=hr;s.reset_qpc=now();s.life.reset(SUCCEEDED(hr),id,ref.generation,reinterpret_cast<uintptr_t>(list),ref.identity);}
}
HRESULT reset_passthrough(ID3D12GraphicsCommandList* list,ID3D12CommandAllocator* allocator,ID3D12PipelineState* state,ResetFn forward){
    auto lock=execution_lock();try{const auto hr=forward(list,allocator,state);reset_after_original(list,hr);return hr;}
    catch(...){for(auto& s:slots)if(s.life.appended&&!s.life.saved)s.life.fail(gears_death_hdr_pixels::reset_failed);throw;}
}
bool enable(FILE* file){if(initialized){fprintf(file,"ngx_output_pixels_identity enabled=1 rejected_reinitialization=1\n");return false;}
    enabled=false;stopped=false;published=false;slots={};count=failure=0;world=ticket=eval_sequence=copy_sequence=execute_sequence=0;
#if defined(GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT) && GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT
    suppression={};
#endif
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
    delayed={};
#endif
    const bool ready=!self_testing&&continuous_session&&frame_link_diagnostics&&color_copy_diagnostics&&ngx_route_observer::attested;
    if(ready)initialized=true;enabled=ready&&backend().start();
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
    fprintf(file,"ngx_output_pixels_identity enabled=1 configuration=%u active=%u first_ticket_minimum=0 required_stable_pairs=32 resume_arm_required=1 slots=2 original_copy_source_contract=1 barriers_added=0\n",unsigned(ready),unsigned(enabled));
#else
    fprintf(file,"ngx_output_pixels_identity enabled=1 configuration=%u active=%u first_ticket_minimum=32 slots=2 original_copy_source_contract=1 barriers_added=0\n",unsigned(ready),unsigned(enabled));
#endif
    return enabled;
}
void worker_request(const wchar_t* path,uint64_t pid,uint64_t creation_stamp,uint64_t started,uint64_t frequency){
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
    // Worker only: no render path reads files. The private driver atomically
    // replaces this exact ordered ASCII request after observing normal Resume.
    std::array<char,513> text{};DWORD bytes{};LARGE_INTEGER size{};
    HANDLE input=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(input==INVALID_HANDLE_VALUE)return;
    const bool readable=GetFileSizeEx(input,&size)&&size.QuadPart>0&&size.QuadPart<=512&&ReadFile(input,text.data(),DWORD(size.QuadPart),&bytes,nullptr)&&bytes==size.QuadPart;
    CloseHandle(input);
    static std::array<char,513> previous{};static DWORD previous_bytes{};static long long previous_size=-1;
    if(previous_size==size.QuadPart&&previous_bytes==bytes&&previous==text)return;
    previous=text;previous_bytes=bytes;previous_size=size.QuadPart;
    gears_ngx_delayed::Request request;const bool parsed=readable&&gears_ngx_delayed::parse(std::string_view(text.data(),bytes),request);
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!enabled||stopped)return;
    if(!parsed){++delayed.rejected;delayed.rejection=gears_ngx_delayed::Reason::bad_request;return;}
    if(frequency>UINT64_MAX/30){++delayed.rejected;delayed.rejection=gears_ngx_delayed::Reason::bad_request;return;}
    gears_ngx_delayed::Request identity;identity.pid=pid;identity.creation=creation_stamp;identity.started=started;
    delayed.arm(request,identity,uint64_t(now()),frequency*30);
#else
    (void)path;(void)pid;(void)creation_stamp;(void)started;(void)frequency;
#endif
}
void poll(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!enabled||stopped||failure)return;
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
    delayed.tick(uint64_t(now()));delayed_failure();if(failure||!delayed.closed)return;
#endif
    for(unsigned i=0;i<count;++i){auto& s=slots[i];if(!s.normal||!s.life.appended||s.life.failure||s.life.saved)continue;
        if(backend().poll(i,s)&&s.life.saved){backend().release(i);s.released=true;}}
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
    if(published&&count==2&&slots[0].life.saved&&slots[1].life.saved){delayed.finished=true;delayed.reason=gears_ngx_delayed::Reason::complete;}
#endif
}
void stop(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);stopped=true;for(auto& s:slots)s.life.stop();backend().close();}
void print_control(FILE* file){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
#if defined(GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT) && GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT
    fprintf(file,"ngx_output_preserve_pair_input enabled=1 diagnostic_only=1 omission_count=%u preserve_suppressed=%u preserve_original_forwarded=%u preserve_readback_started=%u preserve_readback_returned=%u preserve_epoch=%llu preserve_world=%llu preserve_ticket=%llu preserve_ordinal=%u preserve_eval_sequence=%llu preserve_copy_sequence=%llu preserve_qpc=%lld preserve_thread=%u preserve_caller=%llx preserve_source_raw=%llx preserve_destination_raw=%llx preserve_source_canonical=%llx preserve_destination_canonical=%llx preserve_list_raw=%llx preserve_list_canonical=%llx preserve_generation=%llu preserve_source_type=%u preserve_destination_type=%u preserve_source_subresource=%u preserve_destination_subresource=%u preserve_x=%u preserve_y=%u preserve_z=%u preserve_box_null=%u preserve_width=%llu preserve_height=%u preserve_format=%u causal_pixels_verified=0 sr_verified=0 temporal_history_verified=0\n",
        unsigned(suppression.valid),unsigned(suppression.suppressed),unsigned(suppression.original_forwarded),unsigned(suppression.readback_started),unsigned(suppression.readback_returned),
        suppression.epoch,suppression.world,suppression.ticket,suppression.ordinal,suppression.eval_sequence,suppression.copy_sequence,suppression.qpc,suppression.thread,suppression.caller,
        suppression.source_raw,suppression.destination_raw,suppression.source_canonical,suppression.destination_canonical,suppression.list_raw,suppression.list_canonical,suppression.generation,
        suppression.source_type,suppression.destination_type,suppression.source_subresource,suppression.destination_subresource,suppression.x,suppression.y,suppression.z,unsigned(suppression.box_null),suppression.width,suppression.height,suppression.format);
#endif
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
    fprintf(file,"ngx_output_delayed enabled=1 epoch=%llu epoch_valid=%u epoch_world=%llu handle=%llx parameters=%llx create=%u,%u,%u,%u,%u,%u,%u accepted=%llu rejected=%llu rejection_reason=%u nonce=%llu armed=%u stable_pairs=%u required_pairs=32 resets=%llu deadline_qpc=%llu selected=%u selected_world=%llu selected_ticket=%llu pair_closed=%u gpu_saved=%u terminal_reason=%u complete=1\n",
        delayed.epoch,unsigned(delayed.epoch_valid),delayed.world,delayed.handle,delayed.parameters,delayed.create[0],delayed.create[1],delayed.create[2],delayed.create[3],delayed.create[4],delayed.create[5],delayed.create[6],
        delayed.accepted,delayed.rejected,unsigned(delayed.rejection),delayed.last_nonce,unsigned(delayed.armed),delayed.stable,delayed.resets,delayed.deadline,unsigned(delayed.selected),delayed.selected_world,delayed.selected_ticket,unsigned(delayed.closed),unsigned(delayed.finished),unsigned(delayed.reason));
    if(delayed.first_invalid.valid){const auto& invalid=delayed.first_invalid;const auto& trigger=invalid.trigger;const auto& incoming=trigger.incoming;const auto& present=trigger.present;
        // One immutable receipt, rewritten only with the existing bounded status.
        // Prefix every key: the session reader also parses the native first line.
        fprintf(file,"ngx_output_delayed_invalid invalid_first=1 invalid_site=%s invalid_site_code=%u invalid_original_reason=%u invalid_qpc=%llu invalid_active=%u invalid_rows=%u invalid_stable=%u invalid_pair_thread=%u invalid_epoch=%llu invalid_world=%llu invalid_last_ticket=%llu invalid_completed_ticket=%llu invalid_selected_ticket=%llu invalid_selected_world=%llu invalid_incoming_known=%u invalid_incoming_associated=%u invalid_incoming_mask=%u invalid_incoming_eye=%u invalid_incoming_world=%llu invalid_incoming_ticket=%llu invalid_incoming_renderer=%llx invalid_incoming_context=%llx invalid_incoming_view=%llx invalid_incoming_views=%llx invalid_incoming_count=%d invalid_at_present=%u invalid_present_eligible=%u invalid_present_ticket=%llu invalid_present_details_known=%u invalid_present_linked=%u invalid_present_contaminated=%u invalid_present_rejection=%u invalid_present_buffer=%llx\n",
            gears_ngx_delayed::site_name(trigger.site),unsigned(trigger.site),unsigned(invalid.original_reason),trigger.qpc,invalid.active,invalid.rows,invalid.stable,invalid.pair_thread,
            invalid.epoch,invalid.world,invalid.last_ticket,invalid.completed_ticket,invalid.selected_ticket,invalid.selected_world,unsigned(trigger.incoming_known),unsigned(incoming.associated),incoming.mask,incoming.eye,
            incoming.world,incoming.ticket,incoming.renderer,incoming.context,incoming.view,incoming.views,incoming.count,unsigned(trigger.at_present),unsigned(trigger.eligible),trigger.present_ticket,
            unsigned(present.known),unsigned(present.linked),unsigned(present.contaminated),present.rejection,present.buffer);}
#else
    (void)file;
#endif
}
void print(FILE* file){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);print_control(file);
    LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);
    fprintf(file,"ngx_output_pixel_clock qpc_frequency=%lld gap_boundary=%u pending_row=%d boundary_qpc=%lld boundary_caller=%llx boundary_command=%llx boundary_structure=%llx boundary_thread=%u boundary_world=%llu boundary_ticket=%llu later_matching_copy=%u copy_qpc=%lld copy_caller=%llx copy_list=%llx copy_source=%llx copy_destination=%llx copy_thread=%u association_not_recovered=1\n",
        frequency.QuadPart,unsigned(gap.boundary),gap.pending,gap.boundary_qpc,static_cast<unsigned long long>(gap.caller),static_cast<unsigned long long>(gap.command),static_cast<unsigned long long>(gap.structure),gap.thread,gap.world,gap.ticket,
        unsigned(gap.copy),gap.copy_qpc,static_cast<unsigned long long>(gap.copy_caller),static_cast<unsigned long long>(gap.copy_list),static_cast<unsigned long long>(gap.source),static_cast<unsigned long long>(gap.destination),gap.copy_thread);
    fprintf(file,"ngx_output_pixels enabled=1 active=%u world=%llu ticket=%llu slots=%u limit=2 failure=%u match_request_published=%u bypassed=%llu limited=%llu unrelated=%llu barriers_added=0 same_original_list=1 matching_xr_snapshot_verified=0 logical_eye_verified=0 quad_pixels_verified=0 temporal_history_verified=0 sr_verified=0\n",unsigned(enabled),world,ticket,count,failure,unsigned(published),bypassed,limited,unrelated);
    for(unsigned i=0;i<count;++i){auto const& s=slots[i];auto const& v=s.before.values;
#if defined(GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT) && GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT
        fprintf(file,"ngx_output_copy_forward row=%u original_forwarded=%u original_returned=%u original_suppressed=%u readback_returned=%u\n",i,unsigned(s.original_forwarded),unsigned(s.original_returned),unsigned(s.original_suppressed),unsigned(s.readback_returned));
#endif
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
        gears_ngx_route::Row diagnostic;diagnostic.evaluate=true;ngx_route_observer::print_sample(file,diagnostic,i,"selected_before",s.before);ngx_route_observer::print_sample(file,diagnostic,i,"selected_after",s.after);
#endif
        fprintf(file,"ngx_output_eval row=%u eval_sequence=%llu entry_qpc=%lld exit_qpc=%lld copy_qpc=%lld thread=%u caller=%llx command=%llx structure=%llx before_slots_mask=%u after_slots_mask=%u before_fields_mask=%x after_fields_mask=%x before_handle=%llx after_handle=%llx before_parameters=%llx after_parameters=%llx\n",
            i,s.sequence,s.entry,s.exit,s.copy_qpc,s.thread,static_cast<unsigned long long>(s.caller),static_cast<unsigned long long>(s.command),static_cast<unsigned long long>(s.structure),s.before.slots_mask,s.after.slots_mask,s.before.fields_mask,s.after.fields_mask,
            static_cast<unsigned long long>(s.before.handle),static_cast<unsigned long long>(s.after.handle),static_cast<unsigned long long>(s.before.parameters),static_cast<unsigned long long>(s.after.parameters));
        fprintf(file,"ngx_output_pixel row=%u eval_sequence=%llu copy_sequence=%llu normal=%u inside_eval=%u source_base=%u,%u output_base=%u,%u render_dimensions=%u,%u reset_raw=%u result_rax=%llx source_raw=%llx color_raw=%llx source_canonical=%llx color_canonical=%llx dimensions=%llu,%u format=%u list_raw=%llx list_identity=%llx generation=%llu execute_order=%lld member=%u queue=%llx appended=%u executed=%u signalled=%u retired=%u gpu_complete=%u saved=%u failure=%u held_until_process=%u hr=%08lx path=%ls copy_point_only=1\n",
            i,s.sequence,s.copy_sequence,unsigned(s.normal),unsigned(s.inside_eval),v[7],v[8],v[13],v[14],v[0],v[1],v[2],s.result,
            static_cast<unsigned long long>(s.before.resources[1]),static_cast<unsigned long long>(s.before.resources[0]),static_cast<unsigned long long>(s.source_desc.canonical),static_cast<unsigned long long>(s.destination_desc.canonical),s.source_desc.width,s.source_desc.height,s.source_desc.format,
            static_cast<unsigned long long>(s.life.list.raw),static_cast<unsigned long long>(s.life.list.identity),s.life.list.generation,s.execute_order,s.execute_member,static_cast<unsigned long long>(s.life.queue),
            unsigned(s.life.appended),unsigned(s.life.executed),unsigned(s.life.signalled),unsigned(s.life.retired),unsigned(s.life.gpu_complete),unsigned(s.life.saved),s.life.failure,unsigned(s.life.appended&&!s.released),static_cast<unsigned long>(s.hr),s.path);
        auto const& r=s.life.reset_receipt;
        fprintf(file,"ngx_output_pixel_reset row=%u observed=%u success=%u raw=%llx canonical=%llx held_canonical=%llx tracker_identity=%llx tracker_generation=%llu hr=%08lx qpc=%lld physical_retirement_required=1\n",i,unsigned(r.observed),unsigned(r.success),static_cast<unsigned long long>(r.raw),static_cast<unsigned long long>(r.identity),static_cast<unsigned long long>(s.life.held_identity),static_cast<unsigned long long>(r.tracker_identity),r.tracker_generation,static_cast<unsigned long>(s.reset_hr),s.reset_qpc);}
}
#endif
}
