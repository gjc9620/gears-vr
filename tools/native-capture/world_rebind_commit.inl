// No engine calls, IO, GPU waits or queued writer inside a closed world gate.
bool world_old_state_retired(){
    const auto life=submitted_lifetime.snapshot();
    if(life.generation!=committed_world_generation.load()||!life.quiescing||!life.closed||life.pending||!life.claimed||
       life.stop_requested||life.permanent_closed||submit_retained.load())return false;
    if(!state_created.load())return !state_attempted.load()&&!state_temporary.load();
    return state_release_started.load()==1&&state_release_returned.load()==1&&state_delete_entered.load()==1&&
        state_destroyed.load()==1&&(state_delete_flags&1)&&submit_sources_released.load()==1;
}
enum class PendingFresh {fresh,transient,stale};
PendingFresh pending_world_fresh(){
    if(!pending_world)return PendingFresh::stale;auto const& pending=*pending_world;
    if(pending.metadata.previous_generation!=committed_world_generation.load()||
       pending.metadata.generation!=pending.metadata.previous_generation+1)return PendingFresh::stale;
    gears_anchor::Anchor anchor{};std::uint32_t stamp{};gears_anchor::GuardDiagnostic diagnostic{};
    if(!gears_anchor::strict_guard(pending.anchor,safe_copy,anchor,stamp,&diagnostic))
        return diagnostic.timestamp_only?PendingFresh::transient:PendingFresh::stale;
    if(!gears_ui::validate(pending.ui,safe_copy,false))return PendingFresh::stale;
#if defined(GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER) && GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
    if(crosshair_live::enabled&&(!pending.crosshair||!crosshair_live::recovery_fresh(*pending.crosshair,pending.anchor,pending.ui)))return PendingFresh::stale;
#endif
    for(auto const& binding:pending.ui.bindings){std::array<unsigned char,16> entry{};
        if(binding.rva>UINTPTR_MAX-pending.ui.base||!read_attested_hook_entry(entry.data(),
            reinterpret_cast<void const*>(pending.ui.base+binding.rva),entry.size())||entry!=binding.bytes)return PendingFresh::stale;
    }
    if(gears_anchor::strict_guard(pending.anchor,safe_copy,anchor,stamp,&diagnostic))return PendingFresh::fresh;
    return diagnostic.timestamp_only?PendingFresh::transient:PendingFresh::stale;
}
void reset_retired_view_state(){
    state_attempted=state_created=state_release_started=state_release_returned=state_destroyed=0;
    state_delete_entered=state_original_preserved=0;state_temporary=0;
    state_delete_flags=state_delete_thread=0;state_delete_caller=0;
    submit_original_state=submit_root=0;submit_attempted=0;
    submit_owned_verified=submit_sources_released=submit_original_preserved=0;epoch_submit_total=0;
    // submit_retained must already be zero; never erase failed ownership proof.
}
void advance_world_handoff(FILE*){
    if(!pending_world||!pending_world->request||world_commit_request.load()!=pending_world->request||submitted_lifetime.stopping())return;
    const auto phase=world_handoff_phase.load();if(phase!=1&&phase!=2)return;
    const auto previous=committed_world_generation.load(),next=pending_world->metadata.generation;
    if(!world_quiesce_requested.load()){
        if(!stereo_publisher.begin_world_rebind(previous))return;
        world_quiesce_requested=true;return;
    }
    if(phase!=2||!world_old_state_retired())return;
    gears_vr::GpuIdleSnapshot idle{};if(!stereo_publisher.try_idle(idle))return;
    gears_world::EpochGate::CloseToken closed{};if(!world_gate.try_close(closed))return;
    struct Reopen {gears_world::EpochGate::CloseToken token;~Reopen(){if(!world_gate.open(token))std::terminate();}} reopen{closed};
    std::unique_lock<std::recursive_mutex> native_lock(session_native_mutex,std::try_to_lock);
    std::unique_lock<std::recursive_mutex> frame_lock_guard(frame_hook_mutex,std::try_to_lock);
    std::unique_lock<std::mutex> stereo_lock(stereo_mutex,std::try_to_lock);
    if(!native_lock.owns_lock()||!frame_lock_guard.owns_lock()||!stereo_lock.owns_lock()||
       !world_old_state_retired()||!ui_world_handoff_ready()||!frame_link.world_handoff_ready(previous,next))return;
    const auto fresh=pending_world_fresh();if(fresh==PendingFresh::transient)return;
    if(fresh==PendingFresh::stale){
        pending_world.reset();world_candidate_generation=0;world_commit_request=0;world_handoff_phase=4;return;
    }
    const auto minimum=gears_vr::monotonic_ns();
    const bool committed=stereo_publisher.commit_world_rebind(previous,next,minimum,[&](){
        const auto destroyed=submitted_lifetime.confirm_destroyed(world_quiesce_token);
        if(!destroyed.owner){request_session_stop(SessionStop::proof_failure);return false;}
        if(!frame_link.advance_world_epoch(previous,next)){request_session_stop(SessionStop::proof_failure);return false;}
        retire_stereo_window();
        if(!stereo_ticket_store.retire_through(stereo_ticket_store.highest_id())){request_session_stop(SessionStop::proof_failure);return false;}
        retire_ui_world();anchor_config=pending_world->anchor;ui_config=pending_world->ui;
#if defined(GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER) && GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
        if(crosshair_live::enabled)crosshair_live::publish_recovery(*pending_world->crosshair,next);
#endif
        auto const& m=pending_world->metadata;
        expected_local=m.local_player;expected_pc=m.controller;expected_camera=m.camera;camera_cache=m.timestamp;camera_yaw=m.camera_yaw;
        reset_retired_view_state();stereo_gate.request_recenter();
        if(!submitted_lifetime.reopen(destroyed)){request_session_stop(SessionStop::proof_failure);return false;}
        world_quiesce_token={};committed_world_generation=next;return true;
    });
    if(committed){
        world_quiesce_requested=false;world_handoff_phase=3;++world_handoff_commits;
        world_candidate_generation=0;world_commit_request=0;pending_world.reset();
    }
}
