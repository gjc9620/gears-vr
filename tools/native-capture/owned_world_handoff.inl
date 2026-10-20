// Worker stage, deliberately BEFORE closing world readers. Queued UI and D3D
// callbacks must retain their ordinary observation path while old work drains.
#if defined(GEARS_OWNED_WORLD_HANDOFF) && GEARS_OWNED_WORLD_HANDOFF
struct OwnedWorldHandoff {
    uint64_t world{},cutoff{};
    bool resource_prepared{},ui_prepared{};
    gears_owned_retirement::Handoff::Receipt resource;
    gears_owned_ui_world::Receipt ui;
} owned_world_handoff;
gears_owned_ui_world::Guards owned_world_ui_guards(bool resources_retired){
    gears_owned_ui_world::Guards g{};
    g.old_world=owned_world_handoff.world;g.ticket_cutoff=owned_world_handoff.cutoff;
    g.production_closed=world_quiesce_requested.load()&&world_old_state_retired();
    g.semantic_pending=ui_semantic_consumer_pending;g.resources_retired=resources_retired;
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
    g.material_pending=game_material::completion.blocks_present();
#endif
    return g;
}
bool prepare_owned_world_handoff(uint64_t previous){
    std::unique_lock<std::recursive_mutex> native_lock(session_native_mutex,std::try_to_lock);
    std::unique_lock<FrameHookMutex> frame_lock(frame_hook_mutex,std::try_to_lock);
    if(!native_lock.owns_lock()||!frame_lock.owns_lock()||submitted_lifetime.stopping()||!world_old_state_retired())return false;
    if(!owned_world_handoff.world){
        owned_world_handoff.world=previous;owned_world_handoff.cutoff=stereo_ticket_counter.load();
    }
    if(owned_world_handoff.world!=previous)return false;
    const auto guards=owned_world_ui_guards(false);
    if(!owned_ui_world_cpu_ready(guards))return false;
    if(!owned_world_handoff.resource_prepared)
        owned_world_handoff.resource_prepared=native_owned_scene_handoff_prepare(previous,owned_world_handoff.cutoff,true,true);
    if(!owned_world_handoff.resource_prepared){
        if(native_owned_scene_handoff_failed())request_session_stop(SessionStop::proof_failure);
        return false;
    }
    const auto generation=native_owned_scene_resource_generation();
    bool bridges=stereo_publisher.retire_owned_source(previous,generation);
#if defined(GEARS_DESKTOP_MIRROR) && GEARS_DESKTOP_MIRROR
    {StereoPublishBypass bypass;bridges=desktop_mirror.retire_source(generation)&&bridges;}
#endif
    if(!native_owned_scene_handoff_step(bridges)){
        if(native_owned_scene_handoff_failed())request_session_stop(SessionStop::proof_failure);
        return false;
    }
    if(!native_owned_scene_handoff_receipt(owned_world_handoff.resource))return false;
    const auto identity=owned_world_handoff.resource.identity();
    if(identity.world!=previous||identity.ticket_cutoff!=owned_world_handoff.cutoff||identity.generation!=generation)return false;
    if(!owned_world_handoff.ui_prepared)
        owned_world_handoff.ui_prepared=owned_ui_prepare_world_retirement(owned_world_ui_guards(true),owned_world_handoff.ui);
    return owned_world_handoff.ui_prepared;
}
bool owned_world_handoff_current(){
    return owned_world_handoff.ui_prepared&&!submitted_lifetime.stopping()&&
        native_owned_scene_handoff_current(owned_world_handoff.resource)&&
        vr_dof_compat::world_retirement_ready(owned_world_handoff.world,owned_world_handoff.cutoff)&&
        owned_ui_world_retirement_current(owned_world_ui_guards(true),owned_world_handoff.ui);
}
#endif
