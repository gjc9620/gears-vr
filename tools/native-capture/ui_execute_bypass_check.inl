// CPU-only actual-hook contract check, invoked in a dedicated DLL-loader process.
#include <vector>
namespace {
namespace ui_bypass_check {
constexpr uint32_t sentinel=0x76543210;
struct TestException {unsigned value;};
UiPayloadContext* parent_payload{};
UiIndexContext* parent_indices{};
void* expected_command{};
unsigned calls{},draws{},depth{},mode{};
bool good=true;
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
owned_ui_live::Route parent_route{};
bool same_route(){auto const& r=owned_ui_live::route;return r.ticket==parent_route.ticket&&
    r.receipt==parent_route.receipt&&r.resource==parent_route.resource&&
    r.source_width==parent_route.source_width&&r.source_height==parent_route.source_height;}
#endif
void STDMETHODCALLTYPE draw(ID3D12GraphicsCommandList* list,UINT indices,UINT instances,UINT start,INT vertex,UINT first){
    ++draws;good=good&&stereo_publish_bypass&&ui_payload_context==parent_payload&&ui_index_context==parent_indices&&
        list==reinterpret_cast<ID3D12GraphicsCommandList*>(0x1234)&&indices==3&&instances==1&&start==0&&vertex==0&&first==0;
}
uint32_t __fastcall execute(void* command){
    ++calls;good=good&&command==expected_command;
    if(!mode){
        // Non-bypass control proves the real observer path entered with an
        // admitted world scope. Its intentionally invalid header is rejected.
        good=good&&ui_observer_enabled&&recording.load()&&world_thread.admitted&&world_thread.depth&&active.load()&&
            !stereo_publish_bypass&&!ui_payload_context&&!ui_index_context&&!ui_execute_entry_return;
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
        good=good&&!owned_ui_live::route.ticket;
#endif
        return 48;
    }
    good=good&&ui_payload_context==parent_payload&&ui_index_context==parent_indices&&
        ui_execute_entry_return==0x19f2f3&&stereo_publish_bypass&&!world_thread.depth&&!active.load();
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
    good=good&&same_route();
#endif
    // Exercise the production indexed-draw bypass as part of native Execute.
    hook_draw_indexed(reinterpret_cast<ID3D12GraphicsCommandList*>(0x1234),3,1,0,0,0);
    if(mode==2&&!depth){
        ++depth;const auto previous=stereo_publish_bypass;
        {StereoPublishBypass nested;good=good&&hook_ui_execute(command)==sentinel&&stereo_publish_bypass==previous+1;}
        good=good&&stereo_publish_bypass==previous;--depth;
    }
    if(mode==3)throw TestException{0x1357};
    return sentinel;
}
std::vector<uint64_t> counters(){
    const auto s=ui_payload_store.stats();
    std::vector<uint64_t> values{
        active.load(),ui_batch_attempts.load(),ui_batch_before_rejected.load(),ui_batch_append_rejected.load(),
        ui_batch_published.load(),ui_batch_publish_rejected.load(),ui_execute_calls.load(),ui_execute_header_rejected.load(),
        ui_execute_claims.load(),ui_execute_content_rejected.load(),ui_execute_content_matches.load(),ui_execute_bad_return.load(),
        ui_gpu_context_calls.load(),ui_gpu_context_missing.load(),ui_index_capture_ok.load(),ui_index_capture_rejected.load(),
        ui_index_callback_ok.load(),ui_index_callback_rejected.load(),ui_index_complete.load(),ui_index_incomplete.load(),ui_execute_sequence.load(),
        ui_hud_counts.considered.load(),ui_hud_counts.missing_context.load(),ui_hud_counts.reticle_excluded.load(),
        ui_hud_counts.unknown_or_mixed.load(),ui_hud_counts.ticket_missing.load(),ui_hud_counts.projection_rejected.load(),
        ui_hud_counts.planned.load(),ui_hud_counts.applied.load(),ui_semantic_consumer_pending,
        frame_counts.draws.load(),frame_counts.filtered.load(),frame_counts.unknown.load(),frame_counts.missing_target.load(),
        frame_counts.missing_ticket.load(),frame_counts.executes.load(),frame_counts.presents.load(),
        s.publish_attempts,s.published,s.rejected,s.capacity_rejected,s.groups_poisoned,s.claimed,s.poison_claimed,
        s.retired,s.resets,s.unknown_vertices_seen,s.allocation_rejected,s.groups_full,s.active_groups,s.poisoned_groups,uint64_t(s.disabled)};
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
    const auto owned=owned_ui_live::ledger.snapshot();auto const& c=owned.counts;
    const uint64_t extra[]={owned_ui_live::read_rejected.load(),owned_ui_live::execute_unassociated.load(),
        owned_ui_live::route_commands.load(),owned_ui_live::route_draws.load(),owned_ui_live::route_faults.load(),
        owned_ui_live::route_plane_draws.load(),owned_ui_live::route_reticle_draws.load(),owned_ui_live::first_route_fault.load(),
        c.bound,c.flushes,c.produced,c.transferred,c.executed,c.completed,c.failed,c.closed,c.published,c.retired,
        c.consumed_before_completion,c.consumed_before_closure,c.retired_with_payload,c.retired_empty,
        c.rejected,c.binding_rejected,c.payload_rejected,c.capacity_rejected,c.stale_rejected,c.target_rejected,c.transfer_rejected,c.kind_rejected,
        owned.active_count,owned.retained_count,owned.retained_nonempty_count};
    values.insert(values.end(),std::begin(extra),std::end(extra));
    for(auto const& k:c.by_kind)for(auto value:{k.expected,k.produced,k.transferred,k.executed,k.completed,k.failed})values.push_back(value);
#endif
    return values;
}
}
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureUiBypassCheck(void*) {
    using namespace ui_bypass_check;
    // This export is a dedicated process self-test, never a live-session probe.
    if(recording.load()||active.load()||stereo_publish_bypass||ui_payload_context||ui_index_context)return 300;
    {Scope admitted;if(!admitted.world()||!world_thread.admitted||world_thread.depth!=1)return 301;}
    base=0;ui_observer_enabled=true;ui_hud_plane_enabled=true;recording=true;
    if(ui_hud_sequence_fault.load()||ui_hud_first_fault.published())return 302;
    const auto saved_execute=ui_original_execute;const auto saved_draw=original_draw_indexed;
    UiPayloadRestore tls_restore;
    struct Restore {UiExecute execute;DrawIndexedFn draw;
        ~Restore(){ui_original_execute=execute;original_draw_indexed=draw;recording=false;ui_observer_enabled=false;}
    } restore{saved_execute,saved_draw};
    ui_original_execute=execute;original_draw_indexed=draw;
    UiPayloadContext payload{};payload.command=0x3000;payload.payload=0x4000;payload.generation=1;
    payload.root_first=payload.root_last=777;payload.vertex_count=4;payload.receipts=1;
    payload.kind_mask=2;payload.kind_vertices[1]=4;
    auto indices=std::make_unique<UiIndexContext>();auto& snap=indices->snapshot;
    snap.payload=payload.payload;snap.generation=payload.generation;snap.vertex_count=4;snap.segment_count=1;snap.total_indices=3;
    auto& segment=snap.segments[0];segment.minimum=0;segment.maximum=3;segment.index_count=3;segment.known[1]=3;segment.kind_mask=2;segment.entry_mask=1;segment.expected={3,1,0,0,0};
    for(unsigned i=0;i<3;++i)snap.indices[i]=snap.global_vertices[i]=i;
    indices->execute_sequence=55;indices->cursor=gears_ui_indices::Cursor(snap);
    if(indices->cursor.poisoned()||indices->cursor.expected_count()!=1)return 303;
    parent_payload=&payload;parent_indices=indices.get();ui_payload_context=parent_payload;ui_index_context=parent_indices;
    ui_execute_entry_return=0x19f2f3;
    gears_ui_batch::Receipt candidate{};candidate.canvas=0x1000;candidate.fcanvas=0x2000;candidate.item=0x3000;
    candidate.payload=payload.payload;candidate.end=4;
    if(!ui_payload_store.publish(candidate,2,777))return 304;
    alignas(16) std::array<uintptr_t,6> command{0x13394f0,payload.payload,0,0,0x9000,0};
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
    const auto thread=GetCurrentThreadId();
    if(!owned_ui_live::ledger.bind(101,0x1000,0x9000,thread,1)||
        owned_ui_live::ledger.begin_flush(0x1000,0x9000,thread,0x2a6b9f,2)!=101||
        !owned_ui_live::ledger.expect(101,1))return 305;
    const auto receipt=owned_ui_live::ledger.produce(101,payload.payload,3);
    if(!receipt||!owned_ui_live::ledger.transferred(receipt,true)||!owned_ui_live::ledger.close(101,4))return 306;
    parent_route={101,receipt,0xa000,1920,1080};owned_ui_live::route=parent_route;
    if(owned_ui_live::ledger.ready(101))return 307;
#endif
    auto unchanged=[&]{
        return ui_payload_context==parent_payload&&ui_index_context==parent_indices&&ui_execute_entry_return==0x19f2f3&&
            payload.command==0x3000&&payload.payload==0x4000&&payload.generation==1&&payload.root_first==777&&payload.root_last==777&&
            payload.vertex_count==4&&payload.receipts==1&&payload.kind_mask==2&&payload.kind_vertices[1]==4&&!payload.unknown_vertices&&
            indices->execute_sequence==55&&indices->cursor.expected_count()==1&&!indices->cursor.observed()&&
            !indices->cursor.poisoned()&&!indices->cursor.completed()&&!ui_hud_sequence_fault.load()&&!ui_hud_first_fault.published()
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
            &&same_route()
#endif
            ;
    };
    expected_command=command.data();const auto before=counters();
    for(mode=1;mode<=3;++mode){
        bool caught=false;
        try {StereoPublishBypass bypass;
            if(hook_ui_execute(command.data())!=sentinel)return 308;
        }catch(TestException const& error){if(mode!=3||error.value!=0x1357)return 309;caught=true;}
        if((mode==3)!=caught||!good||stereo_publish_bypass||!unchanged()||counters()!=before)return 310+mode;
    }
    if(calls!=4||draws!=4)return 314;
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
    // Bypass did not claim/complete the pending owned receipt or manufacture
    // publication readiness. It remains incomplete and deliberately unready.
    const auto owned=owned_ui_live::ledger.snapshot();
    if(owned.counts.executed||owned.counts.completed||owned.counts.failed||owned_ui_live::ledger.ready(101))return 315;
#endif
    // Control is deliberately outside bypass with observer enabled and an open
    // gate. The native stub sees shadowed TLS and Scope; old-hook code therefore
    // cannot pass the bypass checks by silently skipping instrumentation.
    alignas(16) std::array<uintptr_t,6> invalid{};expected_command=invalid.data();mode=0;
    const auto calls_before=ui_execute_calls.load(),headers_before=ui_execute_header_rejected.load();
    if(hook_ui_execute(invalid.data())!=48||!good||!unchanged()||active.load()||world_thread.depth||
        ui_execute_calls.load()!=calls_before+1||ui_execute_header_rejected.load()!=headers_before+1)return 316;
    gears_ui_receipts::Claim claimed{};
    if(!ui_payload_store.claim(payload.payload,claimed)||claimed.generation!=payload.generation||claimed.count!=1||
       claimed.entries[0].root_invocation!=777||claimed.entries[0].receipt.payload!=payload.payload)return 317;
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
    // A HUD semantic cursor may reject one menu transition even when the
    // native Canvas Execute and its independently tracked receipt succeeded.
    // Such a mismatch must remain diagnostic and leave stereo admission open.
    const auto semantic_before=owned_ui_live::semantic_incomplete.load();
    const auto faults_before=owned_ui_live::route_faults.load();
    const auto executing=owned_ui_live::ledger.execute(payload.payload,0x9000,5);
    if(!executing)return 318;
    owned_ui_live::execute_end(executing,48,false);
    if(!owned_ui_live::ledger.ready(101)||owned_ui_live::semantic_incomplete.load()!=semantic_before+1||
       owned_ui_live::route_faults.load()!=faults_before)return 319;
    if(!owned_ui_live::ledger.bind(102,0x1000,0x9000,thread,6)||
       owned_ui_live::ledger.begin_flush(0x1000,0x9000,thread,0x2a6b9f,7)!=102||
       !owned_ui_live::ledger.expect(102,1))return 320;
    const auto failed_receipt=owned_ui_live::ledger.produce(102,0x5000,8);
    if(!failed_receipt||!owned_ui_live::ledger.transferred(failed_receipt,true)||
       !owned_ui_live::ledger.close(102,9))return 321;
    const auto failed_execution=owned_ui_live::ledger.execute(0x5000,0x9000,10);
    if(!failed_execution)return 322;
    owned_ui_live::execute_end(failed_execution,47,true);
    if(owned_ui_live::route_faults.load()!=faults_before+1||
       owned_ui_live::first_route_fault.load()!=6||owned_ui_live::ledger.ready(102))return 323;
#endif
    printf("UI Execute bypass actual hooks: open world gate, observer-on control, owning parent cursor/receipt/TLS, indexed draw, non48, nested, exception, counters/fault preserved; owned_batch=%u\n",
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
        1u
#else
        0u
#endif
    );
    return 0;
}
