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
int main(){
    using namespace hb_rebind_fixture;using D=restore_control::DisplayResult;checks=cases=0;
    for(unsigned mode=0;mode<3;++mode){
        auto f=display_setup(mode);const auto before=transaction.inventory();const auto original_loaded=opening.loaded;
        C(restore_control::display(f)==D::advanced);C(starts==1&&!seeks&&!permit_active);
        C(restore_control::display(f)==D::advanced);C(starts==1&&seeks==1&&!permit_active);
        C(restore_control::display(f)==D::ready);C(restore_display_ready&&restore_display_step==3);
        C(visual_starts==(mode==1?1u:0u)&&stows==visual_starts);
        C(inv::counts_equal(before,transaction.inventory())&&opening.loaded==original_loaded);
        C(!held&&!released&&!lancerreload_state::lease.acquired&&!permit_active);
        C(restore_control::display(f)==D::ready&&starts==1&&seeks==1&&stows==visual_starts);++cases;
    }
    for(unsigned fault=0;fault<7;++fault){
        auto f=display_setup();
        if(fault==0)fresh_value=false;
        if(fault==1)permit_enabled=false;
        if(fault==2)lancerreload_state::lease.acquired=true;
        if(fault==3){anim::ownership.active=true;anim::ownership.inventory=opening.identity;}
        if(fault==4){visual::active=true;visual::saved.weapon=8;}
        if(fault==5)final_geometry=false;
        if(fault==6)observer.available=false;
        C(restore_control::display(f)==D::waiting);C(!starts&&!seeks&&!stows&&!permit_active&&!restore_display_ready);++cases;
    }
    {
        auto f=display_setup();++observer.value.loaded;C(restore_control::display(f)==D::recovery_required);
        C(!starts&&!seeks&&!stows&&!restore_display_ready);++cases;
    }
    for(auto result:{anim::Acquire::rejected,anim::Acquire::recovery_required}){
        auto f=display_setup();start_result=result;
        C(restore_control::display(f)==(result==anim::Acquire::rejected?D::waiting:D::recovery_required));
        C(starts==1&&!seeks&&!permit_active&&restore_display_step==0&&!restore_display_ready);++cases;
    }
    for(bool pending:{false,true}){
        auto f=display_setup();C(restore_control::display(f)==D::advanced);seek_result=false;seek_pending_result=pending;
        C(restore_control::display(f)==(pending?D::recovery_required:D::waiting));
        C(starts==1&&seeks==1&&restore_display_step==1&&!permit_active&&!restore_display_ready);++cases;
    }
    for(unsigned fault=0;fault<3;++fault){
        auto f=display_setup(1);C(restore_control::display(f)==D::advanced);C(restore_control::display(f)==D::advanced);
        if(fault<2){visual_result=false;visual::begin_retryable=fault==0;}else stow_result=false;
        C(restore_control::display(f)==(fault==0?D::waiting:D::recovery_required));
        C(starts==1&&seeks==1&&visual_starts==1&&!permit_active&&!restore_display_ready);++cases;
    }
    std::printf("HammerburstRestoreDisplayTest: %u cases, %u checks; saved-angle/absent-magazine dispatch, no native begin or ammo mutation\n",cases,checks);return 0;
}
