// Included after UiInvocation and ui_invocation_current. No geometry publication,
// parent token changes, or delayed frame consumers are permitted here.
#include "ui_call_boundary.h"
#include <new>
#include <type_traits>
namespace ui_call_boundary {
inline constexpr auto entry_bytes=gears_ui_call_boundary::entry_bytes;
#if defined(GEARS_UI_CALL_BOUNDARY_OBSERVER) && GEARS_UI_CALL_BOUNDARY_OBSERVER
using Exec=uintptr_t(__fastcall*)(void*,void*,void*,void*);
Exec original{};void* target{};bool reserved{};
thread_local gears_ui_call_boundary::Nesting nesting;
std::atomic<uint64_t> calls{},candidates{},rejected{},depth_rejected{},accepted{},completed{},rows_dropped{},canvas_rejected{};
std::array<std::atomic<uint64_t>,3> canvas_calls{};
struct Row {
    gears_ui_call_boundary::Child child{};uintptr_t result_address{};
    gears_ui_call_boundary::Frame before{},after{};long long enter{},exit{};
    unsigned thread{};bool before_valid{},after_valid{},returned{};
    std::array<uint64_t,3> draws{};
};
gears_ui_call_boundary::Samples<Row> samples;std::mutex mutex;
thread_local Row* current_row{};
bool selected_entry(gears_ui::Config const& c,unsigned index,uintptr_t self){
    if(index>=c.scope_count||index>=c.scopes.size())return false;
    auto const& s=c.scopes[index];
    if(s.kind!=1||s.function>=c.identity_count||s.owner>=c.identity_count)return false;
    gears_ui::Reader<decltype(&safe_copy)> read{safe_copy};gears_ui::Registry objects{},names{},script{};uintptr_t entry{};
    return gears_ui::registry(read,c.objects_header,objects)&&gears_ui::registry(read,c.names_header,names)&&
        gears_ui::fresh_identity(c,read,objects,names,c.identities[s.function])&&
        gears_ui::fresh_identity(c,read,objects,names,c.identities[s.owner])&&
        c.identities[s.function].outer==c.identities[s.owner].address&&
        read.field(script,c.identities[s.function].address,0x8c)&&script.data==s.script&&script.count==int(s.size)&&
        script.count>0&&script.count<=script.capacity&&script.capacity<=131072&&
        read.field(entry,c.identities[s.function].address,0x114)&&entry==c.base+gears_ui::binding_rvas[0]&&
        gears_ui::live_scope_object(c,read,index,self);
}
// /EHsc does not destroy automatic C++ objects when the game raises SEH.
// Only this trivially destructible storage crosses the outer __try. A non-null
// object pointer records a completed placement construction, including on
// branches that construct only a prefix of the observer scopes.
template<class T> struct HookSlot {alignas(T) unsigned char storage[sizeof(T)];T* object;};
struct HookState {
    HookSlot<Scope> scope;
    HookSlot<cover_call_live::CallScope> cover;
    HookSlot<ui_logical_group_live::ChildScope> logical;
    HookSlot<weapon_call_live::CallScope> weapon;
    HookSlot<gears_ui_call_boundary::Nest> nest;
    Row row;Row* saved_row;bool row_installed;
};
static_assert(std::is_trivially_destructible_v<HookState>&&std::is_trivially_copyable_v<HookState>);
template<class T> void destroy_slot(HookSlot<T>& slot){
    if(slot.object){auto* object=slot.object;slot.object=nullptr;object->~T();}
}
void destroy_hook_state(HookState& state){
    // Exact reverse of construction/publication. In particular, nested calls
    // restore the parent's row before its Nest and logical child are exposed.
    if(state.row_installed){current_row=state.saved_row;state.row_installed=false;}
    destroy_slot(state.nest);destroy_slot(state.weapon);destroy_slot(state.logical);destroy_slot(state.cover);destroy_slot(state.scope);
}
uintptr_t invoke_original(HookState& state,void* self,void* caller_frame,void* result,void* callee){
    // One trampoline call site for every branch. Its opaque RAX bits and the
    // four arguments are forwarded unchanged; no exception handler wraps it.
    const uintptr_t value=weapon_call_live::invoke(original,self,caller_frame,result,callee);
    if(state.cover.object)state.cover.object->returned(value);
    return value;
}
uintptr_t hook_body(HookState& state,void* self,void* caller_frame,void* result,void* callee){
    state.scope.object=::new(static_cast<void*>(state.scope.storage)) Scope;
    if(!state.scope.object->world()||!recording.load())return invoke_original(state,self,caller_frame,result,callee);
    state.cover.object=::new(static_cast<void*>(state.cover.storage)) cover_call_live::CallScope(self,caller_frame,result,callee);
    auto native=[&]{return invoke_original(state,self,caller_frame,result,callee);};
    if(!ui_observer_enabled)return native();
    state.logical.object=::new(static_cast<void*>(state.logical.storage)) ui_logical_group_live::ChildScope(self,caller_frame,callee,
        ui_logical_group_live::boundary_candidate()&&ui_invocation.frozen&&ui_invocation_current());
    state.weapon.object=::new(static_cast<void*>(state.weapon.storage)) weapon_call_live::CallScope(self,caller_frame,result,callee);
    auto invoke=[&]{const uintptr_t value=state.logical.object->invoke(native);
        state.weapon.object->returned(value);return value;};
    ++calls;state.nest.object=::new(static_cast<void*>(state.nest.storage)) gears_ui_call_boundary::Nest(nesting);
    auto& nest=*state.nest.object;
    // Pointer-only frozen candidate lookup precedes expensive parent validation.
    constexpr unsigned no_candidate=(std::numeric_limits<unsigned>::max)();
    auto const* config=ui_invocation.frozen;unsigned candidate=no_candidate;
    if(config&&config->scope_count<=config->scopes.size()&&config->identity_count<=config->identities.size())
        for(unsigned i=0;i<config->scope_count;++i){auto const& s=config->scopes[i];
            if(s.kind==1&&s.function<config->identity_count&&config->identities[s.function].address==reinterpret_cast<uintptr_t>(callee)){candidate=i;break;}}
    if(candidate==no_candidate)return invoke();
    ++candidates;
    // Mask an outer selected observer while a recognized child is rejected.
    nest.select(nullptr);
    if(!gears_ui_call_boundary::selected_depth(nesting)){++rejected;++depth_rejected;return invoke();}
    if(!ui_invocation.context.kind||!ui_invocation.sequence||!ui_invocation_current()||
       !selected_entry(*config,candidate,reinterpret_cast<uintptr_t>(self))){++rejected;return invoke();}
    auto& row=state.row;row.child={++accepted,ui_invocation.sequence,nest.saved?nest.saved->id:0,
        reinterpret_cast<uintptr_t>(self),reinterpret_cast<uintptr_t>(caller_frame),reinterpret_cast<uintptr_t>(callee),candidate,nesting.depth};
    row.result_address=reinterpret_cast<uintptr_t>(result);row.thread=GetCurrentThreadId();row.enter=now();
    row.before_valid=gears_ui_call_boundary::frame(safe_copy,row.child.caller,row.before);
    nest.select(&row.child);
    state.saved_row=current_row;state.row_installed=true;current_row=&row;
    // Return semantics remain unproven. Forward the original RAX bits unchanged.
    const uintptr_t value=invoke();
    row.exit=now();row.after_valid=gears_ui_call_boundary::frame(safe_copy,row.child.caller,row.after);row.returned=true;++completed;
    {std::lock_guard<std::mutex> lock(mutex);
        if(!samples.append(row,row.draws[0]||row.draws[1]||row.draws[2]))++rows_dropped;}
    return value;
}
uintptr_t __fastcall hook(void* self,void* caller_frame,void* result,void* callee){
    HookState state{};uintptr_t value{};
    __try {value=hook_body(state,self,caller_frame,result,callee);}
    __finally {destroy_hook_state(state);}
    return value;
}
void canvas(unsigned draw,void* frame,gears_ui::ScopeContext const& selected,uintptr_t matched_frame){
    auto const* child=nesting.current;if(!child||!current_row||draw<1||draw>3)return;
    if(child->parent_token!=ui_invocation.sequence||selected.kind!=1||selected.scope_index!=child->scope_index||
       selected.function!=child->callee||selected.object!=child->self||
       !gears_ui_call_boundary::body_ancestry(safe_copy,reinterpret_cast<uintptr_t>(frame),matched_frame,
           child->caller,child->callee,child->self)){++canvas_rejected;return;}
    ++canvas_calls[draw-1];++current_row->draws[draw-1];
}
bool reserve(){
    if(reserved||base>UINTPTR_MAX-gears_ui_call_boundary::rva)return false;
    target=reinterpret_cast<void*>(base+gears_ui_call_boundary::rva);
    std::array<unsigned char,32> before{},after{};
    if(!safe_copy(before.data(),target,before.size())||before!=gears_ui_call_boundary::entry_bytes||
       MH_CreateHook(target,reinterpret_cast<void*>(hook),reinterpret_cast<void**>(&original))!=MH_OK||
       !safe_copy(after.data(),target,after.size())||after!=before)return false;
    reserved=true;return true;
}
void reset_tls(){nesting={};current_row=nullptr;}
void print(FILE* status){
    fprintf(status,"ui_call_boundary compiled=1 observation_only=1 calls=%llu candidates=%llu rejected=%llu depth_rejected=%llu accepted=%llu completed=%llu rows=%u dropped=%llu canvas_rejected=%llu tile=%llu material=%llu text=%llu addresses_diagnostic_only=1 geometry_complete=0 parent_token_unchanged=1 return_semantics_unproven=1 maximum_frames=%u empty_rows=%u body_rows=%u empty_dropped=%llu body_dropped=%llu\n",
        calls.load(),candidates.load(),rejected.load(),depth_rejected.load(),accepted.load(),completed.load(),samples.counts[0]+samples.counts[1],rows_dropped.load(),canvas_rejected.load(),
        canvas_calls[0].load(),canvas_calls[1].load(),canvas_calls[2].load(),gears_ui_call_boundary::frame_limit,
        samples.counts[0],samples.counts[1],samples.dropped[0],samples.dropped[1]);
    for(unsigned bucket=0;bucket<2;++bucket)for(unsigned i=0;i<samples.counts[bucket];++i){auto const& r=samples.rows[bucket][i];auto const& c=r.child;
        fprintf(status,"ui_call_boundary_row row=%u child=%llu parent_child=%llu parent_token=%llu thread=%u depth=%u scope=%u self=%p caller_frame=%p result_address=%p callee=%p enter=%lld exit=%lld returned=%u before_valid=%u caller_node_before=%p caller_code_before=%p after_valid=%u caller_node_after=%p caller_code_after=%p tile=%llu material=%llu text=%llu\n",
            bucket*32+i,c.id,c.parent_child,c.parent_token,r.thread,c.depth,c.scope_index,reinterpret_cast<void*>(c.self),reinterpret_cast<void*>(c.caller),
            reinterpret_cast<void*>(r.result_address),reinterpret_cast<void*>(c.callee),r.enter,r.exit,r.returned?1:0,r.before_valid?1:0,
            reinterpret_cast<void*>(r.before.node),reinterpret_cast<void*>(r.before.code),r.after_valid?1:0,reinterpret_cast<void*>(r.after.node),
            reinterpret_cast<void*>(r.after.code),r.draws[0],r.draws[1],r.draws[2]);}
}
#else
bool reserve(){return true;}
void reset_tls(){}
void canvas(unsigned,void*,gears_ui::ScopeContext const&,uintptr_t){}
void print(FILE* status){fprintf(status,"ui_call_boundary compiled=0 observation_only=1\n");}
#endif
}
