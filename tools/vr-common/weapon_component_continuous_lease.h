#pragma once
#include "weapon_mesh_component_lease.h"
#include <type_traits>

namespace gears_weapon {

// Offline protocol prototype; this is not connected to a live writer or to the
// existing clean-only policy. A retained journal never grants pose authority.
// The first original bytes remain immutable until an explicit drained reset.
// Calls, callbacks and Backend operations are synchronous, serialized and bool-
// returning/nonthrowing, as in the original component Backend contract.
//
// Backend read and three-argument restore setters retain the existing Key /
// Value / expected-value interface and live authentication obligations.
// Forward setters MUST instead expose these four-argument overloads:
//   set_absolute(Key const&,Value const&,Absolute const&,Permit&)
//   set_translation(Key const&,Value const&,Bytes const&,Permit&)
//   set_rotation(Key const&,Value const&,Bytes const&,Permit&)
// A forward setter must call permit() exactly once, at the final actual VM
// setter boundary. It may not dispatch that setter on false, retain Permit, or
// perform another engine boundary after a successful permit before that setter.
// It must also authenticate current identity/schema/complete expected bytes at
// that boundary; a callback that itself invokes engine code invalidates earlier
// live checks. There is deliberately no fallback to a three-argument forward
// setter, and a protocol-level pre-call callback would not meet this contract.
// C++ cannot prove a Backend obeys this physical engine-boundary contract.
//
// Authorize signature:
//   bool(frame,key,previous_complete,candidate_complete,group)
// group0 is independent per-update admission, including no-change updates;
// groups1/2/4 are separately fresh final-write authorizations. A candidate at a
// setter changes only that group of previous_complete. Neither a prior true
// result, the retained Key, nor a journal is reused as permission.
// Frame must own a deep, independent, immutable value snapshot. Trivial copying
// prevents custom copy callbacks but does NOT prove deep ownership or freshness.
// Caller/adapter must prove the current frame/Plan/certificate, same live object
// generation, action/epoch/attachment/tracking eligibility and original100ms
// rules on every authorization. This generic header has no clock or fake clean
// Context and cannot establish those facts or held-pose consumer safety.
template<class Key> class WeaponMeshComponentContinuousLease {
public:
    using Value=WeaponMeshComponentValue;
    using State=WeaponMeshComponentState<Key>;
    using Absolute=MeshComponentAbsolute;
    using Bytes=MeshComponentBytes12;
    static constexpr unsigned absolute_group=1,translation_group=2,rotation_group=4,all_groups=7;
private:
    Key key_{};
    Value original_{},owned_{};
    unsigned pending_{},revoked_{};
    bool snapshot_{},stopped_{},faulted_{},busy_{},relinquished_{};
    struct Busy {
        bool& flag;
        explicit Busy(bool& value)noexcept:flag(value){flag=true;}
        ~Busy(){flag=false;}
    };
    static bool same(Value const& a,Value const& b,unsigned group)noexcept {
        if(group==absolute_group)return a.absolute==b.absolute;
        if(group==translation_group)return a.translation==b.translation;
        return a.rotation==b.rotation;
    }
    static void assign(Value& to,Value const& from,unsigned group)noexcept {
        if(group==absolute_group)to.absolute=from.absolute;
        else if(group==translation_group)to.translation=from.translation;
        else to.rotation=from.rotation;
    }
    bool reject()noexcept {stopped_=faulted_=true;return false;}
    void revoke_pending()noexcept {stopped_=faulted_=true;revoked_|=pending_;}
    void observe_group(Value const& value,unsigned group)noexcept {
        if(same(value,original_,group)){
            // Read-only resolution, including an already revoked group. Do not
            // clear revoked/stopped flags or authorize a new ownership interval.
            pending_&=~group;assign(owned_,original_,group);
        }else if(!(pending_&group)||!same(value,owned_,group)){
            revoked_|=group;stopped_=faulted_=true;
        }
    }
    void observe(Value const& value)noexcept {
        for(unsigned group=1;group<=rotation_group;group<<=1)observe_group(value,group);
    }
    template<class Backend> bool read_valid(Backend& backend,State& state){
        state={};
        if(!backend.read(state)||!state.known||!(state.key==key_)){
            revoke_pending();return false;
        }
        return true;
    }
    template<class Backend> bool forward_read(Backend& backend,State& state){
        const Value expected=owned_;
        if(!read_valid(backend,state))return false;
        observe(state.value);
        // Even an unexpected return to original cannot silently reopen a
        // forward interval. It can only settle that journal read-only.
        if(!mesh_component_exact(state.value,expected))return reject();
        return !stopped_&&!faulted_;
    }
    void settle(Value const& value,Value const& previous,Value const& proposed,
                unsigned active,bool write_was_authorized)noexcept {
        for(unsigned group=1;group<=rotation_group;group<<=1){
            if(group!=active){observe_group(value,group);continue;}
            if(same(value,original_,group)){
                pending_&=~group;assign(owned_,original_,group);
            }else if(same(value,previous,group)){
                assign(owned_,previous,group); // Authenticated no-write result.
            }else if(write_was_authorized&&same(value,proposed,group)){
                assign(owned_,proposed,group); // Full write, including false return.
            }else{
                // Mixed write/foreign value or an unpermitted change: retain
                // pending and revoke the whole logical group. Later old/new
                // ABA values cannot revive permission; only exact original
                // can discharge this journal read-only.
                revoked_|=group;stopped_=faulted_=true;
            }
        }
    }
    template<class Frame,class Authorize> class Permit {
        WeaponMeshComponentContinuousLease& owner_;
        Frame const& frame_;
        Authorize& authorize_;
        Key const& write_key_;
        Value const& previous_;
        Value const& proposed_;
        unsigned group_;
        bool called_{},allowed_{},reused_{};
    public:
        Permit(WeaponMeshComponentContinuousLease& owner,Frame const& frame,Authorize& authorize,
               Key const& identity,Value const& previous,Value const& proposed,unsigned group)noexcept:
            owner_(owner),frame_(frame),authorize_(authorize),write_key_(identity),previous_(previous),proposed_(proposed),group_(group){}
        Permit(Permit const&)=delete;
        Permit& operator=(Permit const&)=delete;
        bool operator()(){
            if(called_){reused_=true;owner_.revoke_pending();return false;}
            called_=true;
            if(owner_.stopped_||owner_.faulted_)return false;
            const bool authorized=authorize_(frame_,write_key_,previous_,proposed_,group_);
            // The callback may synchronously cancel, revoke or reenter. Its
            // returned true is never sufficient after any of those changes.
            allowed_=authorized&&!owner_.stopped_&&!owner_.faulted_;
            if(!allowed_)owner_.stopped_=true;
            return allowed_;
        }
        bool valid_once()const noexcept {return called_&&allowed_&&!reused_;}
    };
    template<class Backend,class Gate>
    bool write_forward(Backend& backend,Value const& expected,Value const& wanted,unsigned group,Gate& permit){
        if(group==absolute_group)return backend.set_absolute(key_,expected,wanted.absolute,permit);
        if(group==translation_group)return backend.set_translation(key_,expected,wanted.translation,permit);
        return backend.set_rotation(key_,expected,wanted.rotation,permit);
    }
    template<class Backend>
    bool write_restore(Backend& backend,Value const& expected,unsigned group){
        if(group==absolute_group)return backend.set_absolute(key_,expected,original_.absolute);
        if(group==translation_group)return backend.set_translation(key_,expected,original_.translation);
        return backend.set_rotation(key_,expected,original_.rotation);
    }
    template<class Frame,class Authorize,class Backend>
    bool update_impl(Value const& desired,Frame const& frame,Authorize& authorize,Backend& backend){
        State current{};
        if(!forward_read(backend,current))return false;
        const Key admission_key=key_;
        const Value admission_previous=owned_;
        if(!authorize(frame,admission_key,admission_previous,desired,0)||stopped_||faulted_)return reject();
        // Admission may itself reach engine work. Do not carry its earlier read
        // set across that callback; every group starts with another live read.
        for(unsigned group=1;group<=rotation_group;group<<=1){
            State before{};
            if(!forward_read(backend,before))return false;
            if(same(before.value,desired,group))continue;
            const Key write_key=key_;
            const Value previous=before.value;
            Value proposed=previous;assign(proposed,desired,group);
            pending_|=group; // Journal BEFORE callback and possibly partial write.
            Permit<Frame,Authorize> permit(*this,frame,authorize,write_key,previous,proposed,group);
            const bool called=write_forward(backend,previous,proposed,group,permit);
            State after{};
            if(!read_valid(backend,after))return false;
            settle(after.value,previous,proposed,group,permit.valid_once());
            if(!called||!permit.valid_once()||!mesh_component_exact(after.value,proposed))return reject();
            if(stopped_||faulted_)return false;
        }
        return !stopped_&&!faulted_;
    }
public:
    WeaponMeshComponentContinuousLease()=default;
    WeaponMeshComponentContinuousLease(WeaponMeshComponentContinuousLease const&)=delete;
    WeaponMeshComponentContinuousLease& operator=(WeaponMeshComponentContinuousLease const&)=delete;
    WeaponMeshComponentContinuousLease(WeaponMeshComponentContinuousLease&&)=delete;
    WeaponMeshComponentContinuousLease& operator=(WeaponMeshComponentContinuousLease&&)=delete;
    bool dirty()const noexcept {return pending_!=0;}
    bool faulted()const noexcept {return faulted_;}
    bool forward_stopped()const noexcept {return stopped_;}
    bool has_snapshot()const noexcept {return snapshot_;}
    bool relinquished()const noexcept {return relinquished_;}
    unsigned dirty_mask()const noexcept {return pending_;}
    unsigned owned_mask()const noexcept {return pending_&~revoked_;}
    unsigned revoked_mask()const noexcept {return revoked_;}
    Key const& key()const noexcept {return key_;}
    Value const& original_value()const noexcept {return original_;}
    Value const& owned_value()const noexcept {return owned_;}
    void cancel_forward()noexcept {stopped_=true;}
    void revoke()noexcept {revoke_pending();}
    // Explicit external handoff record. It withdraws rollback writes but does
    // NOT erase pending bytes or claim restoration. An unresolved journal must
    // be retained by the caller as unresolved external handoff evidence; reset
    // cannot manufacture a clean interval from it.
    void relinquish()noexcept {relinquished_=true;revoke_pending();}
    bool reset()noexcept {
        if(busy_||dirty())return false;
        key_={};original_={};owned_={};pending_=revoked_=0;
        snapshot_=stopped_=faulted_=relinquished_=false;return true;
    }
    template<class Frame,class Authorize,class Backend>
    bool acquire(Key const& expected_key,Value const& wanted,Frame const& input,Authorize& authorize,Backend& backend){
        static_assert(std::is_trivially_copyable<Frame>::value,"Frame must be an independent plain-value snapshot");
        const Value desired=wanted;const Frame frame=input;const Key expected=expected_key;
        if(busy_){revoke_pending();return false;}
        if(snapshot_||dirty())return reject();
        if(stopped_||faulted_)return false;
        Busy busy(busy_);key_=expected;
        State first{};
        if(!read_valid(backend,first))return false;
        original_=owned_=first.value;snapshot_=true;
        return update_impl(desired,frame,authorize,backend);
    }
    template<class Frame,class Authorize,class Backend>
    bool update(Key const& expected_key,Value const& wanted,Frame const& input,Authorize& authorize,Backend& backend){
        static_assert(std::is_trivially_copyable<Frame>::value,"Frame must be an independent plain-value snapshot");
        const Value desired=wanted;const Frame frame=input;const Key expected=expected_key;
        if(busy_){revoke_pending();return false;}
        if(!snapshot_||stopped_||faulted_)return false;
        if(!(expected==key_)){revoke_pending();return false;}
        Busy busy(busy_);return update_impl(desired,frame,authorize,backend);
    }
    // No fresh pose is required for handback. Each Backend read/set still must
    // authenticate the live Key and exact expected bytes. True means only that
    // every journaled group is now observed at its first original value, never
    // a clean-policy Context or permission to resume after cancellation/fault.
    template<class Backend> bool yield(Backend& backend){
        if(busy_){revoke_pending();return false;}
        Busy busy(busy_);stopped_=true;
        for(unsigned group=1;group<=rotation_group;group<<=1){
            if(!(pending_&group))continue;
            State current{};
            if(!read_valid(backend,current))return false;
            observe(current.value);
            if(!(pending_&group)||(revoked_&group))continue;
            const Value previous=current.value;
            Value proposed=previous;assign(proposed,original_,group);
            const bool called=write_restore(backend,previous,group);
            State after{};
            if(!read_valid(backend,after))return false;
            settle(after.value,previous,proposed,group,true);
            if(!called||!same(after.value,original_,group))faulted_=true;
        }
        return !dirty();
    }
};
} // namespace gears_weapon
