// Cosmetic restoration only. Each dispatched operation retains its existing
// animation/visual receipt. Neither an input edge nor inventory is invented.
namespace restore_control {
enum class DisplayResult {waiting,advanced,ready,recovery_required};
DisplayResult display(hand::Frame const& source) noexcept {
    if(stage!=Stage::restoring||!fresh_restore_main(source)||!geometry_current(source)||
       !(inv::identity(source)==opening.identity)||lancerreload_state::lease.acquired)return DisplayResult::waiting;
    if(anim::ownership.active&&(!anim::ownership.visual_only||anim::ownership.serial!=animation_serial()||
       !(anim::ownership.inventory==opening.identity)))return DisplayResult::waiting;
    if(visual::active&&!hand::same(visual::saved.weapon,opening.identity.weapon))return DisplayResult::waiting;
    auto verification=observer;inv::Inventory display_counts{};
    if(!verification.sample(source,display_counts))return DisplayResult::waiting;
    if(!inv::counts_equal(display_counts,transaction.inventory()))return DisplayResult::recovery_required;
    restoration_admission::Scope permit(source);
    if(!permit)return DisplayResult::waiting;
    if(restore_display_ready)return DisplayResult::ready;
    if(restore_display_step==0){
        // Reuse the original gun-only stopped clip, including its durable
        // start/stop journal. Never call ForceReload for a cosmetic restore.
        if(anim::ownership.active)return DisplayResult::recovery_required;
        const auto acquired=hammerburst_visual_clip::start(source,animation_serial());
        if(acquired==anim::Acquire::recovery_required)return DisplayResult::recovery_required;
        if(acquired!=anim::Acquire::acquired)return DisplayResult::waiting;
        restore_display_step=1;return DisplayResult::advanced;
    }
    if(!anim::ownership.active||!anim::ownership.stopped)return DisplayResult::recovery_required;
    if(restore_display_step==1){
        if(!anim::seek_latch_angle(source,animation_serial(),angle))
            return anim::ownership.seek_pending?DisplayResult::recovery_required:DisplayResult::waiting;
        restore_display_step=2;return DisplayResult::advanced;
    }
    if(restore_display_step!=2)return DisplayResult::recovery_required;
    if(!parked_plan.original_at_feed&&!transaction.feed_present()){
        // Physically removed magazine stays absent. The same owned display
        // is hidden at its original attachment, not placed in a phantom hand.
        if(!visual::active&&!visual::begin(source,visual::Profile::hammerburst))
            return visual::active||!visual::begin_retryable?DisplayResult::recovery_required:DisplayResult::waiting;
        if(!visual::stow())return DisplayResult::recovery_required;
    }
    if(!restoration_owned(source))return DisplayResult::waiting;
    restore_display_ready=true;restore_display_step=3;
    held={};released=false;return DisplayResult::ready;
}
}
