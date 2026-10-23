// Included inside handstream. This private alternative reads fresh world-hand
// receipts; the synthetic trajectory and automatic melee dispatch are absent.
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
gears_hand_continuity::WorldBridge xr_bridge;
#else
gears_weapon::EquipmentIntentBridge xr_bridge;
#endif
gears_hand_xr_trial::Start xr_pending{};
constexpr unsigned xr_max_leases=256;

#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
// Read-only diagnostics. No getter, receipt, clock, atomic, or authorization is
// sampled here. One pinned first rejection and 63 tail rows share 64 slots.
namespace component_trace {
using Value=gears_weapon::WeaponMeshComponentValue;
enum class Stage:unsigned {
    acquire_enter,capture,key_match,world,mount_bind,mount_solve,initial_read,absolute_storage,acquire_result,
    backend_read,set_key,schema_valid,schema_registered,schema_size,schema_class,schema_return,
    schema_native,schema_count,schema_bool_argument,schema_bool_mask,schema_struct_argument,schema_triple,
    set_bytes,set_read_before,set_expected_before,prepare_ended,prepare_phase,prepare_epoch,prepare_tick,
    prepare_snapshot,prepare_identity,prepare_idle,prepare_baseline,prepare_quiet,prepare_equipment,prepare_context,
    set_read_after,set_expected_after,authorize_ended,authorize_phase,authorize_tick,authorize_epoch,authorize_hand,
    authorize,set_owned,call_begin,call_end,restore_enter,restore_result,write_until,component_clean,component_not_faulted,pure_acquire_value,count
};
constexpr const wchar_t* names[]={
    L"acquire_enter",L"capture",L"key_match",L"world",L"mount_bind",L"mount_solve",L"initial_read",L"absolute_storage",L"acquire_result",
    L"backend_read",L"set_key",L"schema_valid",L"schema_registered",L"schema_size",L"schema_class",L"schema_return",
    L"schema_native",L"schema_count",L"schema_bool_argument",L"schema_bool_mask",L"schema_struct_argument",L"schema_triple",
    L"set_bytes",L"set_read_before",L"set_expected_before",L"prepare_ended",L"prepare_phase",L"prepare_epoch",L"prepare_tick",
    L"prepare_snapshot",L"prepare_identity",L"prepare_idle",L"prepare_baseline",L"prepare_quiet",L"prepare_equipment",L"prepare_context",
    L"set_read_after",L"set_expected_after",L"authorize_ended",L"authorize_phase",L"authorize_tick",L"authorize_epoch",L"authorize_hand",
    L"authorize",L"set_owned",L"call_begin",L"call_end",L"restore_enter",L"restore_result",L"write_until",L"component_clean",L"component_not_faulted",L"pure_acquire_value"
};
static_assert(sizeof(names)/sizeof(*names)==unsigned(Stage::count));
struct Auth {
    // known: args1/live2/authorize_ns4/policy_mask8/commit_ms16/commit_ns32/cutoff64.
    unsigned known{},checked{},rejected{},mask{},result{};
    std::uint64_t now{},live_now{},commit_now{},write_until{},ticket_ms{},plan_ms{},context_ms{};
    std::int64_t authorize_ns{},commit_ns{},stereo_ns{},input_ns{};
};
struct Meta {unsigned group{};bool forward{},wanted_known{},wanted_original{};Value expected{},wanted{};};
struct Row {
    std::uint64_t ordinal{};
    Stage stage{};unsigned detail{},bits{},pending{},owned{},revoked{},mismatch{};
    bool passed{},snapshot{},faulted{},read_executed{},read_known{},key_equal{},compare_known{};
    bool call_attempted{},call_return_known{},call_returned{},auth_known{};
    Meta meta{};Value read_value{};Auth auth{};
};
void masks(Row&) noexcept; // Defined beside the private component journal.
struct State {
    std::array<Row,64> rows{};
    Meta meta{};Auth last_auth{};
    std::uint64_t seen{},overwritten{};
    unsigned count{},next{},depth{};
    bool enabled{},first_known{},last_auth_known{},dumped{},counter_saturated{};
};
State state;
unsigned mismatch(Value const& a,Value const& b) noexcept {
    return (a.absolute==b.absolute?0u:1u)|(a.translation==b.translation?0u:2u)|(a.rotation==b.rotation?0u:4u);
}
void add(Row row){
    if(!state.enabled||state.dumped)return;
    if(state.seen!=UINT64_MAX)++state.seen;else state.counter_saturated=true;
    row.ordinal=state.seen;row.meta=state.meta;masks(row);
    if(!row.passed&&!state.first_known){state.rows[0]=row;state.first_known=true;}
    state.rows[1+state.next]=row;state.next=(state.next+1)%63;
    if(state.count<63)++state.count;
    else if(state.overwritten!=UINT64_MAX)++state.overwritten;else state.counter_saturated=true;
}
bool gate(Stage stage,bool passed,unsigned detail=0,unsigned bits=0){
    Row row{};row.stage=stage;row.passed=passed;row.detail=detail;row.bits=bits;add(row);return passed;
}
void authorization(Auth const& value,Stage stage=Stage::authorize){
    state.last_auth=value;state.last_auth_known=true;
    Row row{};row.stage=stage;row.passed=value.result!=0;row.auth_known=true;row.auth=value;add(row);
}
bool rejected(Auth& auth,unsigned check,bool value){
    auth.checked|=1u<<check;if(value)auth.rejected|=1u<<check;return value;
}
struct AuthEnd {Auth& row;~AuthEnd(){authorization(row);}};
struct Scope {
    Scope(){++state.depth;}
    ~Scope(){--state.depth;}
};
struct SetScope {
    Meta saved;
    SetScope(unsigned group,bool forward,Value const& expected,Value const& wanted,bool wanted_original):saved(state.meta){
        ++state.depth;state.meta={group,forward,true,wanted_original,expected,wanted};
    }
    ~SetScope(){state.meta=saved;--state.depth;}
};
void begin(){
    if(state.enabled)return;
    state.enabled=true;
    if(state.last_auth_known)authorization(state.last_auth);
}
void reset(){state={};}
void print_value(const wchar_t* label,Value const& value){
    fwprintf(log_file,L" %ls_absolute=%u%u%u %ls_translation=",label,unsigned(value.absolute[0]),unsigned(value.absolute[1]),unsigned(value.absolute[2]),label);
    for(auto byte:value.translation)fwprintf(log_file,L"%02x",unsigned(byte));
    fwprintf(log_file,L" %ls_rotation=",label);for(auto byte:value.rotation)fwprintf(log_file,L"%02x",unsigned(byte));
}
void print(Row const& row,const wchar_t* kind){
    fwprintf(log_file,L"hand_weapon_component_gate schema=1 kind=%ls ordinal=%llu stage=%ls group=%u detail=%u bits=%u passed=%u "
        L"forward_present=%u wanted_is_original_known=%u wanted_is_original=%u read_executed=%u read_known=%u key_equal=%u "
        L"compare_known=%u mismatch_mask=%u snapshot=%u pending_mask=%u owned_mask=%u revoked_mask=%u faulted=%u "
        L"call_attempted=%u call_return_known=%u call_returned=%u auth_known=%u",
        kind,row.ordinal,names[unsigned(row.stage)],row.meta.group,row.detail,row.bits,unsigned(row.passed),unsigned(row.meta.forward),
        unsigned(row.meta.wanted_known),unsigned(row.meta.wanted_original),unsigned(row.read_executed),unsigned(row.read_known),unsigned(row.key_equal),
        unsigned(row.compare_known),row.mismatch,unsigned(row.snapshot),row.pending,row.owned,row.revoked,unsigned(row.faulted),
        unsigned(row.call_attempted),unsigned(row.call_return_known),unsigned(row.call_returned),unsigned(row.auth_known));
    print_value(L"expected",row.meta.expected);print_value(L"wanted",row.meta.wanted);print_value(L"read",row.read_value);
    auto const& auth=row.auth;
    fwprintf(log_file,L" auth_fields_known=%u auth_checks_known=%u auth_rejected=%u now=%llu live_now=%llu commit_now=%llu "
        L"write_until=%llu authorize_ns=%lld commit_ns=%lld ticket_ms=%llu plan_ms=%llu context_ms=%llu stereo_ns=%lld input_ns=%lld "
        L"policy_mask=%u returned_mask=%u\n",auth.known,auth.checked,auth.rejected,auth.now,auth.live_now,auth.commit_now,auth.write_until,
        auth.authorize_ns,auth.commit_ns,auth.ticket_ms,auth.plan_ms,auth.context_ms,auth.stereo_ns,auth.input_ns,auth.mask,auth.result);
}
void dump(){
    if(!state.enabled||state.dumped||state.depth||dual_draining||!log_file)return;
    state.dumped=true;
    fwprintf(log_file,L"hand_weapon_component_gate_begin schema=1 capacity=64 pinned_first=1 count=%u seen=%llu overwritten=%llu "
        L"first_failure_known=%u counter_saturated=%u clocks_resampled=0 extra_authorizations=0\n",
        state.count,state.seen,state.overwritten,unsigned(state.first_known),unsigned(state.counter_saturated));
    if(state.first_known)print(state.rows[0],L"first_failure");
    const unsigned start=state.count==63?state.next:0;
    for(unsigned i=0;i<state.count;++i)print(state.rows[1+(start+i)%63],L"event");
    fwprintf(log_file,L"hand_weapon_component_gate_end schema=1\n");fflush(log_file);
}
struct DumpAtExit {~DumpAtExit(){dump();}};
} // namespace component_trace
#endif

unsigned authorize_xr_plan(gears_hand_stream::Plan const& plan,
    gears_hand_stream::Context const& context,std::uint64_t now,std::uint64_t write_until){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    component_trace::Auth trace{};component_trace::AuthEnd trace_end{trace};
    trace.known=1|64;trace.now=now;trace.write_until=write_until;
    trace.ticket_ms=plan.ticket.created_ms;trace.plan_ms=plan.context.observed_ms;trace.context_ms=context.observed_ms;
    trace.stereo_ns=plan.ticket.certificate.stereo_received_ns;trace.input_ns=plan.ticket.certificate.input_received_ns;
    const auto live_now=GetTickCount64();trace.live_now=live_now;trace.known|=2;
    if(component_trace::rejected(trace,0,live_now<now)||component_trace::rejected(trace,1,live_now>write_until)||
       component_trace::rejected(trace,2,stop_requested.load(std::memory_order_acquire))||
       component_trace::rejected(trace,3,shutdown_requested.load(std::memory_order_acquire)))return 0;
    const auto authorize_ns=gears_vr::monotonic_ns();trace.authorize_ns=authorize_ns;trace.known|=4;
    const auto mask=trial.xr_policy.authorize_certified(plan,context,live_now,authorize_ns);trace.mask=mask;trace.known|=8;
    const auto commit_now=GetTickCount64();trace.commit_now=commit_now;trace.known|=16;
    const auto commit_ns=gears_vr::monotonic_ns();trace.commit_ns=commit_ns;trace.known|=32;
    if(component_trace::rejected(trace,4,commit_now<live_now)||component_trace::rejected(trace,5,commit_ns<authorize_ns)||
       component_trace::rejected(trace,6,commit_now>write_until)||
       component_trace::rejected(trace,7,!gears_hand_stream::recent(plan.ticket.created_ms,commit_now))||
       component_trace::rejected(trace,8,!gears_hand_stream::recent(plan.context.observed_ms,commit_now))||
       component_trace::rejected(trace,9,!gears_hand_stream::recent(context.observed_ms,commit_now))||
       component_trace::rejected(trace,10,!gears_hand_continuity::certificate_recent(plan.ticket.certificate,commit_ns))||
       component_trace::rejected(trace,11,stop_requested.load(std::memory_order_acquire))||
       component_trace::rejected(trace,12,shutdown_requested.load(std::memory_order_acquire)))return 0;
    trace.result=mask;return mask;
#else
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    const auto live_now=GetTickCount64();
    if(live_now<now||live_now>write_until||stop_requested.load(std::memory_order_acquire)||
       shutdown_requested.load(std::memory_order_acquire))return 0;
    const auto authorize_ns=gears_vr::monotonic_ns();
    const auto mask=trial.xr_policy.authorize_certified(plan,context,live_now,authorize_ns);
    const auto commit_now=GetTickCount64();
    const auto commit_ns=gears_vr::monotonic_ns();
    // Authorization can itself be descheduled. The last explicit commit
    // decision observes the same finite cutoff, Stop and clock boundaries.
    if(commit_now<live_now||commit_ns<authorize_ns||commit_now>write_until||
       !gears_hand_stream::recent(plan.ticket.created_ms,commit_now)||
       !gears_hand_stream::recent(plan.context.observed_ms,commit_now)||
       !gears_hand_stream::recent(context.observed_ms,commit_now)||
       !gears_hand_continuity::certificate_recent(plan.ticket.certificate,commit_ns)||
       stop_requested.load(std::memory_order_acquire)||shutdown_requested.load(std::memory_order_acquire))return 0;
    return mask;
#else
    (void)write_until;
    return trial.xr_policy.authorize(plan,context,now);
#endif
#endif
}
unsigned authorize_current_xr_plan(gears_hand_stream::Plan const& plan,
    gears_hand_stream::Context const& context,std::uint64_t now){
    auto write_until=trial.deadline;
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    if(!gears_hand_session::write_until(plan.ticket.created_ms,plan.context.observed_ms,
        context.observed_ms,write_until)){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        component_trace::Auth trace{};trace.known=1;trace.now=now;
        trace.ticket_ms=plan.ticket.created_ms;trace.plan_ms=plan.context.observed_ms;trace.context_ms=context.observed_ms;
        trace.stereo_ns=plan.ticket.certificate.stereo_received_ns;trace.input_ns=plan.ticket.certificate.input_received_ns;
        component_trace::authorization(trace,component_trace::Stage::write_until);
#endif
        return 0;
    }
#endif
    return authorize_xr_plan(plan,context,now,write_until);
}

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
    if(log_file&&diagnostic_event()){fwprintf(log_file,L"hand_ik_equipment_gate stage=%ls expected_phase=%u current_phase=%u "
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    component_reset_binding(mask);
#endif
    for(unsigned hand=0;hand<2;++hand)if(mask&(1u<<hand)){
        trial.xr_binding[hand].reset();count_add(trial.xr_resets);
    }
}
#if defined(GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL)
constexpr bool transport_consumed_rejection(bool fresh,std::uint64_t before,std::uint64_t after) noexcept {
    return !fresh&&before!=UINT64_MAX&&after>before;
}
static_assert(transport_consumed_rejection(false,7,8)); // Rejected newer receipt, including a future stamp.
static_assert(!transport_consumed_rejection(false,UINT64_MAX,8)); // Initial attachment discards old receipts.
static_assert(!transport_consumed_rejection(false,7,7)); // No new frame or mutex contention.
static_assert(!transport_consumed_rejection(true,7,8)); // Accepted fresh receipt follows the Policy path.
static_assert(!transport_consumed_rejection(false,8,7)); // Not a cursor advance.
void refresh_transport_before_restore(EquipmentEntryObservation const* entry){
    EquipmentTransportRow row{};row.phase=trial.xr_equipment_phase;row.epoch=trial.epoch;
    row.writer_before=trial.xr_cursor;
    const auto reader_before=transport_trial.cursor;
    const auto read_begin=timing_clock();
    gears_weapon::HandWorldFrame frame{};
    row.fresh=trial.xr_open&&xr_bridge.read_pose(transport_trial.cursor,frame);
    const auto read_end=timing_clock();row.now=GetTickCount64();
    if(!read_begin||read_end<read_begin)++transport_trial.invalid_clock;else row.read_qpc=read_end-read_begin;
    if(transport_consumed_rejection(row.fresh,reader_before,transport_trial.cursor)){
        // The bridge consumed but filtered this receipt without returning its
        // identity. Do not let the independent late reader retry it after a
        // future stamp catches up. Seal writes now; restore before terminating.
        transport_trial.reader_rejected=true;
        stop_requested.store(true,std::memory_order_release);
    }
    const auto before=trial.xr_policy.continuity_snapshot(row.now);row.before_bound=before.bound_mask;
    row.policy_serial_before=before.serial;row.policy_sequence_before=before.sequence;
    if(row.fresh){
        row.serial=frame.serial;row.sequence=frame.sequence;row.created_ms=frame.created_ms;row.tracked=frame.valid_mask;
        row.source=gears_hand_stream::observe_age(frame.created_ms,row.now);
    }
    // This reader is independent of both the diagnostic and writer cursors.
    // It can preserve existing transport continuity, never grant a write plan.
    if(row.fresh&&before.bound_mask&&!stop_requested.load(std::memory_order_acquire)&&
       !shutdown_requested.load(std::memory_order_acquire)&&!trial.ended){
        gears_hand_stream::Ticket ticket{};
        ticket.stream={frame.session,frame.space_epoch,frame.input_epoch,frame.world_generation};
        ticket.serial=frame.serial;ticket.sequence=frame.sequence;ticket.created_ms=frame.created_ms;
        ticket.controller=frame.controller;ticket.pawn=frame.pawn;ticket.weapon=frame.weapon;
        ticket.units_per_metre=frame.units_per_metre;ticket.valid_mask=frame.valid_mask;
        const auto refresh_begin=timing_clock();
        const auto result=trial.xr_policy.refresh_bound_transport(ticket,trial.epoch,row.now);
        const auto refresh_end=timing_clock();
        if(!refresh_begin||refresh_end<refresh_begin)++transport_trial.invalid_clock;
        else row.refresh_qpc=refresh_end-refresh_begin;
        row.called=true;row.refreshed=result.refreshed_mask;row.reset=result.reset_binding_mask;
        row.reason=unsigned(result.reason);row.capacity_exhausted=result.capacity_exhausted;
        xr_reset_bindings(result.reset_binding_mask);
    }
    const auto after=trial.xr_policy.continuity_snapshot(row.now);row.after_bound=after.bound_mask;
    row.writer_after=trial.xr_cursor;row.policy_serial_after=after.serial;row.policy_sequence_after=after.sequence;
    row.state_unchanged=gears_hand_stream::same(before.context.game,after.context.game)&&
        before.context.expected_native_world_generation==after.context.expected_native_world_generation&&
        before.context.action_epoch==after.context.action_epoch&&before.context.observed_ms==after.context.observed_ms&&
        before.context.idle_ready==after.context.idle_ready&&before.context.clean==after.context.clean&&
        before.context_known==after.context_known&&before.stream_known==after.stream_known&&before.ready==after.ready&&
        gears_hand_stream::same(before.stream,after.stream)&&before.units_per_metre==after.units_per_metre;
    for(unsigned hand=0;hand<2;++hand){
        row.before[hand]=before.old_ticket_age[hand];row.after[hand]=after.old_ticket_age[hand];
        row.before_stamp[hand]=before.source_stamp[hand];row.after_stamp[hand]=after.source_stamp[hand];
    }
    entry_record(entry,before,row.phase,row.epoch,read_begin,transport_trial.count);
    if(transport_trial.count<transport_trial.rows.size())transport_trial.rows[transport_trial.count++]=row;
    else ++transport_trial.dropped;
    // Always return to the caller's original dirty-journal restoration path,
    // including rejection, tracking loss, expiry or diagnostic buffer overflow.
}
#endif
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
void xr_log_pose(gears_weapon::HandWorldFrame const& frame,unsigned mask,unsigned bind_mask
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    ,gears_hand_continuity::Certificate const& certificate
#endif
){
    if(!log_file)return;
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    if(!trial.record_lease)return;
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    fwprintf(log_file,L" continuity=GHW1 native_activation=%llu xr_pid=%llu xr_start=%llu "
        L"stream_epoch=%llu reference_epoch=%llu reference_generation=%llu "
        L"left_loss_epoch=%llu right_loss_epoch=%llu source=%u profile_generation=%llu "
        L"stereo_ns=%lld input_ns=%lld",certificate.native_started_qpc,certificate.xr_publisher_pid,
        certificate.xr_publisher_start,certificate.stream_epoch,certificate.reference_epoch,
        certificate.reference_generation,certificate.tracking_epoch[0],certificate.tracking_epoch[1],
        certificate.source,certificate.profile_generation,certificate.stereo_received_ns,certificate.input_received_ns);
#endif
    fwprintf(log_file,L"\n");fflush(log_file);
}
#if defined(GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL)
void pre_tick(uptr,EquipmentEntryObservation const* entry){
#else
void pre_tick(uptr){
#endif
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return;
    if(journals_dirty()){
        if(!drain_journals(L"pre_tick_dual_drain")&&!trial.ended)end(8,L"dual_drain_failed");
    }else publish_journals();
    if(trial.ended){component_dump_end();return;}
    if(journals_dirty()||component_faulted()){
        end(journals_dirty()?8:7,L"component_restore_fault");component_dump_end();return;
    }
#endif
    if(!active_visible.load(std::memory_order_acquire)||trial.ended)return;
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
#if !defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    if(trial.phase&&trial.timing.frequency&&!stop_requested.load(std::memory_order_acquire)){
        const auto writer_before=trial.xr_cursor,begin=timing_clock();
        gears_weapon::HandWorldFrame early{};
        const bool fresh=trial.xr_open&&xr_bridge.read_pose(shadow.cursor,early);
        const auto finish=timing_clock();
        shadow_sample(fresh?&early:nullptr,GetTickCount64(),writer_before,begin,finish);
    }
#endif
#if defined(GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL)
    if(trial.phase&&!stop_requested.load(std::memory_order_acquire)&&
       !shutdown_requested.load(std::memory_order_acquire)){
        refresh_transport_before_restore(entry);
    }
#endif
#endif
    if(trial.lease.dirty()){
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        timing_pre_restore();
#endif
        if(!restore(L"pre_tick"))return;
    }
#if defined(GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL)
    if(transport_trial.reader_rejected){
        yield_for_equipment(L"xr_transport_consumed_pose_rejected");
        if(!trial.ended)end(7,L"xr_transport_consumed_pose_rejected");
        return;
    }
#endif
    if(stop_requested.load(std::memory_order_acquire)){end(6,L"xr_stopped_after_restore");return;}
    trial.tick_epoch=trial.epoch;
}
void post_tick(uptr pc){
    if(requested.exchange(false,std::memory_order_acq_rel)){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        if(journals_dirty()||!component_reset()){end(8,L"component_start_not_clean");return;}
        hand_writes_relinquished=false;
#endif
        xr_bridge.close();trial={};trial.xr_start=xr_pending;
        trial.xr_world_time=xr_pending.baseline_world_time;
        trial.phase=1;trial.began=GetTickCount64();
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
        if(trial.began>std::numeric_limits<std::uint64_t>::max()-gears_hand_session::recording_ms){
            end(7,L"xr_session_start_clock_exhausted");return;
        }
        trial.session.begin(trial.began);counters_saturated=false;
#endif
        trial.deadline=trial.began+15000;
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
        // The continuous host admits the runtime wire profile. The unrelated
        // optional R2 test path does not select this hand stream's provenance.
        constexpr unsigned continuity_source=static_cast<unsigned>(gears_vr::StereoSource::RuntimeTracked);
        if(!trial.xr_policy.enable_certified_calibration(trial.xr_start.native_started_qpc,continuity_source)){
            end(7,L"xr_certified_policy_initialization_rejected");return;
        }
#endif
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        timing_begin();
#endif
        if(trial.began>trial.xr_start.arm_deadline_ms){end(7,L"xr_arm_deadline_expired_before_tick");return;}
        if(log_file){fwprintf(log_file,L"hand_ik_stream_trial_begin source="
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
            L"GHW1_grip_world "
#else
            L"GVE6_grip_world "
#endif
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
            L"recording_max_leases=%u continuous_session=1 "
#else
            L"max_leases=%u "
#endif
            L"head_controllers_untouched=1 force_evaluation=0 ownership_proved=0 "
            L"external_frozen_native_epoch=%llu native_started_qpc=%llu world_handoff_supported=0\n",
            xr_max_leases,trial.xr_start.expected_native_world_generation,trial.xr_start.native_started_qpc);
            fflush(log_file);}
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        if(log_file){fwprintf(log_file,L"hand_ik_equipment_begin original_hand_phase=1 owned_detached_phase=2 "
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
            L"blocked_phase=0 recording_max_leases=%u recording_ms=15000 session_lifetime=explicit_stop ownership_proved=0\n",
#else
            L"blocked_phase=0 max_leases=%u budget_ms=15000 ownership_proved=0\n",
#endif
            xr_max_leases);fflush(log_file);}
#endif
    }
    if(!active_visible.load(std::memory_order_acquire)||trial.ended||!trial.phase)return;
    const auto now=GetTickCount64();
    if(stop_requested.load(std::memory_order_acquire)){end(6,L"xr_stopped_in_clean_phase");return;}
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    if(!trial.session.running(now,false,shutdown_requested.load(std::memory_order_acquire))){
        end(7,L"xr_continuous_session_ended_or_clock_reversed");return;
    }
#else
    if(now>trial.deadline){end(7,L"xr_bounded_trial_timeout");return;}
    if(leases.load()>=xr_max_leases){
        // Original actions during the final observation can reset readiness,
        // but can never reopen the exhausted write budget.
        if(trial.phase!=4){trial.phase=4;trial.final_ms=now;}
        if(now-trial.final_ms>=250)end(5,L"bounded_xr_dual_hand_recording_complete");
        return;
    }
#endif
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
        if(trial.ended)return; // A clean rollback may still terminate the session.
        trial.xr_equipment_phase=equipment_phase;count_add(trial.xr_equipment_changes);
        if(log_file&&diagnostic_event()){fwprintf(log_file,L"hand_ik_equipment_phase previous=%u current=%u hand_clean=%u "
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
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
        trial.xr_open=xr_bridge.open(false,trial.xr_start.native_started_qpc);trial.xr_next_open=now+250;
#else
        trial.xr_open=xr_bridge.open(false);trial.xr_next_open=now+250;
#endif
    }
    gears_weapon::HandWorldFrame frame{};
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    gears_hand_continuity::WorldFrame continuous_frame{};
    const auto read_result=trial.xr_open?xr_bridge.read(trial.xr_cursor,continuous_frame):
        gears_hand_continuity::WorldRead::none;
    if(read_result==gears_hand_continuity::WorldRead::rejected){
        yield_for_equipment(L"xr_certified_pose_rejected");
        if(!trial.ended)end(7,L"xr_certified_pose_rejected");return;
    }
    const bool fresh=read_result==gears_hand_continuity::WorldRead::received;
    if(fresh)frame=continuous_frame.pose;
#else
    const bool fresh=trial.xr_open&&xr_bridge.read_pose(trial.xr_cursor,frame);
#endif
    gears_hand_stream::Ticket ticket{};
    if(fresh){
        count_add(trial.xr_fresh);
        if(frame.world_generation!=trial.xr_start.expected_native_world_generation){
            end(7,L"xr_native_epoch_changed");return;
        }
        ticket.stream={frame.session,frame.space_epoch,frame.input_epoch,frame.world_generation};
        ticket.serial=frame.serial;ticket.sequence=frame.sequence;ticket.created_ms=frame.created_ms;
        ticket.controller=frame.controller;ticket.pawn=frame.pawn;ticket.weapon=frame.weapon;
        ticket.units_per_metre=frame.units_per_metre;ticket.valid_mask=frame.valid_mask;
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
        ticket.certificate=continuous_frame.certificate;
#endif
    }else count_add(trial.xr_none);
    const auto context=xr_context(value,qualified&&trial.xr_ready);
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    timing_observation();
#endif
    const auto plan=observe_xr_policy(context,fresh?&ticket:nullptr,GetTickCount64(),true);
    xr_reset_bindings(plan.reset_binding_mask);
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    if(trial.xr_policy.certified_failed()){
        yield_for_equipment(L"xr_continuity_certificate_rejected");
        if(!trial.ended)end(7,L"xr_continuity_certificate_rejected");return;
    }
#endif
    if(!plan.accept_mask)return;
    for(unsigned hand=0;hand<2;++hand)if(plan.bind_mask&(1u<<hand)){
        if(!trial.xr_binding[hand].bind(hand?frame.right:frame.left,hand?value.right:value.left)||
           !trial.xr_policy.commit_binding(plan,1u<<hand)){
            end(7,L"xr_grip_to_bone_binding_rejected");return;
        }
        count_add(trial.xr_binds);
    }
    if(!xr_targets(frame,plan.accept_mask)){end(7,L"xr_bone_target_invalid");return;}
    trial.sequence=frame.sequence;
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    if(leases.load(std::memory_order_acquire)==std::numeric_limits<unsigned>::max()){
        end(7,L"xr_lease_counter_exhausted");return;
    }
#endif
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
    const unsigned mask=authorize_current_xr_plan(plan,authorize_context,GetTickCount64());
#else
    const unsigned mask=authorize_current_xr_plan(plan,xr_context(check,trial.xr_ready),GetTickCount64());
#endif
    if(!mask)return;
    if(!log_file||ferror(log_file)){end(7,L"xr_log_unavailable_before_write");return;}
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    trial.record_lease=trial.session.record_lease(GetTickCount64());
#endif
    {
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        EquipmentTimingScope timing(EquipmentTiming::write_evidence);
#endif
        sample(L"before",value,lease,0);
        xr_log_pose(frame,mask,plan.bind_mask
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
            ,continuous_frame.certificate
#endif
        );
    }
    // fflush may block. Recheck the receipt and game sample after all logging,
    // immediately before the first journal snapshot or mutation.
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    if(!xr_equipment_gate(check,equipment_phase,L"after_log_flush",EquipmentTiming::postflush_phase))return;
    const auto final_context=xr_context(check,trial.xr_ready);
#endif
    const auto write_now=GetTickCount64();
    if(stop_requested.load(std::memory_order_acquire)){end(6,L"xr_stopped_before_write");return;}
#if !defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    if(write_now>trial.deadline){end(7,L"xr_deadline_before_write");return;}
#endif
    if(ferror(log_file)||trial.tick_epoch!=trial.epoch||
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
       authorize_current_xr_plan(plan,final_context,write_now)!=mask){
#else
       authorize_current_xr_plan(plan,xr_context(check,trial.xr_ready),write_now)!=mask){
#endif
        end(7,L"xr_receipt_or_log_expired_before_write");return;
    }
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    safe_to_unload.store(false,std::memory_order_release);
    if(!component_apply(plan,frame,check,mask,lease)){
        drain_journals(L"component_acquire_failed");
        end(journals_dirty()?8:7,L"component_acquire_failed");return;
    }
    // The component setter may dispatch engine work. Reacquire the hand sample
    // and certify this exact Plan after the entire component transaction.
    Snapshot after_component{};
    if(!handik::snapshot(pc,after_component)||!xr_identity(after_component)||
       !after_component.action_idle||!handik::baseline_strengths(after_component)||
       !handik::same_controls(check,after_component)||!slots_quiet(after_component)||
       trial.tick_epoch!=trial.epoch||
       !xr_equipment_gate(after_component,equipment_phase,L"after_component",EquipmentTiming::prewrite_phase)||
       (equipment_phase==1&&(mask&2)&&!component_owned())||
       authorize_current_xr_plan(plan,xr_context(after_component,trial.xr_ready),GetTickCount64())!=mask){
        drain_journals(L"hand_reauthorize_after_component");
        end(journals_dirty()?8:7,L"hand_reauthorize_after_component");return;
    }
    check=after_component;
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        drain_journals(L"hand_acquire_failed_after_component");
#endif
        end(trial.lease.dirty()?8:7,L"xr_lease_acquisition_failed");return;
    }
    trial.applied_mask=mask;leases=lease;count_add(trial.xr_masks[mask]);
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    EquipmentTimingScope timing(EquipmentTiming::written_snapshot_evidence);
#endif
    Snapshot written{};
    if(!handik::snapshot(pc,written)||!same_layout(check,written)){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        drain_journals(L"xr_failed_written_snapshot");
#else
        restore(L"xr_failed_written_snapshot");
#endif
        end(7,L"xr_written_snapshot_unknown");return;
    }
    sample(L"written",written,lease,mask);
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    component_log_written();
#endif
}
