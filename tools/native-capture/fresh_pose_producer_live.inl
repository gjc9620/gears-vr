// Private read-only producer -> C0 observer. No engine/getter/bridge calls.
#include "fresh_pose_producer_certificate.h"
#include "root_blend_overwrite.h"
#include "sequence_pose_overwrite_source.h"
#include "sequence_set_overwrite_source.h"
#include "per_bone_overwrite_source.h"
#include <new>
namespace fresh_pose_producer_live {
bool enabled{},per_bone_witness_enabled{};std::array<void*,3> targets{},originals{};bool own_entries();
unsigned hook_count()noexcept{return per_bone_witness_enabled?3u:2u;}
#if defined(GEARS_FRESH_POSE_PRODUCER_OBSERVER) && GEARS_FRESH_POSE_PRODUCER_OBSERVER
namespace pose=gears_cover_pose_phase;namespace phase=cover_pose_phase_live;
namespace hand=continuous_hand_owner_live;namespace binding=weapon_hand_scene_binding;
namespace cert=gears_fresh_pose_certificate;
using Entry=gears_continuous_hand_entry::Entry;
constexpr unsigned capacity=8,child_limit=16,all_reads=2047;
enum class Reject:unsigned {waiting,complete,owner,arguments,entry,read,changed,duplicate,exception,missing_c0,stopped,protocol};
struct Child {std::array<unsigned char,68> raw{};pose::Identity identity{};uintptr_t node{},table{},method{};
    std::array<unsigned char,32> prefix{};float weight{};uint32_t flags{};unsigned read_mask{};};
struct Stage {LONGLONG qpc{};pose::Metadata metadata{};pose::Identity root{},asset{};
    uintptr_t root_address{},root_mesh{},root_table{},root_method{},asset14c{},optional_instance{},children_data{};
    int32_t children_count{},root70{};uint32_t root_bc{};uint8_t optional_mode{};
    pose::ArrayHeader cache{};std::array<pose::Atom,115> local{},cached{};std::array<unsigned char,115> desired{};
    std::array<Child,child_limit> children{};unsigned read_mask{},alias_mask{};bool cache_alias{},desired_full{},stable{},entry_ok{};};
struct NativeCall {pose::CallPacket input{},output{};uintptr_t caller{},rsp{},fifth{};uint64_t serial{};
    LONGLONG begin_qpc{},end_qpc{};std::array<unsigned char,32> motion_before{},motion_after{};uint32_t flag_before{},flag_after{},argument_mask{};
    bool entered{},returned{},exception{},end_known{},packet_unchanged{};};
struct CopyImport {std::array<unsigned char,6> raw{};uintptr_t slot{},target{},module_base{};std::array<unsigned char,32> prefix{};
    std::array<wchar_t,MAX_PATH> module_path{};unsigned read_mask{};};
struct Record {pose::OwnerKey owner{};uint64_t tick{},compose{};unsigned thread{},index{},stages{};Reject reject{Reject::waiting};
    gears_native_rig_tracking::CoreSnapshot source{};gears_hand_authored_mount::Signature reference_signature{};
    std::array<gears_hand_authored_mount::ReferenceRecord68,115> reference{};
    std::array<NativeCall,2> calls{};std::array<Stage,5> stage{};
    Entry c0_entry{};CopyImport copy_before{},copy_c0{};LONGLONG close_qpc{};
    bool closed{},tick_returned{},optional_executed{},capture_policy1{},fresh_certified{},writer{};};
struct Header {char magic[8]{};unsigned version{},header_size{},record_size{},count{},capacity_value{},read_only{},writer{},fresh_certified{};
    uint64_t session{},module_base{},attempts{};unsigned child_capacity{},stage_count{},packet_size{},fifth_forwarded{};};
static_assert(std::is_trivially_copyable_v<Record>&&std::is_trivially_destructible_v<Record>);
std::mutex mutex;std::array<Record,capacity> records{};unsigned count{};
std::atomic<unsigned> in_flight{},open_frames{},protocol_debt{};
std::atomic<uint64_t> calls{},serial{1},attempts{},ignored{};
std::atomic<bool> stopping{},failed{},frozen{},file_attempted{},file_written{};std::atomic<DWORD> file_error{};
std::array<std::atomic<uint64_t>,12> blend_rejections{};
std::atomic<uint64_t> sequence_started{},sequence_finished{};
std::atomic<uint64_t> sequence_set_started{},sequence_set_finished{};
std::atomic<uint64_t> per_bone_started{},per_bone_finished{};
std::array<std::atomic<uint64_t>,6> per_bone_rejections{};
struct PerBoneWitness {gears_per_bone_overwrite::Source before{};gears_anchor::Identity identity{};};
struct SequenceFailureObservation {unsigned context{},root_snapshot{},root_same{},root_return_epoch{},sequence_snapshot{},sequence_same{},sequence_return{},local_same{},desired_same{};
    uint32_t root_before{},root_after{},mesh{},sequence_before{},sequence_after{},time_before{},time_after{};int32_t node70_before{},node70_after{},cache_before{},cache_after{},link_before{},link_after{};};
SequenceFailureObservation sequence_failure{};std::atomic<bool> sequence_failure_known{};
bool certified{},reserved{},sequence_witness_enabled{};std::array<std::array<unsigned char,6>,2> caller_bytes{};
struct Call {Call* previous{};Call const* counter_self{};uint64_t serial{};unsigned thread{},counter_thread{},slot{capacity},kind{},tree_index{128};
    bool linked{},counter_owned{},blend_active{},blend_chain{};uint64_t blend_tick{};
    gears_root_blend_overwrite::Source blend_before{};gears_anchor::Identity blend_root{};
#if defined(GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT) && GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT
    gears_atom_overwrite::BlendTicket blend_ticket{};
    gears_sequence_pose_overwrite::Source sequence_before{};gears_anchor::Identity sequence_identity{};bool sequence_active{};
    gears_sequence_set_overwrite::Source sequence_set_before{};bool sequence_set_active{};
    PerBoneWitness* per_bone{};
    alignas(::Scope) unsigned char blend_scope_bytes[sizeof(::Scope)]{};::Scope* blend_scope{};
#endif
};
struct Thread {Call* call{};unsigned depth{},slot{capacity};uint64_t tick{};Entry const* tick_address{};Stage repeat{};};
inline thread_local Thread local{};
extern "C" void GearsFreshRootHook();extern "C" void GearsFreshOptionalHook();extern "C" void GearsFreshPerBoneHook();
extern "C" void GearsFreshRawInvoke5(pose::CallPacket*,void*,uintptr_t) noexcept(false);
extern "C" void GearsFreshRawInvoke4(pose::CallPacket*,void*) noexcept(false);
const std::array<void*,3> detours{{reinterpret_cast<void*>(GearsFreshRootHook),reinterpret_cast<void*>(GearsFreshOptionalHook),reinterpret_cast<void*>(GearsFreshPerBoneHook)}};
unsigned pending_count(){const uint64_t n=uint64_t(in_flight.load())+open_frames.load()+protocol_debt.load();return n>UINT_MAX?UINT_MAX:unsigned(n);}
void debt() noexcept {auto d=protocol_debt.load();while(d!=UINT_MAX&&!protocol_debt.compare_exchange_weak(d,d+1)){}failed=true;stopping=true;}
void acquire(Call& c) noexcept {c.counter_self=&c;c.counter_thread=GetCurrentThreadId();c.counter_owned=true;++in_flight;++active;++calls;}
template<class T>bool read(T& v,uintptr_t at){return pose::span(at,sizeof(T))&&safe_copy(&v,reinterpret_cast<void const*>(at),sizeof(T));}
template<class T>bool field(T& v,uintptr_t at,size_t offset){uintptr_t p{};return pose::add(at,offset,p)&&read(v,p);}
bool overlap(uintptr_t a,size_t an,uintptr_t b,size_t bn){return an&&bn&&pose::span(a,an)&&pose::span(b,bn)&&a<=b+bn-1&&b<=a+an-1;}
void copy_import(CopyImport& r){r={};const uintptr_t thunk=base+0x577e5f;
    constexpr std::array<unsigned char,6> expected{{0xff,0x25,0xb3,0x4b,0xda,0}};
    if(!read(r.raw,thunk))return;r.read_mask|=1;if(r.raw!=expected)return;r.read_mask|=2;
    int32_t displacement{};memcpy(&displacement,r.raw.data()+2,4);
    if(displacement<0||!pose::add(thunk+6,size_t(displacement),r.slot))return;
    if(!read(r.target,r.slot)||!pose::span(r.target,32))return;r.read_mask|=4;
    if(safe_copy(r.prefix.data(),reinterpret_cast<void const*>(r.target),32))r.read_mask|=8;
    HMODULE module{};if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(r.target),&module)){
        r.module_base=reinterpret_cast<uintptr_t>(module);const auto n=GetModuleFileNameW(module,r.module_path.data(),unsigned(r.module_path.size()));
        if(n&&n<r.module_path.size())r.read_mask|=16;}
    std::array<unsigned char,6> raw_again{};uintptr_t target_again{};std::array<unsigned char,32> prefix_again{};
    if(read(raw_again,thunk)&&raw_again==r.raw&&read(target_again,r.slot)&&target_again==r.target&&
        safe_copy(prefix_again.data(),reinterpret_cast<void const*>(r.target),32)&&prefix_again==r.prefix)r.read_mask|=32;
    // The bytes decode one slot; module attribution does not identify memcpy.
}
bool caller_current(unsigned kind){std::array<unsigned char,6> b{};return kind<2&&certified&&safe_copy(b.data(),reinterpret_cast<void const*>(base+cert::calls[kind]),cert::call_sizes[kind])&&b==caller_bytes[kind];}
#if defined(GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT) && GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT
bool blend_world_current()noexcept {
    auto* c=local.call;
    return c&&c->counter_self==c&&c->counter_owned&&c->linked&&c->blend_active&&(c->kind==0||c->kind==2)&&
        c->thread==GetCurrentThreadId()&&c->counter_thread==c->thread&&
        c->blend_scope==reinterpret_cast<::Scope*>(c->blend_scope_bytes)&&c->blend_scope->world();
}
bool root_code_current(){
    if(!own_entries()||!caller_current(0))return false;
    for(unsigned i=0;i<2;++i){auto const& chunk=cert::chunks[i];std::array<unsigned char,2569> bytes{};
        if(chunk.size>bytes.size())return false;const auto address=base+chunk.rva;
        if(i==0){if(!read_attested_hook_entry(bytes.data(),reinterpret_cast<void const*>(address),32)||
            !safe_copy(bytes.data()+32,reinterpret_cast<void const*>(address+32),chunk.size-32))return false;}
        else if(!safe_copy(bytes.data(),reinterpret_cast<void const*>(address),chunk.size))return false;
        BCRYPT_ALG_HANDLE a{};BCRYPT_HASH_HANDLE h{};unsigned char digest[32]{};
        bool ok=BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
        if(ok)ok=BCryptCreateHash(a,&h,nullptr,0,nullptr,0,0)>=0;
        if(ok)ok=BCryptHashData(h,bytes.data(),chunk.size,0)>=0;
        if(ok)ok=BCryptFinishHash(h,digest,32,0)>=0;
        if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);if(!ok)return false;
        constexpr char hex[]="0123456789abcdef";for(unsigned j=0;j<32;++j)
            if(chunk.sha256[j*2]!=hex[digest[j]>>4]||chunk.sha256[j*2+1]!=hex[digest[j]&15])return false;
    }return true;
}
// Observe the actual per-bone blend entry: during reload its children write
// scratch arrays, so neither a child decoder nor F7940 proves the final copy.
// This witness retires only an existing owned pose after the original returns.
bool per_bone_code_current(){
    if(!own_entries())return false;
    for(unsigned i=0;i<cert::per_bone_pose.size();++i){const auto& chunk=cert::per_bone_pose[i];
        std::array<unsigned char,4055> bytes{};const auto address=base+chunk.rva;
        if(chunk.size>bytes.size())return false;
        if(i==0){if(!read_attested_hook_entry(bytes.data(),reinterpret_cast<void const*>(address),32)||
            !safe_copy(bytes.data()+32,reinterpret_cast<void const*>(address+32),chunk.size-32))return false;}
        else if(!safe_copy(bytes.data(),reinterpret_cast<void const*>(address),chunk.size))return false;
        BCRYPT_ALG_HANDLE a{};BCRYPT_HASH_HANDLE h{};unsigned char digest[32]{};
        bool ok=BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
        if(ok)ok=BCryptCreateHash(a,&h,nullptr,0,nullptr,0,0)>=0;
        if(ok)ok=BCryptHashData(h,bytes.data(),chunk.size,0)>=0;
        if(ok)ok=BCryptFinishHash(h,digest,32,0)>=0;
        if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);if(!ok)return false;
        constexpr char hex[]="0123456789abcdef";for(unsigned j=0;j<32;++j)
            if(chunk.sha256[j*2]!=hex[digest[j]>>4]||chunk.sha256[j*2+1]!=hex[digest[j]&15])return false;
    }return true;
}
void per_bone_begin(Call& c,pose::CallPacket const& p){
    const auto mesh=native_atom_overwrite_live::registered_mesh();
    if(!per_bone_witness_enabled||!enabled||c.kind!=2||!mesh||local.depth>32||
       p.gpr[2]!=mesh+0x27c||p.gpr[3]!=mesh+0x28c){++per_bone_rejections[0];return;}
    // Allocate only for a relevant original callback; ordinary root callbacks
    // keep their bounded small stack frame, including deeply nested trees.
    c.per_bone=new(std::nothrow) PerBoneWitness;
    if(!c.per_bone){++per_bone_rejections[1];return;}
    namespace pb=gears_per_bone_overwrite;auto& w=*c.per_bone;pb::Source repeat{};
    if(!pb::snapshot(safe_copy,base,p.gpr[1],mesh,w.before)||!pb::eligible(w.before)||
       !pb::snapshot(safe_copy,base,p.gpr[1],mesh,repeat)||!pb::same(w.before,repeat)){
        ++per_bone_rejections[1];return;}
    if(!per_bone_code_current()){++per_bone_rejections[2];return;}
    {std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
        if(!lock.owns_lock()||!binding::metadata_current()||!binding::identity(p.gpr[1],w.identity,true)){
            ++per_bone_rejections[3];return;}}
    c.blend_scope=::new(static_cast<void*>(c.blend_scope_bytes))::Scope;
    if(!c.blend_scope->world()){++per_bone_rejections[3];return;}
    c.blend_active=true;
    if(!native_atom_overwrite_live::blend_begin(c.blend_ticket,w.before.mesh_epoch)){
        c.blend_active=false;++per_bone_rejections[4];return;}
    ++per_bone_started;
}
void per_bone_return(Call& c,bool returned){
    if(!c.blend_active||!c.per_bone)return;
    namespace pb=gears_per_bone_overwrite;auto const& w=*c.per_bone;pb::Source after{},repeat{};
    bool valid=returned&&blend_world_current()&&per_bone_code_current()&&
        pb::snapshot(safe_copy,base,w.before.node,w.before.mesh,after)&&
        pb::snapshot(safe_copy,base,w.before.node,w.before.mesh,repeat)&&
        pb::same(after,repeat)&&pb::returned(w.before,after);
    if(valid){std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);gears_anchor::Identity id{};
        valid=lock.owns_lock()&&binding::metadata_current()&&binding::identity(w.before.node,id,true)&&binding::same(id,w.identity);}
    if(native_atom_overwrite_live::blend_return(c.blend_ticket,valid))++per_bone_finished;
    else ++per_bone_rejections[5];c.blend_active=false;
}
void blend_begin(Call& c,pose::CallPacket const& p,uintptr_t ra){
    if(!sequence_witness_enabled)return;
    namespace blend=gears_root_blend_overwrite;auto const& f=hand::slot;
    // Actual63508 canonical output passes through depth18 during locomotion.
    // Child-count16 is unrelated to call nesting. Keep bounded nesting while
    // proving the same actual implementation, canonical arrays and overwrite
    // cohort at every depth; no new C0 writer or inferred retirement is added.
    constexpr unsigned maximum_witness_depth=32;
    const bool direct=local.depth>0&&local.depth<=maximum_witness_depth;
    // F7940's full-weight child receives the same canonical output/desired
    // headers. Its nested root may perform the blend. Scratch-child calls at
    // F7BE0 and mirror dispatch are deliberately excluded.
    bool delegated{};auto* parent=c.previous;
    if(parent&&parent->counter_self==parent&&parent->counter_owned&&parent->linked&&parent->blend_chain&&
       parent->thread==GetCurrentThreadId()&&local.depth<=maximum_witness_depth&&ra==base+0xf7a6a){
        delegated=blend::delegates(parent->blend_before,p.gpr[1]);
    }
    const auto mesh=native_atom_overwrite_live::registered_mesh();
    if(!enabled||c.kind||(!direct&&!delegated)||!mesh||p.gpr[2]!=mesh+0x27c||p.gpr[3]!=mesh+0x28c){++blend_rejections[0];return;}
    // Retirement observes a real original write, independently of admitting a
    // NEW current-Tick pose. Acquire the same owned world lease as the existing
    // per-bone receipt path; never refresh a Scene/tracking timestamp here.
    c.blend_scope=::new(static_cast<void*>(c.blend_scope_bytes))::Scope;
    if(!c.blend_scope->world()){++blend_rejections[0];return;}
    blend::Source again{};
    if(!blend::snapshot(safe_copy,base,p.gpr[1],mesh,p.gpr[2],p.gpr[3],c.blend_before,true)){++blend_rejections[1];return;}
    if(!blend::snapshot(safe_copy,base,p.gpr[1],mesh,p.gpr[2],p.gpr[3],again,true)||!blend::same(c.blend_before,again)){++blend_rejections[3];return;}
    if(!root_code_current()){++blend_rejections[4];return;}
    {std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
        if(!lock.owns_lock()||!binding::metadata_current()||!binding::identity(p.gpr[1],c.blend_root,true)){++blend_rejections[5];return;}}
    c.blend_chain=true;
    if(!blend::blend(c.blend_before)){
        const auto child=blend::full_weight_child(c.blend_before);
        uintptr_t table{},method{};
        if(!child||!read(table,child)||!field(method,table,0x270)){++blend_rejections[8];return;}
        if(method==base+0x2fde20){
            namespace set=gears_sequence_set_overwrite;set::Source repeat{};
            if(!set::snapshot(safe_copy,base,child,mesh,c.sequence_set_before)||!set::eligible(c.sequence_set_before)||
               !set::snapshot(safe_copy,base,child,mesh,repeat)||!set::same(c.sequence_set_before,repeat)){
                ++blend_rejections[8];return;}
            for(auto const& chunk:cert::sequence_set_pose)if(!phase::digest(chunk)||!phase::digest(chunk)){++blend_rejections[10];return;}
            c.sequence_set_active=true;
        }else{
            gears_sequence_pose_overwrite::Source repeat{};
            if(!gears_sequence_pose_overwrite::snapshot(safe_copy,base,child,mesh,c.sequence_before)||
               !gears_sequence_pose_overwrite::snapshot(safe_copy,base,child,mesh,repeat)||!gears_sequence_pose_overwrite::same(c.sequence_before,repeat)){
                ++blend_rejections[8];return;}
            for(auto const& chunk:cert::sequence_pose)if(!phase::digest(chunk)||!phase::digest(chunk)){++blend_rejections[10];return;}
        }
        {std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
            if(!lock.owns_lock()||!binding::metadata_current()||!binding::identity(child,c.sequence_identity,true)){++blend_rejections[11];return;}}
        c.sequence_active=true;
    }
    c.blend_tick=f.active?f.tick:0;c.blend_active=true;
    if(!native_atom_overwrite_live::blend_begin(c.blend_ticket,c.blend_before.mesh_epoch)){++blend_rejections[6];c.blend_active=false;}else if(c.sequence_active){++sequence_started;if(c.sequence_set_active)++sequence_set_started;}
}
void blend_return(Call& c,bool returned){
    if(!c.blend_active)return;namespace blend=gears_root_blend_overwrite;
    blend::Source after{},again{};auto const& before=c.blend_before;bool valid=returned&&blend_world_current()&&root_code_current()&&
        blend::snapshot(safe_copy,base,before.root,before.mesh,before.mesh+0x27c,before.mesh+0x28c,after,true)&&
        blend::snapshot(safe_copy,base,before.root,before.mesh,before.mesh+0x27c,before.mesh+0x28c,again,true)&&
        blend::same(after,again)&&(c.sequence_active?(blend::same(before,after,false)&&after.root_epoch==before.mesh_epoch):blend::returned(before,after));
    if(valid&&c.sequence_active&&c.sequence_set_active){
        namespace set=gears_sequence_set_overwrite;set::Source last{},repeat{};
        valid=set::snapshot(safe_copy,base,c.sequence_set_before.node,before.mesh,last)&&
            set::snapshot(safe_copy,base,c.sequence_set_before.node,before.mesh,repeat)&&set::same(last,repeat)&&
            set::returned(c.sequence_set_before,last);
        if(valid)for(auto const& chunk:cert::sequence_set_pose)if(!phase::digest(chunk)||!phase::digest(chunk)){valid=false;break;}
        if(valid){std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);gears_anchor::Identity id{};
            valid=lock.owns_lock()&&binding::metadata_current()&&binding::identity(c.sequence_set_before.node,id,true)&&binding::same(id,c.sequence_identity);}
    }else if(valid&&c.sequence_active){
        gears_sequence_pose_overwrite::Source last{},repeat{};
        valid=gears_sequence_pose_overwrite::snapshot(safe_copy,base,c.sequence_before.node,before.mesh,last,false)&&
            gears_sequence_pose_overwrite::snapshot(safe_copy,base,c.sequence_before.node,before.mesh,repeat,false)&&
            gears_sequence_pose_overwrite::same(last,repeat)&&gears_sequence_pose_overwrite::returned(c.sequence_before,last);
        if(valid)for(auto const& chunk:cert::sequence_pose)if(!phase::digest(chunk)||!phase::digest(chunk)){valid=false;break;}
        if(valid){std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);gears_anchor::Identity id{};
            valid=lock.owns_lock()&&binding::metadata_current()&&binding::identity(c.sequence_before.node,id,true)&&binding::same(id,c.sequence_identity);}
    }
    if(valid){std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);gears_anchor::Identity id{};
        valid=lock.owns_lock()&&binding::metadata_current()&&binding::identity(before.root,id,true)&&binding::same(id,c.blend_root);}
    if(c.sequence_active&&!c.sequence_set_active&&!valid&&!sequence_failure_known.load()){
        auto& d=sequence_failure;d.context=unsigned(returned&&blend_world_current()&&root_code_current());
        gears_root_blend_overwrite::Source root{};gears_sequence_pose_overwrite::Source seq{};
        d.root_snapshot=unsigned(blend::snapshot(safe_copy,base,before.root,before.mesh,before.mesh+0x27c,before.mesh+0x28c,root,true));
        d.root_same=unsigned(blend::same(before,root,false));d.root_return_epoch=unsigned(root.root_epoch==before.mesh_epoch);
        d.sequence_snapshot=unsigned(gears_sequence_pose_overwrite::snapshot(safe_copy,base,c.sequence_before.node,before.mesh,seq,false));
        d.sequence_same=unsigned(gears_sequence_pose_overwrite::same(c.sequence_before,seq,true));d.sequence_return=unsigned(gears_sequence_pose_overwrite::returned(c.sequence_before,seq));
        d.local_same=unsigned(pose::header_equal(c.sequence_before.local,seq.local));d.desired_same=unsigned(pose::header_equal(c.sequence_before.desired,seq.desired));
        d.root_before=before.root_epoch;d.root_after=root.root_epoch;d.mesh=before.mesh_epoch;d.sequence_before=c.sequence_before.node_epoch;d.sequence_after=seq.node_epoch;
        d.time_before=c.sequence_before.time_bits;d.time_after=seq.time_bits;d.node70_before=c.sequence_before.node70;d.node70_after=seq.node70;
        d.cache_before=c.sequence_before.cache.count;d.cache_after=seq.cache.count;d.link_before=c.sequence_before.link_index;d.link_after=seq.link_index;
        sequence_failure_known.store(true,std::memory_order_release);
    }
    const bool completed=native_atom_overwrite_live::blend_return(c.blend_ticket,valid);
    if(!completed)++blend_rejections[7];else if(c.sequence_active){++sequence_finished;if(c.sequence_set_active)++sequence_set_finished;}c.blend_active=false;
}
#else
bool blend_world_current()noexcept{return false;}void blend_begin(Call&,pose::CallPacket const&,uintptr_t){}void blend_return(Call&,bool){}
void per_bone_begin(Call&,pose::CallPacket const&){}void per_bone_return(Call&,bool){}
#endif
bool context(Record const& r){return hand::owner_current()&&hand::same_source()&&hand::slot.tick==r.tick&&
    hand::slot.tick_address==local.tick_address&&r.thread==GetCurrentThreadId()&&phase::same(r.owner,hand::slot.data.owner.key);}
#include "fresh_pose_call_tree.inl"
#if defined(GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT) && GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT
bool record_base_entry{};
struct BaseEntryObservation {
    PendingBranch pending{};
    uintptr_t root{},method{},ra{},out{},desired{};uint64_t count{},tick{};
    uint64_t pending_count{},pending_cached{},pending_uncached{};
    unsigned canonical{},depth{};int32_t cache_count{};uint32_t child_epoch{};int32_t child_cache_count{};
    uint32_t root_epoch{},mesh_epoch{};int32_t child_count{};
    pose::ArrayHeader locals{},wanted{};std::array<uint32_t,4> weights{};
    std::array<uintptr_t,4> child_methods{};
    int32_t nested_count{},descriptors_count{},layer_count{};unsigned char nested_mode{};uint32_t nested_epoch{};std::array<uint32_t,4> nested_weights{};
};
std::mutex base_entry_mutex;std::array<BaseEntryObservation,16> base_entries{};unsigned base_entry_count{};
void note_base_entry(pose::CallPacket const& p,unsigned kind,uintptr_t ra)noexcept {
    if(!record_base_entry||kind==1||!hand::slot.active||hand::slot.tick_depth!=1||hand::slot.compose_depth||
       !hand::slot.lease||!hand::slot.lease->world()||hand::slot.data.owner.thread!=GetCurrentThreadId())return;
    const auto mesh=hand::slot.data.owner.key.mesh.address;
    BaseEntryObservation row{};row.root=p.gpr[1];row.ra=ra;row.out=p.gpr[2];row.desired=p.gpr[3];row.tick=hand::slot.tick;
    uintptr_t table{},children{},bound{};
    if(!read(table,row.root)||!field(row.method,table,0x270)||!field(bound,row.root,0x60)||bound!=mesh||
       !field(row.root_epoch,row.root,0xbc)||!field(row.mesh_epoch,mesh,0x338)||!field(row.child_count,row.root,0xec)||
       !field(children,row.root,0xe4)||!read(row.locals,row.out)||!read(row.wanted,row.desired))return;
    pose::ArrayHeader canonical_local{},canonical_desired{},cache{};
    if(!read(canonical_local,mesh+0x27c)||!read(canonical_desired,mesh+0x28c)||!read(cache,row.root+0x80))return;
    row.canonical=unsigned(row.out==mesh+0x27c)|unsigned(row.desired==mesh+0x28c)*2|
        unsigned(row.locals.data==canonical_local.data)*4|unsigned(row.wanted.data==canonical_desired.data)*8;
    if(row.canonical!=15)return; // This investigation concerns canonical stores only.
    row.depth=local.depth;row.cache_count=cache.count;
    auto& observation=row.pending;
    observation.qpc=uint64_t(phase::qpc());observation.tick=row.tick;
    observation.root=row.root;observation.mesh=mesh;
    observation.method=row.method-base;observation.caller=row.ra-base;
    observation.canonical=row.canonical;observation.depth=row.depth;
    observation.root_epoch=row.root_epoch;observation.mesh_epoch=row.mesh_epoch;
    observation.child_count=row.child_count;observation.cache_data=cache.data;
    observation.cache_count=cache.count;observation.cache_capacity=cache.capacity;
    observation.local_data=row.locals.data;observation.local_count=row.locals.count;
    observation.local_capacity=row.locals.capacity;
    if(local.call&&local.call->sequence_set_before.node){const auto& s=local.call->sequence_set_before;
        observation.sequence_count=s.sequences.count;observation.sequence_selected=s.selected;
        observation.sequence_blended=unsigned(s.blended);
        for(unsigned i=0;i<4;++i)observation.sequence_weights[i]=gears_sequence_set_overwrite::weight(s.entries[i]);}
    observation.kind=kind;
    if(kind==2){unsigned char mode{};pose::ArrayHeader descriptors{},layer{};
        if(field(mode,row.root,0x110))observation.mode=mode;
        if(field(descriptors,row.root,0x100))observation.descriptors=descriptors.count;
        if(descriptors.count>0&&descriptors.count<16&&field(layer,descriptors.data,0x2c))observation.layer_count=layer.count;}
    row.pending_count=unsigned(native_atom_overwrite_live::registered_mesh()==mesh);
    row.pending_cached=row.pending_count&&row.root_epoch==row.mesh_epoch&&row.cache_count==row.locals.count;
    row.pending_uncached=row.pending_count&&!row.pending_cached;
    for(int i=0;i<row.child_count&&i<4;++i){uintptr_t child{},child_table{};
        if(!field(row.weights[unsigned(i)],children,uintptr_t(i)*68+16)||!field(child,children,uintptr_t(i)*68+8))break;
        if(child&&read(child_table,child))field(row.child_methods[unsigned(i)],child_table,0x270);
        observation.child_nodes[unsigned(i)]=child;
        observation.child_methods[unsigned(i)]=row.child_methods[unsigned(i)]?row.child_methods[unsigned(i)]-base:0;
        observation.weights[unsigned(i)]=row.weights[unsigned(i)];
        pose::ArrayHeader observed_cache{};
        if(child){field(observation.child_epochs[unsigned(i)],child,0xbc);
            if(field(observed_cache,child,0x80))observation.child_cache_counts[unsigned(i)]=observed_cache.count;}
        if(child&&row.weights[unsigned(i)]==0x3f800000){
            pose::ArrayHeader child_cache{};field(row.child_epoch,child,0xbc);
            if(field(child_cache,child,0x80))row.child_cache_count=child_cache.count;
        }
        if(child&&row.child_methods[unsigned(i)]==base+0xfacb0&&row.weights[unsigned(i)]==0x3f800000){
            uintptr_t nested_children{};pose::ArrayHeader descriptors{},layer{};
            field(row.nested_count,child,0xec);field(row.nested_mode,child,0x110);field(row.nested_epoch,child,0xbc);
            field(nested_children,child,0xe4);field(descriptors,child,0x100);row.descriptors_count=descriptors.count;
            if(descriptors.count>0&&descriptors.count<16&&field(layer,descriptors.data,0x2c))row.layer_count=layer.count;
            for(int j=0;j<row.nested_count&&j<4;++j)field(row.nested_weights[unsigned(j)],nested_children,uintptr_t(j)*68+16);
        }
    }
    std::lock_guard<std::mutex> lock(base_entry_mutex);unsigned index=0;
    for(;index<base_entry_count;++index)if(base_entries[index].root==row.root&&base_entries[index].canonical==row.canonical)break;
    if(index==base_entries.size()){
        // Retain the latest canonical branches, rather than silently losing
        // a newly selected walking node after the first16 identities.
        index=0;for(unsigned i=1;i<base_entry_count;++i)
            if(base_entries[i].pending.qpc<base_entries[index].pending.qpc)index=i;
        base_entries[index]={};
    }else if(index==base_entry_count)++base_entry_count;
    row.count=base_entries[index].count+1;
    row.pending_count+=base_entries[index].pending_count;row.pending_cached+=base_entries[index].pending_cached;
    row.pending_uncached+=base_entries[index].pending_uncached;base_entries[index]=row;
}
unsigned pending_branches(uintptr_t mesh,std::array<PendingBranch,16>& output) noexcept {
    output={};if(!record_base_entry||!mesh)return 0;
    std::lock_guard<std::mutex> lock(base_entry_mutex);unsigned count=0;
    for(unsigned i=0;i<base_entry_count;++i){auto const& p=base_entries[i].pending;
        if(p.mesh==mesh&&p.canonical==15&&count<output.size())output[count++]=p;}
    return count;
}
void base_entry_summary(FILE* f){if(!f||!record_base_entry)return;std::lock_guard<std::mutex> lock(base_entry_mutex);
    for(unsigned i=0;i<base_entry_count;++i){auto const& v=base_entries[i];
        fprintf(f,"base_entry_%u_method=%llu base_entry_%u_ra=%llu base_entry_%u_count=%llu base_entry_%u_tick=%llu base_entry_%u_children=%d base_entry_%u_root_epoch=%u base_entry_%u_mesh_epoch=%u base_entry_%u_locals=%d base_entry_%u_desired=%d ",
            i,uint64_t(v.method-base),i,uint64_t(v.ra-base),i,v.count,i,v.tick,i,v.child_count,i,v.root_epoch,i,v.mesh_epoch,i,v.locals.count,i,v.wanted.count);
        fprintf(f,"base_entry_%u_root=%llu base_entry_%u_nested_count=%d base_entry_%u_nested_mode=%u base_entry_%u_nested_epoch=%u base_entry_%u_descriptors=%d base_entry_%u_layer=%d ",i,uint64_t(v.root),i,v.nested_count,i,unsigned(v.nested_mode),i,v.nested_epoch,i,v.descriptors_count,i,v.layer_count);
        fprintf(f,"base_entry_%u_canonical=%u base_entry_%u_depth=%u base_entry_%u_cache=%d base_entry_%u_pending=%llu base_entry_%u_pending_cached=%llu base_entry_%u_pending_uncached=%llu base_entry_%u_child_epoch=%u base_entry_%u_child_cache=%d ",
            i,v.canonical,i,v.depth,i,v.cache_count,i,v.pending_count,i,v.pending_cached,i,v.pending_uncached,i,v.child_epoch,i,v.child_cache_count);
        for(unsigned j=0;j<4;++j)fprintf(f,"base_entry_%u_nested_weight_%u=%u ",i,j,v.nested_weights[j]);
        for(unsigned j=0;j<4;++j)fprintf(f,"base_entry_%u_child_%u_method=%llu base_entry_%u_child_%u_weight=%u ",i,j,v.child_methods[j]?uint64_t(v.child_methods[j]-base):0,i,j,v.weights[j]);
    }
}


#else
bool record_base_entry{};void note_base_entry(pose::CallPacket const&,unsigned,uintptr_t)noexcept{}void base_entry_summary(FILE*){}
unsigned pending_branches(uintptr_t,std::array<PendingBranch,16>& output) noexcept {output={};return 0;}
#endif
bool desired_complete(Stage& s){if(s.metadata.desired.count!=115)return false;std::array<bool,115> seen{};
    for(unsigned i=0;i<115;++i){const auto b=s.desired[i];if(b>=115||seen[b])return false;
        int32_t parent{};memcpy(&parent,hand::slot.data.reference[b].bytes+0x38,4);
        if(b&&(parent<0||parent>=115||!seen[unsigned(parent)]))return false;seen[b]=true;}return true;}
// All copies are to our private Stage. Children stride=0x44 is certified by F7940.
// No inference of +F0 capacity or a broader child class layout is made.
void sample_once(Record const& r,Stage& s){s={};s.qpc=phase::qpc();
    if(context(r))s.read_mask|=1;else return;
    pose::Request request{};request.owner=r.owner;auto reader=[](void* o,void const* p,size_t n){return safe_copy(o,p,n);};
    if(!pose::metadata(reader,request,s.metadata))return;s.read_mask|=2;
    const auto mesh=r.owner.mesh.address;
    bool roots=field(s.root_address,mesh,0x230)&&s.root_address&&field(s.root_mesh,s.root_address,0x60)&&
        read(s.root_table,s.root_address)&&field(s.root_method,s.root_table,0x270);
    {std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);gears_anchor::Identity a{},b{};
        if(roots&&lock.owns_lock()&&binding::metadata_current()&&binding::identity(s.root_address,a,true)&&binding::identity(s.metadata.asset,b,true)){
            s.root=phase::compact(a);s.asset=phase::compact(b);if(s.root_mesh==mesh&&s.root_method==reinterpret_cast<uintptr_t>(targets[0]))s.read_mask|=4;}}
    if(pose::header_valid(s.metadata.local,32)&&s.metadata.local.count==115&&safe_copy(s.local.data(),reinterpret_cast<void const*>(s.metadata.local.data),sizeof s.local))s.read_mask|=8;
    if(pose::header_valid(s.metadata.desired,1,true)&&s.metadata.desired.count<=115&&
        (!s.metadata.desired.count||safe_copy(s.desired.data(),reinterpret_cast<void const*>(s.metadata.desired.data),size_t(s.metadata.desired.count))))s.read_mask|=16;
    if(field(s.cache,s.root_address,0x80)&&s.cache.count>=0&&s.cache.count<=115&&s.cache.capacity>=s.cache.count&&
        (!s.cache.count||(pose::header_valid(s.cache,32)&&safe_copy(s.cached.data(),reinterpret_cast<void const*>(s.cache.data),size_t(s.cache.count)*32))))s.read_mask|=32;
    const uintptr_t lh=mesh+0x27c,ch=s.root_address+0x80;const size_t cache_bytes=s.cache.count>0&&s.cache.count<=115?size_t(s.cache.count)*32:0;
    if(overlap(s.metadata.local.data,sizeof s.local,s.cache.data,cache_bytes))s.alias_mask|=1;
    if(overlap(lh,16,ch,16))s.alias_mask|=2;if(overlap(lh,16,s.cache.data,cache_bytes))s.alias_mask|=4;
    if(overlap(s.metadata.local.data,sizeof s.local,ch,16))s.alias_mask|=8;
    if(overlap(lh,16,s.metadata.local.data,sizeof s.local))s.alias_mask|=16;if(overlap(ch,16,s.cache.data,cache_bytes))s.alias_mask|=32;
    s.cache_alias=s.alias_mask!=0;
    if(field(s.children_data,s.root_address,0xe4)&&field(s.children_count,s.root_address,0xec)&&s.children_count>=0&&s.children_count<=int(child_limit)){
        bool raw=true,identities=true;
        for(int i=0;i<s.children_count;++i){auto& c=s.children[unsigned(i)];uintptr_t at{};
            if(!pose::add(s.children_data,size_t(i)*68,at)||!read(c.raw,at)){raw=false;identities=false;break;}c.read_mask=1;
            memcpy(&c.node,c.raw.data()+8,8);memcpy(&c.weight,c.raw.data()+0x10,4);memcpy(&c.flags,c.raw.data()+0x3c,4);
            if(!c.node){c.read_mask|=2;continue;}
            std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);gears_anchor::Identity identity{};
            if(lock.owns_lock()&&binding::metadata_current()&&binding::identity(c.node,identity,true)&&read(c.table,c.node)&&
                field(c.method,c.table,0x270)&&pose::span(c.method,32)&&safe_copy(c.prefix.data(),reinterpret_cast<void const*>(c.method),32)){
                c.identity=phase::compact(identity);c.read_mask|=2;
            }else identities=false;
        }if(raw)s.read_mask|=64;if(identities)s.read_mask|=128;
    }
    if(hand::metadata(true)&&hand::geometry()&&hand::slot.data.signature==r.reference_signature&&
        !memcmp(hand::slot.data.reference.data(),r.reference.data(),sizeof r.reference))s.read_mask|=512;
    if(field(s.root70,s.root_address,0x70)&&field(s.root_bc,s.root_address,0xbc)&&field(s.asset14c,s.metadata.asset,0x14c)&&
        field(s.optional_instance,mesh,0x470)&&field(s.optional_mode,mesh,0x46d))s.read_mask|=1024;
    s.entry_ok=own_entries()&&caller_current(0)&&caller_current(1);s.desired_full=(s.read_mask&16)&&desired_complete(s);
}
bool same_stage(Stage const& a,Stage const& b){
    if(!pose::metadata_equal(a.metadata,b.metadata)||!phase::same(a.root,b.root)||!phase::same(a.asset,b.asset)||
        a.root_address!=b.root_address||a.root_mesh!=b.root_mesh||a.root_table!=b.root_table||a.root_method!=b.root_method||
        a.asset14c!=b.asset14c||a.optional_instance!=b.optional_instance||a.children_data!=b.children_data||a.children_count!=b.children_count||
        a.root70!=b.root70||a.root_bc!=b.root_bc||a.optional_mode!=b.optional_mode||!pose::header_equal(a.cache,b.cache)||
        memcmp(a.local.data(),b.local.data(),sizeof a.local)||memcmp(a.cached.data(),b.cached.data(),sizeof a.cached)||a.desired!=b.desired||
        a.read_mask!=b.read_mask||a.alias_mask!=b.alias_mask||a.cache_alias!=b.cache_alias||a.desired_full!=b.desired_full||a.entry_ok!=b.entry_ok)return false;
    for(unsigned i=0;i<child_limit;++i){auto const& x=a.children[i];auto const& y=b.children[i];
        if(x.raw!=y.raw||!phase::same(x.identity,y.identity)||x.node!=y.node||x.table!=y.table||x.method!=y.method||x.prefix!=y.prefix||x.flags!=y.flags||x.read_mask!=y.read_mask)return false;}
    return true; // qpc is independently recorded; C++ struct padding is never evidence.
}
void sample(Record const& r,Stage& s){sample_once(r,s);sample_once(r,local.repeat);
    s.stable=same_stage(s,local.repeat);
    if(s.stable&&context(r))s.read_mask|=256;
}
bool usable(Stage const& s){return s.read_mask==all_reads&&s.stable&&s.entry_ok&&!s.cache_alias&&s.desired_full;}
void reject(Record& r,Reject why){if(r.reject==Reject::waiting||r.reject==Reject::complete)r.reject=why;}
void begin(pose::CallPacket const& p,unsigned kind,uintptr_t ra,uintptr_t rsp,uintptr_t fifth,Call& c){
    c.previous=local.call;c.serial=serial.fetch_add(1);c.thread=GetCurrentThreadId();c.kind=kind;c.linked=true;local.call=&c;++local.depth;
    if(kind==2){note_base_entry(p,kind,ra);per_bone_begin(c,p);return;}
    blend_begin(c,p,ra);
    note_base_entry(p,kind,ra);
    tree_nested(p,ra,rsp,fifth,c);
    if(!enabled||stopping.load()||kind>1||ra!=base+cert::returns[kind]||!hand::owner_current()||!hand::same_source()){++ignored;return;}
    if(local.depth!=1){if(local.slot<capacity)reject(records[local.slot],Reject::duplicate);return;}
    const auto mesh=hand::slot.data.owner.key.mesh.address;
    if(p.gpr[2]!=mesh+0x27c||(kind==1&&p.gpr[1]!=mesh)){++ignored;return;}
    if(!kind){
        if(local.slot<capacity){reject(records[local.slot],Reject::duplicate);return;}
        if(frozen.load())return;
        std::lock_guard<std::mutex> lock(mutex);if(count==capacity)return;
        local.slot=count++;local.tick=hand::slot.tick;local.tick_address=hand::slot.tick_address;++open_frames;++attempts;
        auto& r=records[local.slot];r={};r.index=local.slot;r.tick=local.tick;r.thread=c.thread;r.owner=hand::slot.data.owner.key;r.source=hand::slot.data.record.source;
        copy_import(r.copy_before);
        if(hand::metadata(true)&&hand::geometry()){r.reference_signature=hand::slot.data.signature;r.reference=hand::slot.data.reference;}else reject(r,Reject::read);
    }else if(local.slot==capacity){++ignored;return;}
    c.slot=local.slot;auto& r=records[c.slot];auto& n=r.calls[kind];
    if(n.entered){reject(r,Reject::duplicate);c.slot=capacity;return;}
    n.entered=true;n.serial=c.serial;n.input=p;n.caller=ra;n.rsp=rsp;n.fifth=kind?0:fifth;n.begin_qpc=phase::qpc();
    if(!kind){if(read(n.motion_before,p.gpr[4]))n.argument_mask|=1;if(read(n.flag_before,fifth))n.argument_mask|=2;}
    uintptr_t actual_ra{},actual_fifth{};
    if(!context(r)||hand::slot.compose_depth)reject(r,Reject::owner);
    if((rsp&15)!=8||!read(actual_ra,rsp)||actual_ra!=ra)reject(r,Reject::arguments);
    if(!kind&&(!field(actual_fifth,rsp,0x28)||actual_fifth!=fifth))reject(r,Reject::arguments);
    if(!own_entries()||!caller_current(kind))reject(r,Reject::entry);
    const unsigned stage=kind?2u:0u;sample(r,r.stage[stage]);r.stages|=1u<<stage;
    if(!usable(r.stage[stage]))reject(r,Reject::read);
    if(!kind&&(p.gpr[1]!=r.stage[0].root_address||p.gpr[3]!=mesh+0x28c||!pose::span(p.gpr[4],32)||!pose::span(fifth,4)))reject(r,Reject::arguments);
    if(!kind&&n.argument_mask!=3)reject(r,Reject::arguments);
    if(!kind)tree_start(r,p,ra,rsp,fifth,c);
    if(kind){r.optional_executed=true;if(!r.calls[0].returned||!r.calls[0].end_known||r.stage[1].metadata.pose_epoch!=r.stage[2].metadata.pose_epoch)reject(r,Reject::changed);}
}
void finish(Call& c,pose::CallPacket& p,bool returned){
    if(c.kind==2){per_bone_return(c,returned);return;}
    blend_return(c,returned);if(c.slot==capacity)return;auto& r=records[c.slot];auto& n=r.calls[c.kind];
    if(n.serial!=c.serial||r.closed){debt();return;}n.output=p;n.end_qpc=phase::qpc();n.returned=returned;n.exception=!returned;
    if(!returned)reject(r,Reject::exception);if(stopping.load())reject(r,Reject::stopped);
    if(returned&&!c.kind){if(read(n.motion_after,n.input.gpr[4]))n.argument_mask|=4;if(read(n.flag_after,n.fifth))n.argument_mask|=8;
        if(n.argument_mask!=15)reject(r,Reject::arguments);}
    if(returned){const unsigned stage=c.kind?3u:1u;sample(r,r.stage[stage]);r.stages|=1u<<stage;
        if(!usable(r.stage[stage]))reject(r,Reject::read);
        const auto& before=r.stage[stage-1];const auto& after=r.stage[stage];
        if(!context(r)||!phase::same(before.root,after.root)||!phase::same(before.asset,after.asset)||
            before.metadata.pose_epoch!=after.metadata.pose_epoch||!pose::header_equal(before.metadata.desired,after.metadata.desired)||
            memcmp(before.desired.data(),after.desired.data(),115))reject(r,Reject::changed);}
    // Compare the current wrapper packet only AFTER every observer read against
    // the immutable raw-return copy taken above. A mismatch is not admissible.
    n.packet_unchanged=returned&&!memcmp(&p,&n.output,sizeof(p));
    if(returned&&!n.packet_unchanged){reject(r,Reject::changed);p=n.output;} // Restore ONLY our private wrapper packet.
}
void release(Call& c) noexcept {
#if defined(GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT) && GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT
    delete c.per_bone;c.per_bone=nullptr;
    if(c.blend_scope){if(c.blend_scope==reinterpret_cast<::Scope*>(c.blend_scope_bytes)){c.blend_scope->~Scope();c.blend_scope=nullptr;}else debt();}
#endif
    const bool counted=c.counter_self==&c&&c.counter_owned&&c.counter_thread==GetCurrentThreadId();
    const bool known=counted&&c.linked&&c.serial&&c.thread==GetCurrentThreadId()&&local.call==&c&&local.depth;
    if(known){local.call=c.previous;--local.depth;}else debt();if(counted)c.counter_owned=false;
    if(c.slot<capacity){auto& r=records[c.slot];if(r.calls[c.kind].serial==c.serial){r.calls[c.kind].end_known=known;if(!known)reject(r,Reject::protocol);}else debt();}
    tree_release(c,known);
    if(counted){--in_flight;--active;}
}
void begin_guarded(pose::CallPacket& p,unsigned k,uintptr_t ra,uintptr_t rsp,uintptr_t fifth,Call& c) noexcept {
    const auto saved=p;try{begin(p,k,ra,rsp,fifth,c);}catch(...){debt();}
    if(record_call_tree&&memcmp(&p,&saved,sizeof p)){tree_packet_changed(c);p=saved;}
}
void finish_guarded(Call& c,pose::CallPacket& p,bool returned) noexcept {
    const auto saved=p;try{tree_finish(c,returned);}catch(...){tree_fail();}
    if(record_call_tree&&memcmp(&p,&saved,sizeof p)){tree_packet_changed(c);p=saved;}
    try{finish(c,p,returned);}catch(...){debt();}
}
extern "C" __declspec(noinline) void GearsFreshInvokeProtected(pose::CallPacket* p,unsigned k,uintptr_t ra,uintptr_t rsp,uintptr_t fifth) noexcept(false){
    Call c{};bool returned{};acquire(c);
    __try{begin_guarded(*p,k,ra,rsp,k==1?0:fifth,c);
        if(k==1)GearsFreshRawInvoke4(p,originals[1]);else GearsFreshRawInvoke5(p,originals[k],fifth);returned=true;}
    __finally{__try{finish_guarded(c,*p,returned&&!AbnormalTermination());}__finally{release(c);}}
}
void pose_begin(Entry const& e) noexcept {if(!enabled||e.kind!=0||local.slot==capacity)return;try{auto& r=records[local.slot];
    if(r.closed||r.compose){reject(r,Reject::duplicate);return;}r.compose=e.invocation;r.c0_entry=e;
    if(!e.selected||!context(r)||local.depth||e.ra!=base+pose::compose_caller||e.rcx!=r.owner.mesh.address||e.rdx!=e.rcx+0x27c||e.r8!=e.rcx+0x28c||!r.calls[0].end_known||!r.calls[0].returned){reject(r,Reject::arguments);return;}
    sample(r,r.stage[4]);copy_import(r.copy_c0);r.stages|=16;if(!usable(r.stage[4]))reject(r,Reject::read);
    const auto& prior=r.stage[r.optional_executed?3:1];
    if(!phase::same(prior.root,r.stage[4].root)||!phase::same(prior.asset,r.stage[4].asset)||prior.metadata.pose_epoch!=r.stage[4].metadata.pose_epoch||
        !pose::header_equal(prior.metadata.local,r.stage[4].metadata.local)||!pose::header_equal(prior.metadata.desired,r.stage[4].metadata.desired)||
        memcmp(prior.desired.data(),r.stage[4].desired.data(),115))reject(r,Reject::changed);
    if(r.optional_executed&&(!r.calls[1].returned||!r.calls[1].end_known))reject(r,Reject::exception);
}catch(...){debt();}}
void pose_end(Entry const& e,bool returned) noexcept {if(!enabled||e.kind!=2||local.slot==capacity)return;
    if(!e.selected||&e!=local.tick_address||e.invocation!=local.tick||e.thread!=GetCurrentThreadId()||local.depth){debt();return;}
    auto& r=records[local.slot];if(r.closed){debt();return;}r.closed=true;r.tick_returned=returned;r.close_qpc=phase::qpc();
    if(!returned)reject(r,Reject::exception);if(stopping.load())reject(r,Reject::stopped);if(!(r.stages&16))reject(r,Reject::missing_c0);
    if(r.reject==Reject::waiting){r.reject=Reject::complete;r.capture_policy1=true;frozen=true;}
    // capture_policy1 only means a complete joined observation; no child proof,
    // writer permission, cross-thread exclusion or fresh-write coverage follows.
    tree_close(r);
    local.slot=capacity;local.tick=0;local.tick_address=nullptr;--open_frames;
}
bool prepare(){if(!enabled)return true;if(certified)return true;
    for(auto const& c:cert::chunks)if(!phase::digest(c)||!phase::digest(c))return false;
    if(per_bone_witness_enabled)for(auto const& c:cert::per_bone_pose)if(!phase::digest(c)||!phase::digest(c))return false;
    for(unsigned i:{6u,7u})if(!phase::digest(phase::certificate::chunks[i])||!phase::digest(phase::certificate::chunks[i]))return false;
    // F9120 caller + cold chunk, checked again before saving callsite bytes.
    for(unsigned i=0;i<2;++i)if(!safe_copy(caller_bytes[i].data(),reinterpret_cast<void const*>(base+cert::calls[i]),cert::call_sizes[i]))return false;
    certified=true;return true;}
bool reserve(){if(!enabled)return true;if(reserved||!prepare())return false;
    for(unsigned i=0;i<hook_count();++i){targets[i]=reinterpret_cast<void*>(base+cert::entries[i]);std::array<unsigned char,32> a{},b{};
        if(!read(a,reinterpret_cast<uintptr_t>(targets[i]))||a!=cert::prefixes[i]||MH_CreateHook(targets[i],detours[i],&originals[i])!=MH_OK||!originals[i]||
            !read(b,reinterpret_cast<uintptr_t>(targets[i]))||a!=b)return false;}reserved=true;return true;}
bool reserved_target(void* t){if(!reserved)return false;for(unsigned i=0;i<hook_count();++i)if(t==targets[i])return true;return false;}
bool configure(wchar_t const* ini,FILE* f){
    wchar_t pb[8]{};GetPrivateProfileStringW(L"FreshPoseProducerObserver",L"experimental_per_bone_overwrite",L"0",pb,8,ini);
    if(wcscmp(pb,L"0")&&wcscmp(pb,L"1"))return false;
    per_bone_witness_enabled=!wcscmp(pb,L"1");
    wchar_t experimental[8]{};GetPrivateProfileStringW(L"FreshPoseProducerObserver",L"experimental_sequence_overwrite",L"0",experimental,8,ini);
    if(wcscmp(experimental,L"0")&&wcscmp(experimental,L"1"))return false;
    sequence_witness_enabled=!wcscmp(experimental,L"1");
    wchar_t b[8]{};GetPrivateProfileStringW(L"FreshPoseProducerObserver",L"enabled",L"0",b,8,ini);
    record_base_entry=GetPrivateProfileIntW(L"FreshPoseProducerObserver",L"record_base_entry",0,ini)==1;
    wchar_t t[8]{};GetPrivateProfileStringW(L"FreshPoseProducerObserver",L"record_call_tree",L"0",t,8,ini);
    if((wcscmp(t,L"0")&&wcscmp(t,L"1"))||(wcscmp(b,L"0")&&wcscmp(b,L"1"))||(!wcscmp(t,L"1")&&wcscmp(b,L"1")))return false;
    if(!wcscmp(b,L"1")){if(enabled||!hand::enabled||!phase::certified)return false;enabled=true;}record_call_tree=!wcscmp(t,L"1");
    if(f)fprintf(f,"fresh_pose_compiled=1 fresh_pose_enabled=%u fresh_pose_record_call_tree=%u writer=0 fresh_certified=0\n",unsigned(enabled),unsigned(record_call_tree));return true;}
void request_drain(){stopping=true;}
void summary(FILE* f){if(f){fprintf(f,"per_bone_overwrite_started=%llu per_bone_overwrite_finished=%llu ",per_bone_started.load(),per_bone_finished.load());
    for(unsigned i=0;i<per_bone_rejections.size();++i)fprintf(f,"per_bone_rejected_%u=%llu ",i,per_bone_rejections[i].load());}
base_entry_summary(f);if(f&&sequence_failure_known.load(std::memory_order_acquire)){const auto& d=sequence_failure;
    fprintf(f,"sequence_failure_context=%u sequence_failure_root_snapshot=%u sequence_failure_root_same=%u sequence_failure_root_epoch=%u sequence_failure_child_snapshot=%u sequence_failure_child_same=%u sequence_failure_child_return=%u sequence_failure_local_same=%u sequence_failure_desired_same=%u sequence_failure_root_before=%u sequence_failure_root_after=%u sequence_failure_mesh=%u sequence_failure_child_before=%u sequence_failure_child_after=%u sequence_failure_time_before=%u sequence_failure_time_after=%u sequence_failure_node70_before=%d sequence_failure_node70_after=%d sequence_failure_cache_before=%d sequence_failure_cache_after=%d sequence_failure_link_before=%d sequence_failure_link_after=%d ",
        d.context,d.root_snapshot,d.root_same,d.root_return_epoch,d.sequence_snapshot,d.sequence_same,d.sequence_return,d.local_same,d.desired_same,d.root_before,d.root_after,d.mesh,d.sequence_before,d.sequence_after,d.time_before,d.time_after,d.node70_before,d.node70_after,d.cache_before,d.cache_after,d.link_before,d.link_after);}
if(f)fprintf(f,"sequence_set_started=%llu sequence_set_finished=%llu sequence_overwrite_started=%llu sequence_overwrite_finished=%llu ",sequence_set_started.load(),sequence_set_finished.load(),sequence_started.load(),sequence_finished.load());if(f)for(unsigned i=0;i<blend_rejections.size();++i)fprintf(f,"root_blend_rejected_%u=%llu ",i,blend_rejections[i].load());std::lock_guard<std::mutex> lock(mutex);if(f)fprintf(f,"fresh_pose_enabled=%u fresh_pose_count=%u fresh_pose_pending=%u fresh_pose_inflight=%u fresh_pose_frames=%u fresh_pose_debt=%u fresh_pose_frozen=%u fresh_pose_failed=%u fresh_pose_written=%u ",unsigned(enabled),count,pending_count(),in_flight.load(),open_frames.load(),protocol_debt.load(),unsigned(frozen.load()),unsigned(failed.load()),unsigned(file_written.load()));tree_summary(f);}
void print(FILE* f,bool disabled){if(enabled&&disabled&&stopping.load()&&!pending_count()&&!file_attempted.exchange(true)){
    Header h{};memcpy(h.magic,"FPOSE001",8);h.version=1;h.header_size=sizeof h;h.record_size=sizeof(Record);h.count=count;h.capacity_value=capacity;h.read_only=1;
    h.session=uint64_t(session_started_qpc);h.module_base=base;h.attempts=attempts;h.child_capacity=child_limit;h.stage_count=5;h.packet_size=sizeof(pose::CallPacket);h.fifth_forwarded=1; // bit 0: Root only; Optional bit 1 remains zero.
    wchar_t path[MAX_PATH]{};if(directory[0]&&swprintf_s(path,L"%s\\fresh-pose-producer.bin",directory)>0){HANDLE out=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(out!=INVALID_HANDLE_VALUE){DWORD n{};bool ok=WriteFile(out,&h,sizeof h,&n,nullptr)&&n==sizeof h;const auto bytes=DWORD(count*sizeof(Record));
            if(ok&&bytes)ok=WriteFile(out,records.data(),bytes,&n,nullptr)&&n==bytes;if(!ok)file_error=GetLastError();if(!CloseHandle(out)){file_error=GetLastError();ok=false;}file_written=ok;
        }else file_error=GetLastError();}else file_error=ERROR_INVALID_NAME;
}tree_print(disabled);summary(f);}
#else
bool blend_world_current()noexcept{return false;}
unsigned pending_branches(uintptr_t,std::array<PendingBranch,16>& output) noexcept {output={};return 0;}
bool configure(wchar_t const* ini,FILE*){wchar_t b[8]{},t[8]{};GetPrivateProfileStringW(L"FreshPoseProducerObserver",L"enabled",L"0",b,8,ini);GetPrivateProfileStringW(L"FreshPoseProducerObserver",L"record_call_tree",L"0",t,8,ini);return !wcscmp(b,L"0")&&!wcscmp(t,L"0");}
bool reserve(){return true;}bool reserved_target(void*){return false;}void pose_begin(gears_continuous_hand_entry::Entry const&) noexcept {}void pose_end(gears_continuous_hand_entry::Entry const&,bool) noexcept {}
void request_drain(){}unsigned pending_count(){return 0;}void summary(FILE* f){if(f)fprintf(f,"fresh_pose_compiled=0 ");}void print(FILE* f,bool){summary(f);}
#endif
}
