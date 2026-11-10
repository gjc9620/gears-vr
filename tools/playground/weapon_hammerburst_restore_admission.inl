// Included inside hammerburst_runtime. A scoped cosmetic restoration permit
// for an already rebound parked ledger. It never grants a hand/item lease,
// inventory request, firing source, original reload start or reward.
namespace restoration_admission {
struct Entry {
    hand::FrameKey key{};inv::Identity owner{};
    std::uint64_t token{},generation{};double saved_angle{};bool active{};
};
Entry entry;
std::uint64_t next_generation{};
void cancel() noexcept {entry.active=false;}
bool ready(hand::Frame const& source) noexcept {
    const auto ledger_stage=transaction.stage();
    if(ledger_stage!=gears_weapon::ReloadTxnStage::open&&ledger_stage!=gears_weapon::ReloadTxnStage::interrupted&&
       ledger_stage!=gears_weapon::ReloadTxnStage::closing)return false;
    if(!park_control::enabled()||stage!=Stage::restoring||!parked_plan.valid||
       transaction.pending()||transaction.reserved()||!transaction.inventory().valid()||
       !(transaction.inventory().identity==opening.identity)||
       !(parked_plan.transaction.inventory().identity==opening.identity)||
       !inv::counts_equal(transaction.inventory(),parked_plan.transaction.inventory())||
       !gears_vr::controller_pose_usable(source.controllers.hands[1].grip,source.controllers.hands[1].grip_flags,source.controllers.hands[1].grip_active)||
       !(inv::identity(source)==opening.identity)||!geometry_current(source))return false;
    const auto observed_ns=weaponhand::clock_ns(nullptr);
    return observed_ns>=source.now_ns&&observed_ns-source.now_ns<=100000000&&
        source.controllers.sync_ns>0&&observed_ns>=source.controllers.sync_ns&&
        observed_ns-source.controllers.sync_ns<=100000000&&
        hand::frame_active(source,observed_ns)&&eligible(source)&&nativeequipment::current(nullptr,source);
}
class Scope {
    std::uint64_t generation_{};
public:
    explicit Scope(hand::Frame const& source) noexcept {
        if(entry.active||next_generation==UINT64_MAX||!ready(source)||
           !animation_serial()||!std::isfinite(angle)||angle<0||angle>3.14)return;
        generation_=++next_generation;
        entry={source.key,opening.identity,animation_serial(),generation_,angle,true};
    }
    Scope(Scope const&)=delete;Scope& operator=(Scope const&)=delete;
    ~Scope() noexcept {if(generation_&&entry.generation==generation_)entry.active=false;}
    explicit operator bool()const noexcept{return generation_&&entry.active&&entry.generation==generation_;}
};
}
bool restoration_owned(hand::Frame const& source) noexcept {
    const auto& permit=restoration_admission::entry;
    if(!permit.active||!hand::same(permit.key,source.key))return false;
    if(!(permit.owner==opening.identity)||permit.token!=animation_serial()||permit.saved_angle!=angle||
       !restoration_admission::ready(source)){
        restoration_admission::cancel();return false;
    }
    return true;
}
bool restoration_seek_owned(hand::Frame const& source,std::uint64_t requested_serial,float requested_time) noexcept {
    if(!restoration_owned(source)||requested_serial!=restoration_admission::entry.token||
       !anim::ownership.active||anim::ownership.serial!=requested_serial||
       !(anim::ownership.inventory==opening.identity)||!std::isfinite(requested_time))return false;
    float exact_time{};
    return anim::ownership.latch.curve.time_for_angle(restoration_admission::entry.saved_angle,exact_time,anim::ownership.latch.sampling)&&
        std::abs(exact_time-requested_time)<=1e-4f;
}
