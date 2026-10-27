// Default-OFF, immediate caller context for the existing raw Result observer.
// FFrame locals/previous are addresses only. No parameter decoding, engine call,
// clock, heap, result access, shot classification or game-memory write.
namespace gripdamage_context {
namespace metadata=gripdamage_metadata;
using U32=metadata::U32;
constexpr U32 max_identities=64,max_names=48,window_capacity=64;
enum class Failure:U32 {none,environment,frame_address,frame_read,metadata,type,
    script_bounds,window_read,capacity,changed};
struct Frame {
    uptr node{},object{},code{},locals{},previous{};
    metadata::Table script{};
};
struct Function {U32 flags{};std::uint16_t native_index{};uptr exec{};};
struct Snapshot {
    U32 known{};Failure failure{};uptr failure_address{},module_base{},caller{},callee{},pawn{};
    metadata::Failure metadata_failure{};
    metadata::Table objects{},names{};
    Frame frame{};Function caller_function{},callee_function{};
    U32 node_identity{},node_type{},object_identity{},object_type{},callee_identity{},callee_type{};
    U32 identity_count{},name_count{},code_offset{},window_length{};uptr window_address{};
    std::array<metadata::Identity,max_identities> identities{};
    std::array<metadata::NameText,max_names> name_text{};
    std::array<unsigned char,window_capacity> window{};
};
static_assert(std::is_trivially_copyable<Snapshot>::value);
static_assert(sizeof(Snapshot)<16*1024,"Keep immediate context below 16 KiB");
bool frame(uptr address,Frame& value) noexcept {
    value={};
    return metadata::read_at(address,0x18,value.node)&&metadata::read_at(address,0x20,value.object)&&
        metadata::read_at(address,0x28,value.code)&&metadata::read_at(address,0x30,value.locals)&&
        metadata::read_at(address,0x38,value.previous);
}
bool function(uptr address,Function& value) noexcept {
    value={};
    return metadata::read_at(address,0xf4,value.flags)&&metadata::read_at(address,0xf8,value.native_index)&&
        metadata::read_at(address,0x114,value.exec);
}
bool same_function(Function const& a,Function const& b) noexcept {
    return a.flags==b.flags&&a.native_index==b.native_index&&a.exec==b.exec;
}
bool same_frame(Frame const& a,Frame const& b) noexcept {
    return a.node==b.node&&a.object==b.object&&a.code==b.code&&a.locals==b.locals&&a.previous==b.previous&&
        metadata::same_table(a.script,b.script);
}
bool read(uptr caller,uptr callee,uptr pawn,Snapshot& out) noexcept {
    out={};metadata::Snapshot workspace{};Snapshot value{};
    workspace.module_base=base;workspace.arena_pawn=pawn;
    metadata::Reader reader{workspace,metadata::Stage::filter};
    const auto fail=[&](Failure why,uptr address) noexcept {
        out={};out.failure=why;out.failure_address=address;out.metadata_failure=workspace.failure;
        out.module_base=workspace.module_base;out.caller=caller;out.callee=callee;out.pawn=pawn;
        return false;
    };
    if(!pawn||pawn!=playground::arena_pawn||!metadata::tables(base,workspace.objects,workspace.names))
        return fail(Failure::environment,pawn);
    if(!caller||(caller&7)||caller>UINTPTR_MAX-0x40)return fail(Failure::frame_address,caller);
    if(!frame(caller,value.frame))return fail(Failure::frame_read,caller);
    if(!reader.path(value.frame.node,value.node_identity)||
       !reader.path(workspace.identities[value.node_identity].klass,value.node_type)||
       !reader.path(value.frame.object,value.object_identity)||
       !reader.path(workspace.identities[value.object_identity].klass,value.object_type)||
       !reader.path(callee,value.callee_identity)||
       !reader.path(workspace.identities[value.callee_identity].klass,value.callee_type))
        return fail(Failure::metadata,caller);
    if(!reader.named(value.node_type,L"Function")||!reader.named(value.callee_type,L"Function"))
        return fail(Failure::type,value.frame.node);
    if(!metadata::read_at(value.frame.node,0x8c,value.frame.script)||
       !function(value.frame.node,value.caller_function)||!function(callee,value.callee_function))
        return fail(Failure::frame_read,value.frame.node);
    auto const& script=value.frame.script;
    if(!script.data||script.count<=0||script.count>script.capacity||script.capacity>131072||
       uptr(script.count)>UINTPTR_MAX-script.data||value.frame.code<script.data||
       value.frame.code>script.data+uptr(script.count))return fail(Failure::script_bounds,value.frame.code);
    value.code_offset=static_cast<U32>(value.frame.code-script.data);
    value.window_address=value.code_offset>16?value.frame.code-16:script.data;
    value.window_length=static_cast<U32>((std::min)(uptr(window_capacity),script.data+uptr(script.count)-value.window_address));
    if(!value.window_length||!copy(value.window.data(),value.window_address,value.window_length))
        return fail(Failure::window_read,value.window_address);
    if(workspace.identity_count>max_identities||workspace.name_count>max_names)
        return fail(Failure::capacity,caller);
    const auto identity_count=workspace.identity_count,name_count=workspace.name_count;
    for(U32 i=0;i<identity_count;++i){
        U32 slot{};
        if(!reader.identity(workspace.identities[i].address,slot)||slot!=i)
            return fail(Failure::changed,workspace.identities[i].address);
    }
    Frame fresh{};Function caller_function{},callee_function{};
    metadata::Table objects{},names{};std::array<unsigned char,window_capacity> window{};
    if(!frame(caller,fresh)||!metadata::read_at(fresh.node,0x8c,fresh.script)||
       !function(fresh.node,caller_function)||!function(callee,callee_function)||
       !same_frame(value.frame,fresh)||!same_function(value.caller_function,caller_function)||
       !same_function(value.callee_function,callee_function)||
       !copy(window.data(),value.window_address,value.window_length)||window!=value.window||
       workspace.identity_count!=identity_count||workspace.name_count!=name_count||
       !metadata::tables(workspace.module_base,objects,names)||
       !metadata::same_table(workspace.objects,objects)||!metadata::same_table(workspace.names,names)||
       base!=workspace.module_base||playground::arena_pawn!=pawn)return fail(Failure::changed,caller);
    value.module_base=workspace.module_base;value.caller=caller;value.callee=callee;value.pawn=pawn;
    value.objects=workspace.objects;value.names=workspace.names;
    value.identity_count=identity_count;value.name_count=name_count;
    for(U32 i=0;i<identity_count;++i)value.identities[i]=workspace.identities[i];
    for(U32 i=0;i<name_count;++i)value.name_text[i]=workspace.name_text[i];
    value.known=1;out=value;return true;
}
void dump(Snapshot const& s,unsigned generation,unsigned call,const wchar_t* phase){
    fwprintf(log_file,L"weapon_grip_call_context generation=%u call=%u phase=%ls known=%u failure=%u "
        L"failure_address=%p metadata_stage=%u metadata_reason=%u metadata_address=%p metadata_ordinal=%u "
        L"module=%p pawn=%p caller=%p callee=%p identities=%u names=%u immediate_frame_only=1 "
        L"caller_locals_decoded=0 previous_frame_followed=0 native_return_type_proved=0 authoritative_shot_proved=0 observer_writes=0\n",
        generation,call,phase,s.known,unsigned(s.failure),reinterpret_cast<void*>(s.failure_address),
        unsigned(s.metadata_failure.stage),unsigned(s.metadata_failure.reason),
        reinterpret_cast<void*>(s.metadata_failure.address),s.metadata_failure.ordinal,
        reinterpret_cast<void*>(s.module_base),reinterpret_cast<void*>(s.pawn),reinterpret_cast<void*>(s.caller),
        reinterpret_cast<void*>(s.callee),s.identity_count,s.name_count);
    if(s.known!=1)return;
    fwprintf(log_file,L"weapon_grip_call_frame generation=%u call=%u phase=%ls node=%p object=%p code=%p "
        L"locals=%p previous=%p script=%p script_size=%d script_capacity=%d code_offset=%u "
        L"node_identity=%u node_type=%u object_identity=%u object_type=%u callee_identity=%u callee_type=%u "
        L"caller_flags=%x caller_native_index=%u caller_exec=%p callee_flags=%x callee_native_index=%u callee_exec=%p "
        L"window_address=%p window_length=%u window=",
        generation,call,phase,reinterpret_cast<void*>(s.frame.node),reinterpret_cast<void*>(s.frame.object),
        reinterpret_cast<void*>(s.frame.code),reinterpret_cast<void*>(s.frame.locals),reinterpret_cast<void*>(s.frame.previous),
        reinterpret_cast<void*>(s.frame.script.data),s.frame.script.count,s.frame.script.capacity,s.code_offset,
        s.node_identity,s.node_type,s.object_identity,s.object_type,s.callee_identity,s.callee_type,
        s.caller_function.flags,unsigned(s.caller_function.native_index),reinterpret_cast<void*>(s.caller_function.exec),
        s.callee_function.flags,unsigned(s.callee_function.native_index),reinterpret_cast<void*>(s.callee_function.exec),
        reinterpret_cast<void*>(s.window_address),s.window_length);
    for(U32 i=0;i<(std::min)(s.window_length,window_capacity);++i)fwprintf(log_file,L"%02x",unsigned(s.window[i]));
    fwprintf(log_file,L"\n");
    for(U32 i=0;i<(std::min)(s.identity_count,max_identities);++i){
        auto const& v=s.identities[i];
        fwprintf(log_file,L"weapon_grip_call_identity generation=%u call=%u phase=%ls slot=%u address=%p "
            L"outer=%p class=%p index=%u name_index=%u name_number=%u name_slot=%u\n",generation,call,phase,i,
            reinterpret_cast<void*>(v.address),reinterpret_cast<void*>(v.outer),reinterpret_cast<void*>(v.klass),
            v.index,v.name.index,v.name.number,v.name_slot);
    }
    for(U32 i=0;i<(std::min)(s.name_count,max_names);++i){
        auto const& v=s.name_text[i];
        fwprintf(log_file,L"weapon_grip_call_name generation=%u call=%u phase=%ls slot=%u entry=%p index=%u "
            L"length=%u utf16=",generation,call,phase,i,reinterpret_cast<void*>(v.entry),v.index,v.length);
        for(U32 j=0;j<(std::min)(v.length,metadata::max_name_units);++j)fwprintf(log_file,L"%04x",unsigned(v.units[j]));
        fwprintf(log_file,L"\n");
    }
}
} // namespace gripdamage_context
