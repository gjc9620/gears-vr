// Actual live ActualContext/pose_begin/before_original/pose_end plus real
// Backend read/CAS, Kernel and EpochGate. Hand/actor/certificate facts are Host
// seams, not game proof. The actor reader has its own actual-reader test.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "weapon_hand_broker.h"
#define main PriorNativeBackendMain
#include "full_body_c0_native_backend_test.cpp"
#undef main
#include "anchor_observer.h"
#include "continuous_hand_owner_entry.h"
#include "../vr-common/native_rig_tracking_frame.h"
#include "body_c0_hands.h"
#include "../vr-common/hand_part_grasp.h"
#include "full_body_c0_native_live.h"
#include "fresh_pose_producer.h"
#include "body_tick_tracking_probe.h"
#include "world_epoch_gate.h"
#include <atomic>
#include <mutex>
#define GEARS_FULL_BODY_C0_NATIVE_BACKEND 1
uintptr_t base=0x140000000;
wchar_t directory[MAX_PATH]{};
std::atomic<bool> recording{true};std::atomic<uint64_t> committed_world_generation{444};int64_t session_started_qpc=333;
gears_world::EpochGate world_gate;thread_local gears_world::ThreadState world_thread;
struct Scope {gears_world::RecursiveAdmission admission{world_thread,world_gate};bool valid=true;
    bool world()const{return valid&&bool(admission);}};
// Real reflection types and helper bodies; host boundaries are named below.
bool safe_copy(void*,void const*,size_t);
gears_anchor::Config anchor_config;
#define GEARS_COVER_BODY_PROTOTYPE 1
#define sample HostUnusedBindingSample
#define sample_visual HostUnusedBindingSampleVisual
#define metadata_current HostUnusedMetadataCurrent
#define identity HostUnusedIdentity
#define current_actor HostUnusedCurrentActor
#define current_actor_storage HostUnusedCurrentActorStorage
#define current_actor_retained HostUnusedCurrentActorRetained
#define property HostUnusedProperty
#define current HostUnusedProofCurrent
#define begin_prepare HostUnusedBeginPrepare
#define field HostUnusedField
#define source_field HostUnusedSourceField
#include "weapon_hand_scene_binding.inl"
#undef sample
#undef sample_visual
#undef metadata_current
#undef identity
#undef current_actor
#undef current_actor_storage
#undef current_actor_retained
#undef property
#undef current
#undef begin_prepare
#undef field
#undef source_field
bool actor_valid=true,metadata_valid=true,entries_valid=true,caller_valid=true,latch_fresh=true;
bool latch_age_only{};unsigned source_expire_after=UINT_MAX;
extern bool simulator_operator;
bool safe_copy(void*,void const*,size_t);
namespace weapon_hand_runtime {
struct Session {uint32_t game_pid{};uint64_t game_start{},native_started_qpc{};};
Session session();gears_weapon_hand::Broker* current();
gears_weapon_hand::MainMountQueryResult query_main_mount(gears_rig_tracking::Frame const&,
    gears_weapon_main_mount_provider::BodyGeometry const&,gears_weapon_main_mount_provider::Response&) noexcept;
gears_weapon_hand::MainMountQueryResult query_body_mount(gears_body_main_mount_provider::Source const&,
    gears_body_main_mount_provider::Response&) noexcept;
}
namespace rig_tracking_live {
enum class TickFailure {none,source_age_only,hard};
int64_t clock_ns();bool source_fresh(int64_t,int64_t,uint64_t);
bool reference_snapshot(gears_rig_tracking::NativeReference&,uint64_t&);
struct MetadataReject {int64_t at_ns{},scene_qpc{},frequency{};uint64_t published_ns{},publication{},revision{};unsigned stage{},reason{};};
std::array<MetadataReject,128> metadata_rejects{};
std::atomic<unsigned> metadata_reject_count{};bool metadata_reject_trace{};
gears_weapon_hand::ObjectIdentity object(gears_anchor::Identity const& id){return id.address?
    gears_weapon_hand::ObjectIdentity{id.address,id.outer,id.cls,id.index,id.name_index,id.number,1}:gears_weapon_hand::ObjectIdentity{};}
}
unsigned owner_calls{},source_calls{},retained_actor_calls{};
namespace weapon_hand_scene_binding {
bool current_actor_retained(Binding const&){++retained_actor_calls;return actor_valid;}
// New Native retirement code is included below. Implementations remain in the
// Goal Host test, after its synthetic actor/asset values have been declared.
extern std::mutex mutex;
bool metadata_current();
bool identity(uintptr_t,gears_anchor::Identity&,bool=false,bool=false);
bool current_actor(Binding const&);
bool property(Proof&,uintptr_t,wchar_t const*,wchar_t const*,unsigned,wchar_t const*,Property&);
bool begin_prepare(Proof&);bool current(Proof const&);
template<class T>bool source_field(Proof&,T&,uintptr_t,unsigned);
template<class T>bool field(T&,uintptr_t,unsigned);
bool sample(gears_anchor::Receipt const*,Binding&);
bool sample_visual(gears_anchor::Receipt const*,Binding&){return false;}
bool current_actor_storage(Binding const&);
}
namespace cover_pose_phase_live {
struct Owner {weapon_hand_scene_binding::Binding binding{};p::OwnerKey key{};unsigned thread{};bool valid{};};
struct Scope {Owner owner{};uint64_t invocation{};bool arguments{};unsigned kind{};};
bool same(p::OwnerKey const&a,p::OwnerKey const&b){return !std::memcmp(&a,&b,sizeof a);}
bool own_entries(){return entries_valid;}
int64_t qpc(){LARGE_INTEGER value{};return QueryPerformanceCounter(&value)?value.QuadPart:0;}
}
namespace fresh_pose_producer_live {bool enabled=true,certified=true;
unsigned pending_branches(uintptr_t,std::array<PendingBranch,16>&)noexcept{return 0;}
bool own_entries(){return entries_valid;}bool caller_current(unsigned){return caller_valid;}}
namespace continuous_hand_owner_live {
using Entry=gears_continuous_hand_entry::Entry;
bool enabled=true;std::atomic<bool> stopping{},failed{};std::atomic<uint64_t> conflicts{};std::atomic<unsigned> matching_ticks{1},callbacks{},live_frames{};
bool passive(){return true;}
struct Slot {
    Entry const*tick_address{},*compose_address{},*control_address{};uint64_t tick{},conflict{};unsigned tick_depth{},compose_depth{},control_depth{};
    struct Key {struct Request {gears_rig_tracking::NativeKey native;}request;uint64_t outer_tick{};}key;
    struct Compose {cover_pose_phase_live::Scope scope{};uint64_t outer_tick{};bool admitted{};}compose;
    struct Data {cover_pose_phase_live::Owner owner{};gears_anchor::Identity asset{};
        struct Record {uint64_t tick{},compose{};gears_native_rig_tracking::CoreSnapshot source{};}record;
        body::rig::Signature signature{};}data;
    ::Scope*lease{};bool active{},sticky{},hard_control_fault{},observer_source_expired{};
};
thread_local Slot slot;
bool tick_target(uintptr_t){return entries_valid;}
void (*metadata_hook)(){};
bool metadata(bool){if(metadata_hook)metadata_hook();return metadata_valid;}
bool owner_current(){++owner_calls;return !stopping.load()&&recording.load()&&!slot.sticky&&actor_valid;}
// Host identity seam only; production uses the existing continuous owner.
bool control_current(cover_pose_phase_live::Scope const& scope)noexcept {
    return owner_current()&&slot.compose_depth==1&&&scope==&slot.compose.scope&&scope.arguments;
}
bool same_source(rig_tracking_live::TickFailure* failure=nullptr){++source_calls;
    const bool owner=owner_current(),age=latch_age_only||source_calls>source_expire_after;
    if(failure)*failure=!owner||!latch_fresh?rig_tracking_live::TickFailure::hard:age?
        rig_tracking_live::TickFailure::source_age_only:rig_tracking_live::TickFailure::none;
    return owner&&latch_fresh&&!age;}
}
namespace cover_control_observer_live {
std::atomic<bool> stopping{},failed{};
bool retained_boundary_current()noexcept{return !stopping.load()&&!failed.load()&&entries_valid;}
}
#include "first_person_view.h"
namespace first_person_live {bool body_basis{};gears_first_person::Toggle toggle;
bool authored_eye(gears_anchor::Receipt const*,gears_vr::Vector&){return false;}}
#include "lancer_grasp_live.inl"
#include "full_body_c0_native_live.inl"
namespace live=full_body_c0_native_live;namespace hand=continuous_hand_owner_live;
using Entry=gears_continuous_hand_entry::Entry;
void original_return(Entry const&e){bool returned=false,called=false;__try{live::before_original(e);called=true;returned=true;}
    __finally{live::pose_end(e,returned,called);}}
void original_seh_inner(Entry const&e){bool returned=false,called=false;__try{live::before_original(e);called=true;RaiseException(0xe0424242,0,0,nullptr);}
    __finally{live::pose_end(e,returned,called);}}
bool original_seh(Entry const&e){__try{original_seh_inner(e);return false;}__except(GetExceptionCode()==0xe0424242?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}}
void original_cpp(Entry const&e){bool returned=false,called=false;struct Finally {Entry const&e;bool&returned;bool&called;
    ~Finally(){live::pose_end(e,returned,called);}}finally{e,returned,called};live::before_original(e);called=true;throw 73;}
