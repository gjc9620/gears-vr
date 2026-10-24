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
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
bool counters_saturated{};
#endif
template<class T> void count_add(T& value,T amount=T{1}){
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    if(!gears_hand_session::saturating_add(value,amount))counters_saturated=true;
#else
    value+=amount;
#endif
}
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
    void add(std::uint64_t value){count_add(count);count_add(sum,value);maximum=(std::max)(maximum,value);}
};
struct EquipmentAgeStat {
    EquipmentTimingStat past;
    std::uint64_t future{};
    void add(gears_hand_stream::ObserveAge const& age){
        if(age.known){if(age.future)count_add(future);else past.add(age.ms);}
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
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
namespace reach_diagnostic {
// Session-owned POD only. Static Marcus parent edges are NOT live verified.
constexpr unsigned capacity=16;
struct Key {
    uptr object{},outer{},klass{};
    unsigned index{},name_index{},name_number{},reserved{};
};
enum class Status:unsigned {not_attempted,unsafe_context,snapshot_unknown,identity_changed,
    context_changed,index_unknown,index_mismatch,matrix_unknown,end_unknown,tick_changed,complete,matrix_nonrigid};
struct Row {
    std::array<Key,7> keys{}; // World, PC, pawn, mesh, tree, weapon, asset.
    std::array<std::array<float,16>,4> bones{};
    std::array<float,16> mesh{},left{},right{};
    std::array<policy::Target,4> targets{};
    std::array<int,4> actual{{-1,-1,-1,-1}};
    std::uint64_t sequence{},interval{},session_began{},sample_ms{};
    unsigned lease{},sample{},epoch{},tick_epoch{},mask{},operation_depth{},known{};
    unsigned index_known{},index_match{},matrix_known{},matrix_rigid{},failed_bone{4};
    DWORD thread{};int tick_before{},tick_after{};
    bool tick_before_known{},dual{},reentered{},closed{};
    Status status{Status::not_attempted};
};
struct State {
    std::array<Row,capacity> rows{};
    unsigned count{};
    bool busy{},full{},dumped{};
};
} // namespace reach_diagnostic
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
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
    gears_hand_ik_tail::Lease lease;
    unsigned tail_field_writes{},tail_writes_at_begin{},tail_revoked_mask{};
#else
    policy::Lease lease;
#endif
    gears_weapon::FreeHandControl consumer;
    std::array<gears_vr::Viewpoint,2> bind{};
    std::array<policy::Target,4> targets{};
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    reach_diagnostic::State reach;
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    gears_hand_session::Session session;
    bool record_lease{},terminal_restore_recorded{};
#endif
#endif
};
Trial trial{};
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
bool component_dirty();
bool component_faulted();
bool component_related(uptr);
bool component_restore();
bool component_owned(bool forward=false);
bool component_reset();
void component_reset_binding(unsigned);
void component_relinquish();
void component_pre_restore();
void component_log_written();
void component_dump_end();
bool component_apply(gears_hand_stream::Plan const&,gears_weapon::HandWorldFrame const&,Snapshot const&,unsigned,unsigned);
bool dual_draining{},hand_writes_relinquished{};
bool journals_dirty(){return trial.lease.dirty()||component_dirty();}
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
bool component_safe_to_unload();
void publish_journals(){safe_to_unload.store(component_safe_to_unload(),std::memory_order_release);}
#else
void publish_journals(){safe_to_unload.store(!journals_dirty(),std::memory_order_release);}
#endif
bool restore(const wchar_t*);
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
void component_cancel_forward();
bool component_has_interval();
bool component_forward_open();
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
bool component_resume_interval();
bool component_hand_only_ready();
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
bool component_pause_owner_healthy();
#endif
bool component_after_hand_only(Snapshot const&,Snapshot const&,unsigned,unsigned,unsigned,unsigned,unsigned,
                               std::array<policy::Target,4> const&);
#endif
bool component_after_hand(Snapshot const&,Snapshot const&,unsigned,unsigned,unsigned,unsigned,
                          std::array<policy::Target,4> const&);
void end(unsigned,const wchar_t*);
namespace component_continuous {
// Adapter lifecycle only. These flags never replace a Plan, raw read or Permit.
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
enum class Interval {waiting,owned,closing,suspended};
enum class Pause {none,age,boundary};
enum class Mode {joint,hand_only};
#endif
struct State {
    bool begun{},closed{},deferred{},finishing{},terminal{},finalized{},cleanup_failed{},failure_known{};
    bool drain_attempted{},last_drain_result{};
    unsigned operation_depth{},action_depth{},terminal_code{},first_failure{};
    std::uint64_t acquire_attempts{},update_attempts{},commits{};
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    Interval interval{Interval::waiting};Pause pause{Pause::none};
    bool resume_stable{},hand_only_journal{},hand_only_started{};
    std::uint64_t interval_id{},clean_closes{},last_clean_interval{},hand_only_commits{};
#endif
    std::array<wchar_t,96> reason{},failure_reason{};
};
State state;
void close(const wchar_t*,bool force=false);
bool finish();
void request_terminal(unsigned,const wchar_t*);
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
void suspend_interval(const wchar_t*,bool keep_geometry);
bool pause_pending();
#endif
void terminal(unsigned,const wchar_t*);
bool normal_tick();
struct Operation {
    Operation(){++state.operation_depth;safe_to_unload.store(false,std::memory_order_release);}
    ~Operation(){--state.operation_depth;if(!state.operation_depth){finish();publish_journals();}}
    Operation(Operation const&)=delete;Operation& operator=(Operation const&)=delete;
};
struct PostTick {
    bool committed{};
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    bool waiting{},suspending{}; // Set only at a classified no-write branch.
    Mode mode{Mode::joint};bool hand_committed{};
    bool forward_open()const {return mode==Mode::hand_only?component_hand_only_ready():component_forward_open();}
#endif
    PostTick(){++state.operation_depth;safe_to_unload.store(false,std::memory_order_release);}
    ~PostTick();
};
struct Action {
    Action(){++state.action_depth;safe_to_unload.store(false,std::memory_order_release);}
    ~Action(){--state.action_depth;if(!state.action_depth){finish();publish_journals();}}
    Action(Action const&)=delete;Action& operator=(Action const&)=delete;
};
} // namespace component_continuous
bool component_safe_to_unload(){
    return !journals_dirty()&&!component_continuous::state.operation_depth&&!component_continuous::state.action_depth&&
        !component_continuous::state.finishing&&(!component_continuous::state.terminal||component_continuous::state.finalized)&&
        !trial.releasing&&!dual_draining;
}
#endif
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
// The only two live reset sites first drain/reset the component owner. Never
// assign or destroy a pending noncopyable hand journal, even on a fresh Start.
bool clear_tail_trial(){
    auto const& state=component_continuous::state;
    if(trial.lease.dirty()||trial.lease.busy()||trial.lease.restoring()||trial.releasing||
       component_dirty()||state.operation_depth||state.action_depth||state.finishing||dual_draining)return false;
    trial.~Trial();::new(static_cast<void*>(&trial)) Trial{};return true;
}
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
// HOLD_STATE_BEGIN: private diagnostic, not an authority or a new lease.
namespace component_hold {
constexpr unsigned target_ticks=2;
struct Token;
void token_exit(Token*);
void dump();
thread_local Token* current{};
struct Token {
    Token* previous;uptr object,function;
    Token(uptr object_value,uptr function_value) noexcept:
        previous(current),object(object_value),function(function_value){current=this;}
    Token(Token const&)=delete;Token& operator=(Token const&)=delete;
    bool outer() const noexcept {return previous==nullptr;}
    ~Token(){
        const bool owning_thread=GetCurrentThreadId()==game_thread.load(std::memory_order_acquire);
        if(owning_thread)token_exit(this);current=previous;if(owning_thread)dump();
    }
};
struct State {
    bool closed{},arming{},armed{},observer_busy{},ending{},dumped{},complete{};
    bool written_ok{},hand_before{},hand_after{},hand_restore_observed{},drain_called{},hand_result{},component_result{},reason_truncated{};
    bool last_hand_result{},last_component_result{};
    unsigned completed{},epoch{},written_epoch{},written_lease{},mask{},hand_setters{},hand_writes{};
    std::uint64_t generation{},dispatch_serial{},written_sequence{},written_revision{},deadline{},began{},component_setters{};
    uptr pc{},function{};Token* dispatch{};
    std::array<wchar_t,96> reason{}; // Owned copy: watched-action labels can be stack locals.
};
State state;
// Only the observer is recursion-masked. before_action still precedes the
// event_hook recursion bypass, and the real original dispatch is unmasked.
struct ObserverScope {
    bool saved_inside,saved_busy;
    ObserverScope():saved_inside(inside),saved_busy(state.observer_busy){inside=true;state.observer_busy=true;}
    ~ObserverScope(){state.observer_busy=saved_busy;inside=saved_inside;dump();}
};
bool stream_tick_allowed(Token const& token){return !state.closed||token.outer();}
void cancel(const wchar_t*);
void before_end(const wchar_t*);
bool pre_tick();
bool post_tick();
void arm(gears_hand_stream::Plan const&,unsigned,Snapshot const&);
void before_original(Token&,bool);
void after_original(Token&,bool);
void reset();
} // namespace component_hold
// HOLD_STATE_END
#endif
bool drain_journals(const wchar_t* reason,bool hand_restore_names=false){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    component_names::Scope names_scope(false);
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
    component_hold::cancel(reason);
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    component_continuous::close(reason,true);
    // No recursive restore on a live hand/generic-owner stack. Retain journals.
    if(component_continuous::state.operation_depth||trial.releasing||dual_draining){
        component_continuous::state.deferred=true;return false;
    }
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    auto& lifecycle=component_continuous::state;
    if(lifecycle.drain_attempted){
        if(!lifecycle.last_drain_result||lifecycle.cleanup_failed)return false;
        if(!lifecycle.hand_only_journal){
            const bool still_clean=!journals_dirty()&&!trial.lease.faulted()&&!component_faulted();
            if(!still_clean){lifecycle.last_drain_result=false;lifecycle.cleanup_failed=true;}
            return still_clean;
        }
        // A successful older component close does not cover this new hand journal.
        if(component_dirty()||component_faulted()){
            lifecycle.last_drain_result=false;lifecycle.cleanup_failed=true;return false;
        }
    }
#else
    if(component_continuous::state.drain_attempted)return component_continuous::state.last_drain_result;
#endif
    component_continuous::state.drain_attempted=true;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    component_continuous::state.last_drain_result=false;
#endif
#endif
    // Terminal producer state is deliberately absent from rollback admission.
    if(dual_draining)return false;
    dual_draining=true;
    component_pre_restore();
    bool hand{};
    {
        component_names::Scope hand_names_scope(hand_restore_names);
        hand=restore(reason);
    }
    const bool component=component_restore(); // Always attempted, even after hand failure/end.
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
    if(component_hold::state.closed){
        if(!component_hold::state.drain_called){component_hold::state.hand_result=hand;component_hold::state.component_result=component;}
        component_hold::state.drain_called=true;component_hold::state.last_hand_result=hand;component_hold::state.last_component_result=component;
    }
#endif
    dual_draining=false;publish_journals();
    if(trial.ended)component_dump_end(); // Diagnostic retry after both journals have been attempted.
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    const bool verified=hand&&component&&!journals_dirty()&&!trial.lease.faulted()&&!component_faulted();
    component_continuous::state.last_drain_result=verified;
    if(!verified)component_continuous::state.cleanup_failed=true;
    return verified;
#else
    return hand&&component&&!journals_dirty();
#endif
}
#endif

bool diagnostic_event(){
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return false;
    return trial.session.diagnostic_event(GetTickCount64());
#else
    return true;
#endif
}

#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
// Bounded observation buffer only. It has its own receipt cursor and never
// supplies a Context, receipt, binding or plan to the writer. Flush at end.
struct EquipmentShadowRow {
    std::uint64_t early_ms{},late_ms{},epoch{},late_epoch{},old_serial{},old_sequence{};
    std::uint64_t serial{},sequence{},created_ms{},writer_before{},writer_after{},read_qpc{};
    unsigned phase{},late_phase{},bound{},checks{},tracked{},candidate{},expired{},other_reset{},useful{},reason_bits[2]{};
    bool fresh{},paired{},same_state{};
    gears_hand_stream::ObserveAge old[2]{},context{},source{},late_old[2]{},late_source{};
};
struct EquipmentShadowState {
    std::array<EquipmentShadowRow,1024> rows{};
    gears_hand_stream::ContinuitySnapshot pending_state{};
    std::uint64_t cursor{UINT64_MAX},dropped{},invalid_clock{},writer_changes{};
    unsigned count{},pending{UINT32_MAX};
};
EquipmentShadowState shadow;
#if defined(GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL)
struct EquipmentTransportRow {
    std::uint64_t now{},epoch{},serial{},sequence{},created_ms{},writer_before{},writer_after{};
    std::uint64_t policy_serial_before{},policy_serial_after{},policy_sequence_before{},policy_sequence_after{};
    std::uint64_t before_stamp[2]{},after_stamp[2]{},read_qpc{},refresh_qpc{};
    unsigned phase{},before_bound{},after_bound{},refreshed{},reset{},tracked{},reason{15};
    bool fresh{},called{},state_unchanged{},capacity_exhausted{};
    gears_hand_stream::ObserveAge before[2]{},after[2]{},source{};
};
struct EquipmentTransportState {
    std::array<EquipmentTransportRow,1024> rows{};
    std::uint64_t cursor{UINT64_MAX},dropped{},invalid_clock{};
    unsigned count{};
    bool reader_rejected{};
};
EquipmentTransportState transport_trial;
struct EquipmentEntryObservation {
    gears_hand_stream::ContinuitySnapshot state{};
    std::uint64_t now{},qpc{},epoch{};
    unsigned phase{};
    bool known{};
};
thread_local unsigned equipment_entry_depth{};
class EquipmentEntryScope {
    EquipmentEntryObservation observation_{};
public:
    EquipmentEntryScope(uptr object,uptr function) noexcept {
        const bool outer=equipment_entry_depth++==0;
        const auto thread=game_thread.load(std::memory_order_acquire);
        // Check the known game thread before touching ordinary trial state.
        if(!outer||inside||!thread||thread!=GetCurrentThreadId()||
           !enabled.load(std::memory_order_acquire)||!playground::automatic||
           !active_visible.load(std::memory_order_acquire))return;
        if(trial.ended||!trial.phase||stop_requested.load(std::memory_order_acquire)||
           shutdown_requested.load(std::memory_order_acquire)||
           object!=trial.xr_start.objects[gears_hand_xr_trial::controller].address||
           !playground::player_tick_index||get<Name>(function+0x48).index!=playground::player_tick_index)return;
        observation_.known=true;observation_.phase=trial.xr_equipment_phase;observation_.epoch=trial.epoch;
        observation_.now=GetTickCount64();
        LARGE_INTEGER stamp{};
        if(QueryPerformanceCounter(&stamp)&&stamp.QuadPart>0)observation_.qpc=std::uint64_t(stamp.QuadPart);
        observation_.state=trial.xr_policy.continuity_snapshot(observation_.now);
    }
    ~EquipmentEntryScope(){--equipment_entry_depth;}
    EquipmentEntryScope(EquipmentEntryScope const&)=delete;
    EquipmentEntryScope& operator=(EquipmentEntryScope const&)=delete;
    EquipmentEntryObservation const* observation() const noexcept {return observation_.known?&observation_:nullptr;}
};
struct EquipmentEntryRow {
    std::uint64_t epoch{},ms{},qpc{},transport_qpc{},prefix_qpc{},stamp[2]{};
    unsigned transport_row{},phase{},bound{};
    bool known{},same_state{};
};
struct EquipmentEntryState {
    std::array<EquipmentEntryRow,1024> rows{};
    std::uint64_t dropped{},invalid_clock{};
    unsigned count{};
};
EquipmentEntryState entry_trial;
static_assert(std::size(entry_trial.rows)==std::size(transport_trial.rows));
bool entry_same_state(EquipmentEntryObservation const& entry,gears_hand_stream::ContinuitySnapshot const& before,
    unsigned phase,std::uint64_t epoch) noexcept {
    const auto& state=entry.state;
    if(entry.phase!=phase||entry.epoch!=epoch||
       !gears_hand_stream::same(state.context.game,before.context.game)||
       state.context.expected_native_world_generation!=before.context.expected_native_world_generation||
       state.context.action_epoch!=before.context.action_epoch||state.context.observed_ms!=before.context.observed_ms||
       state.context.idle_ready!=before.context.idle_ready||state.context.clean!=before.context.clean||
       state.context_known!=before.context_known||state.stream_known!=before.stream_known||state.ready!=before.ready||
       !gears_hand_stream::same(state.stream,before.stream)||state.serial!=before.serial||state.sequence!=before.sequence||
       state.units_per_metre!=before.units_per_metre||state.bound_mask!=before.bound_mask)return false;
    for(unsigned hand=0;hand<2;++hand)
        if(state.source_stamp[hand]!=before.source_stamp[hand]||
           state.old_ticket_age[hand].known!=before.old_ticket_age[hand].known)return false;
    return true; // Ages at different times are deliberately not compared.
}
void entry_record(EquipmentEntryObservation const* entry,gears_hand_stream::ContinuitySnapshot const& before,
    unsigned phase,std::uint64_t epoch,std::uint64_t transport_qpc,unsigned transport_row){
    if(transport_row>=entry_trial.rows.size()){++entry_trial.dropped;return;}
    auto& row=entry_trial.rows[transport_row];row={};
    row.transport_row=transport_row;row.transport_qpc=transport_qpc;
    if(entry&&entry->known){
        row.known=true;row.phase=entry->phase;row.epoch=entry->epoch;row.ms=entry->now;row.qpc=entry->qpc;
        row.bound=entry->state.bound_mask;row.stamp[0]=entry->state.source_stamp[0];row.stamp[1]=entry->state.source_stamp[1];
        row.same_state=entry_same_state(*entry,before,phase,epoch);
        if(!entry->qpc||!transport_qpc||transport_qpc<entry->qpc)++entry_trial.invalid_clock;
        else row.prefix_qpc=transport_qpc-entry->qpc;
    }
    entry_trial.count=transport_row+1;
}
void entry_dump(){
    fwprintf(log_file,L"hand_ik_equipment_entry_begin schema=1 capacity=%zu count=%u dropped=%llu "
        L"invalid_clock=%llu frequency=%llu\n",entry_trial.rows.size(),entry_trial.count,
        entry_trial.dropped,entry_trial.invalid_clock,trial.timing.frequency);
    for(unsigned i=0;i<entry_trial.count;++i){const auto& row=entry_trial.rows[i];
        fwprintf(log_file,L"hand_ik_equipment_entry row=%u transport_row=%u known=%u same_state=%u "
            L"entry_phase=%u entry_epoch=%llu entry_ms=%llu entry_qpc=%llu transport_qpc=%llu prefix_qpc=%llu "
            L"entry_bound=%u entry_left_stamp=%llu entry_right_stamp=%llu\n",i,row.transport_row,
            unsigned(row.known),unsigned(row.same_state),row.phase,row.epoch,row.ms,row.qpc,row.transport_qpc,
            row.prefix_qpc,row.bound,row.stamp[0],row.stamp[1]);
    }
    fwprintf(log_file,L"hand_ik_equipment_entry_end schema=1\n");
}
void transport_reset(){
    transport_trial.cursor=UINT64_MAX;transport_trial.dropped=transport_trial.invalid_clock=0;
    transport_trial.count=0;transport_trial.reader_rejected=false;
    entry_trial.count=0;entry_trial.dropped=entry_trial.invalid_clock=0;
}
void transport_dump(){
    fwprintf(log_file,L"hand_ik_equipment_transport_begin schema=1 capacity=%zu count=%u dropped=%llu "
        L"invalid_clock=%llu frequency=%llu rejected_reads=%u new_plans=0 writer_cursor_updates=0\n",transport_trial.rows.size(),
        transport_trial.count,transport_trial.dropped,transport_trial.invalid_clock,trial.timing.frequency,
        unsigned(transport_trial.reader_rejected));
    for(unsigned i=0;i<transport_trial.count;++i){const auto& row=transport_trial.rows[i];
        fwprintf(log_file,L"hand_ik_equipment_transport row=%u phase=%u now_ms=%llu epoch=%llu "
            L"fresh=%u called=%u serial=%llu sequence=%llu created_ms=%llu "
            L"writer_before=%llu writer_after=%llu policy_serial_before=%llu policy_serial_after=%llu "
            L"policy_sequence_before=%llu policy_sequence_after=%llu "
            L"before_bound=%u after_bound=%u refreshed=%u reset=%u tracked=%u reason=%u state_unchanged=%u "
            L"capacity_exhausted=%u before_left_stamp=%llu before_right_stamp=%llu "
            L"after_left_stamp=%llu after_right_stamp=%llu read_qpc=%llu refresh_qpc=%llu",
            i,row.phase,row.now,row.epoch,unsigned(row.fresh),unsigned(row.called),row.serial,row.sequence,row.created_ms,
            row.writer_before,row.writer_after,row.policy_serial_before,row.policy_serial_after,
            row.policy_sequence_before,row.policy_sequence_after,row.before_bound,row.after_bound,row.refreshed,
            row.reset,row.tracked,row.reason,unsigned(row.state_unchanged),unsigned(row.capacity_exhausted),
            row.before_stamp[0],row.before_stamp[1],row.after_stamp[0],row.after_stamp[1],row.read_qpc,row.refresh_qpc);
        const auto print_age=[&](const wchar_t* label,gears_hand_stream::ObserveAge const& age){
            fwprintf(log_file,L" %ls_known=%u %ls_future=%u %ls_ms=%llu",label,unsigned(age.known),label,unsigned(age.future),label,age.ms);
        };
        print_age(L"before_left",row.before[0]);print_age(L"before_right",row.before[1]);
        print_age(L"after_left",row.after[0]);print_age(L"after_right",row.after[1]);print_age(L"source",row.source);
        fwprintf(log_file,L"\n");
    }
    fwprintf(log_file,L"hand_ik_equipment_transport_end schema=1\n");entry_dump();
}
#endif
void shadow_reset(){
    // Old buffer contents are never emitted: every row below count is replaced.
    shadow.cursor=UINT64_MAX;shadow.dropped=shadow.invalid_clock=shadow.writer_changes=0;
    shadow.count=0;shadow.pending=UINT32_MAX;shadow.pending_state={};
}
bool shadow_recent(gears_hand_stream::ObserveAge const& age){
    return age.known&&!age.future&&age.ms<=gears_hand_stream::maximum_age_ms;
}
void shadow_sample(gears_weapon::HandWorldFrame const* frame,std::uint64_t now,
    std::uint64_t writer_before,std::uint64_t begin,std::uint64_t finish){
    if(shadow.count==shadow.rows.size()){++shadow.dropped;shadow.pending=UINT32_MAX;return;}
    auto& row=shadow.rows[shadow.count];row={};row.early_ms=now;row.phase=trial.xr_equipment_phase;
    row.epoch=trial.epoch;row.writer_before=writer_before;row.writer_after=trial.xr_cursor;
    if(row.writer_before!=row.writer_after)++shadow.writer_changes;
    if(!begin||finish<begin)++shadow.invalid_clock;else row.read_qpc=finish-begin;
    const auto state=trial.xr_policy.continuity_snapshot(now);
    row.old_serial=state.serial;row.old_sequence=state.sequence;row.bound=state.bound_mask;
    row.context=state.context_age;for(unsigned hand=0;hand<2;++hand)row.old[hand]=state.old_ticket_age[hand];
    row.fresh=frame!=nullptr;
    if(frame){
        row.serial=frame->serial;row.sequence=frame->sequence;row.created_ms=frame->created_ms;
        row.tracked=frame->valid_mask;row.source=gears_hand_stream::observe_age(frame->created_ms,now);
        const auto& context=state.context;
        const gears_hand_stream::GameKey expected{
            trial.xr_start.objects[gears_hand_xr_trial::world_info].address,
            trial.xr_start.objects[gears_hand_xr_trial::controller].address,
            trial.xr_start.objects[gears_hand_xr_trial::pawn].address,
            trial.xr_start.objects[gears_hand_xr_trial::mesh].address,
            trial.xr_start.objects[gears_hand_xr_trial::weapon].address};
        const gears_hand_stream::StreamKey stream{frame->session,frame->space_epoch,
            frame->input_epoch,frame->world_generation};
        // These are shadow predicates, never write permissions. In particular
        // state.context and state.ready remain historical observations.
        if(state.context_known&&state.stream_known&&state.ready&&
           context.action_epoch==trial.epoch&&gears_hand_stream::same(context.game,expected)&&
           context.expected_native_world_generation==trial.xr_start.expected_native_world_generation)row.checks|=1;
        if(frame->controller==context.game.controller&&frame->pawn==context.game.pawn&&
           frame->weapon==context.game.weapon)row.checks|=2;
        if(gears_hand_stream::same(stream,state.stream))row.checks|=4;
        if(std::isfinite(frame->units_per_metre)&&frame->units_per_metre>0.&&
           frame->units_per_metre==state.units_per_metre)row.checks|=8;
        if(frame->serial>state.serial&&frame->sequence>state.sequence)row.checks|=16;
        if(shadow_recent(row.source))row.checks|=32;
        if(row.checks==63)for(unsigned hand=0;hand<2;++hand)
            if((row.bound&row.tracked&(1u<<hand))&&shadow_recent(row.old[hand]))row.candidate|=1u<<hand;
    }
    shadow.pending_state=state;shadow.pending=shadow.count++;
}
void shadow_pair(gears_hand_stream::ContinuitySnapshot const& before,
    gears_hand_stream::ObserveDiagnostic const& diagnostic){
    if(shadow.pending==UINT32_MAX)return;
    auto& row=shadow.rows[shadow.pending];shadow.pending=UINT32_MAX;
    const auto& early=shadow.pending_state;
    row.paired=true;row.late_ms=diagnostic.observed_now;row.late_phase=trial.xr_equipment_phase;row.late_epoch=trial.epoch;
    row.same_state=row.phase==row.late_phase&&row.epoch==row.late_epoch&&
        early.context_known==before.context_known&&early.stream_known==before.stream_known&&
        early.ready==before.ready&&gears_hand_stream::same(early.context.game,before.context.game)&&
        early.context.action_epoch==before.context.action_epoch&&
        early.context.expected_native_world_generation==before.context.expected_native_world_generation&&
        early.context.observed_ms==before.context.observed_ms&&
        early.context.idle_ready==before.context.idle_ready&&early.context.clean==before.context.clean&&
        gears_hand_stream::same(early.stream,before.stream)&&
        early.serial==before.serial&&early.sequence==before.sequence&&early.bound_mask==before.bound_mask&&
        early.units_per_metre==before.units_per_metre;
    for(unsigned hand=0;hand<2;++hand)
        row.same_state=row.same_state&&early.source_stamp[hand]==before.source_stamp[hand]&&
            early.old_ticket_age[hand].known==before.old_ticket_age[hand].known;
    row.expired=diagnostic.mask(gears_hand_stream::ObserveReason::binding_age);
    for(unsigned reason=0;reason<gears_hand_stream::observe_reason_count;++reason){
        const auto mask=diagnostic.reason_masks[reason];
        for(unsigned hand=0;hand<2;++hand)if(mask&(1u<<hand))row.reason_bits[hand]|=1u<<reason;
        const auto kind=gears_hand_stream::ObserveReason(reason);
        if(kind!=gears_hand_stream::ObserveReason::binding_age&&
           kind!=gears_hand_stream::ObserveReason::no_receipt&&kind!=gears_hand_stream::ObserveReason::not_newer)
            row.other_reset|=mask;
    }
    for(unsigned hand=0;hand<2;++hand)row.late_old[hand]=diagnostic.old_ticket_age[hand];
    row.late_source=gears_hand_stream::observe_age(row.created_ms,row.late_ms,row.fresh);
    if(row.same_state&&shadow_recent(row.late_source))row.useful=row.candidate&row.expired&~row.other_reset;
}
void shadow_print_age(const wchar_t* label,gears_hand_stream::ObserveAge const& age){
    fwprintf(log_file,L" %ls_known=%u %ls_future=%u %ls_ms=%llu",label,unsigned(age.known),label,unsigned(age.future),label,age.ms);
}
void shadow_dump(){
    fwprintf(log_file,L"hand_ik_equipment_shadow_begin schema=1 capacity=%zu count=%u dropped=%llu "
        L"invalid_clock=%llu writer_changes=%llu frequency=%llu policy_mutations=0 geometry_mutations=0\n",
        shadow.rows.size(),shadow.count,shadow.dropped,shadow.invalid_clock,shadow.writer_changes,trial.timing.frequency);
    for(unsigned i=0;i<shadow.count;++i){const auto& row=shadow.rows[i];
        fwprintf(log_file,L"hand_ik_equipment_shadow row=%u phase=%u early_ms=%llu late_ms=%llu "
            L"epoch=%llu late_epoch=%llu old_serial=%llu old_sequence=%llu serial=%llu sequence=%llu created_ms=%llu "
            L"writer_before=%llu writer_after=%llu read_qpc=%llu bound=%u checks=%u tracked=%u candidate=%u "
            L"fresh=%u paired=%u same_state=%u late_phase=%u expired=%u other_reset=%u useful=%u "
            L"reason_left=%u reason_right=%u",
            i,row.phase,row.early_ms,row.late_ms,row.epoch,row.late_epoch,row.old_serial,row.old_sequence,
            row.serial,row.sequence,row.created_ms,row.writer_before,row.writer_after,row.read_qpc,row.bound,
            row.checks,row.tracked,row.candidate,unsigned(row.fresh),unsigned(row.paired),unsigned(row.same_state),
            row.late_phase,row.expired,row.other_reset,row.useful,row.reason_bits[0],row.reason_bits[1]);
        shadow_print_age(L"old_left",row.old[0]);shadow_print_age(L"old_right",row.old[1]);
        shadow_print_age(L"context",row.context);shadow_print_age(L"source",row.source);
        shadow_print_age(L"late_old_left",row.late_old[0]);shadow_print_age(L"late_old_right",row.late_old[1]);
        shadow_print_age(L"late_source",row.late_source);fwprintf(log_file,L"\n");
    }
    fwprintf(log_file,L"hand_ik_equipment_shadow_end schema=1\n");
}
std::uint64_t timing_clock(){
    LARGE_INTEGER stamp{};
    return QueryPerformanceCounter(&stamp)&&stamp.QuadPart>0?std::uint64_t(stamp.QuadPart):0;
}
unsigned timing_phase(){return trial.xr_equipment_phase<=2?trial.xr_equipment_phase:0;}
bool timing_active(){return trial.phase&&!trial.ended&&trial.timing.frequency&&!trial.timing.dumped;}
void timing_begin(){
    shadow_reset();
#if defined(GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL)
    transport_reset();
#endif
    LARGE_INTEGER frequency{};
    if(QueryPerformanceFrequency(&frequency)&&frequency.QuadPart>0)
        trial.timing.frequency=std::uint64_t(frequency.QuadPart);
}
void timing_record(EquipmentTiming stage,unsigned phase,std::uint64_t begin,std::uint64_t finish){
    auto& timing=trial.timing;
    if(timing.dumped)return;
    if(!begin||finish<begin){count_add(timing.invalid_clock);return;}
    timing.spans[phase][unsigned(stage)].add(finish-begin);
}
struct EquipmentTimingScope {
    EquipmentTiming stage;
    unsigned phase{};
    std::uint64_t begin{};
    bool active{};
    explicit EquipmentTimingScope(EquipmentTiming value,bool measure=true):stage(value){
        active=measure&&timing_active();
        if(active){phase=timing_phase();begin=timing_clock();count_add(trial.timing.open_scopes);}
    }
    ~EquipmentTimingScope(){
        if(active){timing_record(stage,phase,begin,timing_clock());--trial.timing.open_scopes;}
    }
};
void timing_pre_restore(){
    if(!timing_active())return;
    auto& timing=trial.timing;
    // A missing next observation is left visibly incomplete, never fabricated.
    timing.pending_restore=timing_clock();timing.pending_phase=timing_phase();count_add(timing.interval_started);
}
void timing_observation(){
    auto& timing=trial.timing;
    if(!timing.pending_restore)return;
    timing_record(EquipmentTiming::pre_restore_to_observe,timing.pending_phase,
        timing.pending_restore,timing_clock());
    timing.pending_restore=0;count_add(timing.interval_completed);
}
void timing_policy(gears_hand_stream::ObserveDiagnostic const& diagnostic){
    if(!trial.timing.frequency||trial.timing.dumped)return;
    for(unsigned reason=0;reason<gears_hand_stream::observe_reason_count;++reason){
        const auto mask=diagnostic.reason_masks[reason];
        if(!mask||mask>3)continue;
        auto& value=trial.timing.reasons[timing_phase()][reason];count_add(value.masks[mask]);
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
    fwprintf(log_file,L"hand_ik_equipment_timing_end schema=1\n");
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    fwprintf(log_file,L"hand_ik_equipment_shadow_skipped schema=1 source=GHW1 reason=legacy_lane_not_paired\n");
#else
    shadow_dump();
#endif
#if defined(GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL)
    transport_dump();
#endif
    fflush(log_file);
}
// Nested actions may end the trial inside a measured span. Delay the one dump
// until the outer event returns and every live span has added its final sample.
struct EquipmentTimingHookScope {~EquipmentTimingHookScope(){timing_dump_if_ended();}};
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
gears_hand_stream::Plan observe_xr_policy(gears_hand_stream::Context const& context,
    gears_hand_stream::Ticket const* fresh,std::uint64_t now,bool regular=false){
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL) && !defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    const auto before=trial.xr_policy.continuity_snapshot(now);
#else
    (void)regular;
#endif
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    const auto plan=trial.xr_policy.observe_certified(context,fresh,now,gears_vr::monotonic_ns());
#else
    const auto plan=trial.xr_policy.observe(context,fresh,now);
#endif
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    timing_policy(trial.xr_policy.last_observe());
#if !defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    if(regular)shadow_pair(before,trial.xr_policy.last_observe());
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
// Ordinary snapshots can observe a rejection before Extra's own read path.
// Retain that observation for this exact pending hand; matching bytes later
// cannot regain permission. Only a full quiescent session reset clears it.
void tail_observation_failed(uptr pawn,uptr mesh,uptr tree,unsigned index){
    if(index!=3&&index!=7)return;
    const unsigned hand=index==3?0u:1u;
    if(pawn==trial.lease_anchor.pawn&&mesh==trial.lease_anchor.mesh&&tree==trial.lease_anchor.tree&&
       trial.lease.translation().pending_fields(hand))trial.tail_revoked_mask|=1u<<hand;
}
bool tail_layout_allowed(unsigned index,uptr object,std::vector<unsigned char> const& fixed){
    if(index!=3&&index!=7)return false;
    const unsigned hand=index==3?0u:1u;
    if(!trial.lease.translation().pending_fields(hand))return true;
    auto const& baseline_control=trial.lease_anchor.controls[index];
    return !(trial.tail_revoked_mask&(1u<<hand))&&object==baseline_control.object&&fixed==baseline_control.fixed;
}
bool tail_value_allowed(unsigned index,uptr object,handtail::Observed const& sighting){
    if(index!=3&&index!=7)return false;
    const unsigned hand=index==3?0u:1u;
    auto const& extra=trial.lease.translation();
    if(extra.pending_fields(hand)){
        if(trial.tail_revoked_mask&(1u<<hand))return false;
        auto const& baseline_control=trial.lease_anchor.controls[index];
        return object==baseline_control.object&&handtail::same_schema(sighting,baseline_control.translation)&&
            extra.accepts(hand,sighting.value);
    }
    Name space_bone{};std::memcpy(&space_bone,sighting.value.space_bone.data(),sizeof(space_bone));
    const std::array<unsigned char,12> zero{};
    return sighting.value.translation==zero&&sighting.value.space==0&&sighting.value.flags==0&&
        handik::name_is(space_bone,L"None");
}
unsigned authorize_current_xr_plan(gears_hand_stream::Plan const&,gears_hand_stream::Context const&,std::uint64_t);
bool tail_values_match(Snapshot const& before,Snapshot const& written,unsigned mask){
    const unsigned hands=((mask&1)?2u:0u)|((mask&2)?1u:0u);
    auto const& extra=trial.lease.translation();
    if(extra.selected_mask()!=hands||!extra.prepared()||extra.faulted()||trial.tail_revoked_mask)return false;
    unsigned writes{};
    for(unsigned i=0;i<8;++i){
        auto const& old=before.controls[i].translation;auto const& value=written.controls[i].translation;
        const unsigned hand=i==3?0u:i==7?1u:2u;
        if(hand<2&&(hands&(1u<<hand))){
            const auto saved_value=extra.original(hand),owned=extra.owned(hand);
            if(!saved_value||!owned||!handtail::same_schema(old,value)||
               !gears_hand_tail_translation::exact(old.value,*saved_value)||
               !gears_hand_tail_translation::exact(value.value,*owned))return false;
            for(unsigned fields=extra.selected_fields(hand);fields;fields>>=1)writes+=fields&1u;
        }else if(!gears_hand_tail_translation::exact(old.value,value.value))return false;
    }
    return trial.tail_writes_at_begin<=std::numeric_limits<unsigned>::max()-writes&&
        trial.tail_field_writes==trial.tail_writes_at_begin+writes;
}
#endif
// Only identity/layout and this lease's dynamic fields participate in rollback.
// New melee/action/weapon flags cannot prevent restoration of unchanged tails.
struct Backend {
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
    gears_hand_stream::Plan const* tail_plan{};
    gears_hand_stream::Context tail_context{};
    unsigned tail_mask{};
    bool tail_wanted(unsigned,std::array<policy::Target,4> const&,
                     std::array<gears_hand_tail_translation::Value,2>&);
    bool tail_forward_allowed()const;
    bool tail_read(unsigned,gears_hand_tail_translation::Value&,gears_hand_tail_translation::Strength&);
    bool tail_write_field(unsigned,gears_hand_tail_translation::Field,
                          gears_hand_tail_translation::FieldValue const&,gears_hand_tail_translation::FieldValue const&);
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    bool forward_write{};unsigned forward_epoch{};
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    component_continuous::Mode forward_mode{component_continuous::Mode::joint};
#endif
    bool forward_allowed()const {
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
        if(!forward_write||trial.releasing||trial.lease.restoring()||trial.lease.primary_faulted())return true;
        if(trial.lease.faulted()||trial.tail_revoked_mask)return false;
#else
        if(!forward_write||trial.lease.faulted()||trial.releasing)return true;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
        const bool writer_open=forward_mode==component_continuous::Mode::hand_only?
            component_continuous::state.hand_only_journal&&component_hand_only_ready():!component_continuous::state.closed;
        return writer_open&&!trial.ended&&trial.epoch==forward_epoch&&
#else
        return !component_continuous::state.closed&&!trial.ended&&trial.epoch==forward_epoch&&
#endif
            trial.tick_epoch==forward_epoch&&!stop_requested.load(std::memory_order_acquire)&&
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
            !shutdown_requested.load(std::memory_order_acquire)&&tail_plan&&tail_mask&&
            authorize_current_xr_plan(*tail_plan,tail_context,GetTickCount64())==tail_mask;
#else
            !shutdown_requested.load(std::memory_order_acquire);
#endif
    }
#endif
    bool control(unsigned channel,handik::Control& out){
        if(channel>=indices.size())return false;
        const auto& anchor=trial.lease_anchor;
        for(const auto object:{anchor.pawn,anchor.mesh,anchor.tree})
            if(!registered(object)||playground::deleted(object))return false;
        const auto index=indices[channel];
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
        const bool known=handik::read_control(anchor.pawn,anchor.mesh,anchor.tree,index,out)&&
            out.object==anchor.controls[index].object&&out.fixed==anchor.controls[index].fixed;
        if(!known)tail_observation_failed(anchor.pawn,anchor.mesh,anchor.tree,index);
        return known;
#else
        return handik::read_control(anchor.pawn,anchor.mesh,anchor.tree,index,out)&&
            out.object==anchor.controls[index].object&&out.fixed==anchor.controls[index].fixed;
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        if(hand_writes_relinquished)return false;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        if(!forward_allowed())return false;
#endif
        handik::Control value{};if(!control(channel,value))return false;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        if(!forward_allowed())return false;
#endif
        count_add(trial.setter_calls);return handik::set_strength(value.object,strength);
    }
};
bool write_bytes(uptr address,policy::Target const& wanted){
    __try{std::memcpy(reinterpret_cast<void*>(address),wanted.data(),wanted.size());return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool Backend::write_target(unsigned channel,policy::Target const& expected,policy::Target const& wanted){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(hand_writes_relinquished)return false;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    if(!forward_allowed())return false;
#endif
    policy::Value value{};if(!read(channel,value)||value.target!=expected)return false;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    if(!forward_allowed())return false;
#endif
    const auto object=trial.lease_anchor.controls[indices[channel]].object;
    const auto property=playground::property(object,(channel&1)?L"BoneRotation":L"EffectorLocation");
    if(!registered(property))return false;
    count_add(trial.target_writes);
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
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
#include "vr_hand_tail_backend.inl"
#endif
void watch(uptr object){
    if(!object)return;
    for(unsigned i=0;i<trial.action_object_count;++i)if(trial.action_objects[i]==object)return;
    if(trial.action_object_count<trial.action_objects.size())trial.action_objects[trial.action_object_count++]=object;
}
bool quiet_reject(int index,handanim::Slot const& slot,int child,const wchar_t* reason){
    const auto now=GetTickCount64();
    if(!log_file||now<trial.next_slot_log||!diagnostic_event())return false;
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
        count_add(trial.quiet_slots);
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
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    if(!trial.record_lease)return;
#endif
    const auto sample_number=trial.samples;count_add(trial.samples);
    LARGE_INTEGER stamp{};QueryPerformanceCounter(&stamp);
    fwprintf(log_file,L"hand_ik_stream_trial_sample phase=%ls lease=%u segment=%u sample=%u "
        L"sequence=%llu qpc=%lld ms=%llu world_time=%.9g last_render_time=%.9g "
        L"mesh_tick_known=%u mesh_tick=%d action_idle=%u action_epoch=%u "
        L"trial_apply_mask=%u ownership_proved=0",
        phase,lease,(lease-1)/8,sample_number,trial.sequence,stamp.QuadPart,value.ms,
        value.world_time,value.last_render_time,unsigned(value.mesh_tick_known),value.mesh_tick,
        unsigned(value.action_idle),trial.epoch,mask);
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    fwprintf(log_file,L" equipment_phase=%u",trial.xr_equipment_phase);
#endif
    freehands::print_matrix(L"mesh",value.mesh_world);freehands::print_matrix(L"left",value.left);
    freehands::print_matrix(L"right",value.right);
    for(unsigned i=0;i<4;++i){fwprintf(log_file,L" target%u=",i);
        for(const auto byte:trial.targets[i])fwprintf(log_file,L"%02x",unsigned(byte));}
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
    fwprintf(log_file,L"\n");
    for(const unsigned index:{3u,7u}){
        auto const& tail_fields=value.controls[index].translation;
        const unsigned hand=index==3?0u:1u;
        fwprintf(log_file,L"hand_tail_translation_sample schema=1 phase=%ls lease=%u control=%u "
            L"sample=%u epoch=%u selected=%u pending=%u raw_flags=%u apply_mask=%u add_mask=%u space=%u translation=",
            phase,lease,index,sample_number,trial.epoch,trial.lease.translation().selected_mask(),
            trial.lease.translation().pending_fields(hand),tail_fields.raw_flags,tail_fields.apply_mask,tail_fields.add_mask,unsigned(tail_fields.value.space));
        for(const auto byte:tail_fields.value.translation)fwprintf(log_file,L"%02x",unsigned(byte));
        fwprintf(log_file,L" space_bone=");
        for(const auto byte:tail_fields.value.space_bone)fwprintf(log_file,L"%02x",unsigned(byte));
        fwprintf(log_file,L" flags=%u field_writes=%u\n",tail_fields.value.flags,trial.tail_field_writes);
    }
#endif
    fwprintf(log_file,L"\n");fflush(log_file);
}
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
namespace reach_diagnostic {
constexpr const wchar_t* bone_names[]={L"b_MF_UpperArm_L",L"b_MF_Forearm_L",L"b_MF_UpperArm_R",L"b_MF_Forearm_R"};
constexpr int expected_indices[]={8,10,32,34};
bool snapshot_keys(Snapshot const& value,Row& row){
    // Exact leading handik::identity encoding, not its vector object or padding.
    constexpr std::size_t stride=sizeof(uptr)*3+sizeof(unsigned)+sizeof(Name);
    static_assert(sizeof(uptr)==8&&sizeof(Name)==8&&stride==36);
    if(value.identity.size()<stride*row.keys.size())return false;
    for(unsigned i=0;i<row.keys.size();++i){
        auto const* bytes=value.identity.data()+stride*i;auto& key=row.keys[i];Name label{};
        std::memcpy(&key.object,bytes,8);std::memcpy(&key.index,bytes+8,4);
        std::memcpy(&key.outer,bytes+12,8);std::memcpy(&label,bytes+20,8);
        std::memcpy(&key.klass,bytes+28,8);key.name_index=label.index;key.name_number=label.number;
        if(!key.object||!key.klass)return false;
    }
    return row.keys[1].object==value.pc&&row.keys[2].object==value.pawn&&
        row.keys[3].object==value.mesh&&row.keys[4].object==value.tree&&row.keys[5].object==value.weapon;
}
bool identity_matches(Row const& row){
    for(auto const& key:row.keys){
        unsigned index{};uptr outer{},klass{};Name label{};
        if(!registered(key.object)||!copy(&index,key.object+8,sizeof(index))||
           !copy(&outer,key.object+0x40,sizeof(outer))||!copy(&label,key.object+0x48,sizeof(label))||
           !copy(&klass,key.object+0x50,sizeof(klass))||index!=key.index||outer!=key.outer||
           klass!=key.klass||label.index!=key.name_index||label.number!=key.name_number)return false;
    }
    uptr asset{},tree{};
    return freehands::read_field(row.keys[3].object,L"SkeletalMesh",L"ObjectProperty",&asset,sizeof(asset))&&
        freehands::read_field(row.keys[3].object,L"Animations",L"ObjectProperty",&tree,sizeof(tree))&&
        asset==row.keys[6].object&&tree==row.keys[4].object&&
        playground::named(asset,L"COG_Marcus_Fenix_CamSkel");
}
bool context_matches(Row const& row){
    auto const& state=component_continuous::state;
    return GetCurrentThreadId()==row.thread&&game_thread.load(std::memory_order_acquire)==row.thread&&
        !row.reentered&&trial.releasing&&!trial.ended&&trial.record_lease&&trial.xr_equipment_phase==2&&
        trial.began==row.session_began&&leases.load()==row.lease&&trial.sequence==row.sequence&&
        trial.epoch==row.epoch&&trial.tick_epoch==row.tick_epoch&&trial.applied_mask==row.mask&&
        trial.lease_anchor.pc==row.keys[1].object&&trial.lease_anchor.pawn==row.keys[2].object&&
        trial.lease_anchor.mesh==row.keys[3].object&&trial.lease_anchor.tree==row.keys[4].object&&
        trial.lease_anchor.weapon==row.keys[5].object&&state.interval_id==row.interval&&
        state.operation_depth==row.operation_depth&&row.operation_depth==1&&!state.action_depth&&
        !state.finishing&&!state.terminal&&!state.deferred&&!state.failure_known&&!state.cleanup_failed&&
        dual_draining==row.dual&&!hand_writes_relinquished&&trial.lease.dirty()&&!trial.lease.faulted()&&
        !component_faulted()&&!stop_requested.load(std::memory_order_acquire)&&
        !shutdown_requested.load(std::memory_order_acquire)&&
        !std::memcmp(trial.targets.data(),row.targets.data(),sizeof(row.targets));
}
struct Busy {
    State& state;
    explicit Busy(State& value):state(value){state.busy=true;}
    ~Busy(){state.busy=false;}
    Busy(Busy const&)=delete;Busy& operator=(Busy const&)=delete;
};
void capture(Snapshot const& value,unsigned lease){
    // All existing action hooks still run. A nested action may revoke/terminate;
    // this diagnostic then abandons its observation, never the original restore.
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return;
    auto& state=trial.reach;
    if(state.busy){if(state.count)state.rows[state.count-1].reentered=true;return;}
    if(!trial.record_lease||trial.xr_equipment_phase!=2||state.dumped)return;
    if(state.count==capacity){state.full=true;return;}
    auto& row=state.rows[state.count++];Busy busy(state);
    component_names::Scope reach_names(true);
    component_cost::InvokePause cost_pause; // No new scan clocks/aggregate ownership.
    row.lease=lease;row.sample=trial.samples?trial.samples-1:0;row.sequence=trial.sequence;
    row.interval=component_continuous::state.interval_id;row.session_began=trial.began;row.sample_ms=value.ms;
    row.epoch=trial.epoch;row.tick_epoch=trial.tick_epoch;row.mask=trial.applied_mask;
    row.operation_depth=component_continuous::state.operation_depth;row.thread=GetCurrentThreadId();
    row.dual=dual_draining;row.tick_before=value.mesh_tick;row.tick_before_known=value.mesh_tick_known;
    row.mesh=value.mesh_world;row.left=value.left;row.right=value.right;row.targets=trial.targets;
    const auto collect=[&](){
        if(!snapshot_keys(value,row)||!row.tick_before_known)return Status::snapshot_unknown;
        row.known|=1;
        // Normal pre_tick restore has depth 1 and dual_draining=true. Neither is
        // changed here. Action/final cleanup paths stay inert/unknown.
        if(!context_matches(row))return Status::unsafe_context;
        row.known|=2;
        if(!identity_matches(row))return Status::identity_changed;
        for(unsigned i=0;i<4;++i){
            row.failed_bone=i;
            const bool index_ok=freehands::bone_index(row.keys[3].object,bone_names[i],row.actual[i]);
            if(index_ok)row.index_known|=1u<<i;
            if(!context_matches(row))return Status::context_changed;
            if(!index_ok)return Status::index_unknown;
            if(row.actual[i]!=expected_indices[i])return Status::index_mismatch;
            row.index_match|=1u<<i;
            if(!identity_matches(row))return Status::identity_changed;
            const bool matrix_ok=freehands::bone_matrix(row.keys[3].object,row.actual[i],row.bones[i]);
            if(matrix_ok)row.matrix_known|=1u<<i;
            if(!context_matches(row))return Status::context_changed;
            if(!matrix_ok)return Status::matrix_unknown;
            gears_vr::Viewpoint ignored{};
            if(!matrix_pose(row.bones[i],ignored))return Status::matrix_nonrigid;
            row.matrix_rigid|=1u<<i;
            if(!identity_matches(row))return Status::identity_changed;
        }
        row.failed_bone=4;return Status::complete;
    };
    row.status=collect();
    // Pure reads only after the final VM. TickTag is not a new clock sample.
    if(row.known&1){
        if(context_matches(row))row.known|=4;
        if(identity_matches(row))row.known|=8;
        if(freehands::read_field(row.keys[3].object,L"TickTag",L"IntProperty",&row.tick_after,sizeof(row.tick_after))){
            row.known|=16;if(row.tick_after==row.tick_before)row.known|=32;
        }
        if(row.status==Status::complete&&row.known!=63)
            row.status=(row.known&16)&&!(row.known&32)?Status::tick_changed:Status::end_unknown;
    }
    row.closed=true;
}
void dump(bool clean){
    auto& state=trial.reach;
    if(!clean||!log_file||state.busy||state.dumped||!trial.ended||trial.releasing||dual_draining||
       component_continuous::state.operation_depth||component_continuous::state.action_depth||
       component_continuous::state.finishing||!component_continuous::state.finalized)return;
    state.dumped=true;
    fwprintf(log_file,L"hand_reach_diagnostic_begin schema=1 capacity=%u count=%u full=%u "
        L"parent_edges_live_verified=0 static_chain_left=11,10,8,6 static_chain_right=35,34,32,30 "
        L"matrix_order=upper_left,fore_left,upper_right,fore_right atomic_snapshot=0 writes=0\n",
        capacity,state.count,unsigned(state.full));
    constexpr const wchar_t* key_names[]={L"world",L"pc",L"pawn",L"mesh",L"tree",L"weapon",L"asset"};
    constexpr const wchar_t* matrix_names[]={L"upper_left",L"fore_left",L"upper_right",L"fore_right"};
    for(unsigned n=0;n<state.count;++n){auto const& row=state.rows[n];
        fwprintf(log_file,L"hand_reach_diagnostic schema=1 row=%u lease=%u sample=%u sequence=%llu "
            L"component_interval=%llu sample_ms=%llu action_epoch=%u tick_epoch=%u apply_mask=%u "
            L"thread=%lu operation_depth=%u dual_draining=%u known=%u status=%u closed=%u reentered=%u "
            L"tick_before_known=%u tick_before=%d tick_after_known=%u tick_after=%d "
            L"index_known=%u index_match=%u matrix_known=%u matrix_rigid=%u failed_bone=%u "
            L"expected_indices=8,10,32,34 actual_indices=%d,%d,%d,%d",
            n,row.lease,row.sample,row.sequence,row.interval,row.sample_ms,row.epoch,row.tick_epoch,row.mask,
            row.thread,row.operation_depth,unsigned(row.dual),row.known,unsigned(row.status),unsigned(row.closed),
            unsigned(row.reentered),unsigned(row.tick_before_known),row.tick_before,unsigned((row.known&16)!=0),row.tick_after,
            row.index_known,row.index_match,row.matrix_known,row.matrix_rigid,row.failed_bone,
            row.actual[0],row.actual[1],row.actual[2],row.actual[3]);
        for(unsigned k=0;k<row.keys.size();++k){auto const& key=row.keys[k];
            fwprintf(log_file,L" %ls_key=%p,%u,%u,%u,%p,%p,%u",key_names[k],reinterpret_cast<void*>(key.object),
                key.index,key.name_index,key.name_number,reinterpret_cast<void*>(key.klass),
                reinterpret_cast<void*>(key.outer),key.reserved);
        }
        freehands::print_matrix(L"mesh",row.mesh);freehands::print_matrix(L"left",row.left);freehands::print_matrix(L"right",row.right);
        for(unsigned i=0;i<4;++i){
            if(row.matrix_known&(1u<<i))freehands::print_matrix(matrix_names[i],row.bones[i]);
            else fwprintf(log_file,L" %ls=unknown",matrix_names[i]);
            fwprintf(log_file,L" target%u=",i);for(const auto byte:row.targets[i])fwprintf(log_file,L"%02x",unsigned(byte));
        }
        fwprintf(log_file,L"\n");
    }
    fwprintf(log_file,L"hand_reach_diagnostic_end schema=1 count=%u writes=0\n",state.count);fflush(log_file);
}
} // namespace reach_diagnostic
#endif
void end(unsigned code,const wchar_t* reason){
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    component_continuous::terminal(code,reason);
#else
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
    component_hold::before_end(reason);
#endif
    if(trial.ended)return;
    trial.ended=true;trial.phase=0;requested=false;active_visible=false;
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    trial.session.end();
#endif
    // No terminal state authorizes another target. A partial acquisition must
    // never claim mask 3; dirty/faulted separately describe unresolved fields.
    const bool clean=!trial.lease.dirty()
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        &&!component_dirty()
#endif
        ;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
    // Two recorded pairs do not certify cleanup. Preserve all earlier failures,
    // including a false setter/restore that nevertheless returned exact originals.
    if(code==5&&component_hold::state.closed&&
       (!component_hold::state.drain_called||!component_hold::state.hand_result||!component_hold::state.component_result||
        !component_hold::state.last_hand_result||!component_hold::state.last_component_result||
        trial.lease.faulted()||component_faulted()))code=7;
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    if(log_file){fwprintf(log_file,L"hand_ik_continuous_session_end schema=1 leases=%u restored=%u "
        L"recorded_leases=%u diagnostic_events=%u event_limit=%u counters_saturated=%u "
        L"dirty=%u faulted=%u same_instance_only=1 ownership_proved=0\n",leases.load(),trial.restored,
        trial.session.recorded_leases(),trial.session.diagnostic_events(),gears_hand_session::event_limit,
        unsigned(counters_saturated),unsigned(!clean),unsigned(trial.lease.faulted()));fflush(log_file);}
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    component_dump_end();
#endif
#endif
}

#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
namespace component_continuous {
void remember(std::array<wchar_t,96>& out,const wchar_t* value){
    out={};if(!value)return;
    for(std::size_t i=0;i+1<out.size()&&value[i];++i)out[i]=value[i];
}
void close(const wchar_t* reason,bool force){
    if(!force&&!state.begun)return;
    if(!state.closed){state.closed=true;remember(state.reason,reason);}
    component_cancel_forward();
    if(journals_dirty())state.deferred=true;
}
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
void suspend_interval(const wchar_t* reason,bool keep_geometry){
    if(state.terminal||trial.ended)return;
    if(!keep_geometry)state.pause=Pause::boundary;
    else if(state.interval!=Interval::closing)state.pause=Pause::age;
    if(state.interval!=Interval::closing){
        state.interval=Interval::closing;state.resume_stable=false;trial.stability.reset();
    }
    close(reason,true);state.deferred=true;
}
bool pause_pending(){
    return state.interval==Interval::closing&&state.deferred&&!state.terminal&&
        !state.failure_known&&!state.cleanup_failed&&!trial.lease.faulted()&&!component_faulted()&&
        state.operation_depth&&!state.action_depth&&!trial.releasing&&!dual_draining;
}
#endif
void record_failure(unsigned code,const wchar_t* reason){
    if(code>=7&&!state.failure_known){
        state.failure_known=true;state.first_failure=code;remember(state.failure_reason,reason);
    }
}
void finalize(){
    if(state.finalized||!state.terminal||state.finishing||state.operation_depth||state.action_depth||trial.releasing||dual_draining)return;
    state.finalized=true;
    const bool clean=!journals_dirty();
    unsigned code=state.failure_known?state.first_failure:state.terminal_code;
    if((state.cleanup_failed||trial.lease.faulted()||component_faulted())&&code<7)code=7;
    if(!clean)code=8;
    const wchar_t* reason=state.failure_known?state.failure_reason.data():state.reason.data();
    safe_to_unload.store(clean,std::memory_order_release);status.store(code,std::memory_order_release);
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(log_file){fwprintf(log_file,L"hand_weapon_component_persistent_end schema=1 intervals=%llu clean_closes=%llu "
        L"acquire_attempts=%llu update_attempts=%llu commits=%llu cleanup_failed=%u first_failure_known=%u first_failure=%u "
        L"dirty=%u faulted=%u safe_to_unload=%u hand_only_commits=%llu consumer_proved=0\n",state.interval_id,state.clean_closes,
        state.acquire_attempts,state.update_attempts,state.commits,unsigned(state.cleanup_failed),unsigned(state.failure_known),
        state.first_failure,unsigned(!clean),unsigned(trial.lease.faulted()||component_faulted()),unsigned(clean),
        state.hand_only_commits);fflush(log_file);}
#else
    if(log_file){fwprintf(log_file,L"hand_weapon_component_continuous_end schema=1 acquire_attempts=%llu update_attempts=%llu "
        L"commits=%llu closed=%u drain_attempted=%u drain_result=%u cleanup_failed=%u first_failure_known=%u first_failure=%u "
        L"dirty=%u faulted=%u safe_to_unload=%u single_interval=1 hand_restore_each_tick=1 component_restore_at_close=1 "
        L"consumer_proved=0\n",state.acquire_attempts,state.update_attempts,state.commits,unsigned(state.closed),
        unsigned(state.drain_attempted),unsigned(state.last_drain_result),unsigned(state.cleanup_failed),unsigned(state.failure_known),
        state.first_failure,unsigned(!clean),unsigned(trial.lease.faulted()||component_faulted()),unsigned(clean));fflush(log_file);}
#endif
    if(log_file){fwprintf(log_file,L"hand_ik_stream_trial_end status=%u leases=%u restored_leases=%u "
        L"action_yields=%u immediate_action_test=%u original_melee_seen=%u original_slot_play_seen=%u "
        L"resumed=%u setter_calls=%u target_writes=%u "
        L"trial_apply_mask=%u dirty=%u faulted=%u safe_to_unload=%u ownership_proved=0 reason=%ls\n",
        code,leases.load(),trial.restored,trial.action_yields,unsigned(trial.immediate_test),
        unsigned(trial.melee_seen),unsigned(trial.slot_play_seen),unsigned(trial.resumed),
        trial.setter_calls,trial.target_writes,0u,unsigned(!clean),unsigned(trial.lease.faulted()),unsigned(clean),reason);
        fflush(log_file);}
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    if(log_file){fwprintf(log_file,L"hand_ik_continuous_session_end schema=1 leases=%u restored=%u "
        L"recorded_leases=%u diagnostic_events=%u event_limit=%u counters_saturated=%u "
        L"dirty=%u faulted=%u same_instance_only=1 ownership_proved=0\n",leases.load(),trial.restored,
        trial.session.recorded_leases(),trial.session.diagnostic_events(),gears_hand_session::event_limit,
        unsigned(counters_saturated),unsigned(!clean),unsigned(trial.lease.faulted()));fflush(log_file);}
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    component_dump_end();
#endif
}
bool finish(){
    if(state.finalized)return state.last_drain_result&&!journals_dirty()&&!state.cleanup_failed;
    if(!state.deferred&&!state.terminal)return !journals_dirty();
    if(state.finishing||state.operation_depth||state.action_depth||trial.releasing||dual_draining)return false;
    state.finishing=true;
    // Even a clean hand is not a reason to skip the retained component journal.
    bool restored{};
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(state.drain_attempted&&(!state.last_drain_result||state.cleanup_failed))restored=false;
    else if(state.hand_only_journal)restored=drain_journals(L"continuous_closed_hand_only");
    else if(state.drain_attempted)restored=state.last_drain_result&&!journals_dirty();
#else
    if(state.drain_attempted)restored=state.last_drain_result;
#endif
    else if(!journals_dirty()){
        state.drain_attempted=true;state.last_drain_result=true;restored=true;
    }else restored=drain_journals(L"continuous_closed_interval");
    state.deferred=false;
    if(!restored||trial.lease.faulted()||component_faulted()){
        state.cleanup_failed=true;record_failure(journals_dirty()?8u:7u,L"continuous_handback_failed");
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
        request_terminal(journals_dirty()?8u:7u,L"continuous_handback_failed");
#endif
    }
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(!state.terminal&&restored&&!journals_dirty()&&!state.cleanup_failed&&
       !trial.lease.faulted()&&!component_faulted()&&state.interval==Interval::closing){
        state.interval=Interval::suspended;
        if(component_has_interval()&&state.interval_id!=state.last_clean_interval){
            count_add(state.clean_closes);state.last_clean_interval=state.interval_id;
        }
    }
#endif
    state.finishing=false;
    if(state.terminal)finalize();
    return restored&&!journals_dirty()&&!state.cleanup_failed&&!trial.lease.faulted()&&!component_faulted();
}
void request_terminal(unsigned code,const wchar_t* reason){
    record_failure(code,reason);
    if(!state.terminal){
        state.terminal=true;state.terminal_code=code;remember(state.reason,reason);
        trial.ended=true;trial.phase=0;requested=false;active_visible=false;
        trial.session.end();
    }
    close(reason,true);state.deferred=true;
}
void terminal(unsigned code,const wchar_t* reason){
    request_terminal(code,reason);
    finish();
}
bool normal_tick(){
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(trial.ended||state.terminal||stop_requested.load(std::memory_order_acquire)||
       shutdown_requested.load(std::memory_order_acquire)||trial.lease.faulted()||component_faulted()){
#else
    if(state.closed||trial.ended||stop_requested.load(std::memory_order_acquire)||
       shutdown_requested.load(std::memory_order_acquire)||trial.lease.faulted()||component_faulted()){
#endif
        if(!state.terminal){
            const unsigned code=(stop_requested.load(std::memory_order_acquire)||
                shutdown_requested.load(std::memory_order_acquire))?6u:7u;
            end(code,L"continuous_pre_tick_closed");
        }else{state.deferred=true;finish();}
        return false;
    }
    if(state.operation_depth||trial.releasing||dual_draining){
        end(8,L"continuous_recursive_pre_tick");return false;
    }
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if((state.interval==Interval::closing||state.deferred)&&!finish())return false;
    if(trial.ended||state.terminal)return false;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    {
#endif
    Operation operation;
    bool hand=true;
    dual_draining=true;
    {
        component_names::Scope names_scope(true);
        hand=restore(L"continuous_pre_tick_hand_only");
    }
    dual_draining=false;
    if(!hand||trial.lease.dirty()||trial.lease.faulted())end(8,L"continuous_hand_restore_failed");
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    } // Hand restore and any deferred cleanup finish before the zero-depth reset.
    if(trial.ended||state.terminal)return false;
    if(state.interval==Interval::suspended&&!component_resume_interval()){
        end(journals_dirty()?8u:7u,L"persistent_interval_reset_rejected");return false;
    }
    Operation retained_operation;
#endif
    if(trial.ended||state.closed)return false;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(state.interval==Interval::owned&&(!component_has_interval()||!component_owned())){
#else
    if(component_has_interval()&&!component_owned()){
#endif
        end(journals_dirty()?8u:7u,L"continuous_retained_component_changed");return false;
    }
    if(trial.ended||state.closed||trial.lease.dirty()||trial.lease.faulted()||component_faulted())return false;
    publish_journals();return true;
}
PostTick::~PostTick(){
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if((state.begun||state.hand_only_started)&&!state.terminal&&!committed){
        const bool pause_deferred=suspending&&pause_pending();
        const bool clean_wait=waiting&&(state.interval==Interval::waiting||state.interval==Interval::suspended)&&!journals_dirty()&&
            !state.hand_only_journal&&!state.failure_known&&!state.cleanup_failed&&!trial.lease.faulted()&&!component_faulted();
        const bool verified_hand=hand_committed&&mode==Mode::hand_only&&state.hand_only_journal&&
            trial.lease.dirty()&&component_hand_only_ready();
        if(!pause_deferred&&!clean_wait&&!verified_hand)end(7,L"continuous_post_tick_without_fresh_joint_commit");
    }
#else
    if(state.begun&&!state.terminal&&(state.closed||!committed))end(7,L"continuous_post_tick_without_fresh_joint_commit");
#endif
    --state.operation_depth;
    if(!state.operation_depth){finish();publish_journals();}
}
} // namespace component_continuous
#endif

bool restore(const wchar_t* reason){
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(!trial.lease.dirty()&&!trial.lease.faulted()&&!hand_writes_relinquished)
        component_continuous::state.hand_only_journal=false;
#endif
    if(!trial.lease.dirty())return true;
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    EquipmentTimingScope timing(EquipmentTiming::restore);
#endif
    trial.releasing=true;
    const auto lease=leases.load();Snapshot current{};
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    const bool terminal=(stop_requested.load(std::memory_order_acquire)||
        shutdown_requested.load(std::memory_order_acquire))&&!trial.terminal_restore_recorded;
    const bool log_restore=trial.record_lease||terminal||diagnostic_event();
    if(terminal)trial.terminal_restore_recorded=true;
#else
    constexpr bool log_restore=true;
#endif
    const bool sampled=handik::snapshot(trial.lease_anchor.pc,current);
    if(sampled)sample(L"pre_restore",current,lease,trial.applied_mask);
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(sampled&&log_file&&!ferror(log_file))reach_diagnostic::capture(current,lease);
#endif
    Backend backend{};
    const bool clean=trial.lease.restore(backend);trial.releasing=false;
    if(clean)count_add(trial.restored);
    if(log_file&&log_restore){fwprintf(log_file,L"hand_ik_stream_trial_restore lease=%u dirty_before=1 dirty_after=%u "
        L"restored=%u reason=%ls\n",lease,unsigned(!clean),unsigned(clean),reason);fflush(log_file);}
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    trial.record_lease=false;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(component_continuous::state.hand_only_journal){
        if(clean&&!trial.lease.faulted()&&sampled)component_continuous::state.hand_only_journal=false;
        else{
            component_continuous::state.cleanup_failed=true;
            component_continuous::state.last_drain_result=false;
        }
    }
#endif
    if(!clean||trial.lease.faulted()){end(clean?7:8,L"lease_restore_unverified_or_faulted");return false;}
    if(!sampled){end(7,L"restored_but_pre_restore_sample_unknown");return false;}
    return true;
}
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
// Game-thread hand journal readiness only. Stop/ended status must not prevent
// the equipment owner from completing its original attachment restoration.
bool equipment_ready(){return !trial.lease.dirty()
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    &&!component_dirty()
#else
    &&!trial.lease.faulted()
#endif
    ;}
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
bool yield_for_equipment(const wchar_t* reason,bool initial_phase_hint){
#else
bool yield_for_equipment(const wchar_t* reason){
#endif
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return false;
    const bool dirty=trial.lease.dirty()
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        ||component_dirty()
#endif
        ,faulted=trial.lease.faulted()
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        ||component_faulted()
#endif
        ;
    if(!used.load(std::memory_order_acquire)&&!dirty&&!faulted)return true;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    component_continuous::suspend_interval(reason?reason:L"equipment_boundary",false);
#else
    if(component_continuous::state.begun)
        component_continuous::request_terminal(7,reason?reason:L"equipment_boundary");
#endif
#endif
    // Revoke geometry and all previously issued plans before rollback, even
    // when rollback fails. Never reopen a stopped trial or its write budget.
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    const bool epoch_available=trial.epoch!=std::numeric_limits<unsigned>::max();
    if(!epoch_available)stop_requested.store(true,std::memory_order_release);
    else ++trial.epoch;
#else
    ++trial.epoch;
#endif
    trial.stability.reset();trial.xr_ready=false;
    gears_hand_stream::Context blocked{};
    blocked.action_epoch=trial.epoch;blocked.observed_ms=GetTickCount64();
    blocked.clean=!dirty&&!faulted;
    observe_xr_policy(blocked,nullptr,blocked.observed_ms);
    for(auto& binding:trial.xr_binding){binding.reset();count_add(trial.xr_resets);}
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    component_reset_binding(3);
#endif
    if(!trial.ended&&!stop_requested.load(std::memory_order_acquire)&&
       active_visible.load(std::memory_order_acquire)&&(trial.phase==1||trial.phase==2)){
        trial.phase=1;status=1;
    }
    if(dirty)count_add(trial.action_yields);
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    const bool restored=!dirty||drain_journals(reason?reason:L"equipment_boundary");
#else
    const bool restored=!dirty||(!trial.releasing&&restore(reason?reason:L"equipment_boundary"));
#endif
    // A diagnostic sample failure can end the hand trial after a verified
    // clean rollback. It must not strand the equipment owner's attachment.
    const bool ready=equipment_ready();
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
    if(component_hold::state.closed&&!trial.ended)end(ready?7u:8u,L"hold_action_cancelled");
#endif
    if(!trial.lease.dirty())trial.applied_mask=0;
    if(log_file&&diagnostic_event()){fwprintf(log_file,L"hand_ik_equipment_yield reason=%ls equipment_phase=%u "
        L"epoch=%u dirty_before=%u faulted_before=%u dirty_after=%u faulted_after=%u "
        L"restored=%u helper_clean=%u bindings_cleared=3 leases=%u stopped=%u ended=%u\n",
        reason?reason:L"equipment_boundary",trial.xr_equipment_phase,trial.epoch,unsigned(dirty),unsigned(faulted),
        unsigned(trial.lease.dirty()),unsigned(trial.lease.faulted()),unsigned(restored),unsigned(ready),
        leases.load(),unsigned(stop_requested.load(std::memory_order_acquire)),unsigned(trial.ended));
        fflush(log_file);}
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    if(!epoch_available&&!trial.ended)end(ready?7u:8u,L"xr_action_epoch_exhausted");
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if((!restored&&!component_continuous::pause_pending())||trial.lease.faulted()||component_faulted()){
        component_continuous::state.cleanup_failed=true;
        component_continuous::request_terminal(ready?7u:8u,L"continuous_equipment_handback_failed");
    }
#else
    if(!restored||trial.lease.faulted()||component_faulted()){
        component_continuous::state.cleanup_failed=true;
        component_continuous::record_failure(ready?7u:8u,L"continuous_equipment_handback_failed");
    }
#endif
    // Clean attachment restoration remains admissible even after a sticky
    // experiment fault. A busy stack is never reported as synchronous handback.
    if(component_continuous::state.terminal)component_continuous::finish();
    // Keep all original quiescent synchronous handbacks, including callers
    // outside PostTick. The hint only admits the clean never-held depth1 case.
    if(ready&&!component_continuous::state.operation_depth&&!trial.releasing&&!dual_draining)return true;
    return initial_phase_hint&&ready&&component_continuous::state.operation_depth==1&&
        !component_continuous::state.action_depth&&!component_continuous::state.begun&&
        !component_continuous::state.closed&&!component_continuous::state.terminal&&
        !component_continuous::state.deferred&&!component_continuous::state.finishing&&
        !trial.ended&&active_visible.load(std::memory_order_acquire)&&trial.phase==1&&trial.xr_equipment_phase==0&&
        leases.load(std::memory_order_acquire)==0&&!component_has_interval()&&
        !dirty&&!faulted&&!journals_dirty()&&!trial.lease.faulted()&&!component_faulted()&&
        !trial.releasing&&!dual_draining&&!stop_requested.load(std::memory_order_acquire)&&
        !shutdown_requested.load(std::memory_order_acquire);
#else
    return ready;
#endif
}
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
// All old callers keep the original strict busy result. The initial hint is
// explicit only at the observed0-to1 branch; it grants no pose/write authority.
bool yield_for_equipment(const wchar_t* reason){return yield_for_equipment(reason,false);}
#endif
#endif
bool related(uptr object){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(component_related(object))return true;
#endif
    for(unsigned i=0;i<trial.action_object_count;++i)if(trial.action_objects[i]==object)return registered(object);
    if(!registered(object)||playground::deleted(object))return false;
    uptr pawn{};
    if(playground::instance(object,L"WarWeapon"))
        return handik::pointer(object,L"Instigator",pawn)&&pawn==trial.anchor.pawn;
    if(playground::instance(object,L"WarSpecialMove"))
        return handik::pointer(object,L"PawnOwner",pawn)&&pawn==trial.anchor.pawn;
    return false;
}
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
// Pure name classification only. It grants no object association or mutation.
bool component_action_name(const wchar_t* label) noexcept {
    constexpr const wchar_t* actions[]={
        L"ServerUse",L"GrabPickup",L"ServerDictateSpecialMove",L"DoSpecialMove",
        L"EndSpecialMove",L"StartSpecialMove",L"StopSpecialMove",L"BodyStanceAnimEndNotification",
        L"OnAnimEnd",L"PlayBodyStance",L"PlayBodyStanceByDuration",L"SetBodyStance",
        L"StopBodyStance",L"StopAllBodyStances",L"StartMeleeAttack",L"StopMeleeAttack",
        L"ForceStopMeleeAttack",L"ForciblyStopMeleeAttack",L"ClientStopMeleeAttack",L"MeleeAttackStarted",
        L"MeleeAttackEnded",L"BeginState",L"EndState",L"SetWeaponTemporarilyHolstered",
        L"HolsterWeaponTemporarily",L"SetWeaponFromSlot",L"SetCurrentWeapon",L"ChangedWeapon",
        L"ClientWeaponSet",L"AttachWeaponToHand",L"DetachWeapon",L"AttachWeaponToSlot",
        L"DetachWeaponFromSlot",L"AttachComponent",L"AttachComponentToSocket",L"DetachComponent",
        L"DetachComponentFromSocket",L"ForceReload",L"Died",L"Destroyed",
        L"SetAbsolute",L"SetTranslation",L"SetRotation",L"PlayCustomAnim",
        L"PlayCustomAnimByDuration",L"StopCustomAnim"
    };
    for(const auto item:actions)if(!wcscmp(item,label))return true;
    return false;
}
#endif
void before_action(uptr object,uptr function
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    ,bool owned_invoke
#endif
){
    // Reject other threads before touching any non-atomic game-thread state.
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return;
    if((!active_visible.load(std::memory_order_acquire)&&!journals_dirty())||
       !trial.anchored||!registered(function))return;
#else
    if(!active_visible.load(std::memory_order_acquire)||
       GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return;
    if(trial.releasing||!trial.anchored||!registered(function)||!related(object))return;
#endif
    wchar_t label[96]{};
    if(!name(function,label,96)){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        // Unknown names retain the original related-object fallback handling.
        (void)related(object);
#endif
        return;
    }
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(!component_action_name(label)||!related(object))return;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    component_continuous::Action continuous_action;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    component_continuous::suspend_interval(label,false);
#else
    component_continuous::request_terminal(7,label);
#endif
#endif
#else
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        L"SetAbsolute",L"SetTranslation",L"SetRotation",
#endif
#endif
        L"PlayCustomAnim",L"PlayCustomAnimByDuration",L"StopCustomAnim"};
    bool watched=false;for(const auto item:actions)if(!wcscmp(item,label)){watched=true;break;}
    if(!watched)return;
#endif
    if(trial.phase==3&&(!wcscmp(label,L"PlayCustomAnim")||!wcscmp(label,L"PlayCustomAnimByDuration")))
        trial.slot_play_seen=true;
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
    if(!wcscmp(label,L"StartMeleeAttack"))trial.melee_seen=true;
    if(!wcscmp(label,L"PlayCustomAnim")||!wcscmp(label,L"PlayCustomAnimByDuration"))trial.slot_play_seen=true;
#endif
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    const bool dirty=trial.lease.dirty();
    const bool restored=yield_for_equipment(label);
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(!restored&&!owned_invoke){
        // Original PE/CallFunction must keep consuming/forwarding its ABI.
        // Relinquish before it runs; late ABA cannot resurrect rollback rights.
        hand_writes_relinquished=true;component_relinquish();
        if(!trial.ended)end(8,L"external_action_with_unclean_journals");
        publish_journals();
    }
#endif
    if(log_file&&diagnostic_event()){fwprintf(log_file,L"hand_ik_stream_trial_action function=%ls epoch=%u "
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(journals_dirty()||!component_reset())return;
    dual_draining=hand_writes_relinquished=false;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    component_continuous::state={};
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
    component_hold::reset();
#endif
#endif
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
    if(!clear_tail_trial())return;
    requested=false;stop_requested=false;used=false;shutdown_requested=false;
#else
    trial={};requested=false;stop_requested=false;used=false;shutdown_requested=false;
#endif
    active_visible=false;safe_to_unload=true;status=0;leases=0;handik::reset();
}
DWORD stop_and_wait(){
    stop_requested.store(true,std::memory_order_release);
    if(game_thread.load(std::memory_order_acquire)==GetCurrentThreadId())return 41;
    const auto ready=[](){return safe_to_unload.load(std::memory_order_acquire)&&
        !requested.load(std::memory_order_acquire)&&!active_visible.load(std::memory_order_acquire);};
    const auto until=GetTickCount64()+2000;
    while(!ready()
#if !defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
          &&status.load(std::memory_order_acquire)!=8
#endif
          &&GetTickCount64()<until)Sleep(1);

    return ready()?0:40;
}
} // namespace handstream
