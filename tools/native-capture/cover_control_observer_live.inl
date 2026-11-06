// Include after cover_pose_phase_live. Original-value observations plus optional
// Native-owned arm alpha arbitration; no pose/storage writer.
#include "body_c0_arm_alpha_policy.h"
#include "cover_control_code_certificate.h"
#include "cover_future_g_code_certificate.h"
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
bool retained_boundary_current()noexcept{return enabled&&certified&&!stopping.load()&&!failed.load()&&own_entries();}
std::atomic<uint64_t> calls{},dropped{},rejected{},nested_calls{},unmatched{},protocol_errors{};
bool arm_policy_enabled() noexcept {
#if defined(GEARS_FULL_BODY_C0_NATIVE_BACKEND) && GEARS_FULL_BODY_C0_NATIVE_BACKEND
    return full_body_c0_native_live::arm_control_policy_enabled();
#else
    return false;
#endif
}
std::atomic<uint64_t> arm_alpha_applied{},arm_alpha_rejected{};
// Existing summary diagnostics only: distinguish rejected transition weights
// from identity/scope failures without any new engine read or permission.
std::array<std::array<std::atomic<uint64_t>,32>,8> arm_alpha_reasons{};
std::array<std::atomic<uint64_t>,4> arm_alpha_weight_classes{};
std::atomic<uint32_t> arm_alpha_last_rejected_weight{};
std::array<std::atomic<uint64_t>,16> arm_alpha_token_compute_failures{},arm_alpha_token_weight_failures{};
std::array<std::atomic<uint64_t>,4> arm_alpha_token_roles{};
std::array<std::atomic<uint64_t>,8> arm_node_state_failures{};
struct Compose {
    phase::Scope const* address{};phase::Scope const* end_address{};uint64_t end_invocation{};phase::Scope scope{};uint64_t outer_tick{},ordinal{},last_ordinal{};uintptr_t last_node{};
    pose::Identity last_identity{};unsigned last_row{};bool admitted{},nested{},last_valid{},continuous{};
    Row live_compute{};uint64_t live_token{};bool live_armed{};
    // Last attempted compute only. Never consumed by token/permission logic.
    struct LastComputeDiagnostic {uint64_t ordinal{};uintptr_t node{},raw_rdx{};unsigned kind{},failure{};} diag_last{};

};
struct Call {Row row{};unsigned index{},frame{};Call* previous{};uint64_t arm_token{};bool linked{},live_checked{};};
struct Thread {std::array<Compose,scope_limit> frames{};unsigned depth{},overflow{};Call* call{};};
inline thread_local Thread thread{};
void clear_arm(Compose& frame) noexcept {frame.live_armed=false;frame.live_token=0;}
void clear_arms() noexcept {for(auto& frame:thread.frames)clear_arm(frame);}

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
bool current(Compose const& frame){if(frame.continuous){
    if(!frame.admitted||!own_entries()||!frame.address)return false;
#if defined(GEARS_FULL_BODY_C0_NATIVE_BACKEND) && GEARS_FULL_BODY_C0_NATIVE_BACKEND
    const auto retained=full_body_c0_native_live::retained_control_current(*frame.address,frame.outer_tick);
    if(retained!=gears_body_c0_arm_alpha::RetainedControl::not_applicable)return retained==gears_body_c0_arm_alpha::RetainedControl::verified;
#endif
    return continuous_hand_owner_live::control_current(*frame.address);
}return frame.admitted&&phase::thread.ticks==1&&frame.scope.owner.thread==GetCurrentThreadId()&&
#else
bool current(Compose const& frame){return frame.admitted&&phase::thread.ticks==1&&frame.scope.owner.thread==GetCurrentThreadId()&&
#endif
    own_entries()&&episode_current(frame)&&phase::current(frame.scope.owner);}
bool node_state(Compose const& frame,uintptr_t address,NodeState& state){
    const auto deny=[](unsigned reason){++arm_node_state_failures[reason];return false;};
    if(!current(frame))return deny(1);std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
    if(!lock.owns_lock())return deny(2);if(!binding::metadata_current())return deny(3);
    gears_anchor::Identity node_id{},root_id{},again_id{};uintptr_t tree_again{},tree_field{},parameters{};
    if(!pose::add(frame.scope.owner.key.mesh.address,0x230,tree_field)||!pose::add(address,0x70,parameters)||
        !read(state.tree,tree_field)||!state.tree||!binding::identity(state.tree,root_id,true,true)||
        !binding::identity(address,node_id,true,true)||node_id.outer!=state.tree||!read(state.vtable,address)||!state.vtable||
        !safe_copy(state.parameters.data(),reinterpret_cast<void const*>(parameters),state.parameters.size()))return deny(4);
    pose::Request request{};request.owner=frame.scope.owner.key;auto reader=[](void* out,void const* in,size_t n){return safe_copy(out,in,n);};
    if(!pose::metadata(reader,request,state.metadata)||!pose::header_valid(state.metadata.reference,68)||!pose::header_valid(state.metadata.local,32)||
        !pose::header_valid(state.metadata.space,64)||!pose::header_valid(state.metadata.desired,1,true)||
        state.metadata.reference.count!=state.metadata.local.count||state.metadata.reference.count!=state.metadata.space.count||!pose::finite(state.metadata.component))return deny(5);
    constexpr unsigned slots[]={0x230,0x238,0x240,0x248,0x278};
    for(unsigned i=0;i<5;++i){uintptr_t slot{};if(!pose::add(state.vtable,slots[i],slot)||!read(state.methods[i].target,slot)||!pose::span(state.methods[i].target,32)||
        !safe_copy(state.methods[i].current_prefix.data(),reinterpret_cast<void const*>(state.methods[i].target),32))return deny(6);}
    if(!binding::identity(address,again_id,true,true)||!binding::same(node_id,again_id)||!read(tree_again,tree_field)||tree_again!=state.tree||
        !binding::identity(state.tree,again_id,true,true)||!binding::same(root_id,again_id)||!binding::metadata_current())return deny(7);
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
    auto& frame=thread.frames[thread.depth++];frame={};frame.address=&scope;frame.end_address=&scope;frame.end_invocation=scope.invocation;frame.scope=scope;
    if(thread.depth!=1){clear_arms();++nested_calls;for(unsigned i=0;i+1<thread.depth;++i){thread.frames[i].nested=true;mark_rows(thread.frames[i].scope.invocation,false,true);}return;}
#if defined(GEARS_FULL_BODY_C0_NATIVE_BACKEND) && GEARS_FULL_BODY_C0_NATIVE_BACKEND && defined(GEARS_CONTINUOUS_HAND_OWNER) && GEARS_CONTINUOUS_HAND_OWNER
    if(arm_policy_enabled()){
        phase::Scope const* exact_scope{};uint64_t outer_tick{};
        if(stopping.load()||!full_body_c0_native_live::arm_control_scope(exact_scope,outer_tick)||!exact_scope||!outer_tick||
           exact_scope->kind!=0||!exact_scope->arguments||exact_scope->owner.thread!=GetCurrentThreadId()||
           !own_entries()||!continuous_hand_owner_live::control_current(*exact_scope))return;
        frame.address=exact_scope;frame.scope=*exact_scope;frame.outer_tick=outer_tick;frame.continuous=true;frame.admitted=true;return;
    }
#endif
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
void compose_begin(phase::Scope const& scope) noexcept {try{compose_begin_impl(scope);}catch(...){clear_arms();failed=true;stopping=true;}}
namespace post_weight_scale {void on_compose_end(uint64_t,bool,bool,bool) noexcept;}
namespace future_g {void on_compose_end(uint64_t,bool,bool,bool) noexcept;}
void compose_end(phase::Scope const& scope,bool returned) noexcept {
    try{if(!enabled||scope.kind!=0)return;if(thread.overflow){--thread.overflow;--compose_in_flight;return;}
        if(!thread.depth||thread.frames[thread.depth-1].end_address!=&scope||thread.frames[thread.depth-1].end_invocation!=scope.invocation){clear_arms();++protocol_errors;failed=true;stopping=true;return;}
        const auto frame=thread.frames[thread.depth-1];bool concurrent{},nested=frame.nested;
        if(frame.admitted){{std::lock_guard<std::mutex> lock(phase::mutex);if(scope.episode&&scope.episode<=phase::episode_count){concurrent=phase::episodes[scope.episode-1].concurrent;nested=nested||phase::episodes[scope.episode-1].nested;}}
            mark_rows(scope.invocation,returned,nested,concurrent);post_weight_scale::on_compose_end(scope.invocation,returned,nested,concurrent);future_g::on_compose_end(scope.invocation,returned,nested,concurrent);}
        --thread.depth;thread.frames[thread.depth]={};--compose_in_flight;
    }catch(...){failed=true;stopping=true;}
}
void begin_call(pose::CallPacket const& packet,unsigned kind,uintptr_t ra,uintptr_t entry_rsp,Call& call){
    call.previous=thread.call;call.linked=true;thread.call=&call;
    if(!enabled||stopping.load()||kind>2||!thread.depth||thread.overflow){clear_arms();return;}
    auto& frame=thread.frames[thread.depth-1];if(!frame.admitted||thread.depth!=1){clear_arms();return;}
    if(call.previous){clear_arms();call.previous->row.nested=true;++nested_calls;return;}
    auto& row=call.row;row.owner=frame.scope.owner.key;row.compose=frame.scope.invocation;row.outer_tick=frame.outer_tick;row.episode=frame.scope.episode;
    row.kind=kind;row.thread=GetCurrentThreadId();row.ordinal=++frame.ordinal;if(!row.ordinal){failed=true;stopping=true;return;}
    row.node_argument=packet.gpr[1];row.raw_rdx=packet.gpr[2];row.mesh_argument=packet.gpr[3];row.out_argument=packet.gpr[4];row.entry_rsp=entry_rsp;row.caller=ra;row.before_qpc=phase::qpc();
    const bool realtime=arm_policy_enabled();call.live_checked=realtime;if(!realtime)clear_arm(frame);
    call.frame=thread.depth-1;
    if(realtime){
        if(kind==2&&frame.live_armed&&frame.live_token&&row.ordinal==frame.live_token+1&&frame.live_compute.node_argument==row.node_argument)
            call.arm_token=frame.live_token;
        // Consume on entry even if this weight's before/after later rejects.
        frame.live_armed=false;if(kind!=2||!call.arm_token)clear_arm(frame);
    }
    {std::lock_guard<std::mutex> lock(mutex);if(row_count==row_limit){++dropped;if(!realtime)return;}else call.index=++row_count;}
    if(kind==2){row.compute_ordinal=frame.last_ordinal;row.linked_row=frame.last_row;
        if(!frame.last_valid||frame.last_node!=row.node_argument||(realtime&&!call.arm_token)){row.failure=Failure::prior_compute;return;}}
    else row.compute_ordinal=row.ordinal;
    row.failure=before(frame,row);row.before_valid=row.failure==Failure::okay;
}
void finish_call(Call& call,pose::CallPacket const& packet,bool returned){
    if(!call.index&&!call.live_checked)return;auto& row=call.row;row.returned=returned;row.exception=!returned;row.after_qpc=phase::qpc();row.result_rax=packet.gpr[0];row.result_xmm0=packet.xmm[0];
    if(row.kind==2){memcpy(&row.weight_bits,packet.xmm[0].data(),4);memcpy(&row.weight,&row.weight_bits,4);}
    auto& frame=thread.frames[call.frame];if(!returned)row.failure=Failure::exception;
    else if(row.before_valid){row.failure=after(frame,row);row.after_valid=row.failure==Failure::okay;}
    {std::lock_guard<std::mutex> lock(phase::mutex);if(row.episode&&row.episode<=phase::episode_count)row.concurrent=phase::episodes[row.episode-1].concurrent;}
    if(row.nested||frame.nested){row.nested=true;row.after_valid=false;row.failure=Failure::nested;}
    if(row.concurrent){row.after_valid=false;row.failure=Failure::concurrent;}
    if(row.kind<2){frame.last_node=row.node_argument;frame.last_ordinal=row.ordinal;frame.last_row=call.index;frame.last_identity=row.after.node;frame.last_valid=row.after_valid&&returned;}
    if(call.live_checked&&row.kind<2){
        clear_arm(frame);
        if(row.after_valid&&returned&&!row.nested&&!row.concurrent&&!stopping.load()&&current(frame)){
            frame.live_compute=row;frame.live_token=row.ordinal;frame.live_armed=true;}}
    if(call.live_checked&&(row.failure!=Failure::okay||stopping.load()))clear_arm(frame);
    if(row.kind<2){
        frame.diag_last={row.ordinal,row.node_argument,row.raw_rdx,row.kind,unsigned(row.failure)};
#if defined(GEARS_FULL_BODY_C0_NATIVE_BACKEND) && GEARS_FULL_BODY_C0_NATIVE_BACKEND
        gears_body_c0_arm_alpha::Diagnostic d{};d.compose=row.compose;d.outer_tick=row.outer_tick;d.compute_ordinal=row.ordinal;
        d.node_argument=row.node_argument;d.scope_address=reinterpret_cast<uintptr_t>(frame.address);d.raw_rdx=row.raw_rdx;d.qpc=row.after_qpc;d.thread=row.thread;
        d.kind=row.kind;d.end=row.raw_rdx==11?11u:row.raw_rdx==35?35u:0u;d.stage=unsigned(gears_body_c0_arm_alpha::Stage::compute);
        d.compute_failure=unsigned(row.failure);d.flags=(call.live_checked?4u:0u)|(returned?8u:0u);
        full_body_c0_native_live::arm_alpha_diagnostic(d);
#endif
    }
    if(row.failure!=Failure::okay)++rejected;
    if(call.index){std::lock_guard<std::mutex> lock(mutex);rows[call.index-1]=row;}
}
#include "cover_post_weight_scale_observer.inl"
#include "cover_future_g_observer_live.inl"
// Separate TLS scratch: observer capacity and exported row indices never grant
// or limit eligibility. The original-value observers run before this decision.
inline thread_local post_weight_scale::Workspace arm_alpha_workspace{};
void arbitrate_arm_alpha(Call const& call,pose::CallPacket& packet,bool returned){
    if(!call.live_checked||call.row.kind!=2)return;
    if(call.frame>=scope_limit){clear_arms();return;}auto& frame=thread.frames[call.frame];
    const bool token=call.arm_token&&call.arm_token==frame.live_token&&
        frame.live_compute.ordinal==call.arm_token&&call.row.ordinal==call.arm_token+1;
    gears_body_c0_arm_alpha::Diagnostic diagnostic{};
    const auto& last=frame.diag_last;const bool adjacent=last.ordinal&&last.node==call.row.node_argument&&call.row.ordinal==last.ordinal+1;
    diagnostic.compose=call.row.compose;diagnostic.outer_tick=call.row.outer_tick;diagnostic.compute_ordinal=last.ordinal;
    diagnostic.weight_ordinal=call.row.ordinal;diagnostic.token=call.arm_token;diagnostic.node_argument=call.row.node_argument;
    diagnostic.scope_address=reinterpret_cast<uintptr_t>(frame.address);diagnostic.raw_rdx=last.raw_rdx;diagnostic.qpc=call.row.after_qpc;diagnostic.thread=call.row.thread;
    diagnostic.kind=adjacent?last.kind:UINT32_MAX;diagnostic.end=adjacent&&last.kind<2?(last.raw_rdx==11?11u:last.raw_rdx==35?35u:0u):0u;
    diagnostic.compute_failure=last.failure;diagnostic.weight_failure=unsigned(call.row.failure);
    diagnostic.original_alpha_bits=call.row.weight_bits;memcpy(&diagnostic.final_alpha_bits,packet.xmm[0].data(),4);
    diagnostic.flags=(adjacent?1u:0u)|(token?2u:0u)|4u|(returned?8u:0u);
    auto event=[&](gears_body_c0_arm_alpha::Stage stage,unsigned reason=0u)noexcept{
        diagnostic.stage=unsigned(stage);diagnostic.reason=reason;
        if(reason&&unsigned(stage)<arm_alpha_reasons.size()&&reason<32){
            ++arm_alpha_reasons[unsigned(stage)][reason];
            float original{};std::memcpy(&original,&diagnostic.original_alpha_bits,4);
            ++arm_alpha_weight_classes[original==1.f?0u:original==0.f?1u:
                std::isfinite(original)&&original>0.f&&original<1.f?2u:3u];
            arm_alpha_last_rejected_weight=diagnostic.original_alpha_bits;
        }
#if defined(GEARS_FULL_BODY_C0_NATIVE_BACKEND) && GEARS_FULL_BODY_C0_NATIVE_BACKEND
        full_body_c0_native_live::arm_alpha_diagnostic(diagnostic);
#endif
    };
    event(gears_body_c0_arm_alpha::Stage::entry);
    // Clear before invoking any further validator/Native owner. This exact
    // compute->weight pair has already consumed its one opportunity.
    clear_arm(frame);
    unsigned denied{};
    if(!token)denied=1;else if(!arm_policy_enabled())denied=2;else if(stopping.load())denied=3;
    else if(failed.load())denied=4;else if(in_flight.load()!=1)denied=5;else if(!returned)denied=6;
    if(denied){
        if(denied==1){
            if(last.failure<arm_alpha_token_compute_failures.size())++arm_alpha_token_compute_failures[last.failure];
            if(unsigned(call.row.failure)<arm_alpha_token_weight_failures.size())++arm_alpha_token_weight_failures[unsigned(call.row.failure)];
            if(adjacent&&last.kind<2&&(last.raw_rdx==11||last.raw_rdx==35))++arm_alpha_token_roles[(last.raw_rdx==35?2:0)+last.kind];
        }
        event(gears_body_c0_arm_alpha::Stage::token,denied);++arm_alpha_rejected;return;}
    auto& r=arm_alpha_workspace.record;r={};r.weight=call.row;r.compute=frame.live_compute;
    r.linked_source=true;r.original_returned=returned;r.original_return_packet=packet;
    const auto sampled=post_weight_scale::sample(frame,call,r,true);
    diagnostic.read_mask=r.before.read_mask;diagnostic.read_mask_after=r.after.read_mask;
    diagnostic.cached_scale_word=r.before.has_scale_word;diagnostic.cached_scale_word_after=r.after.has_scale_word;
    diagnostic.scale_count=r.before.scale.count;diagnostic.scale_count_after=r.after.scale.count;
    diagnostic.original_alpha_bits=r.weight.weight_bits;diagnostic.compute_failure=unsigned(r.compute.failure);diagnostic.weight_failure=unsigned(r.weight.failure);
    event(gears_body_c0_arm_alpha::Stage::sample,sampled==post_weight_scale::Reject::okay?0u:unsigned(sampled));
    if(sampled!=post_weight_scale::Reject::okay){++arm_alpha_rejected;return;}
    gears_body_c0_arm_alpha::Proof proof{};proof.owner=frame.scope.owner.key;proof.compose=frame.scope.invocation;proof.outer_tick=frame.outer_tick;
    proof.token=call.arm_token;proof.compute_ordinal=r.compute.ordinal;proof.weight_ordinal=r.weight.ordinal;
    proof.thread=GetCurrentThreadId();proof.side=r.compute.raw_rdx==11?0u:1u;proof.kind=r.compute.kind;proof.affected_count=r.matrix_count;
    proof.original_alpha_bits=r.weight.weight_bits;proof.cached_scale_flag=r.after.has_scale_word;
    proof.scope_address=reinterpret_cast<uintptr_t>(frame.address);proof.node_argument=r.weight.node_argument;proof.entry_rsp=r.weight.entry_rsp;
    proof.metadata=r.node_after.metadata;proof.node=r.node_after.node;proof.root=r.node_after.root;proof.affected=r.after.bones;
    proof.output=r.after.output;proof.affected_header=r.after.affected;proof.scale=r.after.scale;proof.normal_return=returned;
#if defined(GEARS_FULL_BODY_C0_NATIVE_BACKEND) && GEARS_FULL_BODY_C0_NATIVE_BACKEND
    diagnostic.flags|=16u;const bool owned=full_body_c0_native_live::owns_arm_control(proof);if(owned)diagnostic.flags|=32u;
    unsigned post_denied{};
    if(!owned)post_denied=1;else if(stopping.load())post_denied=2;else if(failed.load())post_denied=3;
    else if(!arm_policy_enabled())post_denied=4;else if(!full_body_c0_native_live::arm_control_policy_active())post_denied=5;
    if(post_denied){event(gears_body_c0_arm_alpha::Stage::post_owner,post_denied);++arm_alpha_rejected;return;}
    // Last possible rejection precedes this single exact bit edit. No other
    // register, flag, XMM byte, node, output matrix, Local or Space is written.
    const uint32_t positive_zero=0;memcpy(packet.xmm[0].data(),&positive_zero,4);++arm_alpha_applied;
    memcpy(&diagnostic.final_alpha_bits,packet.xmm[0].data(),4);event(gears_body_c0_arm_alpha::Stage::applied);
#else
    (void)proof;++arm_alpha_rejected;
#endif
}

// Pure copies already obtained by existing callbacks. No validator is rerun.
void observe_ready_weight_return(Call const& call,pose::CallPacket const& packet,bool returned) noexcept {
#if defined(GEARS_FULL_BODY_C0_NATIVE_BACKEND) && GEARS_FULL_BODY_C0_NATIVE_BACKEND
    if(call.row.kind!=2||call.frame>=scope_limit)return;
    auto const& frame=thread.frames[call.frame];auto const& weight=call.row;auto const& compute=frame.live_compute;
    gears_body_c0_arm_alpha::WeightReturn input{};
    input.compose=weight.compose;input.outer_tick=weight.outer_tick;input.weight_ordinal=weight.ordinal;
    input.node_argument=weight.node_argument;input.mesh=weight.owner.mesh.address;
    input.scope_address=reinterpret_cast<uintptr_t>(frame.address);input.entry_rsp=weight.entry_rsp;
    input.caller=weight.caller;input.thread=weight.thread;input.original_alpha_bits=weight.weight_bits;
    uint32_t bits{};memcpy(&bits,packet.xmm[0].data(),4);input.final_alpha_bits=bits;
    input.flags=(returned?1u:0u)|(call.live_checked?2u:0u)|(call.linked?4u:0u)|
        (weight.before_valid?128u:0u)|(weight.after_valid?256u:0u)|(weight.weight_finite?512u:0u);
    auto const& last=frame.diag_last;
    if(last.ordinal&&compute.ordinal==last.ordinal&&compute.compose==weight.compose&&compute.outer_tick==weight.outer_tick&&
       compute.node_argument==weight.node_argument&&weight.ordinal==compute.ordinal+1&&compute.thread==weight.thread){
        input.flags|=8u;input.compute_ordinal=compute.ordinal;input.kind=compute.kind;input.end=compute.raw_rdx;
        input.flags|=(compute.before_valid?16u:0u)|(compute.after_valid?32u:0u)|(compute.returned?64u:0u);
        input.node=compute.after.node;input.root=compute.after.root;input.output=compute.out_after;
        input.affected_header=compute.affected_after;input.affected=compute.affected;
    }
    full_body_c0_native_live::arm_weight_return(input);
#else
    (void)call;(void)packet;(void)returned;
#endif
}
void begin_guarded(pose::CallPacket const& packet,unsigned kind,uintptr_t ra,uintptr_t rsp,Call& call) noexcept {try{begin_call(packet,kind,ra,rsp,call);}catch(...){clear_arms();failed=true;stopping=true;call.row.failure=Failure::protocol;}}
void finish_guarded(Call& call,pose::CallPacket& packet,bool returned) noexcept {try{finish_call(call,packet,returned);post_weight_scale::observe(call,packet,returned);future_g::arm(call);
#if defined(GEARS_COVER_NATIVE_RIG_OBSERVER) && GEARS_COVER_NATIVE_RIG_OBSERVER
    if(call.index&&call.row.kind<2)cover_native_rig_observer_live::control(call.row);
#endif
    arbitrate_arm_alpha(call,packet,returned);
    }catch(...){clear_arms();failed=true;stopping=true;}
    observe_ready_weight_return(call,packet,returned);
}
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
    reserved=true;reserve_stage="ready";reserve_rva=0;return future_g::reserve();
}
bool reserved_target(void* target){for(unsigned i=0;i<3;++i)if((reserved_mask&(1u<<i))&&targets[i]==target)return true;return future_g::reserved_target(target);}
bool configure(wchar_t const* ini,FILE* file){wchar_t value[16]{};GetPrivateProfileStringW(L"CoverControlObserver",L"enabled",L"0",value,16,ini);
    reserve_stage="configure_value";if(wcscmp(value,L"0")&&wcscmp(value,L"1"))return false;
    if(!wcscmp(value,L"1")){reserve_stage="configure_context";if(enabled||!phase::enabled||!continuous_session||!stereo_xr||!anchor_observer_enabled)return false;enabled=true;}
    reserve_stage=enabled?"configured":"disabled";
    if(!post_weight_scale::configure(ini,file)||!future_g::configure(ini,file))return false;
    if(file)fprintf(file,"cover_control_enabled=%u read_only=%u rows_max=128 first_pose_episode_only=1 atomic_verified=0 writer=0 arm_alpha_behavior_policy=%u\n",unsigned(enabled),unsigned(!arm_policy_enabled()),unsigned(arm_policy_enabled()));return true;
}
void request_drain(){stopping=true;clear_arms();future_g::request_drain();}
unsigned pending_count(){const uint64_t sum=uint64_t(in_flight.load())+compose_in_flight.load()+future_g::pending_count();return sum>UINT_MAX?UINT_MAX:unsigned(sum);}
void summary(FILE* file){std::lock_guard<std::mutex> lock(mutex);if(file)fprintf(file,"cover_control_reserve_stage=%s cover_control_reserve_rva=0x%llx cover_control_reserve_mh_status=%d cover_control_enabled=%u cover_control_certified=%u cover_control_reserved_mask=%u cover_control_calls=%llu cover_control_inflight=%u cover_control_compose_inflight=%u cover_control_rows=%u cover_control_episode=%u cover_control_dropped=%llu cover_control_rejected=%llu cover_control_nested=%llu cover_control_protocol=%llu cover_control_failed=%u cover_control_file_written=%u cover_control_file_error=%lu ",reserve_stage,static_cast<unsigned long long>(reserve_rva),reserve_mh_status,unsigned(enabled),unsigned(certified),reserved_mask,calls.load(),in_flight.load(),compose_in_flight.load(),row_count,selected_episode,dropped.load(),rejected.load(),nested_calls.load(),protocol_errors.load(),unsigned(failed.load()),unsigned(file_written.load()),file_error.load());if(file)fprintf(file,"arm_alpha_policy_requested=%u arm_alpha_behavior_changes=%llu arm_alpha_rejected=%llu ",unsigned(arm_policy_enabled()),arm_alpha_applied.load(),arm_alpha_rejected.load());
    if(file){for(unsigned s=0;s<arm_alpha_reasons.size();++s)for(unsigned r=1;r<32;++r)
        if(auto n=arm_alpha_reasons[s][r].load())fprintf(file,"arm_alpha_reject_stage_%u_reason_%u=%llu ",s,r,n);
        for(unsigned i=0;i<arm_alpha_weight_classes.size();++i)fprintf(file,"arm_alpha_reject_weight_class_%u=%llu ",i,arm_alpha_weight_classes[i].load());
        fprintf(file,"arm_alpha_last_rejected_weight=%08x ",arm_alpha_last_rejected_weight.load());
        for(unsigned i=0;i<16;++i){if(auto n=arm_alpha_token_compute_failures[i].load())fprintf(file,"arm_alpha_token_compute_failure_%u=%llu ",i,n);
            if(auto n=arm_alpha_token_weight_failures[i].load())fprintf(file,"arm_alpha_token_weight_failure_%u=%llu ",i,n);}
        for(unsigned i=0;i<4;++i)fprintf(file,"arm_alpha_token_role_%u=%llu ",i,arm_alpha_token_roles[i].load());
        for(unsigned i=0;i<8;++i)if(auto n=arm_node_state_failures[i].load())fprintf(file,"arm_node_state_failure_%u=%llu ",i,n);}
    future_g::summary(file);}
void print(FILE* file,bool disabled){
    future_g::print(file,disabled);post_weight_scale::print(file,disabled);
    if(enabled&&disabled&&stopping.load()&&!pending_count()&&!file_attempted.exchange(true)){wchar_t path[MAX_PATH]{};Header header{};memcpy(header.magic,"CCTRL001",8);
        header.version=1;header.header_size=sizeof(Header);header.row_size=sizeof(Row);header.count=row_count;header.capacity=row_limit;header.session=uint64_t(session_started_qpc);header.module_base=base;header.pid=GetCurrentProcessId();header.pose_episode=selected_episode;header.read_only=unsigned(!arm_policy_enabled()&&!arm_alpha_applied.load());
        if(directory[0]&&swprintf_s(path,L"%s\\control-node-phases.bin",directory)>0){HANDLE out=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(out!=INVALID_HANDLE_VALUE){DWORD written{};bool okay=WriteFile(out,&header,sizeof header,&written,nullptr)&&written==sizeof header;
                const auto n=DWORD(row_count*sizeof(Row));if(okay&&n)okay=WriteFile(out,rows.data(),n,&written,nullptr)&&written==n;
                if(!okay)file_error=GetLastError();if(!CloseHandle(out)){file_error=GetLastError();okay=false;}file_written=okay;
            }else file_error=GetLastError();}else file_error=ERROR_INVALID_NAME;
    }
    summary(file);if(file)fprintf(file,"read_only=%u atomic_verified=0 writer=0 arm_alpha_behavior_changes=%llu\n",unsigned(!arm_policy_enabled()&&!arm_alpha_applied.load()),arm_alpha_applied.load());
}
#else
bool retained_boundary_current()noexcept{return false;}
bool prepare(){return true;}bool reserve(){return true;}bool reserved_target(void*){return false;}
bool configure(wchar_t const* ini,FILE*){wchar_t value[16]{};GetPrivateProfileStringW(L"CoverControlObserver",L"enabled",L"0",value,16,ini);if(wcscmp(value,L"0"))return false;GetPrivateProfileStringW(L"CoverPostWeightScaleObserver",L"enabled",L"0",value,16,ini);if(wcscmp(value,L"0"))return false;GetPrivateProfileStringW(L"CoverFutureGObserver",L"enabled",L"0",value,16,ini);return !wcscmp(value,L"0");}
void compose_begin(phase::Scope const&) noexcept {}void compose_end(phase::Scope const&,bool) noexcept {}
void request_drain(){}unsigned pending_count(){return 0;}void summary(FILE* file){if(file)fprintf(file,"cover_control_enabled=0 cover_control_compiled=0 ");}
void print(FILE* file,bool){summary(file);}
#endif
} // namespace cover_control_observer_live
