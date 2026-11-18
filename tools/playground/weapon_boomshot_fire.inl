// Original projectile getter consumer on the existing synchronous Call owner.
// Reuses the common metadata, source preparation and compare/rollback writer.
// Never overwrites GetProjectileFirePosition, SafeSpawnLoc or collision results.
namespace boomshot_fire {
namespace f=unified_weapon_fire;namespace hand=gears_weapon_hand;
namespace call=gears_weapon_call;namespace shot=gears_weapon_shot;
thread_local f::MuzzleSample sample;
thread_local std::array<f::SourceScope,f::scope_capacity> scopes;
thread_local std::array<f::Pending,f::pending_capacity> pending;
std::uint64_t projectile_receipts{};
void clear_source() noexcept {sample={};}
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
bool prepare(hand::Frame const& source,hand::GripGameState const& game) noexcept {
    clear_source();
    if(boomshot_runtime::blocks_fire(source))return false;
    // An empty original BeginFire can otherwise request a timed refill before
    // any physical reload. Do not admit the first shot without its owned veto.
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    if(!lancerreload_manual::bound_for(source))return false;
#else
    return false;
#endif
    if(!f::prepare_source(source,game,lancermain_asset::AssetScope::boomshot_projectile,sample,preflight))return false;
    if(!boomshot_muzzle_clear::query(sample)){
        clear_source();return false;
    }
    return true;
}
bool ready_for(hand::Frame const& source,std::int64_t reserve) noexcept {
    const auto now=weaponhand::clock_ns(nullptr);
    return !boomshot_runtime::blocks_fire(source)&&reserve>=0&&reserve<=100000000&&now<=INT64_MAX-reserve&&
        f::live_sample(sample,now+reserve)&&hand::same_frame(source,sample.source);
}
f::SourceScope* active() noexcept {
    const auto depth=weaponshot::stack.depth();
    if(!depth||depth>scopes.size())return nullptr;
    auto& source=scopes[depth-1];return source.valid?&source:nullptr;
}
bool packet(call::Invocation const& invoked,f::SourceScope const& source,shot::Packet& out) noexcept {
    return !boomshot_runtime::blocks_fire(source.sample.source)&&call::invocation_valid(invoked)&&invoked.thread_id==GetCurrentThreadId()&&weaponhand::current_gt(nullptr)&&
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
        hand::ObjectIdentity weapon{};
        if(!call::invocation_valid(invoked)||!weaponshot::stack.binding(source.binding,weapon)||
           (source.binding.role!=shot::Role::war_projectile&&source.binding.role!=shot::Role::engine_projectile)||
           !f::live_sample(sample,weaponhand::clock_ns(nullptr))||!hand::same(weapon,sample.source.weapon)||
           invoked.self!=weapon.address||invoked.hand_cookie!=sample.source.key.registration_cookie||
           invoked.native_started_qpc!=sample.source.key.native_started_qpc)return;
        source.sample=sample;source.valid=true;return;
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
    // Read-only original spawn receipt, before this exact root scope leaves.
    // No object getter, retry, projectile mutation or new fire is dispatched.
    auto* source=active();
    if(!source||!call::same(source->call,invoked)||!playground::named(invoked.callee,L"ProjectileFire")||
       source->binding.role!=shot::Role::war_projectile||!invoked.result||(invoked.result&7)||
       !weaponshot::active_binding_current()||!f::held_current(source->sample)||
       !weaponhand::client.registration_alive(invoked.hand_cookie,invoked.native_started_qpc))return;
    namespace meta=gripdamage_metadata;
    meta::Snapshot report_schema{};report_schema.module_base=base;meta::Reader r{report_schema,meta::Stage::final_check};
    meta::Structure structure{};std::uint16_t bytes{},offset{};
    if(!meta::tables(base,report_schema.objects,report_schema.names)||!r.structure(invoked.callee,structure)||
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
    uptr projectile{};hand::ObjectIdentity object{};
    if(returns!=1||!copy(&projectile,invoked.result,8)||!projectile||
       !lancermain_asset::identify(r,projectile,object)||
       !weaponhand::derives(r,projectile,L"Proj_Boomer",L"warfaregamecontentweapons")||
       !f::retained_metadata(r)||!weaponshot::active_binding_current())return;
    // Observe the ORIGINAL returned projectile, never synthesize a shot or trajectory.
    playground::Vector projectile_location{},projectile_velocity{};playground::Rotation projectile_rotation{};
    weaponhand::BoolField location_field{},velocity_field{},rotation_field{};
    const bool projectile_geometry=weaponhand::field(r,projectile,L"Location",L"StructProperty",12,location_field)&&
        weaponhand::field(r,projectile,L"Velocity",L"StructProperty",12,velocity_field)&&
        weaponhand::field(r,projectile,L"Rotation",L"StructProperty",12,rotation_field)&&
        r.at(projectile,location_field.property.offset,projectile_location)&&
        r.at(projectile,velocity_field.property.offset,projectile_velocity)&&
        r.at(projectile,rotation_field.property.offset,projectile_rotation)&&f::retained_metadata(r);
    if(log_file&&projectile_receipts<16){
        fwprintf(log_file,L"boomshot_projectile_geometry call=%llu projectile=%llx known=%u muzzle=%.9g,%.9g,%.9g aim=%d,%d,%d location=%.9g,%.9g,%.9g velocity=%.9g,%.9g,%.9g rotation=%d,%d,%d observation_only=1\n",
            invoked.call_id,static_cast<unsigned long long>(projectile),unsigned(projectile_geometry),
            double(source->sample.origin.x),double(source->sample.origin.y),double(source->sample.origin.z),
            source->sample.aim.pitch,source->sample.aim.yaw,source->sample.aim.roll,
            double(projectile_location.x),double(projectile_location.y),double(projectile_location.z),
            double(projectile_velocity.x),double(projectile_velocity.y),double(projectile_velocity.z),
            projectile_rotation.pitch,projectile_rotation.yaw,projectile_rotation.roll);fflush(log_file);
    }
    ++projectile_receipts;
    if(log_file&&projectile_receipts<=16){
        fwprintf(log_file,L"boomshot_original_projectile receipt=%llu call=%llu frame=%llu input_sequence=%llu weapon=%llx projectile=%llx class=Proj_Boomer original_spawn_return=1\n",
            projectile_receipts,invoked.call_id,source->sample.source.key.frame,source->sample.source.controllers.sequence,
            static_cast<unsigned long long>(source->sample.source.weapon.address),static_cast<unsigned long long>(object.address));fflush(log_file);
    }
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
    }
    return clean;
}
}
