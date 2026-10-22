// One-shot synthetic game-thread pull. It exercises the production gesture
// state machine and original B boundary with current original gun matrices,
// but publishes no XR frame and does not prove a visible or reachable starter.
std::atomic<bool> lancer_saw_gesture_trial_requested{};
std::atomic<unsigned> lancer_saw_gesture_trial_status{};
struct LancerSawGestureTrial {
    gears_weapon::LancerChainsawInteraction interaction{};
    ULONGLONG started_ms{},grip_ms{},last_ms{};
    std::uint64_t sequence{};
    bool active{};
};
LancerSawGestureTrial lancer_saw_gesture_trial{};

void lancer_saw_gesture_trial_fail(const wchar_t* reason){
    auto& trial=lancer_saw_gesture_trial;
    trial.active=false;
    const auto released=lancer_saw_end_hold();
    lancer_saw_gesture_trial_status.store(released==3?5u:6u,
                                           std::memory_order_release);
    if(log_file){fwprintf(log_file,L"lancer_saw_gesture_trial_failed reason=%ls "
        L"sequence=%llu released=%u\n",reason,trial.sequence,released);fflush(log_file);}
}
void lancer_saw_gesture_trial_tick(uptr pc){
    const auto now=GetTickCount64();
    if(lancer_saw_gesture_trial_requested.exchange(false,std::memory_order_acq_rel)){
        lancer_saw_gesture_trial={};
        lancer_saw_gesture_trial.active=true;
        lancer_saw_gesture_trial.started_ms=now;
        lancer_saw_gesture_trial_status.store(1,std::memory_order_release);
    }
    auto& trial=lancer_saw_gesture_trial;
    if(!trial.active||now==trial.last_ms)return;
    if(now-trial.started_ms>2000){lancer_saw_gesture_trial_fail(L"timeout");return;}
    const auto pawn=playground::pointer(pc,L"Pawn");
    const auto weapon=playground::pointer(pawn,L"Weapon");
    const auto world=playground::pointer(pc,L"WorldInfo");
    MeshPoseProbe gun{};
    if(!lancer_saw_same_game(pc,pawn,weapon,world)||
       model.held_weapon()!=weapon||!mesh_pose_probe(weapon,gun)||gun.hidden||
       wcscmp(gun.asset_name,L"COGAssaultRifle")){
        lancer_saw_gesture_trial_fail(L"identity_or_original_gun");return;
    }
    std::array<float,16> matrix{};
    std::memcpy(matrix.data(),gun.local_to_world,sizeof(gun.local_to_world));
    gears_weapon::LancerStarterWorld mount{};
    if(!gears_weapon::lancer_starter_world(matrix,mount)){
        lancer_saw_gesture_trial_fail(L"starter_mount");return;
    }
    bool can_melee{},b_held{};
    if(!lancer_saw_original_bool(weapon,L"CanDoSpecialMeleeAttack",can_melee)||
       !read_bool(pc,L"bHoldingButton_B",b_held)){
        lancer_saw_gesture_trial_fail(L"original_state");return;
    }
    ++trial.sequence;
    if(trial.sequence==2)trial.grip_ms=now;
    const double travel=trial.sequence<=2?0.:
        (std::min)(19.,double(now-trial.grip_ms)*19./300.);
    gears_weapon::HandWorldFrame pose{};
    pose.created_ms=now;pose.sequence=trial.sequence;
    pose.session=pose.space_epoch=pose.input_epoch=pose.world_generation=1;
    pose.controller=pc;pose.pawn=pawn;pose.weapon=weapon;
    pose.units_per_metre=100;
    pose.valid_mask=3;pose.grip_down_mask=trial.sequence>=2?1:0;
    pose.right.position={gun.local_to_world[12],gun.local_to_world[13],
                         gun.local_to_world[14]};
    pose.left.position={mount.anchor_raw.x+mount.pull_axis.x*travel,
                        mount.anchor_raw.y+mount.pull_axis.y*travel,
                        mount.anchor_raw.z+mount.pull_axis.z*travel};
    gears_weapon::LancerSawGamePose game{};
    game.world=world;game.world_generation=1;
    game.controller=pc;game.pawn=pawn;game.weapon=weapon;
    game.starter_anchor_raw=mount.anchor_raw;
    game.pull_axis_world=mount.pull_axis;
    game.lancer_active=game.right_owns_weapon=true;
    game.game_can_melee=can_melee;game.original_hold=b_held;
    gears_weapon::LancerSawSample sample{};
    if(!gears_weapon::lancer_saw_sample_from_world(pose,game,now,sample)){
        lancer_saw_gesture_trial_fail(L"sample_rejected");return;
    }
    const auto decision=trial.interaction.observe(sample);
    trial.last_ms=now;
    if(decision.event==gears_weapon::LancerSawEvent::request_original_hold){
        const auto held=lancer_saw_begin_hold(pc);
        lancer_saw_status.store(held,std::memory_order_release);
        if(held!=2){
            lancer_saw_gesture_trial_fail(L"original_hold_rejected");return;
        }
        lancer_saw_gesture_trial_status.store(3,std::memory_order_release);
        if(log_file){fwprintf(log_file,L"lancer_saw_gesture_trial_pull "
            L"sequence=%llu travel=%.3f elapsed_ms=%llu\n",trial.sequence,
            travel,now-trial.grip_ms);fflush(log_file);}
    }else if(decision.event==gears_weapon::LancerSawEvent::hold_started){
        trial.active=false;
        lancer_saw_gesture_trial_status.store(4,std::memory_order_release);
    }else if(decision.event==gears_weapon::LancerSawEvent::interrupted){
        lancer_saw_gesture_trial_fail(L"gesture_interrupted");
    }else if(decision.phase==gears_weapon::LancerSawPhase::pulling){
        lancer_saw_gesture_trial_status.store(2,std::memory_order_release);
    }
}
