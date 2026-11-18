// Included in longshot_clip. Validate stopped/uncertain time BEFORE capture
// can update last. Cleanup only stops the exact owned clip and never uses this
// observation to grant a seek, physical endpoint, ammunition or reward.
struct UiStartReceipt {std::uint64_t serial{};float before{},after{};bool pending{};};
UiStartReceipt ui_start_receipt;
void report_ui_start(unsigned phase,bool accepted,UiStartReceipt const& receipt) noexcept {
    static unsigned rows{};if(!log_file||rows>=8)return;++rows;
    const auto& ui=lancerreload_state::lease;const auto& owner=anim::ownership;
    // Existing local journal/cache values only, never a new node observation.
    fwprintf(log_file,L"longshot_ui_start phase=%u accepted=%u serial=%llu ui_serial=%llu expected_before=%.9g expected_after=%.9g ui_before=%.9g ui_wanted=%.9g ui_start=%.9g reward_start=%.9g cache_start=%.9g cache_time=%.9g stopped=%u seek_pending=%u node_values_are_cached=1\n",
        phase,unsigned(accepted),receipt.serial,ui.serial,double(receipt.before),double(receipt.after),
        double(ui.ui_before[5]),double(ui.ui_wanted[5]),double(ui.original_start),double(ui.reward_start),
        double(owner.last.reload_start),double(owner.last.time),unsigned(owner.stopped),unsigned(owner.seek_pending));fflush(log_file);
}
// Called only after the owning HUD setter returned success. Its journal
// records the exact prior value and the value it wrote/read back. This is
// not permission to accept an arbitrary observed ReloadStartTime change.
bool acknowledge_ui_start(std::uint64_t serial) noexcept {
    const auto& ui=lancerreload_state::lease;
    const auto& owner=anim::ownership;
    if(!owned()||serial!=owned_serial||ui_start_receipt.pending||!ui.acquired||ui.serial!=serial||
       !ui.ui_owned||!(ui.identity==owned_identity)||!owner.stopped||owner.seek_pending||
       !std::isfinite(ui.ui_before[5])||!std::isfinite(ui.ui_wanted[5])||
       owner.last.reload_start!=ui.ui_before[5]||ui.original_start!=ui.ui_wanted[5]||
       ui.reward_start!=ui.ui_wanted[5]){report_ui_start(1,false,{serial,ui.ui_before[5],ui.ui_wanted[5],false});return false;}
    ui_start_receipt={serial,ui.ui_before[5],ui.ui_wanted[5],true};report_ui_start(0,true,ui_start_receipt);return true;
}
bool clock_current_for_capture(bool cleanup) noexcept {
    if(cleanup)return true;
    if(ui_start_receipt.pending){
        const auto& ui=lancerreload_state::lease;const auto saved=anim::ownership.last;
        if(!owned()||ui_start_receipt.serial!=owned_serial||!ui.acquired||!ui.ui_owned||
           ui.serial!=owned_serial||!(ui.identity==owned_identity)||!anim::ownership.stopped||anim::ownership.seek_pending||
           saved.reload_start!=ui_start_receipt.before||ui.ui_before[5]!=ui_start_receipt.before||
           ui.ui_wanted[5]!=ui_start_receipt.after||ui.original_start!=ui_start_receipt.after||
           ui.reward_start!=ui_start_receipt.after){report_ui_start(3,false,ui_start_receipt);return false;}
        // Reconcile still checks actual node time/rate/play/loop and every
        // read_owned identity/link. Only this acknowledged HUD-owned field's
        // expected value changes; failure restores the prior receipt.
        anim::ownership.last.reload_start=ui_start_receipt.after;
        if(!anim::reconcile_seek()){anim::ownership.last=saved;report_ui_start(4,false,ui_start_receipt);return false;}
        const auto completed=ui_start_receipt;ui_start_receipt={};report_ui_start(2,true,completed);return true;
    }
    if(!anim::ownership.stopped&&!anim::ownership.seek_pending)return true;
    return anim::reconcile_seek();
}
