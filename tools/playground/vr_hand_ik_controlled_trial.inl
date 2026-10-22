// Default-off, private recording candidate. Reuse only the reviewed original
// controller reflection, snapshots and compare-before-write helpers in handik.
// No free-hand/XR writer, animation evaluation or render-flag override is used.
namespace handikcontrolled {
namespace policy=gears_hand_ik_controlled;
using handik::Snapshot;
using handik::Triple;
constexpr unsigned right_ik=handik::right_ik;
std::atomic<bool> requested{},stop_requested{},used{},safe_to_unload{true},shutdown_requested{},active_visible{};
std::atomic<unsigned> status{},samples{};
struct Trend {
    policy::ReadOnlyTrend qualification;
    Snapshot first{},last{};
    unsigned first_sample{},last_sample{};
    void observe(Snapshot const& value){
        const auto sample_index=samples.load()-1;
        if(!qualification.summary().samples){first=value;first_sample=sample_index;}
        last=value;last_sample=sample_index;
        qualification.observe(value.ms,value.world_time,value.last_render_time,
            value.mesh_tick_known,value.mesh_tick,true);
    }
};
struct Trial {
    unsigned phase{},window{}; // 1 initial wait/qualification, 2 pulse, 3 read-only recovery.
    Snapshot anchor{},before{};
    bool anchored{},dirty{},ever_written{},cancel_after_restore{};
    gears_hand_ik::StabilityGate stability;
    gears_hand_ik::OwnedWrite owned;
    Trend natural;
    policy::WindowEvidence window_evidence;
    policy::SequenceEvidence evidence;
    std::uint64_t began_ms{},wait_deadline{},wrote_ms{},write_tick{};
    std::uint64_t next_wait_log_ms{};
    unsigned wait_observations{},wait_snapshot_unknown{},wait_nonidle{},wait_strengths{};
    unsigned pc_tick_enter{},pc_tick_exit{},pawn_tick_enter{},pawn_tick_exit{};
};
Trial trial{};
const wchar_t* label(unsigned window){
    return window==0||window==5?L"S":policy::offset(window)>0?L"+X":L"-X";
}
bool same_world(Snapshot const& a,Snapshot const& b){
    return std::memcmp(a.mesh_world.data(),b.mesh_world.data(),sizeof(a.mesh_world))==0;
}
bool rollback_identity(Snapshot const& value){
    const auto& anchor=trial.anchor;
    // Keep all observed object/mesh/tree/list identities and the entire right
    // controller layout fixed. Other controllers' dynamic values belong to the
    // original game and cannot make our still-owned two fields unrestorable.
    return value.pc==anchor.pc&&value.pawn==anchor.pawn&&value.mesh==anchor.mesh&&
        value.tree==anchor.tree&&value.weapon==anchor.weapon&&value.identity==anchor.identity&&
        value.controls[right_ik].object==anchor.controls[right_ik].object&&
        value.controls[right_ik].fixed==anchor.controls[right_ik].fixed;
}
bool baseline(Snapshot const& value){
    return value.action_idle&&handik::baseline_strengths(value)&&
        (!trial.anchored||(handik::same_controls(trial.anchor,value)&&same_world(trial.anchor,value)));
}
bool read_only_expired(std::uint64_t now){
    return now<trial.began_ms||now>trial.wait_deadline||
        now-trial.began_ms>policy::total_limit_ms||samples.load()>=policy::maximum_samples;
}
void initial_wait_diagnostic(bool snapshot_ok,Snapshot const& value,std::uint64_t now){
    ++trial.wait_observations;
    if(!snapshot_ok)++trial.wait_snapshot_unknown;
    else{
        if(!value.action_idle)++trial.wait_nonidle;
        if(!handik::baseline_strengths(value))++trial.wait_strengths;
    }
    // The wait is read-only and finite. Limit detail rows to one per 250 ms;
    // these rows do not count toward samples or stability qualification.
    if(!log_file||now<trial.next_wait_log_ms)return;
    trial.next_wait_log_ms=now+gears_hand_ik::minimum_stable_ms;
    unsigned mismatches{};
    if(snapshot_ok)for(unsigned i=0;i<8;++i){
        const auto expected=(i%4)<2?1.f:0.f;
        if(!gears_hand_ik::exact(value.controls[i].strength,Triple{{expected,expected,0}}))mismatches|=1u<<i;
    }
    const bool strengths_ok=snapshot_ok&&!mismatches;
    const wchar_t* reason=!snapshot_ok?L"snapshot_unknown":
        !value.action_idle?(strengths_ok?L"action_not_idle":L"action_and_strengths_not_ready"):
        L"strengths_not_baseline";
    fwprintf(log_file,L"hand_ik_controlled_trial_wait observation=%u ms=%llu elapsed_ms=%llu deadline_ms=%llu "
        L"snapshot_ok=%u action_idle_known=%u action_idle=%u strengths_known=%u baseline_strengths=%u "
        L"strength_mismatch_mask=%u reason=%ls stable_ticks=0 writes=0",
        trial.wait_observations,now,now-trial.began_ms,trial.wait_deadline,
        unsigned(snapshot_ok),unsigned(snapshot_ok),unsigned(snapshot_ok&&value.action_idle),
        unsigned(snapshot_ok),unsigned(strengths_ok),mismatches,reason);
    if(snapshot_ok)for(unsigned i=0;i<8;++i){
        const auto& strength=value.controls[i].strength;
        fwprintf(log_file,L" strength%u=%.9g,%.9g,%.9g",i,strength[0],strength[1],strength[2]);
    }
    fwprintf(log_file,L"\n");fflush(log_file);
}
void initial_wait_summary(const wchar_t* outcome,std::uint64_t now){
    if(!log_file)return;
    fwprintf(log_file,L"hand_ik_controlled_trial_wait_end outcome=%ls ms=%llu elapsed_ms=%llu "
        L"observations=%u snapshot_unknown=%u nonidle=%u strengths_not_baseline=%u "
        L"stable_ticks=0 writes=0\n",outcome,now,now>=trial.began_ms?now-trial.began_ms:0,
        trial.wait_observations,trial.wait_snapshot_unknown,trial.wait_nonidle,trial.wait_strengths);
    fflush(log_file);
}
void sample(const wchar_t* phase,Snapshot const& value){
    if(!log_file)return;
    LARGE_INTEGER stamp{};QueryPerformanceCounter(&stamp);
    // Optional reflected Actor tag is telemetry only. It is never guessed from
    // a native offset, and an unavailable tag is explicitly recorded as unknown.
    int pawn_tag{};
    const bool pawn_tag_known=freehands::read_field(value.pawn,L"TickTag",L"IntProperty",&pawn_tag,sizeof(pawn_tag));
    const auto& control=value.controls[right_ik];
    fwprintf(log_file,L"hand_ik_controlled_trial_sample window=%u label=%ls phase=%ls sample=%u "
        L"qpc=%lld ms=%llu world_time=%.9g last_render_time=%.9g "
        L"pc=%p pawn=%p mesh=%p tree=%p weapon=%p control=%p "
        L"player_tick_enter=%u player_tick_exit=%u pawn_tick_enter=%u pawn_tick_exit=%u "
        L"pawn_tag_known=%u pawn_tag=%d mesh_tick_known=%u mesh_tick=%d "
        L"control_tag_known=%u control_tag=%u action_idle=%u "
        L"strength=%.9g target=%.9g blend=%.9g effector=%.9g,%.9g,%.9g stable_ticks=%u",
        trial.window,label(trial.window),phase,samples.fetch_add(1),stamp.QuadPart,value.ms,
        value.world_time,value.last_render_time,reinterpret_cast<void*>(value.pc),
        reinterpret_cast<void*>(value.pawn),reinterpret_cast<void*>(value.mesh),reinterpret_cast<void*>(value.tree),
        reinterpret_cast<void*>(value.weapon),reinterpret_cast<void*>(control.object),
        trial.pc_tick_enter,trial.pc_tick_exit,trial.pawn_tick_enter,trial.pawn_tick_exit,
        unsigned(pawn_tag_known),pawn_tag,unsigned(value.mesh_tick_known),value.mesh_tick,
        unsigned(value.control_tag_known),unsigned(value.control_tick_tag),unsigned(value.action_idle),
        control.strength[0],control.strength[1],control.strength[2],
        control.effector[0],control.effector[1],control.effector[2],trial.stability.ticks());
    freehands::print_matrix(L"mesh",value.mesh_world);freehands::print_matrix(L"left",value.left);
    freehands::print_matrix(L"right",value.right);fwprintf(log_file,L"\n");fflush(log_file);
}
void natural_summary(const wchar_t* side,Trend const& trend){
    if(!log_file)return;
    const auto& value=trend.qualification.summary();
    const auto first_right=handik::position(trend.first.right),last_right=handik::position(trend.last.right);
    const auto first_left=handik::position(trend.first.left),last_left=handik::position(trend.last.left);
    fwprintf(log_file,L"hand_ik_controlled_trial_natural window=%u label=%ls side=%ls "
        L"first_sample=%u last_sample=%u samples=%u began_ms=%llu ended_ms=%llu "
        L"valid=%u mesh_tick_known=%u distinct_mesh_ticks=%u mesh_tick_progress=%u render_progress=%u complete=%u "
        L"right_delta=%.9g,%.9g,%.9g left_delta=%.9g,%.9g,%.9g writes=0\n",
        trial.window,label(trial.window),side,trend.first_sample,trend.last_sample,value.samples,
        value.began,value.ended,unsigned(value.valid),unsigned(value.mesh_tick_known),value.distinct_mesh_ticks,
        unsigned(value.mesh_tick_progress),unsigned(value.render_progress),unsigned(value.complete()),
        last_right[0]-first_right[0],last_right[1]-first_right[1],last_right[2]-first_right[2],
        last_left[0]-first_left[0],last_left[1]-first_left[1],last_left[2]-first_left[2]);
    fflush(log_file);
}
void end(unsigned code,const wchar_t* reason,bool safe){
    // A dirty or unverified target is never repaired by another thread.
    safe=safe&&!trial.dirty;trial.phase=0;requested=false;
    active_visible.store(false,std::memory_order_release);
    safe_to_unload.store(safe,std::memory_order_release);status.store(code,std::memory_order_release);
    if(log_file){
        fwprintf(log_file,L"hand_ik_controlled_trial_end status=%u completed_windows=%u "
            L"evidence_complete=%u restored=%u setter_calls=%u effector_writes=%u "
            L"effect_evidence=0 inconclusive=1 safe_to_unload=%u reason=%ls\n",
            code,trial.evidence.completed(),unsigned(trial.evidence.complete()),
            unsigned(trial.ever_written&&!trial.dirty),handik::trial.setter_calls,
            handik::trial.effector_writes,unsigned(safe),reason);fflush(log_file);
    }
}
void reset(){
    trial={};requested=false;stop_requested=false;used=false;shutdown_requested=false;
    active_visible=false;safe_to_unload=true;status=0;samples=0;
    handik::reset();
}
// This scope precedes the normal automatic-mode and recursion filters. It
// records actual dispatched Pawn Tick / PlayerTick calls without invoking one.
// Absence of Pawn Tick callbacks is observable, not fabricated as mesh progress.
struct TickScope {
    unsigned kind{};uptr object{};
    TickScope(uptr value,uptr function):object(value){
        if(!enabled.load(std::memory_order_acquire)||!active_visible.load(std::memory_order_acquire)||
           GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire)||!trial.anchored||
           !registered(value)||!registered(function))return;
        if(value==trial.anchor.pc&&playground::named(function,L"PlayerTick"))kind=1;
        if(value==trial.anchor.pawn&&playground::named(function,L"Tick"))kind=2;
        if(!kind)return;
        if(kind==1)++trial.pc_tick_enter;else ++trial.pawn_tick_enter;
        emit(L"pre");
    }
    void emit(const wchar_t* phase){
        if(!log_file)return;
        LARGE_INTEGER stamp{};QueryPerformanceCounter(&stamp);
        fwprintf(log_file,L"hand_ik_controlled_trial_tick window=%u label=%ls kind=%ls phase=%ls "
            L"object=%p qpc=%lld ms=%llu thread=%u player_tick_enter=%u player_tick_exit=%u "
            L"pawn_tick_enter=%u pawn_tick_exit=%u writes=0\n",trial.window,label(trial.window),
            kind==1?L"PlayerTick":L"PawnTick",phase,reinterpret_cast<void*>(object),stamp.QuadPart,
            GetTickCount64(),GetCurrentThreadId(),trial.pc_tick_enter,trial.pc_tick_exit,
            trial.pawn_tick_enter,trial.pawn_tick_exit);fflush(log_file);
    }
    ~TickScope(){
        if(!kind)return;
        if(kind==1)++trial.pc_tick_exit;else ++trial.pawn_tick_exit;
        emit(L"post");
    }
};
void arm_window(uptr pc,Snapshot const& current){
    trial.before=current;trial.owned={};trial.owned.original_target=current.controls[right_ik].effector;
    trial.cancel_after_restore=false;
    trial.window_evidence={};trial.window_evidence.before=trial.natural.qualification.summary();
    if(!policy::target(trial.window,handik::position(current.right),trial.owned.written_target)){
        end(7,L"target_unknown",true);return;
    }
    natural_summary(L"before",trial.natural);sample(L"before",current);
    Snapshot check{};
    if(!handik::snapshot(pc,check)||!baseline(check)||!handik::same_controls(current,check)||
       !same_world(current,check)||std::memcmp(current.left.data(),check.left.data(),sizeof(current.left))||
       std::memcmp(current.right.data(),check.right.data(),sizeof(current.right))){
        end(7,L"prewrite_state_changed",true);return;
    }
    if(!log_file||ferror(log_file)){end(7,L"log_unavailable_before_write",true);return;}
    const auto write_time=GetTickCount64();
    if(read_only_expired(write_time)){end(7,L"read_only_qualification_timeout_or_sample_limit",true);return;}
    // Install the owned record before the first potentially partial write.
    trial.phase=2;trial.dirty=true;trial.ever_written=true;trial.wrote_ms=write_time;
    trial.write_tick=trial.pc_tick_enter;status.store(2,std::memory_order_release);
    if(!handik::write_effector(check.controls[right_ik].object,trial.owned.original_target,trial.owned.written_target)){
        end(8,L"effector_write_unverified",false);return;
    }
    if(!handik::set_strength(check.controls[right_ik].object,gears_hand_ik::trial_strength)){
        end(8,L"strength_call_failed",false);return;
    }
    Snapshot written{};
    if(!handik::snapshot(pc,written)||!rollback_identity(written)||
       !trial.owned.can_restore(written.controls[right_ik].strength,written.controls[right_ik].effector)){
        end(8,L"written_state_unexpected",false);return;
    }
    trial.cancel_after_restore=!written.action_idle||!handik::same_controls(trial.anchor,written,true)||
        !same_world(trial.anchor,written);
    trial.window_evidence.strength_verified=true;sample(L"written",written);
}
void pre_tick(uptr pc){
    if(status.load(std::memory_order_acquire)==8)return;
    if(stop_requested.load(std::memory_order_acquire)&&trial.phase!=2&&
       (trial.phase||requested.load(std::memory_order_acquire))){
        end(6,L"stopped_in_read_only_phase",true);return;
    }
    if(trial.phase!=2)return;
    Snapshot current{};
    // An identity or owned-value change relinquishes ownership immediately.
    // A new action or moving mesh cancels later windows after a verified restore
    // while these exact original fields are still owned by this trial.
    if(!handik::snapshot(pc,current)||!rollback_identity(current)||
       !trial.owned.can_restore(current.controls[right_ik].strength,current.controls[right_ik].effector)){
        end(8,L"ownership_changed_no_rollback",false);return;
    }
    sample(L"pre_restore",current);
    auto& evidence=trial.window_evidence;
    evidence.pulse_in_time=gears_hand_ik::pulse_in_time(trial.wrote_ms,current.ms)&&
        trial.pc_tick_enter==trial.write_tick+1;
    evidence.mesh_tick_progress=trial.before.mesh_tick_known&&current.mesh_tick_known&&
        trial.before.mesh_tick!=current.mesh_tick;
    evidence.render_progress=current.last_render_time>trial.before.last_render_time&&
        current.last_render_time>0&&current.world_time>=current.last_render_time&&
        current.world_time-current.last_render_time<=.1f;
    evidence.same_mesh_world=same_world(trial.before,current);
    if(!handik::set_strength(current.controls[right_ik].object,0.f)){
        end(8,L"zero_strength_call_failed",false);return;
    }
    handik::Control zeroed{};
    if(!handik::read_control(current.pawn,current.mesh,current.tree,right_ik,zeroed)||
       zeroed.fixed!=current.controls[right_ik].fixed||!gears_hand_ik::zero_strength(zeroed.strength)||
       !gears_hand_ik::exact(zeroed.effector,trial.owned.written_target)||
       !handik::write_effector(zeroed.object,trial.owned.written_target,trial.owned.original_target)){
        end(8,L"zero_or_target_restore_failed",false);return;
    }
    Snapshot restored{};
    if(!handik::snapshot(pc,restored)||!rollback_identity(restored)||
       !gears_hand_ik::exact(restored.controls[right_ik].strength,trial.anchor.controls[right_ik].strength)||
       !gears_hand_ik::exact(restored.controls[right_ik].effector,trial.owned.original_target)){
        end(8,L"restore_verification_failed",false);return;
    }
    trial.dirty=false;evidence.restore_verified=true;sample(L"restored",restored);
    if(!evidence.pulse_in_time){end(7,L"next_tick_or_400ms_deadline_missed",true);return;}
    if(trial.cancel_after_restore||!current.action_idle||!handik::same_controls(trial.anchor,current,true)||
       !same_world(trial.anchor,current)||!baseline(restored)){
        end(7,L"restored_but_original_baseline_changed",true);return;
    }
    if(stop_requested.load(std::memory_order_acquire)){end(6,L"stopped_after_verified_restore",true);return;}
    trial.phase=3;status.store(3,std::memory_order_release);
    trial.wait_deadline=restored.ms+policy::wait_limit_ms;
    trial.stability.reset();trial.stability.observe(restored.ms,true,true);
    trial.natural={};trial.natural.observe(restored);
}
void post_tick(uptr pc){
    if(status.load(std::memory_order_acquire)==8)return;
    if(requested.exchange(false,std::memory_order_acq_rel)){
        trial={};trial.phase=1;trial.began_ms=GetTickCount64();
        trial.wait_deadline=trial.began_ms+policy::wait_limit_ms;
        active_visible.store(true,std::memory_order_release);
        if(log_file){fwprintf(log_file,L"hand_ik_controlled_trial_begin sequence=S,+X,-X,-X,+X,S "
            L"windows=6 strength=%.9g offset_units=%.9g stable_ticks=8 stable_ms=250 "
            L"pulse_limit_ms=400 recovery_ms=250 total_limit_ms=%llu apply_mask=0 effect_policy=offline_inconclusive\n",
            gears_hand_ik::trial_strength,gears_hand_ik::trial_offset_units,policy::total_limit_ms);fflush(log_file);}
    }
    if(!trial.phase||trial.phase==2)return;
    if(stop_requested.load(std::memory_order_acquire)){end(6,L"stopped_in_read_only_phase",true);return;}
    const bool initial_wait=trial.phase==1&&!trial.anchored;
    const auto reject_timeout=[&](std::uint64_t now){
        if(!read_only_expired(now))return false;
        if(initial_wait)initial_wait_summary(L"timeout",now);
        else if(log_file){
            const auto& qualification=trial.natural.qualification.summary();
            fwprintf(log_file,L"hand_ik_controlled_trial_qualification_timeout phase=%u samples=%u "
                L"distinct_mesh_ticks=%u mesh_tick_known=%u mesh_tick_progress=%u render_progress=%u "
                L"span_ms=%llu writes=0\n",trial.phase,qualification.samples,qualification.distinct_mesh_ticks,
                unsigned(qualification.mesh_tick_known),unsigned(qualification.mesh_tick_progress),
                unsigned(qualification.render_progress),qualification.ended>=qualification.began?
                    qualification.ended-qualification.began:0);fflush(log_file);
        }
        end(7,initial_wait?L"initial_baseline_wait_timeout":L"read_only_qualification_timeout_or_sample_limit",true);
        return true;
    };
    if(reject_timeout(GetTickCount64()))return;
    Snapshot current{};
    const bool snapshot_ok=handik::snapshot(pc,current);
    const auto observed_time=GetTickCount64();
    if(reject_timeout(observed_time))return;
    if(!snapshot_ok||!baseline(current)){
        if(initial_wait){
            initial_wait_diagnostic(snapshot_ok,current,observed_time);
            if(!log_file||ferror(log_file))end(7,L"log_unavailable_in_initial_wait",true);
            return;
        }
        end(7,L"baseline_identity_action_or_value_changed",true);return;
    }
    if(initial_wait){
        initial_wait_summary(L"baseline_ready",observed_time);
        trial.anchor=current;trial.anchored=true;
        // Start the eight-Tick / 250 ms qualification only after the original
        // action and all strengths meet baseline. Keep began_ms unchanged so
        // the initial wait consumes the existing overall 15-second budget.
        trial.wait_deadline=observed_time+policy::wait_limit_ms;
        trial.stability.reset();trial.natural={};
    }
    sample(trial.phase==1?L"pre_natural":L"post_natural",current);
    trial.natural.observe(current);
    const bool stable=trial.stability.observe(current.ms,true,true);
    if(!log_file||ferror(log_file)){end(7,L"log_unavailable_in_read_only_phase",true);return;}
    if(!stable||!trial.natural.qualification.summary().complete())return;
    if(trial.phase==3){
        trial.window_evidence.after=trial.natural.qualification.summary();
        natural_summary(L"after",trial.natural);
        if(!trial.evidence.append(trial.window,trial.window_evidence)){
            end(7,L"window_sequence_invalid",true);return;
        }
        if(log_file){fwprintf(log_file,L"hand_ik_controlled_trial_window window=%u label=%ls "
            L"offset_x=%d strength_verified=%u restore_verified=%u pulse_in_time=%u "
            L"mesh_tick_progress=%u render_progress=%u same_mesh_world=%u evidence_complete=%u "
            L"effect_evidence=0 inconclusive=1\n",trial.window,label(trial.window),policy::offset(trial.window),
            unsigned(trial.window_evidence.strength_verified),unsigned(trial.window_evidence.restore_verified),
            unsigned(trial.window_evidence.pulse_in_time),unsigned(trial.window_evidence.mesh_tick_progress),
            unsigned(trial.window_evidence.render_progress),unsigned(trial.window_evidence.same_mesh_world),
            unsigned(trial.window_evidence.complete()));fflush(log_file);}
        if(trial.window+1==policy::window_count){end(5,L"six_windows_recorded_offline_analysis_required",true);return;}
        ++trial.window;
        // The preceding >=250 ms read-only recovery is the next window's
        // natural baseline. No extra setter or synthetic evaluation is added.
    }
    arm_window(pc,current);
}
DWORD stop_and_wait(){
    stop_requested.store(true,std::memory_order_release);
    if(game_thread.load(std::memory_order_acquire)==GetCurrentThreadId())return 41;
    const auto unload_ready=[](){return gears_hand_ik::unload_allowed(
        safe_to_unload.load(std::memory_order_acquire),requested.load(std::memory_order_acquire),
        status.load(std::memory_order_acquire));};
    const auto until=GetTickCount64()+2000;
    while(!unload_ready()&&status.load(std::memory_order_acquire)!=8&&GetTickCount64()<until)Sleep(1);
    return unload_ready()?0:40;
}
} // namespace handikcontrolled
