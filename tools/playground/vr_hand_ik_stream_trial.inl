// Default-off, owned-arena dual-hand recording. Four original world-space
// tail controllers receive a one-frame reversible lease. Original PlayerTick
// always runs with its saved fields restored. No animation evaluation, graph,
// render flag, head controller, input binding or public ownership flag changes.
namespace handstream {
namespace policy=gears_hand_ik_stream;
using handik::Snapshot;
constexpr std::array<unsigned,4> indices{{2,3,6,7}}; // R position/rotation, L position/rotation.
std::atomic<bool> requested{},stop_requested{},used{},safe_to_unload{true},shutdown_requested{},active_visible{};
std::atomic<unsigned> status{},leases{};
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
// Private, game-thread-only measurement. These counters never participate in
// permission decisions, and no measured span writes or flushes diagnostic text.
enum class EquipmentTiming:unsigned {
    pre_restore_to_observe,restore,dirty_stow_tick,original_player_tick,
    hand_snapshot_identity,slots_quiet,observed_phase,prewrite_phase,postflush_phase,
    phase_read_before,phase_snapshot,phase_read_after,write_evidence,lease_acquire,written_snapshot_evidence,count
};
constexpr unsigned equipment_timing_count=unsigned(EquipmentTiming::count);
struct EquipmentTimingStat {
    std::uint64_t count{},sum{},maximum{};
    void add(std::uint64_t value){++count;sum+=value;maximum=(std::max)(maximum,value);}
};
struct EquipmentAgeStat {
    EquipmentTimingStat past;
    std::uint64_t future{};
    void add(gears_hand_stream::ObserveAge const& age){
        if(age.known){if(age.future)++future;else past.add(age.ms);}
    }
};
struct EquipmentReasonStat {
    std::uint64_t masks[4]{};
    EquipmentAgeStat old_ticket[2],context,previous_context,new_ticket;
};
struct EquipmentTimingState {
    EquipmentTimingStat spans[3][equipment_timing_count];
    EquipmentReasonStat reasons[3][gears_hand_stream::observe_reason_count];
    std::uint64_t frequency{},pending_restore{},interval_started{},interval_completed{},invalid_clock{};
    unsigned pending_phase{},open_scopes{};
    bool dumped{};
};
#endif
struct Trial {
    unsigned phase{},restored{},action_yields{},setter_calls{},target_writes{},epoch{},tick_epoch{};
    unsigned quiet_slots{},samples{};
    unsigned applied_mask{};
    bool anchored{},ended{},immediate_pending{},immediate_test{},stop_sent{},resumed{},releasing{};
    bool melee_seen{},slot_play_seen{};
    std::uint64_t began{},deadline{},action_ms{},final_ms{},next_wait{},next_slot_log{},sequence{};
    Snapshot previous{},anchor{},lease_anchor{};
    gears_hand_ik::StabilityGate stability;
    policy::Lease lease;
    gears_weapon::FreeHandControl consumer;
    std::array<gears_vr::Viewpoint,2> bind{};
    std::array<policy::Target,4> targets{};
    std::array<uptr,320> action_objects{};
    unsigned action_object_count{};
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
    gears_hand_xr_trial::Start xr_start{};
    gears_hand_stream::Policy xr_policy;
    std::array<gears_hand_stream::GripHandBinding,2> xr_binding{};
    std::array<gears_vr::Viewpoint,2> xr_grips{},xr_targets{};
    std::uint64_t xr_next_open{},xr_cursor{UINT64_MAX};
    unsigned xr_fresh{},xr_none{},xr_resets{},xr_binds{},xr_masks[4]{};
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    unsigned xr_equipment_phase{},xr_equipment_changes{};
    EquipmentTimingState timing;
#endif
    bool xr_open{},xr_ready{};
    float xr_world_time{};
#endif
};
Trial trial{};

#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
std::uint64_t timing_clock(){
    LARGE_INTEGER stamp{};
    return QueryPerformanceCounter(&stamp)&&stamp.QuadPart>0?std::uint64_t(stamp.QuadPart):0;
}
unsigned timing_phase(){return trial.xr_equipment_phase<=2?trial.xr_equipment_phase:0;}
bool timing_active(){return trial.phase&&!trial.ended&&trial.timing.frequency&&!trial.timing.dumped;}
void timing_begin(){
    LARGE_INTEGER frequency{};
    if(QueryPerformanceFrequency(&frequency)&&frequency.QuadPart>0)
        trial.timing.frequency=std::uint64_t(frequency.QuadPart);
}
void timing_record(EquipmentTiming stage,unsigned phase,std::uint64_t begin,std::uint64_t finish){
    auto& timing=trial.timing;
    if(timing.dumped)return;
    if(!begin||finish<begin){++timing.invalid_clock;return;}
    timing.spans[phase][unsigned(stage)].add(finish-begin);
}
struct EquipmentTimingScope {
    EquipmentTiming stage;
    unsigned phase{};
    std::uint64_t begin{};
    bool active{};
    explicit EquipmentTimingScope(EquipmentTiming value,bool measure=true):stage(value){
        active=measure&&timing_active();
        if(active){phase=timing_phase();begin=timing_clock();++trial.timing.open_scopes;}
    }
    ~EquipmentTimingScope(){
        if(active){timing_record(stage,phase,begin,timing_clock());--trial.timing.open_scopes;}
    }
};
void timing_pre_restore(){
    if(!timing_active())return;
    auto& timing=trial.timing;
    // A missing next observation is left visibly incomplete, never fabricated.
    timing.pending_restore=timing_clock();timing.pending_phase=timing_phase();++timing.interval_started;
}
void timing_observation(){
    auto& timing=trial.timing;
    if(!timing.pending_restore)return;
    timing_record(EquipmentTiming::pre_restore_to_observe,timing.pending_phase,
        timing.pending_restore,timing_clock());
    timing.pending_restore=0;++timing.interval_completed;
}
void timing_policy(gears_hand_stream::ObserveDiagnostic const& diagnostic){
    if(!trial.timing.frequency||trial.timing.dumped)return;
    for(unsigned reason=0;reason<gears_hand_stream::observe_reason_count;++reason){
        const auto mask=diagnostic.reason_masks[reason];
        if(!mask||mask>3)continue;
        auto& value=trial.timing.reasons[timing_phase()][reason];++value.masks[mask];
        for(unsigned hand=0;hand<2;++hand)value.old_ticket[hand].add(diagnostic.old_ticket_age[hand]);
        value.context.add(diagnostic.context_age);value.previous_context.add(diagnostic.previous_context_age);
        value.new_ticket.add(diagnostic.new_ticket_age);
    }
}
void timing_print_age(const wchar_t* label,EquipmentAgeStat const& value){
    fwprintf(log_file,L" %ls_count=%llu %ls_sum_ms=%llu %ls_max_ms=%llu %ls_future=%llu",
        label,value.past.count,label,value.past.sum,label,value.past.maximum,label,value.future);
}
void timing_dump_if_ended(){
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return;
    auto& timing=trial.timing;
    if(!trial.ended||timing.dumped||!timing.frequency||timing.open_scopes)return;
    timing.dumped=true;
    if(!log_file)return;
    constexpr const wchar_t* stages[]={L"pre_restore_to_observe",L"restore",L"dirty_stow_tick",
        L"original_player_tick",L"hand_snapshot_identity",L"slots_quiet",L"observed_phase",
        L"prewrite_phase",L"postflush_phase",L"phase_read_before",L"phase_snapshot",L"phase_read_after",
        L"write_evidence",L"lease_acquire",L"written_snapshot_evidence"};
    constexpr const wchar_t* reasons[]={L"context_identity",L"context_readiness",L"binding_age",
        L"not_ready",L"no_receipt",L"stream_changed",L"stream_invalid",L"ticket_shape",L"ticket_owner",
        L"ticket_world",L"not_newer",L"ticket_age",L"scale_changed",L"tracking_loss",L"revision_exhausted"};
    static_assert(std::size(stages)==equipment_timing_count);
    static_assert(std::size(reasons)==gears_hand_stream::observe_reason_count);
    fwprintf(log_file,L"hand_ik_equipment_timing_begin schema=1 frequency=%llu phase_at_span_entry=1 "
        L"interval_started=%llu interval_completed=%llu interval_pending=%u invalid_clock=%llu "
        L"permission_changes=0 timing_hot_path_text_writes=0\n",timing.frequency,timing.interval_started,
        timing.interval_completed,unsigned(timing.pending_restore!=0),timing.invalid_clock);
    for(unsigned phase=0;phase<3;++phase){
        for(unsigned stage=0;stage<equipment_timing_count;++stage){
            const auto& value=timing.spans[phase][stage];if(!value.count)continue;
            fwprintf(log_file,L"hand_ik_equipment_timing phase=%u stage=%ls count=%llu sum_qpc=%llu max_qpc=%llu\n",
                phase,stages[stage],value.count,value.sum,value.maximum);
        }
        for(unsigned reason=0;reason<gears_hand_stream::observe_reason_count;++reason){
            const auto& value=timing.reasons[phase][reason];
            if(!value.masks[1]&&!value.masks[2]&&!value.masks[3])continue;
            fwprintf(log_file,L"hand_ik_equipment_policy phase=%u reason=%ls left=%llu right=%llu both=%llu",
                phase,reasons[reason],value.masks[1],value.masks[2],value.masks[3]);
            timing_print_age(L"old_left",value.old_ticket[0]);timing_print_age(L"old_right",value.old_ticket[1]);
            timing_print_age(L"context",value.context);timing_print_age(L"previous_context",value.previous_context);
            timing_print_age(L"new_ticket",value.new_ticket);fwprintf(log_file,L"\n");
        }
    }
    fwprintf(log_file,L"hand_ik_equipment_timing_end schema=1\n");fflush(log_file);
}
// Nested actions may end the trial inside a measured span. Delay the one dump
// until the outer event returns and every live span has added its final sample.
struct EquipmentTimingHookScope {~EquipmentTimingHookScope(){timing_dump_if_ended();}};
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
gears_hand_stream::Plan observe_xr_policy(gears_hand_stream::Context const& context,
    gears_hand_stream::Ticket const* fresh,std::uint64_t now){
    const auto plan=trial.xr_policy.observe(context,fresh,now);
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    timing_policy(trial.xr_policy.last_observe());
#endif
    return plan;
}
#endif
bool same_layout(Snapshot const& a,Snapshot const& b){
    if(a.identity!=b.identity)return false;
    for(unsigned i=0;i<8;++i)
        if(a.controls[i].object!=b.controls[i].object||a.controls[i].fixed!=b.controls[i].fixed)return false;
    return true;
}
// Only identity/layout and this lease's dynamic fields participate in rollback.
// New melee/action/weapon flags cannot prevent restoration of unchanged tails.
struct Backend {
    bool control(unsigned channel,handik::Control& out){
        if(channel>=indices.size())return false;
        const auto& anchor=trial.lease_anchor;
        for(const auto object:{anchor.pawn,anchor.mesh,anchor.tree})
            if(!registered(object)||playground::deleted(object))return false;
        const auto index=indices[channel];
        return handik::read_control(anchor.pawn,anchor.mesh,anchor.tree,index,out)&&
            out.object==anchor.controls[index].object&&out.fixed==anchor.controls[index].fixed;
    }
    bool read(unsigned channel,policy::Value& out){
        handik::Control value{};if(!control(channel,value))return false;
        out.strength=value.strength;
        if(channel&1)std::memcpy(out.target.data(),&value.rotation,out.target.size());
        else std::memcpy(out.target.data(),value.effector.data(),out.target.size());
        return true;
    }
    bool write_target(unsigned channel,policy::Target const& expected,policy::Target const& wanted);
    bool set_strength(unsigned channel,float strength){
        handik::Control value{};if(!control(channel,value))return false;
        ++trial.setter_calls;return handik::set_strength(value.object,strength);
    }
};
bool write_bytes(uptr address,policy::Target const& wanted){
    __try{std::memcpy(reinterpret_cast<void*>(address),wanted.data(),wanted.size());return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool Backend::write_target(unsigned channel,policy::Target const& expected,policy::Target const& wanted){
    policy::Value value{};if(!read(channel,value)||value.target!=expected)return false;
    const auto object=trial.lease_anchor.controls[indices[channel]].object;
    const auto property=playground::property(object,(channel&1)?L"BoneRotation":L"EffectorLocation");
    if(!registered(property))return false;
    ++trial.target_writes;
    if(!write_bytes(object+get<unsigned>(property+0x94),wanted))return false;
    return read(channel,value)&&value.target==wanted;
}
bool target_layout(uptr object,bool rotation){
    const auto property=playground::property(object,rotation?L"BoneRotation":L"EffectorLocation");
    const auto structure=get<uptr>(property+0xd0);
    if(!registered(property)||!playground::named(get<uptr>(property+0x50),L"StructProperty")||
       get<unsigned>(property+0x74)!=12||get<unsigned>(property+0x70)!=1||
       !registered(structure)||!playground::named(structure,rotation?L"Rotator":L"Vector")||
       get<unsigned>(structure+0x88)!=12)return false;
    constexpr const wchar_t* vectors[]={L"X",L"Y",L"Z"};
    constexpr const wchar_t* rotators[]={L"Pitch",L"Yaw",L"Roll"};
    for(unsigned i=0;i<3;++i){unsigned offset{};
        if(!handanim::structure_field(structure,rotation?rotators[i]:vectors[i],
            rotation?L"IntProperty":L"FloatProperty",4,12,offset)||offset!=i*4)return false;
    }
    return true;
}
void watch(uptr object){
    if(!object)return;
    for(unsigned i=0;i<trial.action_object_count;++i)if(trial.action_objects[i]==object)return;
    if(trial.action_object_count<trial.action_objects.size())trial.action_objects[trial.action_object_count++]=object;
}
bool quiet_reject(int index,handanim::Slot const& slot,int child,const wchar_t* reason){
    const auto now=GetTickCount64();
    if(!log_file||now<trial.next_slot_log)return false;
    trial.next_slot_log=now+250;wchar_t node[96]{};handanim::text(slot.node_name,node,96);
    fwprintf(log_file,L"hand_ik_stream_trial_slot_wait index=%d node=%ls child=%d reason=%ls "
        L"known=%u component=%p total=%.9g blend=%.9g pending=%.9g playing=%u "
        L"custom_index=%d target_index=%d children=%d child_data=",
        index,node[0]?node:L"unknown",child,reason,slot.known,reinterpret_cast<void*>(slot.component),
        slot.total,slot.blend,slot.pending,unsigned(slot.playing),slot.custom_index,slot.target_index,slot.children);
    for(int i=1;i<slot.children;++i){const auto& value=slot.child[size_t(i)];
        wchar_t animation[96]{};handanim::text(value.animation,animation,96);
        fwprintf(log_file,L"%ls%d:%u:%.9g:%.9g:%.9g:%u:%ls",i==1?L"":L",",i,value.known,
            value.weight,value.total,value.target,unsigned(value.playing),animation[0]?animation:L"unknown");
    }
    fwprintf(log_file,L"\n");fflush(log_file);return false;
}
bool slots_quiet(Snapshot const& value){
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    EquipmentTimingScope timing(EquipmentTiming::slots_quiet);
#endif
    Array slots{};trial.action_object_count=0;trial.quiet_slots=0;
    for(const auto object:{value.pc,value.pawn,value.mesh,value.tree,value.weapon})watch(object);
    if(!handanim::array(value.pawn,L"BodyStanceNodes",L"ObjectProperty",8,
          handanim::max_slot_allocation,slots)||slots.count<=0||slots.count>int(handanim::max_slots))return false;
    for(int i=0;i<slots.count;++i){uptr object{};
        // A failed pointer read must never be treated as an empty slot.
        if(!copy(&object,slots.data+size_t(i)*8,8))return false;
        if(!object)continue;
        handanim::Slot slot{};handanim::read_slot(object,slot);watch(object);
        if(slot.known!=2047||slot.component!=value.mesh||!policy::pose_weight_zero(slot.blend)||slot.playing||
           slot.target_index!=0||slot.children<2)return quiet_reject(i,slot,-1,L"parent_not_quiet_or_unknown");
        for(int n=0;n<slot.children;++n){
            const auto& child=slot.child[size_t(n)];watch(child.object);
            if(n&&!policy::custom_child_zero_pose(child.known,child.weight,child.total,child.target,child.playing))
                return quiet_reject(i,slot,n,L"custom_child_not_quiet_or_unknown");
        }
        ++trial.quiet_slots;
    }
    // NodeTotalWeight is the parent pose weight; PendingBlendOutTime retains
    // durations/-1. Neither means that a custom child is still playing.
    return trial.quiet_slots>0;
}
bool matrix_pose(std::array<float,16> const& value,gears_vr::Viewpoint& out){
    gears_vr::Matrix matrix{};
    for(unsigned col=0;col<3;++col){
        double length{};
        for(unsigned row=0;row<3;++row)length+=double(value[col*4+row])*value[col*4+row];
        length=std::sqrt(length);if(!std::isfinite(length)||length<.9||length>1.1)return false;
        for(unsigned row=0;row<3;++row)matrix[row][col]=value[col*4+row]/length;
    }
    for(unsigned a=0;a<3;++a)for(unsigned b=a+1;b<3;++b){
        double dot{};for(unsigned row=0;row<3;++row)dot+=matrix[row][a]*matrix[row][b];
        if(std::abs(dot)>.01)return false;
    }
    const double determinant=matrix[0][0]*(matrix[1][1]*matrix[2][2]-matrix[1][2]*matrix[2][1])-
        matrix[0][1]*(matrix[1][0]*matrix[2][2]-matrix[1][2]*matrix[2][0])+
        matrix[0][2]*(matrix[1][0]*matrix[2][1]-matrix[1][1]*matrix[2][0]);
    if(determinant<.99||determinant>1.01)return false;
    out={{value[12],value[13],value[14]},gears_vr::ue_angles(matrix)};
    return gears_vr::finite(out.position);
}
void sample(const wchar_t* phase,Snapshot const& value,unsigned lease,unsigned mask){
    if(!log_file)return;
    LARGE_INTEGER stamp{};QueryPerformanceCounter(&stamp);
    fwprintf(log_file,L"hand_ik_stream_trial_sample phase=%ls lease=%u segment=%u sample=%u "
        L"sequence=%llu qpc=%lld ms=%llu world_time=%.9g last_render_time=%.9g "
        L"mesh_tick_known=%u mesh_tick=%d action_idle=%u action_epoch=%u "
        L"trial_apply_mask=%u ownership_proved=0",
        phase,lease,(lease-1)/8,trial.samples++,trial.sequence,stamp.QuadPart,value.ms,
        value.world_time,value.last_render_time,unsigned(value.mesh_tick_known),value.mesh_tick,
        unsigned(value.action_idle),trial.epoch,mask);
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    fwprintf(log_file,L" equipment_phase=%u",trial.xr_equipment_phase);
#endif
    freehands::print_matrix(L"mesh",value.mesh_world);freehands::print_matrix(L"left",value.left);
    freehands::print_matrix(L"right",value.right);
    for(unsigned i=0;i<4;++i){fwprintf(log_file,L" target%u=",i);
        for(const auto byte:trial.targets[i])fwprintf(log_file,L"%02x",unsigned(byte));}
    fwprintf(log_file,L"\n");fflush(log_file);
}
void end(unsigned code,const wchar_t* reason){
    if(trial.ended)return;
    trial.ended=true;trial.phase=0;requested=false;active_visible=false;
    // No terminal state authorizes another target. A partial acquisition must
    // never claim mask 3; dirty/faulted separately describe unresolved fields.
    const bool clean=!trial.lease.dirty();
    if(!clean)code=8;
    safe_to_unload.store(clean,std::memory_order_release);status.store(code,std::memory_order_release);
    if(log_file){fwprintf(log_file,L"hand_ik_stream_trial_end status=%u leases=%u restored_leases=%u "
        L"action_yields=%u immediate_action_test=%u original_melee_seen=%u original_slot_play_seen=%u "
        L"resumed=%u setter_calls=%u target_writes=%u "
        L"trial_apply_mask=%u dirty=%u faulted=%u safe_to_unload=%u ownership_proved=0 reason=%ls\n",
        code,leases.load(),trial.restored,trial.action_yields,unsigned(trial.immediate_test),
        unsigned(trial.melee_seen),unsigned(trial.slot_play_seen),unsigned(trial.resumed),
        trial.setter_calls,trial.target_writes,0u,unsigned(!clean),unsigned(trial.lease.faulted()),unsigned(clean),reason);
        fflush(log_file);}
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
    if(log_file){fwprintf(log_file,L"hand_ik_xr_stream_end fresh=%u missing=%u binding_resets=%u "
        L"bindings=%u left_leases=%u right_leases=%u both_leases=%u "
        L"external_frozen_native_epoch=%llu native_started_qpc=%llu "
        L"world_handoff_supported=0 ownership_proved=0\n",
        trial.xr_fresh,trial.xr_none,trial.xr_resets,trial.xr_binds,
        trial.xr_masks[1],trial.xr_masks[2],trial.xr_masks[3],
        trial.xr_start.expected_native_world_generation,trial.xr_start.native_started_qpc);
        fflush(log_file);}
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    if(log_file){fwprintf(log_file,L"hand_ik_equipment_end equipment_phase=%u phase_changes=%u "
        L"leases=%u dirty=%u faulted=%u ownership_proved=0\n",trial.xr_equipment_phase,
        trial.xr_equipment_changes,leases.load(),unsigned(trial.lease.dirty()),unsigned(trial.lease.faulted()));
        fflush(log_file);}
#endif
#endif
}
bool restore(const wchar_t* reason){
    if(!trial.lease.dirty())return true;
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    EquipmentTimingScope timing(EquipmentTiming::restore);
#endif
    trial.releasing=true;
    const auto lease=leases.load();Snapshot current{};
    const bool sampled=handik::snapshot(trial.lease_anchor.pc,current);
    if(sampled)sample(L"pre_restore",current,lease,trial.applied_mask);
    Backend backend{};
    const bool clean=trial.lease.restore(backend);trial.releasing=false;
    if(clean)++trial.restored;
    if(log_file){fwprintf(log_file,L"hand_ik_stream_trial_restore lease=%u dirty_before=1 dirty_after=%u "
        L"restored=%u reason=%ls\n",lease,unsigned(!clean),unsigned(clean),reason);fflush(log_file);}
    if(!clean||trial.lease.faulted()){end(clean?7:8,L"lease_restore_unverified_or_faulted");return false;}
    if(!sampled){end(7,L"restored_but_pre_restore_sample_unknown");return false;}
    return true;
}
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
// Game-thread hand journal readiness only. Stop/ended status must not prevent
// the equipment owner from completing its original attachment restoration.
bool equipment_ready(){return !trial.lease.dirty()&&!trial.lease.faulted();}
bool yield_for_equipment(const wchar_t* reason){
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return false;
    const bool dirty=trial.lease.dirty(),faulted=trial.lease.faulted();
    if(!used.load(std::memory_order_acquire)&&!dirty&&!faulted)return true;
    // Revoke geometry and all previously issued plans before rollback, even
    // when rollback fails. Never reopen a stopped trial or its write budget.
    ++trial.epoch;trial.stability.reset();trial.xr_ready=false;
    gears_hand_stream::Context blocked{};
    blocked.action_epoch=trial.epoch;blocked.observed_ms=GetTickCount64();
    blocked.clean=!dirty&&!faulted;
    observe_xr_policy(blocked,nullptr,blocked.observed_ms);
    for(auto& binding:trial.xr_binding){binding.reset();++trial.xr_resets;}
    if(!trial.ended&&!stop_requested.load(std::memory_order_acquire)&&
       active_visible.load(std::memory_order_acquire)&&(trial.phase==1||trial.phase==2)){
        trial.phase=1;status=1;
    }
    if(dirty)++trial.action_yields;
    const bool restored=!dirty||(!trial.releasing&&restore(reason?reason:L"equipment_boundary"));
    // A diagnostic sample failure can end the hand trial after a verified
    // clean rollback. It must not strand the equipment owner's attachment.
    const bool ready=equipment_ready();
    if(!trial.lease.dirty())trial.applied_mask=0;
    if(log_file){fwprintf(log_file,L"hand_ik_equipment_yield reason=%ls equipment_phase=%u "
        L"epoch=%u dirty_before=%u faulted_before=%u dirty_after=%u faulted_after=%u "
        L"restored=%u helper_clean=%u bindings_cleared=3 leases=%u stopped=%u ended=%u\n",
        reason?reason:L"equipment_boundary",trial.xr_equipment_phase,trial.epoch,unsigned(dirty),unsigned(faulted),
        unsigned(trial.lease.dirty()),unsigned(trial.lease.faulted()),unsigned(restored),unsigned(ready),
        leases.load(),unsigned(stop_requested.load(std::memory_order_acquire)),unsigned(trial.ended));
        fflush(log_file);}
    return ready;
}
#endif
bool related(uptr object){
    for(unsigned i=0;i<trial.action_object_count;++i)if(trial.action_objects[i]==object)return registered(object);
    if(!registered(object)||playground::deleted(object))return false;
    uptr pawn{};
    if(playground::instance(object,L"WarWeapon"))
        return handik::pointer(object,L"Instigator",pawn)&&pawn==trial.anchor.pawn;
    if(playground::instance(object,L"WarSpecialMove"))
        return handik::pointer(object,L"PawnOwner",pawn)&&pawn==trial.anchor.pawn;
    return false;
}
void before_action(uptr object,uptr function){
    // Reject other threads before touching any non-atomic game-thread state.
    if(!active_visible.load(std::memory_order_acquire)||
       GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return;
    if(trial.releasing||!trial.anchored||!registered(function)||!related(object))return;
    wchar_t label[96]{};if(!name(function,label,96))return;
    constexpr const wchar_t* actions[]={L"ServerUse",L"GrabPickup",L"ServerDictateSpecialMove",L"DoSpecialMove",
        L"EndSpecialMove",L"StartSpecialMove",L"StopSpecialMove",L"BodyStanceAnimEndNotification",L"OnAnimEnd",
        L"PlayBodyStance",L"PlayBodyStanceByDuration",L"SetBodyStance",L"StopBodyStance",L"StopAllBodyStances",
        L"StartMeleeAttack",L"StopMeleeAttack",L"ForceStopMeleeAttack",L"ForciblyStopMeleeAttack",
        L"ClientStopMeleeAttack",L"MeleeAttackStarted",L"MeleeAttackEnded",L"BeginState",L"EndState",
        L"SetWeaponTemporarilyHolstered",L"HolsterWeaponTemporarily",
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        L"SetWeaponFromSlot",L"SetCurrentWeapon",L"ChangedWeapon",L"ClientWeaponSet",
        L"AttachWeaponToHand",L"DetachWeapon",L"AttachWeaponToSlot",L"DetachWeaponFromSlot",
        L"AttachComponent",L"AttachComponentToSocket",L"DetachComponent",L"DetachComponentFromSocket",
        L"ForceReload",L"Died",L"Destroyed",
#endif
        L"PlayCustomAnim",L"PlayCustomAnimByDuration",L"StopCustomAnim"};
    bool watched=false;for(const auto item:actions)if(!wcscmp(item,label)){watched=true;break;}
    if(!watched)return;
    if(trial.phase==3&&(!wcscmp(label,L"PlayCustomAnim")||!wcscmp(label,L"PlayCustomAnimByDuration")))
        trial.slot_play_seen=true;
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
    if(!wcscmp(label,L"StartMeleeAttack"))trial.melee_seen=true;
    if(!wcscmp(label,L"PlayCustomAnim")||!wcscmp(label,L"PlayCustomAnimByDuration"))trial.slot_play_seen=true;
#endif
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    const bool dirty=trial.lease.dirty();
    const bool restored=yield_for_equipment(label);
    if(log_file){fwprintf(log_file,L"hand_ik_stream_trial_action function=%ls epoch=%u "
        L"dirty_before=%u dirty_after=%u restored=%u equipment_phase=%u\n",label,trial.epoch,
        unsigned(dirty),unsigned(trial.lease.dirty()),unsigned(restored),trial.xr_equipment_phase);fflush(log_file);}
#else
    ++trial.epoch;trial.stability.reset();
    const bool dirty=trial.lease.dirty();
    if(dirty){
        ++trial.action_yields;
        const bool restored=restore(L"action_boundary");
        if(log_file){fwprintf(log_file,L"hand_ik_stream_trial_action function=%ls epoch=%u "
            L"dirty_before=1 dirty_after=%u restored=%u\n",label,trial.epoch,
            unsigned(trial.lease.dirty()),unsigned(restored));fflush(log_file);}
        if(!restored)return;
#if !defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
        if(!trial.immediate_pending)end(7,L"unexpected_action_yield");
#endif
    }
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
    // Block the old plan synchronously, even when the observer log is full.
    gears_hand_stream::Context blocked{};
    blocked.action_epoch=trial.epoch;blocked.observed_ms=GetTickCount64();blocked.clean=!trial.lease.dirty();
    observe_xr_policy(blocked,nullptr,blocked.observed_ms);
    for(auto& binding:trial.xr_binding)binding.reset();
    trial.xr_ready=false;trial.phase=1;status=1;
#endif
#endif
}
bool melee_signature(uptr pawn,bool stop){
    playground::Args call(pawn,stop?L"StopMeleeAttack":L"StartMeleeAttack");
    if(!call.valid||get<unsigned short>(call.r.function+0x108)!=0xffff||
       handik::parameter_count(call)!=(stop?2u:0u))return false;
    if(!stop)return call.r.size==0;
    return call.r.size<=8&&freehands::argument(call,L"bForcedByDamage",L"BoolProperty",4)&&
        freehands::argument(call,L"bForceStopFire",L"BoolProperty",4);
}
#if !defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
bool make_targets(Snapshot const& value){
    gears_weapon::FreeHandGameState game{};unsigned known{};std::uint8_t special{};
    if(!freehands::game_state(value.pc,game,known,special)||known!=15)return false;
    // Synthetic *bone-world* poses exercise the same validated target consumer.
    // The per-hand idle binding is temporary, not a physical palm calibration.
    gears_weapon::HandWorldFrame frame{};
    frame.session=1;frame.sequence=++trial.sequence;frame.space_epoch=1;frame.input_epoch=1;
    frame.world_generation=1;frame.created_ms=GetTickCount64();frame.controller=value.pc;
    frame.pawn=value.pawn;frame.weapon=value.weapon;frame.units_per_metre=100;frame.valid_mask=3;
    frame.left=trial.bind[0];frame.right=trial.bind[1];
    const unsigned segment=leases.load()/8;const double sign=(segment&1)?-1.:1.;
    if(segment<2){
        frame.right.position.x+=sign;frame.right.position.y+=sign*.5;frame.right.position.z+=sign*.75;
        frame.left.position.x-=sign*.75;frame.left.position.y+=sign;frame.left.position.z+=sign*.5;
    }else{
        frame.right.rotation.pitch+=sign*.02;frame.right.rotation.yaw+=sign*.03;frame.right.rotation.roll-=sign*.025;
        frame.left.rotation.pitch-=sign*.03;frame.left.rotation.yaw-=sign*.02;frame.left.rotation.roll+=sign*.015;
    }
    const auto targets=trial.consumer.advance(game,&frame,frame.created_ms,true);
    if(targets.tracked_mask!=3||targets.sequence!=frame.sequence||targets.apply_mask||targets.eligible_mask||
       targets.phase!=gears_weapon::FreeHandPhase::unproved_animation_state)return false;
    for(unsigned hand=0;hand<2;++hand){
        const auto& pose=targets.world[hand?0:1]; // Channels are right, then left.
        handik::Triple position{{float(pose.position.x),float(pose.position.y),float(pose.position.z)}};
        if(!gears_hand_ik::finite(position))return false;
        const playground::Rotation rotation{gears_vr::to_rotator_units(pose.rotation.pitch),
            gears_vr::to_rotator_units(pose.rotation.yaw),gears_vr::to_rotator_units(pose.rotation.roll)};
        static_assert(sizeof(rotation)==12&&sizeof(position)==12);
        std::memcpy(trial.targets[hand*2].data(),position.data(),12);
        std::memcpy(trial.targets[hand*2+1].data(),&rotation,12);
    }
    return true;
}
void immediate_action(){
    const bool dirty=trial.lease.dirty();const auto yields=trial.action_yields;
    trial.immediate_pending=true;trial.action_ms=GetTickCount64();trial.phase=3;status=3;
    const bool invoked=playground::Args(trial.anchor.pawn,L"StartMeleeAttack").run();
    trial.immediate_pending=false;
    const bool yielded=trial.action_yields==yields+1&&!trial.lease.dirty();
    trial.immediate_test=dirty&&yielded&&invoked;
    if(log_file){fwprintf(log_file,L"hand_ik_stream_trial_action_dispatch_test dirty_before=%u "
        L"dirty_after=%u yielded=%u invoked=%u\n",unsigned(dirty),unsigned(trial.lease.dirty()),
        unsigned(yielded),unsigned(invoked));fflush(log_file);}
    if(!trial.immediate_test)end(7,L"immediate_action_handback_failed");
}
void pre_tick(uptr){
    if(!active_visible.load(std::memory_order_acquire)||trial.ended)return;
    if(trial.lease.dirty()&&!restore(L"pre_tick"))return;
    if(stop_requested.load(std::memory_order_acquire)){end(6,L"stopped_after_restore");return;}
    trial.tick_epoch=trial.epoch;
    if(leases.load()==32&&trial.phase==2){trial.phase=4;trial.final_ms=GetTickCount64();}
    if(trial.phase==3&&!trial.stop_sent&&GetTickCount64()-trial.action_ms>=1400){
        trial.stop_sent=true;
        const bool ok=playground::Args(trial.anchor.pawn,L"StopMeleeAttack")
            .flag(L"bForcedByDamage",false).flag(L"bForceStopFire",false).run();
        if(!ok)end(7,L"original_melee_stop_failed");
    }
}
void post_tick(uptr pc){
    if(requested.exchange(false,std::memory_order_acq_rel)){
        trial={};trial.phase=1;trial.began=GetTickCount64();trial.deadline=trial.began+30000;
        if(log_file){fwprintf(log_file,L"hand_ik_stream_trial_begin source=synthetic_bone_world trajectory=all_axes "
            L"max_leases=32 head_controllers_untouched=1 force_evaluation=0 ownership_proved=0\n");fflush(log_file);}
    }
    if(!active_visible.load(std::memory_order_acquire)||trial.ended||!trial.phase)return;
    const auto now=GetTickCount64();
    if(stop_requested.load(std::memory_order_acquire)){end(6,L"stopped_in_clean_phase");return;}
    if(now>trial.deadline){end(7,L"bounded_trial_timeout");return;}
    if(trial.phase==4){
        if(now-trial.final_ms>=250)end(5,L"bounded_dual_hand_recording_complete");
        return;
    }
    if(trial.phase==3){bool melee{};
        if(freehands::read_bool(trial.anchor.pawn,L"bDoingMeleeAttack",melee)&&melee)trial.melee_seen=true;
    }
    Snapshot value{};const bool snapshot_ok=handik::snapshot(pc,value);
    const bool quiet=snapshot_ok&&slots_quiet(value);
    const bool qualified=snapshot_ok&&value.mesh_tick_known&&value.action_idle&&
        handik::baseline_strengths(value)&&quiet&&(!trial.anchored||same_layout(trial.anchor,value));
    if(trial.phase==3&&!trial.stop_sent)return;
    if(!qualified||trial.tick_epoch!=trial.epoch){
        trial.stability.reset();
        if(trial.phase==2){end(7,L"stream_state_changed_after_restore");return;}
        if(log_file&&now>=trial.next_wait){
            fwprintf(log_file,L"hand_ik_stream_trial_wait snapshot_ok=%u idle=%u quiet=%u quiet_slots=%u "
                L"mesh_tick_known=%u epoch_changed=%u writes=0\n",unsigned(snapshot_ok),unsigned(value.action_idle),
                unsigned(quiet),trial.quiet_slots,unsigned(value.mesh_tick_known),unsigned(trial.tick_epoch!=trial.epoch));
            fflush(log_file);trial.next_wait=now+250;
        }
        trial.previous=value;return;
    }
    if(trial.phase==1||trial.phase==3){
        if(value.mesh_tick==trial.previous.mesh_tick)return;
        const bool stable=trial.stability.observe(now,true,handik::same_controls(trial.previous,value));
        trial.previous=value;if(!stable)return;
        if(!melee_signature(value.pawn,false)||!melee_signature(value.pawn,true)){
            end(7,L"original_melee_signature_unknown");return;
        }
        for(unsigned channel=0;channel<4;++channel)
            if(!target_layout(value.controls[indices[channel]].object,(channel&1)!=0)){
                end(7,L"target_abi_unknown");return;
            }
        if(!matrix_pose(value.left,trial.bind[0])||!matrix_pose(value.right,trial.bind[1])){
            end(7,L"temporary_hand_binding_unknown");return;
        }
        if(trial.phase==3){
            if(!trial.melee_seen||!trial.slot_play_seen){end(7,L"original_melee_animation_not_observed");return;}
            trial.resumed=true;
        }
        trial.anchor=value;trial.anchored=true;trial.phase=2;status=2;
    }
    if(!make_targets(value)){end(7,L"pose_consumer_rejected");return;}
    const auto lease=leases.load()+1;
    sample(L"before",value,lease,0);
    Snapshot check{};
    if(!handik::snapshot(pc,check)||!check.action_idle||!handik::baseline_strengths(check)||
       !handik::same_controls(value,check)||value.mesh_world!=check.mesh_world||
       value.left!=check.left||value.right!=check.right||!slots_quiet(check)||trial.tick_epoch!=trial.epoch){
        end(7,L"prewrite_state_changed");return;
    }
    if(!log_file||ferror(log_file)){end(7,L"log_unavailable_before_write");return;}
    trial.lease_anchor=check;Backend backend{};
    if(!trial.lease.acquire(trial.targets,1.f,backend)){
        end(trial.lease.dirty()?8:7,L"lease_acquisition_failed");return;
    }
    trial.applied_mask=3;
    leases=lease;
    Snapshot written{};
    if(!handik::snapshot(pc,written)||!same_layout(check,written)){
        restore(L"failed_written_snapshot");end(7,L"written_snapshot_unknown");return;
    }
    sample(L"written",written,lease,3);
    if(lease==16)immediate_action();
}
#else
#include "vr_hand_ik_xr_stream_trial.inl"
#endif
void reset(){
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
    xr_bridge.close();xr_pending={};
#endif
    trial={};requested=false;stop_requested=false;used=false;shutdown_requested=false;
    active_visible=false;safe_to_unload=true;status=0;leases=0;handik::reset();
}
DWORD stop_and_wait(){
    stop_requested.store(true,std::memory_order_release);
    if(game_thread.load(std::memory_order_acquire)==GetCurrentThreadId())return 41;
    const auto ready=[](){return safe_to_unload.load(std::memory_order_acquire)&&
        !requested.load(std::memory_order_acquire)&&!active_visible.load(std::memory_order_acquire);};
    const auto until=GetTickCount64()+2000;
    while(!ready()&&status.load(std::memory_order_acquire)!=8&&GetTickCount64()<until)Sleep(1);
    return ready()?0:40;
}
} // namespace handstream
