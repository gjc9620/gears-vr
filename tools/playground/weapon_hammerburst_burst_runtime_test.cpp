// Actual completion bridge with bounded metadata/engine/source fakes. The
// original-function certificate is a stub here, not retail contract proof.
#define HB_CANCEL_FIXTURE_ONLY
#include "weapon_hammerburst_burst_cancel_test.cpp"
namespace nativeequipment {
bool current(void*,gears_weapon_hand::Frame const&){return fixture::model.native_source;}
}
namespace hammerburst_burst_contract {
struct Certificate {
    weaponhand::HoldBinding held{};gripdamage_metadata::Property count{},limit{};
    uptr count_property{},limit_property{};bool known{};
};
bool retain(gripdamage_metadata::Reader& r,gripdamage_metadata::Property const& p){
    if(r.result.field_count==gripdamage_metadata::max_fields)return false;
    r.result.fields[r.result.field_count++]=p;return true;
}
bool prepare(weaponhand::HoldBinding const& h,uptr,Certificate& out){
    if(!fixture::model.receiver)return false;
    out={h,gripdamage_metadata::Reader::properties[fixture::hb_count],
        gripdamage_metadata::Reader::properties[fixture::hb_limit],
        fixture::address(fixture::hb_count),fixture::address(fixture::hb_limit),true};return true;
}
}
#include "weapon_hammerburst_burst_runtime.inl"
namespace bridge=hammerburst_burst_runtime;
gears_weapon_hand::Frame frame(){
    gears_weapon_hand::Frame f{};auto const& h=weaponhand::held_binding;
    f.key={101,h.native_started,h.native_cookie,104,1000,h.world_generation,105,106};
    f.world=h.world;f.controller=h.controller;f.pawn=h.pawn;f.weapon=h.weapon;
    f.reference_generation=200;f.stereo.session=201;f.stereo.space_epoch=202;
    f.controllers.input_epoch=203;f.controllers.profile_generation=204;
    f.controllers.hands[1].pressed[gears_vr::control_trigger]=12;
    f.controllers.hands[1].released[gears_vr::control_trigger]=11;
    f.controllers.hands[1].trigger_active=1;f.controllers.hands[1].trigger=1;
    f.controllers.sync_ns=f.stereo.received_ns=fixture::model.now-10000000;return f;
}
bridge::policy::Source fresh(gears_weapon_hand::Frame const& f){
    return bridge::source(f,fixture::model.now-10000000,fixture::model.now-1000000,fixture::model.now);
}
void reset_bridge(){
    reset();bridge::certificate={};bridge::completion={};bridge::pressed={};
    bridge::first_debt=bridge::firing_debt=bridge::cancel_requested=bridge::scope_active=bridge::scope_first=false;
    bridge::ticket={};bridge::entered={};bridge::observed={};
}
void prime_first(gears_weapon_hand::Frame const& f){
    assert(bridge::prime(f,0x1234));assert(bridge::press(f));assert(bridge::capture_first());
}
void first_shot(gears_weapon_hand::Frame const& f){
    prime_first(f);auto s=fresh(f);assert(bridge::enter(true,s,1,fixture::model.now));
    fixture::model.burst_count=1;bridge::trace(s,100,true);assert(bridge::leave(true));
}
int main(){
    unsigned checks{};
    for(unsigned release_after=1;release_after<=6;++release_after){
        reset_bridge();auto f=frame();first_shot(f);
        for(unsigned before=1;before<6;++before){
            fixture::model.now+=70000000;++f.key.frame;
            f.controllers.sync_ns=f.stereo.received_ns=fixture::model.now-10000000;
            if(before>=release_after){fixture::model.owned_press=false;
                f.controllers.hands[1].trigger=0;f.controllers.hands[1].released[gears_vr::control_trigger]=12;}
            assert(bridge::need_source(f));auto s=fresh(f);
            assert(bridge::enter(false,s,before+1,fixture::model.now));assert(bridge::permits(s));
            fixture::model.burst_count=before+1;bridge::trace(s,100+before,true);
            bridge::trace(s,100+before,true); // War and Engine roles share ONE actual shot.
            assert(bridge::leave(true));assert(!bridge::permits(s));checks+=5;
        }
        assert(bridge::completion.state()==bridge::policy::State::idle);
        assert(!bridge::need_source(f));
        // Original automatic next burst is a NEW captured Engine BeginState,
        // not a seventh continuation permit. It remains bounded by real R2.
        fixture::model.burst_count=0;
        assert(bridge::capture_first());auto s=fresh(f);
        const bool held=release_after==6;fixture::model.owned_press=held;
        assert(bridge::enter(true,s,7,fixture::model.now)==held);
        bridge::revoke();fixture::model.r2=false;fixture::model.owned_press=false;
        if(held)assert(!bridge::leave(true));
        assert(bridge::cancel());assert(!bridge::firing_debt);checks+=6;
    }
    // Callback ambiguity, stale source, missing trace, partial execution and
    // revoked callbacks must all retain cancellation debt without replay.
    for(unsigned failure=0;failure<7;++failure){
        reset_bridge();auto f=frame();prime_first(f);auto s=fresh(f);
        assert(bridge::enter(true,s,1,fixture::model.now));fixture::model.burst_count=1;
        if(failure!=0)bridge::trace(s,100,failure!=1);
        if(failure==2)bridge::trace(s,101,true);
        if(failure==3){auto stale=s;stale.now+=101000000;bridge::trace(stale,100,true);}
        if(failure==4)fixture::model.burst_count=2;
        if(failure==5)bridge::revoke();
        assert(!bridge::leave(failure!=6));assert(bridge::cancel_requested);
        assert(!bridge::enter(true,s,1,fixture::model.now));
        fixture::model.r2=true;assert(!bridge::cancel());assert(!fixture::model.go_calls);
        fixture::model.r2=false;assert(bridge::cancel());assert(fixture::model.go_calls==1);checks+=8;
    }
    // Cancellation while an original invocation is on the stack may revoke
    // permission, but cannot change its state until that invocation unwinds.
    {reset_bridge();auto f=frame();prime_first(f);auto s=fresh(f);
        assert(bridge::enter(true,s,1,fixture::model.now));assert(!bridge::cancel());
        assert(!fixture::model.go_calls);fixture::model.burst_count=1;bridge::trace(s,100,true);
        assert(!bridge::leave(true));assert(bridge::cancel());checks+=5;}
    for(unsigned failure=0;failure<5;++failure){
        reset_bridge();auto f=frame();first_shot(f);
        if(failure==0)++f.key.lifecycle;
        if(failure==1)++f.controllers.input_epoch;
        if(failure==2)++f.reference_generation;
        if(failure==3)++f.controllers.hands[1].pressed[gears_vr::control_trigger];
        if(failure==4)f.controllers.hands[1].released[gears_vr::control_trigger]+=2;
        assert(!bridge::need_source(f));assert(bridge::cancel_requested);
        assert(!bridge::prime(frame(),0x1234));assert(bridge::cancel());checks+=4;
    }
    {reset_bridge();auto f=frame();prime_first(f);auto s=fresh(f);
        s.sync=fixture::model.now-80000001;assert(!bridge::enter(true,s,1,fixture::model.now));
        assert(bridge::first_debt&&!bridge::scope_active);assert(bridge::cancel());checks+=3;}
    {reset_bridge();auto f=frame();prime_first(f);fixture::model.owned_press=false;
        assert(!bridge::enter(true,fresh(f),1,fixture::model.now));assert(bridge::cancel());checks+=2;}
    {reset_bridge();auto f=frame();fixture::model.native_source=false;
        assert(!bridge::prime(f,0x1234));assert(!bridge::capture_first());assert(!bridge::firing_debt);checks+=3;}
    std::printf("HB actual completion bridge: %u fake scope/input/cleanup boundary checks passed; original certificate is a stub, no game proof\n",checks);
}
