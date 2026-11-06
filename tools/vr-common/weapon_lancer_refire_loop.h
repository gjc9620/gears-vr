#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace gears_lancer_refire_loop {
struct Name {std::uint32_t index{},number{};};
// The complete original WeaponFiring.BeginState body. Its normal dispatch is
// retained as one event, rather than jumping into a live VM frame after ammo.
inline bool first_shot(unsigned char const* code,std::size_t bytes,
    Name ammunition,Name timing,std::uint64_t current_mode) noexcept {
    if(!code||!ammunition.index||ammunition.number||!timing.index||timing.number||!current_mode)return false;
    std::array<unsigned char,32> expected{};std::size_t n{};
    expected[n++]=0x1b;std::memcpy(expected.data()+n,&ammunition,sizeof ammunition);n+=sizeof ammunition;
    expected[n++]=0x16;expected[n++]=0x1b;
    std::memcpy(expected.data()+n,&timing,sizeof timing);n+=sizeof timing;
    expected[n++]=0x01; // CurrentFireMode is an instance property, not a callee local.
    std::memcpy(expected.data()+n,&current_mode,sizeof current_mode);n+=sizeof current_mode;
    expected[n++]=0x16;expected[n++]=0x04;expected[n++]=0x0b;expected[n++]=0x53;
    return bytes==n&&!std::memcmp(code,expected.data(),n);
}

enum class EventKind:unsigned {none,first,refire};
// One actually observed original invocation. Coalescing never renews its
// original deadline. An undelivered BeginState owes original state handback,
// whereas a cancelled timer call leaves the existing looping timer intact.
class DeferredEvent {
    EventKind kind_{};std::int64_t began_{};bool cancelled_{};
public:
    static constexpr std::int64_t max_wait_ns=250000000;
    EventKind kind() const noexcept{return kind_;}
    bool empty() const noexcept{return kind_==EventKind::none;}
    bool capture(EventKind kind,std::int64_t now) noexcept {
        if(kind==EventKind::none||now<=0)return false;
        if(!empty())return kind_==kind&&!cancelled_&&current(now);
        kind_=kind;began_=now;cancelled_=false;return true;
    }
    bool current(std::int64_t now) const noexcept {
        return !empty()&&now>=began_&&now-began_<=max_wait_ns;
    }
    bool dispatchable(std::int64_t now) const noexcept{return !cancelled_&&current(now);}
    bool release_required(std::int64_t now) const noexcept{return !empty()&&(cancelled_||!current(now));}
    bool first_debt() const noexcept{return kind_==EventKind::first;}
    void cancel() noexcept {if(first_debt())cancelled_=true;else reset();}
    bool claim(std::int64_t now) noexcept {
        if(!dispatchable(now))return false;
        reset();return true;
    }
    void reset() noexcept {kind_=EventKind::none;began_=0;cancelled_=false;}
};
// Evidence-only match of the original TimeWeaponFiring expression:
// SetTimer(GetFireInterval(FireModeNum),true,'RefireCheckTimer'). This is not
// a VM interpreter and never consumes a live FFrame or grants a shot.
// Common tokens: Development/Src/Core/Inc/UnStack.h, EExprToken. The retail
// TimeWeaponFiring observed in session18592 also has the GoW 0x50 undefined
// optional argument (UELib1.12 DefaultEngineBranch/UndefinedVariableToken).
inline bool looping_timer(unsigned char const* code,std::size_t bytes,
    Name interval,std::uint64_t mode_property,Name refire) noexcept {
    if(!code||bytes>256||!mode_property||!interval.index||interval.number||!refire.index||refire.number)return false;
    std::array<unsigned char,32> expected{};std::size_t n{};
    expected[n++]=0x61;expected[n++]=0x18; // original Actor.SetTimer native280
    expected[n++]=0x1b;std::memcpy(expected.data()+n,&interval,sizeof interval);n+=sizeof interval;
    expected[n++]=0x00;std::memcpy(expected.data()+n,&mode_property,sizeof mode_property);n+=sizeof mode_property;
    expected[n++]=0x16;expected[n++]=0x27;expected[n++]=0x21;
    std::memcpy(expected.data()+n,&refire,sizeof refire);n+=sizeof refire;
    unsigned matches{};
    for(std::size_t i=0;i+n<bytes;++i){
        if(std::memcmp(code+i,expected.data(),n))continue;
        auto end=i+n;
        if(code[end]==0x4a||code[end]==0x50)++end; // exact omitted optional argument forms
        if(end<bytes&&code[end]==0x16)++matches;
    }
    return matches==1;
}
}
