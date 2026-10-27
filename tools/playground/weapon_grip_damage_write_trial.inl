// Default-OFF, bounded retail instant-hit write trial. The input is an explicit
// test fixture frozen for one observer epoch, NOT a live grip/shot adapter.
// No engine calls, delayed pointers, Result address deduplication or rollback.
namespace gripdamage_write {
namespace metadata=gripdamage_metadata;
namespace context=gripdamage_context;
using U32=metadata::U32;
enum class Grip:U32 {unknown,single,two,excluded};
#if defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL)
static_assert(unsigned(Grip::unknown)==unsigned(gears_weapon_hand::Grip::unknown)&&
    unsigned(Grip::single)==unsigned(gears_weapon_hand::Grip::single)&&
    unsigned(Grip::two)==unsigned(gears_weapon_hand::Grip::two)&&unsigned(Grip::excluded)==unsigned(gears_weapon_hand::Grip::excluded));
#endif
enum class Outcome:U32 {pending,disarmed,pre_unknown,post_unknown,scope_changed,
    unknown_grip,excluded,dual_passthrough,invalid_value,unchanged,applied,
    compare_changed,write_fault,readback_unknown,unwind,expired,closed_by_fault};
struct Gate {
    bool known{};
    uptr caller_node{},local_property{},float_class{},locals{},result{},script{},exec{};
    U32 extent{},offset{},field_count{},script_size{},code_offset{};
};
struct Entry {
#if defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL)
    gears_weapon_shot::Packet packet{};
#endif
    gripdamage::Token token{};Grip grip{};Gate before{},after{};Outcome outcome{};
    U32 original_bits{},desired_bits{},observed_bits{},readback_bits{};
    bool closed{},attempted{},exchanged{},readback_known{};
};
struct State {
    U32 generation{};Grip grip{};bool armed{},faulted{};
    std::array<Entry,gripdamage::capacity> rows{};
};
static_assert(std::is_trivially_copyable<State>::value);
State state{};
bool environment_value(wchar_t const* key,wchar_t const* expected) noexcept {
    wchar_t value[32]{};const auto count=GetEnvironmentVariableW(key,value,32);
    return count>0&&count<32&&!wcscmp(value,expected);
}
void begin(unsigned value) noexcept {
    state={};state.generation=value;
    state.armed=gripdamage::environment()&&playground::automatic&&playground::ready&&
        environment_value(L"GEARSVR_TEST_GRIP_DAMAGE_WRITE",L"1");
#if !defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL)
    if(environment_value(L"GEARSVR_TEST_GRIP_DAMAGE_FIXTURE",L"single"))state.grip=Grip::single;
    else if(environment_value(L"GEARSVR_TEST_GRIP_DAMAGE_FIXTURE",L"two"))state.grip=Grip::two;
    else if(environment_value(L"GEARSVR_TEST_GRIP_DAMAGE_FIXTURE",L"excluded"))state.grip=Grip::excluded;
#endif
}
bool named(metadata::Reader const& reader,U32 slot,wchar_t const* literal) noexcept {
    auto const& s=reader.result;
    if(slot>=s.identity_count)return false;auto const& id=s.identities[slot];
    if(id.name.number||id.name_slot>=s.name_count)return false;
    auto const& text=s.name_text[id.name_slot];const auto length=wcslen(literal);
    return length==text.length&&length<metadata::max_name_units&&
        !wmemcmp(text.units.data(),literal,length+1);
}
bool path(metadata::Reader& reader,uptr address,
          std::initializer_list<wchar_t const*> components) noexcept {
    U32 first{};if(!reader.path(address,first))return false;
    for(auto const label:components){
        const auto slot=reader.locate(address);
        if(!named(reader,slot,label))return false;
        address=reader.result.identities[slot].outer;
    }
    return !address;
}
bool qualify(gripdamage::Row const& row,bool after,Gate& out) noexcept {
    out={};auto const& schema=after?row.after:row.before;
    auto const& c=after?row.after_context:row.before_context;
    if(schema.known!=1||c.known!=1||c.caller!=row.caller||c.callee!=row.callee||
       c.module_base!=base||c.pawn!=playground::arena_pawn||
       c.frame.object!=row.receiver||schema.receiver!=row.receiver||
       !c.frame.locals||(c.frame.locals&3)||(row.result&3)||
       c.code_offset!=(after?379u:322u)||c.frame.script.count!=1421||
       c.caller_function.flags!=0xc44103u||c.caller_function.native_index||
       c.callee_function.flags!=0x24102u||c.callee_function.native_index||
       base>UINTPTR_MAX-0x1d6ca0||c.caller_function.exec!=base+0x1d6ca0||
       c.callee_function.exec!=c.caller_function.exec)return false;
    // Full registered paths supplement the observer's leaf-name filters.
    auto workspace=schema;metadata::Reader reader{workspace,metadata::Stage::final_check};
    if(!path(reader,c.frame.node,{L"ProcessInstantHit_Internal",L"WarWeapon",L"WarfareGame"})||
       !path(reader,workspace.identities[workspace.function_identity].klass,{L"Function",L"Core"})||
       !path(reader,workspace.identities[reader.locate(c.frame.node)].klass,{L"Function",L"Core"})||
       !path(reader,c.identities[c.node_type].address,{L"Function",L"Core"})||
       !path(reader,row.callee,{L"GetFireModeDamage",L"WarWeapon",L"WarfareGame"})||
       !path(reader,workspace.identities[workspace.owner_identity].klass,{L"Class",L"Core"})||
       !path(reader,workspace.identities[workspace.receiver_identity].klass,{L"Weap_AssaultRifle",L"WarfareGame"})||
       workspace.return_field>=workspace.field_count)return false;
    auto const& return_field=workspace.fields[workspace.return_field];
    const auto float_class=workspace.identities[return_field.type].address;
    if(!path(reader,float_class,{L"FloatProperty",L"Core"}))return false;
    metadata::Structure caller{};if(!reader.structure(c.frame.node,caller)||!caller.extent||caller.extent>65536)return false;
    std::array<metadata::Property,metadata::max_fields> fields{};
    U32 count{},matches{};metadata::Property local{};
    for(auto current=caller.children;current;){
        if(count==fields.size())return false;
        for(U32 i=0;i<count;++i)if(workspace.identities[fields[i].identity].address==current)return false;
        auto& field=fields[count++];
        if(!reader.property(current,field)||workspace.identities[field.identity].outer!=c.frame.node||
           !metadata::property_fits(field,caller.extent))return false;
        if(named(reader,field.identity,L"DamageDealt")){++matches;local=field;}
        current=field.next;
    }
    if(matches!=1||local.flags||local.dimension!=1||local.element_bytes!=4||
       workspace.identities[local.type].address!=float_class||
       !path(reader,float_class,{L"FloatProperty",L"Core"})||
       c.frame.locals>UINTPTR_MAX-caller.extent||
       c.frame.locals+local.offset!=row.result)return false;
    for(U32 i=0;i<count;++i){
        auto const& f=fields[i];
        if(f.identity!=local.identity&&f.offset<local.offset+4&&
           local.offset<f.offset+f.element_bytes*f.dimension)return false;
        metadata::Property fresh{};
        if(!reader.property(workspace.identities[f.identity].address,fresh)||!metadata::same_property(f,fresh))return false;
    }
    // The immediate stack frame must still be live and unchanged within this
    // phase. Across the original call its code cursor is expected to advance.
    context::Frame fresh{};context::Function caller_function{},callee_function{};
    if(!reader.final_check()||!context::frame(row.caller,fresh)||
       !metadata::read_at(fresh.node,0x8c,fresh.script)||!context::same_frame(c.frame,fresh)||
       !context::function(fresh.node,caller_function)||!context::function(row.callee,callee_function)||
       !context::same_function(c.caller_function,caller_function)||
       !context::same_function(c.callee_function,callee_function))return false;
    out={true,c.frame.node,workspace.identities[local.identity].address,float_class,
        c.frame.locals,row.result,c.frame.script.data,c.callee_function.exec,
        caller.extent,local.offset,count,U32(c.frame.script.count),c.code_offset};
    return true;
}
bool same_gate(Gate const& a,Gate const& b) noexcept {
    return a.known&&b.known&&a.caller_node==b.caller_node&&a.local_property==b.local_property&&
        a.float_class==b.float_class&&a.locals==b.locals&&a.result==b.result&&a.script==b.script&&
        a.exec==b.exec&&a.extent==b.extent&&a.offset==b.offset&&a.field_count==b.field_count&&
        a.script_size==b.script_size; // Code is deliberately not compared here.
}
Entry* entry(gripdamage::Token const& token) noexcept {
    if(!token||token.generation!=state.generation||token.row>=state.rows.size())return nullptr;
    auto& row=state.rows[token.row];
    return row.token.generation==token.generation&&row.token.call==token.call?&row:nullptr;
}
void enter(gripdamage::Token const& token) noexcept {
    if(!token||token.generation!=state.generation||token.row>=state.rows.size())return;
    auto& e=state.rows[token.row];e={};e.token=token;e.grip=state.grip;
    if(!state.armed){e.outcome=Outcome::disarmed;return;}
    if(state.faulted){e.outcome=Outcome::closed_by_fault;return;}
#if defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL)
    auto const& row=gripdamage::state.rows[token.row];
    if(weaponshot::packet(row.receiver,row.caller,e.packet))e.grip=static_cast<Grip>(e.packet.grip.grip);
#endif
    if(!qualify(gripdamage::state.rows[token.row],false,e.before))e.outcome=Outcome::pre_unknown;
}
void abandon(gripdamage::Token const& token) noexcept {
    if(auto e=entry(token)){e->outcome=Outcome::unwind;e->closed=true;}
}
// Aligned four-byte compare/exchange is one guarded store attempt. Expected
// bytes must still be the normal original result. Never retry or roll back.
bool exchange(uptr address,U32 expected,U32 desired,U32& previous_bits,bool& exchanged) noexcept {
    LONG expected_long{},desired_long{};std::memcpy(&expected_long,&expected,4);std::memcpy(&desired_long,&desired,4);
    __try{
        const LONG previous=InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(address),desired_long,expected_long);
        std::memcpy(&previous_bits,&previous,4);exchanged=previous_bits==expected;return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void complete(gripdamage::Token const& token) noexcept {
    auto e=entry(token);if(!e||e->closed)return;e->closed=true;
    if(e->outcome!=Outcome::pending)return;
    if(state.faulted){e->outcome=Outcome::closed_by_fault;return;}
    auto const& row=gripdamage::state.rows[token.row];
    if(!enabled.load(std::memory_order_acquire)||GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire)||
       token.generation!=gripdamage::state.generation||!row.closed||!row.original_returned||
       !row.session_unchanged||!row.schema_unchanged||!row.after_bytes_known||
       !gripdamage::environment()||!playground::automatic||!playground::ready||
       playground::arena_world!=gripdamage::state.world||playground::arena_pc!=gripdamage::state.pc||
       playground::arena_pawn!=gripdamage::state.pawn){e->outcome=Outcome::scope_changed;return;}
    if(GetTickCount64()>gripdamage::state.deadline){e->outcome=Outcome::expired;return;}
    if(!qualify(row,true,e->after)||!same_gate(e->before,e->after)||
       row.before_context.frame.previous!=row.after_context.frame.previous||
       !metadata::same_table(row.before_context.objects,row.after_context.objects)||
       !metadata::same_table(row.before_context.names,row.after_context.names)){
        e->outcome=Outcome::post_unknown;return;
    }
    e->original_bits=row.after_bits;e->desired_bits=row.after_bits;
#if defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL)
    if(!weaponshot::current(e->packet,row.receiver,row.caller)){e->outcome=Outcome::scope_changed;return;}
#endif
    if(e->grip==Grip::unknown){e->outcome=Outcome::unknown_grip;return;}
    if(e->grip==Grip::excluded){e->outcome=Outcome::excluded;return;}
    if(e->grip==Grip::two){e->outcome=Outcome::dual_passthrough;return;}
    float original_damage{};std::memcpy(&original_damage,&e->original_bits,4);
    if(!std::isfinite(original_damage)||original_damage<0){e->outcome=Outcome::invalid_value;return;}
    const float scaled=original_damage*0.7f;std::memcpy(&e->desired_bits,&scaled,4);
    if(e->desired_bits==e->original_bits){e->outcome=Outcome::unchanged;return;}
#if defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL)
    if(!weaponshot::write_admitted(e->packet)){e->outcome=Outcome::scope_changed;return;}
#endif
    e->attempted=true;
    if(!exchange(row.result,e->original_bits,e->desired_bits,e->observed_bits,e->exchanged)){
        e->outcome=Outcome::write_fault;state.faulted=true;return;
    }
    if(!e->exchanged){e->outcome=Outcome::compare_changed;state.faulted=true;return;}
    e->readback_known=gripdamage::bytes(row.result,e->readback_bits);
    if(!e->readback_known||e->readback_bits!=e->desired_bits){
        e->outcome=Outcome::readback_unknown;state.faulted=true;return;
    }
    e->outcome=Outcome::applied;
}
void dump(){
#if defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL)
    fwprintf(log_file,L"weapon_grip_write_begin schema=2 generation=%u armed=%u "
        L"grip_source=native_shared_owner grip_scope=fire_invocation fixture_ignored=1 "
        L"shot_entry_runtime_verified=0 projectile_supported=0 default_enabled=0 faulted=%u\n",
        state.generation,unsigned(state.armed),unsigned(state.faulted));
#else
    fwprintf(log_file,L"weapon_grip_write_begin schema=1 generation=%u armed=%u fixture_grip=%u "
        L"grip_source=test_fixture fixture_scope=observer_epoch actual_grip_adapter=0 authoritative_shot_proved=0 "
        L"projectile_supported=0 default_enabled=0 faulted=%u\n",state.generation,unsigned(state.armed),unsigned(state.grip),unsigned(state.faulted));
#endif
    for(auto const& e:state.rows)if(e.token){
#if defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL)
        fwprintf(log_file,L"weapon_grip_write_shot generation=%u call=%u row=%u shot=%llu scope_call=%llu "
            L"root=%u mode=%u mode_function=%p script_hash=%llx owner_frame=%llu input_sequence=%llu "
            L"cookie=%llu native_started=%llu world_generation=%llu\n",
            e.token.generation,e.token.call,e.token.row,e.packet.shot,e.packet.call,unsigned(e.packet.root),unsigned(e.packet.mode),
            reinterpret_cast<void*>(e.packet.mode_function.address),e.packet.script_hash,e.packet.grip.key.frame,
            e.packet.grip.input_sequence,e.packet.grip.key.registration_cookie,e.packet.grip.key.native_started_qpc,
            e.packet.grip.key.world_generation);
#endif
        fwprintf(log_file,L"weapon_grip_write_result generation=%u call=%u row=%u grip=%u outcome=%u closed=%u "
            L"pre_qualified=%u post_qualified=%u node=%p local_property=%p float_class=%p locals=%p result=%p "
            L"extent=%u offset=%u pre_code=%u post_code=%u original_bits=%08x desired_bits=%08x "
            L"attempted=%u exchanged=%u observed_bits=%08x readback_known=%u readback_bits=%08x\n",
            e.token.generation,e.token.call,e.token.row,unsigned(e.grip),unsigned(e.outcome),unsigned(e.closed),
            unsigned(e.before.known),unsigned(e.after.known),reinterpret_cast<void*>(e.after.caller_node),
            reinterpret_cast<void*>(e.after.local_property),reinterpret_cast<void*>(e.after.float_class),
            reinterpret_cast<void*>(e.after.locals),reinterpret_cast<void*>(e.after.result),e.after.extent,e.after.offset,
            e.before.code_offset,e.after.code_offset,e.original_bits,e.desired_bits,unsigned(e.attempted),
            unsigned(e.exchanged),e.observed_bits,unsigned(e.readback_known),e.readback_bits);
    }
    fwprintf(log_file,L"weapon_grip_write_end generation=%u faulted=%u final_damage_acceptance=0\n",state.generation,unsigned(state.faulted));
}
bool reset_drained() noexcept {
    for(auto const& e:state.rows)if(e.token&&!e.closed)return false;
    state={};return true;
}
} // namespace gripdamage_write
