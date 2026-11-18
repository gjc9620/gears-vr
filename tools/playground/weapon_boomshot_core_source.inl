// Actual normal-GT mechanism input. This reads original state and the exact
// Native Scene companion; it neither manufactures a tracking sample nor grants
// projectile/reload/inventory authority.
namespace boomshot_core_source {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
namespace core=gears_weapon;namespace rigabi=gears_rig_tracking;
struct Snapshot {
    inv::Identity context{};inv::Counts counts{};hand::GripGameState game{};
    core::BoomshotOwnerEvents::Input input{};
    rigabi::Frame rig{};
    std::int64_t acquired_begin_ns{},acquired_end_ns{};
};
// The normal-GT consumer already revalidated this exact Native frame/game.
// Reuse that observation within this one call; no cross-frame state or input.
hand::Frame observed_frame{};hand::GripGameState observed_game_state{};bool observed_game_known{};
struct GameScope {
    GameScope(hand::Frame const& f,hand::GripGameState const& game,bool known)noexcept{observed_frame=f;observed_game_state=game;observed_game_known=known;}
    ~GameScope(){observed_game_known=false;}
};
bool observed_game(hand::Frame const& f,hand::GripGameState& game)noexcept{
    if(observed_game_known&&hand::same_frame(observed_frame,f)&&hand::frame_active(f,weaponhand::clock_ns(nullptr))){game=observed_game_state;return true;}
    return weaponhand::observe_game(f,game,false,false);
}
bool allowed(hand::GripGameState const& game) noexcept {
    return game.known&&game.main_hand_held&&!game.busy&&!game.animation_owns_hands&&game.kind==hand::WeaponKind::firearm;
}
bool read(hand::Frame const& f,Snapshot& out,std::uint64_t reload_serial=0) noexcept {
    out={};const auto began=weaponhand::clock_ns(nullptr);
    if(!weaponhand::current_gt(nullptr)||f.left_tracked!=1||f.recentered||f.heading_deferred||
       !nativeequipment::current(nullptr,f)||!boomshot_carrier::selected(f))return false;
    const auto context=inv::identity(f);if(!context.valid()||(reload_serial&&!lancerreload_state::owns(f,reload_serial)))return false;
    Snapshot value{};hand::GripGameState closing_game{};inv::Counts closing_counts{};rigabi::Frame tracking{};
    const auto source=unsigned(f.stereo.source);bool reloading{},closing_reload{};float game_time{},closing_time{};
    // No candidate/support query or client callback reentry in this read.
    if(!observed_game(f,value.game)||!allowed(value.game)||
       !inv::read(f,value.counts)||value.counts.capacity!=1||value.counts.used<0||value.counts.used>1||
       value.counts.reserve<0||value.counts.reserve>1000000||
       !lancerreload_state::bool_call(f.weapon.address,L"IsReloading",reloading)||reloading!=(reload_serial!=0)||
       handrig::read(tracking,0,source).reply!=rigabi::Reply::accepted||
       gears_equipment_rig::match(f,tracking,weaponhand::clock_ns(nullptr),source)!=gears_equipment_rig::Status::matched||
       (tracking.world_pose_mask&(rigabi::left_grip_pose|rigabi::right_grip_pose))!=(rigabi::left_grip_pose|rigabi::right_grip_pose)||
       !lancerreload_state::time_field(f.world.address,L"TimeSeconds",game_time)||
       !std::isfinite(game_time)||game_time<0||game_time>1000000000.f)return false;
    if(!inv::read(f,closing_counts)||!(closing_counts==value.counts)||
       !lancerreload_state::bool_call(f.weapon.address,L"IsReloading",closing_reload)||closing_reload!=(reload_serial!=0)||
       !lancerreload_state::time_field(f.world.address,L"TimeSeconds",closing_time)||closing_time!=game_time||
       !observed_game(f,closing_game)||!allowed(closing_game)||
       !(context==inv::identity(f))||!nativeequipment::current(nullptr,f)||!boomshot_carrier::selected(f)||
       (reload_serial&&!lancerreload_state::owns(f,reload_serial)))return false;
    const auto ended=weaponhand::clock_ns(nullptr);
    if(began<=0||ended<began||ended-began>100000000||
       gears_equipment_rig::match(f,tracking,ended,source)!=gears_equipment_rig::Status::matched)return false;
    value.context=context;value.game=closing_game;value.rig=tracking;value.acquired_begin_ns=began;value.acquired_end_ns=ended;
    value.input={f.controllers.sequence,f.controllers.input_epoch,tracking.native_reference.epoch,
        f.controllers.profile_generation,f.controllers.sync_ns,std::int64_t(double(game_time)*1000000.)};
    out=value;return true;
}
enum class Result {source_unavailable,inventory_changed,waiting_input,rejected_input,ready};
Result input(hand::Frame const& f,core::BoomshotInteraction const& owner,core::BoomshotOwnerEvents& events,
    core::BoomshotOwnerEvents::InputEvent& event,core::BoomshotFrame& frame,std::uint64_t reload_serial=0,Snapshot* sampled_source=nullptr,Snapshot const* same_frame_source=nullptr) noexcept {
    event={};frame={};Snapshot source{};
    if(sampled_source)*sampled_source={};
    if(same_frame_source){
        source=*same_frame_source;
        const auto now=weaponhand::clock_ns(nullptr);
        if(!(source.context==inv::identity(f))||!nativeequipment::current(nullptr,f)||
           source.input.sequence!=f.controllers.sequence||source.input.sync_ns!=f.controllers.sync_ns||
           now<source.acquired_end_ns||now-source.acquired_end_ns>100000000||
           gears_equipment_rig::match(f,source.rig,now,unsigned(f.stereo.source))!=gears_equipment_rig::Status::matched)return Result::source_unavailable;
    }else if(!read(f,source,reload_serial))return Result::source_unavailable;
    const auto inventory=owner.inventory();
    if(!inventory.identity.valid()||inventory.identity.world!=f.world.address||inventory.identity.pawn!=f.pawn.address||
       inventory.identity.weapon!=f.weapon.address||!inventory.revision||
       inventory.loaded!=unsigned(source.counts.capacity-source.counts.used)||inventory.reserve!=unsigned(source.counts.reserve)||
       inventory.infinite_reserve!=source.counts.infinite)return Result::inventory_changed;
    core::BoomshotOwnerEvents::InputEvent candidate{};
    const auto accepted=events.input(source.input,source.acquired_end_ns,candidate);
    if(accepted==core::BoomshotOwnerEvents::Result::awaiting_new_input||accepted==core::BoomshotOwnerEvents::Result::replayed)return Result::waiting_input;
    if(accepted!=core::BoomshotOwnerEvents::Result::accepted||candidate.stamp().binding!=owner.binding_token()||
       !(candidate.stamp().identity==inventory.identity))return Result::rejected_input;
    // Fire is deliberately false: inspection state observation certifies no
    // original CanFire/projectile permission or new reload acceptance. Live hand ownership comes from
    // the strict current game read, not the inspection animation's local flag.
    frame={candidate.stamp(),inventory,true,true,source.game.main_hand_held,true,false,source.game.animation_owns_hands};
    event=candidate;if(sampled_source)*sampled_source=source;return Result::ready;
}
}
