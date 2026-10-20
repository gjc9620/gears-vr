// Candidate transport and opt-in HUD viewport control. Source vertices stay
// unchanged; control additionally requires certified native segment callbacks.
#include "ui_segment_sources.h"
#include "ui_hud_first_fault.h"
#include "ui_aim_locked_plan.h"
using UiExecute=uint32_t(__fastcall*)(void*);
UiExecute ui_original_execute{};void* ui_execute_target{};
uintptr_t ui_execute_observer_begin{},ui_execute_observer_end{};
thread_local uintptr_t ui_execute_entry_return{};
constexpr std::array<unsigned char,32> ui_execute_bytes{
    0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x68,0x18,0x56,0x57,0x41,0x57,0x48,
    0x83,0xec,0x50,0xc5,0xf8,0x29,0x70,0xd8,0x48,0x8b,0xd9,0x48,0x8b,0x49,0x28,0x41};
gears_ui_receipts::Store ui_payload_store;
std::mutex ui_payload_mutex;
std::atomic<uint64_t> ui_batch_attempts{},ui_batch_before_rejected{},ui_batch_append_rejected{},
    ui_batch_published{},ui_batch_publish_rejected{},ui_execute_calls{},ui_execute_header_rejected{},
    ui_execute_claims{},ui_execute_content_rejected{},ui_execute_content_matches{},ui_execute_bad_return{},
    ui_gpu_context_calls{},ui_gpu_context_missing{};
std::atomic<uint64_t> ui_index_capture_ok{},ui_index_capture_rejected{},ui_index_callback_ok{},
    ui_index_callback_rejected{},ui_index_complete{},ui_index_incomplete{},ui_execute_sequence{};
struct UiHudCounts {std::atomic<uint64_t> considered{},missing_context{},reticle_excluded{},unknown_or_mixed{},
    ticket_missing{},projection_rejected{},planned{},applied{};} ui_hud_counts;
std::atomic<bool> ui_hud_sequence_fault{};
std::atomic<uint64_t> ui_ads_anchor_planned{},ui_ads_anchor_finite{},ui_ads_anchor_direction{};
gears_ui_fault::First ui_hud_first_fault;
#if defined(GEARS_UI_RETICLE_DIRECTION) && GEARS_UI_RETICLE_DIRECTION
constexpr bool ui_reticle_direction_enabled=true;
#else
constexpr bool ui_reticle_direction_enabled=false;
#endif
struct UiReticleCounts {std::atomic<uint64_t> considered{},center_rejected{},geometry_missing{},projection_rejected{},planned{},applied{},finite_planned{},finite_suppressed{},finite_applied{};} ui_reticle_counts;
struct UiLayoutContext {gears_ui_layout::Groups groups{};gears_ui_layout::Replay replay{};};
struct UiIndexContext {
    uint64_t execute_sequence{};
    gears_ui_indices::Snapshot snapshot{};
    gears_ui_indices::Cursor cursor{};
    unsigned ordinal{};
    bool callback_valid{};
    bool finite_reticle{};
    gears_ui_indices::DrawArgs last_observed_draw{};
    bool observed_draw_valid{};
    uint64_t verified_ticket{};
    std::array<gears_ui_reticle::Extent,gears_ui_indices::segment_capacity> reticle_extents{};
    std::array<gears_ui_sources::Summary,gears_ui_indices::segment_capacity> sources{};
    std::unique_ptr<UiLayoutContext> layout;
};
thread_local UiIndexContext* ui_index_context{};
struct UiPayloadContext {
    uintptr_t command{},payload{};uint64_t generation{},root_first{},root_last{};
    unsigned vertex_count{},receipts{},kind_mask{};std::array<unsigned,3> kind_vertices{};
    uint64_t unknown_vertices{};
};
thread_local UiPayloadContext* ui_payload_context{};
struct UiPayloadRestore {
    UiPayloadContext* saved=ui_payload_context;
    UiIndexContext* saved_indices=ui_index_context;
    uintptr_t saved_return=ui_execute_entry_return;
    ~UiPayloadRestore(){ui_payload_context=saved;ui_index_context=saved_indices;ui_execute_entry_return=saved_return;}
};
struct UiPayloadGpuRow {
    UiPayloadContext context{};uint64_t ticket{};gears_frame_link::ListRef list{};
    gears_frame_link::Target target{};long long qpc{};
    unsigned thread{},indices{},instances{},start{};int vertex{};
    uint64_t execute_sequence{};
    gears_ui_indices::Segment segment{};
    unsigned segment_ordinal{};
    bool index_callback_matched{},command_completed{};
    gears_vr::StereoTicketGeometry geometry{};
    bool geometry_matched{};
    bool hud_plane_applied{};
    gears_viewport::OverlayPlan hud_plan{};
    bool reticle_direction_applied{};
    gears_viewport::OverlayPlan reticle_plan{};
#if (defined(GEARS_UI_CALL_BOUNDARY_OBSERVER) && GEARS_UI_CALL_BOUNDARY_OBSERVER) || (defined(GEARS_UI_RETICLE_LATE_SAMPLE) && GEARS_UI_RETICLE_LATE_SAMPLE)
    gears_ui_reticle::Extent reticle_extent{};
#endif
};
std::array<std::array<UiPayloadGpuRow,24>,8> ui_payload_gpu_rows{};
std::array<unsigned,8> ui_payload_gpu_counts{};
std::array<long long,8> ui_payload_next_row{};
std::array<uint64_t,8> ui_payload_gpu_calls{};
bool ui_ticket_geometry(uint64_t ticket,gears_vr::StereoTicketGeometry& geometry);
#include "reticle_sample_live.inl"
struct UiSubtitleGpuRow {
    gears_ui_sources::Summary source{};uint64_t execute_sequence{},ticket{};
    gears_frame_link::ListRef list{};gears_frame_link::Target target{};
    long long qpc{};unsigned thread{};bool hud_applied{},command_completed{},layout_applied{};
};
std::array<UiSubtitleGpuRow,16> ui_subtitle_gpu_rows{};
unsigned ui_subtitle_gpu_count{};uint64_t ui_subtitle_gpu_calls{},ui_subtitle_gpu_dropped{},ui_subtitle_gpu_layout_applied{};
long long ui_subtitle_gpu_next{};

// Called only at the existing permanent-fault branches. TLS points to our
// owning Execute-entry copies; native command/payload storage is never read.
void record_ui_hud_first_fault(gears_ui_fault::Reason reason,bool result_valid=false,
        uint32_t result=0,bool sequence_complete=false,gears_ui_fault::FinishEvidence const& finish={}){
    if(ui_hud_first_fault.published()||!ui_index_context)return;
    auto const& index=*ui_index_context;
    gears_ui_fault::Row row{};row.reason=reason;row.qpc=now();row.frequency=qpc_frequency;
    row.thread=GetCurrentThreadId();row.execute_sequence=index.execute_sequence;
    row.cursor_expected=index.cursor.expected_count();row.cursor_observed=index.cursor.observed();
    row.cursor_poisoned=index.cursor.poisoned();row.sequence_complete=sequence_complete;
    row.result_valid=result_valid;row.result=result;row.finish=finish;
    row.observed_draw_valid=index.observed_draw_valid;row.observed_draw=index.last_observed_draw;
    if(row.cursor_observed<index.snapshot.segment_count){
        row.expected_draw_valid=true;row.expected_draw=index.snapshot.segments[row.cursor_observed].expected;
    }
    row.ticket=index.verified_ticket;row.ticket_valid=row.ticket!=0;
    if(ui_payload_context){auto const& payload=*ui_payload_context;
        row.command=payload.command;row.payload=payload.payload;row.candidate_generation=payload.generation;
        row.root_first=payload.root_first;row.root_last=payload.root_last;
    }
    ui_hud_first_fault.record(row);
}

bool validate_ui_payload_content(gears_ui_receipts::Claim const& claim,UiPayloadContext& context,gears_ui_binding::Binding& binding){
    if(!ui_capture_binding(binding)){++ui_binding_claims[3];return false;}
    for(size_t i=0;i<claim.count&&i<claim.entries.size();++i)
        if(!ui_binding_current(claim.entries[i].root_invocation,binding))return false;
    gears_ui_batch::detail::Reader<decltype(&safe_copy)> read{safe_copy};
    gears_ui_batch::detail::Array vertices{},after{};
    if(!claim.count||claim.count>claim.entries.size()||!read.field(vertices,claim.payload,0x48)||
       !gears_ui_batch::detail::valid(vertices,1048576,gears_ui_batch::vertex_stride)||
       !gears_ui_indices::detail::claim_valid(claim,claim.payload,unsigned(vertices.count)))return false;
    unsigned covered=0;
    for(size_t i=0;i<claim.count;++i){auto const& entry=claim.entries[i];auto const& receipt=entry.receipt;
        const auto length=receipt.end-receipt.begin;
        for(unsigned v=0;v<length;++v){uintptr_t address{};gears_ui_batch::Vertex copied{};
            if(!gears_ui_batch::detail::add(vertices.data,size_t(receipt.begin+v)*gears_ui_batch::vertex_stride,address)||
               !read.bytes(copied.data(),address,copied.size())||copied!=claim.vertices->data[entry.vertex_offset+v])return false;
        }
        covered+=length;context.kind_vertices[entry.kind-1]+=length;context.kind_mask|=1u<<(entry.kind-1);
        if(!i)context.root_first=entry.root_invocation;
        context.root_last=entry.root_invocation;
    }
    if(covered>unsigned(vertices.count)||!read.field(after,claim.payload,0x48)||
       !gears_ui_batch::detail::same(vertices,after))return false;
    context.payload=claim.payload;context.generation=claim.generation;
    context.vertex_count=unsigned(vertices.count);context.receipts=unsigned(claim.count);
    context.unknown_vertices=unsigned(vertices.count)-covered;
    return true;
}

uint32_t __fastcall hook_ui_execute(void* command){
    if(stereo_publish_bypass)return ui_original_execute(command);
    ui_scene_root_live::executing(1);
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    Scope active_scope;if(!active_scope.world())return ui_original_execute(command);
    UiPayloadRestore restore;ui_payload_context=nullptr;ui_index_context=nullptr;ui_execute_entry_return=0;
    const auto owned_receipt=owned_ui_live::execute_begin(command);
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
    owned_ui_live::CommandScope owned_command(owned_receipt,command,caller);
#endif
    UiPayloadContext context{};
    gears_ui_binding::Binding consumer_binding{};
    UiSemanticScope semantic_scope;
    std::unique_ptr<UiIndexContext> index_storage;
    // Empty contexts deliberately shadow a parent, including nested unsupported
    // Execute calls; their GPU work cannot inherit the parent's semantic label.
    if(ui_observer_enabled){
        ++ui_execute_calls;
        std::array<unsigned char,48> header{},checked{};uintptr_t table{},payload{};
        const bool readable=command&&safe_copy(header.data(),command,header.size());
        if(readable){std::memcpy(&table,header.data(),8);std::memcpy(&payload,header.data()+8,8);}
        if(!readable||table!=base+0x13394f0||!payload)++ui_execute_header_rejected;
        else {
            // Route identity is separate from semantic receipts: glyph/unknown
            // payloads must retain their existing stereo overlay behavior too.
            ui_execute_entry_return=caller;
            gears_ui_receipts::Claim claim{};bool claimed=false;
            {std::lock_guard<std::mutex> lock(ui_payload_mutex);claimed=ui_payload_store.claim(payload,claim);}
            if(claimed){
                ++ui_execute_claims;
                if(recording.load()&&validate_ui_payload_content(claim,context,consumer_binding)&&
                   safe_copy(checked.data(),command,checked.size())&&checked==header){
                    context.command=reinterpret_cast<uintptr_t>(command);
                    semantic_scope.begin(consumer_binding);
                    ++ui_execute_content_matches;ui_payload_context=&context;
                    index_storage.reset(new(std::nothrow) UiIndexContext{});
                    if(index_storage&&caller==base+0x19f2f3&&
                       gears_ui_indices::capture(safe_copy,payload,claim,index_storage->snapshot)&&
                       safe_copy(checked.data(),command,checked.size())&&checked==header){
                        index_storage->execute_sequence=++ui_execute_sequence;
                        index_storage->cursor=gears_ui_indices::Cursor(index_storage->snapshot);
                        if(ui_reticle_direction_enabled&&(context.kind_mask&1u)){
                            for(unsigned i=0;i<index_storage->snapshot.segment_count;++i)
                                gears_ui_reticle::capture_extent(claim,index_storage->snapshot,i,index_storage->reticle_extents[i]);
                            gears_ui_reticle::group_extents(index_storage->reticle_extents,index_storage->snapshot.segment_count);
                        }
                        for(unsigned i=0;i<index_storage->snapshot.segment_count;++i)
                            gears_ui_sources::capture(claim,index_storage->snapshot,i,index_storage->sources[i]);
                        if(!gears_ui_layout::identity(ui_hud_layout_parameters)){
                            // Certified Execute passes payload+0x80 to 1720a0.
                            // Freeze/recheck before native uploads/destruction;
                            // no later GPU callback reads the native payload.
                            gears_ui_layout::Matrix matrix{},again{};
                            std::unique_ptr<UiLayoutContext> layout(new(std::nothrow) UiLayoutContext{});
                            gears_ui_batch::detail::Reader<decltype(&safe_copy)> reader{safe_copy};
                            if(layout&&reader.field(matrix,payload,0x80)&&
                               gears_ui_layout::capture(claim,index_storage->snapshot,matrix,layout->groups)&&
                               reader.field(again,payload,0x80)&&matrix==again&&
                               safe_copy(checked.data(),command,checked.size())&&checked==header)
                                index_storage->layout=std::move(layout);
                        }
                        ui_index_context=index_storage.get();++ui_index_capture_ok;
                    }else ++ui_index_capture_rejected;
                }else ++ui_execute_content_rejected;
            }
        }
    }
    // Original owns/destructs the payload. No command/payload reads after return.
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
    const uint32_t result=ui_original_execute(owned_command.argument);
#else
    const uint32_t result=ui_original_execute(command);
#endif
    // Finish semantic HUD observation before recording the independent owned
    // Canvas Execute result. Unlabelled batches have no semantic cursor.
    gears_ui_fault::FinishEvidence finish_evidence{};
    if(ui_index_context){
        finish_evidence.before_finish_valid=true;
        finish_evidence.before_finish_poisoned=ui_index_context->cursor.poisoned();
        finish_evidence.before_finish_expected=ui_index_context->cursor.expected_count();
        finish_evidence.before_finish_observed=ui_index_context->cursor.observed();
    }
    const bool sequence_complete=!ui_index_context||ui_index_context->cursor.finish();
    const bool binding_complete=semantic_scope.end(sequence_complete&&result==48);
    finish_evidence.binding_complete_valid=true;finish_evidence.binding_complete=binding_complete;
    if(ui_index_context&&!binding_complete&&sequence_complete&&result==48){
        ui_hud_sequence_fault=true;
        record_ui_hud_first_fault(gears_ui_fault::Reason::finish,true,result,sequence_complete,finish_evidence);
    }
    owned_ui_live::execute_end(owned_receipt,result,sequence_complete&&binding_complete);
    if(ui_observer_enabled&&result!=48)++ui_execute_bad_return;
    // Only our independent value copies remain live. The native command and
    // payload have already been destroyed and must not be read here.
    if(ui_index_context){
        const bool complete=binding_complete&&sequence_complete&&result==48;
        if(complete)++ui_index_complete;else ++ui_index_incomplete;
        // A bad semantic sequence disables later HUD placement. The Canvas
        // receipt above remains governed by its own Execute and GPU proof.
        if((!sequence_complete||result!=48)&&ui_hud_plane_enabled){
            ui_hud_sequence_fault=true;
            record_ui_hud_first_fault(result!=48?gears_ui_fault::Reason::return_value:gears_ui_fault::Reason::finish,
                true,result,sequence_complete,finish_evidence);
        }
        std::lock_guard<std::mutex> lock(ui_payload_mutex);
        for(unsigned i=0;i<ui_subtitle_gpu_count;++i)
            if(ui_subtitle_gpu_rows[i].execute_sequence==ui_index_context->execute_sequence)
                ui_subtitle_gpu_rows[i].command_completed=complete;
        for(unsigned k=1;k<ui_payload_gpu_rows.size();++k)
            for(unsigned i=0;i<ui_payload_gpu_counts[k];++i){auto& row=ui_payload_gpu_rows[k][i];
                if(row.execute_sequence==ui_index_context->execute_sequence)row.command_completed=complete;}
        ui_reticle_late_sample::complete(ui_index_context->execute_sequence,complete);
    }
    return result;
}

void observe_ui_index_callback(uintptr_t caller,gears_ui_indices::DrawArgs args){
    if(!ui_index_context)return;
    ui_index_context->callback_valid=false;
    if(caller!=base+0x172741)return;
    ui_index_context->last_observed_draw=args;ui_index_context->observed_draw_valid=true;
    unsigned ordinal{};
    if(ui_execute_entry_return==base+0x19f2f3&&ui_index_context->cursor.consume(args,ordinal)){
        ui_index_context->ordinal=ordinal;ui_index_context->callback_valid=true;++ui_index_callback_ok;
    }else {++ui_index_callback_rejected;if(ui_hud_plane_enabled){
        ui_hud_sequence_fault=true;record_ui_hud_first_fault(gears_ui_fault::Reason::callback);
    }}
}

bool ui_ticket_geometry(uint64_t ticket,gears_vr::StereoTicketGeometry& geometry){
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
    if(owned_ui_live::route.ticket)return owned_ui_live::route.ticket==ticket&&
        owned_scene_gpu::image_tickets.lookup_geometry(ticket,geometry,simulator_operator);
#endif
    return stereo_ticket_store.lookup_geometry(ticket,geometry,simulator_operator);
}
bool ui_ticket_sample(uint64_t ticket,gears_vr::StereoSample& sample){
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
    if(owned_ui_live::route.ticket)return owned_ui_live::route.ticket==ticket&&
        owned_scene_gpu::image_tickets.lookup(ticket,sample,simulator_operator);
#endif
    return stereo_ticket_store.lookup(ticket,sample,simulator_operator);
}
double ui_source_aspect(){
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
    if(owned_ui_live::route.ticket)return double(owned_ui_live::route.source_width)/owned_ui_live::route.source_height;
#endif
    return 0;
}
bool prepare_ui_panel(uint64_t ticket,gears_viewport::DrawState const& saved,unsigned width,unsigned height,
    gears_vr::StereoSample const& sample,gears_viewport::OverlayPlan& out,
    gears_ui_hud::Parameters parameters,gears_ui_hud::Diagnostic* diagnostic=nullptr){
    gears_vr::StereoTicketGeometry geometry{};
    if(ui_ticket_geometry(ticket,geometry)&&geometry.aiming){
        const bool planned=gears_ui_aim::plan(saved,width,height,sample,geometry,out,parameters,diagnostic);
        if(planned){++ui_ads_anchor_planned;if(geometry.finite_aim.valid)++ui_ads_anchor_finite;else ++ui_ads_anchor_direction;}
        return planned;
    }
    return gears_ui_hud::plan(saved,width,height,sample,out,parameters,diagnostic);
}
bool prepare_ui_reticle_overlay(uint64_t ticket,gears_viewport::DrawState const& saved,
        unsigned width,unsigned height,gears_viewport::OverlayPlan& out){
    if(ui_index_context)ui_index_context->finite_reticle=false;
    if(!ui_reticle_direction_enabled||!ui_hud_plane_enabled||ui_hud_sequence_fault.load()||
       !ui_payload_context||!ui_index_context||!ui_index_context->callback_valid||
       ui_index_context->snapshot.payload!=ui_payload_context->payload||
       ui_index_context->snapshot.generation!=ui_payload_context->generation)return false;
    const auto ordinal=ui_index_context->ordinal;
    const auto& segment=ui_index_context->snapshot.segments[ordinal];
    if(segment.kind_mask!=1||segment.unknown||!segment.index_count)return false;
    ++ui_reticle_counts.considered;
    unsigned source_width=width,source_height=height;
#if defined(GEARS_OWNED_UI_BATCH) && GEARS_OWNED_UI_BATCH
    if(owned_ui_live::route.ticket){source_width=owned_ui_live::route.source_width;source_height=owned_ui_live::route.source_height;}
#endif
    gears_vr::StereoTicketGeometry geometry{};
    if(!ui_ticket_geometry(ticket,geometry)){
        ++ui_reticle_counts.geometry_missing;
        auto hidden=gears_viewport::overlay_plan(saved,width,height);if(!hidden.valid)return false;
        for(unsigned eye=0;eye<2;++eye)hidden.scissors[eye]={float(eye*width/2),0,0,0};
        out=hidden;return true;}
    if(!gears_ui_reticle::centered(ui_index_context->reticle_extents[ordinal],source_width,source_height)){
        ++ui_reticle_counts.center_rejected;
        // Pure certified ADS segments (including isolated ring quadrants) share
        // one source-center map. Never route them onto the head-locked HUD.
        if(!geometry.aiming){auto hidden=gears_viewport::overlay_plan(saved,width,height);if(!hidden.valid)return false;
            for(unsigned eye=0;eye<2;++eye)hidden.scissors[eye]={float(eye*width/2),0,0,0};out=hidden;return true;}}
    gears_viewport::OverlayPlan corrected{};
    bool suppressed=false;
    bool planned=gears_ui_reticle::display_plan(saved,width,height,geometry,corrected,ui_source_aspect(),&suppressed);
    if(geometry.finite_aim.valid){
        if(planned){++ui_reticle_counts.finite_planned;ui_index_context->finite_reticle=true;}
        if(suppressed)++ui_reticle_counts.finite_suppressed;}
    if(!planned){++ui_reticle_counts.projection_rejected;
        auto hidden=gears_viewport::overlay_plan(saved,width,height);if(!hidden.valid)return false;
        for(unsigned eye=0;eye<2;++eye)hidden.scissors[eye]={float(eye*width/2),0,0,0};
        out=hidden;return true;}
    out=corrected;++ui_reticle_counts.planned;return true;
}

bool prepare_ui_hud_overlay(uint64_t ticket,gears_viewport::DrawState const& saved,
        unsigned width,unsigned height,gears_viewport::OverlayPlan& out){
    if(!ui_hud_plane_enabled||ui_hud_sequence_fault.load())return false;
    ++ui_hud_counts.considered;
    if(!ui_payload_context||!ui_index_context||!ui_index_context->callback_valid||
       ui_index_context->snapshot.payload!=ui_payload_context->payload||
       ui_index_context->snapshot.generation!=ui_payload_context->generation){++ui_hud_counts.missing_context;return false;}
    const auto& segment=ui_index_context->snapshot.segments[ui_index_context->ordinal];
    // Weapon UI and ordinary HUD share one head-locked plane. Reticle indices
    // require a world target and cannot inherit a fixed HUD depth.
    if(segment.kind_mask==1&&!segment.unknown){++ui_hud_counts.reticle_excluded;return false;}
    if(!segment.index_count||segment.unknown||!segment.kind_mask||(segment.kind_mask&~6u)){
        ++ui_hud_counts.unknown_or_mixed;return false;}
    gears_vr::StereoSample sample{};
    if(!ui_ticket_sample(ticket,sample)){++ui_hud_counts.ticket_missing;return false;}
    gears_viewport::OverlayPlan corrected{};
    const auto parameters=gears_ui_hud::for_source(ui_hud_parameters,ui_source_aspect());
    if(!prepare_ui_panel(ticket,saved,width,height,sample,corrected,parameters)){++ui_hud_counts.projection_rejected;return false;}
    out=corrected;++ui_hud_counts.planned;return true;
}

gears_ui_layout::Replay const* prepare_ui_hud_layout(gears_viewport::DrawState const& saved,unsigned width,unsigned height,
        gears_viewport::OverlayPlan const& panel){
    if(gears_ui_layout::identity(ui_hud_layout_parameters)||!ui_hud_plane_enabled||ui_hud_sequence_fault.load()||
       !ui_payload_context||!ui_index_context||!ui_index_context->callback_valid||
       ui_index_context->snapshot.payload!=ui_payload_context->payload||
       ui_index_context->snapshot.generation!=ui_payload_context->generation)return nullptr;
    ++ui_layout_counts.considered;
    auto* layout=ui_index_context->layout.get();
    const bool prepared=layout&&gears_ui_layout::prepare(layout->groups,ui_index_context->ordinal,saved,width,height,
        panel,ui_hud_layout_parameters,layout->replay);
    if(prepared)++ui_layout_counts.planned;else ++ui_layout_counts.fallback;
    return prepared?&layout->replay:nullptr;
}

bool reserve_ui_execute(){
    ui_execute_target=reinterpret_cast<void*>(base+0x19e64c);
    std::array<unsigned char,32> before{},after{};uintptr_t entry{};
    DWORD64 observer_image{};
    const auto observer_function=RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(hook_ui_execute),&observer_image,nullptr);
    if(!observer_function||!observer_image||observer_function->EndAddress<=observer_function->BeginAddress)return false;
    ui_execute_observer_begin=observer_image+observer_function->BeginAddress;
    ui_execute_observer_end=observer_image+observer_function->EndAddress;
    if(!safe_copy(before.data(),ui_execute_target,before.size())||before!=ui_execute_bytes||
       !safe_copy(&entry,reinterpret_cast<void*>(base+0x13394f0+8),8)||entry!=base+0x19e64c||
       !gears_ui_batch::validate_binding({base},safe_copy)||
       MH_CreateHook(ui_execute_target,reinterpret_cast<void*>(hook_ui_execute),reinterpret_cast<void**>(&ui_original_execute))!=MH_OK||
       !safe_copy(after.data(),ui_execute_target,after.size())||after!=before)return false;
    return true;
}
bool ui_observed_overlay_chain(const uintptr_t* addresses,size_t count){
    return ui_observer_enabled&&ui_execute_entry_return&&gears_viewport::overlay_observed_caller_chain(
        addresses,count,base,ui_execute_entry_return,ui_execute_observer_begin,ui_execute_observer_end);
}

void observe_ui_gpu_payload(uint64_t ticket,gears_frame_link::ListRef list,gears_frame_link::Target target,
        unsigned indices,unsigned instances,unsigned start,int vertex,bool hud_applied=false,
        gears_viewport::OverlayPlan const* hud_plan=nullptr,bool reticle_applied=false,
        gears_viewport::OverlayPlan const* reticle_plan=nullptr,bool layout_applied=false){
    if(!ui_observer_enabled||!recording.load())return;
    ++ui_gpu_context_calls;
    if(hud_applied)++ui_hud_counts.applied;
    if(reticle_applied)++ui_reticle_counts.applied;
    if(reticle_applied&&ui_index_context&&ui_index_context->finite_reticle)++ui_reticle_counts.finite_applied;
    if(!ui_payload_context||!ticket){++ui_gpu_context_missing;return;}
    const auto& context=*ui_payload_context;const auto kind=context.kind_mask;
    if(!kind||kind>=ui_payload_gpu_rows.size()){++ui_gpu_context_missing;return;}
    const auto timestamp=now();std::lock_guard<std::mutex> lock(ui_payload_mutex);
#if defined(GEARS_UI_RETICLE_LATE_SAMPLE) && GEARS_UI_RETICLE_LATE_SAMPLE
    if(kind&1u)ui_reticle_late_sample::observe(context,ticket,list,target,timestamp,indices,instances,start,vertex,reticle_applied,reticle_plan);
#endif
    // This function's sole caller is after normal replay_overlay return: valid
    // plan executes both eyes or throws before this observer. This proves API
    // replay completion, not subtitle pixels, visibility or whole Execute exit.
    if(ui_index_context&&ui_index_context->callback_valid&&
       ui_index_context->snapshot.payload==context.payload&&
       ui_index_context->snapshot.generation==context.generation&&
       ui_index_context->ordinal<ui_index_context->snapshot.segment_count){
        // Existing caller has completed replay for this validated ticket and
        // matching segment. Retain only the value, never a latest/global lookup.
        ui_index_context->verified_ticket=ticket;
        const auto ordinal=ui_index_context->ordinal;const auto& source=ui_index_context->sources[ordinal];
        const auto& segment=ui_index_context->snapshot.segments[ordinal];
        if(gears_ui_sources::native_subtitle(source,context.payload,context.generation,ordinal)&&
           indices==segment.expected.index_count&&instances==segment.expected.instance_count&&
           start==segment.expected.start_index&&vertex==segment.expected.base_vertex){
            ++ui_subtitle_gpu_calls;
            if(layout_applied)++ui_subtitle_gpu_layout_applied;
#if defined(GEARS_OWNED_SUBTITLE_PIXELS) && GEARS_OWNED_SUBTITLE_PIXELS
            if(owned_ui_live::route.ticket==ticket&&hud_applied)
                owned_scene_gpu::subtitle_candidate(ticket,ui_index_context->execute_sequence,list,target,ordinal,
                    source.native_root_min,source.native_root_max);
#endif
            if(ui_subtitle_gpu_count<ui_subtitle_gpu_rows.size()&&
               (ui_subtitle_gpu_count<4||timestamp>=ui_subtitle_gpu_next)){
                ui_subtitle_gpu_rows[ui_subtitle_gpu_count++]={source,ui_index_context->execute_sequence,ticket,list,target,timestamp,GetCurrentThreadId(),hud_applied,false,layout_applied};
                ui_subtitle_gpu_next=timestamp+qpc_frequency;
            }else ++ui_subtitle_gpu_dropped;
        }
    }
    ++ui_payload_gpu_calls[kind];auto& count=ui_payload_gpu_counts[kind];
    bool consecutive_reticle=false;
#if defined(GEARS_UI_CALL_BOUNDARY_OBSERVER) && GEARS_UI_CALL_BOUNDARY_OBSERVER
    consecutive_reticle=kind==1; // Same fixed capacity, diagnostic sampling only.
#endif
    if(count>=ui_payload_gpu_rows[kind].size()||(!consecutive_reticle&&count>=8&&timestamp<ui_payload_next_row[kind]))return;
    ui_payload_next_row[kind]=timestamp+qpc_frequency;
    auto& row=ui_payload_gpu_rows[kind][count++];
    row={context,ticket,list,target,timestamp,GetCurrentThreadId(),indices,instances,start,vertex};
    row.geometry_matched=ui_ticket_geometry(ticket,row.geometry);
    row.hud_plane_applied=hud_applied;if(hud_plan)row.hud_plan=*hud_plan;
    row.reticle_direction_applied=reticle_applied;if(reticle_plan)row.reticle_plan=*reticle_plan;
    if(ui_index_context&&ui_index_context->callback_valid){
        row.execute_sequence=ui_index_context->execute_sequence;
        row.segment_ordinal=ui_index_context->ordinal;
        row.segment=ui_index_context->snapshot.segments[row.segment_ordinal];
        row.index_callback_matched=true;
#if defined(GEARS_UI_CALL_BOUNDARY_OBSERVER) && GEARS_UI_CALL_BOUNDARY_OBSERVER
        if(row.segment_ordinal<ui_index_context->reticle_extents.size()){
            auto const& extent=ui_index_context->reticle_extents[row.segment_ordinal];
            if(extent.valid)row.reticle_extent=extent;
        }
#endif
    }
}

void print_ui_payload_observer(FILE* status){
    ui_reticle_late_sample::print(status);
    fprintf(status,"ui_ads_anchor planned=%llu finite=%llu direction=%llu head_locked_fallback=0 scope=certified_targeting_state\n",
        ui_ads_anchor_planned.load(),ui_ads_anchor_finite.load(),ui_ads_anchor_direction.load());
    gears_ui_fault::print(status,ui_hud_first_fault);
    fprintf(status,"ui_subtitle_gpu calls=%llu layout_applied=%llu rows=%u dropped=%llu source_is_actual_index_membership=1 pixel_visibility_proven=0\n",ui_subtitle_gpu_calls,ui_subtitle_gpu_layout_applied,ui_subtitle_gpu_count,ui_subtitle_gpu_dropped);
    for(unsigned i=0;i<ui_subtitle_gpu_count;++i){auto const& row=ui_subtitle_gpu_rows[i];auto const& source=row.source;
        fprintf(status,"ui_subtitle_gpu_row row=%u payload=%p candidate_generation=%llu ordinal=%u source_mask=%u native_indices=%u unknown=%u native_root_min=%llu native_root_max=%llu execute_sequence=%llu ticket=%llu list=%p list_generation=%llu target=%p width=%u height=%u qpc=%lld thread=%u hud_applied=%u layout_applied=%u overlay_completed_mask=3 command_completed=%u pixel_visibility_proven=0 allocation_identity_proven=0\n",
            i,reinterpret_cast<void*>(source.payload),source.generation,source.ordinal,source.source_mask,source.native_indices,source.unknown,source.native_root_min,source.native_root_max,row.execute_sequence,row.ticket,reinterpret_cast<void*>(row.list.identity),row.list.generation,reinterpret_cast<void*>(row.target.resource),row.target.width,row.target.height,row.qpc,row.thread,row.hud_applied?1:0,row.layout_applied?1:0,row.command_completed?1:0);
    }
    fprintf(status,"ui_reticle_direction enabled=%u considered=%llu center_rejected=%llu geometry_missing=%llu projection_rejected=%llu planned=%llu applied=%llu finite_hit_proven=0 source_vertices_modified=0\n",
        ui_reticle_direction_enabled?1:0,ui_reticle_counts.considered.load(),ui_reticle_counts.center_rejected.load(),
        ui_reticle_counts.geometry_missing.load(),ui_reticle_counts.projection_rejected.load(),ui_reticle_counts.planned.load(),ui_reticle_counts.applied.load());
    fprintf(status,"ui_reticle_finite_query plans=%llu applied=%llu direction_fallback=0 finite_suppressed=%llu camera_equivalence_proven=0 current_camera_aim_proven=0 bullet_impact_proven=0\n",
        ui_reticle_counts.finite_planned.load(),ui_reticle_counts.finite_applied.load(),ui_reticle_counts.finite_suppressed.load());
    const auto store=ui_payload_store.stats();
    fprintf(status,"ui_payload_store published=%llu rejected=%llu capacity_rejected=%llu groups_poisoned=%llu claimed=%llu poison_claimed=%llu active_groups=%zu poisoned_groups=%zu disabled=%u resets=%llu groups_full=%llu capacity=%zu\n",
        store.published,store.rejected,store.capacity_rejected,store.groups_poisoned,store.claimed,store.poison_claimed,
        store.active_groups,store.poisoned_groups,store.disabled?1:0,store.resets,store.groups_full,gears_ui_receipts::group_capacity);
    fprintf(status,"ui_batch attempts=%llu before_rejected=%llu append_rejected=%llu published=%llu publish_rejected=%llu source_vertices_modified=0\n",
        ui_batch_attempts.load(),ui_batch_before_rejected.load(),ui_batch_append_rejected.load(),ui_batch_published.load(),ui_batch_publish_rejected.load());
    fprintf(status,"ui_payload_execute calls=%llu header_rejected=%llu claims=%llu content_rejected=%llu content_matches=%llu bad_return=%llu gpu_calls=%llu gpu_missing_context=%llu allocation_identity_proven=0 index_membership_requires_per_row=1\n",
        ui_execute_calls.load(),ui_execute_header_rejected.load(),ui_execute_claims.load(),ui_execute_content_rejected.load(),
        ui_execute_content_matches.load(),ui_execute_bad_return.load(),ui_gpu_context_calls.load(),ui_gpu_context_missing.load());
    fprintf(status,"ui_index_execute capture_ok=%llu capture_rejected=%llu callbacks_ok=%llu callbacks_rejected=%llu completed=%llu incomplete=%llu source_vertices_modified=0\n",
        ui_index_capture_ok.load(),ui_index_capture_rejected.load(),ui_index_callback_ok.load(),ui_index_callback_rejected.load(),
        ui_index_complete.load(),ui_index_incomplete.load());
    const auto& hud_parameters=ui_hud_parameters;
    fprintf(status,"ui_hud_plane enabled=%u horizontal_fov_degrees=%.9g depth_metres=%.9g considered=%llu missing_context=%llu reticle_excluded=%llu unknown_or_mixed=%llu ticket_missing=%llu projection_rejected=%llu planned=%llu applied=%llu sequence_fault=%u source_vertices_modified=0\n",
        ui_hud_plane_enabled?1:0,hud_parameters.horizontal_fov_degrees,hud_parameters.depth_metres,ui_hud_counts.considered.load(),ui_hud_counts.missing_context.load(),ui_hud_counts.reticle_excluded.load(),
        ui_hud_counts.unknown_or_mixed.load(),ui_hud_counts.ticket_missing.load(),ui_hud_counts.projection_rejected.load(),
        ui_hud_counts.planned.load(),ui_hud_counts.applied.load(),ui_hud_sequence_fault.load()?1:0);
    fprintf(status,"ui_hud_layout element_scale=%.9g center_scale=%.9g considered=%llu planned=%llu fallback=%llu applied=%llu source_vertices_modified=0 subtitle_policy=complete_single_range_or_frozen_native_multirange_group reticle_policy=unchanged\n",
        ui_hud_layout_parameters.element_scale,ui_hud_layout_parameters.center_scale,ui_layout_counts.considered.load(),
        ui_layout_counts.planned.load(),ui_layout_counts.fallback.load(),ui_layout_counts.applied.load());
    for(unsigned k=1;k<ui_payload_gpu_rows.size();++k){
        fprintf(status,"ui_payload_gpu kind_mask=%u calls=%llu rows=%u\n",k,ui_payload_gpu_calls[k],ui_payload_gpu_counts[k]);
        for(unsigned i=0;i<ui_payload_gpu_counts[k];++i){auto const& r=ui_payload_gpu_rows[k][i];auto const& c=r.context;
            fprintf(status,"ui_payload_gpu_row kind_mask=%u row=%u command=%p payload=%p candidate_generation=%llu root_first=%llu root_last=%llu vertices=%u receipts=%u reticle_vertices=%u weapon_vertices=%u hud_vertices=%u unknown_vertices=%llu ticket=%llu list=%p list_generation=%llu target=%p target_width=%u target_height=%u indices=%u instances=%u start_index=%u base_vertex=%d qpc=%lld thread=%u content_matched_at_execute=1 source_vertices_modified=0\n",
                k,i,reinterpret_cast<void*>(c.command),reinterpret_cast<void*>(c.payload),c.generation,c.root_first,c.root_last,c.vertex_count,c.receipts,
                c.kind_vertices[0],c.kind_vertices[1],c.kind_vertices[2],c.unknown_vertices,r.ticket,reinterpret_cast<void*>(r.list.identity),r.list.generation,
                reinterpret_cast<void*>(r.target.resource),r.target.width,r.target.height,r.indices,r.instances,r.start,r.vertex,r.qpc,r.thread);
            if(r.index_callback_matched){auto const& s=r.segment;
                fprintf(status,"ui_index_gpu_row kind_mask=%u row=%u execute_sequence=%llu ticket=%llu ordinal=%u min_vertex=%d max_vertex=%d indices=%u reticle_indices=%u weapon_indices=%u hud_indices=%u unknown_indices=%u actual_kind_mask=%u entry_mask=%u callback_matched=1 command_completed=%u geometry_matched=%u allocation_identity_proven=0 source_vertices_modified=0\n",
                    k,i,r.execute_sequence,r.ticket,r.segment_ordinal,s.minimum,s.maximum,s.index_count,s.known[0],s.known[1],s.known[2],s.unknown,
                    s.kind_mask,s.entry_mask,r.command_completed?1:0,r.geometry_matched?1:0);
            }
            if(r.geometry_matched&&i<3){
                const gears_vr::StereoMatrix* matrices[]{&r.geometry.source_view,&r.geometry.source_projection,
                    &r.geometry.eyes.view[0],&r.geometry.eyes.projection[0],&r.geometry.eyes.view[1],&r.geometry.eyes.projection[1]};
                for(unsigned m=0;m<6;++m){
                    fprintf(status,"ui_geometry_row kind_mask=%u row=%u ticket=%llu matrix=%u values=",k,i,r.ticket,m);
                    for(unsigned n=0;n<16;++n)fprintf(status,n?",%.9g":"%.9g",double((*matrices[m])[n]));
                    fprintf(status,"\n");
                }
            }
            if(r.hud_plane_applied){
                for(unsigned e=0;e<2;++e){auto const& v=r.hud_plan.viewports[e];auto const& s=r.hud_plan.scissors[e];
                    fprintf(status,"ui_hud_plane_row kind_mask=%u row=%u ticket=%llu eye=%u viewport=%.9g,%.9g,%.9g,%.9g scissor=%.9g,%.9g,%.9g,%.9g applied=1\n",
                        k,i,r.ticket,e,v.x,v.y,v.width,v.height,s.x,s.y,s.width,s.height);
                }
            }
            if(r.reticle_direction_applied){
#if defined(GEARS_UI_CALL_BOUNDARY_OBSERVER) && GEARS_UI_CALL_BOUNDARY_OBSERVER
                auto const& extent=r.reticle_extent;
                fprintf(status,"ui_reticle_source_extent_row kind_mask=%u row=%u ticket=%llu ordinal=%u valid=%u root=%llu bounds=%.9g,%.9g,%.9g,%.9g\n",
                    k,i,r.ticket,r.segment_ordinal,extent.valid?1:0,extent.root_invocation,extent.left,extent.top,extent.right,extent.bottom);
#endif
                for(unsigned e=0;e<2;++e){auto const& v=r.reticle_plan.viewports[e];auto const& s=r.reticle_plan.scissors[e];
                    fprintf(status,"ui_reticle_direction_row kind_mask=%u row=%u ticket=%llu eye=%u viewport=%.9g,%.9g,%.9g,%.9g scissor=%.9g,%.9g,%.9g,%.9g applied=1\n",
                        k,i,r.ticket,e,v.x,v.y,v.width,v.height,s.x,s.y,s.width,s.height);
                }
            }
        }
    }
}
