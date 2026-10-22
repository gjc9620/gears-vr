#pragma once
#include "controller_input_frame.h"
#include <cstdint>

namespace gears_weapon {
enum class OriginalR2Action : std::uint8_t { press,release };
enum class OriginalR2Status : std::uint8_t {
    rejected,baseline,accepted,pressed,released,tapped,suppressed,
    recovered,blocked,abort_required
};
struct OriginalR2Context {
    std::uint64_t world{},controller{},weapon{};
    bool ordinary_player{},game_r2_held{},allow_press{},
         release_on_loss_safe{};
};

// Calls the game's original ButtonPress/Release('R2') exactly at authenticated
// XR edges. The caller must validate the live PC/weapon/world and perform the
// reflected game-thread action. A failed action is never silently committed.
// Weapons whose release can shoot (notably Torque Bow) require a separately
// verified abort before enabling loss-of-tracking recovery.
class OriginalR2Fire {
    std::uint64_t session_{},space_epoch_{},profile_{},input_epoch_{},sequence_{};
    std::uint32_t presses_{},releases_{};
    std::uint64_t world_{},controller_{},weapon_{};
    bool owned_{},suppressed_{},release_safe_{},faulted_{},seal_next_{},await_release_{};
    void baseline(gears_vr::ControllerInputFrame const& frame,
                  OriginalR2Context const& context) noexcept {
        session_=frame.session;space_epoch_=frame.space_epoch;
        profile_=frame.profile_generation;input_epoch_=frame.input_epoch;
        sequence_=frame.sequence;
        presses_=frame.hands[1].pressed[gears_vr::control_trigger];
        releases_=frame.hands[1].released[gears_vr::control_trigger];
        world_=context.world;controller_=context.controller;weapon_=context.weapon;
        if(!owned_)release_safe_=context.release_on_loss_safe;
        const auto& hand=frame.hands[1];
        if(await_release_&&context.allow_press&&hand.trigger_active==1&&
           hand.trigger<=.35f&&!context.game_r2_held){await_release_=false;suppressed_=false;}
    }
    bool same(OriginalR2Context const& c) const noexcept {
        return c.world==world_&&c.controller==controller_&&c.weapon==weapon_&&
            c.ordinary_player;
    }
    void reset_input() noexcept {
        const bool waiting=await_release_;
        const auto world=world_,controller=controller_,weapon=weapon_;
        reset();await_release_=waiting;seal_next_=waiting;
        // Input loss must not make a previously authenticated owner cold.
        world_=world;controller_=controller;weapon_=weapon;
    }
public:
    void reset() noexcept {*this=OriginalR2Fire{};}
    bool owns_press() const noexcept {return owned_;}
    std::uint64_t owned_controller() const noexcept {return owned_?controller_:0;}

    // Synchronous hand-back, independent of XR freshness. The adapter never
    // releases a never-owned/suppressed game press. Context cannot distinguish
    // an external source joining AFTER our press; exclusive source ownership
    // remains a caller requirement, not a fact proved by this ledger.
    // Successful hand-back seals old edges. While allow_press is false, only
    // baselines advance. Re-enable needs a new baseline, then a distinct new
    // release unless a fresh active trigger <= .35 and game key up already
    // prove release. That baseline never replays an edge. Do not reset to re-arm.
    // Before the first input baseline, a valid ordinary context may only seal
    // this gate; it cannot release a game key or bypass the first baseline.
    template<class Execute>
    OriginalR2Status yield(OriginalR2Context const& context,Execute&& execute) {
        if(faulted_)return OriginalR2Status::abort_required;
        const bool cold=!owned_&&!session_&&!world_&&!controller_&&!weapon_;
        if(!context.world||!context.controller||!context.weapon||!context.ordinary_player||
           (!cold&&!same(context))){
            faulted_=true;return OriginalR2Status::abort_required;
        }
        const bool owned=owned_;
        if(owned_){
            if(!release_safe_||!context.release_on_loss_safe||
               (context.game_r2_held&&!execute(OriginalR2Action::release,controller_))){
                faulted_=true;return OriginalR2Status::abort_required;
            }
            owned_=false;
        }
        seal_next_=await_release_=true;
        if(cold)return OriginalR2Status::accepted;
        if(owned)return context.game_r2_held?OriginalR2Status::released:OriginalR2Status::recovered;
        return suppressed_||context.game_r2_held?OriginalR2Status::suppressed:OriginalR2Status::accepted;
    }

    template<class Execute>
    OriginalR2Status advance(gears_vr::ControllerInputFrame const& frame,
                             gears_vr::StereoSample const& stereo,
                             OriginalR2Context const& context,
                             std::int64_t now_ns,Execute&& execute,
                             bool allow_sim=false) {
        if(faulted_)return OriginalR2Status::abort_required;
        if(owned_&&!context.allow_press)return yield(context,execute);
        const bool valid=gears_vr::controller_frame_matches(frame,stereo,now_ns,allow_sim)&&
            gears_vr::controller_pose_usable(frame.hands[1].grip,
                frame.hands[1].grip_flags,frame.hands[1].grip_active)&&
            context.world&&context.controller&&context.weapon&&context.ordinary_player;
        if(!valid||!same(context)||
           (session_&&(session_!=frame.session||space_epoch_!=frame.space_epoch||
               profile_!=frame.profile_generation||input_epoch_!=frame.input_epoch))){
            if(owned_){
                if(!same(context)||!release_safe_||!context.release_on_loss_safe||
                   !execute(OriginalR2Action::release,controller_)){
                    faulted_=true;
                    return OriginalR2Status::abort_required;
                }
                reset_input();return OriginalR2Status::recovered;
            }
            reset_input();
            if(!valid)return OriginalR2Status::rejected;
        }
        if(!session_){baseline(frame,context);seal_next_=await_release_&&!context.allow_press;
            return OriginalR2Status::baseline;}
        if(frame.sequence<=sequence_)return OriginalR2Status::rejected;
        if(await_release_&&!context.allow_press){
            baseline(frame,context);seal_next_=true;return OriginalR2Status::blocked;
        }
        if(seal_next_){baseline(frame,context);seal_next_=false;return OriginalR2Status::baseline;}
        if(owned_&&!context.game_r2_held){
            reset_input();baseline(frame,context);return OriginalR2Status::recovered;
        }
        const auto p=frame.hands[1].pressed[gears_vr::control_trigger];
        const auto r=frame.hands[1].released[gears_vr::control_trigger];
        if(p<presses_||r<releases_||p-presses_>1||r-releases_>1){
            if(owned_){
                if(!release_safe_||!context.release_on_loss_safe||
                   !execute(OriginalR2Action::release,controller_)){
                    faulted_=true;
                    return OriginalR2Status::abort_required;
                }
                reset_input();return OriginalR2Status::recovered;
            }
            baseline(frame,context);return OriginalR2Status::baseline;
        }
        const bool down=p>presses_,up=r>releases_;
        if(await_release_){
            // A combined missed tap cannot prove edge ordering. Consume it;
            // only a distinct release edge or active up state can re-arm.
            if(up&&!down){await_release_=false;suppressed_=false;}
            baseline(frame,context);
            return up&&!down?OriginalR2Status::accepted:OriginalR2Status::blocked;
        }
        if(!down&&!up){baseline(frame,context);return OriginalR2Status::accepted;}
        // If a game tick missed both edges, the prior ownership determines
        // their order: idle => press/release; owned => release/press.
        bool game_held=context.game_r2_held;
        auto press=[&]()->OriginalR2Status {
            if(owned_)return OriginalR2Status::accepted;
            if(suppressed_)return OriginalR2Status::suppressed;
            if(!context.allow_press)return OriginalR2Status::blocked;
            if(game_held){suppressed_=true;return OriginalR2Status::suppressed;}
            if(!execute(OriginalR2Action::press,controller_)){
                faulted_=true;return OriginalR2Status::abort_required;
            }
            owned_=true;game_held=true;release_safe_=context.release_on_loss_safe;
            return OriginalR2Status::pressed;
        };
        auto release=[&]()->OriginalR2Status {
            if(suppressed_){suppressed_=false;return OriginalR2Status::suppressed;}
            if(!owned_)return OriginalR2Status::accepted;
            if(!execute(OriginalR2Action::release,controller_)){
                faulted_=true;return OriginalR2Status::abort_required;
            }
            owned_=false;game_held=false;return OriginalR2Status::released;
        };
        OriginalR2Status result{};
        if(down&&up){
            if(owned_){const auto first=release();
                if(first==OriginalR2Status::released){
                    const auto second=press();result=second==OriginalR2Status::pressed?
                        OriginalR2Status::tapped:second;
                }else result=first;
            }else{const auto first=press();
                if(first==OriginalR2Status::pressed){
                    const auto second=release();result=second==OriginalR2Status::released?
                        OriginalR2Status::tapped:second;
                }else{
                    if(first==OriginalR2Status::suppressed)release();
                    result=first;
                }
            }
        }else result=down?press():release();
        if(!faulted_)baseline(frame,context);
        return result;
    }
};
}
