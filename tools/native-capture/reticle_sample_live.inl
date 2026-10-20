// Included after UiPayloadGpuRow. Read-only optional worker control; no hooks.
#include "reticle_sample_arm.h"
namespace ui_reticle_late_sample {
bool enabled{}; // Root initializes only from explicit runtime opt-in.
#if defined(GEARS_UI_RETICLE_LATE_SAMPLE) && GEARS_UI_RETICLE_LATE_SAMPLE
bool configure(bool requested){enabled=requested;return true;}
gears_reticle_sample::Bank<UiPayloadGpuRow> bank;
std::string last_wire;
std::uint64_t load_rejected{},file_rejected{};
gears_reticle_sample::Identity identity(){
    FILETIME created{},exited{},kernel{},user{};
    if(!GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user))return {};
    ULARGE_INTEGER creation{};creation.LowPart=created.dwLowDateTime;creation.HighPart=created.dwHighDateTime;
    return {GetCurrentProcessId(),creation.QuadPart,base,std::uint64_t(session_started_qpc),committed_world_generation.load()};
}
bool read_file(wchar_t const* path,std::string& out){
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    struct Close {HANDLE file;~Close(){CloseHandle(file);}} close{file};
    BY_HANDLE_FILE_INFORMATION first{},last{};DWORD bytes{},again{};std::array<char,2048> a{},b{};
    if(!GetFileInformationByHandle(file,&first)||first.nFileSizeHigh||!first.nFileSizeLow||first.nFileSizeLow>a.size()||
       !ReadFile(file,a.data(),first.nFileSizeLow,&bytes,nullptr)||bytes!=first.nFileSizeLow||
       SetFilePointer(file,0,nullptr,FILE_BEGIN)==INVALID_SET_FILE_POINTER||
       !ReadFile(file,b.data(),bytes,&again,nullptr)||again!=bytes||a!=b||!GetFileInformationByHandle(file,&last)||
       first.nFileSizeHigh!=last.nFileSizeHigh||first.nFileSizeLow!=last.nFileSizeLow||
       CompareFileTime(&first.ftLastWriteTime,&last.ftLastWriteTime))return false;
    out.assign(a.data(),bytes);return true;
}
void poll(wchar_t const* path,FILE* status){
    if(!enabled||!status||!recording.load())return;
    // Disabled is a true no-op, including no armfile access. Unknown files
    // are never overwritten/deleted; repeated identical content is not retried.
    std::string wire;if(!read_file(path,wire))return;if(wire==last_wire)return;last_wire=wire;
    LARGE_INTEGER frequency{};if(!QueryPerformanceFrequency(&frequency)||frequency.QuadPart<=0){++file_rejected;return;}
    std::lock_guard<FrameHookMutex> frame_lock(frame_hook_mutex);
    std::lock_guard<std::mutex> payload_lock(ui_payload_mutex);
    gears_reticle_sample::Rejection reason{};
    auto ack=[&](gears_reticle_sample::Gate const& gate){
        const auto& r=gate.request;
        const int written=fprintf(status,"ui_reticle_late_ack schema=1 nonce=%llu pid=%llu creation_time=%llu base=%llu started_qpc=%llu world_generation=%llu request_qpc=%llu expires_qpc=%llu arm_qpc=%llu end_qpc=%llu qpc_frequency=%llu row_capacity=%u ticket_capacity=%u observation_only=1\n",
            r.nonce,r.identity.pid,r.identity.creation_time,r.identity.base,r.identity.started_qpc,r.identity.world_generation,
            r.request_qpc,r.expires_qpc,gate.arm_qpc,gate.end_qpc,gate.frequency,gears_reticle_sample::row_capacity,gears_reticle_sample::ticket_capacity);
        return written>0&&fflush(status)==0;
    };
    if(!gears_reticle_sample::worker_admit(bank,true,wire,identity(),std::uint64_t(now()),std::uint64_t(frequency.QuadPart),ack,reason)){
        ++load_rejected;fprintf(status,"ui_reticle_late_arm_rejected reason=%u observation_only=1\n",unsigned(reason));fflush(status);
    }
}
void complete(std::uint64_t sequence,bool success){
    bank.complete(sequence,success,[](UiPayloadGpuRow& row,std::uint64_t wanted,bool completed){
        if(row.execute_sequence==wanted)row.command_completed=completed;
    });
}
void observe(UiPayloadContext const& context,std::uint64_t ticket,gears_frame_link::ListRef list,
    gears_frame_link::Target target,long long timestamp,unsigned indices,unsigned instances,unsigned start,int vertex,
    bool applied,gears_viewport::OverlayPlan const* plan){
    if(!enabled||!bank.gate.accepted)return;
    const bool callback=ui_index_context&&ui_index_context->callback_valid;
    bool member=false;gears_vr::StereoTicketGeometry geometry{};
    if(callback)member=gears_reticle_sample::membership(ui_index_context->snapshot,ui_index_context->ordinal,
        context.payload,context.generation,{indices,instances,start,vertex,0},true);
    const bool geometry_matched=member&&ui_ticket_geometry(ticket,geometry);
    auto* row=bank.append(ticket,std::uint64_t(timestamp),identity(),member,geometry_matched);if(!row)return;
    *row={context,ticket,list,target,timestamp,GetCurrentThreadId(),indices,instances,start,vertex};
    row->execute_sequence=ui_index_context->execute_sequence;row->segment_ordinal=ui_index_context->ordinal;
    row->segment=ui_index_context->snapshot.segments[row->segment_ordinal];row->index_callback_matched=true;
    row->geometry=geometry;row->geometry_matched=true;row->reticle_direction_applied=applied;
    if(plan)row->reticle_plan=*plan;
    row->reticle_extent=ui_index_context->reticle_extents[row->segment_ordinal];
}
void print(FILE* status){
    const auto& gate=bank.gate;const auto nonce=gate.request.nonce;
    fprintf(status,"ui_reticle_late_sample compiled=1 enabled=%u accepted=%u acknowledged=%u nonce=%llu arm_qpc=%llu end_qpc=%llu rows=%u tickets=%u frozen=%u rejected=%llu capacity_rejected=%llu load_rejected=%llu file_rejected=%llu normal_quota_unchanged=1 rendering_modified=0\n",
        enabled?1u:0u,gate.accepted?1u:0u,gate.acknowledged?1u:0u,nonce,gate.arm_qpc,gate.end_qpc,bank.count,bank.ticket_count,gate.frozen?1u:0u,bank.rejected,bank.capacity_rejected,load_rejected,file_rejected);
    for(unsigned i=0;i<bank.count;++i){const auto& row=bank.rows[i];const auto& c=row.context;const auto& s=row.segment;
        fprintf(status,"ui_reticle_late_payload_row bank=late nonce=%llu kind_mask=%u row=%u command=%p payload=%p candidate_generation=%llu root_first=%llu root_last=%llu vertices=%u receipts=%u reticle_vertices=%u weapon_vertices=%u hud_vertices=%u unknown_vertices=%llu ticket=%llu list=%p list_generation=%llu target=%p target_width=%u target_height=%u indices=%u instances=%u start_index=%u base_vertex=%d qpc=%lld thread=%u content_matched_at_execute=1 source_vertices_modified=0\n",
            nonce,c.kind_mask,i,reinterpret_cast<void*>(c.command),reinterpret_cast<void*>(c.payload),c.generation,c.root_first,c.root_last,c.vertex_count,c.receipts,c.kind_vertices[0],c.kind_vertices[1],c.kind_vertices[2],c.unknown_vertices,row.ticket,reinterpret_cast<void*>(row.list.identity),row.list.generation,reinterpret_cast<void*>(row.target.resource),row.target.width,row.target.height,row.indices,row.instances,row.start,row.vertex,row.qpc,row.thread);
        fprintf(status,"ui_reticle_late_index_row bank=late nonce=%llu kind_mask=%u row=%u execute_sequence=%llu ticket=%llu ordinal=%u min_vertex=%d max_vertex=%d indices=%u reticle_indices=%u weapon_indices=%u hud_indices=%u unknown_indices=%u actual_kind_mask=%u entry_mask=%u callback_matched=1 command_completed=%u geometry_matched=%u allocation_identity_proven=0 source_vertices_modified=0\n",
            nonce,c.kind_mask,i,row.execute_sequence,row.ticket,row.segment_ordinal,s.minimum,s.maximum,s.index_count,s.known[0],s.known[1],s.known[2],s.unknown,s.kind_mask,s.entry_mask,row.command_completed?1u:0u,row.geometry_matched?1u:0u);
        const auto& e=row.reticle_extent;
        fprintf(status,"ui_reticle_late_extent_row bank=late nonce=%llu kind_mask=%u row=%u ticket=%llu ordinal=%u valid=%u root=%llu bounds=%.9g,%.9g,%.9g,%.9g applied=%u\n",
            nonce,c.kind_mask,i,row.ticket,row.segment_ordinal,e.valid?1u:0u,e.root_invocation,e.left,e.top,e.right,e.bottom,row.reticle_direction_applied?1u:0u);
        const auto& aim=row.geometry.finite_aim;
        fprintf(status,"ui_reticle_late_anchor_row bank=late nonce=%llu kind_mask=%u row=%u ticket=%llu finite_available=%u world=%llu tick=%llu point=%.9g,%.9g,%.9g current_camera_aim_proven=0 bullet_impact_proven=0\n",
            nonce,c.kind_mask,i,row.ticket,aim.valid?1u:0u,aim.world,aim.tick,aim.point[0],aim.point[1],aim.point[2]);
        if(row.reticle_direction_applied)for(unsigned eye=0;eye<2;++eye){const auto& v=row.reticle_plan.viewports[eye];const auto& clip=row.reticle_plan.scissors[eye];
            fprintf(status,"ui_reticle_late_direction_row bank=late nonce=%llu kind_mask=%u row=%u ticket=%llu eye=%u viewport=%.9g,%.9g,%.9g,%.9g scissor=%.9g,%.9g,%.9g,%.9g applied=1\n",
                nonce,c.kind_mask,i,row.ticket,eye,v.x,v.y,v.width,v.height,clip.x,clip.y,clip.width,clip.height);}
    }
}
#else
bool configure(bool requested){enabled=false;return !requested;}
void poll(wchar_t const*,FILE*){}
void complete(std::uint64_t,bool){}
void print(FILE* status){fprintf(status,"ui_reticle_late_sample compiled=0 enabled=0 normal_quota_unchanged=1 rendering_modified=0\n");}
#endif
}
