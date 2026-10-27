#pragma once
#include "stereo_frame.h"
#include <cstdint>
#include <mutex>

namespace gears_weapon {
// Carries one L3 press from the authenticated game tick to the next stereo
// ticket. An old request is consumed but never applied after world, XR session
// or LOCAL space changes. Repeated presses before a ticket collapse to one.
class ControllerRecenterRequest {
    std::mutex mutex_;
    std::uint64_t world_{},session_{},space_epoch_{};
    bool pending_{};
public:
    ControllerRecenterRequest()=default;
    ControllerRecenterRequest(ControllerRecenterRequest const&)=delete;
    ControllerRecenterRequest& operator=(ControllerRecenterRequest const&)=delete;
    void reset() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_=false;world_=session_=space_epoch_=0;
    }
    bool submit(std::uint64_t world,std::uint64_t session,std::uint64_t space_epoch) noexcept {
        if(!world||!session||!space_epoch)return false;
        std::lock_guard<std::mutex> lock(mutex_);
        world_=world;session_=session;space_epoch_=space_epoch;pending_=true;
        return true;
    }
    bool consume(std::uint64_t world,std::uint64_t session,std::uint64_t space_epoch) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!pending_)return false;
        pending_=false;
        return world==world_&&session==session_&&space_epoch==space_epoch_;
    }
};

// The caller owns the stereo gate's render-thread lock. The result must come
// from a successful gate update of this exact sample. Replaying the same valid
// sample is allowed by StereoGate and updates the reference for this ticket.
inline bool apply_controller_recenter(ControllerRecenterRequest& request,
        std::uint64_t ticket_world,std::uint64_t committed_world,
        gears_vr::StereoSample const& sample,std::int64_t sample_time,bool allow_sim,
        gears_vr::StereoGate& gate,gears_vr::StereoResult& accepted) {
    if(ticket_world!=committed_world||!accepted||!gears_vr::stereo_same(sample,accepted.sample) ||
       !request.consume(ticket_world,sample.session,sample.space_epoch))return false;
    if(accepted.status!=gears_vr::StereoStatus::initial){
        gate.request_recenter();
        accepted=gate.update(sample,sample_time,allow_sim);
    }
    return true;
}
}
