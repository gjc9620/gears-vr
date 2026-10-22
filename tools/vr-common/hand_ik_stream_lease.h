#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace gears_hand_ik_stream {
using Target=std::array<unsigned char,12>;
using Strength=std::array<float,3>;
struct Value {Strength strength{};Target target{};};
inline bool exact(Strength const& a,Strength const& b) noexcept {
    return std::memcmp(a.data(),b.data(),sizeof(float)*a.size())==0;
}
inline bool exact(Target const& a,Target const& b) noexcept {return a==b;}

// This is only one necessary condition for a private idle lease: the caller
// must separately prove a quiet parent, known mesh/slot identities and idle
// original action state. A stopped Lancer slot retains the looping sequence's
// bPlaying flag with all three pose weights exactly zero. This retained flag
// alone is neither pose contribution nor permission to acquire a hand.
inline bool pose_weight_zero(float value) noexcept {
    std::uint32_t bits{};static_assert(sizeof(bits)==sizeof(value));
    std::memcpy(&bits,&value,sizeof(bits));
    return (bits&0x7fffffffu)==0; // Both signed zeros; independent of MXCSR DAZ/FTZ.
}
inline bool custom_child_zero_pose(unsigned known,float weight,float total,float target,
                                   bool /*sequence_playing*/) noexcept {
    return known==31&&pose_weight_zero(weight)&&pose_weight_zero(total)&&pose_weight_zero(target);
}

// Targets are opaque position/Rotator bytes. The caller validates their ABI,
// the object identities and the execution context on every backend operation.
// read/write_target/set_strength report transport success; readback, rather
// than a native return register, establishes the final state.
class Lease {
    struct Channel {Value original{};Target wanted{};bool pending{};};
    std::array<Channel,4> channels_{};
    Strength strength_{};
    bool faulted_{};
    static Strength zero() noexcept {return {{0.f,0.f,0.f}};}
    static bool same(Value const& a,Value const& b) noexcept {
        return exact(a.strength,b.strength)&&exact(a.target,b.target);
    }
    template<class Backend> bool fail(Backend& backend) {
        faulted_=true;restore(backend);return false;
    }
public:
    bool dirty() const noexcept {
        for(auto const& channel:channels_)if(channel.pending)return true;
        return false;
    }
    bool faulted() const noexcept {return faulted_;}
    bool reset() noexcept {
        if(dirty())return false;
        channels_={};strength_={};faulted_=false;return true;
    }
    template<class Backend>
    bool acquire(std::array<Target,4> const& targets,float strength,Backend& backend) {
        if(faulted_||dirty())return fail(backend);
        if(!std::isfinite(strength)||strength<=0.f||strength>1.f)return fail(backend);
        // Finish all four snapshots before the first mutation. Require the
        // exact +0 tuple, since set_strength(0) must restore these same bits.
        for(unsigned i=0;i<channels_.size();++i){
            Value current{};
            if(!backend.read(i,current)||!exact(current.strength,zero()))return fail(backend);
            channels_[i].original=current;channels_[i].wanted=targets[i];
        }
        strength_={{strength,strength,0.f}};
        for(unsigned i=0;i<channels_.size();++i){
            auto& channel=channels_[i];
            channel.pending=true; // A false helper may still have mutated.
            if(!backend.write_target(i,channel.original.target,channel.wanted))return fail(backend);
            Value current{};
            if(!backend.read(i,current)||!exact(current.target,channel.wanted)||
               !exact(current.strength,channel.original.strength))return fail(backend);
            if(!backend.set_strength(i,strength))return fail(backend);
        }
        for(unsigned i=0;i<channels_.size();++i){
            Value current{};
            if(!backend.read(i,current)||!exact(current.target,channels_[i].wanted)||
               !exact(current.strength,strength_))return fail(backend);
        }
        return true;
    }
    // Return whether all journals are clean. A helper failure remains a sticky
    // fault even if readback proves that its write and rollback succeeded.
    template<class Backend> bool restore(Backend& backend) {
        for(unsigned i=0;i<channels_.size();++i){
            auto& channel=channels_[i];
            if(!channel.pending)continue;
            Value current{};
            if(!backend.read(i,current)){faulted_=true;continue;}
            if(same(current,channel.original)){channel.pending=false;continue;}
            // A partial acquire can leave the original target or zero strength.
            // Any other tuple is foreign; leave the whole channel untouched.
            if((!exact(current.target,channel.wanted)&&
                !exact(current.target,channel.original.target))||
               (!exact(current.strength,strength_)&&
                !exact(current.strength,channel.original.strength))){
                faulted_=true;continue;
            }
            const auto expected=current.target;
            if(!backend.set_strength(i,0.f))faulted_=true;
            if(!backend.read(i,current)){faulted_=true;continue;}
            if(same(current,channel.original)){channel.pending=false;continue;}
            if(!exact(current.target,expected)||
               !exact(current.strength,channel.original.strength)){
                faulted_=true;continue;
            }
            if(!backend.write_target(i,expected,channel.original.target))faulted_=true;
            if(!backend.read(i,current)||!same(current,channel.original)){
                faulted_=true;continue;
            }
            channel.pending=false;
        }
        return !dirty();
    }
};
} // namespace gears_hand_ik_stream
