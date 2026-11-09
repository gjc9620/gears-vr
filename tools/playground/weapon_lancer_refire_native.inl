// The existing ProcessEvent owner retains ONE actual original firing event
// until normal PostPlayerTick can prepare its current muzzle, before ammo.
// No VM frame, timer, input edge or inventory value is rewritten here.
#include "../vr-common/weapon_lancer_refire_loop.h"
#include "../vr-common/weapon_hammerburst_refire.h"
namespace lancerrefire {
namespace meta=gripdamage_metadata;
namespace wh=gears_weapon_hand;
struct Script {
    Array array{};std::array<unsigned char,512> bytes{};unsigned bound{256};
};
namespace policy=gears_lancer_refire_loop;
struct Cohort {
    weaponhand::HoldBinding held{};wh::FrameKey key{};
    std::uint64_t reference{},session{},space{},input{},profile{};
    std::uint32_t presses{},releases{};bool known{};
};
Cohort press;
bool press_dispatch{},delivering{},pending_validated{};
unsigned service_stage{},identity_stage{},state_stage{},handoff_stage{},wait_logs{};
struct Deferred {
    policy::DeferredEvent event{};Cohort cohort{};
    wh::ObjectIdentity function{};Script body{};
    std::array<unsigned char,8> parameters{};unsigned parameter_bytes{};
    uptr refire{};
} pending_fire;
bool same_hold(weaponhand::HoldBinding const& a,weaponhand::HoldBinding const& b) noexcept {
    return a.known&&b.known&&a.generation==b.generation&&a.native_cookie==b.native_cookie&&
        a.native_started==b.native_started&&a.world_generation==b.world_generation&&
        wh::same(a.world,b.world)&&wh::same(a.controller,b.controller)&&wh::same(a.pawn,b.pawn)&&wh::same(a.weapon,b.weapon);
}
Cohort cohort(wh::Frame const& f) noexcept {
    Cohort c{};c.held=weaponhand::held_binding;c.key=f.key;c.key.frame=0;
    c.reference=f.reference_generation;c.session=f.stereo.session;c.space=f.stereo.space_epoch;
    c.input=f.controllers.input_epoch;c.profile=f.controllers.profile_generation;
    c.presses=f.controllers.hands[1].pressed[gears_vr::control_trigger];
    c.releases=f.controllers.hands[1].released[gears_vr::control_trigger];
    c.known=c.held.known;return c;
}
bool matches(Cohort const& c,wh::Frame const& f) noexcept {
    auto key=f.key;key.frame=0;
    return c.known&&same_hold(c.held,weaponhand::held_binding)&&wh::same(key,c.key)&&
        c.reference==f.reference_generation&&c.session==f.stereo.session&&c.space==f.stereo.space_epoch&&
        c.input==f.controllers.input_epoch&&c.profile==f.controllers.profile_generation&&
        c.presses==f.controllers.hands[1].pressed[gears_vr::control_trigger]&&
        c.releases==f.controllers.hands[1].released[gears_vr::control_trigger]&&
        f.controllers.hands[1].trigger_active==1&&f.controllers.hands[1].trigger>.35f;
}
// Set only around the already authenticated original R2 ButtonPress call.
// This contains ancestry/counters, not a retained pose or a freshness permit.
void begin_owned_press(wh::Frame const& f) noexcept {press=cohort(f);press_dispatch=true;}
void end_owned_press(bool invoked) noexcept {press_dispatch=false;if(!invoked)press.known=false;}
bool release_required() noexcept {
#if defined(GEARS_PLAYGROUND_HB_FINITE_BURST) && GEARS_PLAYGROUND_HB_FINITE_BURST
    if(hammerburst_burst_event::requires_release())return true;
#endif
    return pending_fire.event.release_required(weaponhand::clock_ns(nullptr));
}
bool has_initial_debt() noexcept {return pending_fire.event.first_debt();}
void report_wait(unsigned previous) noexcept {
    if(log_file&&wait_logs<12&&(previous!=service_stage||handoff_stage)){
        fwprintf(log_file,L"native_lancer_original_fire_wait stage=%u identity=%u state=%u handoff=%u kind=%u release_required=%u\n",
            service_stage,identity_stage,state_stage,handoff_stage,unsigned(pending_fire.event.kind()),unsigned(release_required()));
        fflush(log_file);++wait_logs;
    }
}
unsigned proof_logs{},defer_logs{};
std::uint64_t admitted{},deferred{},proof_rejected{};
weaponhand::HoldBinding certified_hold{};
wh::FrameKey certified_key{};std::uint64_t certified_reference{},certified_session{},certified_space{},certified_input{},certified_profile{};
bool certified{};
bool certified_for(wh::Frame const& frame) noexcept {
    auto key=frame.key;key.frame=0;const auto& held=weaponhand::held_binding;
    return certified&&!release_required()&&vrfire::mode.load(std::memory_order_acquire)==vrfire::running&&held.known&&
        held.generation==certified_hold.generation&&held.native_cookie==certified_hold.native_cookie&&
        held.native_started==certified_hold.native_started&&held.world_generation==certified_hold.world_generation&&
        wh::same(held.world,certified_hold.world)&&wh::same(held.controller,certified_hold.controller)&&
        wh::same(held.pawn,certified_hold.pawn)&&wh::same(held.weapon,certified_hold.weapon)&&
        wh::same(key,certified_key)&&frame.reference_generation==certified_reference&&
        frame.stereo.session==certified_session&&frame.stereo.space_epoch==certified_space&&
        frame.controllers.input_epoch==certified_input&&frame.controllers.profile_generation==certified_profile;
}
bool script(meta::Reader& r,uptr function,Script& out,unsigned bound=256) noexcept {
    if(!bound||bound>out.bytes.size()||!r.at(function,0x8c,out.array)||!out.array.data||out.array.count<1||
       out.array.count>int(bound)||out.array.capacity<out.array.count||out.array.capacity>4096)return false;
    out.bound=bound;
    return copy(out.bytes.data(),out.array.data,std::size_t(out.array.count));
}
bool unchanged(meta::Reader& r,uptr function,Script const& before) noexcept {
    Script after{};
    return script(r,function,after,before.bound)&&!std::memcmp(&before.array,&after.array,sizeof(Array))&&
        !std::memcmp(before.bytes.data(),after.bytes.data(),std::size_t(before.array.count));
}
template<std::size_t N>bool weapon_function(meta::Reader& r,uptr function,
    wchar_t const(&label)[N],unsigned& slot,meta::Structure& structure,bool allow_war=false) noexcept {
    unsigned kind{},owner{},owner_kind{},package{};
    return r.path(function,slot)&&r.named(slot,label)&&
        r.path(r.result.identities[slot].klass,kind)&&weaponhand::core_type(r,kind,L"Function")&&
        r.path(r.result.identities[slot].outer,owner)&&
        r.path(r.result.identities[owner].klass,owner_kind)&&weaponhand::core_type(r,owner_kind,L"Class")&&
        r.path(r.result.identities[owner].outer,package)&&!r.result.identities[package].outer&&
        ((r.named(owner,L"Weapon")&&r.named(package,L"Engine"))||
         (allow_war&&r.named(owner,L"WarWeapon")&&r.named(package,L"WarfareGame")))&&r.structure(function,structure);
}
bool original_loop(meta::Reader& r,uptr weapon,uptr function,unsigned& reason) noexcept;
#include "weapon_hammerburst_refire.inl"
bool original_loop(meta::Reader& r,uptr weapon,uptr function,unsigned& reason) noexcept {
    if(hammerburst_refire::candidate(r,weapon,function))return hammerburst_refire::original_loop(r,weapon,function,reason);
    reason=1;unsigned fn{},kind{},state{},state_kind{},owner{},owner_kind{},package{};
    meta::Structure refire{};std::uint16_t parameters{},result{},native{};unsigned flags{};uptr exec{};
    if(!r.path(function,fn)||!r.named(fn,L"RefireCheckTimer")||
       !r.path(r.result.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(r.result.identities[fn].outer,state)||!r.named(state,L"WeaponFiring")||
       !r.path(r.result.identities[state].klass,state_kind)||!weaponhand::core_type(r,state_kind,L"State")||
       !r.path(r.result.identities[state].outer,owner)||!r.named(owner,L"Weapon")||
       !r.path(r.result.identities[owner].klass,owner_kind)||!weaponhand::core_type(r,owner_kind,L"Class")||
       !r.path(r.result.identities[owner].outer,package)||!r.named(package,L"Engine")||
       r.result.identities[package].outer||!r.structure(function,refire))return false;
    reason=2;
    if(!r.at(function,0x106,parameters)||parameters||!r.at(function,0x108,result)||result!=0xffff||
       !r.at(function,0xf8,native)||native||!r.at(function,0xf4,flags)||(flags&0x400)||
       !r.at(function,0x114,exec)||exec!=base+0x1d6ca0||refire.children||refire.extent)return false;
    playground::Args timing(weapon,L"TimeWeaponFiring"),interval(weapon,L"GetFireInterval");
    unsigned timing_slot{},interval_slot{};meta::Structure timing_structure{},interval_structure{};
    reason=3;
    if(!timing.valid||!interval.valid||!weapon_function(r,timing.r.function,L"TimeWeaponFiring",timing_slot,timing_structure)||
       !weapon_function(r,interval.r.function,L"GetFireInterval",interval_slot,interval_structure,true)||
       !r.at(timing.r.function,0xf8,native)||native||!r.at(timing.r.function,0xf4,flags)||(flags&0x400)||
       !r.at(timing.r.function,0x114,exec)||exec!=base+0x1d6ca0||
       !r.at(timing.r.function,0x108,result)||result!=0xffff)return false;
    reason=4;meta::Property mode{};const auto property=timing.prop(L"FireModeNum");
    if(!property||!r.property(property,mode)||!r.named(mode.identity,L"FireModeNum")||
       r.result.identities[mode.identity].outer!=timing.r.function||!weaponhand::core_type(r,mode.type,L"ByteProperty")||
       mode.dimension!=1||mode.element_bytes!=1||(mode.flags&0x580)!=0x80||
       !meta::property_fits(mode,timing.r.size)||!meta::property_fits(mode,timing_structure.extent))return false;
    r.result.fields[r.result.field_count++]=mode;
    reason=5;Script timing_script{},refire_script{};
    if(!script(r,timing.r.function,timing_script)||!script(r,function,refire_script))return false;
    const auto interval_name=r.result.identities[interval_slot].name;
    const auto timer_name=r.result.identities[fn].name;
    reason=6;
    if(!gears_lancer_refire_loop::looping_timer(timing_script.bytes.data(),std::size_t(timing_script.array.count),
       {interval_name.index,interval_name.number},property,{timer_name.index,timer_name.number})){
        if(log_file&&proof_logs<2){
            fwprintf(log_file,L"native_lancer_refire_loop_unmatched bytes=%d script=",timing_script.array.count);
            for(int i=0;i<timing_script.array.count;++i)fwprintf(log_file,L"%02x",unsigned(timing_script.bytes[std::size_t(i)]));
            fwprintf(log_file,L"\n");fflush(log_file);
        }
        return false;
    }
    reason=7;
    if(!unified_weapon_fire::retained_metadata(r)||!unchanged(r,timing.r.function,timing_script)||!unchanged(r,function,refire_script))return false;
    reason=0;return true;
}
void certify(Cohort const& c) noexcept {
    certified_hold=c.held;certified_key=c.key;certified_reference=c.reference;
    certified_session=c.session;certified_space=c.space;certified_input=c.input;
    certified_profile=c.profile;certified=c.known;
}
wh::ObjectIdentity identity(meta::Reader& r,unsigned slot) noexcept {
    auto const& id=r.result.identities[slot];
    return {id.address,id.outer,id.klass,id.index,id.name.index,id.name.number,1};
}
bool queue(meta::Reader& r,policy::EventKind kind,uptr function,void const* parameters,
    unsigned parameter_bytes,uptr refire,Cohort const* original_intent=nullptr,unsigned script_bound=256) noexcept {
    const auto now=weaponhand::clock_ns(nullptr);
    if(!pending_fire.event.empty())return pending_fire.function.address==function&&
        same_hold(pending_fire.cohort.held,weaponhand::held_binding)&&pending_fire.event.capture(kind,now);
    unsigned slot{};Script body{};auto const& intent=original_intent?*original_intent:press;
    if(!intent.known||parameter_bytes>pending_fire.parameters.size()||!r.path(function,slot)||!script(r,function,body,script_bound))return false;
    std::array<unsigned char,8> actual{};
    if(parameter_bytes&&(!parameters||!copy(actual.data(),reinterpret_cast<uptr>(parameters),parameter_bytes)))return false;
    if(!pending_fire.event.capture(kind,now))return false;
    pending_fire.cohort=intent;pending_fire.function=identity(r,slot);pending_fire.body=body;
    pending_fire.parameters=actual;pending_fire.parameter_bytes=parameter_bytes;pending_fire.refire=refire;
    certify(intent);return true;
}
bool initial_proof(meta::Reader& r,uptr weapon,uptr function,Script& body,unsigned& bytes,uptr& refire,unsigned& reason) noexcept {
    unsigned fn{},type{},state{},timer{};meta::Structure s{};
    reason=1;
    if(!r.path(function,fn)||!r.named(fn,L"BeginState")||!r.path(r.result.identities[fn].klass,type)||
       !weaponhand::core_type(r,type,L"Function")||!r.path(r.result.identities[fn].outer,state)||
       !r.named(state,L"WeaponFiring"))return false;
    refire=playground::field(r.result.identities[state].address,L"RefireCheckTimer",L"Function");
    unsigned loop_reason{};
    if(!refire||!original_loop(r,weapon,refire,loop_reason)||!r.path(refire,timer)||
       r.result.identities[timer].outer!=r.result.identities[fn].outer)return false;
    reason=2;std::uint16_t count{},result{},native{};unsigned flags{};uptr exec{};
    if(!r.structure(function,s)||!r.at(function,0x106,count)||(count!=0&&count!=8)||
       !r.at(function,0x108,result)||result!=0xffff||!r.at(function,0xf8,native)||native||
       !r.at(function,0xf4,flags)||(flags&0x400)||!r.at(function,0x114,exec)||exec!=base+0x1d6ca0)return false;
    bytes=count;reason=3;meta::Property previous{};
    const auto field=playground::field(function,L"PreviousStateName");
    if(!field||!r.property(field,previous)||!r.named(previous.identity,L"PreviousStateName")||
       r.result.identities[previous.identity].outer!=function||!weaponhand::core_type(r,previous.type,L"NameProperty")||
       previous.dimension!=1||previous.element_bytes!=8||previous.offset!=0||
       !meta::property_fits(previous,s.extent)||(count==8&&(previous.flags&0x580)!=0x80)||
       (count==0&&(previous.flags&0x580))||s.children!=field||get<uptr>(field+0x68))return false;
    playground::Args ammunition(weapon,L"FireAmmunition"),timing(weapon,L"TimeWeaponFiring");
    unsigned a{},t{};meta::Structure as{},ts{};meta::Property mode{};
    const auto mode_field=playground::property(weapon,L"CurrentFireMode");
    reason=4;
    if(!ammunition.valid||!timing.valid||!weapon_function(r,ammunition.r.function,L"FireAmmunition",a,as,true)||
       !weapon_function(r,timing.r.function,L"TimeWeaponFiring",t,ts)||!mode_field||!r.property(mode_field,mode)||
       !r.named(mode.identity,L"CurrentFireMode")||!weaponhand::core_type(r,mode.type,L"ByteProperty")||
       mode.dimension!=1||mode.element_bytes!=1||!script(r,function,body))return false;
    const auto an=r.result.identities[a].name,tn=r.result.identities[t].name;
    reason=5;
    if(!policy::first_shot(body.bytes.data(),std::size_t(body.array.count),{an.index,an.number},{tn.index,tn.number},mode_field)){
        if(log_file&&proof_logs<4){fwprintf(log_file,L"native_lancer_initial_unmatched parameters=%u extent=%u bytes=%d script=",bytes,s.extent,body.array.count);
            for(int i=0;i<body.array.count;++i)fwprintf(log_file,L"%02x",unsigned(body.bytes[std::size_t(i)]));
            fwprintf(log_file,L"\n");fflush(log_file);}
        return false;
    }
    reason=6;if(!unified_weapon_fire::retained_metadata(r)||!unchanged(r,function,body))return false;
    reason=0;return true;
}
// Read-only capture before the ordinary recursion bypass: BeginState can be
// called synchronously INSIDE our authenticated ButtonPress. No engine getter
// is invoked here and no live FFrame is retained or interpreted.
bool capture_initial(uptr object,uptr function,void const* parameters) noexcept {
#if defined(GEARS_PLAYGROUND_HB_FINITE_BURST) && GEARS_PLAYGROUND_HB_FINITE_BURST
    if(!delivering&&hammerburst_burst_event::targets(object)&&
       hammerburst_burst_event::capture_initial(object,function,parameters))return true;
#endif
    if(delivering||!enabled.load(std::memory_order_acquire)||
       vrfire::mode.load(std::memory_order_acquire)!=vrfire::running||!unified_weapon_fire::manual_profile()||
       !weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready()||!press.known||
       (!press_dispatch&&!vrfire::input.owns_press())||!same_hold(press.held,weaponhand::held_binding)||
       object!=press.held.weapon.address||!playground::named(function,L"BeginState"))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,press.held.weapon)||
       lancermain_asset::held_asset_name(r,object,lancermain_asset::AssetScope::lancer_only)!=lancermain_asset::HeldAsset::lancer)return false;
    Script body{};unsigned bytes{},reason{};uptr timer{};
    const bool proved=initial_proof(r,object,function,body,bytes,timer,reason);
    if(!proved){
        ++proof_rejected;certified=false;
        if(log_file&&proof_logs++<6){fwprintf(log_file,L"native_lancer_initial_proof_rejected reason=%u parameters=%u\n",reason,bytes);fflush(log_file);}
        // Only a certified original firing-state entry can be consumed. A
        // schema mismatch stays visible; it is never treated as a passed gate.
        return false;
    }
    // A new actual state entry supersedes an unexecuted timer from the old
    // state. A duplicate first event must not bypass an expired pending first
    // and consume ammo a second time.
    if(pending_fire.event.kind()==policy::EventKind::refire){pending_fire.event.cancel();pending_validated=false;}
    const bool retained=queue(r,policy::EventKind::first,function,parameters,bytes,timer);
    if(log_file&&defer_logs++<12){fwprintf(log_file,L"native_lancer_initial_queued retained=%u parameters=%u synchronous_press=%u\n",
        unsigned(retained),bytes,unsigned(press_dispatch));fflush(log_file);}
    return retained||(pending_fire.event.first_debt()&&pending_fire.function.address==function&&
        same_hold(pending_fire.cohort.held,weaponhand::held_binding));
}
// Called only after the normal recursion/disabled bypass. Returning true
// consumes this event invocation, not the original recurring timer.
bool defer(uptr object,uptr function) noexcept {
#if defined(GEARS_PLAYGROUND_HB_FINITE_BURST) && GEARS_PLAYGROUND_HB_FINITE_BURST
    if(hammerburst_burst_event::targets(object)&&hammerburst_burst_event::defer(object,function))return true;
#endif
    if(vrfire::mode.load(std::memory_order_acquire)!=vrfire::running||
       !unified_weapon_fire::manual_profile()||!vrfire::input.owns_press()||
       !weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready()||
       !weaponhand::held_binding.known||object!=weaponhand::held_binding.weapon.address||
       !playground::named(function,L"RefireCheckTimer"))return false;
    struct Recursion {bool before{inside};Recursion(){inside=true;}~Recursion(){inside=before;}} recursion;
    weapon_metadata_parse::TickScope parsing;
    const auto held=weaponhand::held_binding;
    const auto actors=vrfire::input.actors();
    if(!wh::same(actors.weapon,held.weapon)||!wh::same(actors.controller,held.controller)||
       !wh::same(actors.pawn,held.pawn)||!wh::same(actors.world,held.world))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField putting_down{},putting_down_after{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,held.weapon)||
       // HB's finite burst can continue after real R2-up. Its timer contract
       // alone is insufficient; keep that incomplete route withdrawn until
       // original burst-completion ownership is implemented and verified.
       lancermain_asset::held_asset_name(r,object,lancermain_asset::AssetScope::lancer_only)!=lancermain_asset::HeldAsset::lancer||
       !weaponhand::boolean(r,object,L"bWeaponPutDown",putting_down)||
       (putting_down.bits&putting_down.mask))return false;
    unsigned reason{};
    if(!original_loop(r,object,function,reason)){
        certified=false;
        ++proof_rejected;
        if(log_file&&proof_logs<4){
            fwprintf(log_file,L"native_lancer_refire_proof_rejected reason=%u total=%llu params=%u result=%u extent=%u children=%llx native=%u exec_rva=%llx\n",
                reason,proof_rejected,unsigned(get<std::uint16_t>(function+0x106)),unsigned(get<std::uint16_t>(function+0x108)),
                get<unsigned>(function+0x88),get<uptr>(function+0x80),unsigned(get<std::uint16_t>(function+0xf8)),get<uptr>(function+0x114)-base);
            fflush(log_file);++proof_logs;
        }
        // Reasons3+ have already proved this exact original zero-parameter
        // void Engine timer entry. Fail closed, then the ordinary Input tick
        // hands back R2 because no loop certificate remains.
        return reason>=3;
    }
    // Never forward a new due timer and then replay the older pending timer
    // in PostTick. Coalesce without refreshing the first captured deadline.
    if(!pending_fire.event.empty())return true;
    vrfire::lr::Game game{};
    // A metadata-only read has no FrameKey, so read_game intentionally does
    // not grant its main_hand_held bit. Check the actual equipment generation
    // here for a possible deferral; only the fresh Native Frame below can
    // authorize forwarding a shot. Never forge a key for this observation.
    if(!vrfire::read_game(actors,nullptr,game)||!game.known||!game.lancer||!game.r2_known){certified=false;return true;}
    if(!game.r2_held){certified=false;pending_fire.event.cancel();return false;} // Original timer finishes normal Active handoff.
    if(game.animation||game.busy||lancerreload_runtime::blocks_fire(held.weapon)||
       held.generation!=vrequip::model.hold_generation()||vrequip::model.animation_owns_hands()||vrequip::model.switch_pending()||
       !weaponhand::boolean(r,object,L"bWeaponPutDown",putting_down_after)||
       !meta::same_property(putting_down.property,putting_down_after.property)||
       putting_down.mask!=putting_down_after.mask||(putting_down_after.bits&putting_down_after.mask)||
       held.generation!=weaponhand::held_binding.generation||!weaponhand::held_binding.known||
       !vrfire::input.owns_press()){certified=false;return true;}
    // All expensive identity/schema work precedes the new source observation.
    // Current timestamps are never copied onto an older controller/eye pose.
    wh::Frame frame{};wh::GripGameState copied_game{};
    bool ready=weaponhand::client.copy_frame_on_gt(frame,copied_game)&&
        vrfire::lr::same(actors,vrfire::actors(frame))&&copied_game.known&&copied_game.main_hand_held&&
        !copied_game.animation_owns_hands&&!copied_game.busy&&
        frame.controllers.hands[1].trigger_active==1&&frame.controllers.hands[1].trigger>.35f;
    if(ready){
        // This certificate can only retain the already-owned input during a
        // transient muzzle wait. Every original timer invocation still passes
        // through this fresh pre-ammo check; it grants no shot by itself.
        certified_hold=held;certified_key=frame.key;certified_key.frame=0;
        certified_reference=frame.reference_generation;certified_session=frame.stereo.session;
        certified_space=frame.stereo.space_epoch;certified_input=frame.controllers.input_epoch;
        certified_profile=frame.controllers.profile_generation;certified=true;
        ready=unified_weapon_fire::ready_for(frame,20000000);
        if(!ready){
            nativelancerfollow::tick(frame,nativelancerfollow::Phase::prepare_only);
            ready=unified_weapon_fire::prepare_current(frame,copied_game)&&unified_weapon_fire::ready_for(frame,20000000);
        }
    }
    if(ready){
        if(log_file&&!admitted){fwprintf(log_file,L"native_lancer_refire_admitted original_loop_proved=1 frame=%llu source_age_ns=%lld\n",
            frame.key.frame,weaponhand::clock_ns(nullptr)-frame.controllers.sync_ns);fflush(log_file);}
        ++admitted;return false;
    }
    unified_weapon_fire::clear_source();++deferred;
    // Retain this actual timer invocation for the same normal PostTick as the
    // new hand publication. Repeated due timers never renew its first time.
    if(press.known&&same_hold(press.held,held))queue(r,policy::EventKind::refire,function,nullptr,0,function);
    if(log_file&&defer_logs<8){
        fwprintf(log_file,L"native_lancer_refire_deferred total=%llu admitted=%llu frame=%llu source_age_ns=%lld prepare_stage=%u\n",
            deferred,admitted,frame.key.frame,frame.controllers.sync_ns?weaponhand::clock_ns(nullptr)-frame.controllers.sync_ns:0,
            unified_weapon_fire::prepare_stage);fflush(log_file);++defer_logs;
    }
    return true;
}
bool state_name(meta::Reader& r,uptr weapon,Name& value) noexcept {
    playground::Args call(weapon,L"GetStateName");meta::Property result{};
    unsigned fn{},kind{},owner{},package{};
    const auto field=call.prop(L"ReturnValue");
    state_stage=1;if(!call.valid||call.r.size!=8||get<unsigned short>(call.r.function+0x108)!=0||!field)return false;
    state_stage=2;if(!r.path(call.r.function,fn)||!r.path(r.result.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function"))return false;
    state_stage=3;if(!r.path(r.result.identities[fn].outer,owner)||!r.named(owner,L"Object")||
       !r.path(r.result.identities[owner].outer,package)||!r.named(package,L"Core")||r.result.identities[package].outer)return false;
    state_stage=4;if(!r.property(field,result)||!weaponhand::core_type(r,result.type,L"NameProperty")||
       result.dimension!=1||result.element_bytes!=8||result.offset!=0||(result.flags&0x580)!=0x580)return false;
    state_stage=5;if(!call.run())return false;
    value=call.output<Name>();state_stage=6;if(value.number)return false;state_stage=0;return true;
}
bool pending_identity(meta::Reader& r) noexcept {
    auto const& h=pending_fire.cohort.held;
    identity_stage=1;if(!same_hold(h,weaponhand::held_binding)||!weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready())return false;
    identity_stage=2;if(!weaponhand::client.registration_alive(h.native_cookie,h.native_started))return false;
    identity_stage=3;if(!weaponhand::object(r,h.world)||!weaponhand::object(r,h.controller)||!weaponhand::object(r,h.pawn)||!weaponhand::object(r,h.weapon))return false;
    identity_stage=4;if(!weaponhand::object(r,pending_fire.function)||!unchanged(r,pending_fire.function.address,pending_fire.body))return false;
    identity_stage=0;return true;
}
bool invoke_original(uptr object,uptr function,unsigned size,unsigned char const* parameters) noexcept {
    Request call{};call.object=object;call.function=function;call.size=size;
    if(size)std::memcpy(call.params,parameters,size);
    struct Dispatch {
        bool old_inside{inside},old_delivering{delivering};
        Dispatch(){inside=true;delivering=true;}
        ~Dispatch(){inside=old_inside;delivering=old_delivering;}
    } dispatch;
    return invoke_engine(&call);
}
// A cancelled first BeginState never installed TimeWeaponFiring. Before its
// owner may switch/reset, hand its state back through the ORIGINAL refire
// function, only after an actual R2-up and original ShouldRefire==false.
// No source/pose is needed to prove a no-shot state handback.
bool release_completed([[maybe_unused]] wh::Frame const* normal_source) noexcept {
#if defined(GEARS_PLAYGROUND_HB_FINITE_BURST) && GEARS_PLAYGROUND_HB_FINITE_BURST
    if(hammerburst_burst_event::cleanup_pending()){
        press.known=false;certified=false;pending_validated=false;
        return hammerburst_burst_event::release_completed(normal_source);
    }
#endif
    press.known=false;certified=false;pending_validated=false;pending_fire.event.cancel();
    if(!pending_fire.event.first_debt()){handoff_stage=0;return true;}
    handoff_stage=1;if(delivering||!weaponhand::current_gt(nullptr))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    handoff_stage=2;if(!meta::tables(base,schema.objects,schema.names)||!pending_identity(r))return false;
    vrfire::lr::Game game{};
    handoff_stage=3;if(!vrfire::read_game(vrfire::input.actors(),nullptr,game)||!game.known||!game.r2_known||game.r2_held)return false;
    unsigned fn{},owner{};Name current{};
    handoff_stage=4;if(!r.path(pending_fire.function.address,fn)||!r.path(r.result.identities[fn].outer,owner)||
       !state_name(r,game.actors.weapon.address,current))return false;
    const auto firing=r.result.identities[owner].name;
    if(current.index!=firing.index||current.number!=firing.number){pending_fire.event.reset();handoff_stage=0;return true;}
    unsigned reason{};
    handoff_stage=5;if(!original_loop(r,game.actors.weapon.address,pending_fire.refire,reason))return false;
    playground::Args should(game.actors.weapon.address,L"ShouldRefire");unsigned slot{};meta::Structure structure{};
    meta::Property result{};const auto field=should.prop(L"ReturnValue");
    handoff_stage=61;if(!should.valid||should.r.size!=4||!field||
       !weapon_function(r,should.r.function,L"ShouldRefire",slot,structure,true)||
       !r.property(field,result)||!weaponhand::core_type(r,result.type,L"BoolProperty")||
       result.dimension!=1||result.element_bytes!=4||result.offset!=0||(result.flags&0x580)!=0x580)return false;
    handoff_stage=62;if(!should.run())return false;
    handoff_stage=63;if(should.output<unsigned>())return false;
    handoff_stage=64;if(!unified_weapon_fire::retained_metadata(r)||!pending_identity(r))return false;
    handoff_stage=7;if(!invoke_original(game.actors.weapon.address,pending_fire.refire,0,nullptr)||
       !state_name(r,game.actors.weapon.address,current)||current.index==firing.index)return false;
    pending_fire.event.reset();handoff_stage=0;
    if(log_file){fwprintf(log_file,L"native_lancer_initial_cancelled original_refire_handoff=1 shot_dispatched=0\n");fflush(log_file);}
    return true;
}
void service(uptr pc) noexcept {
    pending_validated=false;
#if defined(GEARS_PLAYGROUND_HB_FINITE_BURST) && GEARS_PLAYGROUND_HB_FINITE_BURST
    if(hammerburst_burst_event::service(pc))return;
#endif
    if(pending_fire.event.empty()||delivering||!weaponhand::current_gt(nullptr)||
       pc!=pending_fire.cohort.held.controller.address)return;
    struct Report {unsigned previous{service_stage};~Report(){report_wait(previous);}} report;
    service_stage=1;
    if(vrfire::mode.load(std::memory_order_acquire)!=vrfire::running||!vrfire::input.owns_press()||release_required()){
        pending_fire.event.cancel();
        if(!vrfire::input.owns_press())release_completed();
        return;
    }
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    service_stage=2;if(!meta::tables(base,schema.objects,schema.names)||!pending_identity(r)){pending_fire.event.cancel();certified=false;return;}
    vrfire::lr::Game game{};Name current{};unsigned fn{},owner{},reason{};
    const auto weapon=pending_fire.cohort.held.weapon.address;
    service_stage=3;
    if(!vrfire::read_game(vrfire::input.actors(),nullptr,game)||!game.known||!game.r2_known||!game.r2_held||
       game.busy||game.animation||lancerreload_runtime::blocks_fire(game.actors.weapon)||
       vrequip::model.switch_pending()||vrequip::model.animation_owns_hands()||
       !r.path(pending_fire.function.address,fn)||!r.path(r.result.identities[fn].outer,owner)||
       !state_name(r,weapon,current)||current.index!=r.result.identities[owner].name.index||
       !original_loop(r,weapon,pending_fire.refire,reason)){
        pending_fire.event.cancel();certified=false;return;
    }
    // This is a same-GT ordering witness, not source authority. Run expensive
    // event/state proof BEFORE vrfire::tick takes its current publication and
    // prepares the admitted muzzle. The actual dispatch uses that exact Frame
    // immediately after the unchanged Input ledger advances.
    pending_validated=true;service_stage=4;
}
void deliver_ready(wh::Frame const& frame) noexcept {
#if defined(GEARS_PLAYGROUND_HB_FINITE_BURST) && GEARS_PLAYGROUND_HB_FINITE_BURST
    if(hammerburst_burst_event::cleanup_pending()){hammerburst_burst_event::deliver(frame);return;}
#endif
    if(!pending_validated||pending_fire.event.empty()||delivering||
       vrfire::mode.load(std::memory_order_acquire)!=vrfire::running||!vrfire::input.owns_press()||
       !weaponhand::current_gt(nullptr)||!matches(pending_fire.cohort,frame))return;
    if(!unified_weapon_fire::ready_for(frame,20000000)){
        // Reuse the existing readiness witness at the ACTUAL deferred-event
        // decision. The earlier ButtonPress witness cannot explain an event
        // captured later by the original state machine. Observe only after
        // rejection, never refresh a sample or turn observation into a permit.
        static unsigned not_ready_logs{};
        unified_weapon_fire::ReadinessEvidence evidence{};
        if(log_file&&not_ready_logs<8&&unified_weapon_fire::readiness_evidence(frame,20000000,evidence)){
            ++not_ready_logs;
            fwprintf(log_file,L"native_lancer_original_fire_not_ready kind=%u frame=%llu valid=%u same_frame=%u frame_current=%u reserved_frame_current=%u held=%u registration=%u live=%u reserved_live=%u epoch=%u sync_age_ns=%lld stereo_age_ns=%lld frame_age_ns=%lld sample_age_ns=%lld render_age_ns=%lld\n",
                unsigned(pending_fire.event.kind()),frame.key.frame,unsigned(evidence.valid),unsigned(evidence.same_frame),
                unsigned(evidence.frame_current),unsigned(evidence.reserved_frame_current),unsigned(evidence.held),
                unsigned(evidence.registration),unsigned(evidence.live),unsigned(evidence.reserved_live),unsigned(evidence.epoch),
                evidence.sync_age,evidence.stereo_age,evidence.frame_age,evidence.sample_age,evidence.render_age);fflush(log_file);
        }
        return;
    }
    const auto kind=pending_fire.event.kind();const auto function=pending_fire.function.address;
    const auto bytes=pending_fire.parameter_bytes;const auto parameters=pending_fire.parameters;
    if(!pending_fire.event.claim(weaponhand::clock_ns(nullptr)))return;
    pending_validated=false;
    // Consume before invoking. An exception or partial original execution
    // must never replay FireAmmunition or grant a second inventory decrement.
    const bool invoked=invoke_original(frame.weapon.address,function,bytes,parameters.data());service_stage=0;
    if(!invoked){certified=false;vrfire::mode.store(vrfire::fault,std::memory_order_release);}
    static unsigned delivery_logs{};
    if(log_file&&delivery_logs++<24){fwprintf(log_file,L"native_lancer_original_fire_delivered kind=%u invoked=%u frame=%llu source_age_ns=%lld\n",
        unsigned(kind),unsigned(invoked),frame.key.frame,weaponhand::clock_ns(nullptr)-frame.controllers.sync_ns);fflush(log_file);}
}
}
#if defined(GEARS_PLAYGROUND_HB_FINITE_BURST) && GEARS_PLAYGROUND_HB_FINITE_BURST
#include "weapon_hammerburst_burst_cancel.inl"
#include "weapon_hammerburst_burst_contract.inl"
#include "weapon_hammerburst_burst_runtime.inl"
#include "weapon_hammerburst_first_contract.inl"
#include "weapon_hammerburst_burst_event.inl"
#endif
