namespace restore_control {
enum class ResumeResult {waiting,advanced,ready,recovery_required};
hand::LeaseKind resume_part() noexcept {
    return parked_plan.resume_stage==Stage::waiting_ammo?hand::LeaseKind::ammo:hand::LeaseKind::magazine;
}
bool finish_resume(hand::Frame const& source,hand::Decision const& decision) noexcept {
    if(!fresh_restore_main(source)||!geometry_current(source)||!(inv::identity(source)==opening.identity))return false;
    resume_grant_ready=granted(source,decision,resume_part())&&hand::same(held,decision.lease);
    if(!resume_grant_ready)held={};
    released=false;stroke.reset();insertion.reset();extraction.reset();
    stage=parked_plan.resume_stage;restore_resume_step=0;
    log_restore("resumed",&source);return true;
}
ResumeResult resume(hand::Frame const& source,hand::Decision const& decision) noexcept {
    if(stage!=Stage::restoring||!restore_display_ready||!parked_plan.valid||restoration_failed)return ResumeResult::waiting;
    if(!fresh_restore_main(source)||!geometry_current(source)||!(inv::identity(source)==opening.identity)){
        released=false;return ResumeResult::waiting;
    }
    if(visual::active&&!hand::same(visual::saved.weapon,opening.identity.weapon))return ResumeResult::waiting;
    if(restore_resume_step==0&&(!anim::ownership.active||!anim::ownership.visual_only||
       anim::ownership.serial!=animation_serial()||!(anim::ownership.inventory==opening.identity)||
       lancerreload_state::lease.acquired))return ResumeResult::waiting;
    if(restore_resume_step&&(!lancerreload_state::lease.acquired||lancerreload_state::lease.serial!=serial||
       !(lancerreload_state::lease.identity==opening.identity)))return ResumeResult::recovery_required;
    auto verification=observer;inv::Inventory resume_counts{};
    if(!verification.sample(source,resume_counts))return ResumeResult::waiting;
    if(!inv::counts_equal(resume_counts,transaction.inventory()))return ResumeResult::recovery_required;
    if(restore_resume_step==0){
        if(!source.left_tracked||!source.left_grip_known){released=false;return ResumeResult::waiting;}
        if(!source.left_grip_down){if(!released)log_restore("up_baseline",&source);released=true;held={};return ResumeResult::waiting;}
        if(!released||!granted(source,decision,resume_part(),true)||
           source.press_token<=old_item||source.press_token<=restore_resume_intent)return ResumeResult::waiting;
        auto prepared_transaction=transaction;
        if(prepared_transaction.stage()==gears_weapon::ReloadTxnStage::interrupted&&
           !prepared_transaction.resume(resume_counts,source.press_token,serial))return ResumeResult::waiting;
        if(prepared_transaction.stage()!=gears_weapon::ReloadTxnStage::open&&
           prepared_transaction.stage()!=gears_weapon::ReloadTxnStage::closing)return ResumeResult::recovery_required;
        // Consume this genuine edge before any original call. Rejection or
        // expiration cannot retry it on a later retained DOWN publication.
        restore_resume_intent=source.press_token;released=false;
        log_restore("intent",&source);
        const bool needs_native=serial&&!prepared_transaction.feed_present();
        if(needs_native){
            if(lancerreload_state::lease.acquired||parked_native_lease.serial!=serial||
               !(parked_native_lease.identity==opening.identity))return ResumeResult::recovery_required;
            if(!restore_animation()||!visual::restore())return ResumeResult::recovery_required;
            if(!fresh_restore_main(source)){
                restore_display_ready=false;restore_display_step=0;return ResumeResult::waiting;
            }
            std::uint64_t accepted_serial{};bool dispatched{};
            const auto accepted=lancerreload_state::begin(source,accepted_serial,&parked_native_lease,&dispatched);
            if(accepted!=lancerreload_state::BeginResult::acquired||accepted_serial!=serial){
                if(dispatched&&(!lancerreload_state::lease.acquired||lancerreload_state::lease.serial!=serial||
                   !(lancerreload_state::lease.identity==opening.identity)))restore_native_uncertain=true;
                if(dispatched||accepted==lancerreload_state::BeginResult::recovery_required)return ResumeResult::recovery_required;
                restore_display_ready=false;restore_display_step=0;return ResumeResult::waiting;
            }
            native_exited=false;ui_pending=true;log_restore("native_reacquired",&source);
        }
        transaction=prepared_transaction;parked_plan.transaction=prepared_transaction;
        held=decision.lease;resume_grant_ready=false;
        if(!needs_native)return finish_resume(source,decision)?ResumeResult::ready:ResumeResult::waiting;
        restore_resume_step=1;return ResumeResult::advanced;
    }
    restoration_admission::Scope permit(source);
    if(!permit)return ResumeResult::waiting;
    if(restore_resume_step==1){
        if(anim::ownership.active)return ResumeResult::recovery_required;
        const auto acquired=anim::acquire(source,serial);
        if(acquired==anim::Acquire::recovery_required)return ResumeResult::recovery_required;
        if(acquired!=anim::Acquire::acquired)return ResumeResult::waiting;
        restore_resume_step=2;return ResumeResult::advanced;
    }
    if(restore_resume_step==2){
        if(!anim::seek_latch_angle(source,animation_serial(),angle))
            return anim::ownership.seek_pending?ResumeResult::recovery_required:ResumeResult::waiting;
        restore_resume_step=3;return ResumeResult::advanced;
    }
    if(restore_resume_step!=3)return ResumeResult::recovery_required;
    if(!parked_plan.original_at_feed&&!transaction.feed_present()){
        if(!visual::active&&!visual::begin(source,visual::Profile::hammerburst))
            return visual::active||!visual::begin_retryable?ResumeResult::recovery_required:ResumeResult::waiting;
        if(!visual::stow())return ResumeResult::recovery_required;
    }
    return finish_resume(source,decision)?ResumeResult::ready:ResumeResult::waiting;
}
}
