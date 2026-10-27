// Private read-only producer -> C0 observer. No engine/getter/bridge calls.
#include "fresh_pose_producer_certificate.h"
namespace fresh_pose_producer_live {
bool enabled{};std::array<void*,2> targets{},originals{};bool own_entries();
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
bool certified{},reserved{};std::array<std::array<unsigned char,6>,2> caller_bytes{};
struct Call {Call* previous{};Call const* counter_self{};uint64_t serial{};unsigned thread{},counter_thread{},slot{capacity},kind{},tree_index{128};
    bool linked{},counter_owned{};};
struct Thread {Call* call{};unsigned depth{},slot{capacity};uint64_t tick{};Entry const* tick_address{};Stage repeat{};};
inline thread_local Thread local{};
extern "C" void GearsFreshRootHook();extern "C" void GearsFreshOptionalHook();
extern "C" void GearsFreshRawInvoke5(pose::CallPacket*,void*,uintptr_t) noexcept(false);
extern "C" void GearsFreshRawInvoke4(pose::CallPacket*,void*) noexcept(false);
const std::array<void*,2> detours{{reinterpret_cast<void*>(GearsFreshRootHook),reinterpret_cast<void*>(GearsFreshOptionalHook)}};
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
bool context(Record const& r){return hand::owner_current()&&hand::same_source()&&hand::slot.tick==r.tick&&
    hand::slot.tick_address==local.tick_address&&r.thread==GetCurrentThreadId()&&phase::same(r.owner,hand::slot.data.owner.key);}
#include "fresh_pose_call_tree.inl"
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
void finish(Call const& c,pose::CallPacket& p,bool returned){if(c.slot==capacity)return;auto& r=records[c.slot];auto& n=r.calls[c.kind];
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
void release(Call& c) noexcept {const bool counted=c.counter_self==&c&&c.counter_owned&&c.counter_thread==GetCurrentThreadId();
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
void finish_guarded(Call const& c,pose::CallPacket& p,bool returned) noexcept {
    const auto saved=p;try{tree_finish(c,returned);}catch(...){tree_fail();}
    if(record_call_tree&&memcmp(&p,&saved,sizeof p)){tree_packet_changed(c);p=saved;}
    try{finish(c,p,returned);}catch(...){debt();}
}
extern "C" __declspec(noinline) void GearsFreshInvokeProtected(pose::CallPacket* p,unsigned k,uintptr_t ra,uintptr_t rsp,uintptr_t fifth) noexcept(false){
    Call c{};bool returned{};acquire(c);
    __try{begin_guarded(*p,k,ra,rsp,k?0:fifth,c);
        if(!k)GearsFreshRawInvoke5(p,originals[0],fifth);else GearsFreshRawInvoke4(p,originals[1]);returned=true;}
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
    for(unsigned i:{6u,7u})if(!phase::digest(phase::certificate::chunks[i])||!phase::digest(phase::certificate::chunks[i]))return false;
    // F9120 caller + cold chunk, checked again before saving callsite bytes.
    for(unsigned i=0;i<2;++i)if(!safe_copy(caller_bytes[i].data(),reinterpret_cast<void const*>(base+cert::calls[i]),cert::call_sizes[i]))return false;
    certified=true;return true;}
bool reserve(){if(!enabled)return true;if(reserved||!prepare())return false;
    for(unsigned i=0;i<2;++i){targets[i]=reinterpret_cast<void*>(base+cert::entries[i]);std::array<unsigned char,32> a{},b{};
        if(!read(a,reinterpret_cast<uintptr_t>(targets[i]))||a!=cert::prefixes[i]||MH_CreateHook(targets[i],detours[i],&originals[i])!=MH_OK||!originals[i]||
            !read(b,reinterpret_cast<uintptr_t>(targets[i]))||a!=b)return false;}reserved=true;return true;}
bool reserved_target(void* t){return reserved&&(t==targets[0]||t==targets[1]);}
bool configure(wchar_t const* ini,FILE* f){wchar_t b[8]{};GetPrivateProfileStringW(L"FreshPoseProducerObserver",L"enabled",L"0",b,8,ini);
    wchar_t t[8]{};GetPrivateProfileStringW(L"FreshPoseProducerObserver",L"record_call_tree",L"0",t,8,ini);
    if((wcscmp(t,L"0")&&wcscmp(t,L"1"))||(wcscmp(b,L"0")&&wcscmp(b,L"1"))||(!wcscmp(t,L"1")&&wcscmp(b,L"1")))return false;
    if(!wcscmp(b,L"1")){if(enabled||!hand::enabled||!phase::certified)return false;enabled=true;}record_call_tree=!wcscmp(t,L"1");
    if(f)fprintf(f,"fresh_pose_compiled=1 fresh_pose_enabled=%u fresh_pose_record_call_tree=%u writer=0 fresh_certified=0\n",unsigned(enabled),unsigned(record_call_tree));return true;}
void request_drain(){stopping=true;}
void summary(FILE* f){std::lock_guard<std::mutex> lock(mutex);if(f)fprintf(f,"fresh_pose_enabled=%u fresh_pose_count=%u fresh_pose_pending=%u fresh_pose_inflight=%u fresh_pose_frames=%u fresh_pose_debt=%u fresh_pose_frozen=%u fresh_pose_failed=%u fresh_pose_written=%u ",unsigned(enabled),count,pending_count(),in_flight.load(),open_frames.load(),protocol_debt.load(),unsigned(frozen.load()),unsigned(failed.load()),unsigned(file_written.load()));tree_summary(f);}
void print(FILE* f,bool disabled){if(enabled&&disabled&&stopping.load()&&!pending_count()&&!file_attempted.exchange(true)){
    Header h{};memcpy(h.magic,"FPOSE001",8);h.version=1;h.header_size=sizeof h;h.record_size=sizeof(Record);h.count=count;h.capacity_value=capacity;h.read_only=1;
    h.session=uint64_t(session_started_qpc);h.module_base=base;h.attempts=attempts;h.child_capacity=child_limit;h.stage_count=5;h.packet_size=sizeof(pose::CallPacket);h.fifth_forwarded=1; // bit 0: Root only; Optional bit 1 remains zero.
    wchar_t path[MAX_PATH]{};if(directory[0]&&swprintf_s(path,L"%s\\fresh-pose-producer.bin",directory)>0){HANDLE out=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(out!=INVALID_HANDLE_VALUE){DWORD n{};bool ok=WriteFile(out,&h,sizeof h,&n,nullptr)&&n==sizeof h;const auto bytes=DWORD(count*sizeof(Record));
            if(ok&&bytes)ok=WriteFile(out,records.data(),bytes,&n,nullptr)&&n==bytes;if(!ok)file_error=GetLastError();if(!CloseHandle(out)){file_error=GetLastError();ok=false;}file_written=ok;
        }else file_error=GetLastError();}else file_error=ERROR_INVALID_NAME;
}tree_print(disabled);summary(f);}
#else
bool configure(wchar_t const* ini,FILE*){wchar_t b[8]{},t[8]{};GetPrivateProfileStringW(L"FreshPoseProducerObserver",L"enabled",L"0",b,8,ini);GetPrivateProfileStringW(L"FreshPoseProducerObserver",L"record_call_tree",L"0",t,8,ini);return !wcscmp(b,L"0")&&!wcscmp(t,L"0");}
bool reserve(){return true;}bool reserved_target(void*){return false;}void pose_begin(gears_continuous_hand_entry::Entry const&) noexcept {}void pose_end(gears_continuous_hand_entry::Entry const&,bool) noexcept {}
void request_drain(){}unsigned pending_count(){return 0;}void summary(FILE* f){if(f)fprintf(f,"fresh_pose_compiled=0 ");}void print(FILE* f,bool){summary(f);}
#endif
}
