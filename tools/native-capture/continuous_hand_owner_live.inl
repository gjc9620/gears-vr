// Continuous observation/planning only. Include after Rig/adapter/CCTRL.
#include "continuous_hand_caller_certificate.h"
#include "body_c0_arm_alpha_policy.h"
#include "body_tick_source_live.h"
#include "held_outcome_journal.h"
namespace continuous_hand_owner_live {
namespace budget=::gears_body_c0_budget;
using Entry=gears_continuous_hand_entry::Entry;
namespace phase=cover_pose_phase_live;namespace controls=cover_control_observer_live;
namespace rig=rig_tracking_live;namespace binding=weapon_hand_scene_binding;
namespace adapter=gears_hand_core_target;namespace mount=gears_hand_authored_mount;
namespace math=gears_cover_control_output_pose;namespace core=gears_native_rig_tracking;
bool requested{},enabled{};
#if defined(GEARS_CONTINUOUS_HAND_OWNER) && GEARS_CONTINUOUS_HAND_OWNER
static constexpr unsigned capacity=32;
enum class Failure:unsigned {none,stopped,owner,caller,lease,latch,nested,concurrent,compose,control,metadata,model,role,prepare,math,end,exception,exhausted};
struct Plan {uintptr_t node{};uint64_t ordinal{};unsigned side{},kind{},count{};adapter::Status prepare{},solve{};
    gears_vr::Pose wrist{};math::Matrix64 original[3]{},planned[3]{},component{};};
struct TickRecord {uint64_t tick{},compose{};int64_t begin_qpc{},end_qpc{};unsigned thread{},seen{},planned{};Failure failure{};
    bool returned{},complete{};gears_weapon_hand::ObjectIdentity asset{};mount::Signature geometry_signature{};
    gears_cover_pose_phase::ArrayHeader reference{};core::CoreSnapshot source{};Plan plans[4]{};};
struct Header {char magic[8]{};unsigned version{},header_size{},record_size{},count{},capacity_value{},read_only{},writer{},length_mode_value{};
    uint64_t ticks{},completed{},rejected{},preparations[2]{};};
struct Hand {adapter::ModelBinding binding{};adapter::Target target{};adapter::Status status{};bool attempted{},seen[2]{};};
struct Data {TickRecord record{};phase::Owner owner{};gears_anchor::Identity asset{};
    gears_cover_pose_phase::Metadata metadata{};std::array<mount::ReferenceRecord68,115> reference{};
    mount::CanonicalGeometry canonical{};mount::Signature signature{};
    Hand hands[2]{};adapter::LimbData limb{};adapter::SingleData single{};adapter::Workspace math_work{};};
struct Slot {
    Entry const* tick_address{};Entry const* compose_address{};Entry const* control_address{};
    uint64_t tick{},conflict{},ordinal{};unsigned tick_depth{},compose_depth{},control_depth{};
    rig::TickKey key{};rig::TickLatch latch{};rig::SceneMetadataLatch metadata_latch{};uint64_t metadata_revision{};core::CoreSnapshot verify{};controls::Compose compose{};controls::Row row{};Data data{};
    alignas(::Scope) unsigned char lease_storage[sizeof(::Scope)]{};::Scope* lease{};
    bool active{},attempted{},sticky{},control_valid{},hard_control_fault{},observer_source_expired{};
    Slot()=default;Slot(Slot const&)=delete;Slot& operator=(Slot const&)=delete;
};
inline thread_local Slot slot{};
std::atomic<bool> stopping{},failed{},caller_certified{},file_attempted{},file_written{};
std::atomic<unsigned> callbacks{},live_frames{},matching_ticks{};std::atomic<DWORD> file_error{};
std::atomic<uint64_t> serial{1},conflicts{},ticks{},completed{},rejected{},preparations[2]{};
std::mutex records_mutex;std::array<TickRecord,capacity> records{};uint64_t record_sequence{};unsigned record_count{};
using LengthMode=gears_hand_two_bone::LengthMode;LengthMode length_mode=LengthMode::preserve;
struct Call {Call(){++callbacks;}~Call(){--callbacks;}};
bool passive(){return !cover_body_live::enabled&&!cover_input_live::enabled&&!weapon_hand_runtime::detail::engine_restore_enabled;}
void reject(Failure why){slot.hard_control_fault=true;slot.sticky=true;if(slot.data.record.failure==Failure::none)slot.data.record.failure=why;++rejected;}
template<size_t N>bool code_bytes(uintptr_t rva,std::array<unsigned char,N> const& expected){
    if(base>UINTPTR_MAX-rva||!gears_cover_pose_phase::span(base+rva,N))return false;
    std::array<unsigned char,N> copy{};
    return safe_copy(copy.data(),reinterpret_cast<void const*>(base+rva),N)&&copy==expected&&
        safe_copy(copy.data(),reinterpret_cast<void const*>(base+rva),N)&&copy==expected;
}
bool certify_caller(){namespace c=continuous_hand_caller_certificate;
    // Existing production safe_copy + BCrypt SHA256, over each complete chunk,
    // twice independently. A failed read/hash/mismatch always rejects startup.
    for(auto const& chunk:c::chunks){const phase::certificate::Chunk expected{chunk.rva,chunk.size,chunk.sha256};
        if(!phase::digest(expected)||!phase::digest(expected))return false;}
    return code_bytes(0x12e527,c::callsite);
}
bool tick_target(uintptr_t mesh){uintptr_t table{},target{},again{};
    return caller_certified.load()&&phase::own_entries()&&controls::own_entries()&&
        code_bytes(0x12e527,continuous_hand_caller_certificate::callsite)&&controls::read(table,mesh)&&table&&
        gears_cover_pose_phase::span(table,0x268)&&controls::read(target,table+0x260)&&
        target==reinterpret_cast<uintptr_t>(phase::targets[2])&&controls::read(again,mesh)&&again==table&&
        controls::read(again,table+0x260)&&again==target;
}
bool owner_current(){auto const& f=slot;auto const& o=f.data.owner;
    return enabled&&!stopping.load()&&recording.load()&&passive()&&f.active&&!(body_tick_source_live::enabled?f.hard_control_fault:f.sticky)&&f.lease&&f.lease->world()&&
        f.tick_depth==1&&GetCurrentThreadId()==o.thread&&GetCurrentThreadId()==f.key.request.native.game_thread&&
        conflicts.load()==f.conflict&&matching_ticks.load()==1&&o.key.world_generation==committed_world_generation.load()&&
        o.key.session==uint64_t(session_started_qpc)&&budget::observe(budget::Op::owner_actor,[&]{return binding::current_actor(o.binding);})&&tick_target(o.key.mesh.address);
}
bool control_current(phase::Scope const& scope) noexcept {try{return owner_current()&&slot.compose_depth==1&&
    &scope==&slot.compose.scope&&scope.invocation==slot.data.record.compose&&scope.arguments;}catch(...){return false;}}
bool same_source(rig::TickFailure* failure=nullptr){
    if(failure)*failure=rig::TickFailure::hard;
    if(!slot.attempted)return false;
    rig::TickFailure why{};
    uint64_t revision{};
    const bool fresh=budget::observe(budget::Op::read_tick,[&]{return body_tick_source_live::enabled?
        rig::read_scene_metadata_tick(slot.key,slot.metadata_latch,slot.verify,revision,failure?&why:nullptr):
        rig::read_tick(slot.key,slot.latch,slot.verify,failure?&why:nullptr);});
    if(fresh&&body_tick_source_live::enabled&&revision!=slot.metadata_revision)return false;
    if(!fresh&&(!failure||why!=rig::TickFailure::source_age_only))return false;
    // Even an age failure must pass the complete immutable cohort and live
    // owner checks before the caller is allowed to call it age-only.
    if(memcmp(&slot.verify,&slot.data.record.source,sizeof(slot.verify))||!owner_current()){if(failure)slot.verify={};return false;}
    if(failure)*failure=why;
    return fresh;
}
// The read-only planner retains OLD tracking eligibility. Metadata mode never
// creates or revives a legacy TickLatch; current_copy classifies the unchanged
// original Core against its own original clocks and complete hard guards.
bool observer_same_source(rig::TickFailure* failure=nullptr){
    if(!body_tick_source_live::enabled)return same_source(failure);
    if(failure)*failure=rig::TickFailure::hard;
    if(!slot.attempted||!slot.metadata_revision||!owner_current())return false;
    // Full immutable-byte comparison is still required. A shape-valid edited
    // record must not bypass the original latch cohort just because it is old.
    rig::TickFailure metadata_why{};
    if(!same_source(&metadata_why)&&metadata_why!=rig::TickFailure::source_age_only)return false;
    rig::TickFailure why{};
    const bool fresh=budget::observe(budget::Op::read_tick,[&]{return rig::current_copy(slot.data.record.source,slot.metadata_revision,50000000,&why);});
    if(!owner_current()||(!fresh&&why!=rig::TickFailure::source_age_only))return false;
    if(failure)*failure=why;return fresh;
}
bool metadata(bool initial,controls::Row const* row=nullptr){budget::Span measured(initial?budget::Op::metadata_initial:budget::Op::metadata_repeat);auto& d=slot.data;
    auto reader=[](void* out,void const* in,size_t n){return safe_copy(out,in,n);};
    std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);if(!lock.owns_lock()||!binding::metadata_current())return false;
    gears_cover_pose_phase::Request request{};request.owner=d.owner.key;
    gears_cover_pose_phase::Metadata first{},last{};gears_anchor::Identity a{},b{};math::Matrix64 component{};
    if(!gears_cover_pose_phase::metadata(reader,request,first)||first.master||first.reference.count!=115||
       !gears_cover_pose_phase::header_valid(first.reference,68)||!gears_cover_pose_phase::header_valid(first.local,32)||
       !gears_cover_pose_phase::header_valid(first.space,64)||!gears_cover_pose_phase::header_valid(first.desired,1,true)||
       first.local.count!=115||first.space.count!=115||first.desired.count>115||
       !binding::identity(first.asset,a,true))return false;
    memcpy(&component,&first.component,64);if(!math::detail::rigid(math::detail::unpack(component)))return false;
    const auto owned=reinterpret_cast<uintptr_t>(&slot);
    if(first.reference.data<=owned+sizeof(slot)-1&&owned<=first.reference.data+sizeof(d.reference)-1)return false;
    if(initial){if(!safe_copy(d.reference.data(),reinterpret_cast<void const*>(first.reference.data),sizeof(d.reference)))return false;}
    else if(!binding::same(a,d.asset)||!controls::same_metadata(first,d.metadata))return false;
    if(gears_cover_pose_phase::repeat(reader,d.reference.data(),first.reference.data,sizeof(d.reference))!=gears_cover_pose_phase::Reject::none||
       !gears_cover_pose_phase::metadata(reader,request,last)||!controls::same_metadata(first,last)||
       !binding::identity(first.asset,b,true)||!binding::same(a,b)||!binding::metadata_current())return false;
    if(row&&(!controls::same_metadata(first,row->before.metadata)||!controls::same_metadata(first,row->after.metadata)))return false;
    if(initial){d.asset=a;d.metadata=first;}return true;
}
bool geometry(){budget::Span measured(budget::Op::geometry);auto& d=slot.data;if(!mount::canonical_geometry(d.reference.data(),d.reference.size(),d.canonical))return false;
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};bool ok=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=BCryptHashData(hash,d.canonical.data(),DWORD(d.canonical.size()),0)>=0;
    if(ok)ok=BCryptFinishHash(hash,d.signature.data(),DWORD(d.signature.size()),0)>=0;
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    return ok&&d.signature==mount::marcus115_signature;
}
bool next(Entry& e){e.thread=GetCurrentThreadId();e.invocation=serial.fetch_add(1);
    if(!e.invocation||e.invocation==UINT64_MAX){stopping=true;failed=true;return false;}return true;}
void tick_begin(Entry& e){e.tick_depth=true;e.depth=++slot.tick_depth;
    if(slot.active){reject(Failure::nested);return;}if(e.depth!=1||stopping.load())return;
    binding::Binding actor{};rig::CoreReadRequest request{};
    if(!(body_tick_source_live::enabled?rig::copy_scene_metadata_owner(actor,request):rig::copy_scene_owner(actor,request))||e.rcx!=actor.mesh.address||e.thread!=request.native.game_thread)return;
    // Only cached owner + code here. Actor/vtable reads require the lease below.
    if(e.ra!=base+0x12e537||!caller_certified.load()||!phase::own_entries()||!controls::own_entries()){++rejected;return;}
    slot.active=true;e.selected=true;slot.tick_address=&e;slot.tick=e.invocation;++live_frames;++ticks;
    slot.sticky=false;slot.hard_control_fault=false;slot.observer_source_expired=false;slot.attempted=false;slot.metadata_revision=0;slot.data={};slot.compose={};slot.ordinal=0;
    slot.data.record.tick=e.invocation;slot.data.record.thread=e.thread;slot.data.record.begin_qpc=phase::qpc();
    auto& owner=slot.data.owner;owner.binding=actor;owner.thread=e.thread;owner.valid=true;
    auto& k=owner.key;k.world=phase::compact(actor.world);k.pawn=phase::compact(actor.pawn);k.mesh=phase::compact(actor.mesh);
    k.session=request.native.native_started_qpc;k.world_generation=actor.world_generation;k.scene=request.scene_invocation;k.scene_qpc=request.scene_qpc;
    slot.key.request=request;slot.key.outer_tick=e.invocation;slot.conflict=conflicts.load();
    if(matching_ticks.fetch_add(1)){++conflicts;reject(Failure::concurrent);return;}
    slot.lease=::new(static_cast<void*>(slot.lease_storage)) ::Scope;
    if(!slot.lease->world()){reject(Failure::lease);return;}
    if(!owner_current()){reject(Failure::owner);return;}
    slot.attempted=true;
    if(body_tick_source_live::enabled){
        if(!rig::begin_scene_metadata_tick(slot.key,slot.metadata_latch)||
           !rig::read_scene_metadata_tick(slot.key,slot.metadata_latch,slot.data.record.source,slot.metadata_revision)||!owner_current())reject(Failure::latch);
        else body_tick_source_live::begin(e.invocation,e.thread,slot.data.record.source,slot.metadata_revision);
    }else{
        if(!rig::begin_tick(slot.key,slot.latch)||!rig::read_tick(slot.key,slot.latch,slot.data.record.source)||!owner_current())reject(Failure::latch);
        else {body_tick_tracking_probe_live::sample(e.invocation,e.thread,slot.data.record.source);
            body_tick_input_live::begin(e.invocation,e.thread,slot.data.record.source);}
    }
}
void compose_begin(Entry& e){if(!slot.active)return;
    gears_body_held_outcome::recorder.begin(reinterpret_cast<uintptr_t>(&e),e.invocation,slot.tick,e.thread,slot.data.record.source);
    budget::recorder.begin(reinterpret_cast<uintptr_t>(&e),e.invocation,slot.tick,slot.data.record.begin_qpc,e.thread,slot.data.record.source);
    e.compose_depth=true;e.depth=++slot.compose_depth;
    if(e.depth!=1||slot.compose_address||slot.data.record.compose){reject(Failure::compose);return;}
    slot.compose_address=&e;e.selected=true;slot.compose.scope.owner=slot.data.owner;slot.compose.scope.invocation=e.invocation;
    slot.compose.scope.kind=0;slot.compose.scope.caller=gears_cover_pose_phase::compose_caller;
    slot.compose.scope.arguments=e.ra==base+gears_cover_pose_phase::compose_caller&&e.rcx==slot.data.owner.key.mesh.address&&
        e.rdx==e.rcx+0x27c&&e.r8==e.rcx+0x28c;
    // CCTRL continuous validation consumes this exact canonical Scope address.
    slot.compose.address=&slot.compose.scope;
    slot.compose.continuous=true;slot.compose.admitted=true;slot.compose.outer_tick=slot.tick;slot.data.record.compose=e.invocation;
    if(!control_current(slot.compose.scope)||!same_source()||!metadata(true)||!geometry())reject(Failure::metadata);
    else{slot.data.record.asset=rig::object(slot.data.asset);slot.data.record.geometry_signature=slot.data.signature;slot.data.record.reference=slot.data.metadata.reference;budget::recorder.admitted();}
}
void pose_begin(Entry& e) noexcept {Call call;try{if(!enabled||!next(e))return;if(e.kind==2)tick_begin(e);else if(e.kind==0)compose_begin(e);}catch(...){failed=true;reject(Failure::exception);}}
void save_record(){std::lock_guard<std::mutex> lock(records_mutex);records[record_sequence%capacity]=slot.data.record;
    ++record_sequence;if(record_count<capacity)++record_count;}
void pose_end(Entry const& e,bool returned) noexcept {Call call;
    if(e.kind==0){budget::recorder.finish(reinterpret_cast<uintptr_t>(&e),e.invocation,returned);
        gears_body_held_outcome::recorder.finish(reinterpret_cast<uintptr_t>(&e),e.invocation,returned);}
    try{
    if(e.compose_depth){if(!slot.compose_depth||slot.compose_depth!=e.depth){failed=true;reject(Failure::end);return;}
        if(e.selected&&slot.compose_address!=&e){failed=true;reject(Failure::end);return;}
        if(slot.compose_address==&e){if(!returned)reject(Failure::exception);slot.compose_address=nullptr;slot.compose.admitted=false;slot.compose.address=nullptr;}
        --slot.compose_depth;}
    if(!e.tick_depth)return;
    if(!slot.tick_depth||slot.tick_depth!=e.depth){failed=true;reject(Failure::end);return;}
    if(e.selected){
        if(!slot.active||slot.tick_address!=&e||slot.tick!=e.invocation||GetCurrentThreadId()!=slot.key.request.native.game_thread||
           slot.compose_depth||slot.control_depth||slot.compose_address||slot.control_address){failed=true;reject(Failure::end);return;}
        if(!returned)reject(Failure::exception);if(stopping.load())reject(Failure::stopped);
        if(body_tick_source_live::enabled){
            if(slot.attempted&&!slot.hard_control_fault&&!same_source())reject(Failure::latch);
            const bool ended=!slot.attempted||rig::end_scene_metadata_tick(slot.key,slot.metadata_latch);
            body_tick_source_live::end(e.invocation);slot.metadata_revision=0;
            if(!ended){failed=true;reject(Failure::end);return;}
        }else{
            if(slot.attempted&&!slot.sticky&&!same_source())reject(Failure::latch);
            if(slot.attempted&&!rig::end_tick(slot.key,slot.latch)){failed=true;reject(Failure::end);return;}
            body_tick_tracking_probe_live::end_tick(e.invocation);body_tick_input_live::end(e.invocation);
        }
        auto& r=slot.data.record;r.end_qpc=phase::qpc();r.returned=returned;r.complete=returned&&!slot.sticky&&r.seen==15&&r.planned==15;
        if(r.complete)++completed;
        if(slot.lease){slot.lease->~Scope();slot.lease=nullptr;}
        slot.attempted=false;slot.active=false;slot.tick_address=nullptr;slot.control_address=nullptr;slot.key={};
        --matching_ticks;--live_frames;--slot.tick_depth;save_record();return;
    }
    --slot.tick_depth;
}catch(...){failed=true;reject(Failure::exception);}}
bool observe_source(Failure failure){
    // Legacy checks retain their original call pattern. Only a positively
    // classified observational expiry in an exact consumed typed scope differs.
    if(!body_tick_source_live::enabled&&!body_tick_input_live::enabled){if(same_source())return true;reject(failure);return false;}
    rig::TickFailure why{};if(observer_same_source(&why))return true;
    if(why==rig::TickFailure::source_age_only&&
       full_body_c0_native_live::retained_control_current(slot.compose.scope,slot.tick)==gears_body_c0_arm_alpha::RetainedControl::verified){
        slot.observer_source_expired=true;slot.sticky=true;
        if(slot.data.record.failure==Failure::none)slot.data.record.failure=failure;++rejected;return false;
    }
    reject(failure);return false;
}
void control_begin(Entry& e) noexcept {Call call;try{
    if(!enabled||!slot.active||e.kind>1||!next(e))return;
    e.control_depth=true;e.depth=++slot.control_depth;
    if(e.depth!=1||slot.control_address){reject(Failure::nested);return;}
    slot.control_address=&e;e.selected=true;slot.control_valid=false;
    if(slot.observer_source_expired){
        // The original age failure burned the old latch. Keep structural
        // begin/end plus the ORIGINAL before/after node/stack validators and
        // independently authenticate hard consumer boundaries;
        // do not query/revive that source again or run its hypothetical plan.
        if(full_body_c0_native_live::retained_control_current(slot.compose.scope,slot.tick)!=gears_body_c0_arm_alpha::RetainedControl::verified){reject(Failure::owner);return;}
    }else{
        if(!control_current(slot.compose.scope)){reject(Failure::latch);return;}
        if(!observe_source(Failure::latch)){
            if(!slot.observer_source_expired||slot.hard_control_fault||
               full_body_c0_native_live::retained_control_current(slot.compose.scope,slot.tick)!=gears_body_c0_arm_alpha::RetainedControl::verified)return;
        }
    }
    auto& row=slot.row;row={};row.owner=slot.data.owner.key;row.compose=slot.data.record.compose;row.outer_tick=slot.tick;
    row.ordinal=row.compute_ordinal=++slot.ordinal;row.kind=e.kind;row.thread=e.thread;
    row.node_argument=e.rcx;row.raw_rdx=e.rdx;row.mesh_argument=e.r8;row.out_argument=e.r9;row.entry_rsp=e.rsp;row.caller=e.ra;row.before_qpc=phase::qpc();
    row.failure=controls::before(slot.compose,row);row.before_valid=row.failure==controls::Failure::okay;
    slot.control_valid=row.before_valid;if(!slot.control_valid)reject(Failure::control);
}catch(...){failed=true;reject(Failure::exception);}}
void plan(){auto& d=slot.data;auto const& row=slot.row;auto const& source=d.record.source;
    const unsigned side=row.raw_rdx==11?0u:row.raw_rdx==35?1u:2u;if(side>1){reject(Failure::role);return;}
    const unsigned index=side*2+row.kind;auto& out=d.record.plans[index];auto& hand=d.hands[side];
    if(hand.seen[row.kind]){reject(Failure::role);return;}hand.seen[row.kind]=true;d.record.seen|=1u<<index;
    out.node=row.node_argument;out.ordinal=row.ordinal;out.side=side;out.kind=row.kind;out.count=row.kind?1u:3u;
    memcpy(out.original,row.matrices.data(),out.count*64);memcpy(&out.component,&row.before.metadata.component,64);
    if(!metadata(false,&row)){reject(Failure::metadata);return;}if(!observe_source(Failure::metadata))return;
    if(!hand.attempted){hand.attempted=true;++preparations[side];auto& b=hand.binding;b.native=source.native;b.world=source.world;b.pawn=source.pawn;b.mesh=source.mesh;
        b.asset=rig::object(d.asset);b.world_generation=source.world_generation;b.binding_generation=source.binding_generation;b.model=mount::Model::marcus115;
        b.geometry_signature=d.signature;b.bone_count=115;b.model_to_world_scale=1;
        hand.status=mount::marcus115(d.signature,static_cast<adapter::Side>(side),1,source.units_per_metre,b.authored_mount)?
            adapter::prepare_hand(source,b,static_cast<adapter::Side>(side),rig::clock_ns(),50000000,simulator_operator,{false},hand.target):adapter::Status::unknown_mount;
    }
    out.prepare=hand.status;if(hand.status!=adapter::Status::okay){reject(Failure::prepare);return;}
    auto const& profile=hand.binding.authored_mount;
    for(unsigned i=0;i<out.count;++i)if(uint32_t(row.affected[i])!=(row.kind?profile.hand_bone:profile.chain[i])){reject(Failure::role);return;}
    if(!adapter::same_source(hand.target,source)){reject(Failure::prepare);return;}out.wrist=hand.target.wrist_world;
    if(!row.kind){d.limb={};d.limb.asset=hand.binding.asset;d.limb.bone_count=115;d.limb.affected_count=3;d.limb.affected=profile.chain;
        memcpy(d.limb.component,out.original,192);d.limb.component_to_world=out.component;
        out.solve=adapter::solve_limb(hand.target,d.limb,length_mode,d.math_work,out.planned);
    }else{d.single={};d.single.asset=hand.binding.asset;d.single.bone_count=115;d.single.affected_count=1;d.single.affected=profile.hand_bone;
        d.single.component=out.original[0];d.single.component_to_world=out.component;out.solve=adapter::replace_wrist_rotation(hand.target,d.single,d.math_work,out.planned);}
    if(out.solve!=adapter::Status::okay){reject(Failure::math);return;}
    if(!metadata(false,&row)){memset(out.planned,0,sizeof(out.planned));reject(Failure::metadata);return;}
    if(!observe_source(Failure::metadata)){memset(out.planned,0,sizeof(out.planned));return;}
    d.record.planned|=1u<<index;
}
void control_end(Entry const& e,bool returned) noexcept {Call call;
    if(!e.control_depth)return;if(!slot.control_depth||slot.control_depth!=e.depth){failed=true;reject(Failure::end);return;}
    if(e.selected&&slot.control_address!=&e){failed=true;reject(Failure::end);return;}
    const bool owned=slot.control_address==&e;
    try{if(owned){
        if(!returned)reject(Failure::exception);
        if(returned&&slot.control_valid&&!failed.load()&&!slot.hard_control_fault&&(!slot.sticky||slot.observer_source_expired)){auto& row=slot.row;row.returned=true;row.after_qpc=phase::qpc();row.failure=controls::after(slot.compose,row);
            row.after_valid=row.failure==controls::Failure::okay;if(!row.after_valid)reject(Failure::control);else if(!slot.observer_source_expired)plan();}
    }}catch(...){failed=true;reject(Failure::exception);}
    if(owned){slot.control_address=nullptr;slot.control_valid=false;}--slot.control_depth;
}
bool preconfigure(wchar_t const* ini) noexcept {wchar_t value[8]{};GetPrivateProfileStringW(L"ContinuousHandOwner",L"enabled",L"0",value,8,ini);
    if(wcscmp(value,L"0")&&wcscmp(value,L"1"))return false;requested=!wcscmp(value,L"1");return true;}
bool configure(wchar_t const* ini,FILE* file){wchar_t mode[16]{};GetPrivateProfileStringW(L"ContinuousHandOwner",L"length_mode",L"preserve",mode,16,ini);
    if(wcscmp(mode,L"preserve")&&wcscmp(mode,L"adaptive"))return false;
    if(requested){if(enabled||stopping.load()||!rig::enabled||!weapon_hand_runtime::enabled||!continuous_session||!stereo_xr||!anchor_observer_enabled||
        !passive()||!phase::certified||!controls::certified||!certify_caller())return false;
        length_mode=!wcscmp(mode,L"adaptive")?LengthMode::adaptive:LengthMode::preserve;caller_certified=true;enabled=true;}
    if(file)fprintf(file,"continuous_hand_enabled=%u caller_certified=%u continuous_hand_length_mode=%u read_only=1 writer=0 event_required=0 c0_snapshot_required=0\n",unsigned(enabled),unsigned(caller_certified.load()),unsigned(length_mode));return true;}
void request_drain(){stopping=true;}unsigned pending_count(){return callbacks.load()+live_frames.load();}
void summary(FILE* file){if(file)fprintf(file,"continuous_hand_enabled=%u continuous_hand_ticks=%llu continuous_hand_completed=%llu continuous_hand_rejected=%llu continuous_hand_prepare_left=%llu continuous_hand_prepare_right=%llu continuous_hand_pending=%u continuous_hand_failed=%u continuous_hand_file_written=%u continuous_hand_file_error=%lu ",unsigned(enabled),ticks.load(),completed.load(),rejected.load(),preparations[0].load(),preparations[1].load(),pending_count(),unsigned(failed.load()),unsigned(file_written.load()),file_error.load());}
void print(FILE* file,bool disabled){if(enabled&&disabled&&stopping.load()&&!pending_count()&&!file_attempted.exchange(true)){
    std::lock_guard<std::mutex> lock(records_mutex);Header header{};memcpy(header.magic,"CHOWNER1",8);header.version=1;header.header_size=sizeof(Header);header.record_size=sizeof(TickRecord);
    header.count=record_count;header.capacity_value=capacity;header.read_only=1;header.length_mode_value=unsigned(length_mode);header.ticks=ticks;header.completed=completed;header.rejected=rejected;header.preparations[0]=preparations[0];header.preparations[1]=preparations[1];
    wchar_t path[MAX_PATH]{};if(directory[0]&&swprintf_s(path,L"%s\\continuous-hand-plans.bin",directory)>0){HANDLE output=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(output==INVALID_HANDLE_VALUE)file_error=GetLastError();else{DWORD written{};bool ok=WriteFile(output,&header,sizeof(header),&written,nullptr)&&written==sizeof(header);
            const uint64_t begin=record_sequence-record_count;for(unsigned i=0;ok&&i<record_count;++i){auto const& record=records[(begin+i)%capacity];ok=WriteFile(output,&record,sizeof(record),&written,nullptr)&&written==sizeof(record);}
            if(!ok)file_error=GetLastError()?GetLastError():ERROR_WRITE_FAULT;if(!CloseHandle(output)){file_error=GetLastError();ok=false;}file_written=ok;}
    }else file_error=ERROR_INVALID_NAME;
}summary(file);}
#else
bool preconfigure(wchar_t const* ini) noexcept {return GetPrivateProfileIntW(L"ContinuousHandOwner",L"enabled",0,ini)==0;}
bool configure(wchar_t const*,FILE*){return true;}
void pose_begin(Entry&) noexcept {}void pose_end(Entry const&,bool) noexcept {}
void control_begin(Entry&) noexcept {}void control_end(Entry const&,bool) noexcept {}
bool control_current(phase::Scope const&) noexcept {return false;}
void request_drain(){}unsigned pending_count(){return 0;}void summary(FILE*){}void print(FILE*,bool){}
#endif
} // namespace continuous_hand_owner_live
