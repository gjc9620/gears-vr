// One owned UI preparation shared by initial and restored reloads. On resume,
// apply the preserved reward clock BEFORE observing/stopping the gun clip;
// its strict ReloadStartTime receipt must not be changed by our later UI setup.
bool apply_pending_ui(hand::Frame const& source) {
    if(!ui_pending)return true;
    const auto display=opening.loaded?opening_policy.tactical_ms:opening_policy.empty_ms;
    const auto window=opening_policy.deadline(opening.loaded==0,lancerreload_state::lease.perfect_end_ms);
    if(!lancerreload_state::set_ui_timing(source,serial,display,window,opening_policy.mode!=gears_weapon::LancerPerfectMode::original)){
        if(!(lancerreload_state::lease.ui_owned&&lancerreload_state::lease.attempts_owned&&lancerreload_state::ui_setup_gate>=6))return false;
    }
    lancerreload_state::lease.perfect_end_ms=window;ui_pending=false;return true;
}
