#pragma once
#include <openxr/openxr.h>

namespace gears_vr {

// Evidence only: does not call OpenXR or mutate the caller's running flag.
struct SessionExitState {
    bool stopping_observed{};
    bool end_succeeded{};
    bool terminal_exiting{};
    bool loss_observed{};
    XrSessionState last_state{XR_SESSION_STATE_UNKNOWN};

    void observe(XrSessionState state) noexcept {
        last_state = state;
        if (state == XR_SESSION_STATE_STOPPING) stopping_observed = true;
        if (state == XR_SESSION_STATE_EXITING) terminal_exiting = true;
        if (state == XR_SESSION_STATE_LOSS_PENDING) loss_observed = true;
    }

    void mark_end_success() noexcept { end_succeeded = true; }

    bool complete(bool objects_destroyed, bool running, bool gpu_unsafe) const noexcept {
        return objects_destroyed && !gpu_unsafe && !loss_observed &&
               (!running || terminal_exiting);
    }

    bool exited_without_end() const noexcept {
        return terminal_exiting && !end_succeeded;
    }
};

} // namespace gears_vr
