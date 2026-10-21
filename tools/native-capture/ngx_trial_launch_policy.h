#pragma once

namespace gears_ngx_trial_launch {
enum class Mode { rejected, background, manual_owned };
struct Flags {
    bool attested{},manual_compiled{},manual_requested{},owned_scene{},final_xr{};
    bool windows_behind{},muted{},observe_only{};
};
constexpr Mode mode(Flags f) {
    if(!f.attested)return Mode::rejected;
    if(f.manual_requested) {
        if(f.manual_compiled&&f.owned_scene&&!f.final_xr&&!f.windows_behind&&!f.muted)
            return Mode::manual_owned;
        return Mode::rejected;
    }
    return f.windows_behind&&f.muted&&f.observe_only?Mode::background:Mode::rejected;
}
}
