// Gnasher's original CustomFire/FireAllPellets getter adapter. Preserve the
// original pellet spread, point-blank branches, pushed/pulled origins and all
// collision/impact results. Only original base aim, base trace and muzzle
// getters consume the current authenticated gun source. No callback engine call.
namespace gnasher_muzzle {
namespace f=unified_weapon_fire;namespace hand=gears_weapon_hand;
namespace call=gears_weapon_call;namespace shot=gears_weapon_shot;
struct Scope {f::SourceScope source{};shot::Packet admission{};bool normal_returned{};};
thread_local f::MuzzleSample sample;
thread_local std::array<Scope,f::scope_capacity> scopes;
thread_local std::array<f::Pending,f::pending_capacity> pending;
void clear_source() noexcept {sample={};}
bool preflight(f::MuzzleSample const& source) noexcept {
    const wchar_t* labels[]={L"GetAdjustedAimFor",L"GetWeaponStartTraceLocation",L"GetMuzzleLoc"};
    const f::ResultKind kinds[]={f::ResultKind::base_aim,f::ResultKind::trace_origin,f::ResultKind::physical_origin};
    alignas(4) std::array<f::U32,3> scratch{};
    for(unsigned i=0;i<3;++i){
        const auto receiver=i==2?source.source.weapon.address:source.source.pawn.address;
        playground::Args descriptor(receiver,labels[i]);
        if(!descriptor.valid)return false;
        call::Invocation invoked{};invoked.self=receiver;invoked.callee=descriptor.r.function;
        invoked.result=reinterpret_cast<uptr>(scratch.data());f::ReturnSignature signature{};
        if(!f::typed_result(invoked,source,signature,true)||signature.kind!=kinds[i])return false;
    }
    return true;
}
unsigned prepare_gate{};
// A negative scheduling hint for ONE original undispatched press. Mechanical
// preparation must already belong to this same source; the existing Input
// owns the unchanged intent deadline/counters and each later muzzle admission.
bool defer_new_press(hand::Frame const& source) noexcept {
    const auto* entry=gnasher_fire::entry(source,false);
    return entry&&entry->prepared&&!entry->cycle.blocked()&&
        entry->identity==lancerreload_inventory::identity(source)&&hand::same_frame(entry->source,source);
}
bool reject_prepare(hand::Frame const& source,unsigned gate) noexcept {
    prepare_gate=gate;
    static unsigned emitted{},previous_gate{},previous_refire{};
    const auto refire=gnasher_refire::prepare_stage;
    if(log_file&&emitted<16&&(gate!=previous_gate||refire!=previous_refire)){
        ++emitted;previous_gate=gate;previous_refire=refire;
        fwprintf(log_file,L"gnasher_muzzle_prepare_gate gate=%u refire_stage=%u frame=%llu sequence=%llu sync_ns=%lld now_ns=%lld\n",
            gate,refire,source.key.frame,source.controllers.sequence,source.controllers.sync_ns,weaponhand::clock_ns(nullptr));
        fflush(log_file);
    }
    return false;
}
bool prepare(hand::Frame const& source,hand::GripGameState const& game) noexcept {
    clear_source();auto* entry=gnasher_fire::entry(source,false);
    if(!entry)return reject_prepare(source,1);
    if(!entry->prepared)return reject_prepare(source,2);
    if(entry->cycle.blocked())return reject_prepare(source,3);
    if(!(entry->identity==lancerreload_inventory::identity(source)))return reject_prepare(source,4);
    if(!playground::instance(source.weapon.address,L"Weap_Shotgun"))return reject_prepare(source,5);
    if(!lancerreload_manual::bound_for(source))return reject_prepare(source,6);
    if(!gnasher_refire::prepare(source))return reject_prepare(source,7);
    if(!f::prepare_source(source,game,lancermain_asset::AssetScope::gnasher_custom,sample,preflight))return reject_prepare(source,8);
    prepare_gate=0;return true;
}
void rejected_readiness(hand::Frame const& source,std::int64_t now,std::int64_t reserve,unsigned reason,gnasher_fire::Entry const* entry) noexcept {
    // Bounded GT observation only. Keep the actual sample and producer clocks;
    // this never prepares a source, calls the engine or grants admission.
    static unsigned emitted{},previous_reason{},previous_flags{};
    if(emitted>=16||!log_file||!weaponhand::current_gt(nullptr))return;
    unsigned flags{};
    if(sample.valid)flags|=1;
    if(f::manual_profile())flags|=2;
    if(f::control_running.load(std::memory_order_acquire))flags|=4;
    if(!f::write_faulted.load(std::memory_order_acquire))flags|=8;
    if(sample.epoch==f::control_epoch.load(std::memory_order_acquire))flags|=16;
    if(enabled.load(std::memory_order_acquire))flags|=32;
    if(f::held_current(sample))flags|=64;
    if(weaponhand::client.registration_alive(sample.source.key.registration_cookie,sample.source.key.native_started_qpc))flags|=128;
    if(hand::same_frame(source,sample.source))flags|=256;
    if(reserve>=0&&reserve<=100000000&&now<=INT64_MAX-reserve){
        const auto reserved=now+reserve;
        if(sample.sampled_ns>0&&reserved>=sample.sampled_ns&&reserved-sample.sampled_ns<=100000000)flags|=512;
        if(!sample.render_acquisition_ns||(reserved>=sample.render_acquisition_ns&&reserved-sample.render_acquisition_ns<=100000000))flags|=1024;
        if(hand::frame_active(sample.source,reserved))flags|=2048;
    }
    if(emitted&&reason==previous_reason&&flags==previous_flags)return;
    previous_reason=reason;previous_flags=flags;++emitted;
    fwprintf(log_file,L"gnasher_muzzle_readiness reason=%u flags=%u service_stage=%u mechanical_flags=%u observation_rejection=%u now_ns=%lld reserve_ns=%lld sampled_ns=%lld render_ns=%lld sync_ns=%lld input_received_ns=%lld stereo_ns=%lld source_now_ns=%lld sample_epoch=%llu control_epoch=%llu\n",
        reason,flags,entry?entry->preparation_stage:0,entry?entry->preparation_flags:0,entry?entry->preparation_rejection:0,static_cast<long long>(now),static_cast<long long>(reserve),
        static_cast<long long>(sample.sampled_ns),static_cast<long long>(sample.render_acquisition_ns),
        static_cast<long long>(sample.source.controllers.sync_ns),static_cast<long long>(sample.source.controllers.received_ns),
        static_cast<long long>(sample.source.stereo.received_ns),
        static_cast<long long>(sample.source.now_ns),static_cast<unsigned long long>(sample.epoch),
        static_cast<unsigned long long>(f::control_epoch.load(std::memory_order_acquire)));
    fflush(log_file);
}
bool ready_for(hand::Frame const& source,std::int64_t reserve) noexcept {
    const auto now=weaponhand::clock_ns(nullptr);auto* entry=gnasher_fire::entry(source,false);
    unsigned reason{};
    // Preserve the original predicate order and short circuit engine queries.
    if(!entry)reason=1;
    else if(!entry->prepared)reason=2;
    else if(entry->cycle.blocked())reason=3;
    else if(reserve<0||reserve>100000000||now>INT64_MAX-reserve)reason=4;
    else if(!f::live_sample(sample,now+reserve))reason=5;
    else if(!hand::same_frame(source,sample.source))reason=6;
    else if(!gnasher_refire::bound_for(source))reason=7;
    if(reason){rejected_readiness(source,now,reserve,reason,entry);return false;}
    return true;
}
Scope* active() noexcept {
    const auto depth=weaponshot::stack.depth();if(!depth||depth>scopes.size())return nullptr;
    auto& slot=scopes[depth-1];auto& source=slot.source;shot::Binding binding{};hand::ObjectIdentity weapon{};
    if(!source.valid||!source.scope.entered||source.scope.previous_depth+1!=depth||
       !weaponshot::stack.binding(binding,weapon)||!shot::same(binding,source.binding)||
       !hand::same(weapon,source.sample.source.weapon))return nullptr;
    return &slot;
}
bool packet(call::Invocation const& invoked,Scope const& slot,shot::Packet& out) noexcept {
    auto const& source=slot.source;
    return f::packet_for(invoked,source,out)&&out.root==shot::Role::shotgun_custom&&
        (out.mode==shot::Role::shotgun_custom||out.mode==shot::Role::shotgun_pellets)&&
        out.shot==slot.admission.shot&&out.call==slot.admission.call&&shot::same_grip(out.grip,slot.admission.grip);
}
void begin(call::Invocation const& invoked,shot::Token const& token) noexcept {
    if(!f::manual_profile()||!weaponhand::current_gt(nullptr)||invoked.thread_id!=GetCurrentThreadId())return;
    if(token.entered){
        if(token.previous_depth>=scopes.size())return;
        auto& slot=scopes[token.previous_depth];slot={};auto& source=slot.source;source.scope=token;source.call=invoked;
        hand::ObjectIdentity weapon{};
        if(!call::invocation_valid(invoked)||!weaponshot::stack.binding(source.binding,weapon)||
           !weaponshot::stack.invocation_packet(token,invoked.callee,weapon,slot.admission)||
           slot.admission.root!=shot::Role::shotgun_custom)return;
        if(source.binding.role==shot::Role::shotgun_custom){
            if(token.previous_depth)return; // Never borrow a nested custom shot.
            source.sample=sample;
        }else if(source.binding.role==shot::Role::shotgun_pellets){
            if(!token.previous_depth)return;
            auto const& parent=scopes[token.previous_depth-1];
            if(!parent.source.valid||parent.source.binding.role!=shot::Role::shotgun_custom||
               token.previous_call!=parent.source.scope.call||slot.admission.shot!=parent.admission.shot||
               !shot::same_grip(slot.admission.grip,parent.admission.grip))return;
            source.sample=parent.source.sample;
        }else return;
        if(!f::live_sample(source.sample,weaponhand::clock_ns(nullptr))||
           !f::source_matches_shot(source.sample,slot.admission)||!hand::same(weapon,source.sample.source.weapon)||
           invoked.self!=weapon.address||invoked.hand_cookie!=source.sample.source.key.registration_cookie||
           invoked.native_started_qpc!=source.sample.source.key.native_started_qpc)return;
        source.valid=true;return;
    }
    if(!invoked.result)return;
    auto* source=active();if(!source)return;
    if(!playground::named(invoked.callee,L"GetAdjustedAimFor")&&
       !playground::named(invoked.callee,L"GetWeaponStartTraceLocation")&&
       !playground::named(invoked.callee,L"GetMuzzleLoc"))return;
    if(!f::live_sample(source->source.sample,weaponhand::clock_ns(nullptr)))return;
    shot::Packet envelope{};f::ReturnSignature signature{};
    if(!packet(invoked,*source,envelope)||!f::typed_result(invoked,source->source.sample,signature,true))return;
    if(signature.kind!=f::ResultKind::base_aim&&signature.kind!=f::ResultKind::trace_origin&&signature.kind!=f::ResultKind::physical_origin)return;
    f::Pending* slot{};
    for(auto& p:pending){if(p.live&&p.call.call_id==invoked.call_id)return;if(!p.live&&!slot)slot=&p;}
    if(!slot)return;
    f::Pending value{};value.live=true;value.depth=weaponshot::stack.depth();value.scope_call=source->source.scope.call;
    value.epoch=source->source.sample.epoch;value.call=invoked;value.packet=envelope;value.signature=signature;
    if(signature.kind==f::ResultKind::base_aim)std::memcpy(value.desired.data(),&source->source.sample.aim,12);
    else std::memcpy(value.desired.data(),&source->source.sample.origin,12);
    *slot=value;
}
bool returned(call::Invocation const& invoked) noexcept {
    if(!gnasher_refire::returned(invoked))return false;
    if(auto* finished=active())if(call::same(finished->source.call,invoked))finished->normal_returned=true;
    f::Pending* entry{};for(auto& p:pending)if(p.live&&p.call.call_id==invoked.call_id){entry=&p;break;}
    if(!entry)return true;
    if(entry->uncertain||!call::same(entry->call,invoked))return false;
    auto* source=active();
    if(source&&f::live_sample(source->source.sample,weaponhand::clock_ns(nullptr))&&f::write_context(*entry,source->source)){
        shot::Packet envelope{};f::ReturnSignature signature{};
        if(packet(invoked,*source,envelope)&&envelope.call==entry->packet.call&&envelope.shot==entry->packet.shot&&
           shot::same_grip(envelope.grip,entry->packet.grip)&&f::typed_result(invoked,source->source.sample,signature,true)&&
           f::same_signature(entry->signature,signature)&&f::write_context(*entry,source->source)&&
           !f::store_result(*entry,source->source))return false;
    }
    *entry={};return true;
}
bool cleanup(call::Invocation const& invoked,shot::Token const& token) noexcept {
    bool clean=true;
    for(auto& p:pending)if(p.live&&p.call.call_id==invoked.call_id){
        if(p.uncertain||!call::same(p.call,invoked))clean=false;else p={};
    }
    if(token.entered&&token.previous_depth<scopes.size()){
        auto& slot=scopes[token.previous_depth];
        if(slot.source.scope.entered&&slot.source.scope.call==token.call&&call::same(slot.source.call,invoked)){
            if(slot.source.binding.role==shot::Role::shotgun_custom){
                if(!slot.normal_returned)gnasher_fire::abandoned(invoked,token);
                clear_source();
            }
            slot={};
        }
    }
    return clean;
}
}
