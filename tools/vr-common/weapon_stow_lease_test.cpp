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
int main(){
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
    std::puts("stow journal partial-call rollback, foreign state, identity and exact hand restoration passed");
}
