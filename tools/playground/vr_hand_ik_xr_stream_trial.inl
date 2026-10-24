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
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
namespace world_reader_diagnostic {
bool dumped{};
constexpr const wchar_t* branches[]={L"none",L"consumer_or_mapping",L"lock",L"header",L"serial_regressed",L"frame"};
constexpr const wchar_t* checks[]={L"consumer",L"mapped",L"locked",L"frequency_query",L"frequency_positive",L"magic",L"version",L"size",
    L"pid",L"game_start",L"native_present",L"native_matches",L"frequency_matches",L"serial_available",L"serial_not_regressed",
    L"frame_serial",L"frame_shape",L"frame_native",L"created_not_future",L"pose_recent",L"certificate_recent"};
static_assert(sizeof(checks)/sizeof(*checks)==unsigned(gears_hand_continuity::WorldRejectCheck::count));
void reset(){xr_bridge.reset_read_rejection();dumped=false;}
void dump(){
    if(dumped||!trial.ended||journals_dirty()||dual_draining||!log_file)return;
    dumped=true;
    auto const& r=xr_bridge.first_read_rejection();auto const& o=r.observation;
    fwprintf(log_file,L"hand_world_reader_rejection_begin schema=%u capacity=1 record_size=%u packet_size=%zu known=%u "
        L"branch=%ls evaluated=%08x passed=%08x available=%08x cursor_before=%llu cursor_after=%llu "
        L"expected_pid=%lu expected_game_start=%llu expected_native_started_qpc=%llu "
        L"lock_known=%u lock_result=%lu packet_available=%u frequency_call_known=%u frequency_value_known=%u frequency_result=%d frequency=%lld "
        L"now_known=%u now=%llu now_ns_known=%u now_ns=%lld clocks_resampled=0 helper_subchecks_known=0\n",
        r.schema,r.size,sizeof(r.packet),r.known,branches[unsigned(r.branch)],o.evaluated,o.passed,o.available,
        o.cursor_before,o.cursor_after,o.expected_pid,o.expected_game_start,o.expected_native_started_qpc,
        unsigned((o.available&gears_hand_continuity::read_lock)!=0),o.lock_result,
        unsigned((o.available&gears_hand_continuity::read_packet)!=0),unsigned((o.available&gears_hand_continuity::read_frequency)!=0),
        unsigned((o.available&gears_hand_continuity::read_frequency)!=0&&o.frequency_result!=FALSE),
        o.frequency_result,o.frequency,unsigned((o.available&gears_hand_continuity::read_now)!=0),o.now,
        unsigned((o.available&gears_hand_continuity::read_now_ns)!=0),o.now_ns);
    fwprintf(log_file,L"hand_world_reader_rejection_checks schema=1");
    for(unsigned i=0;i<unsigned(gears_hand_continuity::WorldRejectCheck::count);++i)
        fwprintf(log_file,L" %ls=%u%u",checks[i],unsigned((o.evaluated&(1u<<i))!=0),unsigned((o.passed&(1u<<i))!=0));
    fwprintf(log_file,L"\n");
    if(o.available&gears_hand_continuity::read_packet){
        auto const& p=r.packet;
        fwprintf(log_file,L"hand_world_reader_rejection_packet schema=1 magic=%08x version=%u size=%u latest_serial=%llu "
            L"pose_serial=%llu created_ms=%llu sequence=%llu stereo_ns=%lld input_ns=%lld hex=",
            p.magic,p.version,p.size,p.latest_serial,p.frame.pose.serial,p.frame.pose.created_ms,p.frame.pose.sequence,
            p.frame.certificate.stereo_received_ns,p.frame.certificate.input_received_ns);
        auto const* bytes=reinterpret_cast<unsigned char const*>(&p);
        for(std::size_t i=0;i<sizeof(p);++i)fwprintf(log_file,L"%02x",unsigned(bytes[i]));
        fwprintf(log_file,L"\n");
    }
    fwprintf(log_file,L"hand_world_reader_rejection_end schema=1\n");fflush(log_file);
}
} // namespace world_reader_diagnostic
#endif

#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
namespace tick_boundary_diagnostic {void reset();void dump();}
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
struct AuthObservation {Auth value{};bool known{};};
thread_local AuthObservation* observed_authorization{};
#endif
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
    if(!row.passed)component_cost::failure(row.meta.forward,row.faulted);
    if(!row.passed&&!state.first_known){state.rows[0]=row;state.first_known=true;}
    state.rows[1+state.next]=row;state.next=(state.next+1)%63;
    if(state.count<63)++state.count;
    else if(state.overwritten!=UINT64_MAX)++state.overwritten;else state.counter_saturated=true;
}
bool gate(Stage stage,bool passed,unsigned detail=0,unsigned bits=0){
    Row row{};row.stage=stage;row.passed=passed;row.detail=detail;row.bits=bits;add(row);return passed;
}
void authorization(Auth const& value,Stage stage=Stage::authorize){
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    if(observed_authorization){observed_authorization->value=value;observed_authorization->known=true;}
#endif
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
void reset(){state={};
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    world_reader_diagnostic::reset();tick_boundary_diagnostic::reset();
#endif
}
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
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    world_reader_diagnostic::dump();tick_boundary_diagnostic::dump();
#endif
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

#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
// Fixed tail evidence only. The stack token never authorizes, consumes, restores,
// or resamples an MS/NS clock. Inert recursive callbacks mask the parent token.
namespace tick_boundary_diagnostic {
enum class Boundary:unsigned {pre_entry,after_drain,post_entry,count};
constexpr unsigned capacity=8,boundaries=unsigned(Boundary::count);
struct Source {
    std::uint64_t serial{},sequence{},created_ms{},session{},space_epoch{},input_epoch{},world_generation{};
    std::uintptr_t controller{},pawn{},weapon{};
    unsigned valid_mask{},grip_down_mask{};
    gears_hand_continuity::Certificate certificate{};
};
struct Row {
    std::uint64_t generation{},tick{},parent_tick{},qpc[boundaries]{},snapshot_ms{};
    uptr object{},function{},pc{},pawn{},mesh{},weapon{};
    unsigned sampled{},qpc_known{},epoch{},tick_epoch{},phase{},lease{},terminal_site{};
    int mesh_tick{};float world_time{};
    bool closed{},terminal{},snapshot_known{},mesh_tick_known{},plan_known{},attempt_known{};
    bool read_outcome_known{},final_auth_called{},final_auth_known{};
    gears_hand_continuity::WorldRead read_result{};
    gears_hand_continuity::WorldReadSuccess success{};
    Source source{};gears_hand_stream::Plan plan_value{};
    component_trace::Auth final_auth{};unsigned final_result{};
};
struct State {
    std::array<Row,capacity> rows{};
    std::uint64_t generation{1},next_tick{},overwritten{},terminal_tick{};
    unsigned next{},count{},open{},reset_open{};
    bool frozen{},dumped{},counter_saturated{},disabled{},terminal_context_known{};
};
State state;
struct Token;
thread_local Token* current{};
void flush();
struct Token {
    Token* previous;uptr object,function;bool active{};
    union {Row row;}; // No large row initialization for an inert engine callback.
    Token(uptr object_value,uptr function_value) noexcept:
        previous(current),object(object_value),function(function_value){current=this;}
    Token(Token const&)=delete;Token& operator=(Token const&)=delete;
    void activate() noexcept {
        if(active||state.disabled)return;
        if(state.open==(std::numeric_limits<unsigned>::max)()||state.next_tick==UINT64_MAX){state.counter_saturated=true;return;}
        ::new(static_cast<void*>(&row)) Row{}; // Placement only; storage is in this stack token.
        row.generation=state.generation;row.tick=++state.next_tick;
        if(previous&&previous->active&&previous->row.generation==state.generation)row.parent_tick=previous->row.tick;
        row.object=object;row.function=function;row.epoch=trial.epoch;row.tick_epoch=trial.tick_epoch;row.phase=trial.phase;
        ++state.open;active=true;
    }
    void stamp(Boundary which) noexcept {
        activate();if(!active)return;
        const unsigned index=unsigned(which),bit=1u<<index;
        if(row.sampled&bit)return;
        row.sampled|=bit;LARGE_INTEGER qpc{};
        if(QueryPerformanceCounter(&qpc)&&qpc.QuadPart>0){row.qpc[index]=std::uint64_t(qpc.QuadPart);row.qpc_known|=bit;}
    }
    void adopt_generation() noexcept {
        if(!active)return;
        const auto saved=row;
        row={};row.generation=state.generation;row.tick=saved.tick;row.object=object;row.function=function;
        row.sampled=saved.sampled;row.qpc_known=saved.qpc_known;
        for(unsigned i=0;i<boundaries;++i)row.qpc[i]=saved.qpc[i];
        // A reset cannot give an old parent a new-session identity.
        if(previous&&previous->active&&previous->row.generation==state.generation)row.parent_tick=previous->row.tick;
    }
    bool writable() const noexcept {return active&&!state.disabled&&row.generation==state.generation&&!state.frozen;}
    void snapshot(Snapshot const& value) noexcept {
        if(!writable())return;
        row.snapshot_known=true;row.snapshot_ms=value.ms;row.mesh_tick_known=value.mesh_tick_known;
        row.mesh_tick=value.mesh_tick;row.world_time=value.world_time;
        row.pc=value.pc;row.pawn=value.pawn;row.mesh=value.mesh;row.weapon=value.weapon;
        row.epoch=trial.epoch;row.tick_epoch=trial.tick_epoch;row.phase=trial.phase;
    }
    void read(gears_hand_continuity::WorldRead result,gears_hand_continuity::WorldReadSuccess const& success,
              gears_hand_continuity::WorldFrame const& frame) noexcept {
        if(!writable())return;
        row.read_outcome_known=true;row.read_result=result;
        if(result!=gears_hand_continuity::WorldRead::received||!success.known)return;
        row.success=success;auto const& p=frame.pose;
        row.source={p.serial,p.sequence,p.created_ms,p.session,p.space_epoch,p.input_epoch,p.world_generation,
            p.controller,p.pawn,p.weapon,unsigned(p.valid_mask),unsigned(p.grip_down_mask),frame.certificate};
    }
    void plan(gears_hand_stream::Plan const& value) noexcept {
        if(writable()){row.plan_known=true;row.plan_value=value;}
    }
    void attempt(unsigned lease) noexcept {if(writable()){row.attempt_known=true;row.lease=lease;}}
    void terminal(unsigned site) noexcept {
        if(state.frozen||state.disabled)return;
        activate();
        state.frozen=true;
        if(active&&row.generation==state.generation){
            row.terminal=true;row.terminal_site=site;state.terminal_context_known=true;state.terminal_tick=row.tick;
        }
    }
    ~Token(){
        current=previous;
        if(!active)return;
        row.closed=true;--state.open;
        if(!state.disabled&&row.generation==state.generation&&(!state.frozen||row.terminal)){
            state.rows[state.next]=row;state.next=(state.next+1)%capacity;
            if(state.count<capacity)++state.count;
            else if(state.overwritten!=UINT64_MAX)++state.overwritten;else state.counter_saturated=true;
        }
        flush();
    }
};
void reset(){
    // Called only by the existing successful component_reset. Do not destroy
    // live stack tokens or their open count; post_tick adopts its current token.
    const auto generation=state.generation,next_tick=state.next_tick;const auto open=state.open;
    const bool saturated=state.counter_saturated;
    state={};state.next_tick=next_tick;state.open=open;state.reset_open=open;state.counter_saturated=saturated;
    if(generation==UINT64_MAX){state.generation=generation;state.disabled=true;state.counter_saturated=true;}
    else state.generation=generation+1;
}
template<class Call> unsigned final_authorize(Token& token,Call&& call){
    component_trace::AuthObservation observation{};
    struct Capture {
        component_trace::AuthObservation* previous;
        explicit Capture(component_trace::AuthObservation& value):previous(component_trace::observed_authorization){
            component_trace::observed_authorization=&value;
        }
        ~Capture(){component_trace::observed_authorization=previous;}
    } capture(observation);
    if(token.writable())token.row.final_auth_called=true;
    const unsigned result=call();
    if(token.writable()){
        token.row.final_result=result;token.row.final_auth_known=observation.known;
        if(observation.known)token.row.final_auth=observation.value;
    }
    return result;
}
void flush(){
    if(state.dumped||!state.frozen||state.open||!trial.ended||dual_draining||component_trace::state.depth||!log_file)return;
    if(journals_dirty())return;
    state.dumped=true;
    std::uint64_t frequency{};
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    frequency=trial.timing.frequency; // Original trial QPF; no diagnostic QPF call.
#endif
    fwprintf(log_file,L"hand_tick_boundary_begin schema=1 capacity=8 count=%u generation=%llu overwritten=%llu "
        L"terminal_context_known=%u terminal_tick=%llu open=%u reset_open=%u counter_saturated=%u "
        L"frequency=%llu frequency_resampled=0 ms_ns_resampled=0 extra_authorizations=0\n",
        state.count,state.generation,state.overwritten,unsigned(state.terminal_context_known),state.terminal_tick,
        state.open,state.reset_open,unsigned(state.counter_saturated),frequency);
    const unsigned start=state.count==capacity?state.next:0;
    for(unsigned i=0;i<state.count;++i){
        auto const& r=state.rows[(start+i)%capacity];auto const& s=r.source;auto const& c=s.certificate;
        auto const& p=r.plan_value;auto const& a=r.final_auth;
        fwprintf(log_file,L"hand_tick_boundary schema=1 generation=%llu tick=%llu parent_tick=%llu object=%p function=%p "
            L"closed=%u terminal=%u terminal_site=%u sampled=%u qpc_known=%u pre_qpc=%llu drain_qpc=%llu post_qpc=%llu "
            L"snapshot_known=%u snapshot_ms=%llu mesh_tick_known=%u mesh_tick=%d world_time=%.9g pc=%p pawn=%p mesh=%p weapon=%p "
            L"phase=%u epoch=%u tick_epoch=%u attempt_known=%u lease=%u read_outcome_known=%u read_result=%u read_success_known=%u "
            L"read_now=%llu read_now_ns=%lld serial=%llu sequence=%llu created_ms=%llu session=%llu space_epoch=%llu input_epoch=%llu "
            L"world_generation=%llu source_controller=%p source_pawn=%p source_weapon=%p valid_mask=%u grip_down_mask=%u "
            L"native_started_qpc=%llu xr_pid=%llu xr_start=%llu stream_epoch=%llu reference_epoch=%llu reference_generation=%llu "
            L"tracking_left=%llu tracking_right=%llu profile_generation=%llu source=%u stereo_ns=%lld input_ns=%lld "
            L"plan_known=%u revision=%llu observed_now=%llu plan_ms=%llu plan_has_ticket=%u plan_serial=%llu plan_sequence=%llu "
            L"plan_created_ms=%llu accept_mask=%u bind_mask=%u reset_mask=%u final_auth_called=%u final_auth_known=%u "
            L"auth_fields_known=%u auth_checked=%u auth_rejected=%u now=%llu live_now=%llu commit_now=%llu write_until=%llu "
            L"authorize_ns=%lld commit_ns=%lld ticket_ms=%llu auth_plan_ms=%llu context_ms=%llu auth_stereo_ns=%lld "
            L"auth_input_ns=%lld policy_mask=%u auth_result=%u final_result=%u\n",
            r.generation,r.tick,r.parent_tick,reinterpret_cast<void*>(r.object),reinterpret_cast<void*>(r.function),
            unsigned(r.closed),unsigned(r.terminal),r.terminal_site,r.sampled,r.qpc_known,r.qpc[0],r.qpc[1],r.qpc[2],
            unsigned(r.snapshot_known),r.snapshot_ms,unsigned(r.mesh_tick_known),r.mesh_tick,r.world_time,
            reinterpret_cast<void*>(r.pc),reinterpret_cast<void*>(r.pawn),reinterpret_cast<void*>(r.mesh),reinterpret_cast<void*>(r.weapon),
            r.phase,r.epoch,r.tick_epoch,unsigned(r.attempt_known),r.lease,unsigned(r.read_outcome_known),unsigned(r.read_result),r.success.known,
            r.success.now,r.success.now_ns,s.serial,s.sequence,s.created_ms,s.session,s.space_epoch,s.input_epoch,s.world_generation,
            reinterpret_cast<void*>(s.controller),reinterpret_cast<void*>(s.pawn),reinterpret_cast<void*>(s.weapon),s.valid_mask,s.grip_down_mask,
            c.native_started_qpc,c.xr_publisher_pid,c.xr_publisher_start,c.stream_epoch,c.reference_epoch,c.reference_generation,
            c.tracking_epoch[0],c.tracking_epoch[1],c.profile_generation,c.source,c.stereo_received_ns,c.input_received_ns,
            unsigned(r.plan_known),p.revision,p.observed_now,p.context.observed_ms,unsigned(p.has_ticket),p.ticket.serial,p.ticket.sequence,
            p.ticket.created_ms,p.accept_mask,p.bind_mask,p.reset_binding_mask,unsigned(r.final_auth_called),unsigned(r.final_auth_known),
            a.known,a.checked,a.rejected,a.now,a.live_now,a.commit_now,a.write_until,a.authorize_ns,a.commit_ns,a.ticket_ms,a.plan_ms,
            a.context_ms,a.stereo_ns,a.input_ns,a.mask,a.result,r.final_result);
    }
    fwprintf(log_file,L"hand_tick_boundary_end schema=1\n");fflush(log_file);
}
void dump(){
    if(trial.ended&&!state.frozen){
        if(current)current->terminal(0); // Exact current callback; no latest/first-failure substitution.
        else state.frozen=true; // A termination without a hook token has no claimed terminal row.
    }
    flush();
}
} // namespace tick_boundary_diagnostic
#endif

// Separate from component_cost's acquisition-owned aggregates. These fixed
// rows only observe the post-component short circuit; QPC grants no authority.
namespace component_tail_cost {
enum class Stage:unsigned {snapshot,guards,owned,owned_read,authorize,count};
constexpr unsigned capacity=8,stage_count=unsigned(Stage::count);
constexpr const wchar_t* names[]={L"snapshot",L"guards",L"component_owned",L"owned_live_read",L"authorize"};
static_assert(sizeof(names)/sizeof(*names)==stage_count);
struct Row {
    std::uint64_t sequence{},revision{};
    unsigned lease{},phase{},mask{},ran{},result_known{},passed{},qpc_known{};
    std::array<std::uint64_t,stage_count> begin{},finish{};
    bool closed{};
};
struct State {
    std::array<Row,capacity> rows{};
    std::uint64_t seen{},dropped{};
    unsigned count{},open{};
    bool dumped{},counter_saturated{};
};
State state;
thread_local Row* active{};
std::uint64_t stamp() noexcept {
    LARGE_INTEGER value{};
    return QueryPerformanceCounter(&value)&&value.QuadPart>0?std::uint64_t(value.QuadPart):0;
}
void increment(std::uint64_t& value) noexcept {
    if(value!=UINT64_MAX)++value;else state.counter_saturated=true;
}
void reset(){state={};active=nullptr;}
struct Attempt {
    Row* saved;Row* row{};
    Attempt(std::uint64_t sequence,std::uint64_t revision,unsigned lease,unsigned phase,unsigned mask):saved(active){
        active=nullptr;
        if(saved||state.dumped)return;
        increment(state.seen);
        if(state.count==capacity){increment(state.dropped);return;}
        row=&state.rows[state.count++];row->sequence=sequence;row->revision=revision;
        row->lease=lease;row->phase=phase;row->mask=mask;active=row;++state.open;
    }
    ~Attempt(){if(row){row->closed=true;--state.open;}active=saved;}
    Attempt(Attempt const&)=delete;Attempt& operator=(Attempt const&)=delete;
};
struct Span {
    Row* row{};unsigned index{},bit{};
    explicit Span(Stage stage){
        index=unsigned(stage);if(!active||index>=stage_count)return;
        bit=1u<<index;if(active->ran&bit)return;
        row=active;row->ran|=bit;row->begin[index]=stamp();
    }
    void close(bool known,bool result){
        if(!row)return;
        row->finish[index]=stamp();
        if(row->begin[index]&&row->finish[index]>=row->begin[index])row->qpc_known|=bit;
        if(known)row->result_known|=bit;if(result)row->passed|=bit;row=nullptr;
    }
    void finish(bool result){close(true,result);}
    ~Span(){close(false,false);}
    Span(Span const&)=delete;Span& operator=(Span const&)=delete;
};
template<class F>bool test(Stage stage,F&& call){
    Span span(stage);const bool result=call();span.finish(result);return result;
}
void dump(bool clean){
    if(!clean||!state.count||state.dumped||state.open||active||dual_draining||!log_file)return;
    state.dumped=true;LARGE_INTEGER value{};
    const std::uint64_t frequency=QueryPerformanceFrequency(&value)&&value.QuadPart>0?std::uint64_t(value.QuadPart):0;
    fwprintf(log_file,L"hand_weapon_component_tail_cost_begin schema=1 capacity=%u count=%u seen=%llu dropped=%llu "
        L"frequency=%llu counter_saturated=%u nested_not_additive=1 extra_getters=0 extra_authorizations=0\n",
        capacity,state.count,state.seen,state.dropped,frequency,unsigned(state.counter_saturated));
    for(unsigned i=0;i<state.count;++i){const auto& row=state.rows[i];
        for(unsigned j=0;j<stage_count;++j){const unsigned bit=1u<<j;
            fwprintf(log_file,L"hand_weapon_component_tail_cost schema=1 row=%u stage=%ls lease=%u phase=%u mask=%u "
                L"sequence=%llu revision=%llu ran=%u result_known=%u result=%u closed=%u qpc_known=%u clock_known=%u "
                L"begin_qpc=%llu finish_qpc=%llu frequency=%llu\n",i,names[j],row.lease,row.phase,row.mask,
                row.sequence,row.revision,unsigned((row.ran&bit)!=0),unsigned((row.result_known&bit)!=0),
                unsigned((row.passed&bit)!=0),unsigned(row.closed),unsigned((row.qpc_known&bit)!=0),
                unsigned(frequency&&(row.qpc_known&bit)),row.begin[j],row.finish[j],frequency);
        }
    }
    fwprintf(log_file,L"hand_weapon_component_tail_cost_end schema=1\n");fflush(log_file);
}
} // namespace component_tail_cost
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

#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
// Classification of this original call only; no pose/write authority or new clock.
bool continuous_prewrite_cancelled(component_trace::AuthObservation const& observation,unsigned intended_mask) noexcept {
    auto const& auth=observation.value;
    if(!observation.known||auth.result||!(intended_mask&2u)||(intended_mask&~3u))return false;
    if(auth.known==67u&&auth.mask==0u)
        return (auth.checked==7u&&auth.rejected==4u)||(auth.checked==15u&&auth.rejected==8u);
    if(auth.known!=127u||!(auth.mask&2u)||(auth.mask&~3u)||(auth.mask&~intended_mask))return false;
    return (auth.checked==4095u&&auth.rejected==2048u)||(auth.checked==8191u&&auth.rejected==4096u);
}
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
bool persistent_hand_prewrite_cancelled(component_trace::AuthObservation const& observation,unsigned intended_mask) noexcept {
    auto const& auth=observation.value;
    if(!observation.known||auth.result||!intended_mask||(intended_mask&~3u))return false;
    if(auth.known==67u&&auth.mask==0u)
        return (auth.checked==7u&&auth.rejected==4u)||(auth.checked==15u&&auth.rejected==8u);
    if(auth.known!=127u||!auth.mask||(auth.mask&~3u)||(auth.mask&~intended_mask))return false;
    return (auth.checked==4095u&&auth.rejected==2048u)||(auth.checked==8191u&&auth.rejected==4096u);
}
#endif
#endif

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
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    ,component_continuous::Mode write_mode
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
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    fwprintf(log_file,L" hand_only=%u component_interval=%llu",
        unsigned(write_mode==component_continuous::Mode::hand_only),component_continuous::state.interval_id);
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
#if defined(GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL)
void pre_tick(uptr,EquipmentEntryObservation const* entry,tick_boundary_diagnostic::Token& tick_diagnostic){
#else
void pre_tick(uptr,tick_boundary_diagnostic::Token& tick_diagnostic){
#endif
#else
#if defined(GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL)
void pre_tick(uptr,EquipmentEntryObservation const* entry){
#else
void pre_tick(uptr){
#endif
#endif
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    tick_diagnostic.stamp(tick_boundary_diagnostic::Boundary::pre_entry);
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
    if(component_hold::pre_tick())return;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    if(!component_continuous::normal_tick())return;
#else
    if(journals_dirty()){
        if(!drain_journals(L"pre_tick_dual_drain",true)&&!trial.ended)end(8,L"dual_drain_failed");
    }else publish_journals();
#endif
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    tick_diagnostic.stamp(tick_boundary_diagnostic::Boundary::after_drain);
#endif
    if(trial.ended){component_dump_end();return;}
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    if(trial.lease.dirty()||trial.lease.faulted()||component_faulted()){
        end(journals_dirty()?8:7,L"component_restore_fault");component_dump_end();return;
    }
#else
    if(journals_dirty()||component_faulted()){
        end(journals_dirty()?8:7,L"component_restore_fault");component_dump_end();return;
    }
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
void post_tick(uptr pc,tick_boundary_diagnostic::Token& tick_diagnostic){
    tick_diagnostic.stamp(tick_boundary_diagnostic::Boundary::post_entry);
#else
void post_tick(uptr pc){
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
    if(component_hold::post_tick())return;
#endif
    if(requested.exchange(false,std::memory_order_acq_rel)){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        if(journals_dirty()||!component_reset()){end(8,L"component_start_not_clean");return;}
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
        tick_diagnostic.adopt_generation();
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    component_continuous::PostTick continuous_tick;
#endif
    if(!active_visible.load(std::memory_order_acquire)||trial.ended||!trial.phase)return;
    const auto now=GetTickCount64();
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
#if !defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(!component_forward_open()){end(7,L"continuous_writer_closed");return;}
    // This one diagnostic interval cannot outlive the original finite capture.
    if(now>trial.deadline||leases.load()>=xr_max_leases){end(5,L"continuous_component_interval_complete");return;}
#endif
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL) && defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    component_names::Scope prefix_names_scope(trial.xr_equipment_phase==1 ? true : component_names::enabled);
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    tick_diagnostic.snapshot(value);
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        const bool clean=(previous==0&&equipment_phase==1)?
            yield_for_equipment(L"equipment_phase_changed",true):yield_for_equipment(L"equipment_phase_changed");
#else
        const bool clean=yield_for_equipment(L"equipment_phase_changed");
#endif
        if(trial.ended)return; // A clean rollback may still terminate the session.
        trial.xr_equipment_phase=equipment_phase;count_add(trial.xr_equipment_changes);
        if(log_file&&diagnostic_event()){fwprintf(log_file,L"hand_ik_equipment_phase previous=%u current=%u hand_clean=%u "
            L"epoch=%u leases=%u consume_blocked_baseline=1 writes=0\n",previous,equipment_phase,
            unsigned(clean),trial.epoch,leases.load());fflush(log_file);}
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
        if(component_continuous::pause_pending()){continuous_tick.suspending=true;return;}
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
        component_continuous::state.resume_stable=false;
#endif
    }else if(!trial.xr_ready&&value.mesh_tick!=trial.previous.mesh_tick){
        if(trial.stability.observe(now,true,handik::same_controls(trial.previous,value))){
            for(unsigned channel=0;channel<4;++channel)
                if(!target_layout(value.controls[indices[channel]].object,(channel&1)!=0)){
                    end(7,L"xr_target_abi_unknown");return;
                }
            if(trial.epoch)trial.resumed=true;
            trial.xr_ready=true;trial.phase=2;status=2;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
            component_continuous::state.resume_stable=true;
#endif
        }
    }
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    // Age-only suspension does not invalidate a still-authenticated hand mount.
    // Its component resume still waits for the original stability gate.
    if(qualified&&trial.xr_ready&&component_continuous::state.interval==component_continuous::Interval::waiting&&
       !component_continuous::state.resume_stable&&value.mesh_tick!=trial.previous.mesh_tick)
        component_continuous::state.resume_stable=trial.stability.observe(now,true,handik::same_controls(trial.previous,value));
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    gears_hand_continuity::WorldReadSuccess read_success{};
    const auto read_result=trial.xr_open?xr_bridge.read(trial.xr_cursor,continuous_frame,&read_success):
        gears_hand_continuity::WorldRead::none;
    tick_diagnostic.read(read_result,read_success,continuous_frame);
#else
    const auto read_result=trial.xr_open?xr_bridge.read(trial.xr_cursor,continuous_frame):
        gears_hand_continuity::WorldRead::none;
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    tick_diagnostic.plan(plan);
#endif
    xr_reset_bindings(plan.reset_binding_mask);
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(trial.ended)return; // Still evaluate certified_failed before any recoverable pause.
#else
    if(!component_forward_open()||trial.ended)return;
    if(component_continuous::state.begun&&!(plan.accept_mask&2)){
        end(7,L"continuous_no_fresh_right_plan");return;
    }
#endif
#endif
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    if(trial.xr_policy.certified_failed()){
        yield_for_equipment(L"xr_continuity_certificate_rejected");
        if(!trial.ended)end(7,L"xr_continuity_certificate_rejected");return;
    }
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(component_continuous::pause_pending()){continuous_tick.suspending=true;return;}
    if(component_continuous::state.hand_only_journal){
        end(journals_dirty()?8u:7u,L"persistent_previous_hand_journal_pending");return;
    }
    const bool joint_ready=component_forward_open()&&(plan.accept_mask&2)&&equipment_phase==1&&
        component_continuous::state.resume_stable;
    if(!joint_ready){
        if(component_continuous::state.interval==component_continuous::Interval::owned){
            if(!component_forward_open()){end(7,L"persistent_writer_closed_without_pause");return;}
            const bool age_only=!fresh&&qualified&&trial.xr_ready&&!(plan.reset_binding_mask&2);
            component_continuous::suspend_interval(age_only?L"persistent_no_receipt":L"persistent_right_not_ready",age_only);
            continuous_tick.suspending=true;return; // No hand writes before component handback completes.
        }
        if(!component_hand_only_ready()||trial.lease.dirty()||component_continuous::state.hand_only_journal){
            end(journals_dirty()?8u:7u,L"persistent_hand_only_not_clean");return;
        }
        if(!plan.accept_mask){continuous_tick.waiting=true;return;}
        if(!fresh||(plan.accept_mask&~3u)||(equipment_phase!=1&&equipment_phase!=2)){
            end(7,L"persistent_hand_only_plan_invalid");return;
        }
        continuous_tick.mode=component_continuous::Mode::hand_only;
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    tick_diagnostic.attempt(lease);
#endif
    Snapshot check{};
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    const bool checked=handik::snapshot(pc,check)&&xr_identity(check);
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(checked&&!xr_equipment_gate(check,equipment_phase,L"prewrite_snapshot",EquipmentTiming::prewrite_phase)){
        if(component_continuous::pause_pending())continuous_tick.suspending=true;
        return;
    }
#else
    if(checked&&!xr_equipment_gate(check,equipment_phase,L"prewrite_snapshot",EquipmentTiming::prewrite_phase))return;
#endif
    if(!checked||!check.action_idle||
#else
    if(!handik::snapshot(pc,check)||!xr_identity(check)||!check.action_idle||
#endif
       !handik::baseline_strengths(check)||!handik::same_controls(value,check)||
       value.mesh_world!=check.mesh_world||value.left!=check.left||value.right!=check.right||
       !slots_quiet(check)||trial.tick_epoch!=trial.epoch){
        end(7,L"xr_prewrite_state_changed");return;
    }
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    const auto authorize_context=xr_context(check,trial.xr_ready);
    component_trace::AuthObservation prewrite_authorization{};
    unsigned mask{};
    {
        struct PrewriteAuthorizationCapture {
            component_trace::AuthObservation* previous;
            explicit PrewriteAuthorizationCapture(component_trace::AuthObservation& destination):previous(component_trace::observed_authorization){
                component_trace::observed_authorization=&destination;
            }
            ~PrewriteAuthorizationCapture(){component_trace::observed_authorization=previous;}
        } capture(prewrite_authorization);
        mask=authorize_current_xr_plan(plan,authorize_context,GetTickCount64());
    } // Restore the caller's observer before any diagnostic, end or drain.
    if(!mask){
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
        const bool cancel_only=continuous_tick.mode==component_continuous::Mode::hand_only?
            persistent_hand_prewrite_cancelled(prewrite_authorization,plan.accept_mask):
            continuous_prewrite_cancelled(prewrite_authorization,plan.accept_mask);
#else
        const bool cancel_only=continuous_prewrite_cancelled(prewrite_authorization,plan.accept_mask);
#endif
        if(log_file){
            auto const& auth_packet=prewrite_authorization.value;
            fwprintf(log_file,L"hand_weapon_component_prewrite_rejection schema=1 observation_known=%u "
                L"cancel_only=%u auth_fields_known=%u auth_checked=%u auth_rejected=%u now=%llu live_now=%llu "
                L"commit_now=%llu write_until=%llu authorize_ns=%lld commit_ns=%lld ticket_ms=%llu plan_ms=%llu "
                L"context_ms=%llu stereo_ns=%lld input_ns=%lld intended_mask=%u policy_mask=%u returned_mask=%u\n",
                unsigned(prewrite_authorization.known),unsigned(cancel_only),auth_packet.known,auth_packet.checked,
                auth_packet.rejected,auth_packet.now,auth_packet.live_now,auth_packet.commit_now,auth_packet.write_until,
                auth_packet.authorize_ns,auth_packet.commit_ns,auth_packet.ticket_ms,auth_packet.plan_ms,auth_packet.context_ms,
                auth_packet.stereo_ns,auth_packet.input_ns,plan.accept_mask,auth_packet.mask,auth_packet.result);fflush(log_file);
        }
        end(cancel_only?6u:7u,cancel_only?L"xr_stopped_before_write":L"continuous_prewrite_authorization_rejected");return;
    }
#else
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    const auto authorize_context=xr_context(check,trial.xr_ready);
    const unsigned mask=authorize_current_xr_plan(plan,authorize_context,GetTickCount64());
#else
    const unsigned mask=authorize_current_xr_plan(plan,xr_context(check,trial.xr_ready),GetTickCount64());
#endif
    if(!mask)return;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if((mask&~3u)||(continuous_tick.mode==component_continuous::Mode::joint&&!(mask&2u))){
        end(7,L"persistent_joint_right_not_authorized");return;
    }
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
            ,continuous_tick.mode
#endif
        );
    }
    // fflush may block. Recheck the receipt and game sample after all logging,
    // immediately before the first journal snapshot or mutation.
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(!xr_equipment_gate(check,equipment_phase,L"after_log_flush",EquipmentTiming::postflush_phase)){
        if(component_continuous::pause_pending())continuous_tick.suspending=true;
        return;
    }
#else
    if(!xr_equipment_gate(check,equipment_phase,L"after_log_flush",EquipmentTiming::postflush_phase))return;
#endif
    const auto final_context=xr_context(check,trial.xr_ready);
#endif
    const auto write_now=GetTickCount64();
    if(stop_requested.load(std::memory_order_acquire)){end(6,L"xr_stopped_before_write");return;}
#if !defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    if(write_now>trial.deadline){end(7,L"xr_deadline_before_write");return;}
#endif
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    if(ferror(log_file)||trial.tick_epoch!=trial.epoch||
       tick_boundary_diagnostic::final_authorize(tick_diagnostic,[&]{
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
           return authorize_current_xr_plan(plan,final_context,write_now);
#else
           return authorize_current_xr_plan(plan,xr_context(check,trial.xr_ready),write_now);
#endif
       })!=mask){
        tick_diagnostic.terminal(1); // This call site; an OR-short-circuited auth remains unknown.
        end(7,L"xr_receipt_or_log_expired_before_write");return;
    }
#else
    if(ferror(log_file)||trial.tick_epoch!=trial.epoch||
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
       authorize_current_xr_plan(plan,final_context,write_now)!=mask){
#else
       authorize_current_xr_plan(plan,xr_context(check,trial.xr_ready),write_now)!=mask){
#endif
        end(7,L"xr_receipt_or_log_expired_before_write");return;
    }
#endif
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL) && defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    component_names::Scope restore_prefix_names_scope(prefix_names_scope.saved);
#endif
    safe_to_unload.store(false,std::memory_order_release);
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(continuous_tick.mode==component_continuous::Mode::joint)
#endif
    if(!component_apply(plan,frame,check,mask,lease)){
        drain_journals(L"component_acquire_failed");
        end(journals_dirty()?8:7,L"component_acquire_failed");return;
    }
    // The component setter may dispatch engine work. Reacquire the hand sample
    // and certify this exact Plan after the entire component transaction.
    Snapshot after_component{};
    bool after_component_valid{};
    {
        component_tail_cost::Attempt timing(plan.ticket.sequence,plan.revision,lease,equipment_phase,mask);
        component_names::Scope names_scope(equipment_phase==1&&(mask&2));
        after_component_valid=
            component_tail_cost::test(component_tail_cost::Stage::snapshot,[&]{return handik::snapshot(pc,after_component);})&&
            component_tail_cost::test(component_tail_cost::Stage::guards,[&]{
                return !(!xr_identity(after_component)||!after_component.action_idle||!handik::baseline_strengths(after_component)||
                    !handik::same_controls(check,after_component)||!slots_quiet(after_component)||
                    trial.tick_epoch!=trial.epoch||
                    !xr_equipment_gate(after_component,equipment_phase,L"after_component",EquipmentTiming::prewrite_phase));
            })&&
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
            (continuous_tick.mode!=component_continuous::Mode::joint||
#else
            (!(equipment_phase==1&&(mask&2))||
#endif
                component_tail_cost::test(component_tail_cost::Stage::owned,[&]{return component_owned(true);}))&&
            component_tail_cost::test(component_tail_cost::Stage::authorize,[&]{
                return authorize_current_xr_plan(plan,xr_context(after_component,trial.xr_ready),GetTickCount64())==mask;
            });
    }
    if(!after_component_valid){
        drain_journals(L"hand_reauthorize_after_component");
        end(journals_dirty()?8:7,L"hand_reauthorize_after_component");return;
    }
    check=after_component;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(!continuous_tick.forward_open()||trial.ended||trial.tick_epoch!=trial.epoch||
#else
    if(!component_forward_open()||trial.ended||trial.tick_epoch!=trial.epoch||
#endif
       trial.lease.dirty()||trial.lease.faulted()||stop_requested.load(std::memory_order_acquire)||
       shutdown_requested.load(std::memory_order_acquire)){
        end(journals_dirty()?8u:7u,L"continuous_closed_before_hand_apply");return;
    }
    const auto hand_setters_before=trial.setter_calls,hand_writes_before=trial.target_writes;
    const auto hand_targets=trial.targets;
#endif
    trial.lease_anchor=check;Backend backend{};
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    backend.forward_write=true;backend.forward_epoch=trial.epoch;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    backend.forward_mode=continuous_tick.mode;
    if(continuous_tick.mode==component_continuous::Mode::hand_only){
        if(component_continuous::state.hand_only_journal){end(8,L"persistent_hand_journal_already_pending");return;}
        // Set before acquire so failed/partial acquisitions have the same cleanup duty.
        component_continuous::state.hand_only_journal=true;
        component_continuous::state.hand_only_started=true;
    }
#endif
#endif
    const unsigned channels=((mask&1)?12u:0u)|((mask&2)?3u:0u);
    bool acquired{};
    {
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        EquipmentTimingScope timing(EquipmentTiming::lease_acquire);
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        component_continuous::Operation operation;
#endif
        acquired=trial.lease.acquire(channels,trial.targets,1.f,backend);
    }
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(!continuous_tick.forward_open()||trial.ended){
#else
    if(!component_forward_open()||trial.ended){
#endif
        if(!trial.ended)end(7,L"continuous_closed_during_hand_apply");return;
    }
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    const bool written_hand_valid=continuous_tick.mode==component_continuous::Mode::hand_only?
        component_after_hand_only(check,written,backend.forward_epoch,mask,lease,hand_setters_before,hand_writes_before,hand_targets):
        component_continuous::state.begun&&component_after_hand(check,written,mask,lease,hand_setters_before,hand_writes_before,hand_targets);
    if(!written_hand_valid){
#else
    if(component_continuous::state.begun&&
       !component_after_hand(check,written,mask,lease,hand_setters_before,hand_writes_before,hand_targets)){
#endif
        drain_journals(L"continuous_written_joint_journal_changed");
        end(journals_dirty()?8u:7u,L"continuous_written_joint_journal_changed");return;
    }
#endif
    sample(L"written",written,lease,mask);
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(continuous_tick.mode==component_continuous::Mode::joint)
#endif
    component_log_written();
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(!continuous_tick.forward_open()||trial.ended){end(7,L"continuous_closed_during_written_evidence");return;}
    if(continuous_tick.mode==component_continuous::Mode::hand_only){
        count_add(component_continuous::state.hand_only_commits);continuous_tick.hand_committed=true;
    }
    else continuous_tick.committed=true;
#else
    if(!component_forward_open()||trial.ended){end(7,L"continuous_closed_during_written_evidence");return;}
    continuous_tick.committed=true;
#endif
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
    component_hold::arm(plan,mask,written);
#endif
#endif
}
