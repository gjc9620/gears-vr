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
    bool owned_{},suppressed_{},release_safe_{},faulted_{};
    void baseline(gears_vr::ControllerInputFrame const& frame,
                  OriginalR2Context const& context) noexcept {
        session_=frame.session;space_epoch_=frame.space_epoch;
        profile_=frame.profile_generation;input_epoch_=frame.input_epoch;
        sequence_=frame.sequence;
        presses_=frame.hands[1].pressed[gears_vr::control_trigger];
        releases_=frame.hands[1].released[gears_vr::control_trigger];
        world_=context.world;controller_=context.controller;weapon_=context.weapon;
        release_safe_=context.release_on_loss_safe;
    }
    bool same(OriginalR2Context const& c) const noexcept {
        return c.world==world_&&c.controller==controller_&&c.weapon==weapon_&&
            c.ordinary_player;
    }
public:
    void reset() noexcept {*this=OriginalR2Fire{};}
    bool owns_press() const noexcept {return owned_;}
    std::uint64_t owned_controller() const noexcept {return owned_?controller_:0;}

    template<class Execute>
    OriginalR2Status advance(gears_vr::ControllerInputFrame const& frame,
                             gears_vr::StereoSample const& stereo,
                             OriginalR2Context const& context,
                             std::int64_t now_ns,Execute&& execute,
                             bool allow_sim=false) {
        if(faulted_)return OriginalR2Status::abort_required;
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
                reset();return OriginalR2Status::recovered;
            }
            reset();
            if(!valid)return OriginalR2Status::rejected;
        }
        if(!session_){baseline(frame,context);return OriginalR2Status::baseline;}
        if(frame.sequence<=sequence_)return OriginalR2Status::rejected;
        if(owned_&&!context.game_r2_held){
            reset();baseline(frame,context);return OriginalR2Status::recovered;
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
                reset();return OriginalR2Status::recovered;
            }
            baseline(frame,context);return OriginalR2Status::baseline;
        }
        const bool down=p>presses_,up=r>releases_;
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
