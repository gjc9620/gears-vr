// Private, default-OFF observation of the existing original Result address.
// No damage multiplier, gameplay clock, engine call or game-memory write.
namespace gripdamage {
constexpr unsigned capacity=64,depth_limit=8;
static_assert(sizeof(unsigned)==4);
struct Token {
    unsigned generation{},call{},row{},depth{};
    explicit operator bool()const noexcept{return call!=0;}
};
struct Row {
    Token token{};unsigned parent{};
    uptr receiver{},caller{},result{},callee{};
    gripdamage_metadata::Snapshot before{},after{};
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_CALL_CONTEXT_PROBE)
    gripdamage_context::Snapshot before_context{},after_context{};
#endif
    unsigned before_bits{},after_bits{};
    bool before_schema_known{},before_bytes_known{},original_returned{},closed{},
        after_bytes_known{},after_schema_known{},session_unchanged{},schema_unchanged{},stable_layout_candidate{},typed_candidate{},
        incomplete{},abandoned_after_callback_drain{};
};
struct State {
    uptr world{},pc{},pawn{};ULONGLONG deadline{};
    unsigned generation{},count{},depth{},filtered_unknown{},schema_unknown{},
        gap_count{},finish_code{};
    bool admitting{},pending{},dumped{},capacity_gap{},depth_gap{},pairing_gap{};
    std::array<unsigned,depth_limit> stack{};
    std::array<Row,capacity> rows{};
};
static_assert(std::is_trivially_copyable<State>::value);
State state{};
unsigned generation{};
void increment(unsigned& value) noexcept{if(value<UINT32_MAX)++value;}
void freeze(unsigned code) noexcept{
    state.admitting=false;state.pending=true;
    if(!state.finish_code)state.finish_code=code;
}
bool can_begin() noexcept{
    return !state.admitting&&!state.depth&&(!state.pending||state.dumped)&&generation<UINT32_MAX;
}
bool begin(uptr world,uptr pc,uptr pawn,ULONGLONG deadline) noexcept{
    if(!can_begin())return false;
    std::memset(&state,0,sizeof(state));
    state.generation=++generation;state.world=world;state.pc=pc;state.pawn=pawn;state.deadline=deadline;
    state.admitting=true;return true;
}
bool environment() noexcept{
    for(const auto label:{L"GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY",L"GEARSVR_TEST_WINDOWS_BEHIND",
                          L"GEARSVR_TEST_MUTE_GAME"}){
        wchar_t value[4]{};
        if(GetEnvironmentVariableW(label,value,4)!=1||value[0]!=L'1')return false;
    }
    return true;
}
bool bytes(uptr result,unsigned& value) noexcept{
    return result&&result<=UINTPTR_MAX-3&&copy(&value,result,4);
}
Token enter(uptr receiver,uptr caller,uptr result,uptr callee) noexcept{
    // Every access to the GT-only state is behind these two original gates.
    if(!enabled.load(std::memory_order_acquire)||
       GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return {};
    if(!state.admitting)return {};
    bool known{};
    if(!gripdamage_metadata::candidate(callee,known)){
        if(!known)increment(state.filtered_unknown);
        return {};
    }
    // This is only the existing observer deadline, not a gameplay/write clock.
    if(GetTickCount64()>state.deadline){freeze(3);return {};}
    if(!playground::automatic||!playground::ready||!environment()||
       playground::arena_world!=state.world||playground::arena_pc!=state.pc||
       playground::arena_pawn!=state.pawn){increment(state.gap_count);freeze(5);return {};}
    if(state.count>=capacity||state.depth>=depth_limit){
        state.capacity_gap=state.count>=capacity;state.depth_gap=state.depth>=depth_limit;
        increment(state.gap_count);freeze(6);return {};
    }
    const auto index=state.count++;
    auto& row=state.rows[index];
    row.token={state.generation,index+1,index,state.depth};
    row.parent=state.depth?state.stack[state.depth-1]:0;
    row.receiver=receiver;row.caller=caller;row.result=result;row.callee=callee;
    state.stack[state.depth++]=row.token.call;
    row.before_schema_known=gripdamage_metadata::read(receiver,callee,state.pawn,row.before);
    if(row.before_schema_known)row.before_bytes_known=bytes(result,row.before_bits);
    else increment(state.schema_unknown);
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_CALL_CONTEXT_PROBE)
    gripdamage_context::read(caller,callee,state.pawn,row.before_context);
#endif
    return row.token;
}
void leave(Token const& token,bool returned) noexcept{
    if(!token||GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return;
    // A recorded token must close even after Stop/limit froze admission.
    if(token.generation!=state.generation||token.row>=state.count||
       state.rows[token.row].token.call!=token.call||!state.depth||
       state.stack[state.depth-1]!=token.call||state.depth!=token.depth+1){
        state.pairing_gap=true;increment(state.gap_count);freeze(7);return;
    }
    auto& row=state.rows[token.row];row.original_returned=returned;
    if(returned){
        // Read the same original destination promptly. ReturnOffset is metadata
        // only; neither caller/FFrame nor RAX supplies these four bytes.
        if(row.before_schema_known)row.after_bytes_known=bytes(row.result,row.after_bits);
        row.session_unchanged=playground::automatic&&playground::ready&&environment()&&
            playground::arena_world==state.world&&playground::arena_pc==state.pc&&
            playground::arena_pawn==state.pawn;
        row.after_schema_known=gripdamage_metadata::read(row.receiver,row.callee,state.pawn,row.after);
        row.schema_unchanged=row.before_schema_known&&row.after_schema_known&&
            gripdamage_metadata::same(row.before,row.after);
        row.stable_layout_candidate=row.session_unchanged&&row.schema_unchanged&&row.after_bytes_known;
        // The complete native property type has not yet been bound by retail
        // evidence. Preserve arbitrary raw bits; never classify them as float.
        row.typed_candidate=false;
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_CALL_CONTEXT_PROBE)
        gripdamage_context::read(row.caller,row.callee,state.pawn,row.after_context);
#endif
    }else row.incomplete=true; // No fabricated post value/schema on unwind.
    row.closed=true;state.stack[--state.depth]=0;
}
void dump_metadata(gripdamage_metadata::Snapshot const&,unsigned,const wchar_t*);
void pump(){
    if(!state.pending||state.dumped||state.depth||state.admitting||!log_file)return;
    fwprintf(log_file,L"weapon_grip_damage_probe_begin schema=1 generation=%u world=%p pc=%p pawn=%p "
        L"capacity=%u depth_limit=%u result_bytes=4 pre_is_uninitialized_storage=1 "
        L"result_offset_applied=0 rax_interpreted=0 native_return_type_proved=0 authoritative_shot_proved=0 multiplier_applied=0 observer_writes=0\n",
        state.generation,reinterpret_cast<void*>(state.world),reinterpret_cast<void*>(state.pc),
        reinterpret_cast<void*>(state.pawn),capacity,depth_limit);
    for(unsigned i=0;i<state.count;++i){
        auto const& r=state.rows[i];
        fwprintf(log_file,L"weapon_grip_damage_result generation=%u call=%u parent=%u depth=%u receiver=%p caller=%p "
            L"result=%p callee=%p pre_schema_known=%u pre_bytes_known=%u pre_bits=%08x original_returned=%u "
            L"closed=%u post_bytes_known=%u post_bits=%08x post_schema_known=%u schema_unchanged=%u "
            L"session_unchanged=%u stable_layout_candidate=%u typed_candidate=%u incomplete=%u abandoned_after_callback_drain=%u\n",
            r.token.generation,r.token.call,r.parent,r.token.depth,reinterpret_cast<void*>(r.receiver),
            reinterpret_cast<void*>(r.caller),reinterpret_cast<void*>(r.result),reinterpret_cast<void*>(r.callee),
            unsigned(r.before_schema_known),unsigned(r.before_bytes_known),r.before_bits,unsigned(r.original_returned),
            unsigned(r.closed),unsigned(r.after_bytes_known),r.after_bits,unsigned(r.after_schema_known),
            unsigned(r.schema_unchanged),unsigned(r.session_unchanged),unsigned(r.stable_layout_candidate),unsigned(r.typed_candidate),unsigned(r.incomplete),
            unsigned(r.abandoned_after_callback_drain));
        dump_metadata(r.before,r.token.call,L"pre");
        if(r.original_returned)dump_metadata(r.after,r.token.call,L"post");
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_CALL_CONTEXT_PROBE)
        gripdamage_context::dump(r.before_context,state.generation,r.token.call,L"pre");
        if(r.original_returned)gripdamage_context::dump(r.after_context,state.generation,r.token.call,L"post");
#endif
    }
    fwprintf(log_file,L"weapon_grip_damage_probe_end generation=%u finish_code=%u rows=%u depth=%u "
        L"filtered_unknown=%u schema_unknown=%u gap_count=%u capacity_gap=%u depth_gap=%u pairing_gap=%u "
        L"observer_writes=0 actual_damage_acceptance=0\n",state.generation,state.finish_code,state.count,state.depth,
        state.filtered_unknown,state.schema_unknown,state.gap_count,unsigned(state.capacity_gap),
        unsigned(state.depth_gap),unsigned(state.pairing_gap));
    fflush(log_file);state.dumped=true;
}
// Only the lifecycle owner may call this after disabling hooks and observing
// the original global callback count at zero. It does not change that count.
void callbacks_drained(){
    if(state.admitting||state.pending)freeze(4);
    if(state.depth){
        for(unsigned i=0;i<state.count;++i)if(!state.rows[i].closed){
            state.rows[i].incomplete=true;state.rows[i].abandoned_after_callback_drain=true;
        }
        state.pairing_gap=true;increment(state.gap_count);state.depth=0;state.stack={};
    }
    pump();
}
bool reset_drained() noexcept{
    if(state.depth||state.admitting||(state.pending&&!state.dumped))return false;
    std::memset(&state,0,sizeof(state));return true;
}
void dump_property(gripdamage_metadata::Property const& p,unsigned call,const wchar_t* phase,
                   const wchar_t* role,unsigned ordinal){
    fwprintf(log_file,L"weapon_grip_damage_property generation=%u call=%u phase=%ls role=%ls ordinal=%u "
        L"identity=%u type=%u next=%p flags=%llx dimension=%u element_bytes=%u offset=%u\n",
        state.generation,call,phase,role,ordinal,p.identity,p.type,reinterpret_cast<void*>(p.next),
        static_cast<unsigned long long>(p.flags),p.dimension,p.element_bytes,p.offset);
}
void dump_metadata(gripdamage_metadata::Snapshot const& m,unsigned call,const wchar_t* phase){
    // The recorder has frozen and drained. Output uses only copied POD; names
    // use UTF-16 hex so unknown game text cannot inject diagnostic records.
    fwprintf(log_file,L"weapon_grip_damage_metadata generation=%u call=%u phase=%ls known=%u failure_stage=%u "
        L"failure_reason=%u failure_address=%p failure_ordinal=%u module=%p pawn=%p receiver=%p callee=%p "
        L"object_data=%p object_count=%d object_capacity=%d name_data=%p name_count=%d name_capacity=%d "
        L"identities=%u names=%u structures=%u fields=%u pawn_identity=%u receiver_identity=%u function_identity=%u "
        L"function_type=%u owner_identity=%u package_identity=%u receiver_classes=%u owner_kind=%u "
        L"return_field=%u function_extent=%u function_children=%p parameter_bytes=%u return_offset=%u\n",
        state.generation,call,phase,m.known,unsigned(m.failure.stage),unsigned(m.failure.reason),
        reinterpret_cast<void*>(m.failure.address),m.failure.ordinal,reinterpret_cast<void*>(m.module_base),
        reinterpret_cast<void*>(m.arena_pawn),reinterpret_cast<void*>(m.receiver),reinterpret_cast<void*>(m.callee),
        reinterpret_cast<void*>(m.objects.data),m.objects.count,m.objects.capacity,reinterpret_cast<void*>(m.names.data),
        m.names.count,m.names.capacity,m.identity_count,m.name_count,m.structure_count,m.field_count,m.pawn_identity,
        m.receiver_identity,m.function_identity,m.function_type,m.owner_identity,m.package_identity,m.receiver_class_count,
        m.owner_kind,m.return_field,m.function_extent,reinterpret_cast<void*>(m.function_children),
        unsigned(m.parameter_bytes),unsigned(m.return_offset));
    if(m.known!=1)return;
    for(unsigned i=0;i<(std::min)(m.identity_count,gripdamage_metadata::max_identities);++i){
        auto const& v=m.identities[i];
        fwprintf(log_file,L"weapon_grip_damage_identity generation=%u call=%u phase=%ls slot=%u address=%p "
            L"outer=%p class=%p index=%u name_index=%u name_number=%u name_slot=%u\n",state.generation,call,phase,i,
            reinterpret_cast<void*>(v.address),reinterpret_cast<void*>(v.outer),reinterpret_cast<void*>(v.klass),
            v.index,v.name.index,v.name.number,v.name_slot);
    }
    for(unsigned i=0;i<(std::min)(m.name_count,gripdamage_metadata::max_names);++i){
        auto const& v=m.name_text[i];
        fwprintf(log_file,L"weapon_grip_damage_name generation=%u call=%u phase=%ls slot=%u entry=%p index=%u "
            L"length=%u utf16=",state.generation,call,phase,i,reinterpret_cast<void*>(v.entry),v.index,v.length);
        for(unsigned j=0;j<(std::min)(v.length,gripdamage_metadata::max_name_units);++j)
            fwprintf(log_file,L"%04x",unsigned(v.units[j]));
        fwprintf(log_file,L"\n");
    }
    for(unsigned i=0;i<(std::min)(m.structure_count,gripdamage_metadata::max_structures);++i){
        auto const& v=m.structures[i];
        fwprintf(log_file,L"weapon_grip_damage_structure generation=%u call=%u phase=%ls slot=%u identity=%u "
            L"super=%p children=%p extent=%u\n",state.generation,call,phase,i,v.identity,
            reinterpret_cast<void*>(v.super),reinterpret_cast<void*>(v.children),v.extent);
    }
    for(unsigned i=0;i<(std::min)(m.field_count,gripdamage_metadata::max_fields);++i)
        dump_property(m.fields[i],call,phase,L"function",i);
    for(unsigned i=0;i<(std::min)(m.receiver_class_count,gripdamage_metadata::max_parents);++i)
        fwprintf(log_file,L"weapon_grip_damage_receiver_class generation=%u call=%u phase=%ls ordinal=%u identity=%u\n",
            state.generation,call,phase,i,m.receiver_classes[i]);
    const auto pointers={&m.pawn_weapon,&m.weapon_instigator};unsigned ordinal{};
    for(auto const* v:pointers){
        fwprintf(log_file,L"weapon_grip_damage_pointer generation=%u call=%u phase=%ls ordinal=%u owner=%u value=%p\n",
            state.generation,call,phase,ordinal,v->owner,reinterpret_cast<void*>(v->value));
        dump_property(v->property,call,phase,L"pointer",ordinal++);
    }
}
} // namespace gripdamage
