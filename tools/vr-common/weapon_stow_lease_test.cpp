#include "weapon_stow_lease.h"
#include <cassert>
#include <cstdio>
#include <vector>

using namespace gears_weapon;
struct Backend {
    StowState<unsigned> state{7,StowAttachment::original_hand,true};
    std::vector<bool> calls;
    bool read_ok=true,call_ok=true,mutate=true,change_key{};
    bool read(StowState<unsigned>& out){out=state;return read_ok;}
    bool set_holstered(bool value){
        calls.push_back(value);
        if(mutate)state.attachment=value?StowAttachment::detached:StowAttachment::original_hand;
        if(change_key)++state.key;
        return call_ok;
    }
};
struct GuardRejectedBackend:Backend {
    WeaponStowLease<unsigned>* journal{};
    StowAttachment encountered{StowAttachment::original_hand};
    bool set_holstered(bool value){
        if(!value)return Backend::set_holstered(false);
        // The final guard refuses before any true call. Another game owner
        // may already have detached the same mesh during the preceding read.
        state.attachment=encountered;
        journal->revoke();
        return false;
    }
};
struct RelinquishBackend:Backend {
    WeaponStowLease<unsigned>* journal{};
    bool check_revoked{},reentered{};
    bool read(StowState<unsigned>& out){
        if(check_revoked){
            assert(journal->revoked());
            check_revoked=false;reentered=true;
            assert(!journal->restore(*this)); // Reentrant detached cannot call false.
        }
        return Backend::read(out);
    }
};
struct VisibilityBackend {
    unsigned key{7};bool hidden{},ready{true};unsigned calls{};
    bool read(StowState<unsigned>& out){out={key,hidden?StowAttachment::detached:StowAttachment::original_hand,ready};return true;}
    bool set_holstered(bool value){++calls;hidden=value;return true;}
};
int main(){
    {
        // Empty-hand preview must not flash during ordinary state changes:
        // acquiring is one real write; keeping it is read-only. A separate
        // ground pickup, foreign identity and revoked interval stay outside.
        VisibilityBackend held,ground;ground.key=91;
        WeaponStowLease<unsigned> hidden;
        assert(hidden.acquire(held)&&held.hidden&&held.calls==1);
        for(unsigned tick=0;tick<120;++tick)assert(hidden.retains_detached(held));
        assert(held.calls==1&&!ground.hidden&&ground.calls==0);
        assert(!hidden.retains_detached(ground)&&!ground.hidden&&ground.calls==0);
        held.ready=false;assert(!hidden.retains_detached(held)&&held.calls==1);held.ready=true;
        ++held.key;assert(!hidden.retains_detached(held)&&held.calls==1);--held.key;
        // Real unholster/exit still restores the original bit exactly once.
        assert(hidden.restore(held)&&!held.hidden&&held.calls==2);
        assert(!hidden.retains_detached(held));assert(hidden.reset()&&hidden.acquire(held));
        hidden.revoke();assert(!hidden.retains_detached(held)&&held.calls==3);
        assert(!hidden.restore(held)&&held.hidden&&held.calls==3);
        held.hidden=false;assert(hidden.restore(held)&&held.calls==3);
    }
    assert(empty_pickup_touch_body({0x04,0x0b,0x53},3,0,0xffff,true));
    assert(!empty_pickup_touch_body({0x04,0x0b,0x53},3,0x400,0xffff,true));
    assert(!empty_pickup_touch_body({0x04,0x0b,0x53},3,0,0xffff,false));
    assert(!empty_pickup_touch_body({0x04,0x0b,0x53},3,0,0,true));
    assert(!empty_pickup_touch_body({0x04,0x0b,0x53},4,0,0xffff,true));
    assert(!empty_pickup_touch_body({0x04,0x0b,0x54},3,0,0xffff,true));
    // CanSwitchWeapons/bSwitchingWeapons can report idle while the original
    // ground-pickup SpecialMove is still running. It must finish first.
    assert(!stow_phase_allowed(StowPhase::active,true,false,true,false));
    assert(!stow_phase_allowed(StowPhase::active,true,false,false,false));
    assert(stow_phase_allowed(StowPhase::active,true,false,true,true));
    {
        // Reproduce the visible gap: the weapon has become Active, but the
        // body blend still refuses the normal holster. Do not reveal early.
        VisibilityBackend b;WeaponStowLease<unsigned> visibility;
        assert(visibility.acquire(b));
        assert(retain_equip_visibility(StowPhase::equipping,false,true,true));
        assert(retain_equip_visibility(StowPhase::active,true,true,true));
        assert(retain_equip_visibility(StowPhase::active,false,false,true));
        assert(b.hidden&&b.calls==1);
        assert(!retain_equip_visibility(StowPhase::active,true,false,true));
        assert(stow_phase_allowed(StowPhase::active,true,false,true,true));
        assert(visibility.restore(b)&&!b.hidden&&b.calls==2);
        for(auto phase:{StowPhase::equipping,StowPhase::active,StowPhase::unknown})
            assert(!retain_equip_visibility(phase,false,true,false));
        assert(!retain_equip_visibility(StowPhase::unknown,false,true,true));
    }
    {
        // The same journal also owns one transient visibility bit, independently
        // of the original animation's attachment. It must restore before hold,
        // refuse originally hidden/foreign fields, and never reopen revoked rights.
        VisibilityBackend b;WeaponStowLease<unsigned> visibility;
        assert(visibility.acquire(b)&&b.hidden&&b.calls==1);
        assert(visibility.restore(b)&&!b.hidden&&b.calls==2);
        assert(visibility.reset());b.hidden=true;
        assert(!visibility.acquire(b)&&b.calls==2);
        b.hidden=false;assert(visibility.acquire(b));visibility.revoke();
        assert(!visibility.restore(b)&&b.hidden&&b.calls==3);
        b.hidden=false;assert(visibility.restore(b)&&b.calls==3);
        assert(visibility.reset()&&visibility.acquire(b));++b.key;
        assert(!visibility.restore(b)&&b.hidden&&b.calls==4);
    }
    for(bool can_switch:{false,true})for(bool busy:{false,true})for(bool idle:{false,true})for(bool clear:{false,true}){
        assert(!stow_phase_allowed(StowPhase::unknown,can_switch,busy,idle,clear));
        assert(stow_phase_allowed(StowPhase::active,can_switch,busy,idle,clear)==(can_switch&&!busy&&clear));
        assert(stow_phase_allowed(StowPhase::equipping,can_switch,busy,idle,clear)==(can_switch&&idle&&clear));
    }
    {
        StowIdleCadence<unsigned> cadence;
        assert(cadence.attempt(10,1000000000,true));
        assert(!cadence.attempt(10,1100000000,true));
        assert(cadence.attempt(11,1100000000,true)); // New weapon, not the old timer.
        assert(!cadence.attempt(11,1200000000,false));
        assert(!cadence.attempt(12,1210000000,false)); // Equip busy consumes no attempt.
        assert(cadence.attempt(12,1220000000,true));
        assert(!cadence.attempt(12,1210000000,true)); // Regressed clock is rejected.
        assert(!cadence.attempt(12,1400000000,true));
        assert(cadence.attempt(12,1720000000,true));
        cadence.reset();assert(cadence.attempt(12,10,true));
        assert(!cadence.attempt(12,0,true));
    }
    {
        Backend b;WeaponStowLease<unsigned> lease;
        assert(lease.acquire(b)&&lease.dirty()&&!lease.faulted());
        assert(b.calls==std::vector<bool>{true});
        assert(!lease.acquire(b)&&b.calls.size()==1);
        assert(!lease.reset());
        assert(lease.restore(b)&&!lease.dirty()&&!lease.faulted());
        assert(b.calls==(std::vector<bool>{true,false}));
        assert(lease.restore(b)&&b.calls.size()==2&&lease.reset());
    }
    for(const auto attachment:{StowAttachment::unknown,StowAttachment::detached,StowAttachment::other}){
        Backend b;b.state.attachment=attachment;WeaponStowLease<unsigned> lease;
        assert(!lease.acquire(b)&&!lease.dirty()&&b.calls.empty());
    }
    {
        Backend b;b.read_ok=false;WeaponStowLease<unsigned> lease;
        assert(!lease.acquire(b)&&!lease.dirty()&&b.calls.empty());
        b.read_ok=true;b.state.ready=false;
        assert(!lease.acquire(b)&&!lease.dirty()&&b.calls.empty());
    }
    {
        // A false call which actually wrote must be rolled back by readback.
        Backend b;b.call_ok=false;WeaponStowLease<unsigned> lease;
        assert(!lease.acquire(b)&&!lease.dirty()&&lease.faulted());
        assert(b.calls==(std::vector<bool>{true,false}));
        assert(b.state.attachment==StowAttachment::original_hand);
        assert(!lease.acquire(b));
    }
    {
        Backend b;b.mutate=false;WeaponStowLease<unsigned> lease;
        assert(!lease.acquire(b)&&!lease.dirty()&&lease.faulted());
        assert(b.calls==std::vector<bool>{true});
    }
    {
        Backend b;WeaponStowLease<unsigned> lease;assert(lease.acquire(b));
        ++b.state.key;
        assert(!lease.restore(b)&&lease.dirty()&&lease.faulted()&&b.calls.size()==1);
        assert(!lease.reset());
    }
    for(const auto attachment:{StowAttachment::unknown,StowAttachment::other}){
        Backend b;WeaponStowLease<unsigned> lease;assert(lease.acquire(b));
        b.state.attachment=attachment;
        assert(!lease.restore(b)&&lease.dirty()&&lease.faulted()&&b.calls.size()==1);
        b.state.attachment=StowAttachment::detached;
        assert(lease.revoked()&&!lease.restore(b)&&lease.dirty()&&b.calls.size()==1);
    }
    {
        Backend b;WeaponStowLease<unsigned> lease;assert(lease.acquire(b));
        b.state.ready=false;
        assert(!lease.restore(b)&&lease.dirty()&&lease.faulted()&&b.calls.size()==1);
        b.state.ready=true;b.call_ok=false;
        assert(lease.revoked()&&!lease.restore(b)&&b.calls.size()==1);
        b.state.attachment=StowAttachment::original_hand;
        assert(lease.restore(b)&&!lease.dirty()&&lease.faulted()&&b.calls.size()==1);
    }
    {
        Backend b;WeaponStowLease<unsigned> lease;assert(lease.acquire(b));
        b.state.attachment=StowAttachment::original_hand;
        assert(lease.restore(b)&&!lease.dirty()&&b.calls.size()==1);
    }
    {
        Backend b;b.change_key=true;WeaponStowLease<unsigned> lease;
        assert(!lease.acquire(b)&&lease.dirty()&&lease.faulted()&&b.calls.size()==1);
    }
    {
        Backend b;WeaponStowLease<unsigned> lease;assert(lease.acquire(b));
        b.read_ok=false;
        assert(!lease.restore(b)&&lease.dirty()&&lease.faulted()&&b.calls.size()==1);
        b.read_ok=true;
        assert(lease.revoked()&&!lease.restore(b)&&lease.dirty()&&b.calls.size()==1);
        b.state.attachment=StowAttachment::original_hand;
        assert(lease.restore(b)&&!lease.dirty()&&lease.faulted()&&b.calls.size()==1);
    }
    {
        Backend b;WeaponStowLease<unsigned> lease;assert(lease.acquire(b));
        b.mutate=false;
        assert(!lease.restore(b)&&lease.dirty()&&lease.revoked()&&b.calls.size()==2);
        b.mutate=true;
        assert(!lease.restore(b)&&lease.dirty()&&b.calls.size()==2);
    }
    {
        Backend b;WeaponStowLease<unsigned> lease;assert(lease.acquire(b));
        lease.revoke();
        assert(!lease.restore(b)&&lease.dirty()&&lease.revoked()&&b.calls.size()==1);
    }
    for(const auto attachment:{StowAttachment::original_hand,StowAttachment::detached}){
        WeaponStowLease<unsigned> lease;GuardRejectedBackend b;
        b.journal=&lease;b.encountered=attachment;
        assert(!lease.acquire(b)&&lease.faulted()&&lease.revoked()&&b.calls.empty());
        if(attachment==StowAttachment::original_hand){
            assert(!lease.dirty()&&lease.restore(b)&&b.calls.empty());
        }else{
            assert(lease.dirty()&&!lease.restore(b)&&b.calls.empty());
            b.state.attachment=StowAttachment::original_hand;
            assert(lease.restore(b)&&!lease.dirty()&&b.calls.empty());
        }
    }
    {
        WeaponStowLease<unsigned> lease;Backend b;
        assert(lease.acquire(b));b.state.attachment=StowAttachment::original_hand;
        assert(lease.relinquish(b)&&!lease.dirty()&&lease.revoked()&&!lease.faulted());
        assert(b.calls==std::vector<bool>{true});
        assert(lease.restore(b)&&lease.revoked()); // Restore never reopens authority.
        assert(lease.acquire(b)&&lease.dirty()&&!lease.revoked()); // A new explicit lease only.
        assert(lease.restore(b)&&b.calls==(std::vector<bool>{true,true,false}));
    }
    {
        WeaponStowLease<unsigned> lease;RelinquishBackend b;b.journal=&lease;
        assert(lease.acquire(b));b.check_revoked=true;
        assert(!lease.relinquish(b)&&b.reentered&&lease.dirty()&&lease.faulted()&&lease.revoked());
        assert(b.calls==std::vector<bool>{true});
        b.state.attachment=StowAttachment::original_hand;
        assert(lease.relinquish(b)&&!lease.dirty()&&lease.faulted());
        assert(!lease.acquire(b)&&b.calls==std::vector<bool>{true});
    }
    {
        // Actual51224 AddDecal argument shape. Only this direct local token
        // can be classified; instance access, contexts/calls and missing end
        // markers stay unknown and retain the conservative revoke path.
        std::array<unsigned char,10> local{0x00,0x20,0x88,0x13,0x35,0,0,0,0,0x16};
        assert(stow_local_component_argument(local)==0x35138820);
        local[0]=1;assert(!stow_local_component_argument(local));
        local[0]=0;local[9]=0;assert(!stow_local_component_argument(local));
        local[9]=0x16;for(unsigned i=1;i<9;++i)local[i]=0;
        assert(!stow_local_component_argument(local));
    }
    std::puts("stow journal partial-call rollback, foreign state, identity and exact hand restoration passed");
}
