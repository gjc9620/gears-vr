// Shares only existing quad/Draw/Execute observation sites. No output policy.
#if defined(GEARS_UNPOSSESSED_QUAD_OBSERVER) && GEARS_UNPOSSESSED_QUAD_OBSERVER
#include "unpossessed_quad_observer.h"
struct UnpossessedQuadPayload {
    std::array<ResourceIdentity,4> sources{};uintptr_t queue{};uint64_t execute_order{};unsigned execute_member{};
};
gears_unpossessed_quad::Recorder<UnpossessedQuadPayload> unpossessed_quads;
void observe_unpossessed_ticket(){
    if(!frame_link_diagnostics||!recording.load()||!consuming_frozen_frame.enabled||!consuming_frozen_frame.unpossessed)return;
    std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);
    unpossessed_quads.select(consuming_frozen_frame.unpossessed,{committed_world_generation.load(),consuming_frame_ticket});
}
auto* current_unpossessed_quad_bank(){
    // All callers hold frame_hook_mutex. Selection happens at consume entry,
    // so tickets with no quad do not shift the delayed ticket's ordinal.
    return consuming_frozen_frame.enabled&&consuming_frozen_frame.unpossessed&&
        consuming_frame_ticket==consuming_frozen_frame.id?
        unpossessed_quads.find({committed_world_generation.load(),consuming_frame_ticket}):nullptr;
}
void observe_unpossessed_quad_begin(){
    if(!late_quad_candidate.active||!recording.load()||!consuming_frozen_frame.unpossessed)return;
    std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);
    if(auto* bank=current_unpossessed_quad_bank())++bank->quad_calls;
}
void observe_unpossessed_quad_draw(ID3D12GraphicsCommandList* list,uintptr_t caller,unsigned kind,unsigned elements,unsigned instances){
    if(!late_quad_candidate.active||!frame_link_diagnostics||!recording.load()||!consuming_frozen_frame.unpossessed)return;
    std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);
    auto* bank=current_unpossessed_quad_bank();if(!bank)return;
    gears_unpossessed_quad::Key key{};auto& k=key.call;
    k.args=late_quad_candidate.args;k.dimensions=late_quad_candidate.dimensions;k.quad_caller=late_quad_candidate.caller;
    key.final_quad_eye_valid=final_eye.valid;key.final_quad_eye=final_eye.eye;key.without_draw=!list;
    if(list){
        key.raw_list=reinterpret_cast<uintptr_t>(list);
        const auto ref=frame_link.current(canonical_identity(list));key.list_identity=ref.identity;key.list_generation=ref.generation;
        const auto binding=frame_target_stats.binding(key.raw_list);
        k.draw_caller=caller>=base?caller-base:caller;k.kind=kind;k.elements=elements;k.instances=instances;
        k.target=binding.target;k.targets=binding.targets;k.rectangles=frame_target_stats.draw_state(key.raw_list);
        k.known_buffer=swapchain_bindings.known_buffer(k.target.resource);
        k.descriptor_capacity_lost=frame_target_stats.descriptor_capacity_lost();
        // Same guarded TLS/slot layout as late_quad. These are current engine
        // binding hints, not verified D3D12 root-descriptor or GPU state.
        uintptr_t tls{};unsigned char count{};
        if(safe_copy(&tls,reinterpret_cast<void*>(__readgsqword(0x58)),8)&&tls&&
           safe_copy(&k.context,reinterpret_cast<void*>(tls+0x20),8)&&k.context){
            k.read_mask|=1;
            if(safe_copy(&count,reinterpret_cast<void*>(k.context+0x65d),1)){
                k.read_mask|=2;k.slot_count=count;
                const auto slots=std::min(unsigned(count),unsigned(k.objects.size()));
                if(!slots||safe_copy(k.objects.data(),reinterpret_cast<void*>(k.context+0x768),slots*8))k.read_mask|=4;
            }
        }
        // Separate native current-view evidence from final-quad geometry.
        // Reuse the existing geometry observer's offsets, with exact count=2.
        auto& n=key.native_view;n.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);
        if((k.read_mask&1)&&safe_copy(&n.current_view,reinterpret_cast<void*>(k.context+0x14c0),8))n.read_mask|=1;
        if(n.renderer){
            if(safe_copy(&n.renderer_views,reinterpret_cast<void*>(n.renderer+0x68),8))n.read_mask|=2;
            if(safe_copy(&n.renderer_count,reinterpret_cast<void*>(n.renderer+0x70),4))n.read_mask|=4;
        }
        n.eye_valid=n.read_mask==7&&gears_unpossessed_quad::native_eye_index(n.current_view,n.renderer_views,n.renderer_count,n.eye);
    }else ++bank->without_draw;
    auto* row=bank->admit(key,now());if(!row)return;
    // At most four guarded queries per new row, 256 rows for the entire run.
    if((k.read_mask&7)==7)for(unsigned s=0;s<std::min(k.slot_count,4u);++s)
        query_bound_resource(k.objects[s],row->payload.sources[s]);
}
void observe_unpossessed_quad_execute(uintptr_t queue,gears_frame_link::ListRef ref,uint64_t order,unsigned member){
    if(!ref.identity||!ref.generation)return;
    std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);
    for(unsigned b=0;b<unpossessed_quads.size;++b){auto& bank=unpossessed_quads.banks[b];
        for(unsigned i=0;i<bank.size;++i){auto& r=bank.rows[i];auto& p=r.payload;
            if(!p.execute_order&&r.key.list_identity==ref.identity&&r.key.list_generation==ref.generation){
                p.queue=queue;p.execute_order=order;p.execute_member=member;}}}
}
void print_unpossessed_quads(FILE* file){
    std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);
    fprintf(file,"unpossessed_quad_summary enabled=1 cpu_only=1 pixels_verified=0 admission=frozen_unpossessed_consumed_ticket selection=first3_and_49th consume_order=1 distinct_capped=%u selection_closed=%u banks=%u bank_limit=4 rows_per_bank=64 source_slots_limit=32 source_hints_limit=4 source_queries=new_rows_only engine_binding_hints=1 native_view_read_bits=current_view:1,renderer_views:2,renderer_count:4 native_view_stride=0xd30 native_view_required_count=2 final_quad_eye_source=existing_classification execute_source=cpu_pre_original execute_association=first_observed_matching_generation submission_completed=0\n",
        unpossessed_quads.distinct,unsigned(unpossessed_quads.distinct==49),unpossessed_quads.size);
    for(unsigned b=0;b<unpossessed_quads.size;++b){const auto& bank=unpossessed_quads.banks[b];
        fprintf(file,"unpossessed_quad_bank bank=%u ordinal=%u ticket=%llu world=%llu quad_calls=%llu without_draw=%llu rows=%u dropped_shapes=%llu without_draw_means=no_intercepted_draw_in_quad\n",
            b,bank.ordinal,bank.ticket.id,bank.ticket.world,bank.quad_calls,bank.without_draw,bank.size,bank.dropped);
        for(unsigned i=0;i<bank.size;++i){const auto& r=bank.rows[i];const auto& k=r.key.call;const auto& p=r.payload;const auto& d=k.rectangles;
            const auto& n=r.key.native_view;
            fprintf(file,"unpossessed_quad bank=%u row=%u count=%llu first_qpc=%lld last_qpc=%lld without_draw=%u final_quad_eye_valid=%u final_quad_eye=%u native_current_view=%llx renderer=%llx renderer_views=%llx renderer_count=%d native_view_read_mask=%u native_eye_valid=%u native_eye=%u quad_caller=%llx args=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g dimensions=%u,%u,%u,%u draw_caller=%llx kind=%u elements=%u instances=%u list=%llx list_identity=%llx list_generation=%llu rtv_resource=%llx rtv_size=%llu,%u format=%u mip=%u targets=%u known_buffer=%u descriptor_capacity_lost=%u tracked=%u viewport_count=%u scissor_count=%u viewport=%.9g,%.9g,%.9g,%.9g scissor=%.9g,%.9g,%.9g,%.9g depth_known=%u depth=%.9g,%.9g queue=%llx execute_order=%llu execute_member=%u context=%llx read_mask=%u slot_count=%u bound_objects=",
                b,i,r.count,r.first_qpc,r.last_qpc,unsigned(r.key.without_draw),unsigned(r.key.final_quad_eye_valid),r.key.final_quad_eye,
                static_cast<unsigned long long>(n.current_view),static_cast<unsigned long long>(n.renderer),static_cast<unsigned long long>(n.renderer_views),n.renderer_count,n.read_mask,unsigned(n.eye_valid),n.eye,
                static_cast<unsigned long long>(k.quad_caller),k.args[0],k.args[1],k.args[2],k.args[3],k.args[4],k.args[5],k.args[6],k.args[7],
                k.dimensions[0],k.dimensions[1],k.dimensions[2],k.dimensions[3],static_cast<unsigned long long>(k.draw_caller),k.kind,k.elements,k.instances,
                static_cast<unsigned long long>(r.key.raw_list),static_cast<unsigned long long>(r.key.list_identity),r.key.list_generation,
                static_cast<unsigned long long>(k.target.resource),k.target.width,k.target.height,k.target.format,k.target.mip,k.targets,
                unsigned(k.known_buffer),unsigned(k.descriptor_capacity_lost),unsigned(d.tracked),d.viewport_count,d.scissor_count,
                d.viewport.x,d.viewport.y,d.viewport.width,d.viewport.height,d.scissor.x,d.scissor.y,d.scissor.width,d.scissor.height,
                unsigned(d.depth_known),d.min_depth,d.max_depth,static_cast<unsigned long long>(p.queue),p.execute_order,p.execute_member,
                static_cast<unsigned long long>(k.context),k.read_mask,k.slot_count);
            for(unsigned s=0;s<std::min(k.slot_count,32u);++s)fprintf(file,"%s%llx",s?",":"",static_cast<unsigned long long>(k.objects[s]));
            fprintf(file,"\n");
            for(unsigned s=0;s<std::min(k.slot_count,4u);++s){const auto& v=p.sources[s];const auto& desc=v.desc;
                fprintf(file,"unpossessed_quad_source bank=%u row=%u slot=%u object=%llx native=%llx resource=%llx canonical=%llx hr=%08lx dimension=%u width=%llu height=%u depth=%u mips=%u format=%u samples=%u quality=%u flags=%u wrapper=%llx state_read=%u engine_state=%u engine_flags=%u\n",
                    b,i,s,static_cast<unsigned long long>(k.objects[s]),static_cast<unsigned long long>(v.native),static_cast<unsigned long long>(v.resource),static_cast<unsigned long long>(v.canonical),
                    static_cast<unsigned long>(v.result),unsigned(desc.Dimension),desc.Width,desc.Height,unsigned(desc.DepthOrArraySize),unsigned(desc.MipLevels),unsigned(desc.Format),desc.SampleDesc.Count,desc.SampleDesc.Quality,unsigned(desc.Flags),
                    static_cast<unsigned long long>(v.engine_wrapper),unsigned(v.engine_state_read),v.engine_state,unsigned(v.engine_flags));}
        }
    }
}
#else
void observe_unpossessed_ticket(){}
void observe_unpossessed_quad_begin(){}
void observe_unpossessed_quad_draw(ID3D12GraphicsCommandList*,uintptr_t,unsigned,unsigned,unsigned){}
void observe_unpossessed_quad_execute(uintptr_t,gears_frame_link::ListRef,uint64_t,unsigned){}
void print_unpossessed_quads(FILE* file){fprintf(file,"unpossessed_quad_summary enabled=0 cpu_only=1\n");}
#endif
