// Included in longshot_clip. No Play call: the exact original Reloading owner
// already started LS_Reload. Reads may wait for another real current Frame;
// the shared animation service journals and stops that selected node once.
Result try_adopt_original(hand::Frame const& f) noexcept {
    adopt_gate=1;observe_rejection=0;
    if(!pending_original_serial||!(pending_original_identity==inv::identity(f)))return Result::recovery_required;
    if(!nativeequipment::current(nullptr,f)||!lancerreload_state::owns(f,pending_original_serial)||
       !lancerreload_state::magazine_owned(f))return Result::pending;
    if(anim::ownership.active)return Result::recovery_required;
    if(pending_attempt_frame==f.key.frame)return Result::pending;
    pending_attempt_frame=f.key.frame;
    anim::Observation current{};anim::Controls controls{};adopt_gate=2;
    if(!anim::observe_kind(f,current,lancermain_asset::HeldAsset::longshot))return Result::pending;
    adopt_gate=3;if(!original_sequence(f,current))return Result::pending;
    adopt_gate=4;if(!anim::prepare_controls(f,current,controls,true))return Result::pending;
    adopt_gate=5;
    const auto result=anim::acquire_prepared(f,pending_original_serial,current,controls);
    if(result==anim::Acquire::rejected)return Result::pending;
    owned_serial=pending_original_serial;owned_identity=pending_original_identity;
    pending_original_serial=pending_attempt_frame=0;pending_original_identity={};
    neutral_pending=true;
    adopt_gate=result==anim::Acquire::acquired?0u:6u;
    return result==anim::Acquire::acquired?Result::ready:Result::recovery_required;
}
