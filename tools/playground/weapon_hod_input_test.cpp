// Production HOD input/effect composition with synthetic original calls.
// Tests use the real shared R2 edge ledger and HOD owner, not copied logic.
#define GEARS_PLAYGROUND_NATIVE_LANCER_R2_TRIAL 1
#include "../vr-common/weapon_hand_participant.h"
#include "../vr-common/original_r2_fire.h"
#include "../vr-common/weapon_hod_owner.h"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstdint>
using uptr=std::uintptr_t;namespace hand=gears_weapon_hand;
namespace gears_weapon_hand {struct ExistingActionLifetime {};}
struct Name {unsigned index{},number{};};
FILE* log_file{};
std::int64_t clock_now=1000000100;
bool gt=true,current=true,fields=true,lifetime=true,dispatch_returns=true,release_clears=true,fade_audio=false;
uptr selected_weapon=5;unsigned presses{},releases{},ends{},aim_presses{},aim_releases{};bool aim_release_clears=true;
hand::ObjectIdentity object(uptr at){return {at,0,1,unsigned(at),unsigned(at),0,1};}
namespace hod_state {
struct Observed {
    hand::ObjectIdentity manager=object(90);std::uint64_t fingerprint{1};
    bool manager_enabled=true,input_down{},laser{},valid_emitter{},loop_playing{},beam_firing{},pending_beam{},target_live{},beam_audio{};
    bool aim_key{},toggle_targeting{},controller_aim{},pawn_aim{};
    bool aim_stopped() const {return !aim_key&&!controller_aim&&!pawn_aim;}
    unsigned owned_beams{};
    bool stopped() const {return !input_down&&!laser&&!valid_emitter&&!loop_playing&&!beam_firing&&!pending_beam&&!target_live&&!beam_audio;}
} actual;
bool read(hand::Frame const&,Observed& out){if(!fields)return false;out=actual;return true;}
}
namespace weaponhand {
std::uint32_t current_gt(void*){return gt?1u:0u;}
std::int64_t clock_ns(void*){return clock_now;}
struct Held {std::uint64_t generation=10;}held_binding;
struct Client {
    bool capture_existing_action_lifetime_on_gt(hand::FrameKey const&,hand::ExistingActionLifetime&){return current;}
}client;
}
namespace nativeequipment {bool current(void*,hand::Frame const&){return ::current;}}
namespace unified_weapon_fire {
struct MuzzleSample {hand::Frame source{};std::uint64_t hold_generation{10};};
bool live_sample(MuzzleSample const&,std::int64_t now){return current&&now-clock_now<=20000000;}
}
namespace hod_trace {
bool live(unified_weapon_fire::MuzzleSample const&){return current;}
struct UseCounters {std::uint64_t query_returns{},query_ray_commits{},fx_returns{},fx_origin_bodies{},fx_aim_bodies{},fx_paired_bodies{};};
void begin_use_observation(unified_weapon_fire::MuzzleSample const&,std::uint64_t){}
void end_use_observation(std::uint64_t){}
bool use_counters(std::uint64_t,UseCounters& out){out={};return false;}
}
namespace hod_original {
enum class Method:unsigned {press,release,end_fire};enum class Button:unsigned {fire,aim};
struct Entry {Method method{};hand::ObjectIdentity receiver{};};
bool bind(hand::ObjectIdentity const& id,Method method,Entry& out){out={method,id};return fields;}
bool execute(hand::Frame const& f,Entry const& entry,Name,bool& sent,hand::ExistingActionLifetime const* cleanup=nullptr,Button button=Button::fire){
    sent=false;if(!gt||f.weapon.address!=selected_weapon||(cleanup?!lifetime:!current))return false;
    sent=true;auto& s=hod_state::actual;
    if(button==Button::aim){
        if(entry.method==Method::press){++aim_presses;s.aim_key=s.controller_aim=s.pawn_aim=true;}
        else{++aim_releases;if(aim_release_clears)s.aim_key=s.controller_aim=s.pawn_aim=false;}
        return dispatch_returns;
    }
    if(entry.method==Method::press){++presses;s.input_down=s.laser=s.pending_beam=s.target_live=true;}
    else if(entry.method==Method::release){++releases;s.input_down=false;
        if(release_clears){s.laser=s.pending_beam=s.target_live=s.beam_firing=s.valid_emitter=false;}
        s.loop_playing=fade_audio;
    }else{++ends;s.laser=s.pending_beam=s.target_live=s.beam_firing=s.valid_emitter=false;}
    return dispatch_returns;
}
}
namespace vrfire {Name r2{2,0},l2{3,0};bool name_current(Name const& n,wchar_t first) noexcept {return fields&&n.index==(first==L'L'?3u:2u)&&!n.number;}}
#include "weapon_hod_input.inl"
namespace api=hod_input;
struct Fixture {
    unified_weapon_fire::MuzzleSample sample{};
    Fixture(){
        api::input.reset();api::debt={};api::journal={};api::next_serial=api::read_serial=0;
        api::retained=false;api::cancelling=api::faulted=false;
        presses=releases=ends=0;gt=current=fields=lifetime=dispatch_returns=release_clears=true;
        fade_audio=false;selected_weapon=5;hod_state::actual={};
        auto& f=sample.source;f.world=object(1);f.pawn=object(3);f.controller=object(4);f.weapon=object(5);
        f.key.world_generation=2;
        f.controllers.session=f.stereo.session=7;f.controllers.sequence=f.stereo.sequence=1;
        f.controllers.space_epoch=f.stereo.space_epoch=3;f.controllers.input_epoch=1;
        f.controllers.predicted_time=f.stereo.predicted_time=1000;f.controllers.focused=f.stereo.focused=1;
        f.controllers.sync_ns=999999900;f.controllers.received_ns=1000000000;
        auto& h=f.controllers.hands[1];h.grip_active=1;h.grip_flags=15;h.grip.orientation.w=1;h.trigger_active=1;
        tick();assert(!api::owed()&&presses==0);
    }
    void tick(){api::advance(sample,hod_state::actual);}
    void next(){++sample.source.controllers.sequence;++sample.source.stereo.sequence;}
    void press(){next();++sample.source.controllers.hands[1].pressed[gears_vr::control_trigger];sample.source.controllers.hands[1].trigger=1;tick();}
    void release(){next();++sample.source.controllers.hands[1].released[gears_vr::control_trigger];sample.source.controllers.hands[1].trigger=0;tick();}
};
int main(){
    {Fixture f;f.press();assert(api::owed()&&presses==1&&api::input.owns_press());
        f.release();assert(!api::owed()&&releases==1&&ends==0&&!api::input.owns_press());
        f.next();f.tick();assert(presses==1&&releases==1);}
    {Fixture f;f.press();fade_audio=true;f.release();
        assert(api::owed()&&releases==1&&ends==0);
        api::service();api::service();assert(api::owed()&&releases==1&&ends==0);
        hod_state::actual.loop_playing=false;api::service();assert(!api::owed());}
    {Fixture f;f.press();release_clears=false;f.release();
        assert(releases==1&&ends==1&&!api::owed());api::service(true);assert(ends==1);}
    {Fixture f;f.press();current=false;api::cancel();assert(!api::owed()&&releases==1);
        current=true;for(unsigned i=0;i<3;++i){f.next();f.tick();}assert(presses==1);
        f.release();f.press();assert(presses==2);}
    {Fixture f;f.press();fields=false;api::cancel();assert(api::owed()&&releases==0);
        fields=true;api::service();assert(!api::owed()&&releases==1);}
    {Fixture f;f.press();hod_state::actual.manager=object(91);api::cancel();
        assert(api::owed()&&releases==0&&ends==0);hod_state::actual.manager=object(90);api::service();assert(!api::owed());}
    {Fixture f;f.press();auto foreign=f.sample.source;foreign.weapon=object(6);
        assert(api::yield_for_equipment(foreign)&&api::owed()&&releases==0);
        selected_weapon=6;api::cancel();assert(api::owed()&&releases==0&&ends==0);}
    {Fixture f;f.press();fade_audio=true;assert(api::yield_for_equipment(f.sample.source));
        assert(releases==1&&api::owed());hod_state::actual.loop_playing=false;
        assert(api::yield_for_equipment(f.sample.source)&&!api::owed());}
    {Fixture f;dispatch_returns=false;fade_audio=true;f.press();assert(api::owed()&&api::faulted&&presses==1);
        dispatch_returns=true;hod_state::actual.loop_playing=false;api::service(true);assert(!api::owed()&&releases==1);
        f.release();f.press();assert(presses==1);}
    std::puts("HOD production input/owner composition: edge, fade, fallback, loss/rearm, manager, foreign owner and unknown-call boundaries passed (synthetic only)");
    return 0;
}
