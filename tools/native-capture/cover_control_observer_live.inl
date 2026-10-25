// Include after cover_pose_phase_live. Passive observations only; no pose writer.
#include "cover_control_code_certificate.h"
namespace cover_pose_phase_live {struct Scope;}
namespace cover_control_observer_live {
namespace phase=cover_pose_phase_live;namespace pose=gears_cover_pose_phase;
namespace certificate=gears_cover_control_certificate;namespace binding=weapon_hand_scene_binding;
bool enabled{},reserved{},certified{};std::array<void*,3> targets{},originals{};
char const* reserve_stage="idle";uintptr_t reserve_rva{};int reserve_mh_status{};
bool own_entries(); // Root owns immutable manifest/current-owned-entry attestation.
unsigned reserved_mask{};
#if defined(GEARS_COVER_CONTROL_OBSERVER) && GEARS_COVER_CONTROL_OBSERVER && defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
constexpr unsigned row_limit=128,scope_limit=8;
enum class Failure:unsigned {pending,okay,owner,caller,arguments,stack,identity,shape,read,changed,nested,concurrent,prior_compute,exception,protocol};
struct Method {uintptr_t target{};std::array<unsigned char,32> current_prefix{};};
struct NodeState {
    pose::Identity node{},root{};uintptr_t tree{},vtable{};pose::Metadata metadata{};
    std::array<unsigned char,0x88> parameters{};std::array<Method,5> methods{};
};
struct Row {
    pose::OwnerKey owner{};uint64_t compose{},outer_tick{},ordinal{},compute_ordinal{};
    LONGLONG before_qpc{},after_qpc{};uintptr_t node_argument{},mesh_argument{},out_argument{},entry_rsp{},caller{},raw_rdx{},result_rax{};
    unsigned episode{},kind{},thread{},caller_rva{},linked_row{};Failure failure{Failure::pending};
    NodeState before{},after{};pose::ArrayHeader out_before{},out_after{},affected_before{},affected_after{};
    std::array<int32_t,3> affected{};std::array<pose::Matrix,3> matrices{};
    std::array<unsigned char,16> result_xmm0{};uint32_t weight_bits{};float weight{};
    bool before_valid{},after_valid{},returned{},exception{},nested{},concurrent{},compose_returned{},compose_exception{},weight_finite{},parameters_equal{};
};
struct Header {
    char magic[8]{};unsigned version{},header_size{},row_size{},count{},capacity{};
    uint64_t session{},module_base{};unsigned pid{},pose_episode{},read_only{},atomic_verified{},writer{};
};
static_assert(std::is_trivially_copyable_v<Row>&&std::is_trivially_destructible_v<Row>&&std::is_trivially_copyable_v<Header>);
std::mutex mutex;std::array<Row,row_limit> rows{};unsigned row_count{},selected_episode{};uint64_t selected_session{},selected_compose{};
std::atomic<unsigned> in_flight{},compose_in_flight{};
std::atomic<bool> stopping{},failed{},file_attempted{},file_written{};std::atomic<DWORD> file_error{};
std::atomic<uint64_t> calls{},dropped{},rejected{},nested_calls{},unmatched{},protocol_errors{};
struct Compose {
    phase::Scope const* address{};phase::Scope scope{};uint64_t outer_tick{},ordinal{},last_ordinal{};uintptr_t last_node{};
    pose::Identity last_identity{};unsigned last_row{};bool admitted{},nested{},last_valid{},continuous{};
};
struct Call {Row row{};unsigned index{},frame{};Call* previous{};bool linked{};};
struct Thread {std::array<Compose,scope_limit> frames{};unsigned depth{},overflow{};Call* call{};};
inline thread_local Thread thread{};

template<class T> bool read(T& value,uintptr_t address){return pose::span(address,sizeof(T))&&safe_copy(&value,reinterpret_cast<void const*>(address),sizeof(T));}
bool stack_range(uintptr_t address,size_t size){ULONG_PTR low{},high{};GetCurrentThreadStackLimits(&low,&high);
    return pose::span(address,size)&&address>=low&&address<high&&size<=high-address;}
bool same_metadata(pose::Metadata const& a,pose::Metadata const& b){
    return a.asset==b.asset&&a.master==b.master&&a.physics_asset==b.physics_asset&&a.physics_instance==b.physics_instance&&
        pose::header_equal(a.reference,b.reference)&&pose::header_equal(a.local,b.local)&&pose::header_equal(a.space,b.space)&&pose::header_equal(a.desired,b.desired)&&
        !memcmp(&a.component,&b.component,sizeof(pose::Matrix));
}
bool same_node(NodeState const& a,NodeState const& b){
    if(!phase::same(a.node,b.node)||!phase::same(a.root,b.root)||a.tree!=b.tree||a.vtable!=b.vtable||!same_metadata(a.metadata,b.metadata))return false;
    for(unsigned i=0;i<5;++i)if(a.methods[i].target!=b.methods[i].target||a.methods[i].current_prefix!=b.methods[i].current_prefix)return false;return true;
}
bool episode_current(Compose const& frame){
    std::lock_guard<std::mutex> lock(phase::mutex);const auto index=frame.scope.episode;
    if(!index||index>phase::episode_count)return false;const auto& e=phase::episodes[index-1];
    const auto& request=e.rows[0].snapshot.request;
    return phase::same(e.owner,frame.scope.owner.key)&&e.thread==GetCurrentThreadId()&&!e.closed&&!e.exception&&!e.nested&&!e.concurrent&&
        e.rows[0].snapshot_valid&&pose::request_valid(request)&&request.phase==pose::Phase::C0&&request.kind==pose::Kind::compose&&
        request.invocation==frame.scope.invocation&&request.thread==frame.scope.owner.thread&&phase::same(request.owner,frame.scope.owner.key);
}
#if defined(GEARS_CONTINUOUS_HAND_OWNER) && GEARS_CONTINUOUS_HAND_OWNER
bool current(Compose const& frame){if(frame.continuous)return frame.admitted&&own_entries()&&continuous_hand_owner_live::control_current(frame.scope);return frame.admitted&&phase::thread.ticks==1&&frame.scope.owner.thread==GetCurrentThreadId()&&
#else
bool current(Compose const& frame){return frame.admitted&&phase::thread.ticks==1&&frame.scope.owner.thread==GetCurrentThreadId()&&
#endif
    own_entries()&&episode_current(frame)&&phase::current(frame.scope.owner);}
bool node_state(Compose const& frame,uintptr_t address,NodeState& state){
    if(!current(frame))return false;std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);if(!lock.owns_lock()||!binding::metadata_current())return false;
    gears_anchor::Identity node_id{},root_id{},again_id{};uintptr_t tree_again{},tree_field{},parameters{};
    if(!pose::add(frame.scope.owner.key.mesh.address,0x230,tree_field)||!pose::add(address,0x70,parameters)||
        !read(state.tree,tree_field)||!state.tree||!binding::identity(state.tree,root_id,true,true)||
        !binding::identity(address,node_id,true,true)||node_id.outer!=state.tree||!read(state.vtable,address)||!state.vtable||
        !safe_copy(state.parameters.data(),reinterpret_cast<void const*>(parameters),state.parameters.size()))return false;
    pose::Request request{};request.owner=frame.scope.owner.key;auto reader=[](void* out,void const* in,size_t n){return safe_copy(out,in,n);};
    if(!pose::metadata(reader,request,state.metadata)||!pose::header_valid(state.metadata.reference,68)||!pose::header_valid(state.metadata.local,32)||
        !pose::header_valid(state.metadata.space,64)||!pose::header_valid(state.metadata.desired,1,true)||
        state.metadata.reference.count!=state.metadata.local.count||state.metadata.reference.count!=state.metadata.space.count||!pose::finite(state.metadata.component))return false;
    constexpr unsigned slots[]={0x230,0x238,0x240,0x248,0x278};
    for(unsigned i=0;i<5;++i){uintptr_t slot{};if(!pose::add(state.vtable,slots[i],slot)||!read(state.methods[i].target,slot)||!pose::span(state.methods[i].target,32)||
        !safe_copy(state.methods[i].current_prefix.data(),reinterpret_cast<void const*>(state.methods[i].target),32))return false;}
    if(!binding::identity(address,again_id,true,true)||!binding::same(node_id,again_id)||!read(tree_again,tree_field)||tree_again!=state.tree||
        !binding::identity(state.tree,again_id,true,true)||!binding::same(root_id,again_id)||!binding::metadata_current())return false;
    state.node=phase::compact(node_id);state.root=phase::compact(root_id);return true;
}
bool ranges(Row const& row,pose::ArrayHeader const& output){
    const auto& m=row.before.metadata;const auto count=unsigned(m.reference.count);const unsigned n=row.kind?1:3;
    struct Range {uintptr_t at;size_t size;};
    const Range all[]={{row.entry_rsp+0x80,16},{row.entry_rsp+0x90,16},{output.data,n*64},{row.affected_before.data,n*4},
        {m.local.data,count*32},{m.space.data,count*64},{m.reference.data,count*68},{m.desired.data,size_t(m.desired.count)}};
    const unsigned total=m.desired.count?8:7;
    for(unsigned i=0;i<total;++i){if(!pose::span(all[i].at,all[i].size))return false;
        for(unsigned j=0;j<i;++j)if(all[i].at<=all[j].at+all[j].size-1&&all[j].at<=all[i].at+all[i].size-1)return false;}
    return true;
}
Failure before(Compose const& frame,Row& row){
    if(row.caller<=base||row.caller-base>UINT32_MAX||row.caller-base!=(row.kind==2?0x12911fu:0x1290e5u))return Failure::caller;
    row.caller_rva=unsigned(row.caller-base);
    if((row.entry_rsp&15)!=8||!stack_range(row.entry_rsp,row.kind==2?8:0xa0))return Failure::stack;
    if(!node_state(frame,row.node_argument,row.before))return Failure::identity;
    if(row.kind==2){if(!phase::same(row.before.node,frame.last_identity))return Failure::prior_compute;
        return row.before.methods[4].target==reinterpret_cast<uintptr_t>(targets[2])?Failure::okay:Failure::identity;}
    const auto n=row.kind?1u:3u;const auto end=static_cast<int32_t>(row.raw_rdx);
    if(row.mesh_argument!=frame.scope.owner.key.mesh.address||row.out_argument!=row.entry_rsp+0x80||end<0||end>=row.before.metadata.reference.count||
        row.before.methods[1].target!=reinterpret_cast<uintptr_t>(targets[row.kind]))return Failure::arguments;
    if(!read(row.out_before,row.out_argument)||!read(row.affected_before,row.entry_rsp+0x90))return Failure::read;
    if(!pose::header_valid(row.out_before,64,true)||row.out_before.count!=0||!pose::header_valid(row.affected_before,4)||row.affected_before.count!=int(n)||
        (row.affected_before.data&3)||!safe_copy(row.affected.data(),reinterpret_cast<void const*>(row.affected_before.data),n*4))return Failure::shape;
    for(unsigned i=0;i<n;++i){if(row.affected[i]<0||row.affected[i]>=row.before.metadata.reference.count)return Failure::shape;
        for(unsigned j=0;j<i;++j)if(row.affected[i]==row.affected[j])return Failure::shape;}
    return current(frame)?Failure::okay:Failure::owner;
}
Failure after(Compose const& frame,Row& row){
    if(!stack_range(row.entry_rsp,row.kind==2?8:0xa0)||!node_state(frame,row.node_argument,row.after))return Failure::identity;
    if(!same_node(row.before,row.after))return Failure::changed;row.parameters_equal=row.before.parameters==row.after.parameters;
    if(row.kind==2){row.weight_finite=std::isfinite(row.weight);return row.weight_finite&&current(frame)?Failure::okay:Failure::shape;}
    const unsigned n=row.kind?1:3;std::array<int32_t,3> affected_copy{};pose::ArrayHeader output_again{},affected_again{};
    if(!read(row.out_after,row.out_argument)||!read(row.affected_after,row.entry_rsp+0x90))return Failure::read;
    if(!pose::header_valid(row.out_after,64)||row.out_after.count!=int(n)||(row.out_after.data&3)||
        !pose::header_equal(row.affected_before,row.affected_after)||!ranges(row,row.out_after))return Failure::shape;
    if(!safe_copy(row.matrices.data(),reinterpret_cast<void const*>(row.out_after.data),n*64)||
        !safe_copy(affected_copy.data(),reinterpret_cast<void const*>(row.affected_after.data),n*4))return Failure::read;
    for(unsigned i=0;i<n;++i)if(!pose::finite(row.matrices[i])||affected_copy[i]!=row.affected[i])return Failure::shape;
    auto reader=[](void* out,void const* in,size_t bytes){return safe_copy(out,in,bytes);};NodeState closing{};
    if(!read(output_again,row.out_argument)||!read(affected_again,row.entry_rsp+0x90)||!pose::header_equal(row.out_after,output_again)||
        !pose::header_equal(row.affected_after,affected_again)||pose::repeat(reader,row.matrices.data(),row.out_after.data,n*64)!=pose::Reject::none||
        pose::repeat(reader,row.affected.data(),row.affected_after.data,n*4)!=pose::Reject::none||!node_state(frame,row.node_argument,closing)||
        !same_node(row.after,closing)||!current(frame))return Failure::changed;
    return Failure::okay;
}
void mark_rows(uint64_t invocation,bool returned,bool nested,bool concurrent=false){
    std::lock_guard<std::mutex> lock(mutex);for(unsigned i=0;i<row_count;++i)if(rows[i].compose==invocation){auto& r=rows[i];
        r.compose_returned=returned;r.compose_exception=!returned;r.nested=r.nested||nested;r.concurrent=r.concurrent||concurrent;
        if(nested||concurrent){if(r.failure==Failure::okay)r.failure=concurrent?Failure::concurrent:Failure::nested;r.after_valid=false;}}
}
void compose_begin_impl(phase::Scope const& scope){
    if(!enabled||scope.kind!=0)return;++compose_in_flight;
    if(thread.depth==scope_limit){++thread.overflow;failed=true;stopping=true;return;}
    if(thread.overflow){++thread.overflow;return;}
    auto& frame=thread.frames[thread.depth++];frame={};frame.address=&scope;frame.scope=scope;
    if(thread.depth!=1){++nested_calls;for(unsigned i=0;i+1<thread.depth;++i){thread.frames[i].nested=true;mark_rows(thread.frames[i].scope.invocation,false,true);}return;}
    if(stopping.load()||!scope.episode||phase::thread.episode!=scope.episode||!scope.arguments||phase::thread.ticks!=1||scope.owner.thread!=GetCurrentThreadId())return;
    {std::lock_guard<std::mutex> lock(mutex);if(selected_compose||(selected_episode&&(selected_episode!=scope.episode||selected_session!=scope.owner.key.session)))return;}
    {std::lock_guard<std::mutex> lock(phase::mutex);if(scope.episode>phase::episode_count)return;const auto& e=phase::episodes[scope.episode-1];
        const auto& request=e.rows[0].snapshot.request;
        if(!e.rows[0].snapshot_valid||!pose::request_valid(request)||request.phase!=pose::Phase::C0||request.kind!=pose::Kind::compose||
            request.invocation!=scope.invocation||request.thread!=scope.owner.thread||!phase::same(request.owner,scope.owner.key)||
            e.closed||e.exception||e.nested||e.concurrent||!phase::same(e.owner,scope.owner.key))return;
        frame.outer_tick=e.invocation;}
    if(!phase::current(scope.owner)||!own_entries())return;
    {std::lock_guard<std::mutex> lock(mutex);if(selected_compose||(selected_episode&&(selected_episode!=scope.episode||selected_session!=scope.owner.key.session)))return;
        selected_episode=scope.episode;selected_session=scope.owner.key.session;selected_compose=scope.invocation;}
    frame.admitted=true;
}
void compose_begin(phase::Scope const& scope) noexcept {try{compose_begin_impl(scope);}catch(...){failed=true;stopping=true;}}
namespace post_weight_scale {void on_compose_end(uint64_t,bool,bool,bool) noexcept;}
void compose_end(phase::Scope const& scope,bool returned) noexcept {
    try{if(!enabled||scope.kind!=0)return;if(thread.overflow){--thread.overflow;--compose_in_flight;return;}
        if(!thread.depth||thread.frames[thread.depth-1].address!=&scope||thread.frames[thread.depth-1].scope.invocation!=scope.invocation){++protocol_errors;failed=true;stopping=true;return;}
        const auto frame=thread.frames[thread.depth-1];bool concurrent{},nested=frame.nested;
        if(frame.admitted){{std::lock_guard<std::mutex> lock(phase::mutex);if(scope.episode&&scope.episode<=phase::episode_count){concurrent=phase::episodes[scope.episode-1].concurrent;nested=nested||phase::episodes[scope.episode-1].nested;}}
            mark_rows(scope.invocation,returned,nested,concurrent);post_weight_scale::on_compose_end(scope.invocation,returned,nested,concurrent);}
        --thread.depth;thread.frames[thread.depth]={};--compose_in_flight;
    }catch(...){failed=true;stopping=true;}
}
void begin_call(pose::CallPacket const& packet,unsigned kind,uintptr_t ra,uintptr_t entry_rsp,Call& call){
    call.previous=thread.call;call.linked=true;thread.call=&call;
    if(!enabled||stopping.load()||kind>2||!thread.depth||thread.overflow)return;
    auto& frame=thread.frames[thread.depth-1];if(!frame.admitted||thread.depth!=1)return;
    if(call.previous){call.previous->row.nested=true;++nested_calls;return;}
    auto& row=call.row;row.owner=frame.scope.owner.key;row.compose=frame.scope.invocation;row.outer_tick=frame.outer_tick;row.episode=frame.scope.episode;
    row.kind=kind;row.thread=GetCurrentThreadId();row.ordinal=++frame.ordinal;if(!row.ordinal){failed=true;stopping=true;return;}
    row.node_argument=packet.gpr[1];row.raw_rdx=packet.gpr[2];row.mesh_argument=packet.gpr[3];row.out_argument=packet.gpr[4];row.entry_rsp=entry_rsp;row.caller=ra;row.before_qpc=phase::qpc();
    {std::lock_guard<std::mutex> lock(mutex);if(row_count==row_limit){++dropped;return;}call.index=++row_count;}
    call.frame=thread.depth-1;
    if(kind==2){row.compute_ordinal=frame.last_ordinal;row.linked_row=frame.last_row;
        if(!frame.last_valid||frame.last_node!=row.node_argument){row.failure=Failure::prior_compute;return;}}
    else row.compute_ordinal=row.ordinal;
    row.failure=before(frame,row);row.before_valid=row.failure==Failure::okay;
}
void finish_call(Call& call,pose::CallPacket const& packet,bool returned){
    if(!call.index)return;auto& row=call.row;row.returned=returned;row.exception=!returned;row.after_qpc=phase::qpc();row.result_rax=packet.gpr[0];row.result_xmm0=packet.xmm[0];
    if(row.kind==2){memcpy(&row.weight_bits,packet.xmm[0].data(),4);memcpy(&row.weight,&row.weight_bits,4);}
    auto& frame=thread.frames[call.frame];if(!returned)row.failure=Failure::exception;
    else if(row.before_valid){row.failure=after(frame,row);row.after_valid=row.failure==Failure::okay;}
    {std::lock_guard<std::mutex> lock(phase::mutex);if(row.episode<=phase::episode_count)row.concurrent=phase::episodes[row.episode-1].concurrent;}
    if(row.nested||frame.nested){row.nested=true;row.after_valid=false;row.failure=Failure::nested;}
    if(row.concurrent){row.after_valid=false;row.failure=Failure::concurrent;}
    if(row.kind<2){frame.last_node=row.node_argument;frame.last_ordinal=row.ordinal;frame.last_row=call.index;frame.last_identity=row.after.node;frame.last_valid=row.after_valid&&returned;}
    if(row.failure!=Failure::okay)++rejected;
    {std::lock_guard<std::mutex> lock(mutex);rows[call.index-1]=row;}
}
#include "cover_post_weight_scale_observer.inl"
void begin_guarded(pose::CallPacket const& packet,unsigned kind,uintptr_t ra,uintptr_t rsp,Call& call) noexcept {try{begin_call(packet,kind,ra,rsp,call);}catch(...){failed=true;stopping=true;call.row.failure=Failure::protocol;}}
void finish_guarded(Call& call,pose::CallPacket const& packet,bool returned) noexcept {try{finish_call(call,packet,returned);post_weight_scale::observe(call,packet,returned);
#if defined(GEARS_COVER_NATIVE_RIG_OBSERVER) && GEARS_COVER_NATIVE_RIG_OBSERVER
    if(call.index&&call.row.kind<2)cover_native_rig_observer_live::control(call.row);
#endif
    }catch(...){failed=true;stopping=true;}}
extern "C" __declspec(noinline) void GearsCoverControlInvokeProtected(pose::CallPacket* packet,unsigned kind,uintptr_t ra,uintptr_t entry_rsp) noexcept(false){
#if defined(GEARS_CONTINUOUS_HAND_OWNER) && GEARS_CONTINUOUS_HAND_OWNER
    Call call{};gears_continuous_hand_entry::Entry entry{};bool returned{};++active;++in_flight;++calls;
#else
    Call call{};bool returned{};++active;++in_flight;++calls;
#endif
#if defined(GEARS_CONTINUOUS_HAND_OWNER) && GEARS_CONTINUOUS_HAND_OWNER
#if defined(GEARS_BODY_RECOIL_OBSERVER) && GEARS_BODY_RECOIL_OBSERVER
    body_recoil_observer_live::WeightCall recoil_call{};
#endif
    entry.kind=kind;entry.rcx=packet->gpr[1];entry.rdx=packet->gpr[2];entry.r8=packet->gpr[3];entry.r9=packet->gpr[4];entry.ra=ra;entry.rsp=entry_rsp;
#endif
#if defined(GEARS_CONTINUOUS_HAND_OWNER) && GEARS_CONTINUOUS_HAND_OWNER
    __try{continuous_hand_owner_live::control_begin(entry);
#if defined(GEARS_BODY_RECOIL_OBSERVER) && GEARS_BODY_RECOIL_OBSERVER
        body_recoil_observer_live::weight_begin(entry,recoil_call);
#endif
        begin_guarded(*packet,kind,ra,entry_rsp,call);
#else
    __try{begin_guarded(*packet,kind,ra,entry_rsp,call);
#endif
        if(kind<3&&originals[kind]){cover_body_live::GearsCoverRawInvoke(reinterpret_cast<cover_body_live::CallPacket*>(packet),originals[kind]);returned=true;}else{failed=true;stopping=true;}
#if defined(GEARS_CONTINUOUS_HAND_OWNER) && GEARS_CONTINUOUS_HAND_OWNER
    }__finally{__try{
#if defined(GEARS_BODY_RECOIL_OBSERVER) && GEARS_BODY_RECOIL_OBSERVER
        body_recoil_observer_live::weight_end(recoil_call,*packet,returned&&!AbnormalTermination());
#endif
        continuous_hand_owner_live::control_end(entry,returned&&!AbnormalTermination());finish_guarded(call,*packet,returned&&!AbnormalTermination());}
#else
    }__finally{__try{finish_guarded(call,*packet,returned&&!AbnormalTermination());}
#endif
        __finally{if(call.linked)thread.call=call.previous;--in_flight;--active;}}
}
extern "C" void GearsCoverControlLimbHook();extern "C" void GearsCoverControlSingleHook();extern "C" void GearsCoverControlWeightHook();
#if defined(GEARS_CONTINUOUS_HAND_OWNER) && GEARS_CONTINUOUS_HAND_OWNER
bool prepare(){if(!enabled&&!continuous_hand_owner_live::requested)return true;if(certified||reserved)return certified;
#else
bool prepare(){if(!enabled)return true;if(certified||reserved)return certified;
#endif
    for(auto const& c:certificate::chunks){reserve_rva=c.rva;reserve_stage="code_hash_first";
        if(!phase::digest(c)){failed=true;return false;}reserve_stage="code_hash_repeat";
        if(!phase::digest(c)){failed=true;return false;}}
    certified=true;reserve_stage="code_certified";return true;}
#if defined(GEARS_CONTINUOUS_HAND_OWNER) && GEARS_CONTINUOUS_HAND_OWNER
bool reserve(){if(!enabled&&!continuous_hand_owner_live::requested)return true;if(reserved_mask){reserve_stage="reserve_reentry";return false;}if(!prepare())return false;
#else
bool reserve(){if(!enabled)return true;if(reserved_mask){reserve_stage="reserve_reentry";return false;}if(!prepare())return false;
#endif
    const std::array<void*,3> detours{{reinterpret_cast<void*>(GearsCoverControlLimbHook),reinterpret_cast<void*>(GearsCoverControlSingleHook),reinterpret_cast<void*>(GearsCoverControlWeightHook)}};
    for(unsigned i=0;i<3;++i){targets[i]=reinterpret_cast<void*>(base+certificate::hooks[i].rva);std::array<unsigned char,32> before{},after{};
        reserve_rva=certificate::hooks[i].rva;reserve_stage="prefix_read_before";
        if(!safe_copy(before.data(),targets[i],32)){failed=true;return false;}reserve_stage="prefix_match";
        if(before!=certificate::hooks[i].prefix){failed=true;return false;}reserve_stage="minhook_create";
        reserve_mh_status=int(MH_CreateHook(targets[i],detours[i],&originals[i]));
        if(reserve_mh_status!=int(MH_OK)){failed=true;return false;}
        reserved_mask|=1u<<i;reserve_stage="prefix_read_after";
        if(!safe_copy(after.data(),targets[i],32)){failed=true;return false;}reserve_stage="prefix_unchanged";
        if(before!=after){failed=true;return false;}}
    reserved=true;reserve_stage="ready";reserve_rva=0;return true;
}
bool reserved_target(void* target){for(unsigned i=0;i<3;++i)if((reserved_mask&(1u<<i))&&targets[i]==target)return true;return false;}
bool configure(wchar_t const* ini,FILE* file){wchar_t value[16]{};GetPrivateProfileStringW(L"CoverControlObserver",L"enabled",L"0",value,16,ini);
    reserve_stage="configure_value";if(wcscmp(value,L"0")&&wcscmp(value,L"1"))return false;
    if(!wcscmp(value,L"1")){reserve_stage="configure_context";if(enabled||!phase::enabled||!continuous_session||!stereo_xr||!anchor_observer_enabled)return false;enabled=true;}
    reserve_stage=enabled?"configured":"disabled";
    if(!post_weight_scale::configure(ini,file))return false;
    if(file)fprintf(file,"cover_control_enabled=%u read_only=1 rows_max=128 first_pose_episode_only=1 atomic_verified=0 writer=0\n",unsigned(enabled));return true;
}
void request_drain(){stopping=true;}
unsigned pending_count(){return in_flight.load()+compose_in_flight.load();}
void summary(FILE* file){std::lock_guard<std::mutex> lock(mutex);if(file)fprintf(file,"cover_control_reserve_stage=%s cover_control_reserve_rva=0x%llx cover_control_reserve_mh_status=%d cover_control_enabled=%u cover_control_certified=%u cover_control_reserved_mask=%u cover_control_calls=%llu cover_control_inflight=%u cover_control_compose_inflight=%u cover_control_rows=%u cover_control_episode=%u cover_control_dropped=%llu cover_control_rejected=%llu cover_control_nested=%llu cover_control_protocol=%llu cover_control_failed=%u cover_control_file_written=%u cover_control_file_error=%lu ",reserve_stage,static_cast<unsigned long long>(reserve_rva),reserve_mh_status,unsigned(enabled),unsigned(certified),reserved_mask,calls.load(),in_flight.load(),compose_in_flight.load(),row_count,selected_episode,dropped.load(),rejected.load(),nested_calls.load(),protocol_errors.load(),unsigned(failed.load()),unsigned(file_written.load()),file_error.load());}
void print(FILE* file,bool disabled){
    post_weight_scale::print(file,disabled);
    if(enabled&&disabled&&stopping.load()&&!pending_count()&&!file_attempted.exchange(true)){wchar_t path[MAX_PATH]{};Header header{};memcpy(header.magic,"CCTRL001",8);
        header.version=1;header.header_size=sizeof(Header);header.row_size=sizeof(Row);header.count=row_count;header.capacity=row_limit;header.session=uint64_t(session_started_qpc);header.module_base=base;header.pid=GetCurrentProcessId();header.pose_episode=selected_episode;header.read_only=1;
        if(directory[0]&&swprintf_s(path,L"%s\\control-node-phases.bin",directory)>0){HANDLE out=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(out!=INVALID_HANDLE_VALUE){DWORD written{};bool okay=WriteFile(out,&header,sizeof header,&written,nullptr)&&written==sizeof header;
                const auto n=DWORD(row_count*sizeof(Row));if(okay&&n)okay=WriteFile(out,rows.data(),n,&written,nullptr)&&written==n;
                if(!okay)file_error=GetLastError();if(!CloseHandle(out)){file_error=GetLastError();okay=false;}file_written=okay;
            }else file_error=GetLastError();}else file_error=ERROR_INVALID_NAME;
    }
    summary(file);if(file)fprintf(file,"read_only=1 atomic_verified=0 writer=0\n");
}
#else
bool prepare(){return true;}bool reserve(){return true;}bool reserved_target(void*){return false;}
bool configure(wchar_t const* ini,FILE*){wchar_t value[16]{};GetPrivateProfileStringW(L"CoverControlObserver",L"enabled",L"0",value,16,ini);if(wcscmp(value,L"0"))return false;GetPrivateProfileStringW(L"CoverPostWeightScaleObserver",L"enabled",L"0",value,16,ini);return !wcscmp(value,L"0");}
void compose_begin(phase::Scope const&) noexcept {}void compose_end(phase::Scope const&,bool) noexcept {}
void request_drain(){}unsigned pending_count(){return 0;}void summary(FILE* file){if(file)fprintf(file,"cover_control_enabled=0 cover_control_compiled=0 ");}
void print(FILE* file,bool){summary(file);}
#endif
} // namespace cover_control_observer_live
