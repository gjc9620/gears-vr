// Bow-owned original projectile consumer, adapted from the existing common
// projectile return pattern. All Bow admission and receipts are separate.
// Original projectile getter consumer on the existing synchronous Call owner.
// Reuses the common metadata, source preparation and compare/rollback writer.
// Never overwrites GetProjectileFirePosition, SafeSpawnLoc or collision results.
namespace torquebow_fire {
namespace f=unified_weapon_fire;namespace hand=gears_weapon_hand;
namespace call=gears_weapon_call;namespace shot=gears_weapon_shot;
thread_local f::MuzzleSample sample;
thread_local std::array<f::SourceScope,f::scope_capacity> scopes;
thread_local std::array<f::Pending,f::pending_capacity> pending;
hand::ObjectIdentity arrow_class{};
std::array<hand::ObjectIdentity,f::scope_capacity> source_classes{};
std::array<std::uint64_t,f::scope_capacity> source_owners{};
std::array<SpawnReceipt,4> spawn_receipts{};bool spawn_overflow{};
std::uint64_t last_spawn_call{};
unsigned prepare_gate{};
bool peek_spawn(SpawnReceipt& out) noexcept {
    out={};for(const auto& r:spawn_receipts)if(r.call&&(!out.call||r.call<out.call))out=r;
    return out.call!=0;
}
bool consume_spawn(std::uint64_t id) noexcept {
    if(!id)return false;for(auto& r:spawn_receipts)if(r.call==id){r={};return true;}return false;
}
void clear_source() noexcept {sample={};arrow_class={};}
bool preflight(f::MuzzleSample const& source) noexcept {
    const wchar_t* labels[]={L"GetAdjustedAimFor",L"GetMuzzleLoc",L"GetWeaponStartTraceLocation"};
    const f::ResultKind kinds[]={f::ResultKind::base_aim,f::ResultKind::physical_origin,f::ResultKind::trace_origin};
    alignas(4) std::array<f::U32,3> scratch{};
    for(unsigned i=0;i<3;++i){
        const auto receiver=i==1?source.source.weapon.address:source.source.pawn.address;
        playground::Args descriptor(receiver,labels[i]);
        if(!descriptor.valid)return false;
        call::Invocation identity{};identity.self=receiver;identity.callee=descriptor.r.function;
        identity.result=reinterpret_cast<uptr>(scratch.data());f::ReturnSignature signature{};
        if(!f::typed_result(identity,source,signature,true)||signature.kind!=kinds[i])return false;
    }
    return true;
}
bool reload_veto_bound(hand::Frame const& source) noexcept {
    if(!lancerreload_manual::bound_for(source))return false;
    const auto& current_binding=lancerreload_manual::binding;
    // The current binding alone is insufficient if the bounded retained-
    // inventory table is full. Its actual callback must retain THIS actor.
    for(const auto& b:lancerreload_manual::inventory_bindings){
        if(b.valid&&!b.starter&&b.cookie==source.key.registration_cookie&&b.native==source.key.native_started_qpc&&
           hand::same(b.world,source.world)&&hand::same(b.controller,source.controller)&&
           hand::same(b.pawn,source.pawn)&&hand::same(b.weapon,source.weapon)&&
           hand::same(b.function,current_binding.function)&&hand::same(b.property,current_binding.property)&&
           b.script_hash==current_binding.script_hash&&lancerreload_manual::layout(b))return true;
    }
    return false;
}
bool prepare(hand::Frame const& source,hand::GripGameState const& game) noexcept {
    clear_source();prepare_gate=1;
    // An empty original BeginFire can otherwise request a timed refill before
    // any physical reload. Do not admit the first shot without its owned veto.
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    if(!reload_veto_bound(source))return false;
#else
    return false;
#endif
    hand::ObjectIdentity expected{},closing{};
    prepare_gate=2;if(spawn_overflow)return false;
    prepare_gate=3;if(!torquebow_projectile_class::read(source,expected)){clear_source();return false;}
    prepare_gate=4;if(!f::prepare_source(source,game,lancermain_asset::AssetScope::torque_bow_projectile,sample,preflight)){clear_source();return false;}
    prepare_gate=5;if(!torquebow_projectile_class::read(source,closing)){clear_source();return false;}
    prepare_gate=6;if(!hand::same(expected,closing)){clear_source();return false;}
    arrow_class=expected;
    prepare_gate=7;if(!torquebow_muzzle_clear::query(sample)){
        clear_source();return false;
    }
    prepare_gate=0;return true;
}
bool ready_for(hand::Frame const& source,std::int64_t reserve) noexcept {
    const auto now=weaponhand::clock_ns(nullptr);
    return !spawn_overflow&&hand::identity_valid(arrow_class)&&reserve>=0&&reserve<=100000000&&now<=INT64_MAX-reserve&&
        f::live_sample(sample,now+reserve)&&hand::same_frame(source,sample.source);
}
f::SourceScope* active() noexcept {
    const auto depth=weaponshot::stack.depth();
    if(!depth||depth>scopes.size())return nullptr;
    auto& source=scopes[depth-1];return source.valid?&source:nullptr;
}
bool packet(call::Invocation const& invoked,f::SourceScope const& source,shot::Packet& out) noexcept {
    const auto depth=weaponshot::stack.depth();
    return depth&&depth<=source_owners.size()&&source_owners[depth-1]&&
        torquebow_input_owner::projectile_owner(source.sample.source)==source_owners[depth-1]&&
        call::invocation_valid(invoked)&&invoked.thread_id==GetCurrentThreadId()&&weaponhand::current_gt(nullptr)&&
        invoked.hand_cookie==source.sample.source.key.registration_cookie&&
        invoked.native_started_qpc==source.sample.source.key.native_started_qpc&&
        weaponshot::active_binding_current()&&
        weaponshot::stack.projectile_packet(source.sample.source.weapon,invoked.caller_frame,weaponshot::read,out)&&
        (out.grip.grip==hand::Grip::single||out.grip.grip==hand::Grip::two)&&
        f::source_matches_shot(source.sample,out)&&weaponshot::write_admitted(out);
}
void begin(call::Invocation const& invoked,shot::Token const& scope) noexcept {
    if(!f::manual_profile()||!weaponhand::current_gt(nullptr)||invoked.thread_id!=GetCurrentThreadId())return;
    if(scope.entered){
        if(scope.previous_depth>=scopes.size())return;
        auto& source=scopes[scope.previous_depth];source={};source.scope=scope;source.call=invoked;
        source_owners[scope.previous_depth]=0;source_classes[scope.previous_depth]={};
        hand::ObjectIdentity weapon{};
        const auto input_serial=torquebow_input_owner::projectile_owner(sample.source);
        if(!call::invocation_valid(invoked)||!weaponshot::stack.binding(source.binding,weapon)||
           (source.binding.role!=shot::Role::war_projectile&&source.binding.role!=shot::Role::engine_projectile)||
           !input_serial||!hand::identity_valid(arrow_class)||!f::live_sample(sample,weaponhand::clock_ns(nullptr))||!hand::same(weapon,sample.source.weapon)||
           invoked.self!=weapon.address||invoked.hand_cookie!=sample.source.key.registration_cookie||
           invoked.native_started_qpc!=sample.source.key.native_started_qpc)return;
        source.sample=sample;source_classes[scope.previous_depth]=arrow_class;
        source_owners[scope.previous_depth]=input_serial;source.valid=true;return;
    }
    if(!invoked.result)return;
    auto* source=active();if(!source)return;
    if(!playground::named(invoked.callee,L"GetMuzzleLoc")&&
       !playground::named(invoked.callee,L"GetAdjustedAimFor")&&
       !playground::named(invoked.callee,L"GetWeaponStartTraceLocation"))return;
    if(!f::live_sample(source->sample,weaponhand::clock_ns(nullptr)))return;
    shot::Packet envelope{};f::ReturnSignature signature{};
    if(!packet(invoked,*source,envelope)||!f::typed_result(invoked,source->sample,signature,true))return;
    f::Pending* slot{};
    for(auto& entry:pending){if(entry.live&&entry.call.call_id==invoked.call_id)return;if(!entry.live&&!slot)slot=&entry;}
    if(!slot)return;
    f::Pending entry{};entry.live=true;entry.depth=weaponshot::stack.depth();entry.scope_call=source->scope.call;
    entry.epoch=source->sample.epoch;entry.call=invoked;entry.packet=envelope;entry.signature=signature;
    if(signature.kind==f::ResultKind::base_aim)std::memcpy(entry.desired.data(),&source->sample.aim,12);
    else if(signature.kind==f::ResultKind::physical_origin||signature.kind==f::ResultKind::trace_origin)
        std::memcpy(entry.desired.data(),&source->sample.origin,12);
    else return;
    *slot=entry;
}
void observe_projectile_return(call::Invocation const& invoked) noexcept {
    // A receipt accounts for an ALREADY spawned original arrow. Pose expiry
    // after root entry cannot erase that fact; it never grants a new shot.
    auto* source=active();
    if(!source||!call::same(source->call,invoked)||invoked.call_id<=last_spawn_call||
       !playground::named(invoked.callee,L"ProjectileFire")||source->binding.role!=shot::Role::war_projectile||
       !invoked.result||(invoked.result&7)||!weaponshot::active_binding_current()||
       !weaponhand::client.registration_alive(invoked.hand_cookie,invoked.native_started_qpc))return;
    namespace meta=gripdamage_metadata;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    meta::Structure structure{};std::uint16_t bytes{},offset{};
    if(!meta::tables(base,schema.objects,schema.names)||!r.structure(invoked.callee,structure)||
       !r.at(invoked.callee,0x106,bytes)||bytes!=8||!r.at(invoked.callee,0x108,offset)||offset)return;
    std::array<uptr,meta::max_fields> visited{};unsigned count{},returns{};
    for(auto child=structure.children;child;){
        if(count==visited.size())return;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return;visited[count++]=child;
        meta::Property p{};
        if(!r.property(child,p)||r.result.identities[p.identity].outer!=invoked.callee)return;
        if(p.flags&0x400){
            if(++returns!=1||!r.named(p.identity,L"ReturnValue")||(p.flags&0x480)!=0x480||
               p.offset||p.dimension!=1||p.element_bytes!=8||!weaponhand::core_type(r,p.type,L"ObjectProperty"))return;
        }
        child=p.next;
    }
    const auto depth=weaponshot::stack.depth();
    if(!depth||depth>source_owners.size()||!source_owners[depth-1])return;
    const auto& frame=source->sample.source;const auto expected=source_classes[depth-1];
    shot::Packet original_shot{};
    if(!weaponshot::stack.invocation_packet(source->scope,invoked.callee,frame.weapon,original_shot))return;
    uptr projectile{};hand::ObjectIdentity object{},current_class{};meta::PointerField owner{},instigator{};
    if(returns!=1||!copy(&projectile,invoked.result,8)||!projectile||
       !lancermain_asset::identify(r,projectile,object)||object.klass!=expected.address||
       !weaponhand::object(r,expected)||!weaponhand::derives(r,projectile,L"Proj_Arrow_base",L"WarfareGame")||
       !weaponhand::pointer(r,projectile,L"Owner",frame.weapon.address,owner)||
       !weaponhand::pointer(r,projectile,L"Instigator",frame.pawn.address,instigator)||
       !torquebow_projectile_class::read(frame,current_class)||!hand::same(expected,current_class)||
       !f::retained_metadata(r)||!weaponshot::active_binding_current())return;
    // Retain until the GT owner matches the exact ammo delta and acknowledges
    // this call. No callback I/O, ammo query, retry or projectile mutation.
    for(auto& receipt:spawn_receipts)if(!receipt.call){
        receipt={frame,object,expected,invoked.call_id,original_shot.shot,source_owners[depth-1],weaponhand::clock_ns(nullptr)};
        last_spawn_call=invoked.call_id;return;
    }
    spawn_overflow=true;
}
bool returned(call::Invocation const& invoked) noexcept {
    observe_projectile_return(invoked);
    f::Pending* entry{};for(auto& candidate:pending)if(candidate.live&&candidate.call.call_id==invoked.call_id){entry=&candidate;break;}
    if(!entry)return true;
    if(entry->uncertain||!call::same(entry->call,invoked))return false;
    auto* source=active();
    if(source&&f::live_sample(source->sample,weaponhand::clock_ns(nullptr))&&f::write_context(*entry,*source)){
        shot::Packet envelope{};f::ReturnSignature signature{};
        if(packet(invoked,*source,envelope)&&envelope.call==entry->packet.call&&envelope.shot==entry->packet.shot&&
           shot::same_grip(envelope.grip,entry->packet.grip)&&f::typed_result(invoked,source->sample,signature,true)&&
           f::same_signature(entry->signature,signature)&&f::write_context(*entry,*source)&&!f::store_result(*entry,*source))return false;
    }
    *entry={};return true;
}
bool cleanup(call::Invocation const& invoked,shot::Token const& scope) noexcept {
    bool clean=true;
    for(auto& entry:pending)if(entry.live&&entry.call.call_id==invoked.call_id){
        if(entry.uncertain||!call::same(entry.call,invoked))clean=false;else entry={};
    }
    if(scope.entered&&scope.previous_depth<scopes.size()){
        auto& source=scopes[scope.previous_depth];
        if(source.scope.entered&&source.scope.call==scope.call&&call::same(source.call,invoked))source={};
        source_classes[scope.previous_depth]={};
        source_owners[scope.previous_depth]=0;
    }
    return clean;
}
}
