// Suppress the local head in the engine's newly allocated GPU skin palette.
// No LocalAtoms, SpaceBases, physics data, bone lengths or shared asset is set.
// Mask the queued payload on its actual render-thread consumer, before it is
// installed or uploaded. Compiler-inlined command flushes are irrelevant.
#include "first_person_head_render_certificate.h"
namespace first_person_head_render_live {
bool enabled{},reserved{};
std::array<void*,2> targets{},originals{};
#if defined(GEARS_FULL_BODY_C0_NATIVE_BACKEND) && GEARS_FULL_BODY_C0_NATIVE_BACKEND && defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
namespace phase=cover_pose_phase_live;
namespace binding=weapon_hand_scene_binding;
namespace bone=gears_full_body_c0;
namespace raw=gears_cover_pose_phase;
using Update=uintptr_t(__fastcall*)(uintptr_t,int,uintptr_t,uintptr_t,unsigned);
using Upload=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
struct Context {
    phase::Owner owner{};
    uintptr_t gpu{},asset{};
    raw::ArrayHeader reference{};
    raw::ArrayHeader space{},local{},inverse{};
    std::array<int,bone::bones> parents{};
    bone::Vec reference_head{};
    LONGLONG update_qpc{};
    bool valid{};
};
std::mutex context_mutex;
Context selected{};
std::atomic<uintptr_t> selected_gpu{};
std::atomic<unsigned long long> updates{},local_updates{},palettes{},hidden_bones{},rejected{},write_failures{};
std::atomic<unsigned> last_reject{};
std::atomic<uintptr_t> last_data_type{};
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
    if(!enabled||!recording.load()||phase::stopping.load()||!first_person_live::preview_current||
       !first_person_live::toggle.requested()||!gpu||gpu%8)return false;
    c.owner=phase::owner_copy();
    if(c.owner.key.mesh.address!=mesh||!phase::current(c.owner))return false;
    uintptr_t table{},consumer{};
    if(!binding::read(table,gpu)||!binding::field(consumer,table,56)||consumer!=base+0x19f860)return false;
    std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
    if(!lock.owns_lock()||!binding::metadata_current())return false;
    auto const& cache=first_person_live::authored_cache;
    uintptr_t asset{};raw::ArrayHeader reference{};
    if(!cache.ready||cache.world!=c.owner.key.world_generation||
       !binding::field(asset,mesh,0x218)||asset!=cache.asset.address||
       !binding::field(reference,asset,0xa4)||!raw::header_equal(reference,cache.reference)||
       !binding::field(c.space,mesh,0x26c)||c.space.count!=int(bone::bones)||!raw::header_valid(c.space,64)||
       !binding::field(c.local,mesh,0x27c)||c.local.count!=int(bone::bones)||!raw::header_valid(c.local,32)||
       !binding::field(c.inverse,asset,0xe4)||c.inverse.count!=int(bone::bones)||!raw::header_valid(c.inverse,64))return false;
    c.gpu=gpu;c.asset=asset;c.reference=reference;c.parents=cache.model.parents;
    c.reference_head=cache.marker.reference_head_model().position;c.update_qpc=phase::qpc();c.valid=true;return true;
}
bool head_descendant(Context const& c,unsigned index){
    for(unsigned i=0;i<bone::bones;++i){
        if(index==gears_body_head_authored::head_bone)return true;
        if(!index||index>=bone::bones||c.parents[index]<0||unsigned(c.parents[index])>=index)return false;
        index=unsigned(c.parents[index]);
    }
    return false;
}
void mask(uintptr_t gpu,uintptr_t data){
    if(!enabled||!recording.load()||phase::stopping.load()||selected_gpu.load(std::memory_order_acquire)!=gpu)return;
    Context local{};
    {std::unique_lock<std::mutex> lock(context_mutex,std::try_to_lock);if(!lock.owns_lock())return;local=selected;}
    auto const* c=&local;LARGE_INTEGER frequency{};const auto tick=phase::qpc();
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
    // Only required LOD bones are initialized by the engine. Preserve every
    // unrelated entry byte-for-byte; it need not contain a finite transform.
    if(!raw::finite(before[gears_body_head_authored::head_bone])){reject(5);return;}
    after=before;auto const& h=before[gears_body_head_authored::head_bone].values;auto const& p=c->reference_head;
    const bone::Vec centre{p.x*h[0]+p.y*h[4]+p.z*h[8]+h[12],p.x*h[1]+p.y*h[5]+p.z*h[9]+h[13],p.x*h[2]+p.y*h[6]+p.z*h[10]+h[14]};
    if(!bone::d::finite_vector(centre)){reject(5);return;}
    unsigned count{};
    for(unsigned i=0;i<bone::bones;++i){
        if(head_descendant(*c,i)){
            auto& m=after[i].values;std::fill(std::begin(m),std::end(m),0.f);
            m[12]=float(centre.x);m[13]=float(centre.y);m[14]=float(centre.z);m[15]=1.f;++count;
        }
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
}
uintptr_t __fastcall hook_update(uintptr_t gpu,int lod,uintptr_t mesh,uintptr_t morphs,unsigned flags){
    Call call;++updates;Context local{};
    if(select(gpu,mesh,local)){
        std::lock_guard<std::mutex> lock(context_mutex);selected=local;selected_gpu.store(gpu,std::memory_order_release);++local_updates;
    }else if(selected_gpu.load(std::memory_order_acquire)==gpu){
        std::lock_guard<std::mutex> lock(context_mutex);selected={};selected_gpu.store(0,std::memory_order_release);
    }
    return reinterpret_cast<Update>(originals[0])(gpu,lod,mesh,morphs,flags);
}
uintptr_t __fastcall hook_upload(uintptr_t gpu,uintptr_t data,uintptr_t r8,uintptr_t r9){
    Call call;
    try{mask(gpu,data);}catch(...){reject(7);}
    return reinterpret_cast<Upload>(originals[1])(gpu,data,r8,r9);
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
    if(!continuous_session||!stereo_xr||!anchor_observer_enabled||!phase::enabled||reserved||!certified())return false;
    const std::array<void*,2> hooks{reinterpret_cast<void*>(hook_update),reinterpret_cast<void*>(hook_upload)};
    auto const& rows=gears_first_person_head_render_certificate::hooks;
    for(unsigned i=0;i<2;++i){
        targets[i]=reinterpret_cast<void*>(base+rows[i].rva);std::array<unsigned char,32> before{},after{};
        if(!safe_copy(before.data(),targets[i],before.size())||before!=rows[i].prefix||
           MH_CreateHook(targets[i],hooks[i],&originals[i])!=MH_OK||
           !safe_copy(after.data(),targets[i],after.size())||before!=after)return false;
    }
    reserved=true;
    if(output)fprintf(output,"first_person_head_render compiled=1 enabled=1 palette_only=1 local_bones_unchanged=1\n");
    return true;
}
std::array<void*,2> detours(){return {reinterpret_cast<void*>(hook_update),reinterpret_cast<void*>(hook_upload)};}
void summary(FILE* f){if(f)fprintf(f,"first_person_head_render_enabled=%u head_render_updates=%llu head_render_local=%llu head_render_palettes=%llu head_render_bones=%llu head_render_rejected=%llu head_render_last_reject=%u head_render_data_type=%llx head_render_write_failures=%llu ",unsigned(enabled),updates.load(),local_updates.load(),palettes.load(),hidden_bones.load(),rejected.load(),last_reject.load(),static_cast<unsigned long long>(last_data_type.load()),write_failures.load());}
#else
bool configure(FILE*){return !(first_person_live::enabled&&first_person_live::body_basis);}std::array<void*,2> detours(){return {};}
void summary(FILE*){}
#endif
bool reserved_target(void* target){return reserved&&(targets[0]==target||targets[1]==target);}
}
