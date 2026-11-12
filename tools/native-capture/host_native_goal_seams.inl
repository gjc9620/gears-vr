// Shared offline Host boundaries; no game, engine, tracking or overwrite certificate.
#include "body_c0_neutral.h"
#include "cover_c0_committed_adapter.h"
namespace cover_full_goal_live {
gears_cover_committed::State host_drag{};bool host_available{},host_body_owner{};
bool read_committed(gears_cover_committed::State& out){if(!host_available)return false;out=host_drag;return true;}
bool set_body_owner(bool enabled){host_body_owner=enabled;return false;}
}
bool simulator_operator=true;
bool safe_copy(void* out,void const* in,size_t size){return n::read_memory(out,reinterpret_cast<uintptr_t>(in),size);}
unsigned host_cover{},host_crouched{},host_wants{};bool host_property=true;
int64_t host_now{};gears_native_rig_tracking::CoreSnapshot host_core{};
weapon_hand_scene_binding::Binding host_actor{};gears_anchor::Identity host_asset{};
namespace weapon_hand_scene_binding {

bool metadata_current(){return metadata_valid;}bool current_actor(Binding const& b){return actor_valid&&recording&&same_key(b,host_actor)&&b.special_move==host_actor.special_move;}
bool identity(uintptr_t address,gears_anchor::Identity& out,bool,bool){out=host_asset;if(address!=host_asset.address){out={};out.address=address;}return metadata_valid;}
bool current_actor_storage(Binding const& b){return actor_valid&&gears_anchor::equal_identity(b.world,host_actor.world)&&
    gears_anchor::equal_identity(b.controller,host_actor.controller)&&gears_anchor::equal_identity(b.pawn,host_actor.pawn)&&
    gears_anchor::equal_identity(b.mesh,host_actor.mesh)&&b.world_generation==host_actor.world_generation;}
bool begin_prepare(Proof&){return host_property;}bool current(Proof const&){return host_property;}
bool property(Proof&,uintptr_t,wchar_t const* name,wchar_t const*,unsigned,wchar_t const*,Property& p){
    p.offset=!wcscmp(name,L"CoverType")?1u:!wcscmp(name,L"bIsCrouched")?2u:3u;p.id.address=p.offset;return host_property;}
template<class T>bool source_field(Proof&,T& out,uintptr_t,unsigned){out=T(1);return host_property;}
template<class T>bool field(T& out,uintptr_t,unsigned off){
    if constexpr(std::is_integral_v<T>){out=T(off==1?host_cover:off==2?host_crouched:host_wants);return host_property;}
    else return false; // This Host seam has no synthetic array/matrix authorization.
}
bool sample(gears_anchor::Receipt const*,Binding& out){out=host_actor;return actor_valid;}
}
namespace cover_pose_phase_live {p::Identity compact(gears_anchor::Identity const& i){p::Identity out{};out.address=i.address;return out;}}
namespace weapon_hand_runtime {
Session session(){return {host_core.native.game_pid,host_core.native.game_start,host_core.native.native_started_qpc};}gears_weapon_hand::Broker* current(){return nullptr;}}
namespace rig_tracking_live {
int64_t clock_ns(){return host_now;}
bool source_fresh(int64_t stamp,int64_t now,uint64_t age){return stamp>0&&now>=stamp&&uint64_t(now-stamp)<=age;}
bool native_session_current(void*,weapon_hand_runtime::Session const&){return actor_valid;}
bool retained_core_identity(gears_native_rig_tracking::CoreSnapshot const&)noexcept{return actor_valid&&metadata_valid;}
bool reference_snapshot(gears_rig_tracking::NativeReference& out,uint64_t& revision){out=host_core.native_reference;revision=host_core.native_reference_revision;return metadata_valid;}
}

namespace cover_shared_input_live {std::atomic<uint64_t> goal_absent_epoch{};}
// Current-object admission is a Host seam, while CurrentKey and comparison are real.
#define sample_current HostUnusedCoverSampleCurrent
#include "cover_context_live.inl"
#undef sample_current
namespace cover_context_live {
unsigned host_calls{},host_link_delta{},host_slot_delta{},host_action{};bool host_current=true,host_change_closing=false;void(*host_key_hook)(unsigned){};
bool sample_current(binding::Binding const& actor,CurrentKey& out){
    ++host_calls;if(host_key_hook)host_key_hook(host_calls);if(!host_current||!binding::same_key(actor,host_actor))return false;
    CurrentKey candidate{};auto const& id=host_core.cover_link;
    candidate.link.address=id.address;candidate.link.cls=id.klass;candidate.link.outer=id.outer;
    candidate.link.index=id.registry_index;candidate.link.name_index=id.name_index;candidate.link.number=id.name_number;
    candidate.link.address+=host_link_delta;candidate.slot=host_core.cover_slot+int(host_slot_delta);candidate.type=static_cast<unsigned char>(host_core.cover_type);
    candidate.action=static_cast<unsigned char>(host_action);
    if(host_change_closing&&host_calls>1)candidate.selected[0]=1;
    out=candidate;return true;
}}
