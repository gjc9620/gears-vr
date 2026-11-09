#pragma once
#include <array>
#include <cstdint>
#include <limits>
#include "weapon_hand_participant.h"

// HB-only ledger for completion of an ORIGINAL six-shot burst. This does not
// own R2, retain a pose, install a timer or authorize an engine call by itself.
// Runtime integration must authenticate the original event, actor identities,
// state, metadata and Native source before supplying these observations.
namespace gears_hammerburst_burst {
struct Cohort {
    gears_weapon_hand::FrameKey key{};
    std::array<gears_weapon_hand::ObjectIdentity,4> actors{}; // world/controller/pawn/weapon
    std::uint64_t hold{},reference{},session{},space{},input{},profile{};
    std::uint32_t presses{},releases{};
    bool known{};
};
inline bool same(Cohort const& a,Cohort const& b) noexcept {
    auto ak=a.key,bk=b.key;ak.frame=bk.frame=0;
    if(!a.known||!b.known||!gears_weapon_hand::same(ak,bk)||a.hold!=b.hold||
       a.reference!=b.reference||a.session!=b.session||a.space!=b.space||
       a.input!=b.input||a.profile!=b.profile)return false;
    for(unsigned i=0;i<a.actors.size();++i)
        if(!gears_weapon_hand::same(a.actors[i],b.actors[i]))return false;
    return true;
}
struct Source {
    Cohort cohort{};
    std::uint64_t frame{};
    std::int64_t now{},sync{},stereo{},render{},sampled{};
    bool trigger_down{},authenticated{};
};
inline bool fresh(Source const& s,bool dispatch_reserve=false) noexcept {
    constexpr std::int64_t maximum_age=100000000,reserve=20000000;
    if(!s.authenticated||!s.cohort.known||!s.frame||s.now<=0)return false;
    for(auto stamp:{s.sync,s.stereo,s.render,s.sampled})
        if(stamp<=0||stamp>s.now||s.now-stamp>maximum_age-(dispatch_reserve?reserve:0))return false;
    return true;
}
enum class State {idle,committed,dispatching,cancel_required};
struct Ticket {
    std::uint64_t serial{},event{},frame{};
    unsigned before{};
};
class Completion {
    Cohort cohort_{};
    State state_{};
    std::uint64_t last_shot_{},serial_{},last_event_{};
    unsigned count_{};
    bool released_{};
    Ticket active_{};
    bool counters(Source const& s) const noexcept {
        if(!same(cohort_,s.cohort)||s.cohort.presses!=cohort_.presses)return false;
        if(s.trigger_down)return !released_&&s.cohort.releases==cohort_.releases;
        return cohort_.releases!=(std::numeric_limits<std::uint32_t>::max)()&&
            s.cohort.releases==cohort_.releases+1;
    }
public:
    State state() const noexcept{return state_;}
    unsigned count() const noexcept{return count_;}
    bool needs_cancel() const noexcept{return state_==State::cancel_required;}
    // Only the first VALID, source-consistent original shot under an actually
    // owned R2 press can commit a burst. State entry or a raw edge cannot arm.
    bool commit(Source const& s,std::uint64_t shot,unsigned original_count,
                unsigned original_limit,bool owned_press) noexcept {
        if(state_!=State::idle||!owned_press||!s.trigger_down||!fresh(s)||
           shot<=last_shot_||original_count!=1||original_limit!=6)return false;
        cohort_=s.cohort;last_shot_=shot;count_=1;released_=false;
        active_={};state_=State::committed;return true;
    }
    // Observation of a normal UP changes no button and grants no call. Any
    // incompatible ancestry/counter change permanently revokes this burst.
    bool observe(Source const& s,bool lifecycle_ready) noexcept {
        if(state_==State::idle||state_==State::cancel_required)return false;
        if(!lifecycle_ready||!s.authenticated||!counters(s)){revoke();return false;}
        if(!s.trigger_down)released_=true;
        return true;
    }
    // Event is the adapter's monotonic ID of ONE retained original invocation.
    // Its original 250ms wait is checked here and cannot be renewed by retries.
    bool enter(Source const& s,std::uint64_t event,std::int64_t captured,
               unsigned original_count,unsigned original_limit,Ticket& out) noexcept {
        out={};
        if(state_!=State::committed||!event||event<=last_event_)return false;
        if(!observe(s,true))return false;
        if(original_count!=count_||original_limit!=6||count_>=6){revoke();return false;}
        if(captured<=0||captured>s.now||s.now-captured>250000000){revoke();return false;}
        if(!fresh(s,true))return false; // Wait only for a NEW source, never renew it.
        if(serial_==(std::numeric_limits<std::uint64_t>::max)()){revoke();return false;}
        active_={++serial_,event,s.frame,count_};last_event_=event;
        state_=State::dispatching;out=active_;return true;
    }
    bool permits(Ticket const& ticket,Source const& s) const noexcept {
        return state_==State::dispatching&&ticket.serial&&
            ticket.serial==active_.serial&&ticket.event==active_.event&&
            ticket.frame==active_.frame&&ticket.before==active_.before&&
            s.frame==active_.frame&&counters(s)&&fresh(s);
    }
    // One successful original invocation must yield EXACTLY one next shot and
    // one counter increment. An ambiguous/partial execution is never replayed.
    bool leave(Ticket const& ticket,std::uint64_t shot,unsigned original_count,
               bool invoked,bool valid_trace) noexcept {
        if(state_!=State::dispatching||ticket.serial!=active_.serial||
           ticket.event!=active_.event||ticket.frame!=active_.frame||
           ticket.before!=active_.before)return false;
        if(!invoked||!valid_trace||shot<=last_shot_||original_count!=count_+1){revoke();return false;}
        last_shot_=shot;count_=original_count;active_={};
        state_=count_==6?State::idle:State::committed;return true;
    }
    void revoke() noexcept {
        if(state_!=State::idle)state_=State::cancel_required;
        active_={};
    }
    // Runtime must prove original Firing ended and actual PendingFire is UP.
    // A successful cleanup, not merely input release, discharges this debt.
    bool acknowledge_cancel(bool firing_ended,bool pending_up) noexcept {
        if(state_!=State::cancel_required||!firing_ended||!pending_up)return false;
        state_=State::idle;active_={};cohort_={};count_=0;released_=false;return true;
    }
};
}
