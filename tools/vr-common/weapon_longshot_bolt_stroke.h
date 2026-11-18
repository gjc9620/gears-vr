#pragma once
#include "weapon_longshot_bolt_path.h"
#include "weapon_longshot_reload.h"
#include "controller_relative_motion.h"

namespace gears_weapon::longshot_tracks {
// Current physical input and separately measured native-bone agreement. The
// adapter authenticates the lease and computes the bone error against the
// last requested curve pose. Animation time alone cannot acknowledge travel.
struct BoltSample {
    LongshotReloadStamp stamp{};
    gears_vr::Vector hand_in_root_m{};
    std::uint64_t frame{},lease{},path_generation{};
    double actual_progress{},position_error_m{},orientation_error_degrees{};
    bool grip_down{},lease_current{},new_grant{},bone_measured{};
};
enum class BoltDirection {opening,closing};
enum class BoltResult {rejected,waiting,moved,complete,interrupted};
class BoltStroke {
    LongshotReloadIdentity identity_{};
    gears_vr::Vector hand_begin_{},handle_begin_{},last_hand_{};
    std::uint64_t sequence_{},frame_{},issued_frame_{},lease_{},generation_{};
    std::int64_t observed_ns_{};
    double wanted_{};
    BoltDirection direction_{};
    bool active_{},complete_{};
    static bool native_valid(BoltSample const& s) noexcept {
        return s.bone_measured&&std::isfinite(s.actual_progress)&&s.actual_progress>=0&&s.actual_progress<=1&&
            std::isfinite(s.position_error_m)&&s.position_error_m>=0&&s.position_error_m<=.002&&
            std::isfinite(s.orientation_error_degrees)&&s.orientation_error_degrees>=0&&s.orientation_error_degrees<=2;
    }
    static bool usable(BoltSample const& s,std::int64_t now) noexcept {
        return s.stamp.valid(now)&&s.stamp.weapon_owned&&s.stamp.right_tracked&&s.stamp.left_tracked&&
            s.frame&&s.lease&&s.path_generation&&s.grip_down&&s.lease_current&&
            gears_vr::finite(s.hand_in_root_m);
    }
public:
    bool begin(BoltPath const& path,BoltSample const& s,BoltDirection direction,
               gears_vr::Vector handle,std::int64_t now) noexcept {
        if(active_||!path.bound()||!usable(s,now)||!s.new_grant||!native_valid(s)||!gears_vr::finite(handle))return false;
        const auto delta=gears_vr::Vector{s.hand_in_root_m.x-handle.x,s.hand_in_root_m.y-handle.y,s.hand_in_root_m.z-handle.z};
        if(std::hypot(std::hypot(delta.x,delta.y),delta.z)>.05)return false;
        double fit{};if(!path.fit(handle,s.actual_progress,.03,.002,fit)||std::abs(fit-s.actual_progress)>.025)return false;
        identity_=s.stamp.identity;hand_begin_=s.hand_in_root_m;handle_begin_=handle;
        sequence_=s.stamp.sequence;frame_=issued_frame_=s.frame;lease_=s.lease;generation_=s.path_generation;
        observed_ns_=s.stamp.observed_ns;last_hand_=s.hand_in_root_m;wanted_=s.actual_progress;direction_=direction;active_=true;complete_=false;
        return true;
    }
    BoltResult observe(BoltPath const& path,BoltSample const& s,std::int64_t now) noexcept {
        if(!active_)return BoltResult::rejected;
        if(!path.bound()||!usable(s,now)||!(s.stamp.identity==identity_)||s.lease!=lease_||
           s.path_generation!=generation_||s.stamp.observed_ns<observed_ns_||
           s.stamp.observed_ns-observed_ns_>100000000){active_=false;return BoltResult::interrupted;}
        if(s.stamp.sequence<=sequence_||s.frame<=frame_)return BoltResult::waiting;
        const auto dt=double(s.stamp.observed_ns-observed_ns_)/1e9;
        sequence_=s.stamp.sequence;frame_=s.frame;observed_ns_=s.stamp.observed_ns;last_hand_=s.hand_in_root_m;
        const gears_vr::Vector point{handle_begin_.x+s.hand_in_root_m.x-hand_begin_.x,
            handle_begin_.y+s.hand_in_root_m.y-hand_begin_.y,handle_begin_.z+s.hand_in_root_m.z-hand_begin_.z};
        // A real wrist can be offset within the acquisition sphere. Anchor
        // its subsequent displacement at the actual handle, preserving the
        // whole curved travel rather than teleporting to a new input pose.
        double next{};
        const auto maximum=(std::min)(1.,(1.5*dt+.002)/path.travel_m());
        if(!path.fit(point,wanted_,maximum,.015,next)){active_=false;return BoltResult::interrupted;}
        const auto distance=std::abs(next-wanted_);
        if(distance>.0001){wanted_=next;issued_frame_=s.frame;complete_=false;return BoltResult::moved;}
        const double target=direction_==BoltDirection::opening?1.:0.;
        if(std::abs(wanted_-target)<=.015&&s.frame>issued_frame_&&native_valid(s)&&
           std::abs(s.actual_progress-target)<=.015){active_=false;complete_=true;return BoltResult::complete;}
        return BoltResult::waiting;
    }
    // Original producer points can certify motion missed by GT scheduling.
    // They never become synthetic Frames, leases, seeks or endpoint receipts.
    // Only this current, fresh sample may issue movement or acknowledge bones.
    BoltResult observe_continuous(BoltPath const& path,BoltSample const& s,
        gears_controller_relative_motion::Segment const& segment,std::int64_t now) noexcept {
        if(!active_)return BoltResult::rejected;
        if(s.stamp.observed_ns>=observed_ns_&&s.stamp.observed_ns-observed_ns_<=100000000)
            return observe(path,s,now);
        const auto fail=[&](){active_=false;return BoltResult::interrupted;};
        if(!path.bound()||!usable(s,now)||!(s.stamp.identity==identity_)||s.lease!=lease_||
           s.path_generation!=generation_||s.stamp.sequence<=sequence_||s.frame<=frame_||
           s.stamp.observed_ns<=observed_ns_||segment.count<2||segment.count>segment.points.size())return fail();
        const auto& first=segment.points[0];const auto& last=segment.points[segment.count-1];
        const auto close_position=[](gears_vr::Vector a,gears_vr::Vector b){return gears_vr::finite(a)&&gears_vr::finite(b)&&
            std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z)<=.00025;};
        if(first.sequence!=sequence_||first.sync_ns!=observed_ns_||!close_position(first.position,last_hand_)||
           last.sequence!=s.stamp.sequence||last.sync_ns!=s.stamp.observed_ns||!close_position(last.position,s.hand_in_root_m))return fail();
        double next=wanted_;
        for(unsigned i=1;i<segment.count;++i){auto const& a=segment.points[i-1];auto const& b=segment.points[i];
            if(a.sequence==UINT64_MAX||b.sequence!=a.sequence+1||b.sync_ns<=a.sync_ns||b.sync_ns-a.sync_ns>100000000||
               !gears_vr::finite(b.position))return fail();
            // Use exact physical endpoints for the two measured GT samples;
            // intermediate values are relative motion facts, never bone data.
            const auto physical=i+1==segment.count?s.hand_in_root_m:b.position;
            const gears_vr::Vector point{handle_begin_.x+physical.x-hand_begin_.x,
                handle_begin_.y+physical.y-hand_begin_.y,handle_begin_.z+physical.z-hand_begin_.z};
            const double dt=double(b.sync_ns-a.sync_ns)/1e9;
            const auto maximum=(std::min)(1.,(1.5*dt+.002)/path.travel_m());
            double progress{};if(!path.fit(point,next,maximum,.015,progress))return fail();next=progress;
        }
        // More producer points must not multiply the original per-observation
        // 2mm allowance into a larger net displacement permission.
        const auto elapsed=double(s.stamp.observed_ns-observed_ns_)/1e9;
        const auto net_limit=(std::min)(1.,(1.5*elapsed+.002)/path.travel_m());
        if(std::abs(next-wanted_)>net_limit)return fail();
        sequence_=s.stamp.sequence;frame_=s.frame;observed_ns_=s.stamp.observed_ns;last_hand_=s.hand_in_root_m;
        if(std::abs(next-wanted_)>.0001){wanted_=next;issued_frame_=s.frame;complete_=false;return BoltResult::moved;}
        const double target=direction_==BoltDirection::opening?1.:0.;
        if(std::abs(wanted_-target)<=.015&&s.frame>issued_frame_&&native_valid(s)&&
           std::abs(s.actual_progress-target)<=.015){active_=false;complete_=true;return BoltResult::complete;}
        return BoltResult::waiting;
    }
    void release() noexcept {active_=false;complete_=false;}
    bool active() const noexcept {return active_;}
    bool completed() const noexcept {return complete_;}
    double wanted_progress() const noexcept {return wanted_;}
    // Read-only diagnostic stamp; never supplies continuity or action permission.
    std::int64_t last_observed_ns() const noexcept {return observed_ns_;}
};
}
