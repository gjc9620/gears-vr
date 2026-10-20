#if defined(GEARS_DEATH_HDR_TIMELINE) && GEARS_DEATH_HDR_TIMELINE
#include "death_hdr_timeline.h"
#endif
namespace death_hdr_timeline {
#if defined(GEARS_DEATH_HDR_TIMELINE) && GEARS_DEATH_HDR_TIMELINE
using namespace gears_death_hdr;
std::array<Bank<ResourceIdentity>,4> banks;
Bank<ResourceIdentity>* current(){
    if(!frame_link_diagnostics||!recording.load()||!consuming_frozen_frame.unpossessed)return nullptr;
    auto* selected=current_unpossessed_quad_bank();if(!selected)return nullptr;
    const auto index=selected-unpossessed_quads.banks.data();
    return index>=0&&index<4?&banks[static_cast<unsigned>(index)]:nullptr;
}
void stamp(Event& e,ID3D12GraphicsCommandList* list,uintptr_t caller){
    e.caller=caller;
    if(list){const auto ref=frame_link.current(canonical_identity(list));
        e.list={reinterpret_cast<uintptr_t>(list),ref.identity,ref.generation};
        const auto binding=frame_target_stats.binding(e.list.raw),raw=viewport_stats.binding(e.list.raw);
        e.target=binding.target;e.target.resource=raw.target.resource;e.target_count=binding.targets;
        e.rectangles=frame_target_stats.draw_state(e.list.raw);
        e.descriptor_capacity_lost=frame_target_stats.descriptor_capacity_lost();
    }
    auto& v=e.view;v.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);
    uintptr_t tls{},context{};
    if(safe_copy(&tls,reinterpret_cast<void*>(__readgsqword(0x58)),8)&&tls&&
       safe_copy(&context,reinterpret_cast<void*>(tls+0x20),8)&&context&&
       safe_copy(&v.current_view,reinterpret_cast<void*>(context+0x14c0),8))v.read_mask|=1;
    if(v.renderer){
        if(safe_copy(&v.renderer_views,reinterpret_cast<void*>(v.renderer+0x68),8))v.read_mask|=2;
        if(safe_copy(&v.renderer_count,reinterpret_cast<void*>(v.renderer+0x70),4))v.read_mask|=4;
    }
    v.eye_valid=v.read_mask==7&&gears_unpossessed_quad::native_eye_index(v.current_view,v.renderer_views,v.renderer_count,v.eye);
}
void query(Bank<ResourceIdentity>& bank,uintptr_t raw){
    bank.identities.query(raw,[](uintptr_t pointer,ResourceIdentity& out){query_native_resource(pointer,out);});
}
uintptr_t canonical(Bank<ResourceIdentity> const& bank,uintptr_t raw){
    const auto* identity=bank.identities.find(raw);
    return identity&&SUCCEEDED(identity->result)?identity->canonical:0;
}
void sample_source(Bank<ResourceIdentity>& bank,Row& row){
    auto& e=row.event;uintptr_t tls{},context{},object_table{},wrapper{},wrapper_table{},native{};unsigned char slots{};
    if(safe_copy(&tls,reinterpret_cast<void*>(__readgsqword(0x58)),8)&&tls&&
       safe_copy(&context,reinterpret_cast<void*>(tls+0x20),8)&&context&&
       safe_copy(&slots,reinterpret_cast<void*>(context+0x65d),1)&&slots>=1&&slots<=32&&
       safe_copy(&e.source_object,reinterpret_cast<void*>(context+0x768),8)){
        e.read_mask|=1;e.slots=slots;
        if(e.source_object&&safe_copy(&object_table,reinterpret_cast<void*>(e.source_object),8)&&object_table==base+0x1667bb8&&
           safe_copy(&wrapper,reinterpret_cast<void*>(e.source_object+0x60),8)&&wrapper&&
           safe_copy(&wrapper_table,reinterpret_cast<void*>(wrapper),8)&&wrapper_table==base+0x133ecf8&&
           safe_copy(&native,reinterpret_cast<void*>(wrapper+0xc),8)&&native){
            e.read_mask|=2;e.resources[0]=native;query(bank,native);
        }
    }
    if(e.read_mask!=3)++bank.read_failures;
}
void draw(ID3D12GraphicsCommandList* list,uintptr_t caller){
    if(!recording.load()||!consuming_frozen_frame.unpossessed)return;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto* bank=current();if(!bank)return;
    const bool selected=late_quad_candidate.active&&(late_quad_candidate.caller==0x2ab6d9||late_quad_candidate.caller==0xd80a7b);
    const bool hdr=list&&frame_target_stats.binding(reinterpret_cast<uintptr_t>(list)).target.format==26;
    if(!selected&&!hdr)return;
    Event e{};stamp(e,list,caller);
    if(selected){e.kind=sample;e.flags=list?0u:1u;e.quad_caller=late_quad_candidate.caller;
        e.args=late_quad_candidate.args;e.dimensions=late_quad_candidate.dimensions;
        if(auto* row=bank->record(e,now())){query(*bank,e.target.resource);sample_source(*bank,*row);
            death_hdr_pixels::sample(row->event,canonical(*bank,row->event.target.resource));}
    }
    if(hdr){e.kind=hdr_draw;e.flags=0;e.quad_caller=0;e.args={};e.dimensions={};
        if(bank->record(e,now()))query(*bank,e.target.resource);}
}
void targets_event(ID3D12GraphicsCommandList* list,unsigned count,bool single,uintptr_t caller){
    if(!recording.load()||!consuming_frozen_frame.unpossessed)return;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto* bank=current();if(!bank)return;
    Event e{};e.kind=targets;stamp(e,list,caller);e.target_count=count;e.flags=unsigned(single);
    if(bank->record(e,now())&&e.target.format==26)query(*bank,e.target.resource);
}
Footprint footprint(D3D12_PLACED_SUBRESOURCE_FOOTPRINT const& p){
    return {p.Offset,unsigned(p.Footprint.Format),p.Footprint.Width,p.Footprint.Height,p.Footprint.Depth,p.Footprint.RowPitch};
}
void copy_event(ID3D12GraphicsCommandList* list,const D3D12_TEXTURE_COPY_LOCATION* destination,UINT x,UINT y,UINT z,
    const D3D12_TEXTURE_COPY_LOCATION* source,const D3D12_BOX* box,uintptr_t caller){
    if(!recording.load()||!consuming_frozen_frame.unpossessed)return;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto* bank=current();if(!bank)return;
    Event e{};e.kind=copy;stamp(e,list,caller);e.xyz={x,y,z};e.has_box=box!=nullptr;
    D3D12_TEXTURE_COPY_LOCATION src{},dst{};D3D12_BOX bounds{};
    if(safe_copy(&src,source,sizeof(src)))e.read_mask|=1;
    if(safe_copy(&dst,destination,sizeof(dst)))e.read_mask|=2;
    if(!box||safe_copy(&bounds,box,sizeof(bounds)))e.read_mask|=4;
    if(e.read_mask&1){e.resources[0]=reinterpret_cast<uintptr_t>(src.pResource);e.source_type=src.Type;
        if(src.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX)e.source_subresource=src.SubresourceIndex;
        else if(src.Type==D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT)e.footprints[0]=footprint(src.PlacedFootprint);
        else ++bank->read_failures;}
    if(e.read_mask&2){e.resources[1]=reinterpret_cast<uintptr_t>(dst.pResource);e.destination_type=dst.Type;
        if(dst.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX)e.destination_subresource=dst.SubresourceIndex;
        else if(dst.Type==D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT)e.footprints[1]=footprint(dst.PlacedFootprint);
        else ++bank->read_failures;}
    if(box&&(e.read_mask&4))e.box={bounds.left,bounds.top,bounds.front,bounds.right,bounds.bottom,bounds.back};
    if(e.read_mask!=7)++bank->read_failures;
    if(bank->record(e,now())){query(*bank,e.resources[0]);query(*bank,e.resources[1]);}
}
void barriers_event(ID3D12GraphicsCommandList* list,UINT count,const D3D12_RESOURCE_BARRIER* values,uintptr_t caller){
    if(!recording.load()||!consuming_frozen_frame.unpossessed)return;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto* bank=current();if(!bank)return;
    const auto bounded=std::min(count,32u);
    for(unsigned i=0;i<bounded;++i){D3D12_RESOURCE_BARRIER b{};
        if(!safe_copy(&b,values?values+i:nullptr,sizeof(b))){bank->skip(barrier,count-i);++bank->read_failures;return;}
        Event e{};e.kind=barrier;stamp(e,list,caller);e.flags=b.Flags;e.barrier_type=b.Type;e.read_mask=1;
        if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_TRANSITION){e.resources[0]=reinterpret_cast<uintptr_t>(b.Transition.pResource);
            e.before=b.Transition.StateBefore;e.after=b.Transition.StateAfter;e.subresource=b.Transition.Subresource;}
        else if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_ALIASING)e.resources={reinterpret_cast<uintptr_t>(b.Aliasing.pResourceBefore),reinterpret_cast<uintptr_t>(b.Aliasing.pResourceAfter)};
        else if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_UAV)e.resources[0]=reinterpret_cast<uintptr_t>(b.UAV.pResource);
        else ++bank->read_failures;
        bank->record(e,now());
    }
    if(count>bounded)bank->skip(barrier,count-bounded);
}
void executed(uintptr_t queue,gears_frame_link::ListRef ref,uint64_t order,unsigned member){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    for(unsigned i=0;i<unpossessed_quads.size;++i)banks[i].executed({0,ref.identity,ref.generation},queue,order,member,now());
}
void print(FILE* file){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
#if defined(GEARS_OWNED_OUTPUT_TRACKING) && GEARS_OWNED_OUTPUT_TRACKING
    constexpr unsigned barrier_available=1;
#else
    constexpr unsigned barrier_available=0;
#endif
    fprintf(file,"death_hdr_timeline enabled=1 cpu_only=1 gpu_completed=0 pixels_verified=0 clear_not_observed=1 barrier_hook_available=%u copy_hook_available=%u same_consume_thread_only=1 first_rtv_only=1 max_events_per_ticket=256 om_barrier_row_limit=128 identity_cache_per_ticket=32 identity_source=guarded_actual_resource_com identity_failures_cached=1 raw_pointer_reuse_not_detected=1 draw_rows=consecutive_hdr26_segments_first_last_seq sample_quad_callers=2ab6d9,d80a7b execute_association=first_observed_matching_generation\n",
        barrier_available,unsigned(color_copy_diagnostics));
    for(unsigned b=0;b<unpossessed_quads.size;++b){auto& bank=banks[b];auto const& selected=unpossessed_quads.banks[b];unsigned failed{};
        for(unsigned i=0;i<bank.identities.size;++i)if(!bank.identities.entries[i].identity.canonical)++failed;
        fprintf(file,"death_hdr_bank bank=%u ordinal=%u ticket=%llu world=%llu sequence=%llu rows=%u identity_rows=%u identity_dropped=%llu identity_failed=%u read_failures=%llu stored_observations_complete=%u state_coverage_complete=0 dropped=",
            b,selected.ordinal,selected.ticket.id,selected.ticket.world,bank.sequence,bank.size,bank.identities.size,bank.identities.dropped,failed,bank.read_failures,unsigned(!bank.incomplete()&&!failed));
        for(unsigned k=0;k<kind_count;++k)fprintf(file,"%s%llu",k?",":"",bank.dropped[k]);fprintf(file," kind_order=sample,hdr_draw,targets,barrier,copy,execute\n");
        for(unsigned i=0;i<bank.size;++i){auto const& r=bank.rows[i];auto const& e=r.event;auto const& v=e.view;auto const& d=e.rectangles;
            fprintf(file,"death_hdr_event bank=%u row=%u kind=%u first_seq=%llu last_seq=%llu count=%llu first_qpc=%lld last_qpc=%lld caller=%llx quad_caller=%llx eye_valid=%u eye=%u current_view=%llx view_read_mask=%u list=%llx list_identity=%llx list_generation=%llu target_raw=%llx target_canonical=%llx target_size=%llu,%u format=%u mip=%u targets=%u descriptor_capacity_lost=%u rectangles_tracked=%u viewport_count=%u scissor_count=%u viewport=%.9g,%.9g,%.9g,%.9g scissor=%.9g,%.9g,%.9g,%.9g depth_known=%u depth=%.9g,%.9g resource0_raw=%llx resource0_canonical=%llx resource1_raw=%llx resource1_canonical=%llx flags=%u read_mask=%u execute_seq=%llu queue=%llx execute_order=%llu execute_member=%u\n",
                b,i,e.kind,r.first,r.last,r.count,r.first_qpc,r.last_qpc,static_cast<unsigned long long>(e.caller),static_cast<unsigned long long>(e.quad_caller),unsigned(v.eye_valid),v.eye,static_cast<unsigned long long>(v.current_view),v.read_mask,
                static_cast<unsigned long long>(e.list.raw),static_cast<unsigned long long>(e.list.identity),e.list.generation,static_cast<unsigned long long>(e.target.resource),static_cast<unsigned long long>(canonical(bank,e.target.resource)),e.target.width,e.target.height,e.target.format,e.target.mip,e.target_count,
                unsigned(e.descriptor_capacity_lost),unsigned(d.tracked),d.viewport_count,d.scissor_count,d.viewport.x,d.viewport.y,d.viewport.width,d.viewport.height,d.scissor.x,d.scissor.y,d.scissor.width,d.scissor.height,unsigned(d.depth_known),d.min_depth,d.max_depth,
                static_cast<unsigned long long>(e.resources[0]),static_cast<unsigned long long>(canonical(bank,e.resources[0])),static_cast<unsigned long long>(e.resources[1]),static_cast<unsigned long long>(canonical(bank,e.resources[1])),e.flags,e.read_mask,r.execute_seq,static_cast<unsigned long long>(r.queue),r.execute_order,r.member);
            if(e.kind==sample)fprintf(file,"death_hdr_sample bank=%u row=%u args=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g dimensions=%u,%u,%u,%u source_object=%llx slots=%u\n",
                b,i,e.args[0],e.args[1],e.args[2],e.args[3],e.args[4],e.args[5],e.args[6],e.args[7],e.dimensions[0],e.dimensions[1],e.dimensions[2],e.dimensions[3],static_cast<unsigned long long>(e.source_object),e.slots);
            if(e.kind==barrier)fprintf(file,"death_hdr_barrier bank=%u row=%u type=%u before=%u after=%u subresource=%u\n",b,i,e.barrier_type,e.before,e.after,e.subresource);
            if(e.kind==copy){fprintf(file,"death_hdr_copy bank=%u row=%u source_type=%u destination_type=%u source_subresource=%u destination_subresource=%u xyz=%u,%u,%u has_box=%u box=%u,%u,%u,%u,%u,%u\n",
                    b,i,e.source_type,e.destination_type,e.source_subresource,e.destination_subresource,e.xyz[0],e.xyz[1],e.xyz[2],unsigned(e.has_box),e.box[0],e.box[1],e.box[2],e.box[3],e.box[4],e.box[5]);
                for(unsigned role=0;role<2;++role)if((role?e.destination_type:e.source_type)==D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT){const auto& f=e.footprints[role];
                    fprintf(file,"death_hdr_footprint bank=%u row=%u role=%u offset=%llu format=%u size=%u,%u,%u row_pitch=%u\n",b,i,role,f.offset,f.format,f.width,f.height,f.depth,f.row_pitch);}}
        }
        for(unsigned i=0;i<bank.identities.size;++i){const auto& entry=bank.identities.entries[i];const auto& v=entry.identity;const auto& d=v.desc;
            fprintf(file,"death_hdr_identity bank=%u index=%u raw=%llx resource=%llx canonical=%llx hr=%08lx dimension=%u width=%llu height=%u depth=%u mips=%u format=%u samples=%u\n",
                b,i,static_cast<unsigned long long>(entry.raw),static_cast<unsigned long long>(v.resource),static_cast<unsigned long long>(v.canonical),static_cast<unsigned long>(v.result),unsigned(d.Dimension),d.Width,d.Height,unsigned(d.DepthOrArraySize),unsigned(d.MipLevels),unsigned(d.Format),d.SampleDesc.Count);}
    }
}
#else
void draw(ID3D12GraphicsCommandList*,uintptr_t){}
void targets_event(ID3D12GraphicsCommandList*,unsigned,bool,uintptr_t){}
void copy_event(ID3D12GraphicsCommandList*,const D3D12_TEXTURE_COPY_LOCATION*,UINT,UINT,UINT,const D3D12_TEXTURE_COPY_LOCATION*,const D3D12_BOX*,uintptr_t){}
void barriers_event(ID3D12GraphicsCommandList*,UINT,const D3D12_RESOURCE_BARRIER*,uintptr_t){}
void executed(uintptr_t,gears_frame_link::ListRef,uint64_t,unsigned){}
void print(FILE* file){fprintf(file,"death_hdr_timeline enabled=0 clear_not_observed=1 cpu_only=1\n");}
#endif
}
