#pragma once
#include "controller_continuity_frame.h"
#include "stereo_frame.h"
// Lancer's already-established main hold is not a held-button gesture.
// This pin proves only a live same-publisher/cohort during a missing pose.
// It creates no Frame, pose, press, part lease, equipment or firing action.
namespace gears_lancer_main_hold {
class TrackingPin {
    gears_hand_continuity::Publisher publisher_{};
    gears_hand_continuity::TrackedInput last_{};bool bound_{};
public:
    void reset() noexcept {*this={};}
    bool refresh(gears_vr::StereoSample const& stereo,gears_hand_continuity::TrackedInput const& latest,
        gears_hand_continuity::Publisher publisher,std::int64_t now,bool allow_sim) noexcept {
        auto const& c=latest.input;auto const& p=last_.input;
        if(!bound_||!publisher.pid||!publisher.start||publisher.pid!=publisher_.pid||publisher.start!=publisher_.start||
           !gears_hand_continuity::tracked_input_shape_valid(latest)||latest.stream_epoch!=last_.stream_epoch||
           latest.tracking_epoch[0]!=last_.tracking_epoch[0]||latest.tracking_epoch[1]!=last_.tracking_epoch[1]||
           c.source!=p.source||c.session!=p.session||c.space_epoch!=p.space_epoch||c.input_epoch!=p.input_epoch||
           c.profile_generation!=p.profile_generation||c.sequence<p.sequence||c.sync_ns<p.sync_ns||
           (c.sequence==p.sequence&&!gears_hand_continuity::same_input(c,p))||
           !gears_vr::stereo_geometry_valid(stereo)||!gears_vr::stereo_render_active(stereo)||
           !gears_vr::stereo_tracking_valid(stereo,allow_sim)||!gears_vr::controller_frame_matches(c,stereo,now,allow_sim)||
           now<stereo.received_ns||now-stereo.received_ns>100000000||now<c.sync_ns||now-c.sync_ns>100000000||
           !gears_vr::controller_pose_usable(c.hands[0].grip,c.hands[0].grip_flags,c.hands[0].grip_active)||
           !gears_vr::controller_pose_usable(c.hands[1].grip,c.hands[1].grip_flags,c.hands[1].grip_active))return false;
        last_=latest;return true;
    }
    bool bind(gears_vr::StereoSample const& stereo,gears_hand_continuity::TrackedInput const& input,
        gears_hand_continuity::Publisher publisher,std::int64_t now,bool allow_sim) noexcept {
        reset();publisher_=publisher;last_=input;bound_=true;
        if(refresh(stereo,input,publisher,now,allow_sim))return true;
        reset();return false;
    }
};
}

