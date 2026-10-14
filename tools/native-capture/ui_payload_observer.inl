// Read-only candidate transport observation. Included by ui_observer_hooks.inl.
// A content match is not an allocator identity or an indexed-draw membership proof.
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
struct UiIndexContext {
    uint64_t execute_sequence{};
    gears_ui_indices::Snapshot snapshot{};
    gears_ui_indices::Cursor cursor{};
    unsigned ordinal{};
    bool callback_valid{};
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
};
std::array<std::array<UiPayloadGpuRow,24>,8> ui_payload_gpu_rows{};
std::array<unsigned,8> ui_payload_gpu_counts{};
std::array<long long,8> ui_payload_next_row{};
std::array<uint64_t,8> ui_payload_gpu_calls{};

bool validate_ui_payload_content(gears_ui_receipts::Claim const& claim,UiPayloadContext& context){
    gears_ui_batch::detail::Reader<decltype(&safe_copy)> read{safe_copy};
    gears_ui_batch::detail::Array vertices{},after{};
    if(!claim.count||claim.count>claim.entries.size()||!read.field(vertices,claim.payload,0x48)||
       !gears_ui_batch::detail::valid(vertices,1048576,gears_ui_batch::vertex_stride))return false;
    unsigned covered=0;
    for(size_t i=0;i<claim.count;++i){auto const& entry=claim.entries[i];auto const& receipt=entry.receipt;
        if(entry.kind<1||entry.kind>3||receipt.end>unsigned(vertices.count)||receipt.begin>receipt.end||receipt.end-receipt.begin!=4)return false;
        std::array<gears_ui_batch::Vertex,4> copied{};
        for(unsigned v=0;v<copied.size();++v){uintptr_t address{};
            if(!gears_ui_batch::detail::add(vertices.data,size_t(receipt.begin+v)*gears_ui_batch::vertex_stride,address)||
               !read.bytes(copied[v].data(),address,copied[v].size()))return false;
        }
        if(copied!=receipt.vertices)return false;
        covered+=4;context.kind_vertices[entry.kind-1]+=4;context.kind_mask|=1u<<(entry.kind-1);
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
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    Scope active_scope;UiPayloadRestore restore;ui_payload_context=nullptr;ui_index_context=nullptr;ui_execute_entry_return=0;
    UiPayloadContext context{};
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
                if(recording.load()&&validate_ui_payload_content(claim,context)&&
                   safe_copy(checked.data(),command,checked.size())&&checked==header){
                    context.command=reinterpret_cast<uintptr_t>(command);
                    ++ui_execute_content_matches;ui_payload_context=&context;
                    index_storage.reset(new(std::nothrow) UiIndexContext{});
                    if(index_storage&&caller==base+0x19f2f3&&
                       gears_ui_indices::capture(safe_copy,payload,claim,index_storage->snapshot)&&
                       safe_copy(checked.data(),command,checked.size())&&checked==header){
                        index_storage->execute_sequence=++ui_execute_sequence;
                        index_storage->cursor=gears_ui_indices::Cursor(index_storage->snapshot);
                        ui_index_context=index_storage.get();++ui_index_capture_ok;
                    }else ++ui_index_capture_rejected;
                }else ++ui_execute_content_rejected;
            }
        }
    }
    // Original owns/destructs the payload. No command/payload reads after return.
    const uint32_t result=ui_original_execute(command);
    if(ui_observer_enabled&&result!=48)++ui_execute_bad_return;
    // Only our independent value copies remain live. The native command and
    // payload have already been destroyed and must not be read here.
    if(ui_index_context){
        const bool complete=ui_index_context->cursor.finish()&&result==48;
        if(complete)++ui_index_complete;else ++ui_index_incomplete;
        std::lock_guard<std::mutex> lock(ui_payload_mutex);
        for(unsigned k=1;k<ui_payload_gpu_rows.size();++k)
            for(unsigned i=0;i<ui_payload_gpu_counts[k];++i){auto& row=ui_payload_gpu_rows[k][i];
                if(row.execute_sequence==ui_index_context->execute_sequence)row.command_completed=complete;}
    }
    return result;
}

void observe_ui_index_callback(uintptr_t caller,gears_ui_indices::DrawArgs args){
    if(!ui_index_context)return;
    ui_index_context->callback_valid=false;
    if(caller!=base+0x172741)return;
    unsigned ordinal{};
    if(ui_execute_entry_return==base+0x19f2f3&&ui_index_context->cursor.consume(args,ordinal)){
        ui_index_context->ordinal=ordinal;ui_index_context->callback_valid=true;++ui_index_callback_ok;
    }else ++ui_index_callback_rejected;
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
        unsigned indices,unsigned instances,unsigned start,int vertex){
    if(!ui_observer_enabled||!recording.load())return;
    ++ui_gpu_context_calls;
    if(!ui_payload_context||!ticket){++ui_gpu_context_missing;return;}
    const auto& context=*ui_payload_context;const auto kind=context.kind_mask;
    if(!kind||kind>=ui_payload_gpu_rows.size()){++ui_gpu_context_missing;return;}
    const auto timestamp=now();std::lock_guard<std::mutex> lock(ui_payload_mutex);
    ++ui_payload_gpu_calls[kind];auto& count=ui_payload_gpu_counts[kind];
    if(count>=ui_payload_gpu_rows[kind].size()||(count>=8&&timestamp<ui_payload_next_row[kind]))return;
    ui_payload_next_row[kind]=timestamp+qpc_frequency;
    auto& row=ui_payload_gpu_rows[kind][count++];
    row={context,ticket,list,target,timestamp,GetCurrentThreadId(),indices,instances,start,vertex};
    row.geometry_matched=stereo_ticket_store.lookup_geometry(ticket,row.geometry,simulator_operator);
    if(ui_index_context&&ui_index_context->callback_valid){
        row.execute_sequence=ui_index_context->execute_sequence;
        row.segment_ordinal=ui_index_context->ordinal;
        row.segment=ui_index_context->snapshot.segments[row.segment_ordinal];
        row.index_callback_matched=true;
    }
}

void print_ui_payload_observer(FILE* status){
    const auto store=ui_payload_store.stats();
    fprintf(status,"ui_payload_store published=%llu rejected=%llu capacity_rejected=%llu groups_poisoned=%llu claimed=%llu poison_claimed=%llu active_groups=%zu poisoned_groups=%zu disabled=%u resets=%llu\n",
        store.published,store.rejected,store.capacity_rejected,store.groups_poisoned,store.claimed,store.poison_claimed,
        store.active_groups,store.poisoned_groups,store.disabled?1:0,store.resets);
    fprintf(status,"ui_batch attempts=%llu before_rejected=%llu append_rejected=%llu published=%llu publish_rejected=%llu coordinates_modified=0\n",
        ui_batch_attempts.load(),ui_batch_before_rejected.load(),ui_batch_append_rejected.load(),ui_batch_published.load(),ui_batch_publish_rejected.load());
    fprintf(status,"ui_payload_execute calls=%llu header_rejected=%llu claims=%llu content_rejected=%llu content_matches=%llu bad_return=%llu gpu_calls=%llu gpu_missing_context=%llu allocation_identity_proven=0 index_membership_requires_per_row=1\n",
        ui_execute_calls.load(),ui_execute_header_rejected.load(),ui_execute_claims.load(),ui_execute_content_rejected.load(),
        ui_execute_content_matches.load(),ui_execute_bad_return.load(),ui_gpu_context_calls.load(),ui_gpu_context_missing.load());
    fprintf(status,"ui_index_execute capture_ok=%llu capture_rejected=%llu callbacks_ok=%llu callbacks_rejected=%llu completed=%llu incomplete=%llu coordinates_modified=0\n",
        ui_index_capture_ok.load(),ui_index_capture_rejected.load(),ui_index_callback_ok.load(),ui_index_callback_rejected.load(),
        ui_index_complete.load(),ui_index_incomplete.load());
    for(unsigned k=1;k<ui_payload_gpu_rows.size();++k){
        fprintf(status,"ui_payload_gpu kind_mask=%u calls=%llu rows=%u\n",k,ui_payload_gpu_calls[k],ui_payload_gpu_counts[k]);
        for(unsigned i=0;i<ui_payload_gpu_counts[k];++i){auto const& r=ui_payload_gpu_rows[k][i];auto const& c=r.context;
            fprintf(status,"ui_payload_gpu_row kind_mask=%u row=%u command=%p payload=%p candidate_generation=%llu root_first=%llu root_last=%llu vertices=%u receipts=%u reticle_vertices=%u weapon_vertices=%u hud_vertices=%u unknown_vertices=%llu ticket=%llu list=%p list_generation=%llu target=%p target_width=%u target_height=%u indices=%u instances=%u start_index=%u base_vertex=%d qpc=%lld thread=%u content_matched_at_execute=1 coordinates_modified=0\n",
                k,i,reinterpret_cast<void*>(c.command),reinterpret_cast<void*>(c.payload),c.generation,c.root_first,c.root_last,c.vertex_count,c.receipts,
                c.kind_vertices[0],c.kind_vertices[1],c.kind_vertices[2],c.unknown_vertices,r.ticket,reinterpret_cast<void*>(r.list.identity),r.list.generation,
                reinterpret_cast<void*>(r.target.resource),r.target.width,r.target.height,r.indices,r.instances,r.start,r.vertex,r.qpc,r.thread);
            if(r.index_callback_matched){auto const& s=r.segment;
                fprintf(status,"ui_index_gpu_row kind_mask=%u row=%u execute_sequence=%llu ticket=%llu ordinal=%u min_vertex=%d max_vertex=%d indices=%u reticle_indices=%u weapon_indices=%u hud_indices=%u unknown_indices=%u actual_kind_mask=%u entry_mask=%u callback_matched=1 command_completed=%u geometry_matched=%u allocation_identity_proven=0 coordinates_modified=0\n",
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
        }
    }
}
