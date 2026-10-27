// Explicitly armed, private real-XR left-hand pull to the original Lancer
// saw hold. No blade target is acquired here. The starter mount is provisional
// and must not be enabled in the normal launcher before visual/headset QA.
std::atomic<bool> lancer_saw_xr_arm{},lancer_saw_xr_stop{};
std::atomic<bool> lancer_saw_xr_visible{};
std::atomic<unsigned> lancer_saw_xr_status{};
struct LancerSawXrPull {
    gears_weapon::LancerChainsawInteraction interaction{};
    ULONGLONG armed_ms{};
    bool active{},stop_pending{},release_pending{};
};
LancerSawXrPull lancer_saw_xr{};

// 0 disabled, 2 armed, 3 pulling, 4 original B sent, 5 running,
// 6 original paired animation / deferred stop, 7 restored, 8 stopped,
// 9 original state or release failure.
void lancer_saw_xr_finish(unsigned status){
    auto& trial=lancer_saw_xr;
    if(lancer_saw_hold.active){
        const auto released=lancer_saw_end_hold();
        if(released!=3){
            trial.stop_pending=true;
            lancer_saw_xr_status.store(released==8?6u:9u,
                                       std::memory_order_release);
            return;
        }
    }
    trial={};
    lancer_saw_xr_visible.store(false,std::memory_order_release);
    lancer_saw_xr_status.store(status,std::memory_order_release);
}
bool lancer_saw_xr_original(uptr pc,uptr pawn,uptr weapon,
                            gears_weapon::LancerSawGamePose& game){
    bool button{},running{},melee{},special{};
    std::uint8_t move{};
    if(!read_bool(pc,L"bHoldingButton_B",button)||
       !read_bool(weapon,L"bChainSawRunning",running)||
       !read_bool(pawn,L"bDoingMeleeAttack",melee)||
       !read_bool(pawn,L"bDoingSpecialAttackMove",special)||
       !lancer_saw_special_move_id(pawn,move))return false;
    game.original_button_held=button;
    game.original_execution=move==34;
    game.original_hold=button&&running&&melee&&move==33;
    // This is an original-state exit gate, not proof that both rendered
    // character animations reached their final visible frame.
    // The blade-running flag can remain latched until B is released, so it
    // cannot be a prerequisite for requesting that release.
    game.original_ready=move!=33&&move!=34&&move!=36&&move!=37&&
        !melee&&!special;
    return true;
}
void lancer_saw_xr_tick(uptr pc,ULONGLONG now){
    if(lancer_saw_xr_arm.exchange(false,std::memory_order_acq_rel)){
        if(!lancer_saw_xr.active&&!lancer_saw_hold.active&&
           playground::ready&&test_environment()){
            lancer_saw_xr={};
            lancer_saw_xr.active=true;lancer_saw_xr.armed_ms=now;
            lancer_saw_xr_visible.store(true,std::memory_order_release);
            lancer_saw_xr_status.store(2,std::memory_order_release);
        }else lancer_saw_xr_status.store(9,std::memory_order_release);
    }
    auto& trial=lancer_saw_xr;
    if(lancer_saw_xr_stop.exchange(false,std::memory_order_acq_rel))
        trial.stop_pending=true;
    if(!trial.active)return;
    if(now<trial.armed_ms||now-trial.armed_ms>15000)
        trial.stop_pending=true; // private trial watchdog

    const auto pawn=playground::pointer(pc,L"Pawn");
    const auto weapon=playground::pointer(pawn,L"Weapon");
    const auto world=playground::pointer(pc,L"WorldInfo");
    if(!test_environment()||!lancer_saw_same_game(pc,pawn,weapon,world)){
        lancer_saw_xr_finish(9);return;
    }
    gears_weapon::LancerSawGamePose game{};
    game.world=world;game.controller=pc;game.pawn=pawn;game.weapon=weapon;
    if(!lancer_saw_xr_original(pc,pawn,weapon,game)){
        lancer_saw_xr_finish(9);return;
    }
    const auto& pose=last_intent_pose;
    const bool fresh=last_intent_pose_known&&now>=pose.created_ms&&
        now-pose.created_ms<=100&&pose.controller==pc&&pose.pawn==pawn&&
        pose.weapon==weapon;
    game.lancer_active=true;
    game.right_owns_weapon=model.held_weapon()==weapon;
    game.world_generation=fresh?pose.world_generation:1;
    if(trial.interaction.phase()==gears_weapon::LancerSawPhase::animation||
       (game.original_execution&&
        trial.interaction.phase()!=gears_weapon::LancerSawPhase::idle)){
        gears_weapon::LancerSawSample sample{};
        sample.world=world;sample.pawn=pawn;sample.weapon=weapon;
        sample.original_execution=game.original_execution;
        sample.original_hold=game.original_hold;
        sample.original_ready=game.original_ready;
        sample.original_button_held=game.original_button_held;
        sample.lancer_active=true;
        sample.right_owns_weapon=game.right_owns_weapon;
        sample.right_tracked=fresh&&(pose.valid_mask&2)!=0;
        const auto decision=trial.interaction.observe(sample);
        if(decision.event==gears_weapon::LancerSawEvent::animation_handoff&&
           log_file){
            fwprintf(log_file,L"lancer_saw_xr_animation unexpected=%u victim=%llu\n",
                unsigned(decision.unexpected_original_target),decision.victim);
            fflush(log_file);
        }
        lancer_saw_xr_status.store(6,std::memory_order_release);
        if(decision.event==gears_weapon::LancerSawEvent::request_original_release)
            trial.release_pending=true;
        if(trial.release_pending&&game.original_ready){
            if(lancer_saw_end_hold()==3)trial.release_pending=false;
            else lancer_saw_xr_status.store(9,std::memory_order_release);
        }
        if(decision.event==gears_weapon::LancerSawEvent::restored)
            lancer_saw_xr_finish(trial.stop_pending?8u:7u);
        else if(decision.event==gears_weapon::LancerSawEvent::interrupted)
            lancer_saw_xr_finish(8);
        return;
    }
    if(trial.stop_pending){lancer_saw_xr_finish(8);return;}
    if(!fresh){
        if(trial.interaction.phase()!=gears_weapon::LancerSawPhase::idle)
            lancer_saw_xr_finish(8);
        return;
    }
    if(!game.right_owns_weapon){
        if(trial.interaction.phase()!=gears_weapon::LancerSawPhase::idle)
            lancer_saw_xr_finish(8);
        return;
    }
    MeshPoseProbe gun{};
    std::array<float,16> matrix{};
    gears_weapon::LancerStarterWorld mount{};
    bool can_melee{};
    if(!mesh_pose_probe(weapon,gun)||gun.hidden||
       wcscmp(gun.asset_name,L"COGAssaultRifle")){
        lancer_saw_xr_finish(9);return;
    }
    std::memcpy(matrix.data(),gun.local_to_world,sizeof(gun.local_to_world));
    if(!gears_weapon::lancer_starter_world(matrix,mount)||
       !lancer_saw_original_bool(weapon,L"CanDoSpecialMeleeAttack",can_melee)){
        lancer_saw_xr_finish(9);return;
    }
    game.starter_anchor_raw=mount.anchor_raw;
    game.pull_axis_world=mount.pull_axis;
    game.game_can_melee=can_melee;
    gears_weapon::LancerSawSample sample{};
    if(!gears_weapon::lancer_saw_sample_from_world(pose,game,now,sample)){
        lancer_saw_xr_finish(9);return;
    }
    const auto decision=trial.interaction.observe(sample);
    if(decision.event==gears_weapon::LancerSawEvent::pull_began)
        lancer_saw_xr_status.store(3,std::memory_order_release);
    else if(decision.event==gears_weapon::LancerSawEvent::request_original_hold){
        const auto held=lancer_saw_begin_hold(pc);
        if(held==2){
            lancer_saw_xr_status.store(4,std::memory_order_release);
            if(log_file){fwprintf(log_file,L"lancer_saw_xr_pull_started "
                L"session=%llu sequence=%llu units_per_metre=%.3f\n",
                pose.session,pose.sequence,pose.units_per_metre);fflush(log_file);}
        }else lancer_saw_xr_finish(9);
    }else if(decision.event==gears_weapon::LancerSawEvent::hold_started)
        lancer_saw_xr_status.store(5,std::memory_order_release);
    else if(decision.event==gears_weapon::LancerSawEvent::interrupted)
        lancer_saw_xr_finish(8);
}
