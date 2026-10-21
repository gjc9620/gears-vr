#pragma once
#include "controller_input_frame.h"
#include <cstdint>

namespace gears_weapon {
enum class OriginalL2Action : std::uint8_t { press, release };
enum class OriginalL2Status : std::uint8_t {
    rejected, baseline, accepted, pressed, released, tapped,
    suppressed, blocked, recovered, abort_required
};
struct OriginalL2Context {
    std::uint64_t world{}, controller{}, weapon{};
    bool ordinary_player{}, game_target_key{}, toggle_targeting{}, allow_press{};
};

// The original hold-to-aim path is WarPC.ButtonPress/Release('L2'). This
// adapter owns only its own left-trigger edge. Toggle-to-aim is deliberately
// excluded: the original release does not cancel a toggled aim state.
class OriginalL2Aim {
    std::uint64_t session_{}, space_epoch_{}, profile_{}, input_epoch_{}, sequence_{};
    std::uint32_t presses_{}, releases_{};
    std::uint64_t world_{}, controller_{}, weapon_{};
    bool owned_{}, suppressed_{}, faulted_{};
    void baseline(gears_vr::ControllerInputFrame const& frame,
                  OriginalL2Context const& context) noexcept {
        session_=frame.session;space_epoch_=frame.space_epoch;
        profile_=frame.profile_generation;input_epoch_=frame.input_epoch;
        sequence_=frame.sequence;
        presses_=frame.hands[0].pressed[gears_vr::control_trigger];
        releases_=frame.hands[0].released[gears_vr::control_trigger];
        world_=context.world;controller_=context.controller;weapon_=context.weapon;
    }
    bool same(OriginalL2Context const& c) const noexcept {
        return c.world==world_&&c.controller==controller_&&c.weapon==weapon_&&
            c.ordinary_player;
    }
public:
    void reset() noexcept {*this=OriginalL2Aim{};}
    bool owns_press() const noexcept {return owned_;}

    template<class Execute>
    OriginalL2Status advance(gears_vr::ControllerInputFrame const& frame,
                             gears_vr::StereoSample const& stereo,
                             OriginalL2Context const& context,
                             std::int64_t now_ns, Execute&& execute,
                             bool allow_sim=false) {
        if(faulted_)return OriginalL2Status::abort_required;
        const bool valid=gears_vr::controller_frame_matches(frame,stereo,now_ns,allow_sim)&&
            gears_vr::controller_pose_usable(frame.hands[0].grip,
                frame.hands[0].grip_flags,frame.hands[0].grip_active)&&
            context.world&&context.controller&&context.weapon&&context.ordinary_player;
        if(!valid||!same(context)||
           (session_&&(session_!=frame.session||space_epoch_!=frame.space_epoch||
               profile_!=frame.profile_generation||input_epoch_!=frame.input_epoch))){
            if(owned_){
                if(!same(context)||context.toggle_targeting||
                   !execute(OriginalL2Action::release,controller_)){
                    faulted_=true;return OriginalL2Status::abort_required;
                }
                reset();return OriginalL2Status::recovered;
            }
            reset();
            if(!valid)return OriginalL2Status::rejected;
        }
        if(!session_){baseline(frame,context);return OriginalL2Status::baseline;}
        if(frame.sequence<=sequence_)return OriginalL2Status::rejected;
        if(owned_&&!context.game_target_key){
            reset();baseline(frame,context);return OriginalL2Status::recovered;
        }
        const auto p=frame.hands[0].pressed[gears_vr::control_trigger];
        const auto r=frame.hands[0].released[gears_vr::control_trigger];
        if(p<presses_||r<releases_||p-presses_>1||r-releases_>1){
            if(owned_){
                if(context.toggle_targeting||
                   !execute(OriginalL2Action::release,controller_)){
                    faulted_=true;return OriginalL2Status::abort_required;
                }
                reset();return OriginalL2Status::recovered;
            }
            baseline(frame,context);return OriginalL2Status::baseline;
        }
        const bool down=p>presses_,up=r>releases_;
        if(!down&&!up){baseline(frame,context);return OriginalL2Status::accepted;}
        bool game_aiming=context.game_target_key;
        auto press=[&]()->OriginalL2Status {
            if(owned_)return OriginalL2Status::accepted;
            if(suppressed_)return OriginalL2Status::suppressed;
            if(context.toggle_targeting||!context.allow_press)return OriginalL2Status::blocked;
            if(game_aiming){suppressed_=true;return OriginalL2Status::suppressed;}
            if(!execute(OriginalL2Action::press,controller_)){
                faulted_=true;return OriginalL2Status::abort_required;
            }
            owned_=true;game_aiming=true;return OriginalL2Status::pressed;
        };
        auto release=[&]()->OriginalL2Status {
            if(suppressed_){suppressed_=false;return OriginalL2Status::suppressed;}
            if(!owned_)return OriginalL2Status::accepted;
            if(context.toggle_targeting||
               !execute(OriginalL2Action::release,controller_)){
                faulted_=true;return OriginalL2Status::abort_required;
            }
            owned_=false;game_aiming=false;return OriginalL2Status::released;
        };
        OriginalL2Status result{};
        if(down&&up){
            if(owned_){
                const auto first=release();
                if(first==OriginalL2Status::released){
                    const auto second=press();result=second==OriginalL2Status::pressed?
                        OriginalL2Status::tapped:second;
                }else result=first;
            }else{
                const auto first=press();
                if(first==OriginalL2Status::pressed){
                    const auto second=release();result=second==OriginalL2Status::released?
                        OriginalL2Status::tapped:second;
                }else{
                    if(first==OriginalL2Status::suppressed)release();
                    result=first;
                }
            }
        }else result=down?press():release();
        if(!faulted_)baseline(frame,context);
        return result;
    }
};
}
