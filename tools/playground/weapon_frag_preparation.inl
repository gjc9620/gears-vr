// Normal-GT owner of the ORIGINAL targeting/spin pair. This is not another
// input publisher or equipment arbiter. Preparation never consumes ammunition.
namespace fragpreparation {
namespace hand=gears_weapon_hand;
using Counts=lancerreload_inventory::Counts;
std::atomic<bool> outstanding{},stop_requested{};
struct Journal {
    hand::Frame source{};hand::ExistingActionLifetime lifetime{};
    frag_original::Entry press{},release{},target{},prepare{},stop{},mode{};
    hand::ObjectIdentity projectile{};
    Name l2{};Counts before{};
    std::uint64_t binding{},token{},hold{};
    bool dispatched{},prepared{},key_owed{},target_owed{},spin_owed{};
};
Journal journal{};
struct State {bool key{},controller_target{},pawn_target{},spinning{};};
bool state(hand::Frame const& f,State& out) noexcept {
    State a{},b{};
    const auto read=[&](State& s) noexcept {
        return weapon_part_assets::boolean(f.controller,L"bPressingTargetingKey",s.key)&&
            weapon_part_assets::boolean(f.controller,L"bIsTargeting",s.controller_target)&&
            weapon_part_assets::boolean(f.pawn,L"bIsTargeting",s.pawn_target)&&
            weapon_part_assets::boolean(f.weapon,L"bSpinningGrenade",s.spinning);
    };
    if(!read(a)||!read(b)||a.key!=b.key||a.controller_target!=b.controller_target||
       a.pawn_target!=b.pawn_target||a.spinning!=b.spinning)return false;
    out=a;return true;
}
bool inventory(hand::Frame const& f,Counts& out) noexcept {
    Counts a{},b{};
    if(!weapon_part_assets::object(f.weapon)||
       !lancerreload_inventory::query(f.weapon.address,a)||
       !lancerreload_inventory::query(f.weapon.address,b)||!(a==b)||
       !weapon_part_assets::object(f.weapon))return false;
    out=a;return true;
}
bool name_current(Name const& value) noexcept {
    gripdamage_metadata::Table objects{},names{};gripdamage_metadata::NameText text{};
    return !value.number&&gripdamage_metadata::tables(base,objects,names)&&
        gripdamage_metadata::fresh_name(names,value.index,text)&&gripdamage_metadata::exact_text(text,L"L2");
}
bool owed() noexcept {return outstanding.load(std::memory_order_acquire);}
bool stop() noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    if(!owed())return true;
    // These are idempotent release/stop operations, never a throw retry.
    // Clearing targeting after ButtonRelease also handles stock toggle mode.
    // StopSpin follows both, so the stock targeting callback cannot restart it.
    const auto proven_up=[&]() noexcept {
        State actual{};unsigned char mode{},closing{};
        // StopGrenadeSpin clears PreparedNotify/effort/animations before its
        // original stopped effect mode8 and spinning=false. Require that
        // measured terminal mode too; neutral targeting bits alone do not
        // establish the mechanical/timer stop sequence.
        return weapon_part_assets::object(journal.source.weapon)&&
            lancerreload_state::byte_value(journal.source.weapon.address,L"CurrentFireMode",mode)&&mode==8&&
            state(journal.source,actual)&&!actual.key&&!actual.controller_target&&!actual.pawn_target&&!actual.spinning&&
            lancerreload_state::byte_value(journal.source.weapon.address,L"CurrentFireMode",closing)&&closing==mode&&
            weapon_part_assets::object(journal.source.weapon);
    };
    if(!proven_up()){
        if(!name_current(journal.l2))return false;
        if(journal.key_owed)(void)frag_original::release_existing(journal.source,journal.lifetime,journal.release,journal.l2);
        if(journal.target_owed)(void)frag_original::release_existing(journal.source,journal.lifetime,journal.target);
        if(journal.spin_owed)(void)frag_original::release_existing(journal.source,journal.lifetime,journal.stop);
        // ButtonRelease/target callbacks can already clear all four original
        // states and retire their pose association. A later redundant stop
        // then loses that old Frame witness. Settle only from the double-read
        // typed states of the exact original actors, never from call success
        // alone or a fabricated replacement lifetime. Any bit still owned
        // retains this cleanup debt and its existing guards.
        if(!proven_up())return false;
    }
    journal.key_owed=journal.target_owed=journal.spin_owed=journal.prepared=false;
    outstanding.store(false,std::memory_order_release);return true;
}
struct Result {
    bool token_taken{},dispatched{},prepared{},cleanup_complete{};
    std::int64_t executed_ns{};std::uint64_t preparation_serial{};
    Counts before{},after{};
};
Result begin(hand::Frame const& f,gears_weapon::BoloGrenadeInteraction& owner) noexcept {
    Result result{};
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_R2_TRIAL) && defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
    if(!weaponhand::current_gt(nullptr)||owed()||stop_requested.load(std::memory_order_acquire)||
       !nativeequipment::current(nullptr,f)||vrfire::input.owns_press()||vrfire::aim_owns())return result;
    const auto request=owner.pending_request();
    if(request.action!=gears_weapon::BoloAction::prepare_original||!request.token||
       request.identity.world!=f.world.address||request.identity.controller!=f.controller.address||
       request.identity.pawn!=f.pawn.address||request.identity.weapon!=f.weapon.address||
       request.identity.world_generation!=f.key.world_generation||request.identity.xr_session!=f.stereo.session||
       request.identity.space_epoch!=f.stereo.space_epoch||request.identity.input_epoch!=f.controllers.input_epoch||
       request.identity.profile_generation!=f.controllers.profile_generation||request.reference_generation!=f.reference_generation||
       !request.executable_at(weaponhand::clock_ns(nullptr),request.identity,owner.binding_token(),
            request.motion_epoch,request.reference_generation)||
       !vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address))return result;
    Journal next{};next.source=f;next.binding=owner.binding_token();next.token=request.token;
    next.hold=vrequip::model.hold_generation();next.l2=vrfire::l2;
    lancermain_asset::Evidence asset{};State prior{};bool r2{};
    if(!name_current(next.l2)||
       lancermain_asset::read(f,asset,lancermain_asset::AssetScope::frag_hold)!=lancermain_asset::Status::ready||
       !nativeequipment::attached_frame(f,asset.component.address)||!state(f,prior)||
       prior.key||prior.controller_target||prior.pawn_target||prior.spinning||
       !weapon_part_assets::boolean(f.controller,L"bHoldingButton_R2",r2)||r2||
       !inventory(f,next.before)||next.before.used>=next.before.capacity||
       !frag_original::bind(f.controller.address,frag_original::Method::press,next.press)||
       !frag_original::bind(f.controller.address,frag_original::Method::release,next.release)||
       !frag_original::bind(f.controller.address,frag_original::Method::target,next.target)||
       !frag_original::bind(f.weapon.address,frag_original::Method::prepare,next.prepare)||
       !frag_original::bind(f.weapon.address,frag_original::Method::stop,next.stop)||
       !frag_original::bind(f.weapon.address,frag_original::Method::mode,next.mode)||
       !frag_original::projectile_class(f,next.projectile)||
       !weaponhand::client.capture_existing_action_lifetime_on_gt(f.key,next.lifetime)||
       !nativeequipment::current(nullptr,f)||vrequip::model.hold_generation()!=next.hold)return result;
    const auto taken=owner.take_request();result.token_taken=bool(taken);
    if(!taken||taken->token!=request.token||taken->binding!=next.binding)return result;
    journal=next;result.before=next.before;result.executed_ns=weaponhand::clock_ns(nullptr);
    if(!request.executable_at(result.executed_ns,request.identity,next.binding,request.motion_epoch,request.reference_generation))return result;
    // Establish release debt BEFORE any dispatcher can enter original code.
    outstanding.store(true,std::memory_order_release);
    journal.key_owed=journal.target_owed=journal.spin_owed=true;
    bool invoked{};bool okay=frag_original::button(f,journal.press,journal.l2,&invoked);
    journal.dispatched=invoked;
    if(okay){okay=frag_original::targeting(f,journal.target,true,&invoked);journal.dispatched|=invoked;}
    State actual{};
    if(okay)okay=state(f,actual)&&actual.key&&actual.controller_target&&actual.pawn_target;
    // SetTargetingMode normally starts the spin via TargetingModeChanged.
    // Only call StartGrenadeSpin if the actual state still needs it.
    if(okay&&!actual.spinning){okay=frag_original::call(f,journal.prepare,nullptr,0,nullptr,&invoked);journal.dispatched|=invoked;}
    if(okay)okay=state(f,actual)&&actual.key&&actual.controller_target&&actual.pawn_target&&actual.spinning&&
        inventory(f,result.after)&&result.after==journal.before&&nativeequipment::current(nullptr,f);
    journal.prepared=okay;result.prepared=okay;result.dispatched=journal.dispatched;
    result.preparation_serial=okay?journal.token:0;
    if(!okay){result.cleanup_complete=stop();(void)inventory(f,result.after);}
    return result;
#else
    (void)f;(void)owner;return result;
#endif
}
bool ready(hand::Frame const& f,std::uint64_t serial) noexcept {
    State current{};
    return weaponhand::current_gt(nullptr)&&owed()&&journal.prepared&&serial==journal.token&&
        !stop_requested.load(std::memory_order_acquire)&&
        hand::same(f.weapon,journal.source.weapon)&&hand::same(f.pawn,journal.source.pawn)&&
        hand::same(f.controller,journal.source.controller)&&hand::same(f.world,journal.source.world)&&
        journal.hold==vrequip::model.hold_generation()&&
        weaponhand::client.existing_action_lifetime_on_gt(journal.lifetime)&&
        nativeequipment::current(nullptr,f)&&state(f,current)&&current.key&&
        current.controller_target&&current.pawn_target&&current.spinning;
}
bool prepare_launch(hand::Frame const& f,std::uint64_t serial) noexcept {
    if(!ready(f,serial))return false;
    hand::ObjectIdentity actual{};
    return frag_original::projectile_class(f,actual)&&hand::same(actual,journal.projectile)&&
        frag_original::throw_mode(f,journal.mode)&&nativeequipment::current(nullptr,f);
}
void service() noexcept {
    if(weaponhand::current_gt(nullptr)&&owed()&&stop_requested.load(std::memory_order_acquire))stop();
}
void reset_after_drain() noexcept {
    if(owed())return;
    journal={};stop_requested.store(false,std::memory_order_release);
}
} // namespace fragpreparation
