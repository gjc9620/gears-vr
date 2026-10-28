// Unified manual Lancer muzzle sampling. Include after weapon_hand_client.inl,
// weapon_lancer_asset_read.inl and weapon_shot_scope.inl. Engine calls belong
// ONLY to prepare_current on normal PostPlayerTick, never Native notices.
// Native still owns CallFunction/original. The normal-return consumers below
// never invoke an engine function, consume VM parameters, wait or take a lock.
#include "weapon_unified_fire.h"
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
    hand::Frame source{};
    lancermain_asset::Evidence asset{};
    std::uint64_t hold_generation{},epoch{};
    std::int64_t sampled_ns{};
    playground::Vector origin{};
    playground::Rotation socket_rotation{},aim{};
    bool valid{};
};
bool preflight_returns(MuzzleSample const& sample) noexcept;
thread_local MuzzleSample muzzle_sample;
thread_local bool sampling{};
thread_local unsigned prepare_stage{};

bool retained_metadata(metadata::Reader& r) noexcept {
    auto& schema=r.result;
    const auto identities=schema.identity_count,structures=schema.structure_count,fields=schema.field_count;
    for(U32 i=0;i<identities;++i){U32 slot{};
        if(!r.identity(schema.identities[i].address,slot)||slot!=i)return false;}
    for(U32 i=0;i<structures;++i){metadata::Structure current{};
        if(!r.structure(schema.identities[schema.structures[i].identity].address,current)||
           !metadata::same_structure(current,schema.structures[i]))return false;}
    for(U32 i=0;i<fields;++i){metadata::Property current{};
        if(!r.property(schema.identities[schema.fields[i].identity].address,current)||
           !metadata::same_property(current,schema.fields[i]))return false;}
    for(U32 i=0;i<schema.name_count;++i){metadata::NameText current{};
        if(!metadata::fresh_name(schema.names,schema.name_text[i].index,current)||
           !metadata::same_name_text(current,schema.name_text[i]))return false;}
    metadata::Table objects{},names{};
    return base==schema.module_base&&metadata::tables(base,objects,names)&&
        metadata::same_table(objects,schema.objects)&&metadata::same_table(names,schema.names)&&
        schema.failure.reason==metadata::Reason::none;
}
template<std::size_t N>bool core_struct(metadata::Reader& r,metadata::Property const& property,
    wchar_t const(&label)[N]) noexcept {
    uptr structure{};U32 slot{},kind{},owner{},owner_kind{},package{},extent{};
    return weaponhand::core_type(r,property.type,L"StructProperty")&&property.element_bytes==12&&
        r.at(r.result.identities[property.identity].address,0xd0,structure)&&r.path(structure,slot)&&r.named(slot,label)&&
        r.path(r.result.identities[slot].klass,kind)&&weaponhand::core_type(r,kind,L"ScriptStruct")&&
        r.path(r.result.identities[slot].outer,owner)&&r.named(owner,L"Object")&&
        r.path(r.result.identities[owner].klass,owner_kind)&&weaponhand::core_type(r,owner_kind,L"Class")&&
        r.path(r.result.identities[owner].outer,package)&&r.named(package,L"Core")&&
        !r.result.identities[package].outer&&r.at(structure,0x88,extent)&&extent==12;
}
bool socket_getter_schema(playground::Args const& call,metadata::Reader& r,U32& return_mask) noexcept {
    if(!call.valid||!call.r.function||call.r.size>4096)return false;
    U32 fn{},kind{},owner{},package{};metadata::Structure structure{};
    if(!r.path(call.r.function,fn)||!r.named(fn,L"GetSocketWorldLocationAndRotation")||
       !r.path(r.result.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(r.result.identities[fn].outer,owner)||!r.named(owner,L"SkeletalMeshComponent")||
       !r.path(r.result.identities[owner].outer,package)||!r.named(package,L"Engine")||
       r.result.identities[package].outer||!r.structure(call.r.function,structure))return false;
    std::uint16_t parameter_bytes{},return_offset{};
    if(!r.at(call.r.function,0x106,parameter_bytes)||parameter_bytes!=call.r.size||
       !r.at(call.r.function,0x108,return_offset))return false;
    unsigned found{};std::array<uptr,16> visited{};unsigned count{};
    for(auto child=structure.children;child;){
        if(count==visited.size())return false;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;
        visited[count++]=child;metadata::Property property{};
        if(!r.property(child,property)||r.result.identities[property.identity].outer!=call.r.function||
           property.dimension!=1||!metadata::property_fits(property,structure.extent)||
           ((property.flags&0x80)&&!metadata::property_fits(property,parameter_bytes)))return false;
        if(r.result.field_count==r.result.fields.size())return false;
        r.result.fields[r.result.field_count++]=property;
        if(property.flags&0x80){
            unsigned bit{};
            if(r.named(property.identity,L"InSocketName")&&weaponhand::core_type(r,property.type,L"NameProperty")&&
               property.element_bytes==8&&(property.flags&0x580)==0x80)bit=1;
            else if(r.named(property.identity,L"OutLocation")&&core_struct(r,property,L"Vector")&&
               (property.flags&0x580)==0x180)bit=2;
            else if(r.named(property.identity,L"OutRotation")&&core_struct(r,property,L"Rotator")&&
               (property.flags&0x580)==0x180)bit=4;
            else if(r.named(property.identity,L"ReturnValue")&&weaponhand::core_type(r,property.type,L"BoolProperty")&&
               property.element_bytes==4&&(property.flags&0x480)==0x480&&property.offset==return_offset){
                if(!r.at(child,0xd0,return_mask)||!return_mask||(return_mask&(return_mask-1)))return false;
                bit=8;
            }else if(r.named(property.identity,L"Space")&&weaponhand::core_type(r,property.type,L"IntProperty")&&
               property.element_bytes==4&&(property.flags&0x590)==0x90){
                // Optional coordinate-space argument stays at its ordinary
                // zero/world default in Args' zero-initialized parameter block.
                bit=16;
            }else return false;
            if(found&bit)return false;found|=bit;
        }
        child=property.next;
    }
    return (found&15)==15&&retained_metadata(r);
}
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
bool prepare_current(hand::Frame const& frame,hand::GripGameState const& game) noexcept {
    clear_source();prepare_stage=0;
    const auto now=weaponhand::clock_ns(nullptr);
    if(!manual_profile()||!control_running.load(std::memory_order_acquire)||write_faulted.load(std::memory_order_acquire)||
       sampling||!enabled.load(std::memory_order_acquire)||!weaponhand::current_gt(nullptr)||
       !weaponhand::client.armed()||!weaponhand::client.source_current_on_gt(frame.key)||!hand::frame_active(frame,now)||
       !game.known||!game.main_hand_held||game.animation_owns_hands||game.busy)return false;
    prepare_stage=1;
    MuzzleSample candidate{};candidate.source=frame;candidate.hold_generation=weaponhand::held_binding.generation;
    candidate.epoch=control_epoch.load(std::memory_order_acquire);
    if(!candidate.hold_generation||lancermain_asset::read(frame,candidate.asset)!=lancermain_asset::Status::ready)return false;
    prepare_stage=2;
    Name muzzle{};if(!configured_muzzle(frame,muzzle))return false;
    prepare_stage=3;
    playground::Args call(candidate.asset.component.address,L"GetSocketWorldLocationAndRotation");
    metadata::Snapshot schema{};schema.module_base=base;metadata::Reader r{schema,metadata::Stage::final_check};U32 mask{};
    if(!metadata::tables(base,schema.objects,schema.names)||!socket_getter_schema(call,r,mask))return false;
    prepare_stage=4;
    if(!call.set(L"InSocketName",muzzle).valid)return false;
    const auto identities_before=schema.identity_count;
    sampling=true;const bool ran=call.run();sampling=false;
    U32 after_mask{};
    if(!ran||!socket_getter_schema(call,r,after_mask)||schema.identity_count!=identities_before||
       mask!=after_mask||(call.output<U32>()&mask)==0)return false;
    prepare_stage=5;
    candidate.origin=call.output<playground::Vector>(L"OutLocation");
    candidate.socket_rotation=call.output<playground::Rotation>(L"OutRotation");
    if(!std::isfinite(candidate.origin.x)||!std::isfinite(candidate.origin.y)||!std::isfinite(candidate.origin.z)||
       !aim_from_socket(candidate.socket_rotation,candidate.aim))return false;
    prepare_stage=6;
    lancermain_asset::Evidence after_asset{};hand::GripGameState after{};
    candidate.sampled_ns=weaponhand::clock_ns(nullptr);
    if(!control_running.load(std::memory_order_acquire)||candidate.epoch!=control_epoch.load(std::memory_order_acquire)||
       !hand::frame_active(frame,candidate.sampled_ns)||!weaponhand::client.source_current_on_gt(frame.key)||
       !weaponhand::client.armed()||candidate.hold_generation!=weaponhand::held_binding.generation||
       !weaponhand::observe(nullptr,frame,candidate.sampled_ns,after)||!hand::same_game(game,after)||
       lancermain_asset::read(frame,after_asset)!=lancermain_asset::Status::ready||
       !hand::same(candidate.asset.component,after_asset.component)||!hand::same(candidate.asset.asset,after_asset.asset)||
       !preflight_returns(candidate))return false;
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
    candidate.valid=true;muzzle_sample=candidate;return true;
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
        hand::frame_active(sample.source,now)&&held_current(sample)&&
        weaponhand::client.registration_alive(sample.source.key.registration_cookie,sample.source.key.native_started_qpc);
}
bool ready_for(hand::Frame const& frame) noexcept {
    return live_sample(muzzle_sample,weaponhand::clock_ns(nullptr))&&hand::same_frame(frame,muzzle_sample.source);
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
bool typed_result(callabi::Invocation const& call,MuzzleSample const& sample,ReturnSignature& out) noexcept {
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
        if(!live_sample(source.sample,weaponhand::clock_ns(nullptr))||!hand::same(weapon,source.sample.source.weapon)||
           call.self!=weapon.address||call.hand_cookie!=source.sample.source.key.registration_cookie||
           call.native_started_qpc!=source.sample.source.key.native_started_qpc)return;
        source.valid=true;return;
    }
    auto* source=active_source();
    if(!source||!control_running.load(std::memory_order_acquire)||source->sample.epoch!=control_epoch.load(std::memory_order_acquire)||
       write_faulted.load(std::memory_order_acquire)||!held_current(source->sample))return;
    shot::Packet packet{};
    if(!packet_for(call,*source,packet))return;
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
// All three words are invocation-local Result bytes, not UObject properties.
// A failed partial update restores only words still equal to our own values.
thread_local AppliedEvidence applied_returns;
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
    auto& count=applied_returns.writes[kind-1];
    if(count!=(std::numeric_limits<std::uint64_t>::max)())++count;
    applied_returns.shot=entry.packet.shot;
    auto const& sampled=source.sample;
    auto const& grip=sampled.source.controllers.hands[1].grip.position;
    applied_returns.input_sequence=sampled.source.controllers.sequence;
    applied_returns.input_sync_ns=sampled.source.controllers.sync_ns;
    applied_returns.sampled_ns=sampled.sampled_ns;
    applied_returns.right_grip[0]=grip.x;
    applied_returns.right_grip[1]=grip.y;
    applied_returns.right_grip[2]=grip.z;
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
bool normal_return(callabi::Invocation const& call) noexcept {
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
