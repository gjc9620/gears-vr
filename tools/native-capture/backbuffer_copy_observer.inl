// CPU CopyTextureRegion observation before the existing engine-global/consumer
// filters. All COM calls use current API arguments, never saved resource values.
#include "backbuffer_copy_diagnostics.h"
struct BackbufferCopyPayload {
    D3D12_RESOURCE_DESC source_desc{},destination_desc{};
    uintptr_t source_interface{},destination_interface{},raw_list{},queue{};
    gears_frame_link::ListRef list{};uint64_t candidate_ticket{},candidate_inventory{},execute_order{};
    long long candidate_qpc{};unsigned execute_member{};
};
gears_backbuffer_copy::Recorder<BackbufferCopyPayload> backbuffer_copies;
uint64_t backbuffer_copy_total{},backbuffer_copy_unmatched{},backbuffer_copy_invalid{};
void observe_backbuffer_copy_execute(uintptr_t queue,gears_frame_link::ListRef ref,uint64_t order,unsigned member){
    // Same frame lock and pre-original Execute observation as the late recorder.
    std::lock_guard<FrameHookMutex> diagnostic_guard(frame_hook_mutex);
    if(!ref.identity||!ref.generation)return;
    for(unsigned i=0;i<backbuffer_copies.size;++i){auto& p=backbuffer_copies.rows[i].payload;
        if(!p.execute_order&&p.list.identity==ref.identity&&p.list.generation==ref.generation){
            p.queue=queue;p.execute_order=order;p.execute_member=member;}}
}
void observe_backbuffer_copy(ID3D12GraphicsCommandList* list,const D3D12_TEXTURE_COPY_LOCATION* destination,
    UINT x,UINT y,UINT z,const D3D12_TEXTURE_COPY_LOCATION* source,const D3D12_BOX* box,uintptr_t caller){
    if(self_testing||!frame_link_diagnostics||!recording.load())return;
    // frame_lock is conditional; explicitly serialize even across flag races.
    std::lock_guard<FrameHookMutex> diagnostic_guard(frame_hook_mutex);++backbuffer_copy_total;
    D3D12_TEXTURE_COPY_LOCATION src{},dst{};D3D12_BOX bounds{};gears_backbuffer_copy::Key key{};
    if(!safe_copy(&src,source,sizeof(src))||!safe_copy(&dst,destination,sizeof(dst))||
       (box&&!safe_copy(&bounds,box,sizeof(bounds)))||
       !gears_backbuffer_copy::decode(src,dst,x,y,z,box?&bounds:nullptr,key)){++backbuffer_copy_invalid;return;}
    key.source=canonical_identity(src.pResource);key.destination=canonical_identity(dst.pResource);key.caller=caller;
    if(!key.source||!key.destination){++backbuffer_copy_invalid;return;}
    std::array<uintptr_t,gears_final_eye_diagnostics::Recorder::capacity> rejected{};
    uint64_t candidate_ticket{},candidate_inventory{};long long candidate_qpc{};
    const auto world=committed_world_generation.load();
    for(unsigned i=0;i<final_eye_failures.size;++i){const auto& failure=final_eye_failures.rows[i];
        if(!failure.geometry.quad.ticket)continue;
        rejected[i]=failure.geometry.resource;
        if(failure.epoch.world==world&&failure.last_qpc>=candidate_qpc){
            candidate_ticket=failure.last_ticket;candidate_inventory=failure.epoch.inventory;candidate_qpc=failure.last_qpc;}}
    key.source_match=gears_backbuffer_copy::match_identity(key.source,swapchain_bindings.known_buffer(key.source),rejected.data(),final_eye_failures.size);
    key.destination_match=gears_backbuffer_copy::match_identity(key.destination,swapchain_bindings.known_buffer(key.destination),rejected.data(),final_eye_failures.size);
    if(!key.source_match&&!key.destination_match){++backbuffer_copy_unmatched;return;}
    auto* row=backbuffer_copies.admit({world,final_eye_inventory.epoch,final_eye_inventory.chain},key,candidate_ticket,now());if(!row)return;
    auto& p=row->payload;p.source_desc=src.pResource->GetDesc();p.destination_desc=dst.pResource->GetDesc();
    p.source_interface=reinterpret_cast<uintptr_t>(src.pResource);p.destination_interface=reinterpret_cast<uintptr_t>(dst.pResource);
    p.raw_list=reinterpret_cast<uintptr_t>(list);p.list=frame_link.current(canonical_identity(list));
    p.candidate_ticket=candidate_ticket;p.candidate_inventory=candidate_inventory;p.candidate_qpc=candidate_qpc;
}
void print_backbuffer_copies(FILE* file){
    fprintf(file,"backbuffer_copy_summary total=%llu unmatched=%llu invalid=%llu rows=%u limit=32 per_epoch_limit=8 dropped=%llu match_bits=current_known_buffer:1,rejected_positive_target:2 desc_source=live_api_arguments_new_rows_only candidate_context_not_proof=1 execute_source=cpu_pre_original submission_completed=0 pixels_verified=0\n",
        backbuffer_copy_total,backbuffer_copy_unmatched,backbuffer_copy_invalid,backbuffer_copies.size,backbuffer_copies.dropped);
    for(unsigned i=0;i<backbuffer_copies.size;++i){const auto& r=backbuffer_copies.rows[i];const auto& k=r.key;const auto& p=r.payload;
        fprintf(file,"backbuffer_copy row=%u world=%llu inventory_epoch=%llu chain=%llx count=%llu qpc=%lld,%lld caller=%llx source=%llx destination=%llx source_match=%u destination_match=%u dst_xyz=%u,%u,%u box_null=%u box=%u,%u,%u,%u,%u,%u list=%llx list_identity=%llx list_generation=%llu candidate_ticket=%llu last_candidate_ticket=%llu candidate_inventory=%llu candidate_qpc=%lld queue=%llx execute_order=%llu execute_member=%u\n",
            i,r.epoch.world,r.epoch.inventory,static_cast<unsigned long long>(r.epoch.chain),r.count,r.first_qpc,r.last_qpc,static_cast<unsigned long long>(k.caller),
            static_cast<unsigned long long>(k.source),static_cast<unsigned long long>(k.destination),k.source_match,k.destination_match,k.xyz[0],k.xyz[1],k.xyz[2],unsigned(!k.has_box),
            k.box[0],k.box[1],k.box[2],k.box[3],k.box[4],k.box[5],static_cast<unsigned long long>(p.raw_list),static_cast<unsigned long long>(p.list.identity),p.list.generation,
            p.candidate_ticket,r.last_candidate_ticket,p.candidate_inventory,p.candidate_qpc,static_cast<unsigned long long>(p.queue),p.execute_order,p.execute_member);
        const gears_backbuffer_copy::Location* locations[]{&k.src,&k.dst};
        const D3D12_RESOURCE_DESC* descriptions[]{&p.source_desc,&p.destination_desc};
        const uintptr_t interfaces[]{p.source_interface,p.destination_interface};
        for(unsigned j=0;j<2;++j){const auto& l=*locations[j];const auto& d=*descriptions[j];
            fprintf(file,"backbuffer_copy_resource row=%u role=%s interface=%llx copy_type=%u subresource=%u footprint_offset=%llu footprint=%u,%u,%u,%u,%u dimension=%u alignment=%llu width=%llu height=%u depth=%u mips=%u format=%u samples=%u quality=%u layout=%u flags=%u\n",
                i,j?"destination":"source",static_cast<unsigned long long>(interfaces[j]),l.type,l.subresource,l.offset,l.footprint[0],l.footprint[1],l.footprint[2],l.footprint[3],l.footprint[4],
                unsigned(d.Dimension),d.Alignment,d.Width,d.Height,unsigned(d.DepthOrArraySize),unsigned(d.MipLevels),unsigned(d.Format),d.SampleDesc.Count,d.SampleDesc.Quality,unsigned(d.Layout),unsigned(d.Flags));}
    }
}
