// Targeted CPU command-list observations only. Typed arguments are retained;
// NGX integers are never converted to COM. No added commands or GPU proof.
#include "ngx_mv_constants_capture.inl"
namespace ngx_mv_compute_capture {
#if GEARS_NGX_MV_TARGET_CAPTURE
using gears_ngx_mv_compute::Recording;
using gears_ngx_mv_compute::DescriptorLookup;
using HeapLease=gears_ngx_mv::HeapLease;
using HeapsFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,ID3D12DescriptorHeap* const*);
using RootFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12RootSignature*);
using TableFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,D3D12_GPU_DESCRIPTOR_HANDLE);
using DispatchFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT);
using ClearFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12PipelineState*);
using BundleFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12GraphicsCommandList*);
using SetFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12PipelineState*);
HeapsFn original_heaps{};RootFn original_root{};TableFn original_table{};DispatchFn original_dispatch{};
ClearFn original_clear{};BundleFn original_bundle{};
SetFn original_set{};
struct List {
    uintptr_t raw{},canonical{};uint64_t generation{};
    uint64_t reset_prior_generation{};bool reset_prior_known{};
    unsigned callback_busy{},reset_busy{};bool revoked{},history_pinned{};
    gears_vr::ComPtr<ID3D12GraphicsCommandList> held;
    gears_vr::ComPtr<IUnknown> identity;
    HeapLease heap;
    gears_vr::ComPtr<ID3D12RootSignature> root;
    gears_vr::ComPtr<IUnknown> root_identity;
};
// Declared before the callback's frame lock. Detached owners and validation
// temporaries therefore release only after that lock has been dropped.
struct Retirement {
    List retired;
    gears_vr::ComPtr<IUnknown> identity,device_identity;
    gears_vr::ComPtr<ID3D12Device> device;
};
struct ListUse {
    List* list{};
    explicit ListUse(List* value=nullptr):list(value){if(list)++list->callback_busy;}
    void attach(List* value){list=value;if(list)++list->callback_busy;}
    ~ListUse(){if(list)--list->callback_busy;}
};
std::array<List,16> lists;unsigned list_count{};
gears_ngx_mv_compute::Bindings bindings;
struct Row {
    uint64_t world{},ticket{},generation{};uintptr_t renderer{},raw{},canonical{},root{},heap{},caller{};
    uint64_t root3{};std::array<UINT,3> groups{};DescriptorLookup before{},after{};
    bool returned{},stable{},caller_candidate{};
};
std::array<Row,32> rows;unsigned row_count{};
bool stopped{},failed{},leases_released{};unsigned failure{};
uint64_t dispatches{},unbound{},unrelated{},invalidations{},waiting_bindings{},capture_bindings{};
uint64_t mutation{};
uint64_t foreign_device_skips{},reuse_attempts{},reused{};
struct Occupancy {unsigned occupied{},eligible{},partial{},pinned{},busy{},unrevoked{},unknown_blocked{},stale_blocked{};} capacity_occupancy;
using ExactTarget=ngx_mv_descriptor_capture::ExactTarget;
using PsoWitnessKey=gears_ngx_mv_compute::PsoWitnessKey;
struct PsoWitness:gears_ngx_mv_compute::PsoWitnessValidity {
    PsoWitnessKey key;
    gears_vr::ComPtr<ID3D12GraphicsCommandList> list;
    gears_vr::ComPtr<IUnknown> list_identity;
    gears_vr::ComPtr<ID3D12PipelineState> pso;
    gears_vr::ComPtr<IUnknown> pso_identity;
};
PsoWitness pso_pending,pso_promoted;
struct RootWitness:gears_ngx_mv_compute::PsoWitnessValidity {gears_ngx_mv_compute::RootWitnessKey key;};
RootWitness root_pending,root_promoted;
uint64_t root_serial{},root_sets{},root_candidates{},root_promotions{},root_unknown{},root_rejected{};
thread_local unsigned root_set_depth{};
void root_invalidate(){++root_serial;root_pending.revoke();root_promoted.revoke();}
uint64_t pso_serial{},pso_sets{},pso_candidates{},pso_discards{},pso_promotions{},pso_qi_failures{};unsigned pso_last_discard{};
thread_local unsigned pso_set_depth{};
void pso_invalidate(unsigned reason){
    ++pso_serial;if(pso_pending.valid)++pso_discards;pso_pending.revoke();pso_promoted.revoke();pso_last_discard=reason;
    // Owners and historical stable Dispatch evidence survive revocation.
}
struct ExactAssociation {bool active{};uintptr_t canonical{},renderer{};uint64_t generation{},ticket{},world{},serial{};ExactTarget descriptor;};
ExactAssociation exact_association;
gears_vr::ComPtr<ID3D12GraphicsCommandList> exact_list_owner;
gears_vr::ComPtr<IUnknown> exact_identity_owner;
bool exact_stopped{},exact_arming{};unsigned exact_reason{};uint64_t exact_serial{},exact_tables{},exact_misses{},exact_dispatches{},exact_overflow{};
bool exact_pending(){return !exact_stopped&&ngx_mv_target_capture::pending();}
void exact_stop(unsigned reason){if(!exact_stopped){exact_stopped=true;exact_reason=reason;++exact_serial;pso_invalidate(3);root_invalidate();ngx_mv_constants_capture::invalidate();}exact_association.active=false;exact_arming=false;}
void exact_mutation(unsigned reason){if(exact_pending()){++exact_serial;if(exact_association.active||exact_arming)exact_stop(reason);}}
struct ExactDispatch {ExactAssociation association;ExactTarget after;uintptr_t caller{},pso{};uint64_t pso_serial{};std::array<UINT,3> groups{};bool returned{},stable{},pso_stable{},root_stable{},constants_stable{};gears_ngx_mv_compute::RootWitnessKey root;ngx_mv_constants_capture::Binding constants;};
std::array<ExactDispatch,8> exact_rows;unsigned exact_count{};
enum class CallerOperation:unsigned {heaps,root,table,dispatch};
std::array<uint64_t,4> unknown_skipped{};
struct CallerRow {
    CallerOperation operation{};unsigned phase{};uintptr_t caller{},rva{};bool in_game_image{},historical_candidate{};
    uint64_t calls{},bank_failed_calls{},first_ticket{},last_ticket{};
};
std::array<CallerRow,32> caller_rows;unsigned caller_row_count{};
uint64_t caller_calls{},caller_overflow_calls{};unsigned caller_image_size{};bool caller_image_checked{};
thread_local unsigned depth{};
struct EvidenceContext {const char* operation="none";uintptr_t raw{};bool owned_known{},owned{};};
thread_local EvidenceContext evidence_context;
struct EvidenceScope {
    EvidenceContext previous;
    EvidenceScope(char const* operation,ID3D12GraphicsCommandList* list):previous(evidence_context){
        evidence_context={operation,reinterpret_cast<uintptr_t>(list),false,false};
    }
    ~EvidenceScope(){evidence_context=previous;}
};
struct FirstFailure {
    bool present{},owned_known{},owned{},bypass{},prior_stored_known{};
    char const* site="none";char const* operation="none";
    uintptr_t raw{},canonical{},current_identity{},renderer{};
    uint64_t stored_generation{},current_generation{},prior_stored_generation{},ticket{},world{},frozen_ticket{},frozen_world{},mutation_at_failure{};
    unsigned reason{},core_loss{},window_phase{},depth_at_failure{},lists_at_failure{},rows_at_failure{};
    HRESULT result=E_PENDING;
    Occupancy occupancy{};
} first_failure;
enum Failure:unsigned {invalid_list=1,list_capacity,invalid_heap,invalid_root,nested,descriptor_capacity,row_capacity,changed,unwinding,invalid_generation};
void lose(unsigned reason,char const* site="unspecified",List const* held=nullptr,uintptr_t raw=0,uintptr_t canonical=0,
          HRESULT result=E_PENDING,uint64_t prior_stored=0,bool prior_known=false){
    if(!failed){
        // Scalar evidence only. current() reads the tracker, never the native
        // command-list object. This does not alter rejection or retry policy.
        auto& f=first_failure;f.present=true;f.reason=reason;f.site=site;f.operation=evidence_context.operation;
        f.raw=raw?raw:held?held->raw:evidence_context.raw;f.canonical=canonical?canonical:held?held->canonical:0;
        f.stored_generation=held?held->generation:0;
        if(f.canonical){const auto live=frame_link.current(f.canonical);f.current_identity=live.identity;f.current_generation=live.generation;}
        f.prior_stored_generation=prior_known?prior_stored:held?held->reset_prior_generation:0;
        f.prior_stored_known=prior_known||(held&&held->reset_prior_known);f.result=result;
        f.owned_known=evidence_context.owned_known;f.owned=evidence_context.owned;f.bypass=stereo_publish_bypass!=0;
        f.ticket=consuming_frame_ticket;f.world=committed_world_generation.load();
        f.frozen_ticket=consuming_frozen_frame.id;f.frozen_world=consuming_frozen_frame.world_generation;
        f.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);f.window_phase=unsigned(ngx_mv_target_capture::window.snapshot().phase);
        f.core_loss=unsigned(bindings.loss());f.depth_at_failure=depth;f.lists_at_failure=list_count;f.rows_at_failure=row_count;f.mutation_at_failure=mutation;
        f.occupancy=capacity_occupancy;
        failed=true;failure=reason;bindings.uncertain();
    }stopped=true;
}
bool pending(){return !stopped&&!failed&&ngx_mv_target_capture::pending();}
bool target_observing(bool owned){
    ngx_mv_target_capture::poll();
    if(!ngx_mv_target_capture::pending()||!owned||stereo_publish_bypass||!recording.load())return false;
    auto const& s=ngx_mv_target_capture::window.snapshot();
    if(!consuming_frozen_frame.enabled||!consuming_renderer||consuming_frozen_frame.world_generation!=s.selection.world||
       committed_world_generation.load()!=s.selection.world||consuming_frame_ticket!=consuming_frozen_frame.id)return false;
    if(s.phase==gears_ngx_mv_target::Phase::capturing)
        return consuming_frame_ticket==s.consume.ticket&&reinterpret_cast<uintptr_t>(consuming_renderer)==s.consume.renderer;
    return consuming_frame_ticket>=s.selection.ticket;
}
bool observing(bool owned){
    evidence_context.owned_known=true;evidence_context.owned=owned;
    return target_observing(owned)&&!stopped&&!failed;
}
void census(CallerOperation operation,uintptr_t caller,bool owned){
    // Scalar call-site evidence remains useful after the binding bank fails.
    // It never admits a list, retains COM, or extends the selected Window.
    if(!target_observing(owned))return;
    if(!caller_image_checked){
        caller_image_checked=true;IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
        if(base&&safe_copy(&dos,reinterpret_cast<void*>(base),sizeof(dos))&&dos.e_magic==IMAGE_DOS_SIGNATURE&&
           dos.e_lfanew>0&&dos.e_lfanew<0x100000&&safe_copy(&nt,reinterpret_cast<void*>(base+dos.e_lfanew),sizeof(nt))&&
           nt.Signature==IMAGE_NT_SIGNATURE&&nt.OptionalHeader.Magic==IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            caller_image_size=nt.OptionalHeader.SizeOfImage;
    }
    ++caller_calls;const auto phase=unsigned(ngx_mv_target_capture::window.snapshot().phase);
    const bool in_game=caller_image_size&&caller>=base&&caller-base<caller_image_size;
    const auto rva=in_game?caller-base:0;
    CallerRow* row{};
    for(unsigned i=0;i<caller_row_count;++i){auto& candidate=caller_rows[i];
        if(candidate.operation==operation&&candidate.phase==phase&&candidate.caller==caller){row=&candidate;break;}}
    if(!row){
        if(caller_row_count==caller_rows.size()){++caller_overflow_calls;return;}
        row=&caller_rows[caller_row_count++];row->operation=operation;row->phase=phase;row->caller=caller;row->rva=rva;row->in_game_image=in_game;
        // Current call location alone does not establish historical identity.
        // Candidate labels also require the existing loaded-host attestation.
        row->historical_candidate=in_game&&ngx_stereo_host::attested&&(
            (operation==CallerOperation::table&&rva==0x17b99a)||(operation==CallerOperation::dispatch&&rva==0x1ac833));
        row->first_ticket=consuming_frame_ticket;
    }
    ++row->calls;if(failed)++row->bank_failed_calls;row->last_ticket=consuming_frame_ticket;
}
struct Depth {
    bool outer,relevant;int exceptions=std::uncaught_exceptions();
    explicit Depth(bool observed):outer(depth++==0),relevant(observed){if(!outer&&relevant&&pending())lose(nested,"depth_enter");}
    ~Depth(){--depth;if(relevant&&std::uncaught_exceptions()!=exceptions&&pending())lose(unwinding,"depth_unwind");}
};
List* known(ID3D12GraphicsCommandList* list){
    if(!list)return nullptr;const auto raw=reinterpret_cast<uintptr_t>(list);
    for(unsigned i=0;i<list_count;++i)if(lists[i].raw==raw)return &lists[i];return nullptr;
}
uint64_t resolved_raw{},resolved_alias{},resolved_unmatched{},resolve_qi_failed{},resolve_reentrant{};
thread_local bool resolving{};
struct Resolution {
    List* held{};uintptr_t raw{},canonical{};unsigned kind{};
    gears_vr::ComPtr<IUnknown> identity;
    gears_vr::ComPtr<ID3D12Device> device;
    gears_vr::ComPtr<IUnknown> device_identity;
    explicit Resolution(ID3D12GraphicsCommandList* list):raw(reinterpret_cast<uintptr_t>(list)){
        {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
            if((!pending()&&!exact_pending())||!list)return;
            if(resolving){++resolve_reentrant;exact_stop(1);if(pending())lose(nested,"resolve_reentrant",nullptr,raw);return;}
            // A callback nested under our native forward still owns its outer
            // frame lock. Reject both raw and alias paths before any fast return.
            if(depth){++resolve_reentrant;exact_stop(1);if(pending())lose(nested,"resolve_nested",nullptr,raw);return;}
            if((held=known(list))){++held->callback_busy;canonical=held->canonical;kind=1;++resolved_raw;return;}}
        // Typed callback input only; never reconstruct COM from a scalar key.
        // QI and its eventual temporary Release both run outside our lock.
        resolving=true;HRESULT hr=E_FAIL;
        try{hr=list->QueryInterface(IID_PPV_ARGS(&identity));}catch(...){resolving=false;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++resolve_qi_failed;exact_stop(1);if(pending())lose(unwinding,"resolve_qi_unwind",nullptr,raw);throw;}
        resolving=false;canonical=reinterpret_cast<uintptr_t>(identity.Get());
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!pending()&&!exact_pending())return;
        if(FAILED(hr)||!canonical){++resolve_qi_failed;kind=4;exact_stop(1);if(pending())lose(invalid_list,"resolve_qi_failed",nullptr,raw,canonical,hr);return;}
        for(unsigned i=0;i<list_count;++i)if(lists[i].canonical==canonical){held=&lists[i];++held->callback_busy;kind=2;++resolved_alias;return;}
        kind=3;++resolved_unmatched;
    }
    bool selected_device(ID3D12GraphicsCommandList* list){
        {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!exact_pending()||!canonical)return false;
            if(resolving||depth){exact_stop(1);return false;}}
        resolving=true;HRESULT hr=E_FAIL;
        try{hr=list->GetDevice(IID_PPV_ARGS(&device));if(SUCCEEDED(hr))hr=device?device.As(&device_identity):E_FAIL;}
        catch(...){resolving=false;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);exact_stop(1);throw;}
        resolving=false;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        if(FAILED(hr)||!device_identity){exact_stop(1);return false;}
        return exact_pending()&&reinterpret_cast<uintptr_t>(device_identity.Get())==ngx_mv_target_capture::selected_mv.device;
    }
    ~Resolution(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(held)--held->callback_busy;}
    // identity releases after the destructor body's short lock has gone away.
};
bool exact_identity_current(ExactAssociation const& a){
    const auto live=frame_link.current(a.canonical);auto const& w=ngx_mv_target_capture::window.snapshot();
    return a.canonical&&a.generation&&live.identity==a.canonical&&live.generation==a.generation&&
        w.phase==gears_ngx_mv_target::Phase::capturing&&w.selection.world==a.world&&w.consume.ticket==a.ticket&&w.consume.renderer==a.renderer;
}
bool witness_matches(PsoWitness const& witness,ExactAssociation const& a){
    return gears_ngx_mv_compute::pso_witness_matches(witness.key,witness.valid,a.canonical,a.generation,a.world,a.ticket,a.renderer,pso_serial);
}
bool root_matches(RootWitness const& witness,ExactAssociation const& a){
    return gears_ngx_mv_compute::root_witness_matches(witness.key,witness.valid,a.canonical,a.generation,a.world,a.ticket,a.renderer,root_serial);
}
// Root identity is obtained only from this typed setter. The serialized-root
// bank owns the matching object through the observation; no scalar becomes COM.
// Temporary QI/GetDevice owners release after the callback's frame lock.
struct RootScope {
    Resolution& resolved;RootWitness candidate;bool entered{},outer{},attempted{},finished{},owned{};
    gears_vr::ComPtr<IUnknown> identity,device_identity;
    gears_vr::ComPtr<ID3D12Device> device;
    RootScope(Resolution& r,ID3D12RootSignature* native,uintptr_t caller,bool own):resolved(r),owned(own){
        {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!exact_pending())return;
            entered=true;outer=root_set_depth++==0;++root_sets;root_invalidate();
            if(!outer||!native||!ngx_stereo_host::attested||!target_observing(owned)||!resolved.canonical||
               (caller!=base+0x138124&&caller!=base+0x22018d))return;
            const auto live=frame_link.current(resolved.canonical);auto const& w=ngx_mv_target_capture::window.snapshot();
            if(live.identity!=resolved.canonical||!live.generation){++root_rejected;return;}
            candidate.key={resolved.canonical,0,0,reinterpret_cast<uintptr_t>(consuming_renderer),live.generation,w.selection.world,consuming_frame_ticket,root_serial};}
        if(resolving){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++root_rejected;return;}
        resolving=true;HRESULT hr=E_FAIL;
        try{hr=native->QueryInterface(IID_PPV_ARGS(&identity));if(SUCCEEDED(hr))hr=native->GetDevice(IID_PPV_ARGS(&device));
            if(SUCCEEDED(hr))hr=device.As(&device_identity);}
        catch(...){resolving=false;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++root_rejected;root_invalidate();return;}
        resolving=false;
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        if(FAILED(hr)||!identity||!device_identity||!exact_pending()||candidate.key.serial!=root_serial||
           reinterpret_cast<uintptr_t>(device_identity.Get())!=ngx_mv_target_capture::selected_mv.device){++root_rejected;return;}
        candidate.key.root=reinterpret_cast<uintptr_t>(identity.Get());candidate.key.device=reinterpret_cast<uintptr_t>(device_identity.Get());
        const auto source=ngx_root_signature_capture::lookup(candidate.key.root,candidate.key.device);
        if(!source.found||!source.retained||source.fixture){++root_unknown;return;}
        candidate.key.provenance=source.index;candidate.key.size=source.size;attempted=true;
    }
    void finish(){
        finished=true;if(!attempted)return;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        const auto live=frame_link.current(candidate.key.canonical);
        if(!exact_pending()||!target_observing(owned)||live.identity!=candidate.key.canonical||
           !gears_ngx_mv_compute::root_witness_matches(candidate.key,true,candidate.key.canonical,live.generation,
                committed_world_generation.load(),consuming_frame_ticket,reinterpret_cast<uintptr_t>(consuming_renderer),root_serial)){
            ++root_rejected;return;}
        candidate.valid=true;root_pending=candidate;++root_candidates;
    }
    ~RootScope(){if(entered){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);--root_set_depth;if(!finished)root_invalidate();}}
};
struct PsoSetActivity {
    PsoWitness retired;bool outer{},entered{};
    PsoSetActivity(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!ngx_mv_target_capture::pending())return;
        entered=true;outer=pso_set_depth++==0;++pso_sets;
        pso_invalidate(1);if(outer){retired=std::move(pso_pending);pso_pending=PsoWitness{};}}
    ~PsoSetActivity(){if(entered)--pso_set_depth;}
};
struct SetScope {
    // Activity revokes before alias QI can reenter. All local owners die after
    // the enclosing callback lock; Scope must enclose this entire object.
    PsoSetActivity activity;Resolution resolved;PsoWitness candidate;
    gears_vr::ComPtr<ID3D12Device> device;gears_vr::ComPtr<IUnknown> device_identity;
    bool owned{},attempted{},finished{};
    SetScope(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso,uintptr_t caller,bool own):resolved(activity.outer?list:nullptr),owned(own){
        {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
            if(!activity.outer||!pso||!ngx_stereo_host::attested||caller!=base+0x17b514||!target_observing(owned)||!resolved.canonical)return;
            auto const& w=ngx_mv_target_capture::window.snapshot();const auto live=frame_link.current(resolved.canonical);
            if(live.identity!=resolved.canonical||!live.generation){++pso_discards;pso_last_discard=6;return;}
            candidate.key={resolved.canonical,0,reinterpret_cast<uintptr_t>(consuming_renderer),live.generation,w.selection.world,consuming_frame_ticket,pso_serial};}
        try{if(!resolved.selected_device(list)){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++pso_discards;pso_last_discard=7;return;}}
        catch(...){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++pso_discards;pso_invalidate(4);return;}
        // Actual typed callback arguments only. Unknown PSO identity/device
        // drops this witness, without stopping the exact-MV association.
        resolving=true;HRESULT hr=E_FAIL;
        try{hr=pso->QueryInterface(IID_PPV_ARGS(&candidate.pso_identity));
            if(SUCCEEDED(hr))hr=pso->GetDevice(IID_PPV_ARGS(&device));
            if(SUCCEEDED(hr))hr=device?device.As(&device_identity):E_FAIL;}
        catch(...){resolving=false;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++pso_qi_failures;++pso_discards;pso_invalidate(4);return;}
        resolving=false;
        {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
            if(FAILED(hr)||!candidate.pso_identity||!device_identity){++pso_qi_failures;++pso_discards;pso_last_discard=4;return;}
            if(reinterpret_cast<uintptr_t>(device_identity.Get())!=ngx_mv_target_capture::selected_mv.device){++pso_discards;pso_last_discard=7;return;}
            candidate.key.pso=reinterpret_cast<uintptr_t>(candidate.pso_identity.Get());
            IUnknown* id=resolved.identity?resolved.identity.Get():resolved.held?resolved.held->identity.Get():nullptr;
            if(!id||reinterpret_cast<uintptr_t>(id)!=candidate.key.canonical)return;
            candidate.list=list;candidate.list_identity=id;candidate.pso=pso;attempted=true;++pso_candidates;}
    }
    void finish(){
        finished=true;if(!attempted)return;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        const auto live=frame_link.current(candidate.key.canonical);
        if(!target_observing(owned)||live.identity!=candidate.key.canonical||live.generation!=candidate.key.generation||
           !gears_ngx_mv_compute::pso_witness_matches(candidate.key,true,candidate.key.canonical,live.generation,
                committed_world_generation.load(),consuming_frame_ticket,reinterpret_cast<uintptr_t>(consuming_renderer),pso_serial)){
            ++pso_discards;pso_last_discard=3;return;}
        candidate.valid=true;pso_pending=std::move(candidate);
    }
    ~SetScope(){if(activity.entered&&!finished){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);pso_invalidate(5);}}
};
struct ExactTableCall {
    Resolution& resolved;ID3D12GraphicsCommandList* list;ExactAssociation candidate;bool owned{},attempted{},finished{};
    ExactTableCall(Resolution& r,ID3D12GraphicsCommandList* l,UINT index,uint64_t gpu,uintptr_t caller,bool own,bool device):resolved(r),list(l),owned(own){
        if(index!=3)return;exact_mutation(2);
        if(!exact_pending()||!device||!ngx_stereo_host::attested||caller!=base+0x17b99a||!target_observing(owned))return;
        auto const& w=ngx_mv_target_capture::window.snapshot();if(w.phase!=gears_ngx_mv_target::Phase::capturing)return;
        ++exact_tables;candidate.canonical=r.canonical;candidate.generation=frame_link.current(r.canonical).generation;
        candidate.ticket=w.consume.ticket;candidate.renderer=w.consume.renderer;candidate.world=w.selection.world;candidate.serial=exact_serial;
        if(!exact_identity_current(candidate)){exact_stop(3);return;}
        candidate.descriptor=ngx_mv_descriptor_capture::lookup_exact_target(gpu);
        if(!candidate.descriptor.match){++exact_misses;return;}
        attempted=true;exact_arming=true;
    }
    void finish(){
        finished=true;if(!attempted||!exact_pending())return;
        const auto after=ngx_mv_descriptor_capture::lookup_exact_target(candidate.descriptor.gpu);
        if(!target_observing(owned)||!exact_identity_current(candidate)||exact_serial!=candidate.serial||
           !ngx_mv_descriptor_capture::same_exact(candidate.descriptor,after)){exact_stop(4);return;}
        IUnknown* id=resolved.identity?resolved.identity.Get():resolved.held?resolved.held->identity.Get():nullptr;
        if(!id||reinterpret_cast<uintptr_t>(id)!=candidate.canonical||exact_list_owner){exact_stop(1);return;}
        exact_list_owner=list;exact_identity_owner=id;candidate.active=true;exact_association=candidate;exact_arming=false;
        if(!pso_promoted.pso&&witness_matches(pso_pending,candidate)){pso_promoted=std::move(pso_pending);pso_pending=PsoWitness{};++pso_promotions;}
        else if(pso_pending.valid)pso_invalidate(3);
        if(!root_promoted.key.root&&root_matches(root_pending,candidate)){root_promoted=root_pending;root_pending={};++root_promotions;}
    }
    ~ExactTableCall(){if(attempted&&!finished)exact_stop(5);}
};
struct ExactDispatchCall {
    ExactDispatch* row{};bool owned{},finished{};
    ExactDispatchCall(Resolution const& resolved,uintptr_t caller,UINT x,UINT y,UINT z,bool own):owned(own){
        if(!exact_pending()||!exact_association.active)return;
        if(!resolved.canonical){exact_stop(1);return;}
        if(resolved.canonical!=exact_association.canonical)return;
        if(!target_observing(owned)||!exact_identity_current(exact_association)){exact_stop(3);return;}
        if(!ngx_stereo_host::attested||caller!=base+0x1ac833)return;
        const auto current=ngx_mv_descriptor_capture::lookup_exact_target(exact_association.descriptor.gpu);
        if(!ngx_mv_descriptor_capture::same_exact(exact_association.descriptor,current)){exact_stop(4);return;}
        ++exact_dispatches;if(exact_count==exact_rows.size()){++exact_overflow;exact_stop(6);return;}
        row=&exact_rows[exact_count++];row->association=exact_association;row->caller=caller;row->groups={x,y,z};
        if(witness_matches(pso_promoted,exact_association)){row->pso=pso_promoted.key.pso;row->pso_serial=pso_serial;}
        if(root_matches(root_promoted,exact_association))row->root=root_promoted.key;
        row->constants=ngx_mv_constants_capture::snapshot(resolved.raw,resolved.canonical,row->association.generation);
    }
    void finish(){
        finished=true;if(!row)return;row->returned=true;row->after=ngx_mv_descriptor_capture::lookup_exact_target(row->association.descriptor.gpu);
        row->stable=exact_pending()&&exact_association.active&&target_observing(owned)&&exact_identity_current(row->association)&&
            exact_serial==row->association.serial&&ngx_mv_descriptor_capture::same_exact(row->association.descriptor,row->after);
        if(!row->stable)exact_stop(4);
        row->pso_stable=row->stable&&row->pso&&row->pso==pso_promoted.key.pso&&row->pso_serial==pso_serial&&witness_matches(pso_promoted,row->association);
        pso_promoted.attest_stable_dispatch(row->pso_stable);
        row->root_stable=row->stable&&row->root.root&&root_matches(root_promoted,row->association)&&row->root.serial==root_serial;
        root_promoted.attest_stable_dispatch(row->root_stable);
        row->constants_stable=row->stable&&row->root_stable&&row->pso_stable&&ngx_mv_constants_capture::stable(row->constants);
    }
    ~ExactDispatchCall(){if(row&&!finished)exact_stop(5);}
};
struct DispatchIdentity {
    uintptr_t raw{},canonical{},retained_raw{},current_identity{};
    uint64_t stored_generation{},current_generation{},calls{};unsigned kind{},phase{};
};
std::array<DispatchIdentity,8> dispatch_identities;unsigned dispatch_identity_count{};uint64_t dispatch_identity_overflow{};
void note_dispatch_identity(Resolution const& resolved){
    DispatchIdentity value;value.raw=resolved.raw;value.canonical=resolved.canonical;value.kind=resolved.kind;
    value.phase=unsigned(ngx_mv_target_capture::window.snapshot().phase);
    if(resolved.held){value.retained_raw=resolved.held->raw;value.stored_generation=resolved.held->generation;}
    if(value.canonical){const auto live=frame_link.current(value.canonical);value.current_identity=live.identity;value.current_generation=live.generation;}
    for(unsigned i=0;i<dispatch_identity_count;++i){auto& old=dispatch_identities[i];
        if(old.raw==value.raw&&old.canonical==value.canonical&&old.retained_raw==value.retained_raw&&old.current_identity==value.current_identity&&
           old.stored_generation==value.stored_generation&&old.current_generation==value.current_generation&&old.kind==value.kind&&old.phase==value.phase){++old.calls;return;}}
    if(dispatch_identity_count==dispatch_identities.size()){++dispatch_identity_overflow;return;}
    value.calls=1;dispatch_identities[dispatch_identity_count++]=value;
}
bool may_admit(CallerOperation operation,uintptr_t caller){
    const auto phase=ngx_mv_target_capture::window.snapshot().phase;
    return operation==CallerOperation::root&&ngx_stereo_host::attested&&caller==base+0x22018d&&
        (phase==gears_ngx_mv_target::Phase::waiting||phase==gears_ngx_mv_target::Phase::capturing);
}
bool scoped_list(Resolution const& resolved,CallerOperation operation,uintptr_t caller){
    if(resolved.held||may_admit(operation,caller))return true;
    ++unknown_skipped[unsigned(operation)];return false;
}
Recording ref(List const& list){return {list.canonical,list.generation,frame_link.current(list.canonical).generation};}
List* reclaim(){
    ++reuse_attempts;capacity_occupancy={};List* victim{};
    for(unsigned i=0;i<list_count;++i){auto& held=lists[i];auto& o=capacity_occupancy;++o.occupied;
        const auto live=frame_link.current(held.canonical);const bool empty=bindings.binding_empty(held.canonical);
        const bool busy=held.callback_busy||held.reset_busy;
        const bool identity_matches=live.identity==held.canonical||(!held.generation&&!live.generation&&!live.identity);
        const bool eligible=identity_matches&&gears_ngx_mv_compute::reclaimable(held.revoked,busy,held.history_pinned,empty,held.generation,live.generation);
        // Counts overlap deliberately: each scalar states a protection present
        // on this bounded snapshot, rather than hiding multiple blockers.
        if(eligible){++o.eligible;if(!victim)victim=&held;}
        if(!empty)++o.partial;if(held.history_pinned)++o.pinned;if(busy)++o.busy;if(!held.revoked)++o.unrevoked;
        if(!held.generation&&live.generation)++o.unknown_blocked;
        if(!identity_matches||(held.generation&&held.generation!=live.generation))++o.stale_blocked;
    }return victim;
}
List* track(ID3D12GraphicsCommandList* list,Retirement& retirement,Resolution& resolved,CallerOperation operation,uintptr_t caller){
    auto* old=resolved.held;if(old){const auto observed=ref(*old);
        if(!old->generation||!observed.attested_current_generation||observed.attested_current_generation!=old->generation){
            lose(invalid_generation,"track_existing",old,reinterpret_cast<uintptr_t>(list));return nullptr;}return old;}
    if(!may_admit(operation,caller))return nullptr;
    if(!list||list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT)return nullptr;
    auto& id=retirement.identity;auto& device=retirement.device;auto& device_id=retirement.device_identity;
    id=std::move(resolved.identity);
    if(!id||FAILED(list->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device.As(&device_id))||!device_id){
        lose(invalid_list,"track_identity",nullptr,reinterpret_cast<uintptr_t>(list),reinterpret_cast<uintptr_t>(id.Get()));return nullptr;}
    // A verified list belonging to another device cannot describe the selected
    // target. It neither consumes a row nor invalidates this device's evidence.
    if(reinterpret_cast<uintptr_t>(device_id.Get())!=ngx_mv_target_capture::selected_mv.device){++foreign_device_skips;return nullptr;}
    const auto canonical=reinterpret_cast<uintptr_t>(id.Get());const auto current=frame_link.current(canonical);
    if(!canonical||current.identity!=canonical||!current.generation){lose(invalid_generation,"track_new_generation",nullptr,reinterpret_cast<uintptr_t>(list),canonical);return nullptr;}
    // Resolution ran before the callback lock. Another callback may have
    // admitted this same canonical identity in that interval; reuse strictly.
    for(unsigned i=0;i<list_count;++i)if(lists[i].canonical==canonical){
        auto& held=lists[i];if(!held.generation||held.generation!=current.generation){lose(invalid_generation,"track_setup_alias",&held,reinterpret_cast<uintptr_t>(list));return nullptr;}return &held;}
    List* slot{};
    if(list_count==lists.size()){
        slot=reclaim();if(!slot){lose(list_capacity,"track_capacity",nullptr,reinterpret_cast<uintptr_t>(list),canonical);return nullptr;}
        if(!bindings.erase_empty(slot->canonical)){lose(list_capacity,"track_reclaim_core",slot);return nullptr;}
        retirement.retired=std::move(*slot);*slot=List{};++reused;
    }else slot=&lists[list_count++];
    auto& out=*slot;out.raw=reinterpret_cast<uintptr_t>(list);out.canonical=canonical;
    out.generation=current.generation;out.held=list;out.identity=std::move(id);return &out;
}
bool current(List const& list){ngx_mv_target_capture::poll();const auto r=ref(list);if(!r.attested_current_generation||r.generation!=r.attested_current_generation){lose(invalid_generation,"current",&list);return false;}return pending();}
void noted(List& list){if(bindings.failed()){lose(invalid_generation,"noted_core",&list);return;}list.revoked=false;++mutation;
    if(ngx_mv_target_capture::window.snapshot().phase==gears_ngx_mv_target::Phase::waiting)++waiting_bindings;else ++capture_bindings;}
void invalidate_held(List* held,unsigned kind){
    if(!pending()||!held)return;
    // Erasing knowledge requires only the retained identity. It cannot admit a
    // recording and must not validate a generation already replaced by Reset.
    ++invalidations;++mutation;held->revoked=true;bindings.forget_binding(held->canonical);
    (void)kind;
}
struct RevocationScope {
    Resolution resolved;unsigned kind;bool finished{};
    RevocationScope(ID3D12GraphicsCommandList* list,unsigned value):resolved(list),kind(value){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(ngx_mv_target_capture::pending()){pso_invalidate(2);root_invalidate();ngx_mv_constants_capture::invalidate();}exact_mutation(8);invalidate_held(resolved.held,kind);}
    void finish(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);finished=true;invalidate_held(resolved.held,kind);}
    ~RevocationScope(){if(!finished){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);invalidate_held(resolved.held,kind);if(pending())lose(unwinding,"revoke_unwind",resolved.held,resolved.raw);}}
};
struct ResetScope {
    Resolution resolved;ID3D12GraphicsCommandList* list;List* retained{};bool returned{};
    explicit ResetScope(ID3D12GraphicsCommandList* l):resolved(l),list(l){
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);EvidenceScope evidence("reset_begin",list);
        if(ngx_mv_target_capture::pending()){pso_invalidate(2);root_invalidate();ngx_mv_constants_capture::invalidate();}
        exact_mutation(8);
        if(!pending())return;auto* held=resolved.held;if(!held)return;
        retained=held;++held->reset_busy;
        held->reset_prior_generation=held->generation;held->reset_prior_known=true;
        invalidate_held(held,0);held->generation=0;
    }
    void finish(HRESULT hr,gears_frame_link::ListRef actual={}){
        EvidenceScope evidence("reset_finish",list);
        returned=true;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        auto* held=retained;if(held){--held->reset_busy;retained=nullptr;}if(!pending()||!held)return;
        if(FAILED(hr)){lose(invalid_generation,"reset_finish_failed",held,reinterpret_cast<uintptr_t>(list),0,hr);return;}
        // A successful bypass/unowned Reset supplies no birth receipt. Leave
        // this list unknown rather than borrowing a still-visible old tracker
        // generation. Only this exact Reset invocation may supply `actual`.
        if(!actual.identity&&!actual.generation)return;
        const auto live=frame_link.current(held->canonical);
        if(actual.identity!=held->canonical||!actual.generation||live.identity!=actual.identity||live.generation!=actual.generation){
            lose(invalid_generation,"reset_finish_attestation",held,reinterpret_cast<uintptr_t>(list),0,hr);return;
        }
        ++mutation;held->generation=actual.generation;
        bindings.reset({held->canonical,actual.generation,live.generation});
        if(bindings.failed())lose(invalid_generation,"reset_finish_core",held,reinterpret_cast<uintptr_t>(list),0,hr);
    }
    ~ResetScope(){if(!returned){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);EvidenceScope evidence("reset_unwind",list);
        auto* held=retained;if(held){--held->reset_busy;retained=nullptr;}if(pending())lose(unwinding,"reset_unwind",held,reinterpret_cast<uintptr_t>(list));}}
};
void STDMETHODCALLTYPE heaps(ID3D12GraphicsCommandList* list,UINT count,ID3D12DescriptorHeap* const* native){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());Scope scope;Resolution resolved(list);Retirement retirement;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);census(CallerOperation::heaps,caller,scope.world());EvidenceScope evidence("heaps",list);
    ngx_mv_constants_capture::invalidate();exact_mutation(9);
    const bool observe=observing(scope.world()),scoped=observe&&scoped_list(resolved,CallerOperation::heaps,caller);Depth call(resolved.held||scoped);
    List* held=call.outer&&scoped?track(list,retirement,resolved,CallerOperation::heaps,caller):nullptr;ListUse use(held);
    HeapLease selected;std::array<ID3D12DescriptorHeap*,2> values{};
    if(held){
        if(count>values.size()||(count&&!safe_copy(values.data(),native,count*sizeof(values[0]))))lose(invalid_heap,"heaps_array",held);
        else for(UINT i=0;i<count;++i){if(!values[i]){lose(invalid_heap,"heaps_null",held);break;}
            const auto desc=values[i]->GetDesc();if(desc.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)continue;
            if(selected.retained()){lose(invalid_heap,"heaps_duplicate",held);break;}selected=HeapLease::observe(values[i]);
            if(!selected.retained()||!selected.visible||selected.device!=ngx_mv_target_capture::selected_mv.device){lose(invalid_heap,"heaps_identity",held);break;}}
    }else invalidate_held(resolved.held,0);
    const auto before=mutation;original_heaps(list,count,native);
    if(held&&current(*held)){if(before!=mutation){lose(changed,"heaps_post_mutation",held);return;}
        held->heap=std::move(selected);bindings.shader_heap(ref(*held),held->heap.canonical);noted(*held);}
}
void STDMETHODCALLTYPE root(ID3D12GraphicsCommandList* list,ID3D12RootSignature* native){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());Scope scope;Resolution resolved(list);Retirement retirement;
    RootScope witness(resolved,native,caller,scope.world());
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);census(CallerOperation::root,caller,scope.world());EvidenceScope evidence("root",list);
    ngx_mv_constants_capture::invalidate();exact_mutation(9);
    const bool observe=observing(scope.world()),scoped=observe&&scoped_list(resolved,CallerOperation::root,caller);Depth call(resolved.held||scoped);
    List* held=call.outer&&scoped?track(list,retirement,resolved,CallerOperation::root,caller):nullptr;ListUse use(held);
    gears_vr::ComPtr<ID3D12RootSignature> owner;gears_vr::ComPtr<IUnknown> id;
    if(held&&native){gears_vr::ComPtr<ID3D12Device> device;gears_vr::ComPtr<IUnknown> di;owner=native;
        if(FAILED(native->QueryInterface(IID_PPV_ARGS(&id)))||FAILED(native->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device.As(&di))||
           reinterpret_cast<uintptr_t>(di.Get())!=ngx_mv_target_capture::selected_mv.device)lose(invalid_root,"root_identity",held);
    }else if(!held)invalidate_held(resolved.held,0);
    const auto before=mutation;original_root(list,native);witness.finish();
    if(held&&current(*held)){if(before!=mutation){lose(changed,"root_post_mutation",held);return;}held->root=std::move(owner);held->root_identity=std::move(id);
        bindings.compute_root_signature(ref(*held),reinterpret_cast<uintptr_t>(held->root_identity.Get()));noted(*held);}
}
void STDMETHODCALLTYPE table(ID3D12GraphicsCommandList* list,UINT index,D3D12_GPU_DESCRIPTOR_HANDLE gpu){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());Scope scope;Resolution resolved(list);Retirement retirement;
    const bool probe_device=index==3&&ngx_stereo_host::attested&&caller==base+0x17b99a&&resolved.selected_device(list);
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);census(CallerOperation::table,caller,scope.world());EvidenceScope evidence("table",list);
    const bool observe=observing(scope.world()),scoped=observe&&scoped_list(resolved,CallerOperation::table,caller);Depth call(resolved.held||scoped);
    List* held=call.outer&&scoped?track(list,retirement,resolved,CallerOperation::table,caller):nullptr;ListUse use(held);if(!held)invalidate_held(resolved.held,0);
    ExactTableCall probe(resolved,list,index,gpu.ptr,caller,scope.world(),probe_device);
    const auto before=mutation;original_table(list,index,gpu);probe.finish();
    ngx_mv_constants_capture::table(resolved.raw,resolved.canonical,frame_link.current(resolved.canonical).generation,index,gpu.ptr,caller);
    if(held&&current(*held)){if(before!=mutation){lose(changed,"table_post_mutation",held);return;}bindings.compute_root_table(ref(*held),index,gpu.ptr);noted(*held);}
}
void STDMETHODCALLTYPE dispatch(ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());Scope scope;
    Resolution resolved(list);Retirement retirement;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);census(CallerOperation::dispatch,caller,scope.world());EvidenceScope evidence("dispatch",list);
    if(target_observing(scope.world()))note_dispatch_identity(resolved);
    const bool observe=observing(scope.world()),scoped=observe&&scoped_list(resolved,CallerOperation::dispatch,caller);Depth call(resolved.held||scoped);
    if(!observe)invalidate_held(resolved.held,0);
    Row* row{};List* held{};ListUse use;
    ExactDispatchCall probe(resolved,caller,x,y,z,scope.world());
    if(call.outer&&observe&&ngx_mv_target_capture::window.snapshot().phase==gears_ngx_mv_target::Phase::capturing){
        ++dispatches;if(scoped)held=track(list,retirement,resolved,CallerOperation::dispatch,caller);use.attach(held);
        if(held){const auto candidate=bindings.dispatch(ref(*held));
            if(!candidate.binding_observed)++unbound;
            else {auto observed=ngx_mv_descriptor_capture::lookup_target(candidate.shader_visible_heap,candidate.root3_gpu);
                if(observed.overflow)lose(descriptor_capacity,"dispatch_descriptor_capacity",held);
                else if(!observed.ready||(!observed.exact_base_mv&&!observed.count))++unrelated;
                else if(row_count==rows.size())lose(row_capacity,"dispatch_row_capacity",held);
                else {held->history_pinned=true;row=&rows[row_count++];auto const& w=ngx_mv_target_capture::window.snapshot();
                    row->world=w.selection.world;row->ticket=w.consume.ticket;row->renderer=w.consume.renderer;
                    row->raw=reinterpret_cast<uintptr_t>(list);row->canonical=held->canonical;row->generation=held->generation;
                    row->root=candidate.compute_root_signature;row->heap=candidate.shader_visible_heap;row->root3=candidate.root3_gpu;
                    row->caller=caller;row->groups={x,y,z};row->before=observed;
                    row->caller_candidate=ngx_stereo_host::attested&&caller==base+0x1ac833;}
            }
        }
    }
    // No descriptor observer mutex is held across the native Dispatch.
    const auto before=mutation;original_dispatch(list,x,y,z);probe.finish();
    if(row){row->returned=true;row->after=ngx_mv_descriptor_capture::lookup_target(row->heap,row->root3);
        row->stable=held&&current(*held)&&before==mutation&&gears_ngx_mv_compute::same(row->before,row->after);
        if(!row->stable)lose(changed,"dispatch_post",held);}
}
void STDMETHODCALLTYPE clear(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso){
    Scope scope;RevocationScope revoke(list,1);std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);EvidenceScope evidence("clear",list);original_clear(list,pso);revoke.finish();
}
void STDMETHODCALLTYPE set(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());Scope scope;SetScope witness(list,pso,caller,scope.world());
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);original_set(list,pso);witness.finish();
}
void STDMETHODCALLTYPE bundle(ID3D12GraphicsCommandList* list,ID3D12GraphicsCommandList* child){
    Scope scope;RevocationScope revoke(list,2);std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);EvidenceScope evidence("bundle",list);original_bundle(list,child);revoke.finish();
}
bool install_hooks(void** vtable){
    constexpr unsigned first=viewport_hook_base_capacity+ngx_mv_descriptor_hooks;
    viewport_targets[first]=vtable[28];viewport_targets[first+1]=vtable[29];viewport_targets[first+2]=vtable[31];viewport_targets[first+3]=vtable[14];
    bool ok=install(viewport_targets[first],reinterpret_cast<void*>(heaps),reinterpret_cast<void**>(&original_heaps));
    ok=install(viewport_targets[first+1],reinterpret_cast<void*>(root),reinterpret_cast<void**>(&original_root))&&ok;
    ok=install(viewport_targets[first+2],reinterpret_cast<void*>(ngx_mv_compute_capture::table),reinterpret_cast<void**>(&original_table))&&ok;
    ok=install(viewport_targets[first+3],reinterpret_cast<void*>(dispatch),reinterpret_cast<void**>(&original_dispatch))&&ok;
#if !defined(GEARS_DEATH_PSO_CACHE) || !GEARS_DEATH_PSO_CACHE
    static_assert(ngx_mv_compute_hooks==7,"direct PSO witness adds one compute hook");
    viewport_targets[first+4]=vtable[11];viewport_targets[first+5]=vtable[27];viewport_targets[first+6]=vtable[25];
    ok=install(viewport_targets[first+4],reinterpret_cast<void*>(clear),reinterpret_cast<void**>(&original_clear))&&ok;
    ok=install(viewport_targets[first+5],reinterpret_cast<void*>(bundle),reinterpret_cast<void**>(&original_bundle))&&ok;
    ok=install(viewport_targets[first+6],reinterpret_cast<void*>(set),reinterpret_cast<void**>(&original_set))&&ok;
#else
    static_assert(ngx_mv_compute_hooks==4,"PSO cache owns Set/Clear/Bundle hooks");
#endif
    return ok;
}
void stop(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);stopped=true;exact_stop(7);}
void release_drained(){
    std::array<List,16> retired;
    PsoWitness retired_pending,retired_promoted;
    gears_vr::ComPtr<ID3D12GraphicsCommandList> retired_exact_list;gears_vr::ComPtr<IUnknown> retired_exact_identity;
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        if(!stopped||active.load()||recording.load()||exact_pending())return;
        for(unsigned i=0;i<list_count;++i)if(lists[i].callback_busy||lists[i].reset_busy)return;
        lists.swap(retired);exact_list_owner.Swap(retired_exact_list);exact_identity_owner.Swap(retired_exact_identity);
        retired_pending=std::move(pso_pending);retired_promoted=std::move(pso_promoted);leases_released=true;}
    // Local retired owners release after the observation lock is dropped.
}
bool snapshot_picked_pso(gears_vr::ComPtr<ID3D12PipelineState>& out,PsoWitnessKey& key){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(out||leases_released||ngx_mv_target_capture::window.snapshot().phase!=gears_ngx_mv_target::Phase::frozen||
       !pso_promoted.historical_stable||!pso_promoted.pso||!pso_promoted.pso_identity)return false;
    out=pso_promoted.pso;key=pso_promoted.key;return true;
}
struct BlobReceipt {
    PsoWitnessKey key;bool attempted{},completed{},saved{},details_readable{};HRESULT hr=E_PENDING;
    size_t size{};DWORD error{},written{},worker{};wchar_t path[MAX_PATH]{};
} blob_receipt;
std::atomic<DWORD> blob_worker_thread{};
void register_worker(){DWORD empty{};blob_worker_thread.compare_exchange_strong(empty,GetCurrentThreadId());}
HRESULT picked_cached_blob(ID3D12PipelineState* pso,ID3DBlob** blob){
    __try{return pso->GetCachedBlob(blob);}__except(EXCEPTION_EXECUTE_HANDLER){*blob=nullptr;return E_FAIL;}
}
bool picked_blob_data(ID3DBlob* blob,size_t& size,void*& data){
    __try{size=blob->GetBufferSize();data=blob->GetBufferPointer();return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){size=0;data=nullptr;return false;}
}
void poll_picked_blob(){
    const auto worker=blob_worker_thread.load();if(!worker||worker!=GetCurrentThreadId())return;
    // This independent activity outlives every native Release below. Worker
    // calls own no callback/frame lock; self-test/RHI callers are rejected.
    NativeOwnedShutdownRhiScope::Activity activity;
    gears_vr::ComPtr<ID3D12PipelineState> pso;gears_vr::ComPtr<ID3DBlob> blob;PsoWitnessKey key;
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        if(blob_receipt.attempted||!snapshot_picked_pso(pso,key))return;
        blob_receipt.attempted=true;blob_receipt.key=key;blob_receipt.worker=worker;}
    BlobReceipt result;result.attempted=true;result.key=key;result.worker=worker;
    ID3DBlob* returned{};result.hr=picked_cached_blob(pso.Get(),&returned);blob.Attach(returned);
    if(SUCCEEDED(result.hr)&&blob){
        void* data{};result.details_readable=picked_blob_data(blob.Get(),result.size,data);
        if(!result.details_readable||!data||!result.size)result.error=ERROR_INVALID_DATA;
        else if(!gears_ngx_mv_compute::pso_blob_save_size(result.size))result.error=ERROR_FILE_TOO_LARGE;
        else {
            wchar_t parent[MAX_PATH]{};const auto length=GetModuleFileNameW(own_module,parent,MAX_PATH);
            auto* slash=length&&length<MAX_PATH?wcsrchr(parent,L'\\'):nullptr;
            if(!slash)result.error=length>=MAX_PATH?ERROR_BUFFER_OVERFLOW:ERROR_PATH_NOT_FOUND;
            else {*slash=0;
                if(_snwprintf_s(result.path,MAX_PATH,_TRUNCATE,L"%s\\ngx-mv-picked-pso-%lu-%llu-%llu-%lld.raw",parent,GetCurrentProcessId(),key.world,key.ticket,now())<0)result.error=ERROR_BUFFER_OVERFLOW;
                else {
                    HANDLE file=CreateFileW(result.path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
                    if(file==INVALID_HANDLE_VALUE)result.error=GetLastError();
                    else {
                        if(!WriteFile(file,data,static_cast<DWORD>(result.size),&result.written,nullptr))result.error=GetLastError();
                        else if(result.written!=result.size)result.error=ERROR_WRITE_FAULT;
                        else if(!FlushFileBuffers(file))result.error=GetLastError();
                        else result.saved=true;
                        if(!CloseHandle(file)){result.saved=false;if(!result.error)result.error=GetLastError();}
                    }
                }
            }
        }
    }else if(SUCCEEDED(result.hr))result.error=ERROR_INVALID_DATA;
    // Explicitly finish all COM ownership outside locks and before publishing
    // completion. The activity guard remains alive through both Releases.
    blob.Reset();pso.Reset();result.completed=true;
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);blob_receipt=result;}
}
void print(FILE* file){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!file)return;
    ngx_mv_constants_capture::print(file);
    fprintf(file,"ngx_mv_compute enabled=1 lists=%u list_limit=16 rows=%u row_limit=32 failed=%u failure=%u core_loss=%u stopped=%u leases_released=%u dispatches=%llu unbound=%llu unrelated=%llu invalidations=%llu waiting_bindings=%llu capture_bindings=%llu row_scope=selected_mv_related table_membership_verified=0 gpu_execution_verified=0 producer_authorized=0\n",
        list_count,row_count,unsigned(failed),failure,unsigned(bindings.loss()),unsigned(stopped),unsigned(leases_released),dispatches,unbound,unrelated,invalidations,waiting_bindings,capture_bindings);
    fprintf(file,"ngx_mv_compute_admission scope=new_admission_setup22018d incomplete_coverage=1 new_phase=waiting_or_capturing attestation_required=1 known_lookup=typed_raw_then_canonical_qi skip_scope=bank_pending_owned skipped_unknown_heaps=%llu skipped_unknown_root=%llu skipped_unknown_table=%llu skipped_unknown_dispatch=%llu selected_mv_inferred=0\n",
        unknown_skipped[unsigned(CallerOperation::heaps)],unknown_skipped[unsigned(CallerOperation::root)],unknown_skipped[unsigned(CallerOperation::table)],unknown_skipped[unsigned(CallerOperation::dispatch)]);
    fprintf(file,"ngx_mv_compute_resolution raw_matches=%llu alias_matches=%llu unmatched=%llu qi_failures=%llu reentrant_rejections=%llu scope=bank_or_exact_probe_pending temporary_release_after_local_frame_lock=1 alias_bank=0\n",
        resolved_raw,resolved_alias,resolved_unmatched,resolve_qi_failed,resolve_reentrant);
    auto const& picked=pso_promoted.key;
    fprintf(file,"ngx_mv_pso_witness sets=%llu candidates=%llu promotions=%llu discards=%llu qi_failures=%llu last_discard=%u serial=%llu pending_valid=%u pending_retained=%u promoted_current_valid=%u promoted_retained=%u historical_stable_dispatch=%u canonical=%llx pso=%llx generation=%llu world=%llu ticket=%llu renderer=%llx witness_serial=%llu scope=attested_set17b514_single_pending_single_promoted blob_calls=%u gpu_execution_verified=0 producer_authorized=0\n",
        pso_sets,pso_candidates,pso_promotions,pso_discards,pso_qi_failures,pso_last_discard,pso_serial,unsigned(pso_pending.valid),unsigned(bool(pso_pending.pso)),unsigned(pso_promoted.valid),unsigned(bool(pso_promoted.pso)),unsigned(pso_promoted.historical_stable),picked.canonical,picked.pso,picked.generation,picked.world,picked.ticket,picked.renderer,picked.serial,unsigned(blob_receipt.attempted));
    auto const& b=blob_receipt;auto const& k=b.key;
    auto const& rk=root_promoted.key;
    fprintf(file,"ngx_mv_root_witness sets=%llu candidates=%llu promotions=%llu unknown=%llu rejected=%llu serial=%llu pending_valid=%u current_valid=%u historical_stable_dispatch=%u canonical=%llx root=%llx device=%llx generation=%llu world=%llu ticket=%llu renderer=%llx witness_serial=%llu provenance_row=%u serialized_size=%zu scope=typed_set138124_or22018d_exact_mv_dispatch gpu_execution_verified=0\n",
        root_sets,root_candidates,root_promotions,root_unknown,root_rejected,root_serial,unsigned(root_pending.valid),unsigned(root_promoted.valid),unsigned(root_promoted.historical_stable),
        rk.canonical,rk.root,rk.device,rk.generation,rk.world,rk.ticket,rk.renderer,rk.serial,rk.provenance,rk.size);
    for(unsigned i=0;i<exact_count;++i){auto const& r=exact_rows[i];auto const& key=r.root;
        fprintf(file,"ngx_mv_exact_root row=%u stable=%u canonical=%llx root=%llx device=%llx generation=%llu world=%llu ticket=%llu renderer=%llx witness_serial=%llu provenance_row=%u serialized_size=%zu\n",
            i,unsigned(r.root_stable),key.canonical,key.root,key.device,key.generation,key.world,key.ticket,key.renderer,key.serial,key.provenance,key.size);}
    for(unsigned i=0;i<exact_count;++i){auto const& r=exact_rows[i];auto const& c=r.constants;auto const& u=c.upload;
        fprintf(file,"ngx_mv_exact_constants row=%u valid=%u stable=%u eye=%u context=%llx view=%llx bank=%llx meta=%llx source=%llx mapped=%llx ring=%llx resource=%llx cbv_cpu=%llx cbv_gpu=%llx table_gpu=%llx offset=%u used=%u bytes=160 source_rect=%.9g,%.9g,%.9g,%.9g target_rect=%.9g,%.9g,%.9g,%.9g values=",
            i,unsigned(c.valid),unsigned(r.constants_stable),c.eye,u.context,c.view,u.bank,u.meta,u.data,u.mapped,u.ring,u.resource,u.cpu,u.gpu,c.gpu,u.offset,u.used,
            c.rectangles[0],c.rectangles[1],c.rectangles[2],c.rectangles[3],c.rectangles[4],c.rectangles[5],c.rectangles[6],c.rectangles[7]);
        for(auto byte:u.upload)fprintf(file,"%02x",unsigned(byte));fprintf(file," cpu_upload_only=1 gpu_execution_verified=0\n");}
    fprintf(file,"ngx_mv_pso_blob attempted=%u completed=%u saved=%u hr=%08lx size=%zu limit=4194304 written=%lu error=%lu details_readable=%u worker=%lu canonical=%llx pso=%llx generation=%llu world=%llu ticket=%llu renderer=%llx witness_serial=%llu path=%ls attempts_limit=1 create_new=1 worker_only=1 internal_allocation_bounded=0 interpretation=opaque_cache_only shader_identity_verified=0 gpu_execution_verified=0 producer_authorized=0\n",
        unsigned(b.attempted),unsigned(b.completed),unsigned(b.saved),static_cast<unsigned long>(b.hr),b.size,b.written,b.error,unsigned(b.details_readable),b.worker,k.canonical,k.pso,k.generation,k.world,k.ticket,k.renderer,k.serial,b.path);
    fprintf(file,"ngx_mv_exact_probe tables=%llu misses=%llu dispatches=%llu rows=%u limit=8 overflow_calls=%llu active=%u arming=%u stopped=%u reason=%u serial=%llu typed_list_retained=%u identity_retained=%u scope=owned_capturing_attested_table17b99a_dispatch1ac833 independent_of_binding_bank=1 heap_observed=0 root_witness_separate=1 full_bindings_verified=0 table_extent_verified=0 gpu_execution_verified=0 producer_authorized=0 reason_identity=1 reason_table_overwrite=2 reason_generation=3 reason_changed=4 reason_unwind=5 reason_capacity=6 reason_window_end=7 reason_reset_clear_bundle=8 reason_heap_root=9\n",
        exact_tables,exact_misses,exact_dispatches,exact_count,exact_overflow,unsigned(exact_association.active),unsigned(exact_arming),unsigned(exact_stopped),exact_reason,exact_serial,unsigned(bool(exact_list_owner)),unsigned(bool(exact_identity_owner)));
    for(unsigned i=0;i<exact_count;++i){auto const& r=exact_rows[i];auto const& a=r.association;auto const& d=a.descriptor;
        fprintf(file,"ngx_mv_exact_dispatch row=%u world=%llu ticket=%llu renderer=%llx canonical=%llx generation=%llu serial=%llu heap=%llx resource=%llx gpu=%llx version=%llu origin=%llu source_handle=%llx source_version=%llu after_match=%u after_version=%llu after_origin=%llu caller=%llx groups=%u,%u,%u returned=%u stable=%u pso=%llx pso_serial=%llu pso_stable=%u heap_observed=0 root_observed=%u full_bindings_verified=0 gpu_execution_verified=0 producer_authorized=0\n",
            i,a.world,a.ticket,a.renderer,a.canonical,a.generation,a.serial,d.heap,d.resource,d.gpu,d.version,d.origin,d.source_handle,d.source_version,unsigned(r.after.match),r.after.version,r.after.origin,r.caller,r.groups[0],r.groups[1],r.groups[2],unsigned(r.returned),unsigned(r.stable),r.pso,r.pso_serial,unsigned(r.pso_stable),unsigned(r.root_stable));}
    fprintf(file,"ngx_mv_compute_dispatch_identities rows=%u limit=8 overflow_calls=%llu kind_not_attempted=0 kind_raw=1 kind_alias=2 kind_unmatched=3 kind_qi_failed=4\n",dispatch_identity_count,dispatch_identity_overflow);
    for(unsigned i=0;i<dispatch_identity_count;++i){auto const& d=dispatch_identities[i];
        fprintf(file,"ngx_mv_compute_dispatch_identity row=%u raw=%llx canonical=%llx retained_raw=%llx stored_generation=%llu current_identity=%llx current_generation=%llu kind=%u phase=%u calls=%llu scalar_only=1\n",
            i,d.raw,d.canonical,d.retained_raw,d.stored_generation,d.current_identity,d.current_generation,d.kind,d.phase,d.calls);}
    auto const& o=capacity_occupancy;
    fprintf(file,"ngx_mv_compute_reuse attempts=%llu reused=%llu foreign_device_skips=%llu capacity_snapshot=%u occupied=%u eligible=%u partial_or_absent=%u pinned=%u busy=%u unrevoked=%u unknown_blocked=%u stale_blocked=%u blocker_counts_overlap=1\n",
        reuse_attempts,reused,foreign_device_skips,unsigned(reuse_attempts!=0),o.occupied,o.eligible,o.partial,o.pinned,o.busy,o.unrevoked,o.unknown_blocked,o.stale_blocked);
    auto const& f=first_failure;
    fprintf(file,"ngx_mv_compute_first_failure present=%u reason=%u site=%s operation=%s raw=%llx canonical=%llx stored_generation=%llu current_identity=%llx current_generation=%llu prior_stored_known=%u prior_stored_generation=%llu result=%08lx bypass=%u owned_known=%u owned=%u ticket=%llu world=%llu frozen_ticket=%llu frozen_world=%llu renderer=%llx phase=%u core_loss_at_failure=%u depth=%u lists=%u rows=%u mutation=%llu\n",
        unsigned(f.present),f.reason,f.site,f.operation,f.raw,f.canonical,f.stored_generation,f.current_identity,f.current_generation,
        unsigned(f.prior_stored_known),f.prior_stored_generation,static_cast<unsigned long>(f.result),unsigned(f.bypass),unsigned(f.owned_known),unsigned(f.owned),
        f.ticket,f.world,f.frozen_ticket,f.frozen_world,f.renderer,f.window_phase,f.core_loss,f.depth_at_failure,f.lists_at_failure,f.rows_at_failure,f.mutation_at_failure);
    auto const& fo=f.occupancy;
    fprintf(file,"ngx_mv_compute_first_failure_occupancy present=%u occupied=%u eligible=%u partial_or_absent=%u pinned=%u busy=%u unrevoked=%u unknown_blocked=%u stale_blocked=%u blocker_counts_overlap=1\n",
        unsigned(f.present),fo.occupied,fo.eligible,fo.partial,fo.pinned,fo.busy,fo.unrevoked,fo.unknown_blocked,fo.stale_blocked);
    fprintf(file,"ngx_mv_compute_callers rows=%u limit=32 calls=%llu overflow_calls=%llu module_base=%llx module_size=%u scalar_only=1 scope=target_pending_owned_consume bank_failure_independent=1 table_membership_verified=0 gpu_execution_verified=0 producer_authorized=0\n",
        caller_row_count,caller_calls,caller_overflow_calls,base,caller_image_size);
    constexpr char const* operations[]={"heaps","root","table","dispatch"};
    for(unsigned i=0;i<caller_row_count;++i){auto const& c=caller_rows[i];
        fprintf(file,"ngx_mv_compute_caller row=%u operation=%s phase=%u caller=%llx rva=%llx rva_valid=%u calls=%llu bank_failed_calls=%llu first_ticket=%llu last_ticket=%llu historical_caller_candidate=%u semantics_verified=0\n",
            i,operations[unsigned(c.operation)],c.phase,c.caller,c.rva,unsigned(c.in_game_image),c.calls,c.bank_failed_calls,c.first_ticket,c.last_ticket,unsigned(c.historical_candidate));}
    for(unsigned i=0;i<row_count;++i){auto const& r=rows[i];
        fprintf(file,"ngx_mv_compute_dispatch row=%u world=%llu ticket=%llu renderer=%llx list_raw=%llx list=%llx generation=%llu root=%llx heap=%llx root3=%llx groups=%u,%u,%u caller=%llx historical_caller_candidate=%u returned=%u stable=%u exact_base_mv=%u candidates=%u table_membership_verified=0 gpu_execution_verified=0 producer_authorized=0\n",
            i,r.world,r.ticket,r.renderer,r.raw,r.canonical,r.generation,r.root,r.heap,r.root3,r.groups[0],r.groups[1],r.groups[2],r.caller,unsigned(r.caller_candidate),unsigned(r.returned),unsigned(r.stable),unsigned(r.before.exact_base_mv),r.before.count);
        for(unsigned stage=0;stage<2;++stage){auto const& lookup=stage?r.after:r.before;
            fprintf(file,"ngx_mv_compute_lookup row=%u stage=%u ready=%u exact_base_mv=%u count=%u overflow=%u\n",i,stage,unsigned(lookup.ready),unsigned(lookup.exact_base_mv),lookup.count,unsigned(lookup.overflow));
            for(unsigned j=0;j<lookup.count;++j){auto const& c=lookup.candidates[j];
                fprintf(file,"ngx_mv_compute_candidate row=%u stage=%u candidate=%u cpu=%llx gpu=%llx version=%llu origin=%llu source=%llx source_version=%llu offset=%llu offset_only=1 table_membership_verified=0\n",i,stage,j,c.cpu,c.gpu,c.version,c.origin,c.source_handle,c.source_version,c.offset);}}
    }
}
#else
struct ResetScope {explicit ResetScope(ID3D12GraphicsCommandList*){}void finish(HRESULT,gears_frame_link::ListRef={}){}};
struct RevocationScope {RevocationScope(ID3D12GraphicsCommandList*,unsigned){}void finish(){}};
struct SetScope {SetScope(ID3D12GraphicsCommandList*,ID3D12PipelineState*,uintptr_t,bool){}void finish(){}};
void register_worker(){}void poll_picked_blob(){}
void stop(){}void release_drained(){}void print(FILE*){}
bool install_hooks(void**){return true;}
#endif
}
