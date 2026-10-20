#include "session_exit_state.h"
#include <cstdio>

#define CHECK(expression) do { if (!(expression)) { \
    std::fprintf(stderr, "failed line %d: %s\n", __LINE__, #expression); return 1; \
} } while (false)

int main() {
    using gears_vr::SessionExitState;
    SessionExitState standard;
    CHECK(standard.last_state == XR_SESSION_STATE_UNKNOWN);
    CHECK(!standard.stopping_observed && !standard.end_succeeded);
    CHECK(!standard.terminal_exiting && !standard.loss_observed);
    CHECK(!standard.complete(true, true, false));
    standard.observe(XR_SESSION_STATE_STOPPING);
    CHECK(standard.stopping_observed);
    CHECK(!standard.end_succeeded);
    CHECK(!standard.complete(true, true, false));
    standard.mark_end_success();
    CHECK(standard.end_succeeded);
    CHECK(standard.complete(true, false, false));
    CHECK(!standard.exited_without_end());
    // An end-success marker alone cannot conceal an unchanged running flag.
    CHECK(!standard.complete(true, true, false));

    SessionExitState terminal;
    bool running = true;
    terminal.observe(XR_SESSION_STATE_EXITING);
    CHECK(terminal.last_state == XR_SESSION_STATE_EXITING);
    CHECK(terminal.terminal_exiting && !terminal.stopping_observed);
    CHECK(!terminal.end_succeeded && terminal.exited_without_end());
    CHECK(terminal.complete(true, running, false));
    CHECK(running);
    CHECK(!terminal.complete(false, running, false));
    CHECK(!terminal.complete(true, running, true));

    SessionExitState timeout;
    timeout.observe(XR_SESSION_STATE_FOCUSED);
    CHECK(timeout.last_state == XR_SESSION_STATE_FOCUSED);
    CHECK(!timeout.complete(true, true, false));
    CHECK(!timeout.exited_without_end());

    SessionExitState loss;
    loss.observe(XR_SESSION_STATE_LOSS_PENDING);
    CHECK(loss.loss_observed);
    CHECK(!loss.complete(true, true, false));
    CHECK(!loss.complete(true, false, false));
    loss.observe(XR_SESSION_STATE_EXITING);
    CHECK(loss.last_state == XR_SESSION_STATE_EXITING);
    CHECK(loss.terminal_exiting && loss.loss_observed);
    CHECK(!loss.complete(true, true, false));
    CHECK(!loss.complete(true, false, false));

    // Historical evidence persists while last_state follows each observation.
    standard.observe(XR_SESSION_STATE_IDLE);
    CHECK(standard.last_state == XR_SESSION_STATE_IDLE);
    CHECK(standard.stopping_observed && standard.end_succeeded);
    CHECK(!standard.complete(false, false, false));
    CHECK(!standard.complete(true, false, true));
    standard.observe(XR_SESSION_STATE_EXITING);
    CHECK(!standard.exited_without_end());
    CHECK(standard.complete(true, false, false));

    std::puts("session exit policy CPU checks passed; no OpenXR calls or OS access");
    return 0;
}
