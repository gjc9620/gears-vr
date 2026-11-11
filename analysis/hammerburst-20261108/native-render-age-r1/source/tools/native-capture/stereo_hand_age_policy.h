#pragma once
#include "../vr-common/stereo_frame.h"
namespace gears_stereo_hand_age {
// Decides ONLY whether to keep logical registration/debt for a future fresh
// Scene. It supplies no pose, Frame, input, lease, clock or dispatch authority.
inline bool preserve(gears_vr::StereoSample const& sample,std::int64_t now,
    gears_vr::StereoResult const& result,gears_vr::ReferenceContinuity before,
    gears_vr::ReferenceContinuity after) noexcept {
    return result.status==gears_vr::StereoStatus::stale&&sample.focused==1&&
        before.epoch&&before.epoch==after.epoch&&before.stable&&after.stable&&
        !before.pending&&!after.pending&&!before.exhausted&&!after.exhausted&&
        sample.received_ns>0&&now>=sample.received_ns&&now-sample.received_ns>100000000;
}
}
