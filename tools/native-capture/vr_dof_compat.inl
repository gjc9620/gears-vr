#include "vr_dof_compat.h"
namespace vr_dof_compat {
using namespace gears_vr_dof;
struct Consume {Key key{},previous{};};
#if defined(GEARS_VR_DOF_COMPAT) && GEARS_VR_DOF_COMPAT
using Fn=uint64_t(__fastcall*)(uintptr_t,const uint16_t*,const void*,unsigned);
Fn original{};void* target{};bool attested{};thread_local Key active_key{};
struct View {
    uintptr_t renderer{},context{},current{},views{};int count{};unsigned eye{},mask{},thread{};bool valid{};
    Float4 source{};std::array<int32_t,4> target{};
    bool operator==(View const& b)const{return renderer==b.renderer&&context==b.context&&current==b.current&&views==b.views&&count==b.count&&
        eye==b.eye&&mask==b.mask&&thread==b.thread&&valid==b.valid&&equal(source,b.source)&&target==b.target;}
};
struct Path {
    uintptr_t root{},slot{},context{},table{},bank{},data{},address{};unsigned char active{};uint32_t highwater{};Float4 value{};bool valid{};
    bool same(Path const& b)const{return valid&&b.valid&&root==b.root&&slot==b.slot&&context==b.context&&table==b.table&&bank==b.bank&&data==b.data&&address==b.address&&active==b.active;}
};
struct Eye {
    uintptr_t descriptor_address{},owner{},quad_caller{};Descriptor descriptor{};Float4 source{},forwarded{};View before{},after{};Path post{};
};
struct Proof {uintptr_t owner{};std::array<Eye,2> eyes{};};
Store<Proof> store;
std::array<uint64_t,2> modified{},already_zero{},setter_proved{},quad_proved{};
std::array<std::array<uint64_t,2>,2> quad_site_calls{},quad_site_proved{};
std::array<uint64_t,static_cast<unsigned>(Decision::count)> decisions{};
uint64_t capture_rejected{},site_calls{};
struct Receipt {bool valid{},complete{},site{};uint64_t ticket{},world{};unsigned decision{},failure{},setters{},quads{};
    std::array<uintptr_t,2> quad_callers{};};
Receipt last_allowed{},last_rejected{};
template<class T> bool read(T& out,uintptr_t address,uintptr_t offset=0){
    return address&&offset<=UINTPTR_MAX-address&&sizeof(T)-1<=UINTPTR_MAX-(address+offset)&&safe_copy(&out,reinterpret_cast<void*>(address+offset),sizeof(T));
}
View view(){
    View out;out.thread=GetCurrentThreadId();out.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);uintptr_t slot{},root=__readgsqword(0x58);
    if(read(slot,root)&&slot&&read(out.context,slot,0x20)&&out.context&&read(out.current,out.context,0x14c0))out.mask|=1;
    if(read(out.views,out.renderer,0x68))out.mask|=2;if(read(out.count,out.renderer,0x70))out.mask|=4;
    if(out.mask==7&&native_eye(out.current,out.views,out.count,out.eye)&&read(out.source,out.current,0x48)&&finite(out.source)&&read(out.target,out.current,0x58))out.valid=true;
    return out;
}
Path destination(){
    Path out;out.root=__readgsqword(0x58);
    if(!read(out.slot,out.root)||!out.slot||!read(out.active,out.slot,0x10)||!out.active||!read(out.context,out.slot,0x20)||!out.context||
       !read(out.table,out.context,0x13bc)||!out.table||!read(out.bank,out.table)||!out.bank||!read(out.data,out.bank,0x3c)||!out.data||
       !read(out.highwater,out.bank,0x44)||out.data>UINTPTR_MAX-207)return out;
    out.address=out.data+192;out.valid=read(out.value,out.address);return out; // Bank 0, exact 16 bytes; highwater is not capacity.
}
bool runtime_size_transport(){
#if defined(GEARS_RUNTIME_RENDER_SIZE) && GEARS_RUNTIME_RENDER_SIZE
    return gears_native_render_size::loaded&&gears_native_render_size::width()&&gears_native_render_size::height();
#else
    return false;
#endif
}
bool supported_transport(){return gears_vr_dof::transport_supported(owned_xr_probe,owned_bindings_enabled,runtime_size_transport());}
bool other_controls(){return !supported_transport()||quad_source_control_enabled()||first_view_mask_control||submit_duplicate_control||
    submit_mono_control||submit_mono_independent_state||projection_control||origin_trial_enabled;}
bool attest(FILE* file){
    const bool configuration=!self_testing&&continuous_session&&stereo_xr&&stereo_gpu_publish&&submit_pair_enabled&&frame_link_diagnostics&&viewport_diagnostics&&!other_controls();
    attested=configuration&&final_transfer_live::verify_range(file,0x1c4b90,200,"10b2617802b5648fea992132f8b9e20d68d58940de7e7779f34298b8be240cb0")&&
        final_transfer_live::verify_range(file,0xd80360,0x71b,"e7bdb0107d1243e27ccdfc2c24e2a477b209ca1e31c3a78ec9e3489ee5423997");
    target=reinterpret_cast<void*>(base+0x1c4b90);
    fprintf(file,"vr_dof_identity enabled=1 configuration=%u runtime_ranges_match=%u setter_rva=1c4b90 setter_size=200 caller_rva=d80360 caller_size=1819 historical_code_sha256=911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6 runtime_owned_transport=%u\n",unsigned(configuration),unsigned(attested),unsigned(owned_xr_probe&&runtime_size_transport()));return attested;
}
bool current(Key key){return attested&&recording.load()&&!other_controls()&&key&&active_key==key&&consuming_renderer&&consuming_frozen_frame.enabled&&
    consuming_frame_ticket==key.id&&consuming_frozen_frame.id==key.id&&consuming_frozen_frame.world_generation==key.world&&committed_world_generation.load()==key.world;}
Consume begin(gears_submitted::FrameTicket const& frame,bool renderer){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);Consume token{{},active_key};active_key={};
    if(attested&&recording.load()&&renderer&&frame.enabled&&frame.id&&frame.id==consuming_frame_ticket&&frame.world_generation&&frame.world_generation==committed_world_generation.load()){
        token.key={frame.world_generation,frame.id};active_key=token.key;store.begin(token.key);
    }
    return token;
}
void finish(Consume const& token){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);store.finish(token.key);active_key=token.previous;}
uint64_t __fastcall hook(uintptr_t rcx,const uint16_t* descriptor,const void* data,unsigned index){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-base;Scope scope;
    if(caller!=0xd803bf)return original(rcx,descriptor,data,index);
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!active_key)return original(rcx,descriptor,data,index);const auto key=active_key;
    auto* entry=store.participate(key);++site_calls; // Participation precedes every payload/view/descriptor check.
    if(!entry)return original(rcx,descriptor,data,index);
    if(!scope.world()||!current(key)){entry->fail(Failure::identity_changed);return original(rcx,descriptor,data,index);}
    const auto before=view();if(!before.valid){entry->fail(Failure::read_failed);return original(rcx,descriptor,data,index);}
    if(!entry->reserve(before.eye,true))return original(rcx,descriptor,data,index);
    Eye value;value.before=before;value.descriptor_address=reinterpret_cast<uintptr_t>(descriptor);value.owner=descriptor_owner(value.descriptor_address);
    if(!read(value.descriptor,value.descriptor_address)||!read(value.source,reinterpret_cast<uintptr_t>(data))){entry->fail(Failure::read_failed);return original(rcx,descriptor,data,index);}
    if(!value.owner||!supported(value.descriptor,index,value.source)){entry->fail(Failure::unsupported);return original(rcx,descriptor,data,index);}
    if(entry->payload.owner&&entry->payload.owner!=value.owner){entry->fail(Failure::identity_changed);return original(rcx,descriptor,data,index);}
    const auto path=destination();if(!path.valid||path.context!=before.context){entry->fail(Failure::read_failed);return original(rcx,descriptor,data,index);}
    entry->payload.owner=value.owner;value.forwarded=gears_vr_dof::forwarded(value.source);const bool change=value.source[0]==1.0f;
    const auto result=call_setter(original,rcx,descriptor,data,index,value.forwarded,change); // One original; only the local X changes.
    value.after=view();value.post=destination();Descriptor after_descriptor{};Float4 after_source{};
    const bool proof=current(key)&&read(after_descriptor,value.descriptor_address)&&after_descriptor==value.descriptor&&
        read(after_source,reinterpret_cast<uintptr_t>(data))&&equal(after_source,value.source)&&value.before==value.after&&value.after.valid&&
        path.same(value.post)&&value.post.context==value.after.context&&finite(value.post.value)&&value.post.value[0]==0.0f&&equal(value.post.value,value.forwarded);
    entry=store.find(key);if(entry){entry->payload.eyes[before.eye]=value;entry->proved(before.eye,true,proof);if(proof)++setter_proved[before.eye];}
    if(change)++modified[before.eye];else ++already_zero[before.eye];return result;
}
void quad(uintptr_t caller,bool world){
    if(!gears_vr_dof::quad_caller(caller))return;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!active_key)return;
    const auto key=active_key;auto* entry=store.participate(key);++site_calls; // A quad without a supported setter is a participating failure.
    if(!entry)return;if(!world||!current(key)){entry->fail(Failure::identity_changed);return;}
    const auto now=view();if(!now.valid){entry->fail(Failure::read_failed);return;}
    const unsigned site=caller==0xd808d8?0u:1u;++quad_site_calls[site][now.eye];
    if(!entry->reserve(now.eye,false))return; // One shared per-eye reserve across both sites.
    const auto& setter=entry->payload.eyes[now.eye];Descriptor descriptor{};const auto path=destination();
    const bool proof=(entry->setter_mask&(1u<<now.eye))&&setter.owner&&setter.owner==entry->payload.owner&&
        read(descriptor,setter.descriptor_address)&&descriptor==setter.descriptor&&setter.after==now&&path.same(setter.post)&&
        path.context==now.context&&finite(path.value)&&path.value[0]==0.0f&&equal(path.value,setter.forwarded);
    entry->proved(now.eye,false,proof);if(proof){entry->payload.eyes[now.eye].quad_caller=caller;
        ++quad_proved[now.eye];++quad_site_proved[site][now.eye];} // No old R8 memory is read here.
}
bool publication(uint64_t world,uint64_t ticket){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);const Key key{world,ticket};const auto decision=store.decision(key);++decisions[static_cast<unsigned>(decision)];
    Receipt receipt;receipt.valid=true;receipt.world=world;receipt.ticket=ticket;receipt.decision=static_cast<unsigned>(decision);
    if(auto* entry=store.find(key)){receipt.complete=entry->complete;receipt.site=entry->site;receipt.failure=static_cast<unsigned>(entry->failure);receipt.setters=entry->setter_mask;receipt.quads=entry->quad_mask;
        for(unsigned eye=0;eye<2;++eye)receipt.quad_callers[eye]=entry->payload.eyes[eye].quad_caller;}
    (allowed(decision)?last_allowed:last_rejected)=receipt;
    if(!allowed(decision))++capture_rejected;return allowed(decision);
}
void retire(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    // Desktop Present is not the completed owned Canvas/publication boundary.
    if(owned_xr_probe&&runtime_size_transport())return;
    store.retire_window();}
bool retire_owned(uint64_t world,uint64_t ticket){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    const Key key{world,ticket};
    if(!owned_xr_probe||!runtime_size_transport()||!allowed(store.decision(key)))return false;
    return store.retire_through(ticket);
}
// Memory-only closed-world transaction, after independent resource retirement.
// No active_key access: Consume begin/finish retain ownership on their thread.
bool world_retirement_ready(uint64_t old_world,uint64_t cutoff){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    return store.world_retirement_ready(old_world,cutoff);
}
bool retire_world(uint64_t old_world,uint64_t cutoff){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    return store.retire_world(old_world,cutoff);
}
void progress(FILE* file){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);fprintf(file,"vr_dof_capture_rejected=%llu vr_dof_no_site=%llu vr_dof_zero_proved=%llu ",capture_rejected,decisions[0],decisions[1]);}
void print(FILE* file){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    fprintf(file,"vr_dof_compat enabled=1 attested=%u both_owned_eyes=1 outside_owned_unchanged=1 store_capacity=128 capacity_lost=%u overflow=%llu late=%llu invalid=%llu begun=%llu completed=%llu retired_through=%llu observed_through=%llu site_calls=%llu modified_left=%llu modified_right=%llu already_zero_left=%llu already_zero_right=%llu setter_proved_left=%llu setter_proved_right=%llu quad_proved_left=%llu quad_proved_right=%llu no_site_allowed=%llu zero_proved_allowed=%llu capture_rejected=%llu missing=%llu incomplete=%llu proof_rejected=%llu overflow_rejected=%llu no_gpu_commands=1 source_memory_written=0 old_r8_read_at_quad=0 highwater_is_capacity=0 runtime_owned_supported=1 owned_publication_proof_required=1\n",
        unsigned(attested),unsigned(store.capacity_lost),store.overflow,store.late,store.invalid,store.begins,store.finishes,store.retired,store.observed,site_calls,modified[0],modified[1],already_zero[0],already_zero[1],setter_proved[0],setter_proved[1],quad_proved[0],quad_proved[1],decisions[0],decisions[1],capture_rejected,decisions[2],decisions[3],decisions[4],decisions[5]);
    fprintf(file,"vr_dof_quad_sites site0=d808d8 calls=%llu,%llu proved=%llu,%llu site1=d80a7b calls=%llu,%llu proved=%llu,%llu eye_order=left,right call_counts_require_valid_view=1\n",
        quad_site_calls[0][0],quad_site_calls[0][1],quad_site_proved[0][0],quad_site_proved[0][1],quad_site_calls[1][0],quad_site_calls[1][1],quad_site_proved[1][0],quad_site_proved[1][1]);
    const Receipt examples[]{last_allowed,last_rejected};for(unsigned i=0;i<2;++i){auto const& r=examples[i];
        fprintf(file,"vr_dof_publication_example allowed=%u valid=%u world=%llu ticket=%llu decision=%u failure=%u complete=%u site=%u setters=%u quads=%u quad_callers=%llx,%llx\n",unsigned(i==0),unsigned(r.valid),r.world,r.ticket,r.decision,r.failure,unsigned(r.complete),unsigned(r.site),r.setters,r.quads,
            static_cast<unsigned long long>(r.quad_callers[0]),static_cast<unsigned long long>(r.quad_callers[1]));}
}
#else
bool attest(FILE*){return true;}
Consume begin(gears_submitted::FrameTicket const&,bool){return {};}
void finish(Consume const&){}void quad(uintptr_t,bool){}void retire(){}
bool publication(uint64_t,uint64_t){return true;}
bool retire_owned(uint64_t,uint64_t){return true;}
bool world_retirement_ready(uint64_t,uint64_t){return true;}
bool retire_world(uint64_t,uint64_t){return true;}
void progress(FILE*){}
void print(FILE* file){fprintf(file,"vr_dof_compat enabled=0\n");}
#endif
}
