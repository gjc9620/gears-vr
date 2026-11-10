// Actual cosmetic dispatcher, real parked transaction; engine dispatch stubs.
// Scope's real Frame/curve policy has its separate production admission suite.
#include "weapon_hammerburst_restore_fixture.inl"
namespace hb_rebind_fixture {
double angle{.6};bool permit_enabled{true},permit_active{};
unsigned starts{},seeks{},visual_starts{},stows{};
anim::Acquire start_result{anim::Acquire::acquired};
bool seek_result{true},seek_pending_result{},visual_result{true},stow_result{true};
std::uint64_t animation_serial()noexcept{return serial?serial:old_item;}
namespace restoration_admission {
struct Scope {bool active{};explicit Scope(hand::Frame const&)noexcept{
    active=permit_enabled&&!permit_active;permit_active=active;}
    explicit operator bool()const noexcept{return active;}~Scope()noexcept{if(active)permit_active=false;}};
}
bool restoration_owned(hand::Frame const&)noexcept{return permit_active&&permit_enabled;}
}
namespace hammerburst_visual_clip {
anim::Acquire start(hand::Frame const& f,std::uint64_t token)noexcept{
    using namespace hb_rebind_fixture;C(permit_active);++starts;
    if(start_result!=anim::Acquire::rejected){anim::ownership.active=anim::ownership.visual_only=anim::ownership.stopped=true;
        anim::ownership.serial=unsigned(token);anim::ownership.inventory=f.identity;}
    return start_result;
}
}
namespace anim {
bool seek_latch_angle(hand::Frame const&,std::uint64_t token,double value)noexcept{
    using namespace hb_rebind_fixture;C(permit_active&&token==animation_serial()&&value==angle);++seeks;
    ownership.seek_pending=seek_pending_result;return seek_result;
}
}
namespace visual {
enum class Profile{hammerburst};bool begin_retryable{true};
bool begin(hand::Frame const& f,Profile)noexcept{
    using namespace hb_rebind_fixture;C(permit_active);++visual_starts;
    if(visual_result){active=true;saved.weapon=f.identity.weapon;}return visual_result;
}
bool stow()noexcept{using namespace hb_rebind_fixture;C(permit_active);++stows;return stow_result;}
}
namespace hb_rebind_fixture {
#include "weapon_hammerburst_restore_display.inl"
hand::Frame display_setup(unsigned mode=0){
    auto f=setup(mode==2);
    if(mode==1){Fixture missing;C(missing.transaction.stow_original(1));
        C(park_ledger::prepare(Stage::waiting_ammo,missing.transaction,1,0,false,parked_plan));
        transaction=parked_plan.transaction;observer.value=transaction.inventory();}
    C(restore_control::rebind(f));angle=.6;permit_enabled=true;permit_active=false;
    starts=seeks=visual_starts=stows=0;start_result=anim::Acquire::acquired;
    seek_result=visual_result=stow_result=true;seek_pending_result=false;visual::begin_retryable=true;return f;
}
}
