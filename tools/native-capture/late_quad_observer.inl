// Included after resource_identity_observer.h. CPU observations inside any quad
// reaching a current backbuffer without classified final-eye/ticket context.
// Historical 2b11a0 and live d83cbf are diagnostic exceptions, not output attestations.
#include "late_quad_diagnostics.h"
struct LateQuadCandidate {
    bool active{};std::array<float,8> args{};std::array<unsigned,4> dimensions{};uintptr_t caller{};unsigned draws{};
};
thread_local LateQuadCandidate late_quad_candidate{};
struct LateQuadPayload {
    std::array<ResourceIdentity,4> sources{};
    gears_frame_link::ListRef list{};uintptr_t raw_list{},queue{};
    uint64_t ticket{},execute_order{};unsigned execute_member{};
    long long inventory_qpc{};unsigned present_width{},present_height{},present_count{};bool present_valid{};
};
gears_late_quad::Recorder<LateQuadPayload> late_quads;
uint64_t late_quad_calls{},late_quad_without_draw{};
void observe_late_execute(uintptr_t queue,gears_frame_link::ListRef ref,uint64_t order,unsigned member){
    // Called inside the existing frame lock, after its bounded member copy.
    // This is CPU pre-original Execute observation, not completed submission.
    if(!ref.identity||!ref.generation)return;
    for(unsigned i=0;i<late_quads.size;++i){auto& s=late_quads.rows[i].payload;
        if(!s.execute_order&&s.list.identity==ref.identity&&s.list.generation==ref.generation){
            s.queue=queue;s.execute_order=order;s.execute_member=member;}}
}
void observe_late_draw(ID3D12GraphicsCommandList* list,uintptr_t caller,unsigned kind,unsigned elements,unsigned instances){
    if(!late_quad_candidate.active||!frame_link_diagnostics||!recording.load())return;
    // frame_lock is conditional: recording may have changed after it checked.
    // Always acquire here, as the final-eye diagnostic recorder does.
    std::lock_guard<FrameHookMutex> diagnostic_guard(frame_hook_mutex);
    ++late_quad_candidate.draws;
    const auto raw=reinterpret_cast<uintptr_t>(list);const auto binding=frame_target_stats.binding(raw);
    const bool known=swapchain_bindings.known_buffer(binding.target.resource);
    if(!gears_late_quad::observe_target(late_quad_candidate.caller,known,final_eye.valid,consuming_frame_ticket))return;
    gears_late_quad::Key key{};key.args=late_quad_candidate.args;key.dimensions=late_quad_candidate.dimensions;
    key.quad_caller=late_quad_candidate.caller;
    key.draw_caller=caller>=base?caller-base:caller;key.kind=kind;key.elements=elements;key.instances=instances;
    key.target=binding.target;key.targets=binding.targets;key.rectangles=frame_target_stats.draw_state(raw);
    key.known_buffer=known;
    key.descriptor_capacity_lost=frame_target_stats.descriptor_capacity_lost();
    // Fresh same-call TLS reads, including outside consuming_renderer scope.
    uintptr_t tls{};unsigned char count{};
    if(safe_copy(&tls,reinterpret_cast<void*>(__readgsqword(0x58)),8)&&tls&&
       safe_copy(&key.context,reinterpret_cast<void*>(tls+0x20),8)&&key.context){
        key.read_mask|=1;
        if(safe_copy(&count,reinterpret_cast<void*>(key.context+0x65d),1)){
            key.read_mask|=2;key.slot_count=count;
            const auto slots=std::min(unsigned(count),unsigned(key.objects.size()));
            if(!slots||safe_copy(key.objects.data(),reinterpret_cast<void*>(key.context+0x768),slots*8))key.read_mask|=4;
        }
    }
    const gears_late_quad::Epoch epoch{committed_world_generation.load(),final_eye_inventory.epoch,final_eye_inventory.chain};
    auto* row=late_quads.admit(epoch,key,consuming_frame_ticket,now());if(!row)return;
    auto& p=row->payload;p.raw_list=raw;p.list=frame_link.current(canonical_identity(list));p.ticket=consuming_frame_ticket;
    p.inventory_qpc=final_eye_inventory.qpc;p.present_width=final_eye_inventory.width;p.present_height=final_eye_inventory.height;
    p.present_count=final_eye_inventory.count;p.present_valid=final_eye_inventory.valid;
    // Only newly admitted rows issue COM queries (at most 8 per epoch/32 total).
    // Resources remain owned by the engine; queries release every interface.
    if((key.read_mask&7)==7)for(unsigned slot=0;slot<std::min(key.slot_count,4u);++slot)
        query_bound_resource(key.objects[slot],p.sources[slot]);
}
void print_late_quad(FILE* file){
    fprintf(file,"late_quad_summary scope=all_hook_quads calls=%llu without_draw=%llu without_draw_means=no_intercepted_draw_in_quad admission=current_known_buffer_without_classified_final_eye_ticket historical_caller_exception=2b11a0 live_caller_exception=d83cbf rows=%u limit=32 per_epoch_limit=8 dropped=%llu cpu_only=1 pixels_verified=0 resource_queries=new_rows_only execute_source=cpu_pre_original submission_completed=0\n",
        late_quad_calls,late_quad_without_draw,late_quads.size,late_quads.dropped);
    for(unsigned i=0;i<late_quads.size;++i){const auto& r=late_quads.rows[i];const auto& k=r.key;const auto& p=r.payload;
        fprintf(file,"late_quad row=%u world=%llu inventory_epoch=%llu chain=%llx count=%llu first_qpc=%lld last_qpc=%lld ticket=%llu last_ticket=%llu quad_caller=%llx args=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g dimensions=%u,%u,%u,%u context=%llx read_mask=%u slot_count=%u bound_objects=",
            i,r.epoch.world,r.epoch.inventory,static_cast<unsigned long long>(r.epoch.chain),r.count,r.first_qpc,r.last_qpc,p.ticket,r.last_ticket,static_cast<unsigned long long>(k.quad_caller),
            k.args[0],k.args[1],k.args[2],k.args[3],k.args[4],k.args[5],k.args[6],k.args[7],
            k.dimensions[0],k.dimensions[1],k.dimensions[2],k.dimensions[3],static_cast<unsigned long long>(k.context),k.read_mask,k.slot_count);
        for(unsigned s=0;s<std::min(k.slot_count,32u);++s)fprintf(file,"%s%llx",s?",":"",static_cast<unsigned long long>(k.objects[s]));
        fprintf(file,"\nlate_quad_draw row=%u caller=%llx kind=%u elements=%u instances=%u list=%llx list_identity=%llx list_generation=%llu resource=%llx width=%llu height=%u format=%u mip=%u targets=%u known_buffer=%u descriptor_capacity_lost=%u tracked=%u viewport_count=%u scissor_count=%u viewport=%.9g,%.9g,%.9g,%.9g scissor=%.9g,%.9g,%.9g,%.9g depth_known=%u depth=%.9g,%.9g queue=%llx execute_order=%llu execute_member=%u inventory_qpc=%lld present_size=%u,%u present_count=%u present_valid=%u\n",
            i,static_cast<unsigned long long>(k.draw_caller),k.kind,k.elements,k.instances,static_cast<unsigned long long>(p.raw_list),
            static_cast<unsigned long long>(p.list.identity),p.list.generation,static_cast<unsigned long long>(k.target.resource),k.target.width,
            k.target.height,k.target.format,k.target.mip,k.targets,unsigned(k.known_buffer),unsigned(k.descriptor_capacity_lost),unsigned(k.rectangles.tracked),
            k.rectangles.viewport_count,k.rectangles.scissor_count,k.rectangles.viewport.x,k.rectangles.viewport.y,k.rectangles.viewport.width,k.rectangles.viewport.height,
            k.rectangles.scissor.x,k.rectangles.scissor.y,k.rectangles.scissor.width,k.rectangles.scissor.height,unsigned(k.rectangles.depth_known),k.rectangles.min_depth,k.rectangles.max_depth,
            static_cast<unsigned long long>(p.queue),p.execute_order,p.execute_member,p.inventory_qpc,p.present_width,p.present_height,p.present_count,unsigned(p.present_valid));
        for(unsigned s=0;s<std::min(k.slot_count,4u);++s){const auto& v=p.sources[s];const auto& d=v.desc;
            fprintf(file,"late_quad_source row=%u slot=%u object=%llx native=%llx resource=%llx canonical=%llx hr=%08lx dimension=%u width=%llu height=%u depth=%u mips=%u format=%u samples=%u quality=%u flags=%u wrapper=%llx state_read=%u engine_state=%u engine_flags=%u\n",
                i,s,static_cast<unsigned long long>(k.objects[s]),static_cast<unsigned long long>(v.native),static_cast<unsigned long long>(v.resource),static_cast<unsigned long long>(v.canonical),
                static_cast<unsigned long>(v.result),unsigned(d.Dimension),d.Width,d.Height,unsigned(d.DepthOrArraySize),unsigned(d.MipLevels),unsigned(d.Format),d.SampleDesc.Count,d.SampleDesc.Quality,unsigned(d.Flags),
                static_cast<unsigned long long>(v.engine_wrapper),unsigned(v.engine_state_read),v.engine_state,unsigned(v.engine_flags));}
    }
}
