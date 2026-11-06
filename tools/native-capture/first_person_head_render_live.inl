// Suppress the local head in the engine's newly allocated GPU skin palette.
// No LocalAtoms, SpaceBases, physics data, bone lengths or shared asset is set.
// Mask the queued payload on its actual render-thread consumer, before it is
// installed or uploaded. Compiler-inlined command flushes are irrelevant.
#include "first_person_head_render_certificate.h"
#include "first_person_body_visibility.h"
#include "first_person_head_section.h"
#include "first_person_head_section_certificate.h"
namespace first_person_head_render_live {
bool enabled{},reserved{};
std::array<void*,4> targets{},originals{};
#if defined(GEARS_FULL_BODY_C0_NATIVE_BACKEND) && GEARS_FULL_BODY_C0_NATIVE_BACKEND && defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
namespace phase=cover_pose_phase_live;
namespace binding=weapon_hand_scene_binding;
namespace bone=gears_full_body_c0;
namespace raw=gears_cover_pose_phase;
using Update=uintptr_t(__fastcall*)(uintptr_t,int,uintptr_t,uintptr_t,unsigned);
using Upload=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
// Opt-in, bounded observations on the EXISTING consumer. Selection is the
// latest Update, not proof of which Update produced a queued palette. These
// values never supply a pose, write permit, or fallback to the feature.
//49384: at the bad magazine view, all24 sampled CPU right-hand poses are
// closed while the actual mirror shows an open hand. Include the actual
// right finger palette entries in this EXISTING opt-in/bounded observer so
// a current SpaceBases pose cannot be mistaken for the queued skin pose.
// No palette write, source timestamp, hook, grant or C0 transaction changes.
constexpr std::array<unsigned,26> trace_bones{{5,8,9,10,11,32,33,34,35,
    36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,54,55}};
struct PoseObservation {
    std::array<raw::Matrix,trace_bones.size()> space{},reference{};
    raw::Matrix component{};
    uint64_t epochs{};
    int64_t at_ns{};
    bool valid{};
};
struct Context {
    unsigned trace_observation_index{UINT32_MAX};
    phase::Owner owner{};
    uintptr_t gpu{},asset{};
    raw::ArrayHeader reference{};
    raw::ArrayHeader space{},local{},inverse{};
    std::array<int,bone::bones> parents{};
    bone::Vec reference_neck{};
    bone::Vec reference_left_shoulder{};
    std::array<bone::Vec,2> reference_shoulder_armor{};
    raw::Matrix reference_wrist{},completed_wrist{};
    int64_t completed_until_ns{};
    bool completed_hand{},hide_left{};
    unsigned render_observation_reason{},endpoint_read_reply{};
    uint64_t completed_serial{},completed_compose{},completed_input{};
    full_body_c0_native_live::ComposeObservation native_compose{};
    int64_t pickup_until_ns{};bool original_pickup{};
    PoseObservation observation{};
    LONGLONG update_qpc{};
    bool valid{};
};
std::mutex context_mutex;
Context selected{};
Context masked_context{};
// Camera admission is based on this actual, identity-checked head mask.
// A failed hand composition remains a hand failure; it must not stop XR or
// choose third person. This receipt grants no hand/action/pose permission.
struct VisualReceipt {uintptr_t pawn{};uint64_t world{};bool okay{};};
VisualReceipt visual_receipt{};
std::atomic<uint64_t> uncomposed_palettes{},original_pickup_palettes{};
struct MaskAdmissionFailure {std::atomic<int64_t> at_ns{};std::atomic<unsigned> reason{};};
std::array<MaskAdmissionFailure,32> mask_admission_failures{};std::atomic<uint64_t> mask_admission_failure_count{};
std::array<std::atomic<uint64_t>,8> mask_admission_rejected{};
bool visual_current(uintptr_t pawn) noexcept {
    if(!enabled||!full_body_c0_native_live::enabled||!full_body_c0_native_live::hands_enabled||!body_tick_source_live::enabled)return true;
    const auto reject=[](unsigned reason){
        ++mask_admission_rejected[reason];
        const auto i=mask_admission_failure_count.fetch_add(1,std::memory_order_relaxed);
        if(i<mask_admission_failures.size()){
            mask_admission_failures[i].reason.store(reason,std::memory_order_relaxed);
            mask_admission_failures[i].at_ns.store(rig_tracking_live::clock_ns(),std::memory_order_release);
        }
        return false;
    };
    Context c{};
    {
        // Every holder copies local POD state only; no engine calls, other
        // locks or IO occur under this mutex. A brief concurrent mask publish
        // must not itself turn a valid first-person view into a refusal.
        std::lock_guard<std::mutex> lock(context_mutex);
        if(!visual_receipt.okay||visual_receipt.pawn!=pawn||
           visual_receipt.world!=committed_world_generation.load())return reject(1);
        c=masked_context;
    }
    // This checks CURRENT rendered head suppression, not an old hand pose.
    // A still-installed masked GPU palette does not become unmasked merely
    // because its allocation is older than100ms. Reobserve it now, bracket
    // its actual pointer/header and local owner, and retain all input deadlines.
    const auto begin=rig_tracking_live::clock_ns();
    if(!recording.load()||phase::stopping.load()||GetCurrentThreadId()!=c.owner.thread||
       c.owner.key.session!=uint64_t(session_started_qpc)||c.owner.key.pawn.address!=pawn||
       c.owner.key.world_generation!=committed_world_generation.load()||
       !binding::current_local_visual_model(c.owner.binding))return reject(2);
    uintptr_t table{},consumer{},asset{},data{},type{},closing_data{};raw::ArrayHeader reference{},palette{},closing{};
    if(!binding::read(table,c.gpu)||!binding::field(consumer,table,56)||consumer!=base+0x19f860||
       !binding::field(asset,c.owner.key.mesh.address,0x218)||asset!=c.asset||
       !binding::field(reference,asset,0xa4)||!raw::header_equal(reference,c.reference)||
       !binding::field(data,c.gpu,0x50)||!data||data%8||!binding::read(type,data)||
       type!=base+gears_first_person_head_render_certificate::palette_vtable_rva||
       !binding::field(palette,data,8)||palette.count!=int(bone::bones)||!raw::header_valid(palette,sizeof(raw::Matrix)))return reject(3);
    std::array<raw::Matrix,bone::bones> actual{};
    if(!safe_copy(actual.data(),reinterpret_cast<void const*>(palette.data),sizeof actual))return reject(4);
    const auto& neck=actual[gears_first_person_visibility::neck_bone].values;unsigned hidden{};
    for(unsigned i=0;i<bone::bones;++i)if(gears_first_person_visibility::neck_descendant(c.parents,i)){
        const auto& m=actual[i].values;
        for(unsigned k=0;k<12;++k)if(m[k]!=0.f)return reject(5);
        if(m[15]!=1.f)return reject(5);
        for(unsigned k=12;k<15;++k)if(!std::isfinite(m[k])||m[k]!=neck[k])return reject(5);
        ++hidden;
    }
    if(hidden<2||!binding::field(closing_data,c.gpu,0x50)||closing_data!=data||
       !binding::field(closing,data,8)||!raw::header_equal(palette,closing)||
       !binding::current_local_visual_model(c.owner.binding))return reject(6);
    const auto end=rig_tracking_live::clock_ns();
    return (begin>0&&end>=begin&&end-begin<=100000000&&recording.load()&&!phase::stopping.load()&&
        c.owner.key.world_generation==committed_world_generation.load())||reject(7);
}
std::atomic<uintptr_t> selected_gpu{};
std::atomic<unsigned long long> updates{},local_updates{},palettes{},hidden_bones{},rejected{},write_failures{};
std::atomic<uint64_t> shoulder_armor_palettes{};
std::atomic<unsigned> last_reject{};
std::atomic<uintptr_t> last_data_type{};
struct SelectionFailure {int64_t at_ns{},scene_qpc{};uintptr_t gpu{},mesh{};unsigned reason{};};
std::array<SelectionFailure,64> selection_failures{};unsigned selection_failure_count{};
std::atomic<bool> selection_exported{};
struct PaletteObservation {
    int64_t at_ns{},selected_ns{};uintptr_t gpu{},data{},mesh{};
    uint64_t epochs{};uint32_t thread{};bool source_valid{};
    unsigned render_observation_reason{},endpoint_read_reply{};bool completed_hand{},composed{};
    uint64_t completed_serial{},completed_compose{},completed_input{};
    full_body_c0_native_live::ComposeObservation native_compose{};
    raw::Matrix component{};
    std::array<raw::Matrix,trace_bones.size()> source{},rendered{};
    std::array<gears_first_person_head_section::Matrix,trace_bones.size()> shader{};
    std::array<unsigned,trace_bones.size()> shader_chunk{},shader_slot{};
    uint32_t shader_valid_mask{};
};
// A full coherent recorded action flow extends past the old first1024 frames.
// Keep the existing opt-in recorder bounded while retaining its later reload
// and pose-reproduction evidence; this buffer supplies no feature permission.
std::array<PaletteObservation,8192> palette_observations{};
std::atomic<unsigned> palette_observation_count{};
std::atomic<bool> palette_observations_exported{};
void reject(unsigned reason){++rejected;last_reject=reason;}
struct Call {Call(){++active;}~Call(){--active;}};
bool writable_store(uintptr_t at,void const* data,size_t size) noexcept {
    MEMORY_BASIC_INFORMATION m{};
    if(!at||!size||at>UINTPTR_MAX-size||VirtualQuery(reinterpret_cast<void*>(at),&m,sizeof m)!=sizeof m||
       m.State!=MEM_COMMIT||m.Protect&(PAGE_GUARD|PAGE_NOACCESS)||
       !(m.Protect&(PAGE_READWRITE|PAGE_WRITECOPY))||
       uintptr_t(m.BaseAddress)>at||m.RegionSize<size||at-uintptr_t(m.BaseAddress)>m.RegionSize-size)return false;
    __try {std::memcpy(reinterpret_cast<void*>(at),data,size);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool select(uintptr_t gpu,uintptr_t mesh,Context& c){
    // The cached Scene identifies the owner; it is not this Update's local
    // model observation. Keep its original key/time, and independently read
    // the actual possession/model now. A slow Scene must not reveal the head.
    const auto observed_begin=phase::qpc();
    const auto observed_begin_ns=rig_tracking_live::clock_ns();
    const auto fail=[&](unsigned reason){
        // Observe only a known local GPU object's failed selection. No writes,
        // retained authorization or log IO are added to the rendering path.
        if(rig_tracking_live::metadata_reject_trace&&selected_gpu.load()==gpu&&
           selection_failure_count<selection_failures.size())
            selection_failures[selection_failure_count++]={rig_tracking_live::clock_ns(),c.owner.key.scene_qpc,gpu,mesh,reason};
        return false;
    };
    if(!enabled||!recording.load()||phase::stopping.load()||!first_person_live::preview_current||
       !first_person_live::toggle.requested()||!gpu||gpu%8)return fail(1);
    c.owner=phase::owner_copy();
    if(c.owner.key.mesh.address!=mesh)return fail(2);
    const auto owner_current=[&](){
        const auto selected_owner=phase::owner_copy();
        return c.owner.valid&&c.owner.thread==GetCurrentThreadId()&&
            c.owner.key.session==uint64_t(session_started_qpc)&&
            c.owner.key.world_generation==committed_world_generation.load()&&
            selected_owner.valid&&phase::same(selected_owner.key,c.owner.key)&&
            phase::own_entries()&&binding::current_local_visual_model(c.owner.binding);
    };
    if(!owner_current())return fail(4);
    // Reconcile the sole gun visual writer with THIS Update's actual wrist,
    // after any late original animation and before taking binding::mutex.
    // The helper preserves the original source clock and cannot grant actions.
    full_body_c0_native_live::refresh_render_observation(mesh,&c.render_observation_reason);
    full_body_c0_native_live::observe_last_compose(mesh,c.native_compose);
    uintptr_t table{},consumer{};
    if(!binding::read(table,gpu)||!binding::field(consumer,table,56)||consumer!=base+0x19f860)return fail(5);
    {
    std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
    if(!lock.owns_lock()||!binding::metadata_current())return fail(6);
    auto const& cache=first_person_live::authored_cache;
    uintptr_t asset{};raw::ArrayHeader reference{};
    if(!cache.ready||cache.world!=c.owner.key.world_generation||
       !binding::field(asset,mesh,0x218)||asset!=cache.asset.address||
       !binding::field(reference,asset,0xa4)||!raw::header_equal(reference,cache.reference)||
       !binding::field(c.space,mesh,0x26c)||c.space.count!=int(bone::bones)||!raw::header_valid(c.space,64)||
       !binding::field(c.local,mesh,0x27c)||c.local.count!=int(bone::bones)||!raw::header_valid(c.local,32)||
       !binding::field(c.inverse,asset,0xe4)||c.inverse.count!=int(bone::bones)||!raw::header_valid(c.inverse,64))return fail(7);
    c.gpu=gpu;c.asset=asset;c.reference=reference;c.parents=cache.model.parents;
    c.reference_neck=bone::d::position(cache.work.reference_space[gears_first_person_visibility::neck_bone]);
    c.reference_left_shoulder=bone::d::position(cache.work.reference_space[8]);
    for(unsigned j=0;j<2;++j)c.reference_shoulder_armor[j]=bone::d::position(
        cache.work.reference_space[gears_first_person_visibility::shoulder_armor_bones[j]]);
    for(unsigned k=0;k<16;++k)c.reference_wrist.values[k]=float(cache.work.reference_space[35].m[k]);
    if(rig_tracking_live::metadata_reject_trace){
        for(unsigned j=0;j<trace_bones.size();++j)
            for(unsigned k=0;k<16;++k)c.observation.reference[j].values[k]=float(cache.work.reference_space[trace_bones[j]].m[k]);
    }
    if(full_body_c0_native_live::enabled&&full_body_c0_native_live::hands_enabled&&body_tick_source_live::enabled){
        namespace completed=full_body_c0_native_live;
        gears_committed_hand::Request request{};gears_committed_hand::Frame result{};
        request.cookie=completed::endpoint_value.parent.registration_cookie;
        request.native_started=completed::endpoint_value.parent.native_started_qpc;
        const auto reply=completed::read_endpoint(&request,&result);c.endpoint_read_reply=unsigned(reply);
        if(reply==gears_rig_tracking::Reply::accepted&&
           result.mesh.address==mesh&&result.pawn.address==c.owner.key.pawn.address&&
           result.parent.world_generation==c.owner.key.world_generation){
            std::copy(completed::endpoint_geometry.wrist.begin(),completed::endpoint_geometry.wrist.end(),c.completed_wrist.values);
            c.completed_serial=result.serial;c.completed_compose=result.compose;c.completed_input=result.controllers.sequence;
            const auto oldest=(std::min)({result.acquisition_begin_ns,result.returned_ns,
                result.stereo.received_ns,result.controllers.received_ns,result.controllers.sync_ns});
            if(oldest>0&&oldest<=INT64_MAX-100000000){
                c.completed_until_ns=oldest+100000000;c.completed_hand=true;
                const auto& left=result.controllers.hands[0];
                c.hide_left=!gears_vr::controller_pose_usable(left.grip,left.grip_flags,left.grip_active);
            }
        }
    }
    }
    if(rig_tracking_live::metadata_reject_trace){
        auto& o=c.observation;raw::ArrayHeader closing{};raw::Matrix component{};uint64_t epochs{};
        bool okay=binding::field(o.component,mesh,0xa0)&&binding::field(o.epochs,mesh,0x334);
        for(unsigned j=0;okay&&j<trace_bones.size();++j)
            okay=binding::read(o.space[j],c.space.data+trace_bones[j]*sizeof(raw::Matrix));
        o.at_ns=rig_tracking_live::clock_ns();
        o.valid=okay&&binding::field(closing,mesh,0x26c)&&raw::header_equal(c.space,closing)&&
            binding::field(component,mesh,0xa0)&&!std::memcmp(&component,&o.component,sizeof component)&&
            binding::field(epochs,mesh,0x334)&&epochs==o.epochs;
    }
    // During original pickup, no completed VR wrist is expected: original
    // animation deliberately owns the arms. Keep fresh head masking and XR
    // viewing, using this Update's actual typed owner/SM observation only.
    c.original_pickup=gears_first_person_visibility::original_pickup(c.owner.binding.special_move)&&
        binding::current_actor(c.owner.binding);
    if(c.original_pickup&&observed_begin_ns>0&&observed_begin_ns<=INT64_MAX-100000000)
        c.pickup_until_ns=observed_begin_ns+100000000;
    else c.original_pickup=false;
    if(!owner_current()||!recording.load()||phase::stopping.load()||
       !first_person_live::preview_current||!first_person_live::toggle.requested())return fail(4);
    LARGE_INTEGER frequency{};const auto observed_end=phase::qpc();
    if(observed_begin<=0||!QueryPerformanceFrequency(&frequency)||frequency.QuadPart<=0||
       observed_end<observed_begin||observed_end-observed_begin>frequency.QuadPart/10)return fail(3);
    // The render consumer's existing 100ms limit starts before the real
    // identity reads, not after them. No old Scene timestamp is renewed.
    c.update_qpc=observed_begin;c.valid=true;return true;
}
void mask(uintptr_t gpu,uintptr_t data,Context* completed=nullptr){
    if(!enabled||!recording.load()||phase::stopping.load()||selected_gpu.load(std::memory_order_acquire)!=gpu)return;
    Context local{};
    {std::lock_guard<std::mutex> lock(context_mutex);local=selected;}
    auto const* c=&local;LARGE_INTEGER frequency{};const auto tick=phase::qpc();
    // Keep the last successful association while preparing this replacement.
    // visual_current reads the ACTUAL installed palette, so an unmasked or
    // replaced payload still fails; a not-yet-installed candidate must not
    // invalidate a different, still-installed and correctly masked payload.
    if(!c->valid||c->gpu!=gpu||c->owner.key.world_generation!=committed_world_generation.load()||
       c->owner.key.session!=uint64_t(session_started_qpc)||!QueryPerformanceFrequency(&frequency)||frequency.QuadPart<=0||
       tick<c->update_qpc||tick-c->update_qpc>frequency.QuadPart/10)return;
    uintptr_t data_vtable{},previous{};
    if(!data||data%8){reject(11);return;}
    if(!binding::read(data_vtable,data)){reject(12);return;}
    last_data_type=data_vtable>=base?data_vtable-base:data_vtable;
    if(data_vtable!=base+gears_first_person_head_render_certificate::palette_vtable_rva){reject(13);return;}
    if(!binding::field(previous,gpu,0x50)){reject(14);return;}
    if(previous==data){reject(15);return;}
    raw::ArrayHeader palette{};
    if(!binding::field(palette,data,8)||palette.count!=int(bone::bones)||!raw::header_valid(palette,sizeof(raw::Matrix))){reject(2);return;}
    using bone::d::range;
    const bone::d::Range buffers[]={range(reinterpret_cast<void const*>(palette.data),bone::bones*64),
        range(reinterpret_cast<void const*>(c->space.data),bone::bones*64),
        range(reinterpret_cast<void const*>(c->local.data),bone::bones*32),
        range(reinterpret_cast<void const*>(c->inverse.data),bone::bones*64),
        range(reinterpret_cast<void const*>(c->reference.data),bone::bones*68)};
    if(!bone::d::disjoint(buffers,5)){reject(3);return;}
    std::array<raw::Matrix,bone::bones> before{},after{};
    if(!safe_copy(before.data(),reinterpret_cast<void const*>(palette.data),sizeof before)){reject(4);return;}
    bool composed=c->completed_hand&&rig_tracking_live::clock_ns()<=c->completed_until_ns;
    if(composed){
        // Reconstruct the ACTUAL queued skin wrist, not the requested target.
        // Matching the latest Update is insufficient if C0 failed or another
        // animation pass replaced the completed pose before palette creation.
        composed=gears_first_person_visibility::matches_completed_wrist(c->reference_wrist,before[35],c->completed_wrist);
    }
    if(!composed)++uncomposed_palettes;
    if(rig_tracking_live::metadata_reject_trace){
        const auto index=palette_observation_count.fetch_add(1,std::memory_order_relaxed);
        if(index<palette_observations.size()){
            local.trace_observation_index=index;
            auto& row=palette_observations[index];row.at_ns=rig_tracking_live::clock_ns();
            row.selected_ns=c->observation.at_ns;row.gpu=gpu;row.data=data;row.mesh=c->owner.key.mesh.address;
            row.epochs=c->observation.epochs;row.thread=GetCurrentThreadId();row.source_valid=c->observation.valid;
            row.render_observation_reason=c->render_observation_reason;row.endpoint_read_reply=c->endpoint_read_reply;
            row.completed_hand=c->completed_hand;row.composed=composed;
            row.completed_serial=c->completed_serial;row.completed_compose=c->completed_compose;row.completed_input=c->completed_input;
            row.native_compose=c->native_compose;
            row.component=c->observation.component;row.source=c->observation.space;
            for(unsigned j=0;j<trace_bones.size();++j){
                const auto& a=c->observation.reference[j].values;const auto& b=before[trace_bones[j]].values;
                for(unsigned r=0;r<4;++r)for(unsigned k=0;k<4;++k){
                    double value=0;for(unsigned n=0;n<4;++n)value+=double(a[r*4+n])*b[n*4+k];
                    row.rendered[j].values[r*4+k]=float(value);
                }
            }
        }
    }
    // Only required LOD bones are initialized by the engine. Preserve every
    // unrelated entry byte-for-byte; it need not contain a finite transform.
    unsigned count{};
    if(!gears_first_person_visibility::mask(c->parents,c->reference_neck,before,after,count)){reject(5);return;}
    // Optional render-only cleanup. An unused/uninitialized LOD armor entry
    // leaves the pair untouched; it cannot refuse an otherwise masked head or
    // turn a first-person camera into third person.
    const bool armor_hidden=gears_first_person_visibility::hide_shoulder_armor(
        c->parents,c->reference_shoulder_armor,before,after);
    if(composed&&c->hide_left){unsigned left_count{};
        if(!gears_first_person_visibility::hide_untracked_left(c->parents,c->reference_left_shoulder,before,after,left_count)){reject(5);return;}
        count+=left_count;
    }
    raw::ArrayHeader closing{};
    if(!count||!binding::field(closing,data,8)||!raw::header_equal(palette,closing)||
       !recording.load()||selected_gpu.load()!=gpu||c->owner.key.world_generation!=committed_world_generation.load()){reject(6);return;}
    if(!writable_store(palette.data,after.data(),sizeof after)){
        ++write_failures;
        if(!writable_store(palette.data,before.data(),sizeof before))request_session_stop(SessionStop::proof_failure);
        return;
    }
    ++palettes;hidden_bones+=count;
    if(armor_hidden)++shoulder_armor_palettes;
    const bool pickup=c->original_pickup&&rig_tracking_live::clock_ns()<=c->pickup_until_ns;
    if(pickup)++original_pickup_palettes;
    {std::lock_guard<std::mutex> lock(context_mutex);
        masked_context=*c;
        visual_receipt={c->owner.key.pawn.address,c->owner.key.world_generation,true};}
    if(completed)*completed=*c;
}
#include "first_person_head_section_live.inl"
uintptr_t __fastcall hook_update(uintptr_t gpu,int lod,uintptr_t mesh,uintptr_t morphs,unsigned flags){
    Call call;++updates;Context local{};
    {std::lock_guard<std::mutex> lock(context_mutex);
        if(masked_context.gpu==gpu&&masked_context.owner.key.mesh.address!=mesh){masked_context={};visual_receipt={};}}
    if(select(gpu,mesh,local)){
        std::lock_guard<std::mutex> lock(context_mutex);selected=local;selected_gpu.store(gpu,std::memory_order_release);++local_updates;
    }else if(selected_gpu.load(std::memory_order_acquire)==gpu){
        std::lock_guard<std::mutex> lock(context_mutex);selected={};selected_gpu.store(0,std::memory_order_release);
    }
    lancer_handle_morph_live::Input input{};
    const auto selected_morphs=lancer_handle_morph_live::prepare(gpu,lod,mesh,morphs,input);
    const auto result=reinterpret_cast<Update>(originals[0])(gpu,lod,mesh,selected_morphs,flags);
    lancer_handle_morph_live::after_update(input);return result;
}
uintptr_t __fastcall hook_upload(uintptr_t gpu,uintptr_t data,uintptr_t r8,uintptr_t r9){
    Call call;Context completed{};
    try{mask(gpu,data,&completed);}catch(...){reject(7);}
    const auto result=reinterpret_cast<Upload>(originals[1])(gpu,data,r8,r9);
    try{head_section::after_upload(gpu,data,completed);}catch(...){head_section::reject(9);}
    lancer_handle_morph_live::after_upload(gpu,data);return result;
}
uintptr_t __fastcall hook_factory(uintptr_t gpu,int lod,int chunk){
    Call call;return lancer_handle_morph_live::factory(reinterpret_cast<lancer_handle_morph_live::Factory>(originals[2]),gpu,lod,chunk,false);
}
uintptr_t __fastcall hook_secondary_factory(uintptr_t gpu,int lod,int chunk){
    Call call;return lancer_handle_morph_live::factory(reinterpret_cast<lancer_handle_morph_live::Factory>(originals[3]),gpu,lod,chunk,true);
}
bool certified(){
    for(auto const& chunk:gears_first_person_head_render_certificate::chunks){
        std::array<unsigned char,4096> bytes{};bone::rig::Signature digest{};
        if(!chunk.size||chunk.size>bytes.size()||!safe_copy(bytes.data(),reinterpret_cast<void const*>(base+chunk.rva),chunk.size)||
           !first_person_live::authored_digest(bytes.data(),chunk.size,digest)||digest!=chunk.sha256)return false;
    }
    return true;
}
bool configure(FILE* output){
    enabled=first_person_live::enabled&&first_person_live::body_basis;
    if(!enabled)return true;
    if(!continuous_session||!stereo_xr||!anchor_observer_enabled||!phase::enabled||reserved||!certified()||
       !lancer_handle_morph_live::factory_certificate())return false;
    const std::array<void*,4> hooks{reinterpret_cast<void*>(hook_update),reinterpret_cast<void*>(hook_upload),
        reinterpret_cast<void*>(hook_factory),reinterpret_cast<void*>(hook_secondary_factory)};
    auto const& rows=gears_first_person_head_render_certificate::hooks;
    for(unsigned i=0;i<2;++i){
        targets[i]=reinterpret_cast<void*>(base+rows[i].rva);std::array<unsigned char,32> before{},after{};
        if(!safe_copy(before.data(),targets[i],before.size())||before!=rows[i].prefix||
           MH_CreateHook(targets[i],hooks[i],&originals[i])!=MH_OK||
           !safe_copy(after.data(),targets[i],after.size())||before!=after)return false;
    }
    auto const& factory_rows=gears_lancer_handle_factory_certificate::hooks;
    for(unsigned j=0;j<2;++j){const auto i=j+2;
        targets[i]=reinterpret_cast<void*>(base+factory_rows[j].rva);std::array<unsigned char,32> before{},after{};
        if(!safe_copy(before.data(),targets[i],before.size())||before!=factory_rows[j].prefix||
           MH_CreateHook(targets[i],hooks[i],&originals[i])!=MH_OK||
           !safe_copy(after.data(),targets[i],after.size())||before!=after)return false;
    }
    reserved=true;
    lancer_handle_morph_live::primary_instance_hook.store(reinterpret_cast<uintptr_t>(hook_factory),std::memory_order_release);
    lancer_handle_morph_live::secondary_instance_hook.store(reinterpret_cast<uintptr_t>(hook_secondary_factory),std::memory_order_release);
    head_section::enabled=head_section::certificate();
    lancer_handle_morph_live::factory_reserved.store(true,std::memory_order_release);
    if(output)fprintf(output,"first_person_head_render compiled=1 enabled=1 palette_only=1 local_bones_unchanged=1 head_and_neck=1\n");
    if(output)fprintf(output,"first_person_head_section enabled=%u section=2 spine_slot=11 new_hooks=0 shared_asset_write=0\n",unsigned(head_section::enabled));
    return true;
}
std::array<void*,4> detours(){return {reinterpret_cast<void*>(hook_update),reinterpret_cast<void*>(hook_upload),
    reinterpret_cast<void*>(hook_factory),reinterpret_cast<void*>(hook_secondary_factory)};}
void summary(FILE* f){
    if(f){fprintf(f,"head_section_enabled=%u head_section_attempted=%llu head_section_applied=%llu head_section_already=%llu head_section_write_failures=%llu ",
        unsigned(head_section::enabled),head_section::attempted.load(),head_section::applied.load(),head_section::already.load(),head_section::failed_writes.load());
        for(unsigned i=1;i<head_section::rejected.size();++i)fprintf(f,"head_section_reject_%u=%llu ",i,head_section::rejected[i].load());}
    if(rig_tracking_live::metadata_reject_trace&&phase::stopping.load()&&!recording.load()&&!active.load()&&!palette_observations_exported.exchange(true)){
        wchar_t path[MAX_PATH]{};
        if(directory[0]&&swprintf_s(path,L"%s\\head-palette-observations.csv",directory)>0){
            FILE* file{};if(!_wfopen_s(&file,path,L"wb")&&file){
                fputs("at_ns,selected_ns,gpu,data,mesh,epochs,thread,source_bracket_consistent,bone",file);
                for(auto const* prefix:{"component","selected_space","palette_space"})
                    for(unsigned k=0;k<16;++k)fprintf(file,",%s_%u",prefix,k);
                fputs(",render_observation_reason,endpoint_read_reply,completed_hand,composed,completed_serial,completed_compose,completed_input",file);
                fputs(",native_begin_ns,native_end_ns,native_tick,native_compose,native_selected,native_begin_result,native_apply_gate,native_commit_result,native_solved_mask,native_returned,native_original_called,native_right_target_x,native_right_target_y,native_right_target_z",file);
                fputs(",native_mount_kind,native_mount_prequery,native_mount_query,native_mount_query_code,native_mount_query_branch,native_mount_callback_status,native_mount_admission,native_hand_status",file);
                fputs(",native_consumer_result,native_finish_result,native_finish_failure,native_endpoint_publish_stage,native_endpoint_publish_flags,native_retained_failure",file);
                fputs(",native_caller,native_source_slot_active",file);
                fputs(",native_right_solved_target_x,native_right_solved_target_y,native_right_solved_target_z",file);
                fputs(",game_pid,native_started_qpc",file);
                fputs(",shader_valid,shader_chunk,shader_slot",file);
                for(unsigned k=0;k<12;++k)fprintf(file,",shader_%u",k);
                fputc('\n',file);
                const auto count=(std::min)(palette_observation_count.load(),unsigned(palette_observations.size()));
                for(unsigned i=0;i<count;++i){auto const& row=palette_observations[i];
                    for(unsigned j=0;j<trace_bones.size();++j){
                        fprintf(file,"%lld,%lld,%llx,%llx,%llx,%llu,%u,%u,%u",row.at_ns,row.selected_ns,
                            (unsigned long long)row.gpu,(unsigned long long)row.data,(unsigned long long)row.mesh,
                            (unsigned long long)row.epochs,row.thread,unsigned(row.source_valid),trace_bones[j]);
                        for(auto const* m:{&row.component,&row.source[j],&row.rendered[j]})
                            for(float value:m->values)fprintf(file,",%.9g",double(value));
                        fprintf(file,",%u,%u,%u,%u,%llu,%llu,%llu",row.render_observation_reason,row.endpoint_read_reply,
                            unsigned(row.completed_hand),unsigned(row.composed),(unsigned long long)row.completed_serial,
                            (unsigned long long)row.completed_compose,(unsigned long long)row.completed_input);
                        auto const& o=row.native_compose;
                        fprintf(file,",%lld,%lld,%llu,%llu,%u,%u,%u,%u,%u,%u,%u,%.9g,%.9g,%.9g",o.begin_ns,o.end_ns,
                            (unsigned long long)o.tick,(unsigned long long)o.compose,o.selected,o.begin_result,o.apply_gate,
                            o.commit_result,o.solved_mask,o.returned,o.original_called,o.right_target.x,o.right_target.y,o.right_target.z);
                        fprintf(file,",%u,%u,%llu,%u,%u,%u,%u,%u",o.mount_kind,o.mount_prequery,
                            (unsigned long long)o.mount_query,o.mount_query_code,o.mount_query_branch,
                            o.mount_callback_status,o.mount_admission,o.hand_status);
                        fprintf(file,",%u,%u,%u,%u,%u,%u",o.consumer_result,o.finish_result,o.finish_failure,
                            o.endpoint_publish_stage,o.endpoint_publish_flags,o.retained_failure);
                        fprintf(file,",%llu,%u",(unsigned long long)o.caller,o.source_slot_active);
                        fprintf(file,",%.9g,%.9g,%.9g",o.right_solved_target.x,o.right_solved_target.y,o.right_solved_target.z);
                        fprintf(file,",%lu,%llu",GetCurrentProcessId(),(unsigned long long)session_started_qpc);
                        fprintf(file,",%u,%u,%u",unsigned((row.shader_valid_mask>>j)&1u),row.shader_chunk[j],row.shader_slot[j]);
                        for(float value:row.shader[j].values)fprintf(file,",%.9g",double(value));
                        fputc('\n',file);
                    }
                }
                fclose(file);
            }
        }
    }
    if(rig_tracking_live::metadata_reject_trace&&phase::stopping.load()&&!recording.load()&&!active.load()&&!selection_exported.exchange(true)){
        wchar_t path[MAX_PATH]{};
        if(directory[0]&&swprintf_s(path,L"%s\\head-selection-failures.csv",directory)>0){
            FILE* file{};if(!_wfopen_s(&file,path,L"wb")&&file){
                fputs("at_ns,scene_qpc,gpu,mesh,reason\n",file);
                for(unsigned i=0;i<selection_failure_count;++i){auto const& r=selection_failures[i];fprintf(file,"%lld,%lld,%llx,%llx,%u\n",r.at_ns,r.scene_qpc,(unsigned long long)r.gpu,(unsigned long long)r.mesh,r.reason);}
                fclose(file);
            }
        }
    }
    if(f){fprintf(f,"first_person_head_render_enabled=%u head_render_updates=%llu head_render_local=%llu head_render_palettes=%llu head_render_bones=%llu head_render_rejected=%llu head_render_last_reject=%u head_render_data_type=%llx head_render_write_failures=%llu head_render_uncomposed_palettes=%llu head_render_shoulder_armor_palettes=%llu ",unsigned(enabled),updates.load(),local_updates.load(),palettes.load(),hidden_bones.load(),rejected.load(),last_reject.load(),static_cast<unsigned long long>(last_data_type.load()),write_failures.load(),uncomposed_palettes.load(),static_cast<unsigned long long>(shoulder_armor_palettes.load()));
        for(unsigned i=1;i<mask_admission_rejected.size();++i)fprintf(f,"head_mask_admission_reject_%u=%llu ",i,mask_admission_rejected[i].load());
        for(unsigned i=0;i<mask_admission_failures.size();++i){const auto at=mask_admission_failures[i].at_ns.load(std::memory_order_acquire);
            if(at>0)fprintf(f,"head_mask_admission_failure_%u=%lld,%u ",i,at,mask_admission_failures[i].reason.load(std::memory_order_relaxed));}
    }}
#else
bool configure(FILE*){return !(first_person_live::enabled&&first_person_live::body_basis);}std::array<void*,4> detours(){return {};}
void summary(FILE*){}
bool visual_current(uintptr_t) noexcept {return true;}
#endif
bool reserved_target(void* target){return reserved&&std::find(targets.begin(),targets.end(),target)!=targets.end();}
}
