// Included inside capture.cpp's anonymous namespace, after Scope/safe_copy.
// Observation only: original VM/Canvas arguments and draw counts are preserved.
bool ui_observer_enabled{},ui_reserved{},ui_hud_plane_enabled{};
gears_ui_hud::Parameters ui_hud_parameters{};
gears_ui_layout::Parameters ui_hud_layout_parameters{};
struct UiLayoutCounts {std::atomic<uint64_t> considered{},planned{},fallback{},applied{};} ui_layout_counts;
gears_ui::Config ui_config;
#include "crosshair_assignment_live.inl"
#include "ui_runtime_binding.h"
#include "ui_canvas_tile_batch.h"
std::mutex ui_binding_mutex;
gears_ui_binding::Receipts ui_binding_receipts;
std::array<std::atomic<uint64_t>,4> ui_binding_claims{};
std::atomic<uint64_t> ui_binding_acquired{},ui_binding_rejected{},ui_binding_end_rejected{};
std::atomic<uint64_t> ui_binding_reassociated{};
std::atomic<uint64_t> ui_binding_timestamp_races{},ui_binding_nested_reused{};
unsigned ui_semantic_consumer_pending{}; // Existing frame_hook_mutex only.
bool ui_capture_binding(gears_ui_binding::Binding& binding){
    if(anchor_observer_enabled){gears_anchor::GuardDiagnostic diagnostic{};
        const bool valid=gears_ui_binding::acquire(ui_config,anchor_config,safe_copy,binding,&diagnostic);
        if(!valid&&diagnostic.timestamp_only)++ui_binding_timestamp_races;return valid;}
    // Legacy configurations without anchor keep their original fixed binding.
    gears_ui::Reader<decltype(&safe_copy)> read{safe_copy};
    if(!gears_ui::quick_environment(ui_config,read))return false;
    binding={ui_config.identities[ui_config.role_pawn],ui_config.identities[ui_config.role_hud],
        ui_config.identities[ui_config.role_inventory],ui_config.identities[ui_config.role_controller].address,
        ui_config.identities[ui_config.role_camera].address,0};return true;
}
void ui_binding_begin(uint64_t token,gears_ui_binding::Binding const& binding){
    std::lock_guard<std::mutex> lock(ui_binding_mutex);ui_binding_receipts.begin(token,binding);
}
void ui_binding_finish(uint64_t token,bool valid){
    std::lock_guard<std::mutex> lock(ui_binding_mutex);ui_binding_receipts.finish(token,valid);
}
bool ui_binding_current(uint64_t token,gears_ui_binding::Binding const& current){
    std::lock_guard<std::mutex> lock(ui_binding_mutex);const auto status=ui_binding_receipts.check(token,current);
    ++ui_binding_claims[unsigned(status)];return status==gears_ui_binding::ReceiptStatus::current;
}
struct UiSemanticScope {
    gears_ui_binding::Binding binding{};bool entered{},finished{};
    void begin(gears_ui_binding::Binding const& b){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        binding=b;entered=true;++ui_semantic_consumer_pending;}
    bool end(bool complete){if(!entered||finished)return complete;
        gears_ui_binding::Binding after{};const bool valid=complete&&ui_capture_binding(after)&&gears_ui_binding::same(binding,after);
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);--ui_semantic_consumer_pending;finished=true;
        // A semantic binding change can invalidate HUD placement for this
        // Execute, but it does not invalidate the independently observed GPU
        // command-list and owned-output proof.
        if(!valid)++ui_binding_end_rejected;return valid;
    }
    ~UiSemanticScope(){if(entered&&!finished)end(false);}
};
using UiExec=void(__fastcall*)(void*,void*,void*);
std::array<UiExec,4> ui_original{};
std::array<void*,4> ui_targets{};
struct UiInvocation {gears_ui::ScopeContext context{};uintptr_t frame{};uint64_t sequence{};long long entered{};
    gears_ui::Config const* frozen{};gears_ui_binding::Binding binding{};};
thread_local UiInvocation ui_invocation;
struct UiInvocationRestore {
    UiInvocation saved=ui_invocation;
    ~UiInvocationRestore(){ui_invocation=saved;}
};
gears_ui::Config const& ui_invocation_config(){return ui_invocation.frozen?*ui_invocation.frozen:ui_config;}
bool ui_invocation_current(){
    if(!ui_invocation.frozen)return false;const auto& c=*ui_invocation.frozen;
    gears_ui::Reader<decltype(&safe_copy)> r{safe_copy};gears_ui::Registry objects{},names{};
    if(!gears_ui::quick_environment(c,r)||!gears_ui::registry(r,c.objects_header,objects)||!gears_ui::registry(r,c.names_header,names)||
       !gears_ui::fresh_identity(c,r,objects,names,c.identities[c.role_inventory])||
       !gears_ui::fresh_identity(c,r,objects,names,c.identities[c.role_camera]))return false;
    uintptr_t target{};return !anchor_observer_enabled||(r.field(target,c.identities[c.role_camera].address,anchor_config.camera_view_target)&&
        target==(c.mounted?c.identities[c.role_controlled].address:ui_invocation.binding.pawn.address));
}
#include "ui_logical_group_observer_live.inl"
#include "ui_call_boundary_live.inl"
#include "ui_scene_root_live.inl"
struct UiRow {
    gears_ui::CanvasSample sample{};
    uint64_t invocation{},calls{},latest_built_ticket{};
    uintptr_t root_frame{},matched_frame{};unsigned matched_depth{};
    long long scope_qpc{},qpc{};unsigned thread{},draw{};
};
std::array<std::array<UiRow,48>,9> ui_rows{};
std::array<unsigned,9> ui_row_counts{};
std::array<long long,9> ui_next_sample{};
std::array<std::atomic<uint64_t>,9> ui_calls{},ui_rejected{},ui_dropped{};
std::array<std::atomic<uint64_t>,8> ui_scopes{},ui_scope_rejected{};
std::array<std::atomic<uint64_t>,8> ui_canvas_scopes{};
std::atomic<uint64_t> ui_chain_calls{},ui_chain_rejected{};
struct UiChainReject {uintptr_t root_frame{},frame{},node{},object{},code{},previous{};long long qpc{};unsigned draw{};};
UiChainReject ui_first_chain_reject;
struct UiScopeReject {uintptr_t self{},frame{},object{},code{};long long qpc{};unsigned thread{};};
std::array<UiScopeReject,8> ui_first_scope_reject{};
std::atomic<uint64_t> ui_vm_calls{},ui_invocation_counter{};
std::mutex ui_rows_mutex;
#include "owned_ui_observer.inl"
#include "ui_payload_observer.inl"
#include "ui_text_observer.inl"
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
#include "ui_material_observer.inl"
#endif

// Caller closes world admission, drains readers, and holds frame/session locks.
bool ui_world_handoff_ready(gears_owned_ui_world::Guards const* owned_guards=nullptr,
    gears_owned_ui_world::Receipt const* owned_receipt=nullptr){
    if(ui_semantic_consumer_pending)return false;
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
    if(game_material::completion.blocks_present())return false;
#endif
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
    return owned_guards&&owned_receipt&&owned_guards->old_world==committed_world_generation.load()&&
        owned_ui_world_retirement_current(*owned_guards,*owned_receipt);
#else
    (void)owned_guards;(void)owned_receipt;return true;
#endif
}
bool retire_ui_world(gears_owned_ui_world::Guards const* owned_guards=nullptr,
    gears_owned_ui_world::Receipt const* owned_receipt=nullptr){
    if(!ui_world_handoff_ready(owned_guards,owned_receipt))return false;
    {std::lock_guard<std::mutex> lock(ui_binding_mutex);ui_binding_receipts.retire_all();}
    {std::lock_guard<std::mutex> lock(ui_payload_mutex);ui_payload_store.retire_all();}
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
    {std::lock_guard<std::mutex> lock(ui_material_mutex);
        for(auto& entry:ui_material_entries)if(entry.occupied)entry.poisoned=true;}
#endif
    return true;
}
// Only on the owning thread's next outer Scope, after old readers drained.
void reset_ui_world_tls(){
    ui_call_boundary::reset_tls();
    ui_logical_group_live::reset_tls();
    ui_scene_root_live::reset_tls();
    ui_invocation={};ui_payload_context=nullptr;ui_index_context=nullptr;ui_execute_entry_return=0;
    ui_text_collect_depth=0;
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
    ui_material_context=nullptr;
#endif
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
    game_material::current=nullptr;game_material::uploading=nullptr;
#endif
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
    owned_ui_live::family={};owned_ui_live::flushing={};
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
    owned_ui_live::route={};
#endif
#endif
#if defined(GEARS_OWNED_UI_MATERIAL) && GEARS_OWNED_UI_MATERIAL
    owned_material::uploading=nullptr;
#endif
}

void observe_ui_canvas(unsigned draw,void* canvas,void* frame,gears_ui::ScopeContext* selected_out=nullptr){
    if(!ui_observer_enabled||!recording.load()||!ui_invocation.context.kind){ui_logical_group_live::canvas_rejected();return;}
    if(draw<1||draw>3)return;
    ++ui_chain_calls;
    gears_ui::ScopeContext selected{};uintptr_t matched_frame{};unsigned matched_depth{};
    if(!ui_invocation_current()||!gears_ui::classify_canvas_scope(ui_invocation_config(),safe_copy,ui_invocation.context,
            ui_invocation.frame,reinterpret_cast<uintptr_t>(frame),selected,matched_frame,&matched_depth)){
        ui_logical_group_live::canvas_rejected();
        if(ui_chain_rejected.fetch_add(1)==0){auto& r=ui_first_chain_reject;
            r.root_frame=ui_invocation.frame;r.frame=reinterpret_cast<uintptr_t>(frame);r.qpc=now();r.draw=draw;
            if(frame){safe_copy(&r.node,static_cast<unsigned char*>(frame)+0x18,8);
                safe_copy(&r.object,static_cast<unsigned char*>(frame)+0x20,8);
                safe_copy(&r.code,static_cast<unsigned char*>(frame)+0x28,8);
                safe_copy(&r.previous,static_cast<unsigned char*>(frame)+0x38,8);}
        }return;
    }
    const auto kind=selected.kind;if(kind<1||kind>3||selected.scope_index>=ui_canvas_scopes.size()){
        ui_logical_group_live::canvas_rejected();return;}
    ui_call_boundary::canvas(draw,frame,selected,matched_frame);
    ui_logical_group_live::canvas(draw,canvas,frame,selected,matched_frame);
    if(selected_out)*selected_out=selected;
    ++ui_canvas_scopes[selected.scope_index];
    const unsigned bucket=(kind-1)*3+draw-1;
    const auto calls=++ui_calls[bucket];const auto timestamp=now();
    {
        std::lock_guard<std::mutex> lock(ui_rows_mutex);
        if(ui_row_counts[bucket]>=ui_rows[bucket].size()){++ui_dropped[bucket];return;}
        if(calls>12&&timestamp<ui_next_sample[bucket])return;
        ui_next_sample[bucket]=timestamp+qpc_frequency/2;
    }
    UiRow row{};
    if(!gears_ui::sample_canvas(ui_invocation_config(),safe_copy,selected,
            reinterpret_cast<uintptr_t>(canvas),reinterpret_cast<uintptr_t>(frame),row.sample)){
        ++ui_rejected[bucket];return;
    }
    row.invocation=ui_invocation.sequence;row.scope_qpc=ui_invocation.entered;
    row.root_frame=ui_invocation.frame;row.matched_frame=matched_frame;row.matched_depth=matched_depth;
    row.qpc=timestamp;row.thread=GetCurrentThreadId();row.draw=draw;row.calls=calls;
    // A temporal hint only. This does not associate a Canvas draw with a GPU ticket.
    row.latest_built_ticket=stereo_ticket_counter.load();
    std::lock_guard<std::mutex> lock(ui_rows_mutex);
    if(ui_row_counts[bucket]<ui_rows[bucket].size())ui_rows[bucket][ui_row_counts[bucket]++]=row;
    else ++ui_dropped[bucket];
}
void __fastcall hook_ui_vm(void* self,void* frame,void* result){
    Scope active_scope;if(!active_scope.world()){ui_original[0](self,frame,result);return;}
    crosshair_live::TickScope crosshair_tick(self,frame);
    if(ui_observer_enabled&&recording.load()){
        ++ui_vm_calls;uintptr_t node{};
        if(frame&&safe_copy(&node,static_cast<unsigned char*>(frame)+0x18,sizeof(node))){
            for(unsigned i=0;i<ui_config.scope_count;++i){
                if(node!=ui_config.identities[ui_config.scopes[i].function].address)continue;
                gears_ui::ScopeContext context{};
                gears_ui_binding::Binding binding{};
                const bool inherited=ui_invocation.frozen&&ui_invocation_current();
                if(inherited){binding=ui_invocation.binding;++ui_binding_nested_reused;}
                const bool bound=inherited||ui_capture_binding(binding);
                std::optional<gears_ui::Config> local_frozen;
                if(bound&&!inherited)local_frozen.emplace(gears_ui_binding::config(ui_config,binding));
                const auto& frozen=inherited?*ui_invocation.frozen:(local_frozen?*local_frozen:ui_config);
                if(bound&&gears_ui::match_scope(frozen,safe_copy,reinterpret_cast<uintptr_t>(self),reinterpret_cast<uintptr_t>(frame),context)){
                    ++ui_binding_acquired;
                    if(!gears_anchor::equal_identity(binding.pawn,ui_config.identities[ui_config.role_pawn]))++ui_binding_reassociated;
                    ++ui_scopes[i];UiInvocationRestore restore;
                    ui_invocation={context,reinterpret_cast<uintptr_t>(frame),++ui_invocation_counter,now(),&frozen,binding};
                    const auto token=ui_invocation.sequence;ui_binding_begin(token,binding);
                    struct Finish {uint64_t token;bool done{};~Finish(){if(!done)ui_binding_finish(token,false);}} finish{token};
                    ui_logical_group_live::Observe group(context.kind==3?1u:0u,token,binding.hud.address,
                        context.function,0,0,reinterpret_cast<uintptr_t>(frame),&frozen);
                    ui_original[0](self,frame,result);
                    gears_ui_binding::Binding after{};
                    const bool valid=inherited?ui_invocation_current():(ui_capture_binding(after)&&gears_ui_binding::same(binding,after));if(!valid)++ui_binding_end_rejected;
                    group.finish(valid);
                    ui_binding_finish(token,valid);finish.done=true;return;
                }
                ++ui_binding_rejected;
                if(ui_scope_rejected[i].fetch_add(1)==0){auto& row=ui_first_scope_reject[i];
                    row.self=reinterpret_cast<uintptr_t>(self);row.frame=reinterpret_cast<uintptr_t>(frame);row.qpc=now();row.thread=GetCurrentThreadId();
                    safe_copy(&row.object,static_cast<unsigned char*>(frame)+0x20,8);
                    safe_copy(&row.code,static_cast<unsigned char*>(frame)+0x28,8);
                }
                // A rejected recognized child must not be mislabeled as its parent HUD.
                UiInvocationRestore restore;ui_invocation={};ui_original[0](self,frame,result);return;
            }
        }
    }
    ui_original[0](self,frame,result);
}
#if defined(GEARS_UI_CALL_BOUNDARY_OBSERVER) && GEARS_UI_CALL_BOUNDARY_OBSERVER
#include "ui_canvas_tail_diagnostic.h"
namespace ui_tile_transaction {
struct Row {
    uint64_t root{};unsigned scope{},thread{};long long qpc{},exit_qpc{};
    bool sample_ok{},snapshot_attempted{},before_ok{},typed_transition{},current_checked{},current_ok{},append_ok{},published{},post_ok{},tail_attempted{},tail_ok{};
    gears_ui_batch::Snapshot before{},post{};
    gears_ui_batch_diagnostic::Tail tail{};
    uintptr_t receipt_payload{};unsigned receipt_begin{},receipt_end{};
};
std::array<std::array<Row,32>,2> rows{};std::array<unsigned,2> counts{};
std::array<uint64_t,2> dropped{};std::mutex mutex;
bool available(bool before_ok){std::lock_guard<std::mutex> lock(mutex);
    return counts[before_ok?1:0]<rows[0].size();}
void append(Row const& row){std::lock_guard<std::mutex> lock(mutex);const unsigned bucket=row.published?1:0;
    if(counts[bucket]<rows[bucket].size())rows[bucket][counts[bucket]++]=row;else ++dropped[bucket];}
void print(FILE* status){
    fprintf(status,"ui_tile_transaction observation_only=1 kind=1 failed_rows=%u successful_rows=%u failed_dropped=%llu successful_dropped=%llu capacity_per_bucket=32 tail_payload_never_read=1\n",
        counts[0],counts[1],dropped[0],dropped[1]);
    for(unsigned b=0;b<rows.size();++b)for(unsigned i=0;i<counts[b];++i){auto const& r=rows[b][i];auto const& t=r.tail;
        fprintf(status,"ui_tile_transaction_row bucket=%u row=%u root=%llu scope=%u thread=%u qpc=%lld exit_qpc=%lld sample_ok=%u snapshot_attempted=%u before_ok=%u typed_transition=%u current_checked=%u current_ok=%u append_ok=%u published=%u receipt_payload=%p receipt_begin=%u receipt_end=%u before_payload=%p before_count=%u before_empty=%u post_ok=%u post_payload=%p post_count=%u post_empty=%u tail_attempted=%u tail_ok=%u tail_canvas=%p tail_fcanvas=%p tail_key=%d tail_element=%p tail_item=%p tail_vtable=%p tail_empty=%u tail_items_count=%d\n",
            b,i,r.root,r.scope,r.thread,r.qpc,r.exit_qpc,r.sample_ok?1:0,r.snapshot_attempted?1:0,r.before_ok?1:0,r.typed_transition?1:0,r.current_checked?1:0,r.current_ok?1:0,r.append_ok?1:0,r.published?1:0,
            reinterpret_cast<void*>(r.receipt_payload),r.receipt_begin,r.receipt_end,reinterpret_cast<void*>(r.before.payload),r.before.vertex_count,r.before.empty?1:0,
            r.post_ok?1:0,reinterpret_cast<void*>(r.post.payload),r.post.vertex_count,r.post.empty?1:0,r.tail_attempted?1:0,r.tail_ok?1:0,reinterpret_cast<void*>(t.canvas),reinterpret_cast<void*>(t.fcanvas),t.sort_key,
            reinterpret_cast<void*>(t.element),reinterpret_cast<void*>(t.item),reinterpret_cast<void*>(t.vtable),t.empty?1:0,t.items.count);
    }
}
}
#endif
void __fastcall hook_ui_tile(void* self,void* frame,void* result){
    Scope active_scope;if(!active_scope.world()){ui_original[1](self,frame,result);return;}
    gears_ui::ScopeContext selected{};observe_ui_canvas(1,self,frame,&selected);
    ui_logical_group_live::CallMark group_call(frame,1,selected.kind!=0);
    gears_ui_tile_batch::Before before{};gears_ui::CanvasSample sample{};
    const bool eligible=selected.kind&&ui_observer_enabled&&recording.load()&&!ui_text_collect_depth;
    if(eligible)++ui_batch_attempts;
#if defined(GEARS_UI_CALL_BOUNDARY_OBSERVER) && GEARS_UI_CALL_BOUNDARY_OBSERVER
    const bool sample_ok=eligible&&gears_ui::sample_canvas(ui_invocation_config(),safe_copy,selected,
        reinterpret_cast<uintptr_t>(self),reinterpret_cast<uintptr_t>(frame),sample);
    const bool captured=sample_ok&&gears_ui_tile_batch::capture_before({base},safe_copy,reinterpret_cast<uintptr_t>(self),before);
    if(eligible&&!captured)++ui_batch_before_rejected;
    const bool diagnostic=eligible&&selected.kind==1&&
        (ui_tile_transaction::available(false)||ui_tile_transaction::available(true));
    ui_tile_transaction::Row transaction{};
    if(diagnostic){transaction.root=ui_invocation.sequence;transaction.scope=selected.scope_index;
        transaction.thread=GetCurrentThreadId();transaction.qpc=now();transaction.sample_ok=sample_ok;
        transaction.snapshot_attempted=sample_ok;transaction.before_ok=before.strict_valid;
        transaction.typed_transition=before.foreign_valid;transaction.before=before.strict;
        if(ui_tile_transaction::available(captured)){transaction.tail_attempted=true;
            transaction.tail_ok=gears_ui_batch_diagnostic::observe_tail(safe_copy,reinterpret_cast<uintptr_t>(self),transaction.tail);}}
    ui_original[1](self,frame,result);
    bool current_ok=false;
    if(captured&&recording.load()){
        current_ok=ui_invocation_current();
        if(diagnostic){transaction.current_checked=true;transaction.current_ok=current_ok;}
    }
    if(captured&&current_ok){
        gears_ui_batch::Receipt receipt{};
        if(gears_ui_tile_batch::appended_quad({base},safe_copy,before,reinterpret_cast<uintptr_t>(self),receipt)){
            group_call.tile(receipt);
            if(diagnostic){transaction.append_ok=true;transaction.receipt_payload=receipt.payload;
                transaction.receipt_begin=receipt.begin;transaction.receipt_end=receipt.end;}
            std::lock_guard<std::mutex> lock(ui_payload_mutex);
            if(ui_payload_store.publish(receipt,selected.kind,ui_invocation.sequence)){
                ++ui_batch_published;
                if(diagnostic)transaction.published=true;
            }
            else ++ui_batch_publish_rejected;
        }else ++ui_batch_append_rejected;
    }
    if(diagnostic){
        if(!transaction.current_checked){transaction.current_checked=true;transaction.current_ok=ui_invocation_current();}
        // This extra strict post snapshot is evidence only; publication above
        // requires the separate typed adapter's complete transition receipt.
        if(!transaction.before_ok)transaction.post_ok=gears_ui_batch::snapshot({base},safe_copy,reinterpret_cast<uintptr_t>(self),transaction.post);
        transaction.exit_qpc=now();ui_tile_transaction::append(transaction);
    }
#else
    const bool captured=eligible&&gears_ui::sample_canvas(ui_invocation_config(),safe_copy,selected,
        reinterpret_cast<uintptr_t>(self),reinterpret_cast<uintptr_t>(frame),sample)&&
        gears_ui_tile_batch::capture_before({base},safe_copy,reinterpret_cast<uintptr_t>(self),before);
    if(eligible&&!captured)++ui_batch_before_rejected;
    ui_original[1](self,frame,result);
    if(captured&&recording.load()&&ui_invocation_current()){
        gears_ui_batch::Receipt receipt{};
        if(gears_ui_tile_batch::appended_quad({base},safe_copy,before,reinterpret_cast<uintptr_t>(self),receipt)){
            group_call.tile(receipt);
            std::lock_guard<std::mutex> lock(ui_payload_mutex);
            if(ui_payload_store.publish(receipt,selected.kind,ui_invocation.sequence))++ui_batch_published;
            else ++ui_batch_publish_rejected;
        }else ++ui_batch_append_rejected;
    }
#endif
}
void __fastcall hook_ui_material(void* self,void* frame,void* result){
    Scope active_scope;if(!active_scope.world()){ui_original[2](self,frame,result);return;}
    gears_ui::ScopeContext selected{};observe_ui_canvas(2,self,frame,&selected);
    ui_logical_group_live::CallMark group_call(frame,2,selected.kind!=0);
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
    gears_ui_material::Before before{};
    const bool captured=ui_material_before(self,before);
#endif
    ui_original[2](self,frame,result);
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
    if(captured&&recording.load())ui_material_after(before,selected.kind&&ui_invocation_current()?selected.kind:0,ui_invocation.sequence);
    if(group_call.observing()&&captured&&selected.kind&&recording.load()&&ui_invocation_current()){
        gears_ui_material::Receipt observed{};
        if(gears_ui_material::snapshot_after({base},safe_copy,before,selected.kind,observed))group_call.material(observed);}
#endif
}
void __fastcall hook_ui_text(void* self,void* frame,void* result){
    Scope active_scope;if(!active_scope.world()){ui_original[3](self,frame,result);return;}
    gears_ui::ScopeContext selected{};observe_ui_canvas(3,self,frame,&selected);
    ui_logical_group_live::CallMark group_call(frame,3,selected.kind!=0);
    auto& counts=ui_text_counts[0];if(ui_observer_enabled&&recording.load())++counts.calls;
    const bool eligible=selected.kind&&ui_observer_enabled&&recording.load()&&!ui_text_collect_depth;
    if(!eligible){ui_original[3](self,frame,result);return;}
    ++counts.eligible;const auto invocation=ui_invocation.sequence;
    gears_ui_text_batch::Snapshot before{};gears_ui::CanvasSample sample{};
    const bool captured=gears_ui::sample_canvas(ui_invocation_config(),safe_copy,selected,
        reinterpret_cast<uintptr_t>(self),reinterpret_cast<uintptr_t>(frame),sample)&&
        gears_ui_text_batch::snapshot_canvas({base},safe_copy,reinterpret_cast<uintptr_t>(self),before);
    if(!captured)++counts.before_rejected;
    UiTextCollectScope collecting;
    ui_original[3](self,frame,result);
    if(captured&&recording.load()&&ui_invocation_current()){
        gears_ui_text_batch::Delta delta{};
        if(gears_ui_text_batch::delta_canvas({base},safe_copy,before,reinterpret_cast<uintptr_t>(self),delta)){
            group_call.text(delta);
            publish_ui_text_delta(delta,selected.kind,invocation,gears_ui_receipts::Source::CanvasText,0);
        }
        else ++counts.delta_rejected;
    }
}
const std::array<UiExec,4> ui_detours{hook_ui_vm,hook_ui_tile,hook_ui_material,hook_ui_text};

bool reserve_ui_hooks(){
    if(ui_reserved)return false;
    for(unsigned i=0;i<4;++i){
        ui_targets[i]=reinterpret_cast<void*>(base+gears_ui::binding_rvas[i]);
        std::array<unsigned char,16> before{},after{};
        if(!safe_copy(before.data(),ui_targets[i],before.size())||before!=gears_ui::binding_bytes[i]||
           MH_CreateHook(ui_targets[i],reinterpret_cast<void*>(ui_detours[i]),reinterpret_cast<void**>(&ui_original[i]))!=MH_OK||
           !safe_copy(after.data(),ui_targets[i],after.size())||after!=before)return false;
    }
    if(!reserve_ui_execute()||!reserve_ui_subtitle()||!ui_call_boundary::reserve()||!ui_scene_root_live::reserve())return false;
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
    if(!owned_ui_live::reserve())return false;
#endif
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
    if(!reserve_ui_material_execute())return false;
#endif
    ui_reserved=true;return true;
}

bool parse_ui_config(const wchar_t* ini,gears_ui::Config& output,uintptr_t controller,uintptr_t camera,bool attested_entries=false){
    auto load=[&](const wchar_t* key,auto& out){
        wchar_t text[64]{};GetPrivateProfileStringW(L"UIObserver",key,L"",text,64,ini);
        if(!*text||*text==L'-'||*text==L'+')return false;
        wchar_t* end{};auto value=_wcstoui64(text,&end,0);
        using T=std::remove_reference_t<decltype(out)>;
        if(!end||*end||value>static_cast<unsigned long long>((std::numeric_limits<T>::max)()))return false;
        out=static_cast<T>(value);return true;
    };
    unsigned schema{},pid{};wchar_t sha[66]{};
    GetPrivateProfileStringW(L"UIObserver",L"sha256",L"",sha,66,ini);
    if(!load(L"schema",schema)||(schema!=1&&schema!=2)||!load(L"pid",pid)||pid!=GetCurrentProcessId()||
       wcscmp(sha,L"bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4"))return false;
    const std::pair<const wchar_t*,unsigned> layout[]={{L"frame_node",0x18},{L"frame_object",0x20},
        {L"frame_code",0x28},{L"frame_locals",0x30},{L"frame_previous",0x38},{L"function_script",0x8c},{L"function_entry",0x114}};
    for(auto const& value:layout){unsigned actual{};if(!load(value.first,actual)||actual!=value.second)return false;}
    gears_ui::Config c{};
#define UI_WIDEN_(s) L##s
#define UI_WIDEN(s) UI_WIDEN_(s)
#define UI_LOAD(field) if(!load(UI_WIDEN(#field),c.field))return false;
    UI_LOAD(base) UI_LOAD(objects_header) UI_LOAD(names_header)
    UI_LOAD(object_internal_index) UI_LOAD(object_name) UI_LOAD(object_class) UI_LOAD(object_outer)
    UI_LOAD(field_super) UI_LOAD(name_entry_index) UI_LOAD(name_entry_text) UI_LOAD(property_offset_internal)
    UI_LOAD(identity_count) UI_LOAD(super_count) UI_LOAD(scope_count)
    UI_LOAD(role_controller) UI_LOAD(role_camera) UI_LOAD(role_pawn) UI_LOAD(role_hud) UI_LOAD(role_inventory) UI_LOAD(role_canvas_class)
    if(schema==2){c.mounted=true;UI_LOAD(role_controlled)
        UI_LOAD(controlled_driver) UI_LOAD(controlled_driver_property) UI_LOAD(pawn_driven_vehicle) UI_LOAD(pawn_driven_vehicle_property)
        UI_LOAD(controlled_controller) UI_LOAD(controlled_controller_property)}
#define UI_FIELD(field) UI_LOAD(field) UI_LOAD(field##_property)
    GEARS_UI_FIELDS(UI_FIELD)
#undef UI_FIELD
#undef UI_LOAD
#undef UI_WIDEN
#undef UI_WIDEN_
    if(c.base!=base||!c.identity_count||c.identity_count>c.identities.size()||c.super_count>c.supers.size()||
       !c.scope_count||c.scope_count>c.scopes.size())return false;
    for(unsigned i=0;i<c.identity_count;++i){auto& id=c.identities[i];wchar_t key[64]{};
        auto field=[&](const wchar_t* label,auto& value){swprintf_s(key,L"id%u_%s",i,label);return load(key,value);};
        if(!field(L"address",id.address)||!field(L"index",id.index)||!field(L"name_index",id.name_index)||
           !field(L"number",id.number)||!field(L"cls",id.cls)||!field(L"outer",id.outer))return false;
        swprintf_s(key,L"id%u_name",i);wchar_t name[65]{};
        auto n=GetPrivateProfileStringW(L"UIObserver",key,L"",name,65,ini);
        if(!n||n>=64)return false;std::copy(name,name+n+1,id.name.begin());
    }
    for(unsigned i=0;i<c.super_count;++i){wchar_t key[64]{};auto& value=c.supers[i];
        swprintf_s(key,L"super%u_identity",i);if(!load(key,value.identity)||value.identity>=c.identity_count)return false;
        swprintf_s(key,L"super%u_parent",i);if(!load(key,value.parent))return false;
        c.identities[value.identity].check_super=1;c.identities[value.identity].super=value.parent;
    }
    for(unsigned i=0;i<c.scope_count;++i){wchar_t key[64]{};auto& s=c.scopes[i];
        auto field=[&](const wchar_t* name,auto& value){swprintf_s(key,L"scope%u_%s",i,name);return load(key,value);};
        if(!field(L"kind",s.kind)||!field(L"owner",s.owner)||!field(L"function",s.function)||!field(L"script",s.script)||!field(L"size",s.size))return false;
    }
    const wchar_t* names[]={L"vm",L"tile",L"material",L"text"};
    for(unsigned i=0;i<4;++i){auto& b=c.bindings[i];wchar_t key[64]{},hex[34]{};
        swprintf_s(key,L"binding_%s_rva",names[i]);if(!load(key,b.rva)||b.rva!=gears_ui::binding_rvas[i])return false;
        if(i){swprintf_s(key,L"binding_%s_function",names[i]);if(!load(key,b.function))return false;}
        swprintf_s(key,L"binding_%s_bytes",names[i]);if(GetPrivateProfileStringW(L"UIObserver",key,L"",hex,34,ini)!=32)return false;
        for(unsigned j=0;j<16;++j){wchar_t expected[3]{};swprintf_s(expected,L"%02x",gears_ui::binding_bytes[i][j]);
            if(hex[j*2]!=expected[0]||hex[j*2+1]!=expected[1])return false;b.bytes[j]=gears_ui::binding_bytes[i][j];}
    }
    if(!(c.role_controller<c.identity_count&&c.role_camera<c.identity_count&&
        c.identities[c.role_controller].address==controller&&c.identities[c.role_camera].address==camera))return false;
    // Data/registry/property/script reads always remain actual memory. Only
    // these four explicit entry checks may use authenticated original bytes.
    if(!gears_ui::validate(c,safe_copy,!attested_entries))return false;
    if(attested_entries)for(auto const& binding:c.bindings){std::array<unsigned char,16> actual{};
        if(binding.rva>UINTPTR_MAX-c.base||!read_attested_hook_entry(actual.data(),
                reinterpret_cast<void const*>(c.base+binding.rva),actual.size())||actual!=binding.bytes)return false;
    }
    output=c;return true;
}
bool load_ui_config(const wchar_t* ini){
    gears_ui::Config candidate{};
    if(!parse_ui_config(ini,candidate,expected_pc,expected_camera))return false;
    ui_config=candidate;return true;
}
void print_ui_observer(FILE* status,bool disabled){
    fprintf(status,"ui_observer enabled=%u available=%u source_vertices_modified=0 hud_plane_enabled=%u vm_calls=%llu qpc_frequency=%lld\n",
        ui_observer_enabled?1:0,disabled&&!active.load()?1:0,ui_hud_plane_enabled?1:0,ui_vm_calls.load(),qpc_frequency);
    if(!ui_observer_enabled||!disabled||active.load())return;
    ui_call_boundary::print(status);
    ui_scene_root_live::print(status);
    ui_logical_group_live::print(status);
#if defined(GEARS_UI_CALL_BOUNDARY_OBSERVER) && GEARS_UI_CALL_BOUNDARY_OBSERVER
    ui_tile_transaction::print(status);
#endif
    fprintf(status,"ui_runtime_binding acquired=%llu rejected=%llu end_rejected=%llu claim_current=%llu claim_pending=%llu claim_retired=%llu claim_changed=%llu consumer_pending=%u timestamp_races=%llu nested_reused=%llu reassociated=%llu startup_config_immutable=1 same_level_only=1 queued_semantics_never_rebound=1\n",
        ui_binding_acquired.load(),ui_binding_rejected.load(),ui_binding_end_rejected.load(),ui_binding_claims[0].load(),ui_binding_claims[1].load(),
        ui_binding_claims[2].load(),ui_binding_claims[3].load(),ui_semantic_consumer_pending,ui_binding_timestamp_races.load(),ui_binding_nested_reused.load(),ui_binding_reassociated.load());
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
    owned_ui_live::print(status);
#endif
    print_ui_payload_observer(status);
    print_ui_text_observer(status);
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
    print_ui_material_observer(status);
#endif
    fprintf(status,"ui_canvas_chain calls=%llu rejected=%llu previous_offset=56 maximum_frames=%u root_identity_required=1\n",
        ui_chain_calls.load(),ui_chain_rejected.load(),gears_ui::canvas_frame_limit);
    if(ui_chain_rejected.load()){auto const& r=ui_first_chain_reject;
        fprintf(status,"ui_canvas_chain_first_reject root_frame=%p frame=%p node=%p object=%p code=%p previous=%p qpc=%lld draw=%u\n",
            reinterpret_cast<void*>(r.root_frame),reinterpret_cast<void*>(r.frame),reinterpret_cast<void*>(r.node),reinterpret_cast<void*>(r.object),
            reinterpret_cast<void*>(r.code),reinterpret_cast<void*>(r.previous),r.qpc,r.draw);}
    for(unsigned i=0;i<ui_config.scope_count;++i)fprintf(status,"ui_canvas_scope index=%u kind=%u calls=%llu calls_are_not_script_invocations=1\n",
        i,ui_config.scopes[i].kind,ui_canvas_scopes[i].load());
    for(unsigned i=0;i<ui_config.scope_count;++i)fprintf(status,"ui_scope index=%u kind=%u function=%p accepted=%llu rejected=%llu\n",
        i,ui_config.scopes[i].kind,reinterpret_cast<void*>(ui_config.identities[ui_config.scopes[i].function].address),ui_scopes[i].load(),ui_scope_rejected[i].load());
    for(unsigned i=0;i<ui_config.scope_count;++i)if(ui_scope_rejected[i].load()){auto const& r=ui_first_scope_reject[i];
        fprintf(status,"ui_scope_first_reject index=%u self=%p frame=%p object=%p code=%p qpc=%lld thread=%u\n",i,
            reinterpret_cast<void*>(r.self),reinterpret_cast<void*>(r.frame),reinterpret_cast<void*>(r.object),reinterpret_cast<void*>(r.code),r.qpc,r.thread);}
    for(unsigned i=0;i<9;++i){
        fprintf(status,"ui_canvas bucket=%u kind=%u draw=%u calls=%llu rejected=%llu rows=%u dropped=%llu\n",i,i/3+1,i%3+1,ui_calls[i].load(),ui_rejected[i].load(),ui_row_counts[i],ui_dropped[i].load());
        for(unsigned j=0;j<ui_row_counts[i];++j){auto const& r=ui_rows[i][j];auto const& s=r.sample;
            fprintf(status,"ui_canvas_row bucket=%u row=%u root_invocation=%llu scope=%u function=%p object=%p canvas=%p frame_node=%p frame_object=%p frame_code=%p controller=%p pawn=%p hud=%p inventory=%p hud_canvas=%p scope_qpc=%lld qpc=%lld thread=%u draw=%u calls=%llu latest_built_ticket_hint=%llu ticket_association_verified=0 clip_x=%.9g clip_y=%.9g cur_x=%.9g cur_y=%.9g org_x=%.9g org_y=%.9g size_x=%d size_y=%d root_frame=%p matched_frame=%p matched_depth=%u\n",
                i,j,r.invocation,s.scope.scope_index,reinterpret_cast<void*>(s.scope.function),reinterpret_cast<void*>(s.scope.object),reinterpret_cast<void*>(s.canvas),
                reinterpret_cast<void*>(s.frame_node),reinterpret_cast<void*>(s.frame_object),reinterpret_cast<void*>(s.frame_code),reinterpret_cast<void*>(s.controller),reinterpret_cast<void*>(s.pawn),
                reinterpret_cast<void*>(s.hud),reinterpret_cast<void*>(s.inventory),reinterpret_cast<void*>(s.hud_canvas),r.scope_qpc,r.qpc,r.thread,r.draw,r.calls,r.latest_built_ticket,
                s.clip_x,s.clip_y,s.cur_x,s.cur_y,s.org_x,s.org_y,s.size_x,s.size_y,
                reinterpret_cast<void*>(r.root_frame),reinterpret_cast<void*>(r.matched_frame),r.matched_depth);
        }
    }
}
