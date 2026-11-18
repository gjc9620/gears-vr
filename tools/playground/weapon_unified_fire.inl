// Unified manual Lancer muzzle sampling. Include after weapon_hand_client.inl,
// weapon_lancer_asset_read.inl and weapon_shot_scope.inl. Engine calls belong
// ONLY to prepare_current on normal PostPlayerTick, never Native notices.
// Native still owns CallFunction/original. The normal-return consumers below
// never invoke an engine function, consume VM parameters, wait or take a lock.
#include "weapon_unified_fire.h"
#include "../native-capture/weapon_call_trace.h"
#include "../native-capture/weapon_hud_query_input.h"
#include "../vr-common/weapon_hud_query_receipt.h"
#include "../vr-common/weapon_lancer_sight_zero.h"
#if !defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE) || !GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE
#error Include weapon_unified_fire.inl only with GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE=1 and its subscriber wiring.
#endif
#if !defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_TRIAL) || !defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_MAIN_MOUNT_TRIAL) || !defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL) || !defined(GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL)
#error Unified muzzle fire requires the existing equipment, Lancer asset, shot-scope and Native subscriber participants.
#endif
namespace unified_weapon_fire {
namespace hand=gears_weapon_hand;
namespace metadata=gripdamage_metadata;
using U32=std::uint32_t;
static_assert(sizeof(playground::Vector)==12&&sizeof(playground::Rotation)==12);
std::atomic<bool> control_running{},write_faulted{};
std::atomic<std::uint64_t> control_epoch{1};
bool set_running(bool value) noexcept {
    control_running.store(false,std::memory_order_release);
    auto previous=control_epoch.load(std::memory_order_acquire);
    do {if(previous==(std::numeric_limits<std::uint64_t>::max)()){write_faulted.store(true,std::memory_order_release);return false;}}
    while(!control_epoch.compare_exchange_weak(previous,previous+1,std::memory_order_acq_rel,std::memory_order_acquire));
    if(value&&(!manual_profile()||write_faulted.load(std::memory_order_acquire)))return false;
    control_running.store(value,std::memory_order_release);return true;
}

// Authored manual profile, awaiting the owner's in-headset direction feedback.
// This is a socket-local axis, not the HMD forward or a bore-certification flag.
enum class MuzzleAxis:unsigned {positive_x,negative_x,positive_y,negative_y,positive_z,negative_z};
constexpr MuzzleAxis lancer_muzzle_axis=MuzzleAxis::positive_x;
struct MuzzleSample {
#if defined(GEARS_PLAYGROUND_HB_FINITE_BURST) && GEARS_PLAYGROUND_HB_FINITE_BURST
    bool hammerburst{};
#endif
    std::int64_t render_acquisition_ns{}; // Original render-result age, never renewed by muzzle sampling.
    hand::Frame source{};
    lancermain_asset::Evidence asset{};
    std::uint64_t hold_generation{},epoch{};
    std::int64_t sampled_ns{};
    playground::Vector origin{},original_socket_origin{};
    playground::Rotation socket_rotation{},aim{};
    gears_vr::Vector follow_world_grip{},follow_target_mesh{};
    double tracking_position_difference{},tracking_orientation_difference{};
    bool follow_target_known{};
    gears_weapon_lancer_sight::Result sight_zero{};
    bool sight_zero_known{};
    bool valid{};
};
bool preflight_returns(MuzzleSample const& sample) noexcept;
thread_local MuzzleSample muzzle_sample;
thread_local std::array<std::uint64_t,5> source_admission_counts{};
thread_local std::array<std::int64_t,10> rejected_source{};
thread_local std::array<std::uint64_t,8> hud_admissions{};
thread_local HudArgumentEvidence hud_arguments{};
bool hud_argument_evidence(HudArgumentEvidence& out) noexcept {
    if(!weaponhand::current_gt(nullptr)||!hud_arguments.valid)return false;
    out=hud_arguments;return true;
}
bool hud_admission_evidence(std::array<std::uint64_t,8>& out) noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    out=hud_admissions;return true;
}
bool rejected_source_evidence(std::array<std::int64_t,10>& out) noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    out=rejected_source;return true;
}
bool admission_evidence(std::array<std::uint64_t,11>& out) noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    std::copy(weaponshot::admission_counts.begin(),weaponshot::admission_counts.end(),out.begin());
    std::copy(source_admission_counts.begin(),source_admission_counts.end(),out.begin()+6);
    return true;
}
thread_local bool sampling{};
thread_local unsigned prepare_stage{};
std::atomic<gears_weapon_hud_receipt::Submit> hud_submit{};
void configure_hud_receiver(void* image) noexcept {
    const auto native=static_cast<HMODULE>(image);
    const auto address=native&&native==weaponhand::native_module?
        GetProcAddress(native,gears_weapon_hud_receipt::submit_export):nullptr;
    hud_submit.store(address&&weaponhand::executable(address,native)?
        reinterpret_cast<gears_weapon_hud_receipt::Submit>(address):nullptr,std::memory_order_release);
}
bool submit_hud_receipt(gears_weapon_hud_receipt::Submit fn,
                        gears_weapon_hud_receipt::Receipt const* value) noexcept {
    __try{return fn(value);}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){return false;}
}

#include "weapon_muzzle_getter_call.inl"
bool configured_muzzle(hand::Frame const& frame,Name& name) noexcept {
    metadata::Snapshot schema{};schema.module_base=base;metadata::Reader r{schema,metadata::Stage::final_check};
    weaponhand::BoolField field{};metadata::RawName value{},again{};metadata::NameText text{};
    if(!metadata::tables(base,schema.objects,schema.names)||!weaponhand::object(r,frame.weapon)||
       !weaponhand::field(r,frame.weapon.address,L"MuzzleSocketName",L"NameProperty",8,field)||
       !r.at(frame.weapon.address,field.property.offset,value)||value.number||
       !metadata::fresh_name(schema.names,value.index,text)||!metadata::exact_text(text,L"Muzzle"))return false;
    metadata::Property current{};metadata::NameText fresh_text{};
    if(!r.property(schema.identities[field.property.identity].address,current)||
       !metadata::same_property(field.property,current)||
       !r.at(frame.weapon.address,field.property.offset,again)||value.index!=again.index||value.number!=again.number||
       !metadata::fresh_name(schema.names,value.index,fresh_text)||!metadata::same_name_text(text,fresh_text)||
       !retained_metadata(r))return false;
    name={value.index,value.number};return true;
}
// One-time held orientation from the live weapon's original configured Muzzle
// socket. Remove model-world rotation, then map socket +X/+Z onto grip +X/+Z.
// Contact translation remains the existing separately measured mount.
bool held_socket_basis(hand::Frame const& frame,uptr component,
    std::array<float,16> const& model,gears_vr::Matrix& out) noexcept {
    if(!nativeequipment::current(nullptr,frame))return false;
    Name muzzle{};if(!configured_muzzle(frame,muzzle))return false;
    playground::Args call(component,L"GetSocketWorldLocationAndRotation");
    metadata::Snapshot schema{};schema.module_base=base;metadata::Reader r{schema,metadata::Stage::final_check};
    U32 mask{},after_mask{};
    if(!metadata::tables(base,schema.objects,schema.names)||!socket_getter_schema(call,r,mask)||
       !call.set(L"InSocketName",muzzle).valid||!call.run()||
       !socket_getter_schema(call,r,after_mask)||mask!=after_mask||(call.output<U32>()&mask)==0)return false;
    const auto rotation=call.output<playground::Rotation>(L"OutRotation");
    constexpr double radians=3.14159265358979323846/32768.;
    const auto socket=gears_vr::ue_matrix({rotation.pitch*radians,rotation.yaw*radians,rotation.roll*radians});
    gears_vr::Matrix mesh{};
    for(unsigned axis=0;axis<3;++axis){
        const double length=std::hypot(std::hypot(double(model[axis*4]),double(model[axis*4+1])),double(model[axis*4+2]));
        if(!std::isfinite(length)||length<.1||length>10)return false;
        for(unsigned row=0;row<3;++row)mesh[row][axis]=model[axis*4+row]/length;
    }
    vrequip::MeshPoseProbe fresh{};
    if(!vrequip::mesh_pose_probe(frame.weapon.address,fresh)||fresh.mesh!=component||fresh.hidden||
       !nativeequipment::current(nullptr,frame))return false;
    for(unsigned i=0;i<16;++i)if(!std::isfinite(model[i])||std::abs(model[i]-fresh.local_to_world[i])>1e-3)return false;
    out=gears_vr::transpose(gears_vr::multiply(gears_vr::transpose(mesh),socket));
    return true; // bind_oriented independently validates proper rigid basis.
}
bool aim_from_socket(playground::Rotation const& rotation,playground::Rotation& aim) noexcept {
    constexpr double pi=3.14159265358979323846,unit=pi/32768.;
    const double p=double(rotation.pitch)*unit,y=double(rotation.yaw)*unit,r=double(rotation.roll)*unit;
    const double cp=std::cos(p),sp=std::sin(p),cy=std::cos(y),sy=std::sin(y),cr=std::cos(r),sr=std::sin(r);
    double x{},yy{},z{};
    switch(lancer_muzzle_axis){
    case MuzzleAxis::positive_x:case MuzzleAxis::negative_x:x=cp*cy;yy=cp*sy;z=sp;break;
    case MuzzleAxis::positive_y:case MuzzleAxis::negative_y:x=sr*sp*cy-cr*sy;yy=sr*sp*sy+cr*cy;z=-sr*cp;break;
    case MuzzleAxis::positive_z:case MuzzleAxis::negative_z:x=-cr*sp*cy-sr*sy;yy=-cr*sp*sy+sr*cy;z=cr*cp;break;
    }
    if(lancer_muzzle_axis==MuzzleAxis::negative_x||lancer_muzzle_axis==MuzzleAxis::negative_y||
       lancer_muzzle_axis==MuzzleAxis::negative_z){x=-x;yy=-yy;z=-z;}
    if(!std::isfinite(x)||!std::isfinite(yy)||!std::isfinite(z))return false;
    aim={int(std::lround(std::atan2(z,std::hypot(x,yy))/unit)),int(std::lround(std::atan2(yy,x)/unit)),0};
    return true;
}
void clear_source() noexcept {muzzle_sample={};}
bool prepare_source(hand::Frame const& frame,hand::GripGameState const& game,
    lancermain_asset::AssetScope asset_scope,MuzzleSample& output,
    bool (*preflight)(MuzzleSample const&) noexcept) noexcept {
    output={};prepare_stage=0;
    const auto now=weaponhand::clock_ns(nullptr);
    if(!manual_profile()||!control_running.load(std::memory_order_acquire)||write_faulted.load(std::memory_order_acquire)||
       sampling||!enabled.load(std::memory_order_acquire)||!weaponhand::current_gt(nullptr)||
       !weaponhand::client.armed()||!weaponhand::client.source_current_on_gt(frame.key)||!hand::frame_active(frame,now)||
       !game.known||!game.main_hand_held||game.animation_owns_hands||game.busy)return false;
    prepare_stage=1;
    MuzzleSample candidate{};candidate.source=frame;candidate.hold_generation=weaponhand::held_binding.generation;
    candidate.epoch=control_epoch.load(std::memory_order_acquire);
#if defined(GEARS_PLAYGROUND_HB_FINITE_BURST) && GEARS_PLAYGROUND_HB_FINITE_BURST
    lancermain_asset::HeldAsset observed_kind{};
    if(!candidate.hold_generation||lancermain_asset::read(frame,candidate.asset,
       asset_scope,&observed_kind)!=lancermain_asset::Status::ready)return false;
    candidate.hammerburst=observed_kind==lancermain_asset::HeldAsset::hammerburst;
#else
    if(!candidate.hold_generation||lancermain_asset::read(frame,candidate.asset,
       asset_scope)!=lancermain_asset::Status::ready)return false;
#endif
    prepare_stage=2;
    Name muzzle{};if(!configured_muzzle(frame,muzzle))return false;
    prepare_stage=3;
    playground::Args call(candidate.asset.component.address,L"GetSocketWorldLocationAndRotation");
    metadata::Snapshot schema{};schema.module_base=base;metadata::Reader r{schema,metadata::Stage::final_check};U32 mask{};
    SocketGetterLayout arguments{},returned{};
    if(!metadata::tables(base,schema.objects,schema.names)||!socket_getter_schema(call,r,mask,&arguments))return false;
    prepare_stage=4;
    if(!arguments.write(call,SocketGetterLayout::input_name,muzzle))return false;
    const auto identities_before=schema.identity_count;
    sampling=true;const bool ran=call.run();sampling=false;
    U32 after_mask{};
    U32 result{};
    if(!ran||!socket_getter_schema(call,r,after_mask,&returned)||schema.identity_count!=identities_before||
       arguments.function!=returned.function||arguments.bytes!=returned.bytes||arguments.offsets!=returned.offsets||
       mask!=after_mask||!returned.read(call,SocketGetterLayout::result,result)||(result&mask)==0)return false;
    prepare_stage=5;
    if(!returned.read(call,SocketGetterLayout::output_location,candidate.origin)||
       !returned.read(call,SocketGetterLayout::output_rotation,candidate.socket_rotation))return false;
    candidate.original_socket_origin=candidate.origin;
    if(!std::isfinite(candidate.origin.x)||!std::isfinite(candidate.origin.y)||!std::isfinite(candidate.origin.z)||
       !aim_from_socket(candidate.socket_rotation,candidate.aim))return false;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    // Use the same authenticated hand/mesh plan for the authoritative shot.
    // The component write follows R2 so it cannot consume this source's age.
    if(!nativelancerfollow::adjust_muzzle(frame,candidate.asset,candidate.origin,candidate.aim,&candidate.sight_zero))return false;
    candidate.sight_zero_known=nativelancerfollow::bound_is_lancer;
    // Keep the target with this admitted source. The visual lease clears its
    // one-frame plan before later shot returns can report their correlation.
    candidate.follow_target_known=nativelancerfollow::sample_target(
        frame,candidate.follow_world_grip,candidate.follow_target_mesh,candidate.render_acquisition_ns,
        candidate.tracking_position_difference,candidate.tracking_orientation_difference);
    if(!candidate.follow_target_known)return false;
#endif
    prepare_stage=6;
    lancermain_asset::Evidence after_asset{};hand::GripGameState after{};
    candidate.sampled_ns=weaponhand::clock_ns(nullptr);
    if(!control_running.load(std::memory_order_acquire)||candidate.epoch!=control_epoch.load(std::memory_order_acquire)||
       !hand::frame_active(frame,candidate.sampled_ns)||!weaponhand::client.source_game_on_gt(frame.key,after)||
       !weaponhand::client.armed()||candidate.hold_generation!=weaponhand::held_binding.generation||
       !hand::same_game(game,after)||
       lancermain_asset::read(frame,after_asset,asset_scope)!=lancermain_asset::Status::ready||
       !hand::same(candidate.asset.component,after_asset.component)||!hand::same(candidate.asset.asset,after_asset.asset)||
       !preflight||!preflight(candidate))return false;
    prepare_stage=7;
    // Preserve original input/stereo receive stamps. The getter does not renew
    // tracking lifetime; the shot consumer must still match this source cohort.
    // The actual-shot consumer needs the existing admitted grip snapshot too.
    // If it is unknown, do not start an R2 press whose return consumers would
    // decline and leave the historical camera-aim path in place. This runs on
    // normal PostTick; callback notifications never enter the Client gate.
    hand::GripSnapshot admitted{};
    if(!weaponhand::client.snapshot_on_gt(admitted)||
       (admitted.grip!=hand::Grip::single&&admitted.grip!=hand::Grip::two)||
       !admitted.current(frame.key,frame.weapon,weaponhand::clock_ns(nullptr))||
       !hand::same(admitted.world,frame.world)||!hand::same(admitted.controller,frame.controller)||
       !hand::same(admitted.pawn,frame.pawn)||!hand::same(admitted.mesh,frame.mesh))return false;
    prepare_stage=8;
    candidate.valid=true;output=candidate;return true;
}
bool prepare_current(hand::Frame const& frame,hand::GripGameState const& game) noexcept {
    return prepare_source(frame,game,lancermain_asset::AssetScope::supported_instant_fire,
        muzzle_sample,preflight_returns);
}
bool held_current(MuzzleSample const& sample) noexcept {
    auto const& held=weaponhand::held_binding;auto const& frame=sample.source;
    return sample.valid&&held.known&&held.generation==sample.hold_generation&&
        hand::same(held.world,frame.world)&&hand::same(held.controller,frame.controller)&&
        hand::same(held.pawn,frame.pawn)&&hand::same(held.weapon,frame.weapon)&&
        held.native_cookie==frame.key.registration_cookie&&held.native_started==frame.key.native_started_qpc&&
        held.world_generation==frame.key.world_generation;
}
bool live_sample(MuzzleSample const& sample,std::int64_t now) noexcept {
    return sample.valid&&manual_profile()&&control_running.load(std::memory_order_acquire)&&
        !write_faulted.load(std::memory_order_acquire)&&sample.epoch==control_epoch.load(std::memory_order_acquire)&&
        enabled.load(std::memory_order_acquire)&&weaponhand::current_gt(nullptr)&&
        sample.sampled_ns>0&&now>=sample.sampled_ns&&now-sample.sampled_ns<=100000000&&
        (!sample.render_acquisition_ns||(now>=sample.render_acquisition_ns&&now-sample.render_acquisition_ns<=100000000))&&
        hand::frame_active(sample.source,now)&&held_current(sample)&&
        weaponhand::client.registration_alive(sample.source.key.registration_cookie,sample.source.key.native_started_qpc);
}
bool ready_for(hand::Frame const& frame,std::int64_t dispatch_reserve_ns) noexcept {
    const auto now=weaponhand::clock_ns(nullptr);
    // Reserve time INSIDE the original lifetime for ShouldRefire/ConsumeAmmo
    // and the real return consumers. This never renews a producer timestamp.
    return dispatch_reserve_ns>=0&&dispatch_reserve_ns<=100000000&&now<=INT64_MAX-dispatch_reserve_ns&&
        live_sample(muzzle_sample,now+dispatch_reserve_ns)&&hand::same_frame(frame,muzzle_sample.source);
}
bool readiness_evidence(hand::Frame const& frame,std::int64_t reserve,ReadinessEvidence& out) noexcept {
    out={};if(!weaponhand::current_gt(nullptr)||reserve<0||reserve>100000000)return false;
    const auto now=weaponhand::clock_ns(nullptr);if(now>INT64_MAX-reserve)return false;
    const auto sample=muzzle_sample;
    const auto age=[&](std::int64_t stamp)noexcept{return stamp>0&&now>=stamp?now-stamp:-1;};
    out.valid=sample.valid;out.same_frame=hand::same_frame(frame,sample.source);
    out.frame_current=hand::frame_active(frame,now);out.reserved_frame_current=hand::frame_active(frame,now+reserve);
    out.held=held_current(sample);out.registration=weaponhand::client.registration_alive(frame.key.registration_cookie,frame.key.native_started_qpc);
    out.live=live_sample(sample,now);out.reserved_live=live_sample(sample,now+reserve);
    out.epoch=sample.epoch==control_epoch.load(std::memory_order_acquire);
    out.sync_age=age(frame.controllers.sync_ns);out.stereo_age=age(frame.stereo.received_ns);out.frame_age=age(frame.now_ns);
    out.sample_age=age(sample.sampled_ns);out.render_age=age(sample.render_acquisition_ns);return true;
}

namespace callabi=gears_weapon_call;
namespace shot=gears_weapon_shot;
constexpr unsigned scope_capacity=12,pending_capacity=16;
struct SourceScope {
    shot::Token scope{};shot::Binding binding{};callabi::Invocation call{};
    MuzzleSample sample{};bool valid{};
};
thread_local std::array<SourceScope,scope_capacity> source_scopes;
enum class ResultKind:unsigned {none,base_aim,trace_origin,pushed_origin,physical_origin};
struct ReturnSignature {
    ResultKind kind{};
    hand::ObjectIdentity function{},result_property{};
    std::uint64_t fingerprint{};
};
struct Pending {
    bool live{},uncertain{};
    unsigned depth{};std::uint64_t scope_call{},epoch{};
    callabi::Invocation call{};shot::Packet packet{};ReturnSignature signature{};
    std::array<U32,3> desired{};
};
thread_local std::array<Pending,pending_capacity> pending;
thread_local FireTraceEvidence fire_trace;
struct TrajectorySlot {
    ShotTrajectoryEvidence record{};
    std::uint64_t reported_revision{};
};
thread_local std::array<TrajectorySlot,64> trajectory_slots;
thread_local std::uint64_t trajectory_epoch{},trajectory_dropped_updates{};
void reset_trajectory_epoch(std::uint64_t epoch) noexcept {
    if(trajectory_epoch==epoch)return;
    trajectory_slots={};trajectory_dropped_updates=0;trajectory_epoch=epoch;
}
TrajectorySlot* trajectory_slot(std::uint64_t shot_id,std::uint64_t epoch) noexcept {
    if(!shot_id)return nullptr;
    reset_trajectory_epoch(epoch);
    TrajectorySlot* reusable{};
    for(auto& slot:trajectory_slots){
        if(slot.record.shot==shot_id)return &slot;
        if(!reusable&&(!slot.record.shot||slot.record.revision==slot.reported_revision))reusable=&slot;
    }
    if(!reusable){
        if(trajectory_dropped_updates!=(std::numeric_limits<std::uint64_t>::max)())++trajectory_dropped_updates;
        return nullptr;
    }
    *reusable={};reusable->record.shot=shot_id;reusable->record.epoch=epoch;
    return reusable;
}
bool next_shot_trajectory(ShotTrajectoryEvidence& out) noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    reset_trajectory_epoch(control_epoch.load(std::memory_order_acquire));
    for(auto& slot:trajectory_slots){
        if(slot.record.shot&&slot.record.revision>slot.reported_revision){
            out=slot.record;slot.reported_revision=slot.record.revision;return true;
        }
    }
    return false;
}
std::uint64_t shot_trajectory_dropped_updates() noexcept {
    if(!weaponhand::current_gt(nullptr))return 0;
    reset_trajectory_epoch(control_epoch.load(std::memory_order_acquire));
    return trajectory_dropped_updates;
}

bool same_context(hand::FrameKey const& source,hand::FrameKey const& current) noexcept {
    return source.game_start==current.game_start&&source.native_started_qpc==current.native_started_qpc&&
        source.registration_cookie==current.registration_cookie&&source.lifecycle==current.lifecycle&&
        source.world_generation==current.world_generation&&source.game_pid==current.game_pid&&
        source.thread_id==current.thread_id&&source.frame<=current.frame;
}
bool source_matches_shot(MuzzleSample const& sample,shot::Packet const& packet) noexcept {
    auto const& frame=sample.source;auto const& grip=packet.grip;auto const& input=frame.controllers;
    // A normal PostTick sample can precede the next Native publication. It is
    // never relabelled as that frame: incarnation/space/hold must agree, counters
    // cannot go backwards, and both original receive stamps remain bounded.
    return same_context(frame.key,grip.key)&&hand::same(frame.weapon,packet.weapon)&&
        hand::same(frame.world,grip.world)&&hand::same(frame.controller,grip.controller)&&
        hand::same(frame.pawn,grip.pawn)&&hand::same(frame.mesh,grip.mesh)&&hand::same(frame.weapon,grip.weapon)&&
        input.session==grip.input_session&&input.space_epoch==grip.space_epoch&&
        input.profile_generation==grip.profile_generation&&input.input_epoch==grip.input_epoch&&
        input.sequence<=grip.input_sequence&&frame.reference_generation==grip.reference_generation&&
        frame.stereo.received_ns<=grip.stereo_received_ns&&input.received_ns<=grip.input_received_ns;
}
SourceScope* active_source() noexcept {
    const auto depth=weaponshot::stack.depth();if(!depth||depth>source_scopes.size())return nullptr;
    auto& source=source_scopes[depth-1];shot::Binding binding{};hand::ObjectIdentity weapon{};
    if(!source.valid||!source.scope.entered||source.scope.previous_depth+1!=depth||
       !weaponshot::stack.binding(binding,weapon)||!shot::same(binding,source.binding)||
       !hand::same(weapon,source.sample.source.weapon))return nullptr;
    return &source;
}
bool packet_for(callabi::Invocation const& call,SourceScope const& source,shot::Packet& packet) noexcept {
    return callabi::invocation_valid(call)&&call.thread_id==GetCurrentThreadId()&&weaponhand::current_gt(nullptr)&&
        call.hand_cookie==source.sample.source.key.registration_cookie&&
        call.native_started_qpc==source.sample.source.key.native_started_qpc&&
        weaponshot::active_binding_current()&&
        weaponshot::stack.packet(source.sample.source.weapon,call.caller_frame,weaponshot::read,packet)&&
        (packet.grip.grip==hand::Grip::single||packet.grip.grip==hand::Grip::two)&&source_matches_shot(source.sample,packet)&&
        weaponshot::write_admitted(packet);
}
bool owner_in_receiver(metadata::Reader& r,uptr receiver,uptr owner) noexcept {
    U32 object{};if(!r.path(receiver,object))return false;
    auto cls=r.result.identities[object].klass;std::array<uptr,metadata::max_parents> seen{};unsigned count{};
    while(cls){
        if(count==seen.size())return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==cls)return false;
        seen[count++]=cls;metadata::Structure structure{};
        if(!r.structure(cls,structure))return false;
        if(cls==owner)return true;cls=structure.super;
    }
    return false;
}
struct Fingerprint {
    std::uint64_t value{14695981039346656037ull};
    void word(std::uint64_t v) noexcept {for(unsigned i=0;i<8;++i){value^=v&255;value*=1099511628211ull;v>>=8;}}
};
void schema_fingerprint(metadata::Snapshot const& schema,Fingerprint& hash) noexcept {
    hash.word(schema.module_base);hash.word(schema.objects.data);hash.word(schema.names.data);
    hash.word(schema.identity_count);hash.word(schema.name_count);hash.word(schema.structure_count);hash.word(schema.field_count);
    for(U32 i=0;i<schema.identity_count;++i){auto const& id=schema.identities[i];
        hash.word(id.address);hash.word(id.outer);hash.word(id.klass);hash.word(id.index);hash.word(id.name.index);hash.word(id.name.number);}
    for(U32 i=0;i<schema.name_count;++i){auto const& name=schema.name_text[i];
        hash.word(name.entry);hash.word(name.index);hash.word(name.length);
        for(U32 j=0;j<name.length&&j<name.units.size();++j)hash.word(name.units[j]);}
    for(U32 i=0;i<schema.structure_count;++i){auto const& structure=schema.structures[i];
        hash.word(structure.identity);hash.word(structure.super);hash.word(structure.children);hash.word(structure.extent);}
    for(U32 i=0;i<schema.field_count;++i){auto const& field=schema.fields[i];
        hash.word(field.identity);hash.word(field.type);hash.word(field.next);hash.word(field.flags);
        hash.word(field.dimension);hash.word(field.element_bytes);hash.word(field.offset);}
}
bool typed_result(callabi::Invocation const& call,MuzzleSample const& sample,ReturnSignature& out,
                  bool projectile_muzzle=false) noexcept {
    // This function only reads metadata. Exact visited-address membership is
    // query-local; it ends before any engine call and never retains selectors,
    // names, values, input or a previous result across queries/callbacks.
    weapon_metadata_parse::TraversalQuery traversal;
    out={};metadata::Snapshot schema{};schema.module_base=base;metadata::Reader r{schema,metadata::Stage::function_path};
    U32 fn{},kind{},owner{},owner_kind{},package{};
    if(!call.result||(call.result&3)||call.result>UINTPTR_MAX-12||!metadata::tables(base,schema.objects,schema.names)||
       !r.path(call.callee,fn)||!r.path(schema.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(schema.identities[fn].outer,owner)||!r.path(schema.identities[owner].klass,owner_kind)||
       !weaponhand::core_type(r,owner_kind,L"Class")||!r.path(schema.identities[owner].outer,package)||
       schema.identities[package].outer)return false;
    const bool war=r.named(package,L"WarfareGame"),engine=r.named(package,L"Engine");
    const bool pawn=(war&&r.named(owner,L"WarPawn"))||(engine&&r.named(owner,L"Pawn"));
    const bool weapon=(war&&r.named(owner,L"WarWeapon"))||(engine&&r.named(owner,L"Weapon"));
    ResultKind result{};
    if(war&&r.named(owner,L"WarPawn")&&r.named(fn,L"GetAdjustedAimFor"))result=ResultKind::base_aim;
    else if((pawn||weapon)&&r.named(fn,L"GetWeaponStartTraceLocation"))result=ResultKind::trace_origin;
    else if((pawn||weapon)&&r.named(fn,L"GetPushedForwardStartTraceLocation"))result=ResultKind::pushed_origin;
    else if((pawn||weapon)&&r.named(fn,L"GetPhysicalFireStartLoc"))result=ResultKind::physical_origin;
    else if(projectile_muzzle&&war&&r.named(owner,L"WarWeapon")&&r.named(fn,L"GetMuzzleLoc"))result=ResultKind::physical_origin;
    else return false;
    auto const& frame=sample.source;
    const auto& receiver=pawn?frame.pawn:frame.weapon;
    if(call.self!=receiver.address||!weaponhand::object(r,frame.world)||!weaponhand::object(r,frame.controller)||
       !weaponhand::object(r,frame.pawn)||!weaponhand::object(r,frame.mesh)||!weaponhand::object(r,frame.weapon)||
       !weaponhand::object(r,sample.asset.component)||!weaponhand::object(r,sample.asset.asset)||
       !owner_in_receiver(r,call.self,schema.identities[owner].address))return false;
    // Recheck the actual object relationships through typed selectors. These
    // bounded metadata reads do not enter an engine getter or client gate.
    std::array<metadata::PointerField,4> links{};
    if(!weaponhand::pointer(r,frame.pawn.address,L"Weapon",frame.weapon.address,links[0])||
       !weaponhand::pointer(r,frame.weapon.address,L"Instigator",frame.pawn.address,links[1])||
       !lancermain_asset::mesh_pointer(r,frame.weapon.address,links[2])||links[2].value!=sample.asset.component.address||
       !weaponhand::pointer(r,sample.asset.component.address,L"SkeletalMesh",sample.asset.asset.address,links[3]))return false;
    for(auto const& link:links){if(schema.field_count==schema.fields.size())return false;schema.fields[schema.field_count++]=link.property;}
    metadata::Structure structure{};std::uint16_t parameter_bytes{},return_offset{},native_index{};U32 flags{};uptr exec{};
    metadata::Table script{};std::uint64_t script_hash{};
    if(!r.structure(call.callee,structure)||!r.at(call.callee,0x106,parameter_bytes)||parameter_bytes<12||parameter_bytes>4096||
       !r.at(call.callee,0x108,return_offset)||!r.at(call.callee,0xf4,flags)||!r.at(call.callee,0xf8,native_index)||
       !r.at(call.callee,0x114,exec)||native_index||exec!=base+0x1d6ca0||!r.at(call.callee,0x8c,script)||
       script.count<=0||script.count>script.capacity||script.capacity>131072||
       !weaponshot::hash_script(script.data,unsigned(script.count),script_hash))return false;
    metadata::Property returned{};uptr return_structure{};unsigned returns{};
    std::array<uptr,metadata::max_fields> visited{};unsigned count{};
    for(auto child=structure.children;child;){
        if(count==visited.size())return false;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;
        visited[count++]=child;metadata::Property property{};
        if(!r.property(child,property)||schema.identities[property.identity].outer!=call.callee||
           !metadata::property_fits(property,structure.extent)||
           ((property.flags&0x80)&&!metadata::property_fits(property,parameter_bytes))||schema.field_count==schema.fields.size())return false;
        schema.fields[schema.field_count++]=property;
        if(property.flags&0x400){
            if(++returns!=1||(property.flags&0x480)!=0x480||property.dimension!=1||property.offset!=return_offset||
               !(result==ResultKind::base_aim?core_struct(r,property,L"Rotator"):core_struct(r,property,L"Vector"))||
               !r.at(child,0xd0,return_structure))return false;
            returned=property;
        }
        child=property.next;
    }
    if(returns!=1||!retained_metadata(r))return false;
    Fingerprint hash{};schema_fingerprint(schema,hash);
    hash.word(parameter_bytes);hash.word(return_offset);hash.word(return_structure);hash.word(flags);hash.word(native_index);hash.word(exec);
    hash.word(script.data);hash.word(unsigned(script.count));hash.word(unsigned(script.capacity));hash.word(script_hash);
    out={result,weaponshot::identity(schema.identities[fn]),weaponshot::identity(schema.identities[returned.identity]),hash.value};
    return true;
}
bool same_signature(ReturnSignature const& a,ReturnSignature const& b) noexcept {
    return a.kind==b.kind&&a.fingerprint==b.fingerprint&&hand::same(a.function,b.function)&&hand::same(a.result_property,b.result_property);
}
// Schema and exact direct-argument preparation. This alone does not authorize
// a HUD write: integration must own the invocation transaction and provenance.
bool hud_query_layout(callabi::Invocation const& call,MuzzleSample const& sample,
                      gears_weapon_hud_query::Layout& out) noexcept {
    if(!callabi::invocation_valid(call)||call.thread_id!=GetCurrentThreadId()||
       !live_sample(sample,weaponhand::clock_ns(nullptr))||call.self!=sample.source.weapon.address||
       call.hand_cookie!=sample.source.key.registration_cookie||
       call.native_started_qpc!=sample.source.key.native_started_qpc)return false;
    ++hud_admissions[0];
    gears_ui_call_boundary::Frame caller{};
    if(!gears_ui_call_boundary::frame(weaponshot::read,call.caller_frame,caller)||
       caller.object!=sample.source.controller.address)return false;
    ++hud_admissions[1];
    metadata::Snapshot schema{};schema.module_base=base;
    metadata::Reader r{schema,metadata::Stage::function_path};
    if(!metadata::tables(base,schema.objects,schema.names))return false;
    auto function=[&](uptr address,auto const& name,auto const& owner_name,auto const& package_name){
        U32 fn{},kind{},owner{},package{};
        return r.path(address,fn)&&r.named(fn,name)&&r.path(schema.identities[fn].klass,kind)&&
            weaponhand::core_type(r,kind,L"Function")&&r.path(schema.identities[fn].outer,owner)&&
            r.named(owner,owner_name)&&r.path(schema.identities[owner].outer,package)&&
            r.named(package,package_name)&&!schema.identities[package].outer;
    };
    if(!function(caller.node,L"UpdateHUDActions",L"WarPC",L"WarfareGame")||
       !function(call.callee,L"CalcWeaponFire",L"Weapon",L"Engine"))return false;
    ++hud_admissions[2];
    metadata::Structure body{};metadata::Table script{};
    if(!r.structure(caller.node,body)||body.extent<36||body.extent>65536||
       !r.at(caller.node,0x8c,script)||script.count<=0||script.count>script.capacity||
       script.capacity>131072||!gears_weapon_call_trace::script_pc(caller.code,script.data,unsigned(script.count)))return false;
    gears_weapon_hud_query::Layout value{caller.node,caller.object,caller.code,body.extent,{}};
    std::array<uptr,metadata::max_fields> visited{};unsigned count{},found{};
    std::array<uptr,3> properties{};
    for(auto child=body.children;child;){
        if(count==visited.size())return false;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;
        visited[count++]=child;metadata::Property p{};
        if(!r.property(child,p)||schema.identities[p.identity].outer!=caller.node)return false;
        unsigned index=3;
        if(r.named(p.identity,L"StartTrace"))index=0;
        else if(r.named(p.identity,L"EndTrace"))index=1;
        else if(r.named(p.identity,L"TraceDir"))index=2;
        if(index<3){
            if((found&(1u<<index))||p.dimension!=1||p.flags||!core_struct(r,p,L"Vector")||
               !metadata::property_fits(p,body.extent))return false;
            value.offsets[index]=p.offset;properties[index]=child;found|=1u<<index;
        }
        child=p.next;
    }
    U32 calc{};
    if(found!=7||!r.path(call.callee,calc)||schema.identities[calc].name.number||!retained_metadata(r))return false;
    ++hud_admissions[3];
    if(!hud_arguments.valid&&script.count>=32&&value.code>=script.data+9&&
       value.code-script.data<=unsigned(script.count)-23){
        HudArgumentEvidence observation{};
        observation.offset=value.code-script.data;
        observation.start_property=properties[0];observation.end_property=properties[1];
        observation.callee_name=schema.identities[calc].name.index;
        observation.valid=copy(observation.bytes.data(),value.code-9,observation.bytes.size());
        if(observation.valid)hud_arguments=observation;
    }
    if(!gears_weapon_hud_query::direct_arguments(weaponshot::read,value,script.data,unsigned(script.count),
           schema.identities[calc].name.index,properties[0],properties[1]))return false;
    ++hud_admissions[4];
    gears_weapon_hud_query::Input snapshot{};
    if(!gears_weapon_hud_query::capture(weaponshot::read,call.caller_frame,value,snapshot))return false;
    ++hud_admissions[5];
    out=value;return true;
}
bool preflight_returns(MuzzleSample const& sample) noexcept {
    // Metadata-only lookups, not synthetic calls or shot evidence. A manual
    // trigger cannot start using the historical camera-aim path merely because
    // one of the three real return consumers has an unsupported schema.
    const std::array<wchar_t const*,3> labels{L"GetAdjustedAimFor",L"GetWeaponStartTraceLocation",L"GetPushedForwardStartTraceLocation"};
    const std::array<ResultKind,3> expected{ResultKind::base_aim,ResultKind::trace_origin,ResultKind::pushed_origin};
    std::array<U32,3> unused_result{};
    for(unsigned i=0;i<labels.size();++i){
        const auto receiver=i==2?sample.source.weapon.address:sample.source.pawn.address;
        playground::Args descriptor(receiver,labels[i]);
        if(!descriptor.valid)return false;
        callabi::Invocation identity{};identity.self=receiver;identity.callee=descriptor.r.function;
        identity.result=reinterpret_cast<uptr>(unused_result.data());
        ReturnSignature signature{};
        if(!typed_result(identity,sample,signature)||signature.kind!=expected[i])return false;
    }
    return true;
}
bool inherits_sample(SourceScope const& parent,callabi::Invocation const& call,shot::Binding const& binding,
    shot::Token const& token) noexcept {
    if(!parent.valid||token.previous_call!=parent.scope.call||parent.binding.role!=shot::Role::war_instant||
       binding.role!=shot::Role::engine_instant)return false;
    gears_ui_call_boundary::Frame caller{};
    return gears_ui_call_boundary::frame(weaponshot::read,call.caller_frame,caller)&&
        caller.node==parent.binding.identities[0].address&&caller.object==parent.sample.source.weapon.address&&
        caller.previous==parent.call.caller_frame&&shot::script_pc(parent.binding,caller.code);
}
// Authenticate the shipped Engine.Weapon.InstantFire local layout and its
// direct CalcWeaponFire bytecode before reading the three invocation-local
// vectors. This observes the real game's query without changing its result.
bool fire_trace_schema(callabi::Invocation const& call,SourceScope const& source,
                       unsigned& locals_size) noexcept {
    locals_size=0;
    if(source.binding.role!=shot::Role::engine_instant||
       source.binding.bytes<0xba||call.self!=source.sample.source.weapon.address)return false;
    metadata::Snapshot schema{};schema.module_base=base;
    metadata::Reader r{schema,metadata::Stage::function_path};
    U32 calc{},kind{},owner{},package{},fire{};
    if(!metadata::tables(base,schema.objects,schema.names)||!r.path(call.callee,calc)||
       !r.named(calc,L"CalcWeaponFire")||schema.identities[calc].name.number||
       !r.path(schema.identities[calc].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(schema.identities[calc].outer,owner)||
       schema.identities[owner].address!=source.binding.identities[1].address||
       !r.named(owner,L"Weapon")||!r.path(schema.identities[owner].outer,package)||
       schema.identities[package].address!=source.binding.identities[2].address||
       !r.named(package,L"Engine")||schema.identities[package].outer||
       !r.path(source.binding.identities[0].address,fire)||!r.named(fire,L"InstantFire")||
       !r.at(source.binding.identities[0].address,0x88,locals_size)||
       locals_size<52||locals_size>4096)return false;
    metadata::Structure body{};
    if(!r.structure(source.binding.identities[0].address,body))return false;
    std::array<uptr,4> fields{};
    constexpr unsigned offsets[]{0,12,24,36};
    std::array<uptr,metadata::max_fields> visited{};unsigned count{};
    for(auto child=body.children;child;){
        if(count==visited.size())return false;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;
        visited[count++]=child;
        metadata::Property property{};
        if(!r.property(child,property)||schema.identities[property.identity].outer!=source.binding.identities[0].address)
            return false;
        unsigned match=4;
        if(r.named(property.identity,L"StartTrace"))match=0;
        else if(r.named(property.identity,L"EndTrace"))match=1;
        else if(r.named(property.identity,L"PushedForwardStartTrace"))match=2;
        else if(r.named(property.identity,L"ImpactList"))match=3;
        if(match<4){const auto i=match;
            if(fields[i]||property.offset!=offsets[i]||property.dimension!=1||
               property.offset+property.element_bytes>locals_size)return false;
            if(i<3){
                if(property.element_bytes!=12||property.flags||!core_struct(r,property,L"Vector"))return false;
            }else if(property.element_bytes!=16||property.flags!=0x400000||
                     !weaponhand::core_type(r,property.type,L"ArrayProperty"))return false;
            fields[i]=child;
        }
        child=property.next;
    }
    for(auto field:fields)if(!field)return false;
    std::array<unsigned char,0xba-0x95> expected{},actual{};unsigned at{};
    expected[at++]=0x1b;
    const auto calc_name=schema.identities[calc].name.index;
    std::memcpy(expected.data()+at,&calc_name,4);at+=8; // FName number is zero.
    for(const auto field:{fields[2],fields[1],fields[3]}){
        expected[at++]=0;
        std::memcpy(expected.data()+at,&field,8);at+=8;
    }
    expected[at++]=0x16;
    return at==expected.size()&&copy(actual.data(),source.binding.script+0x95,actual.size())&&
        actual==expected&&retained_metadata(r);
}
void observe_fire_trace(callabi::Invocation const& call,SourceScope const& source,
                        shot::Packet const& packet) noexcept {
    unsigned locals_size{};
    if(!fire_trace_schema(call,source,locals_size))return;
    gears_weapon_call_trace::FireInput input{};
    if(!gears_weapon_call_trace::direct_fire_input(weaponshot::read,call.caller_frame,
        source.binding.identities[0].address,source.sample.source.weapon.address,
        source.call.caller_frame,source.binding.script,source.binding.bytes,
        locals_size,{0,12,24},input))return;
    const auto epoch=control_epoch.load(std::memory_order_acquire);
    if(fire_trace.epoch!=epoch){fire_trace={};fire_trace.epoch=epoch;}
    if(fire_trace.count!=(std::numeric_limits<std::uint64_t>::max)())++fire_trace.count;
    fire_trace.shot=packet.shot;
    fire_trace.frame=source.sample.source.key.frame;
    fire_trace.input_sequence=source.sample.source.controllers.sequence;
    fire_trace.input_sync_ns=source.sample.source.controllers.sync_ns;
    fire_trace.observed_ns=weaponhand::clock_ns(nullptr);
    longshot_fire::trace(source.sample.source,packet.shot);
    std::memcpy(fire_trace.start,input.start.data(),12);
    std::memcpy(fire_trace.end,input.end.data(),12);
    std::memcpy(fire_trace.pushed,input.pushed.data(),12);
    std::memcpy(fire_trace.muzzle_origin,&source.sample.origin,12);
    std::memcpy(fire_trace.muzzle_aim,&source.sample.aim,12);
    fire_trace.sight_zero_known=source.sample.sight_zero_known;
    fire_trace.sight_units=source.sample.sight_zero.units;
    fire_trace.sight_model_scale=source.sample.sight_zero.model_scale;
    const gears_vr::Vector sight_points[]={source.sample.sight_zero.rear,source.sample.sight_zero.front,source.sample.sight_zero.zero};
    double* sight_fields[]={fire_trace.sight_rear,fire_trace.sight_front,fire_trace.sight_zero};
    for(unsigned i=0;i<3;++i){sight_fields[i][0]=sight_points[i].x;
        sight_fields[i][1]=sight_points[i].y;sight_fields[i][2]=sight_points[i].z;}
    if(auto* slot=trajectory_slot(packet.shot,epoch)){
        if(slot->record.trace_valid&&
           (slot->record.trace.frame!=fire_trace.frame||
            slot->record.trace.input_sequence!=fire_trace.input_sequence||
            slot->record.trace.input_sync_ns!=fire_trace.input_sync_ns))
            slot->record.trace_source_consistent=false;
        slot->record.trace=fire_trace;
        std::memcpy(slot->record.writes_at_trace,slot->record.writes,sizeof(slot->record.writes));
        slot->record.trace_valid=true;
        ++slot->record.revision;
#if defined(GEARS_PLAYGROUND_HB_FINITE_BURST) && GEARS_PLAYGROUND_HB_FINITE_BURST
        if(source.sample.hammerburst)hammerburst_burst_event::trace(source.sample.source,packet.shot,
            source.sample.render_acquisition_ns,source.sample.sampled_ns,fire_trace.observed_ns,
            slot->record.write_source_consistent&&slot->record.trace_source_consistent);
#endif
    }
}
void begin(callabi::Invocation const& call,shot::Token const& scope) noexcept {
    if(!manual_profile()||!weaponhand::current_gt(nullptr)||call.thread_id!=GetCurrentThreadId())return;
    if(scope.entered){
        if(scope.previous_depth>=source_scopes.size())return;
        auto& source=source_scopes[scope.previous_depth];source={};source.scope=scope;source.call=call;
        hand::ObjectIdentity weapon{};
        if(!callabi::invocation_valid(call)||!weaponshot::stack.binding(source.binding,weapon)||
           (source.binding.role!=shot::Role::war_instant&&source.binding.role!=shot::Role::engine_instant))return;
        if(scope.previous_depth&&inherits_sample(source_scopes[scope.previous_depth-1],call,source.binding,scope))
            source.sample=source_scopes[scope.previous_depth-1].sample;
        else source.sample=muzzle_sample;
        const auto entry_now=weaponhand::clock_ns(nullptr);
#if defined(GEARS_PLAYGROUND_HB_FINITE_BURST) && GEARS_PLAYGROUND_HB_FINITE_BURST
        if(source.sample.hammerburst&&!hammerburst_burst_event::source_allowed(source.sample.source,
            source.sample.render_acquisition_ns,source.sample.sampled_ns,entry_now))return;
#endif
        if(!live_sample(source.sample,entry_now)){
            ++source_admission_counts[0];
            rejected_source={std::int64_t(source.sample.valid),
                source.sample.valid?entry_now-source.sample.source.controllers.sync_ns:0,
                source.sample.valid?entry_now-source.sample.sampled_ns:0,
                std::int64_t(held_current(source.sample)),
                source.sample.valid?entry_now-source.sample.source.stereo.received_ns:0,
                source.sample.render_acquisition_ns?entry_now-source.sample.render_acquisition_ns:0,
                source.sample.valid?entry_now-source.sample.source.now_ns:0,
                std::int64_t(source.sample.valid&&hand::frame_active(source.sample.source,entry_now)),
                std::int64_t(source.sample.tracking_position_difference*1000000),
                std::int64_t(source.sample.tracking_orientation_difference*1000000000)};
            return;
        }
        if(!hand::same(weapon,source.sample.source.weapon)||
           call.self!=weapon.address||call.hand_cookie!=source.sample.source.key.registration_cookie||
           call.native_started_qpc!=source.sample.source.key.native_started_qpc){++source_admission_counts[1];return;}
        ++source_admission_counts[2];source.valid=true;return;
    }
    // Most calls inside InstantFire are unrelated helpers. Reject those
    // before repeatedly rebinding the whole shot/metadata schema: doing that
    // work for every helper consumes the live render endpoint's100ms window.
    // This is only a negative name filter; every admitted candidate still
    // passes the unchanged receiver, bytecode, schema, lifetime and shot checks.
    // No live shot source means neither trace nor getter can be consumed.
    // This only reads the local stack; keep names and live guards below in
    // their existing order for a source that could actually use the call.
    auto* source=active_source();
    if(!source)return;
    const bool trace_candidate=playground::named(call.callee,L"CalcWeaponFire");
    const bool result_candidate=call.result&&(
        playground::named(call.callee,L"GetAdjustedAimFor")||
        playground::named(call.callee,L"GetWeaponStartTraceLocation")||
        playground::named(call.callee,L"GetPushedForwardStartTraceLocation")||
        playground::named(call.callee,L"GetPhysicalFireStartLoc"));
    if(!trace_candidate&&!result_candidate)return;
    if(!control_running.load(std::memory_order_acquire)||source->sample.epoch!=control_epoch.load(std::memory_order_acquire)||
       write_faulted.load(std::memory_order_acquire)||!held_current(source->sample))return;
    shot::Packet packet{};
    if(!packet_for(call,*source,packet)){++source_admission_counts[3];return;}
    ++source_admission_counts[4];
    observe_fire_trace(call,*source,packet);
    ReturnSignature signature{};
    if(!typed_result(call,source->sample,signature))return;
    Pending* slot{};
    for(auto& entry:pending){if(entry.live&&entry.call.call_id==call.call_id)return;if(!entry.live&&!slot)slot=&entry;}
    if(!slot)return; // Bounded overflow declines without changing original.
    Pending value{};value.live=true;value.depth=weaponshot::stack.depth();value.scope_call=source->scope.call;
    value.epoch=source->sample.epoch;value.call=call;value.packet=packet;value.signature=signature;
    if(signature.kind==ResultKind::base_aim)std::memcpy(value.desired.data(),&source->sample.aim,12);
    else std::memcpy(value.desired.data(),&source->sample.origin,12);
    *slot=value;
}
bool write_context(Pending const& entry,SourceScope const& source) noexcept {
    return manual_profile()&&control_running.load(std::memory_order_acquire)&&!write_faulted.load(std::memory_order_acquire)&&
        entry.epoch==control_epoch.load(std::memory_order_acquire)&&entry.epoch==source.sample.epoch&&
        entry.depth==weaponshot::stack.depth()&&entry.scope_call==source.scope.call&&held_current(source.sample)&&
        enabled.load(std::memory_order_acquire)&&weaponshot::write_admitted(entry.packet);
}
bool compare_word(uptr address,U32 expected,U32 desired,bool& exchanged) noexcept {
    LONG before{},after{};std::memcpy(&before,&expected,4);std::memcpy(&after,&desired,4);exchanged=false;
    __try {exchanged=InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(address),after,before)==before;return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
struct HudQueryReceipt {
    gears_weapon_hud_receipt::Receipt provenance{};
    std::uint64_t call{},epoch{},frame{},input_sequence{};
    std::int64_t input_sync_ns{},applied_ns{};
    gears_weapon_hud_query::Layout layout{};
    gears_weapon_hud_query::Input before{};
    std::array<std::array<float,3>,3> desired{};
};
// Admission requires the typed Native receiver resolved on the pinned module.
// No query or firing engine function is called here.
gears_weapon_hud_query::ReplaceResult apply_hud_query(callabi::Invocation const& call,
                                                     HudQueryReceipt& out) noexcept {
    namespace hud=gears_weapon_hud_query;
    out={};const auto source=muzzle_sample;const auto submit=hud_submit.load(std::memory_order_acquire);
    if(!submit)return hud::ReplaceResult::rejected;
    hud::Layout layout{};hud::Input before{};
    if(!hud_query_layout(call,source,layout)||
       !hud::capture(weaponshot::read,call.caller_frame,layout,before))return hud::ReplaceResult::rejected;
    constexpr double unit=3.14159265358979323846/32768.;
    const double pitch=source.aim.pitch*unit,yaw=source.aim.yaw*unit;
    const std::array<double,3> direction{std::cos(pitch)*std::cos(yaw),
        std::cos(pitch)*std::sin(yaw),std::sin(pitch)};
    std::array<std::array<float,3>,3> desired{};
    if(!hud::aimed_values(before,{source.origin.x,source.origin.y,source.origin.z},direction,desired))
        return hud::ReplaceResult::rejected;
    gears_weapon_hud_receipt::Receipt provenance{};
    provenance.call=call;provenance.source=source.source;
    provenance.root_frame=call.caller_frame;provenance.hold_generation=source.hold_generation;
    provenance.applied_ns=weaponhand::clock_ns(nullptr);
    provenance.start=desired[0];provenance.end=desired[1];provenance.direction=desired[2];
    if(gears_weapon_hud_receipt::validate(provenance,call,source.source,provenance.applied_ns)!=
       gears_weapon_hud_receipt::Reason::accepted)return hud::ReplaceResult::rejected;
    // No engine call, wait or callback occurs during this synchronous operation.
    // Frame/node/locals are rechecked by replace before each individual word.
    auto lifetime=[&]{return call.thread_id==GetCurrentThreadId()&&weaponhand::current_gt(nullptr);};
    auto writable=[&]{return lifetime()&&live_sample(source,weaponhand::clock_ns(nullptr))&&
        call.hand_cookie==source.source.key.registration_cookie&&
        call.native_started_qpc==source.source.key.native_started_qpc;};
    const auto result=hud::replace(weaponshot::read,compare_word,writable,lifetime,layout,before,desired);
    if(result==hud::ReplaceResult::uncertain){
        write_faulted.store(true,std::memory_order_release);
        control_running.store(false,std::memory_order_release);
    }else if(result==hud::ReplaceResult::applied){
        ++hud_admissions[6];
        provenance.applied_ns=weaponhand::clock_ns(nullptr);
        if(!submit_hud_receipt(submit,&provenance)){
            auto changed=before;changed.values=desired;
            const auto rollback=hud::replace(weaponshot::read,compare_word,lifetime,lifetime,
                layout,changed,before.values);
            if(rollback==hud::ReplaceResult::applied)return hud::ReplaceResult::restored;
            write_faulted.store(true,std::memory_order_release);
            control_running.store(false,std::memory_order_release);return hud::ReplaceResult::uncertain;
        }
        ++hud_admissions[7];out.provenance=provenance;
        out.call=call.call_id;out.epoch=source.epoch;out.frame=source.source.key.frame;
        out.input_sequence=source.source.controllers.sequence;
        out.input_sync_ns=source.source.controllers.sync_ns;out.applied_ns=weaponhand::clock_ns(nullptr);
        out.layout=layout;out.before=before;out.desired=desired;
    }
    return result;
}
// All three words are invocation-local Result bytes, not UObject properties.
bool hud_begin(callabi::Invocation const& call) noexcept {
    // The complete direct-caller/argument checks remain in apply_hud_query.
    // Other calls cannot supply this HUD trace and need no source/schema walk.
    if(!playground::named(call.callee,L"CalcWeaponFire"))return true;
    HudQueryReceipt receipt{};
    return apply_hud_query(call,receipt)!=gears_weapon_hud_query::ReplaceResult::uncertain;
}
// A failed partial update restores only words still equal to our own values.
thread_local AppliedEvidence applied_returns;
bool fire_trace_evidence(FireTraceEvidence& out) noexcept {
    if(!weaponhand::current_gt(nullptr)||fire_trace.epoch!=control_epoch.load(std::memory_order_acquire))return false;
    out=fire_trace;return out.count!=0;
}
bool applied_evidence(AppliedEvidence& out) noexcept {
    if(!weaponhand::current_gt(nullptr)||applied_returns.epoch!=control_epoch.load(std::memory_order_acquire))return false;
    out=applied_returns;return true;
}
void note_applied(Pending const& entry,SourceScope const& source,
                  std::array<U32,3> const& prior_words) noexcept {
    const auto epoch=control_epoch.load(std::memory_order_acquire);
    if(applied_returns.epoch!=epoch){applied_returns={};applied_returns.epoch=epoch;}
    const auto kind=unsigned(entry.signature.kind);
    if(kind<1||kind>4)return;
    if(applied_returns.shot!=entry.packet.shot){
        applied_returns.follow_target_known=false;
        std::memset(applied_returns.world_grip,0,sizeof(applied_returns.world_grip));
        std::memset(applied_returns.target_mesh,0,sizeof(applied_returns.target_mesh));
    }
    auto& count=applied_returns.writes[kind-1];
    if(count!=(std::numeric_limits<std::uint64_t>::max)())++count;
    if(auto* slot=trajectory_slot(entry.packet.shot,epoch)){
        const bool had_write=slot->record.writes[0]||slot->record.writes[1]||
                             slot->record.writes[2]||slot->record.writes[3];
        if(had_write&&(slot->record.write_frame!=source.sample.source.key.frame||
                      slot->record.write_input_sequence!=source.sample.source.controllers.sequence||
                      slot->record.write_input_sync_ns!=source.sample.source.controllers.sync_ns))
            slot->record.write_source_consistent=false;
        auto& shot_count=slot->record.writes[kind-1];
        if(shot_count!=(std::numeric_limits<std::uint64_t>::max)())++shot_count;
        slot->record.write_frame=source.sample.source.key.frame;
        slot->record.write_input_sequence=source.sample.source.controllers.sequence;
        slot->record.write_input_sync_ns=source.sample.source.controllers.sync_ns;
        ++slot->record.revision;
    }
    applied_returns.shot=entry.packet.shot;
    auto const& sampled=source.sample;
    auto const& grip=sampled.source.controllers.hands[1].grip.position;
    applied_returns.input_sequence=sampled.source.controllers.sequence;
    applied_returns.frame=sampled.source.key.frame;
    applied_returns.input_sync_ns=sampled.source.controllers.sync_ns;
    applied_returns.sampled_ns=sampled.sampled_ns;
    applied_returns.right_grip[0]=grip.x;
    applied_returns.right_grip[1]=grip.y;
    applied_returns.right_grip[2]=grip.z;
    applied_returns.original_socket_origin[0]=sampled.original_socket_origin.x;
    applied_returns.original_socket_origin[1]=sampled.original_socket_origin.y;
    applied_returns.original_socket_origin[2]=sampled.original_socket_origin.z;
    applied_returns.socket_rotation[0]=sampled.socket_rotation.pitch;
    applied_returns.socket_rotation[1]=sampled.socket_rotation.yaw;
    applied_returns.socket_rotation[2]=sampled.socket_rotation.roll;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    if(sampled.follow_target_known){
        applied_returns.follow_target_known=true;
        applied_returns.world_grip[0]=sampled.follow_world_grip.x;
        applied_returns.world_grip[1]=sampled.follow_world_grip.y;
        applied_returns.world_grip[2]=sampled.follow_world_grip.z;
        applied_returns.target_mesh[0]=sampled.follow_target_mesh.x;
        applied_returns.target_mesh[1]=sampled.follow_target_mesh.y;
        applied_returns.target_mesh[2]=sampled.follow_target_mesh.z;
    }
#endif
    if(entry.signature.kind==ResultKind::base_aim){
        std::memcpy(applied_returns.aim,entry.desired.data(),12);
        std::memcpy(applied_returns.original_aim,prior_words.data(),12);
    }else std::memcpy(applied_returns.origin,entry.desired.data(),12);
}
bool store_result(Pending& entry,SourceScope const& source) noexcept {
    std::array<U32,3> original_words{};
    if(!copy(original_words.data(),entry.call.result,12)||!write_context(entry,source))return true;
    unsigned changed{};
    for(;changed<3;++changed){bool exchanged{};
        if(!write_context(entry,source)||!compare_word(entry.call.result+changed*4,original_words[changed],entry.desired[changed],exchanged)||!exchanged)break;
    }
    if(changed==3&&write_context(entry,source)){note_applied(entry,source,original_words);return true;}
    bool restored=true;
    while(changed){--changed;bool exchanged{};
        if(!compare_word(entry.call.result+changed*4,entry.desired[changed],original_words[changed],exchanged)||!exchanged)restored=false;
    }
    if(!restored){entry.uncertain=true;write_faulted.store(true,std::memory_order_release);control_running.store(false,std::memory_order_release);}
    return restored;
}
// Observe the original CalcWeaponFire return while its call/result storage is
// still live. Never issue another trace or write the collision result.
void observe_impact_return(callabi::Invocation const& call) noexcept {
    // Cheap rejection only; full schema/shot admission remains below. Avoid
    // repeating live weapon observation for unrelated return callbacks.
    auto* source=active_source();
    if(!source)return;
    if(!playground::named(call.callee,L"CalcWeaponFire"))return;
    if(!held_current(source->sample))return;
    shot::Packet packet{};unsigned locals_size{};
    if(!packet_for(call,*source,packet)||!fire_trace_schema(call,*source,locals_size))return;
    auto* slot=trajectory_slot(packet.shot,source->sample.epoch);
    if(!slot||!slot->record.trace_valid)return;
    slot->record.impact_stage=1;++slot->record.revision;
    metadata::Snapshot schema{};schema.module_base=base;
    metadata::Reader r{schema,metadata::Stage::function_path};
    metadata::Structure fn{};
    if(!metadata::tables(base,schema.objects,schema.names)||!r.structure(source->binding.identities[0].address,fn))return;
    uptr impact{};unsigned extent{},hit_offset{},array_offset{};bool found_return{},found_hit{};
    std::array<uptr,metadata::max_fields> visited{};unsigned count{};
    for(auto child=fn.children;child;){
        if(count==visited.size())return;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return;
        visited[count++]=child;metadata::Property p{};
        if(!r.property(child,p)||schema.identities[p.identity].outer!=source->binding.identities[0].address)return;
        if(r.named(p.identity,L"ImpactList")){
            uptr inner{};metadata::Property element{};
            if(found_return||p.dimension!=1||p.element_bytes!=16||p.offset!=36||
               !weaponhand::core_type(r,p.type,L"ArrayProperty")||
               !r.at(child,0xd0,inner)||!r.property(inner,element)||
               !weaponhand::core_type(r,element.type,L"StructProperty")||
               !r.at(inner,0xd0,impact))return;
            array_offset=p.offset;extent=element.element_bytes;found_return=true;
        }
        child=p.next;
    }
    slot->record.impact_stage=2;
    U32 identity{},kind{};metadata::Structure body{};
    if(!found_return)return;
    slot->record.impact_stage=21;
    if(!r.path(impact,identity)||!r.named(identity,L"ImpactInfo"))return;
    slot->record.impact_stage=22;
    if(!r.path(schema.identities[identity].klass,kind)||!weaponhand::core_type(r,kind,L"ScriptStruct"))return;
    slot->record.impact_stage=23;
    // Reader::structure intentionally accepts only Class/Function. ImpactInfo
    // is the ScriptStruct authenticated above; read its layout explicitly.
    if(!r.at(impact,0x80,body.children)||!r.at(impact,0x88,body.extent))return;
    slot->record.impact_stage=24;
    if(body.extent!=extent||extent<12||extent>4096)return;
    slot->record.impact_stage=3;
    visited={};count=0;
    for(auto child=body.children;child;){
        if(count==visited.size())return;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return;
        visited[count++]=child;metadata::Property p{};
        if(!r.property(child,p)||schema.identities[p.identity].outer!=impact)return;
        if(r.named(p.identity,L"HitLocation")){
            if(found_hit||p.dimension!=1||!core_struct(r,p,L"Vector")||!metadata::property_fits(p,extent))return;
            hit_offset=p.offset;found_hit=true;
        }
        child=p.next;
    }
    slot->record.impact_stage=4;
    gears_weapon_call_trace::Frame before{},after{};
    uptr locals{},again_locals{};metadata::Table hits{},again_hits{};
    if(!found_hit||!gears_ui_call_boundary::frame(weaponshot::read,call.caller_frame,before)||
       before.node!=source->binding.identities[0].address||before.object!=call.self||
       before.previous!=source->call.caller_frame||
       !gears_weapon_call_trace::script_pc(before.code,source->binding.script,source->binding.bytes)||
       !copy(&locals,call.caller_frame+0x30,8)||!locals||locals>UINTPTR_MAX-locals_size||
       !copy(&hits,locals+array_offset,sizeof(hits))||hits.count<1||hits.count>64||
       hits.capacity<hits.count||hits.capacity>1024||!hits.data||hits.data>UINTPTR_MAX-extent)return;
    slot->record.impact_stage=5;
    std::array<float,3> hit{},again{};
    uptr children_again{};unsigned extent_again{};
    if(!copy(hit.data(),hits.data+hit_offset,12)||!retained_metadata(r)||
       !r.at(impact,0x80,children_again)||children_again!=body.children||
       !r.at(impact,0x88,extent_again)||extent_again!=body.extent||
       !copy(again.data(),hits.data+hit_offset,12)||hit!=again||
       !copy(&again_hits,locals+array_offset,sizeof(again_hits))||!metadata::same_table(hits,again_hits)||
       !copy(&again_locals,call.caller_frame+0x30,8)||again_locals!=locals||
       !gears_ui_call_boundary::frame(weaponshot::read,call.caller_frame,after)||
       before.node!=after.node||before.object!=after.object||before.code!=after.code||before.previous!=after.previous)return;
    for(float value:hit)if(!std::isfinite(value))return;
    std::memcpy(slot->record.impact_location,hit.data(),12);
    slot->record.impact_valid=true;slot->record.impact_stage=6;++slot->record.revision;
}
bool normal_return(callabi::Invocation const& call) noexcept {
    observe_impact_return(call);
    Pending* entry{};for(auto& candidate:pending)if(candidate.live&&candidate.call.call_id==call.call_id){entry=&candidate;break;}
    if(!entry)return true;
    if(entry->uncertain||!callabi::same(entry->call,call))return false;
    auto* source=active_source();
    if(source&&write_context(*entry,*source)){
        shot::Packet packet{};ReturnSignature fresh{};
        if(packet_for(call,*source,packet)&&packet.call==entry->packet.call&&packet.shot==entry->packet.shot&&
           shot::same_grip(packet.grip,entry->packet.grip)&&typed_result(call,source->sample,fresh)&&
           same_signature(entry->signature,fresh)&&write_context(*entry,*source)&&!store_result(*entry,*source))return false;
    }
    *entry={};return true;
}
bool cleanup(callabi::Invocation const& call,shot::Token const& scope) noexcept {
    bool clean=true;
    for(auto& entry:pending)if(entry.live&&entry.call.call_id==call.call_id){
        if(entry.uncertain||!callabi::same(entry.call,call))clean=false;else entry={};
    }
    if(scope.entered&&scope.previous_depth<source_scopes.size()){
        auto& source=source_scopes[scope.previous_depth];
        if(source.scope.entered&&source.scope.call==scope.call&&callabi::same(source.call,call))source={};
    }
    return clean;
}
} // namespace unified_weapon_fire
