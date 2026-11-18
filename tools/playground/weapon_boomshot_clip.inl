// Original accepted Reloading only. All StopAnim/SetPosition operations and
// their uncertain outcomes belong to the existing shared animation journal.
namespace boomshot_clip {
namespace hand=gears_weapon_hand;namespace anim=hammerburst_animation;
namespace inv=lancerreload_inventory;
enum class Result {rejected,pending,ready,recovery_required};
std::uint64_t serial{},attempt_frame{};inv::Identity identity{};
boomshot_animation::Curve curve{};bool pending{};
bool owned() noexcept {
    return serial&&!pending&&anim::ownership.active&&anim::ownership.serial==serial&&
        anim::ownership.inventory==identity;
}
Result service(hand::Frame const& f) noexcept {
    if(!serial||!(identity==inv::identity(f)))return Result::rejected;
    if(!nativeequipment::current(nullptr,f)||!lancerreload_state::owns(f,serial)||
       !lancerreload_state::item_owned(f))return Result::pending;
    if(!pending)return owned()&&anim::ownership.stopped?Result::ready:Result::recovery_required;
    if(anim::ownership.active)return Result::recovery_required;
    if(attempt_frame==f.key.frame)return Result::pending;
    attempt_frame=f.key.frame;
    anim::Observation before{};anim::Controls controls{};boomshot_animation::Curve candidate{};
    if(!anim::observe_kind(f,before,lancermain_asset::HeldAsset::boomshot)||
       !boomshot_animation::read_curve(f,before,candidate)||
       !anim::prepare_controls(f,before,controls,true))return Result::pending;
    const auto result=anim::acquire_prepared(f,serial,before,controls);
    if(result==anim::Acquire::rejected)return Result::pending;
    // acquire_prepared journals before StopAnim. Retain our exact identity
    // even if that dispatch needs reconciliation; never borrow another owner.
    pending=false;curve=candidate;
    return result==anim::Acquire::acquired?Result::ready:Result::recovery_required;
}
Result start(hand::Frame const& f,std::uint64_t reload_serial) noexcept {
    if(serial||!reload_serial||anim::ownership.active||!nativeequipment::current(nullptr,f)||
       !lancerreload_state::owns(f,reload_serial)||!lancerreload_state::item_owned(f))return Result::rejected;
    serial=reload_serial;identity=inv::identity(f);attempt_frame=0;pending=true;curve={};
    return service(f);
}
bool seek(hand::Frame const& f,double progress,gears_weapon::boomshot_barrel::Direction direction) noexcept {
    if(!owned()||!anim::ownership.stopped||!(identity==inv::identity(f)))return false;
    boomshot_animation::Curve current{};gears_weapon::boomshot_tracks::Path path{};float time{};
    if(!boomshot_animation::read_curve(f,anim::ownership.last,current)||
       !hand::same(current.sequence,curve.sequence)||current.content!=curve.content||
       !path.bind(current.samples)||!path.time(progress,direction,time))return false;
    return anim::seek(f,serial,time);
}
bool hand_back_after_native_exit() noexcept {
    if(!serial)return true;
    // A pending adoption still owes the original reload-state exit. It cannot
    // disappear just because no animation node was captured yet.
    if(lancerreload_state::lease.acquired&&lancerreload_state::lease.serial==serial&&
       lancerreload_state::lease.identity==identity)return false;
    if(!pending&&(!owned()||!anim::hand_back_after_native_exit()))return false;
    serial=attempt_frame=0;identity={};curve={};pending=false;return true;
}
}
