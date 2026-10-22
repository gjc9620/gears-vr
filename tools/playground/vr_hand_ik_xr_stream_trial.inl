// Included inside handstream. This private alternative reads only fresh GVE6
// receipts; the synthetic trajectory and automatic melee dispatch are absent.
gears_weapon::EquipmentIntentBridge xr_bridge;
gears_hand_xr_trial::Start xr_pending{};
constexpr unsigned xr_max_leases=256;

bool prepare_xr_request(void* argument){
    gears_hand_xr_trial::Start value{};
    LARGE_INTEGER now{},frequency{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&frequency);
    if(!copy(&value,reinterpret_cast<uptr>(argument),sizeof(value))||
       !gears_hand_xr_trial::shape(value,GetCurrentProcessId(),
           gears_vr::controller_process_start(GetCurrentProcess()),GetTickCount64(),
           std::uint64_t(now.QuadPart),std::uint64_t(frequency.QuadPart)))return false;
    xr_pending=value;return true;
}
bool xr_object(gears_hand_xr_trial::ObjectKey const& key){
    const auto object=uptr(key.address);
    if(!registered(object)||playground::deleted(object))return false;
    std::array<unsigned char,0x58> header{};
    if(!copy(header.data(),object,header.size()))return false;
    gears_hand_xr_trial::ObjectKey current{};Name object_name{};
    current.address=object;
    std::memcpy(&current.internal_index,header.data()+8,4);
    std::memcpy(&current.outer,header.data()+0x40,8);
    std::memcpy(&object_name,header.data()+0x48,8);
    std::memcpy(&current.object_class,header.data()+0x50,8);
    current.name_index=object_name.index;current.name_number=object_name.number;
    return !std::memcmp(&current,&key,sizeof(key))&&registered(object)&&!playground::deleted(object);
}
bool xr_identity(Snapshot const& value){
    namespace abi=gears_hand_xr_trial;
    const auto& start=trial.xr_start;
    for(auto const& key:start.objects)if(!xr_object(key))return false;
    const auto address=[&](unsigned role){return uptr(start.objects[role].address);};
    uptr world{};
    return handik::pointer(value.pc,L"WorldInfo",world)&&world==address(abi::world_info)&&
        value.pc==address(abi::controller)&&value.pawn==address(abi::pawn)&&
        value.mesh==address(abi::mesh)&&value.weapon==address(abi::weapon)&&
        get<uptr>(world+0x40)==address(abi::level)&&
        get<uptr>(value.pc+0x40)==address(abi::level)&&
        get<uptr>(value.pawn+0x40)==address(abi::level)&&
        value.world_time>=trial.xr_world_time&&
        (!trial.anchored||same_layout(trial.anchor,value));
}
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
bool xr_equipment_gate(Snapshot const& value,unsigned expected,const wchar_t* stage,EquipmentTiming measured_stage){
    EquipmentTimingScope timing(measured_stage);
    const unsigned current=vrequip::hand_equipment_phase(value.pc,value.pawn,value.weapon);
    if(expected&&expected<=2&&trial.xr_equipment_phase==expected&&current==expected)return true;
    const bool clean=yield_for_equipment(stage);
    if(log_file){fwprintf(log_file,L"hand_ik_equipment_gate stage=%ls expected_phase=%u current_phase=%u "
        L"saved_phase=%u hand_clean=%u writes=0 epoch=%u\n",stage,expected,current,
        trial.xr_equipment_phase,unsigned(clean),trial.epoch);fflush(log_file);}
    return false;
}
#endif
gears_hand_stream::Context xr_context(Snapshot const& value,bool ready){
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    // Context construction must not repeat engine queries. The observed,
    // prewrite and post-flush gates already authenticate equipment explicitly;
    // action epochs and the final authorize still reject any intervening yield.
    ready=ready&&trial.xr_ready;
#endif
    gears_hand_stream::Context context{};
    context.game={trial.xr_start.objects[gears_hand_xr_trial::world_info].address,
        value.pc,value.pawn,value.mesh,value.weapon};
    context.expected_native_world_generation=trial.xr_start.expected_native_world_generation;
    context.action_epoch=trial.epoch;context.observed_ms=value.ms;
    context.idle_ready=ready;context.clean=!trial.lease.dirty();
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    context.clean=context.clean&&!trial.lease.faulted();
#endif
    return context;
}
void xr_reset_bindings(unsigned mask){
    for(unsigned hand=0;hand<2;++hand)if(mask&(1u<<hand)){
        trial.xr_binding[hand].reset();++trial.xr_resets;
    }
}
bool xr_targets(gears_weapon::HandWorldFrame const& frame,unsigned mask){
    trial.xr_grips={{frame.left,frame.right}};
    for(unsigned hand=0;hand<2;++hand)if(mask&(1u<<hand)){
        auto& pose=trial.xr_targets[hand];
        if(!trial.xr_binding[hand].solve(trial.xr_grips[hand],pose))return false;
        const handik::Triple position{{float(pose.position.x),float(pose.position.y),float(pose.position.z)}};
        if(!gears_hand_ik::finite(position))return false;
        const playground::Rotation rotation{gears_vr::to_rotator_units(pose.rotation.pitch),
            gears_vr::to_rotator_units(pose.rotation.yaw),gears_vr::to_rotator_units(pose.rotation.roll)};
        const unsigned channel=hand?0u:2u;
        std::memcpy(trial.targets[channel].data(),position.data(),12);
        std::memcpy(trial.targets[channel+1].data(),&rotation,12);
    }
    return true;
}
void xr_log_pose(gears_weapon::HandWorldFrame const& frame,unsigned mask,unsigned bind_mask){
    if(!log_file)return;
    fwprintf(log_file,L"hand_ik_xr_stream_pose lease=%u serial=%llu sequence=%llu session=%llu "
        L"space_epoch=%llu input_epoch=%llu world_generation=%llu created_ms=%llu "
        L"units_per_metre=%.9g tracked_mask=%u planned_mask=%u bind_mask=%u",
        leases.load()+1,frame.serial,frame.sequence,frame.session,frame.space_epoch,frame.input_epoch,
        frame.world_generation,frame.created_ms,frame.units_per_metre,unsigned(frame.valid_mask),mask,bind_mask);
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    fwprintf(log_file,L" equipment_phase=%u action_epoch=%u",trial.xr_equipment_phase,trial.epoch);
#endif
    for(unsigned hand=0;hand<2;++hand){
        const auto& grip=trial.xr_grips[hand];const auto& target=trial.xr_targets[hand];
        fwprintf(log_file,L" grip%u=%.12g,%.12g,%.12g,%.12g,%.12g,%.12g "
            L"bone_target%u=%.12g,%.12g,%.12g,%.12g,%.12g,%.12g",hand,
            grip.position.x,grip.position.y,grip.position.z,
            grip.rotation.pitch,grip.rotation.yaw,grip.rotation.roll,hand,
            target.position.x,target.position.y,target.position.z,
            target.rotation.pitch,target.rotation.yaw,target.rotation.roll);
    }
    fwprintf(log_file,L"\n");fflush(log_file);
}
void pre_tick(uptr){
    if(!active_visible.load(std::memory_order_acquire)||trial.ended)return;
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    if(trial.phase&&trial.timing.frequency&&!stop_requested.load(std::memory_order_acquire)){
        const auto writer_before=trial.xr_cursor,begin=timing_clock();
        gears_weapon::HandWorldFrame early{};
        const bool fresh=trial.xr_open&&xr_bridge.read_pose(shadow.cursor,early);
        const auto finish=timing_clock();
        shadow_sample(fresh?&early:nullptr,GetTickCount64(),writer_before,begin,finish);
    }
#endif
    if(trial.lease.dirty()){
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        timing_pre_restore();
#endif
        if(!restore(L"pre_tick"))return;
    }
    if(stop_requested.load(std::memory_order_acquire)){end(6,L"xr_stopped_after_restore");return;}
    trial.tick_epoch=trial.epoch;
}
void post_tick(uptr pc){
    if(requested.exchange(false,std::memory_order_acq_rel)){
        xr_bridge.close();trial={};trial.xr_start=xr_pending;
        trial.xr_world_time=xr_pending.baseline_world_time;
        trial.phase=1;trial.began=GetTickCount64();trial.deadline=trial.began+15000;
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        timing_begin();
#endif
        if(trial.began>trial.xr_start.arm_deadline_ms){end(7,L"xr_arm_deadline_expired_before_tick");return;}
        if(log_file){fwprintf(log_file,L"hand_ik_stream_trial_begin source=GVE6_grip_world "
            L"max_leases=%u head_controllers_untouched=1 force_evaluation=0 ownership_proved=0 "
            L"external_frozen_native_epoch=%llu native_started_qpc=%llu world_handoff_supported=0\n",
            xr_max_leases,trial.xr_start.expected_native_world_generation,trial.xr_start.native_started_qpc);
            fflush(log_file);}
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        if(log_file){fwprintf(log_file,L"hand_ik_equipment_begin original_hand_phase=1 owned_detached_phase=2 "
            L"blocked_phase=0 max_leases=%u budget_ms=15000 ownership_proved=0\n",xr_max_leases);fflush(log_file);}
#endif
    }
    if(!active_visible.load(std::memory_order_acquire)||trial.ended||!trial.phase)return;
    const auto now=GetTickCount64();
    if(stop_requested.load(std::memory_order_acquire)){end(6,L"xr_stopped_in_clean_phase");return;}
    if(now>trial.deadline){end(7,L"xr_bounded_trial_timeout");return;}
    if(leases.load()>=xr_max_leases){
        // Original actions during the final observation can reset readiness,
        // but can never reopen the exhausted write budget.
        if(trial.phase!=4){trial.phase=4;trial.final_ms=now;}
        if(now-trial.final_ms>=250)end(5,L"bounded_xr_dual_hand_recording_complete");
        return;
    }
    Snapshot value{};bool known{};
    {
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        EquipmentTimingScope timing(EquipmentTiming::hand_snapshot_identity);
#endif
        known=handik::snapshot(pc,value)&&xr_identity(value);
    }
    if(!known){
        end(7,L"xr_frozen_player_world_or_layout_changed");return;
    }
    trial.xr_world_time=value.world_time;
    if(!trial.anchored){trial.anchor=value;trial.anchored=true;}
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    unsigned observed_phase{};
    {
        EquipmentTimingScope timing(EquipmentTiming::observed_phase);
        observed_phase=vrequip::hand_equipment_phase(pc,value.pawn,value.weapon);
    }
    const unsigned equipment_phase=observed_phase<=2?observed_phase:0;
    const bool equipment_changed=equipment_phase!=trial.xr_equipment_phase;
    if(equipment_changed){
        const auto previous=trial.xr_equipment_phase;
        const bool clean=yield_for_equipment(L"equipment_phase_changed");
        trial.xr_equipment_phase=equipment_phase;++trial.xr_equipment_changes;
        if(log_file){fwprintf(log_file,L"hand_ik_equipment_phase previous=%u current=%u hand_clean=%u "
            L"epoch=%u leases=%u consume_blocked_baseline=1 writes=0\n",previous,equipment_phase,
            unsigned(clean),trial.epoch,leases.load());fflush(log_file);}
        if(!clean){if(!trial.ended)end(trial.lease.dirty()?8:7,L"equipment_phase_handback_failed");return;}
    }
#endif
    const bool quiet=slots_quiet(value);
    const bool qualified=value.mesh_tick_known&&value.action_idle&&
        handik::baseline_strengths(value)&&quiet&&trial.tick_epoch==trial.epoch
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        &&equipment_phase&&!equipment_changed&&equipment_phase==trial.xr_equipment_phase
#endif
        ;
    if(!qualified){
        trial.stability.reset();trial.xr_ready=false;trial.phase=1;status=1;
    }else if(!trial.xr_ready&&value.mesh_tick!=trial.previous.mesh_tick){
        if(trial.stability.observe(now,true,handik::same_controls(trial.previous,value))){
            for(unsigned channel=0;channel<4;++channel)
                if(!target_layout(value.controls[indices[channel]].object,(channel&1)!=0)){
                    end(7,L"xr_target_abi_unknown");return;
                }
            if(trial.epoch)trial.resumed=true;
            trial.xr_ready=true;trial.phase=2;status=2;
        }
    }
    trial.previous=value;
    if(!trial.xr_open&&now>=trial.xr_next_open){
        trial.xr_open=xr_bridge.open(false);trial.xr_next_open=now+250;
    }
    gears_weapon::HandWorldFrame frame{};
    const bool fresh=trial.xr_open&&xr_bridge.read_pose(trial.xr_cursor,frame);
    gears_hand_stream::Ticket ticket{};
    if(fresh){
        ++trial.xr_fresh;
        if(frame.world_generation!=trial.xr_start.expected_native_world_generation){
            end(7,L"xr_native_epoch_changed");return;
        }
        ticket.stream={frame.session,frame.space_epoch,frame.input_epoch,frame.world_generation};
        ticket.serial=frame.serial;ticket.sequence=frame.sequence;ticket.created_ms=frame.created_ms;
        ticket.controller=frame.controller;ticket.pawn=frame.pawn;ticket.weapon=frame.weapon;
        ticket.units_per_metre=frame.units_per_metre;ticket.valid_mask=frame.valid_mask;
    }else ++trial.xr_none;
    const auto context=xr_context(value,qualified&&trial.xr_ready);
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    timing_observation();
#endif
    const auto plan=observe_xr_policy(context,fresh?&ticket:nullptr,GetTickCount64(),true);
    xr_reset_bindings(plan.reset_binding_mask);
    if(!plan.accept_mask)return;
    for(unsigned hand=0;hand<2;++hand)if(plan.bind_mask&(1u<<hand)){
        if(!trial.xr_binding[hand].bind(hand?frame.right:frame.left,hand?value.right:value.left)||
           !trial.xr_policy.commit_binding(plan,1u<<hand)){
            end(7,L"xr_grip_to_bone_binding_rejected");return;
        }
        ++trial.xr_binds;
    }
    if(!xr_targets(frame,plan.accept_mask)){end(7,L"xr_bone_target_invalid");return;}
    trial.sequence=frame.sequence;
    const auto lease=leases.load()+1;
    Snapshot check{};
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    const bool checked=handik::snapshot(pc,check)&&xr_identity(check);
    if(checked&&!xr_equipment_gate(check,equipment_phase,L"prewrite_snapshot",EquipmentTiming::prewrite_phase))return;
    if(!checked||!check.action_idle||
#else
    if(!handik::snapshot(pc,check)||!xr_identity(check)||!check.action_idle||
#endif
       !handik::baseline_strengths(check)||!handik::same_controls(value,check)||
       value.mesh_world!=check.mesh_world||value.left!=check.left||value.right!=check.right||
       !slots_quiet(check)||trial.tick_epoch!=trial.epoch){
        end(7,L"xr_prewrite_state_changed");return;
    }
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    const auto authorize_context=xr_context(check,trial.xr_ready);
    const unsigned mask=trial.xr_policy.authorize(plan,authorize_context,GetTickCount64());
#else
    const unsigned mask=trial.xr_policy.authorize(plan,xr_context(check,trial.xr_ready),GetTickCount64());
#endif
    if(!mask)return;
    if(!log_file||ferror(log_file)){end(7,L"xr_log_unavailable_before_write");return;}
    {
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        EquipmentTimingScope timing(EquipmentTiming::write_evidence);
#endif
        sample(L"before",value,lease,0);
        xr_log_pose(frame,mask,plan.bind_mask);
    }
    // fflush may block. Recheck the receipt and game sample after all logging,
    // immediately before the first journal snapshot or mutation.
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    if(!xr_equipment_gate(check,equipment_phase,L"after_log_flush",EquipmentTiming::postflush_phase))return;
    const auto final_context=xr_context(check,trial.xr_ready);
#endif
    const auto write_now=GetTickCount64();
    if(stop_requested.load(std::memory_order_acquire)){end(6,L"xr_stopped_before_write");return;}
    if(write_now>trial.deadline){end(7,L"xr_deadline_before_write");return;}
    if(ferror(log_file)||trial.tick_epoch!=trial.epoch||
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
       trial.xr_policy.authorize(plan,final_context,write_now)!=mask){
#else
       trial.xr_policy.authorize(plan,xr_context(check,trial.xr_ready),write_now)!=mask){
#endif
        end(7,L"xr_receipt_or_log_expired_before_write");return;
    }
    trial.lease_anchor=check;Backend backend{};
    const unsigned channels=((mask&1)?12u:0u)|((mask&2)?3u:0u);
    bool acquired{};
    {
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        EquipmentTimingScope timing(EquipmentTiming::lease_acquire);
#endif
        acquired=trial.lease.acquire(channels,trial.targets,1.f,backend);
    }
    if(!acquired){
        end(trial.lease.dirty()?8:7,L"xr_lease_acquisition_failed");return;
    }
    trial.applied_mask=mask;leases=lease;++trial.xr_masks[mask];
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    EquipmentTimingScope timing(EquipmentTiming::written_snapshot_evidence);
#endif
    Snapshot written{};
    if(!handik::snapshot(pc,written)||!same_layout(check,written)){
        restore(L"xr_failed_written_snapshot");end(7,L"xr_written_snapshot_unknown");return;
    }
    sample(L"written",written,lease,mask);
}
