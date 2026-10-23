#pragma once
#include <array>
#include <cstdint>

namespace gears_weapon {

using MeshComponentBytes12 = std::array<unsigned char,12>;
using MeshComponentAbsolute = std::array<bool,3>;
struct WeaponMeshComponentValue {
    MeshComponentAbsolute absolute{}; // translation, rotation, scale flags
    MeshComponentBytes12 translation{},rotation{};
};
inline bool mesh_component_exact(WeaponMeshComponentValue const& a,
                                 WeaponMeshComponentValue const& b) noexcept {
    return a.absolute==b.absolute&&a.translation==b.translation&&a.rotation==b.rotation;
}
template<class Key> struct WeaponMeshComponentState {
    Key key{};
    WeaponMeshComponentValue value{};
    bool known{};
};

// Component fields only: this does not own or restore the pawn's attachment.
// Key equality and backend authentication must cover the complete live object
// generation and caller context, including relevant observed action boundaries.
// All operations are serialized by the caller on the owning game thread.
//
// Backend contract:
//   bool read(WeaponMeshComponentState<Key>&);
//   bool set_absolute(Key const&, Value const& expected, Absolute const& wanted);
//   bool set_translation(Key const&, Value const& expected, Bytes12 const& wanted);
//   bool set_rotation(Key const&, Value const& expected, Bytes12 const& wanted);
// Each setter authenticates Key and compares the complete expected value again
// immediately before calling the native setter. It may change only its named
// group. A false return can still have written; subsequent authenticated readback
// is required. Backend operations report failures with bool, without throwing.
// Values/keys alone do not detect an unobserved foreign write-and-return (ABA).
template<class Key> class WeaponMeshComponentLease {
public:
    static constexpr unsigned absolute_group=1,translation_group=2,rotation_group=4;
    static constexpr unsigned all_groups=7;
    using Value=WeaponMeshComponentValue;
    using State=WeaponMeshComponentState<Key>;
private:
    Key key_{};
    Value original_{},owned_{};
    unsigned pending_{},revoked_{};
    bool snapshot_{},faulted_{},busy_{};
    struct Busy {
        bool& flag;
        explicit Busy(bool& value):flag(value){flag=true;}
        ~Busy(){flag=false;}
    };
    static bool same(Value const& a,Value const& b,unsigned group) noexcept {
        if(group==absolute_group)return a.absolute==b.absolute;
        if(group==translation_group)return a.translation==b.translation;
        return a.rotation==b.rotation;
    }
    static void assign(Value& to,Value const& from,unsigned group) noexcept {
        if(group==absolute_group)to.absolute=from.absolute;
        else if(group==translation_group)to.translation=from.translation;
        else to.rotation=from.rotation;
    }
    void revoke_pending() noexcept {faulted_=true;revoked_|=pending_;}
    void observe(Value const& current) noexcept {
        for(unsigned group=1;group<=rotation_group;group<<=1){
            if(same(current,original_,group)){
                pending_&=~group;assign(owned_,original_,group);
            }else if(pending_&group){
                if(!same(current,owned_,group)){faulted_=true;revoked_|=group;}
            }else{
                // A never-owned or already-returned group is foreign. Preserve
                // it, fault the session, and continue cleaning other owned groups.
                faulted_=true;
            }
        }
    }
    template<class Backend> bool read(Backend& backend,State& state) {
        state={};
        if(!backend.read(state)||!state.known||!(state.key==key_)){
            revoke_pending();return false;
        }
        if(snapshot_)observe(state.value);
        return true;
    }
    template<class Backend>
    bool write(Backend& backend,Value const& expected,Value const& wanted,unsigned group) {
        if(group==absolute_group)return backend.set_absolute(key_,expected,wanted.absolute);
        if(group==translation_group)return backend.set_translation(key_,expected,wanted.translation);
        return backend.set_rotation(key_,expected,wanted.rotation);
    }
    template<class Backend> bool restore_owned(Backend& backend) {
        for(unsigned group=1;group<=rotation_group;group<<=1){
            if(!(pending_&group))continue;
            State current{};
            if(!read(backend,current))return false;
            if(!(pending_&group)||(revoked_&group))continue;
            if(!write(backend,current.value,original_,group))faulted_=true;
            State after{};
            if(!read(backend,after))return false;
            if(pending_&group)faulted_=true;
        }
        return !dirty();
    }
    template<class Backend> bool fail(Backend& backend) {
        faulted_=true;restore_owned(backend);return false;
    }
public:
    bool dirty() const noexcept {return pending_!=0;}
    bool faulted() const noexcept {return faulted_;}
    bool has_snapshot() const noexcept {return snapshot_;}
    unsigned dirty_mask() const noexcept {return pending_;}
    unsigned owned_mask() const noexcept {return pending_&~revoked_;}
    unsigned revoked_mask() const noexcept {return revoked_;}
    Key const& key() const noexcept {return key_;}
    Value const& original_value() const noexcept {return original_;}
    Value const& owned_value() const noexcept {return owned_;}
    // Explicit drained lifecycle operation only. A parent session must end on
    // any fault; reset is not permission to silently restart that failed session.
    bool reset() noexcept {
        if(busy_||dirty())return false;
        key_={};original_={};owned_={};pending_=revoked_=0;
        snapshot_=faulted_=false;return true;
    }
    template<class Backend>
    bool acquire(Key const& expected_key,Value const& wanted,Backend& backend) {
        if(busy_){revoke_pending();return false;}
        if(dirty()||faulted_){faulted_=true;return false;}
        Busy busy(busy_);
        const Value desired=wanted; // Snapshot before clearing getter aliases.
        key_=expected_key;original_={};owned_={};snapshot_=false;
        State first{};
        if(!read(backend,first))return false;
        original_=owned_=first.value;snapshot_=true;
        Value expected=original_;
        for(unsigned group=1;group<=rotation_group;group<<=1){
            State before{};
            if(!read(backend,before)||faulted_||!mesh_component_exact(before.value,expected))return fail(backend);
            if(same(original_,desired,group))continue;
            // Journal before the call: even a false result may have applied it.
            pending_|=group;assign(owned_,desired,group);
            const bool called=write(backend,before.value,desired,group);
            assign(expected,desired,group);
            State after{};
            const bool known=read(backend,after);
            if(!called||!known||faulted_||!mesh_component_exact(after.value,expected))return fail(backend);
        }
        return true;
    }
    // True means every journaled group has authenticated exact original bytes.
    // It does not clear sticky fault, or certify never-owned foreign fields.
    // Revoked groups allow read-only clearing at the original value, never a
    // later write merely because the old owned value reappeared.
    template<class Backend> bool restore(Backend& backend) {
        if(busy_){revoke_pending();return false;}
        Busy busy(busy_);return restore_owned(backend);
    }
};
} // namespace gears_weapon
