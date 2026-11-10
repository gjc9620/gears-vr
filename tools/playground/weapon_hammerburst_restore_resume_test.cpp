// Production continuation controller + real transaction; original engine
// calls and grants are seam stubs, not live game acceptance.
#include "weapon_hammerburst_restore_display_fixture.inl"
#include <cmath>
#include <cstdint>
namespace hb_rebind_fixture {
std::uint64_t restore_resume_intent{};bool native_exited{true},ui_pending{};
unsigned native_begins{},animation_backs{},visual_backs{},native_acquires{};
bool handback_ok{true},visual_back_ok{true},begin_dispatched{true},wrong_serial{};
bool restored_time_ok{true};float game_clock{11};
bool granted(hand::Frame const& f,hand::Decision const& d,hand::LeaseKind kind,bool fresh=false)noexcept{
    return f.left_tracked&&f.left_grip_known&&f.left_grip_down&&d.granted&&d.lease.id==f.press_token&&
        d.lease.kind==kind&&(!fresh||d.fresh);
}
bool restore_animation()noexcept{++animation_backs;if(handback_ok)anim::ownership={};return handback_ok;}
bool restored_game_time(hand::Frame const&,float& result)noexcept{result=game_clock;return restored_time_ok;}
}
namespace visual {bool restore()noexcept{using namespace hb_rebind_fixture;++visual_backs;if(visual_back_ok)active=false;return visual_back_ok;}}
namespace lancerreload_state {
enum class BeginResult{rejected,acquired,recovery_required};BeginResult begin_result{BeginResult::acquired};
BeginResult begin(hand::Frame const& f,std::uint64_t& token,Lease const* saved,bool* dispatched)noexcept{
    using namespace hb_rebind_fixture;++native_begins;C(saved&&saved->serial==serial&&saved->identity==f.identity);
    *dispatched=begin_dispatched;token=0;
    if(begin_result==BeginResult::acquired){lease=*saved;lease.acquired=true;token=wrong_serial?serial+1:serial;}
    return begin_result;
}
bool closure_time(hand::Frame const&,std::uint64_t,std::int64_t& result)noexcept{
    result=lease.accepted_begin_ms+1000;return lease.acquired;
}
}
namespace anim {
Acquire acquire(hand::Frame const& f,std::uint64_t token)noexcept{
    using namespace hb_rebind_fixture;++native_acquires;C(permit_active&&lancerreload_state::lease.acquired);
    ownership={true,false,true,false,unsigned(token),f.identity};return Acquire::acquired;
}
}
namespace hb_rebind_fixture {
#include "weapon_hammerburst_restore_resume.inl"
#include "weapon_hammerburst_restore_clock.inl"
hand::Frame resume_setup(unsigned mode=0){
    serial=1;auto f=display_setup(mode);
    C(restore_control::display(f)==restore_control::DisplayResult::advanced);
    C(restore_control::display(f)==restore_control::DisplayResult::advanced);
    C(restore_control::display(f)==restore_control::DisplayResult::ready);
    restore_resume_intent=0;restore_resume_step=0;resume_grant_ready=restore_native_uncertain=false;native_exited=true;ui_pending=false;
    native_begins=animation_backs=visual_backs=native_acquires=0;
    handback_ok=visual_back_ok=begin_dispatched=restored_time_ok=true;wrong_serial=false;
    lancerreload_state::begin_result=lancerreload_state::BeginResult::acquired;
    game_clock=11;return f;
}
hand::Decision down(hand::Frame& f){f.left_grip_down=true;f.press_token=2;hand::Decision d;
    d.lease.id=2;d.lease.kind=restore_control::resume_part();return d;}
}
int main(){
    using namespace hb_rebind_fixture;using R=restore_control::ResumeResult;checks=cases=0;
    for(unsigned mode=0;mode<3;++mode){
        auto f=resume_setup(mode);const auto counts=transaction.inventory();const auto original_loaded=opening.loaded;
        C(restore_control::resume(f,{})==R::waiting&&released);
        auto decision=down(f);const auto first=restore_control::resume(f,decision);
        C(first==(mode==2?R::ready:R::advanced));
        if(mode!=2){
            C(stage==Stage::restoring&&native_begins==1&&lancerreload_state::lease.acquired);
            C(lancerreload_state::lease.accepted_begin_ms==123&&lancerreload_state::lease.reward_start==10);
            decision.fresh=false;
            C(restore_control::resume(f,decision)==R::advanced);
            C(restore_control::resume(f,decision)==R::advanced);
            C(restore_control::resume(f,decision)==R::ready);
            C(native_begins==1&&native_acquires==1&&!native_exited&&ui_pending);
        }else C(!native_begins&&!native_acquires&&native_exited&&!lancerreload_state::lease.acquired);
        C(stage==(mode==0?Stage::opening:mode==1?Stage::waiting_ammo:Stage::closing));
        C(inv::counts_equal(counts,transaction.inventory())&&opening.loaded==original_loaded);
        C(restore_resume_intent==2&&held.id==2&&resume_grant_ready&&!permit_active);
        C(restore_control::resume(f,decision)==R::waiting);++cases;
    }
    for(unsigned fault=0;fault<9;++fault){
        auto f=resume_setup();if(fault!=0)C(restore_control::resume(f,{})==R::waiting);
        auto d=down(f);
        if(fault==1)f.left_grip_known=false;
        if(fault==2)f.left_tracked=false;
        if(fault==3)d.fresh=false;
        if(fault==4)d.lease.kind=hand::LeaseKind::ammo;
        if(fault==5){f.press_token=1;d.lease.id=1;}
        if(fault==6)restore_resume_intent=2;
        if(fault==7)fresh_value=false;
        if(fault==8)++f.identity.hold;
        C(restore_control::resume(f,d)==R::waiting);C(!native_begins&&!animation_backs&&stage==Stage::restoring);++cases;
    }
    {
        auto f=resume_setup();C(restore_control::resume(f,{})==R::waiting);auto d=down(f);
        lancerreload_state::begin_result=lancerreload_state::BeginResult::rejected;begin_dispatched=false;
        C(restore_control::resume(f,d)==R::waiting);C(native_begins==1&&!restore_display_ready&&!released&&restore_resume_intent==2);
        C(restore_control::resume(f,d)==R::waiting&&native_begins==1);++cases;
    }
    for(unsigned fault=0;fault<4;++fault){
        auto f=resume_setup();C(restore_control::resume(f,{})==R::waiting);auto d=down(f);
        if(fault==0)handback_ok=false;
        if(fault==1)visual_back_ok=false;
        if(fault==2)lancerreload_state::begin_result=lancerreload_state::BeginResult::recovery_required;
        if(fault==3)wrong_serial=true;
        C(restore_control::resume(f,d)==R::recovery_required);C(stage==Stage::restoring&&!released&&restore_resume_intent==2);
        C(restore_native_uncertain==(fault==2));++cases;
    }
    {
        auto f=resume_setup();C(restore_control::resume(f,{})==R::waiting&&released);
        fresh_value=false;C(restore_control::resume(f,{})==R::waiting&&!released);
        fresh_value=true;auto d=down(f);C(restore_control::resume(f,d)==R::waiting&&!native_begins);++cases;
    }
    {
        auto f=resume_setup(2);std::int64_t tick{},begin{},end{};
        C(closure_clock(f,tick,begin,end)&&tick==1123&&begin==123&&end==4000);
        game_clock=10;C(closure_clock(f,tick,begin,end)&&tick==123); // paused GAME time, no wall-clock credit.
        game_clock=9;C(!closure_clock(f,tick,begin,end));
        game_clock=11;fresh_value=false;C(!closure_clock(f,tick,begin,end));fresh_value=true;
        restored_time_ok=false;C(!closure_clock(f,tick,begin,end));restored_time_ok=true;
        ++parked_native_lease.serial;C(!closure_clock(f,tick,begin,end));++cases;
    }
    std::printf("HammerburstRestoreResumeTest: %u cases, %u checks; real ledger, fresh edge, once-only native reacquire and retained game clock\n",cases,checks);return 0;
}
