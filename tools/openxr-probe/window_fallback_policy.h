#pragma once
#include "../vr-common/menu_display_state.h"
// Session-owned 2D fallback; it never authenticates a stereo image or pose.
class WindowFallbackPolicy {
    bool active_{};
public:
    static constexpr long long maximum_age_100ns=2500000;
    struct Context {
        bool enabled{},live{},visible{},should_render{},ordinary_wait{},native_layer{},other_layer{};
        gears_menu::Mode mode{gears_menu::Mode::unknown};
    };
    static bool eligible(Context const& c){
        return c.enabled&&c.live&&c.visible&&c.should_render&&c.ordinary_wait&&!c.native_layer&&!c.other_layer&&
            c.mode==gears_menu::Mode::movie_display;
    }
    bool enter(Context const& c){
        if(!eligible(c))return false;
        const bool first=!active_;active_=true;return first;
    }
    bool active()const{return active_;}
    void observe(Context const& c){
        if(!c.enabled||!c.live||c.mode!=gears_menu::Mode::movie_display)active_=false;
    }
    void projection_completed(bool success){if(success)active_=false;}
    void cancel(){active_=false;}
};
