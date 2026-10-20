// Same-thread CPU CopyTextureRegion observation. Never dereference recorded
// addresses later; no resource references, COM calls, barriers or GPU copies.
using ColorCopyFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,const D3D12_TEXTURE_COPY_LOCATION*,UINT,UINT,UINT,
    const D3D12_TEXTURE_COPY_LOCATION*,const D3D12_BOX*);
ColorCopyFn original_color_copy{};
struct ColorCopyLinks {uintptr_t source{},destination{};};
struct ColorCopyTrace {
    uintptr_t caller{},list{},source{},destination{},expected_source{},expected_destination{};
    unsigned source_type{},destination_type{},source_subresource{},destination_subresource{},relation{},eye{2};
    std::array<unsigned,3> xyz{};bool has_box{};std::array<unsigned,6> box{};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT source_footprint{},destination_footprint{};
    uint64_t calls{};
};
std::array<ColorCopyTrace,32> color_copy_traces{};std::mutex color_copy_mutex;
uint64_t color_copy_attempts{},color_copy_scope_rejected{},color_copy_links_rejected{},color_copy_args_rejected{},color_copy_unrelated{},color_copy_overflow{};
uint64_t color_copy_matched{},color_copy_full_subresource_pair{},color_copy_partial_or_other_pair{},color_copy_source_only{},color_copy_destination_only{};
// Used only by the isolated self-test process; never supplied by capture.ini.
uintptr_t color_copy_test_source_global{},color_copy_test_destination_global{};
bool color_copy_engine_native(uintptr_t wrapper,uintptr_t& native){
    uintptr_t table{};
    return wrapper&&safe_copy(&table,reinterpret_cast<void*>(wrapper),8)&&table==base+0x133ecf8&&
        safe_copy(&native,reinterpret_cast<void*>(wrapper+0xc),8)&&system_resource_methods(native);
}
bool color_copy_links(uintptr_t source_global,uintptr_t destination_global,ColorCopyLinks& output){
    uintptr_t source_public{},owner{},wrapper{},destination_public{},object{},table{};unsigned index{};
    ColorCopyLinks candidate{};
    if(!safe_copy(&source_public,reinterpret_cast<void*>(source_global),8)||source_public<0x13c||
       !safe_copy(&owner,reinterpret_cast<void*>(source_public-0x130),8)||!owner||
       !safe_copy(&wrapper,reinterpret_cast<void*>(owner+0xc),8)||!color_copy_engine_native(wrapper,candidate.source))return false;
    if(!safe_copy(&destination_public,reinterpret_cast<void*>(destination_global),8)||destination_public<0x13c)return false;
    const auto texture=destination_public-0x13c;
    if(!safe_copy(&index,reinterpret_cast<void*>(texture+0x138),4)||index>=8||
       !safe_copy(&object,reinterpret_cast<void*>(texture+0x38+index*8),8)||!object||
       !safe_copy(&table,reinterpret_cast<void*>(object),8)||table!=base+0x1667bb8||
       !safe_copy(&wrapper,reinterpret_cast<void*>(object+0x60),8)||!color_copy_engine_native(wrapper,candidate.destination))return false;
    output=candidate;return true;
}
// 0=recordable, 1=unreadable/unsupported arguments, 2=unrelated resources.
unsigned decode_color_copy(const ColorCopyLinks& links,const D3D12_TEXTURE_COPY_LOCATION* destination,UINT x,UINT y,UINT z,
    const D3D12_TEXTURE_COPY_LOCATION* source,const D3D12_BOX* box,ColorCopyTrace& output){
    D3D12_TEXTURE_COPY_LOCATION src{},dst{};D3D12_BOX bounds{};
    if(!safe_copy(&src,source,sizeof(src))||!safe_copy(&dst,destination,sizeof(dst))||
       (box&&!safe_copy(&bounds,box,sizeof(bounds)))||
       (src.Type!=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX&&src.Type!=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT)||
       (dst.Type!=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX&&dst.Type!=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT))return 1;
    ColorCopyTrace value{};value.source=reinterpret_cast<uintptr_t>(src.pResource);value.destination=reinterpret_cast<uintptr_t>(dst.pResource);
    value.relation=unsigned(links.source&&value.source==links.source)|(unsigned(links.destination&&value.destination==links.destination)<<1);
    if(!value.relation)return 2;
    value.expected_source=links.source;value.expected_destination=links.destination;value.xyz={x,y,z};value.has_box=box!=nullptr;
    if(box)value.box={bounds.left,bounds.top,bounds.front,bounds.right,bounds.bottom,bounds.back};
    value.source_type=src.Type;value.destination_type=dst.Type;
    value.source_subresource=src.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX?src.SubresourceIndex:UINT_MAX;
    value.destination_subresource=dst.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX?dst.SubresourceIndex:UINT_MAX;
    if(src.Type==D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT)value.source_footprint=src.PlacedFootprint;
    if(dst.Type==D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT)value.destination_footprint=dst.PlacedFootprint;
    output=value;return 0;
}
bool color_copy_same_footprint(const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& a,const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& b){
    return a.Offset==b.Offset&&a.Footprint.Format==b.Footprint.Format&&a.Footprint.Width==b.Footprint.Width&&
        a.Footprint.Height==b.Footprint.Height&&a.Footprint.Depth==b.Footprint.Depth&&a.Footprint.RowPitch==b.Footprint.RowPitch;
}
// Caller holds color_copy_mutex. Classification includes calls beyond trace capacity.
void record_color_copy(const ColorCopyTrace& value){
    ++color_copy_matched;
    if(value.relation==3){
        if(value.source_type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX&&value.destination_type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX&&
           !value.source_subresource&&!value.destination_subresource&&value.xyz==std::array<unsigned,3>{0,0,0}&&!value.has_box)++color_copy_full_subresource_pair;
        else ++color_copy_partial_or_other_pair;
    } else if(value.relation==1)++color_copy_source_only;
    else if(value.relation==2)++color_copy_destination_only;
    for(auto& entry:color_copy_traces){
        if(!entry.calls){entry=value;entry.calls=1;return;}
        // list is an illustrative first address, not an aggregation key.
        if(entry.caller==value.caller&&entry.source==value.source&&entry.destination==value.destination&&
           entry.expected_source==value.expected_source&&entry.expected_destination==value.expected_destination&&entry.eye==value.eye&&
           entry.source_type==value.source_type&&entry.destination_type==value.destination_type&&entry.source_subresource==value.source_subresource&&
           entry.destination_subresource==value.destination_subresource&&entry.xyz==value.xyz&&entry.has_box==value.has_box&&entry.box==value.box&&
           color_copy_same_footprint(entry.source_footprint,value.source_footprint)&&color_copy_same_footprint(entry.destination_footprint,value.destination_footprint)){
            ++entry.calls;return;
        }
    }
    ++color_copy_overflow;
}
void observe_color_copy(ID3D12GraphicsCommandList* list,const D3D12_TEXTURE_COPY_LOCATION* destination,UINT x,UINT y,UINT z,
    const D3D12_TEXTURE_COPY_LOCATION* source,const D3D12_BOX* box,uintptr_t caller){
    if(!color_copy_diagnostics||!recording.load()||!viewport_diagnostics||!trial_draw_scope||!consuming_renderer)return;
    struct Header {uintptr_t data;int count,capacity;} views{};
    // trial_draw_scope is set only by hook_consume for our lifetime-tracked
    // renderer. Do not require a current eye: family-level resolve may precede it.
    if(!safe_copy(&views,reinterpret_cast<char*>(consuming_renderer)+0x68,sizeof(views))||!views.data||views.data%8||
       views.count!=2||views.capacity<2||views.capacity>1024){
        std::lock_guard<std::mutex> lock(color_copy_mutex);++color_copy_scope_rejected;return;
    }
    ColorCopyLinks links{};ColorCopyTrace value{};
    const bool linked=color_copy_links(self_testing?color_copy_test_source_global:base+0x1c3b420,
        self_testing?color_copy_test_destination_global:base+0x1c3b410,links);
    const unsigned decoded=linked?decode_color_copy(links,destination,x,y,z,source,box,value):1;
    if(linked&&!decoded){
        value.caller=caller;value.list=reinterpret_cast<uintptr_t>(list);
        uintptr_t tls{},context{},current{};
        if(!self_testing&&safe_copy(&tls,reinterpret_cast<void*>(__readgsqword(0x58)),8)&&tls&&
           safe_copy(&context,reinterpret_cast<void*>(tls+0x20),8)&&context&&safe_copy(&current,reinterpret_cast<void*>(context+0x14c0),8)&&
           current>=views.data&&(current-views.data)%0xd30==0&&(current-views.data)/0xd30<2)value.eye=unsigned((current-views.data)/0xd30);
    }
    std::lock_guard<std::mutex> lock(color_copy_mutex);++color_copy_attempts;
    if(!linked){++color_copy_links_rejected;return;}
    if(decoded==1){++color_copy_args_rejected;return;}
    if(decoded==2){++color_copy_unrelated;return;}
    record_color_copy(value);
}
void STDMETHODCALLTYPE hook_color_copy(ID3D12GraphicsCommandList* list,const D3D12_TEXTURE_COPY_LOCATION* destination,UINT x,UINT y,UINT z,
    const D3D12_TEXTURE_COPY_LOCATION* source,const D3D12_BOX* box){
    if(stereo_publish_bypass){original_color_copy(list,destination,x,y,z,source,box);return;}
    Scope scope;if(!scope.world()){auto unknown=frame_lock();frame_link.invalidate();return original_color_copy(list,destination,x,y,z,source,box);}
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
    std::lock_guard<FrameHookMutex> frame_guard(frame_hook_mutex);
    if(owned_scene_gpu::watching()){
        D3D12_TEXTURE_COPY_LOCATION src{},dst{};
        if(!safe_copy(&src,source,sizeof(src))||!safe_copy(&dst,destination,sizeof(dst)))owned_scene_gpu::uncertain(owned_scene_gpu::Unknown::unreadable);
        else {owned_scene_gpu::use(list,canonical_identity(src.pResource),true);owned_scene_gpu::use(list,canonical_identity(dst.pResource),true);}
    }
#endif
    death_hdr_timeline::copy_event(list,destination,x,y,z,source,box,reinterpret_cast<uintptr_t>(_ReturnAddress())-base);
    final_transfer_live::copy_destination(list,destination);
    observe_backbuffer_copy(list,destination,x,y,z,source,box,reinterpret_cast<uintptr_t>(_ReturnAddress())-base);
    observe_color_copy(list,destination,x,y,z,source,box,reinterpret_cast<uintptr_t>(_ReturnAddress())-base);
    auto pixel_guard=death_hdr_pixels::execution_lock();
    const auto pixel_token=death_hdr_pixels::prepare_copy(list,destination,x,y,z,source,box,reinterpret_cast<uintptr_t>(_ReturnAddress())-base);
    owned_source_pixels::copy_passthrough(list,destination,x,y,z,source,box,reinterpret_cast<uintptr_t>(_ReturnAddress())-base,original_color_copy);
    death_hdr_pixels::append_after_original(pixel_token,list,destination,source);
}
void print_color_copy(FILE* file){
    std::lock_guard<std::mutex> lock(color_copy_mutex);
    fprintf(file,"color_copy_summary enabled=%u attempts=%llu scope_rejected=%llu links_rejected=%llu args_rejected=%llu unrelated=%llu overflow=%llu matched=%llu full_subresource_pair=%llu partial_or_other_pair=%llu source_only=%llu destination_only=%llu slots=32 first_list_only=1 classification_includes_overflow=1 same_thread_only=1 copy_texture_region_only=1 pixels_verified=0\n",
        color_copy_diagnostics,color_copy_attempts,color_copy_scope_rejected,color_copy_links_rejected,color_copy_args_rejected,color_copy_unrelated,color_copy_overflow,
        color_copy_matched,color_copy_full_subresource_pair,color_copy_partial_or_other_pair,color_copy_source_only,color_copy_destination_only);
    for(const auto& e:color_copy_traces)if(e.calls){
        fprintf(file,"color_copy caller=%llx first_list=%llx source=%llx destination=%llx expected_source420=%llx expected_destination410=%llx relation=%u eye=%u source_type=%u source_subresource=%u destination_type=%u destination_subresource=%u dst_xyz=%u,%u,%u box_null=%u box=%u,%u,%u,%u,%u,%u calls=%llu\n",
            static_cast<unsigned long long>(e.caller),static_cast<unsigned long long>(e.list),static_cast<unsigned long long>(e.source),static_cast<unsigned long long>(e.destination),
            static_cast<unsigned long long>(e.expected_source),static_cast<unsigned long long>(e.expected_destination),e.relation,e.eye,e.source_type,e.source_subresource,e.destination_type,e.destination_subresource,
            e.xyz[0],e.xyz[1],e.xyz[2],!e.has_box,e.box[0],e.box[1],e.box[2],e.box[3],e.box[4],e.box[5],e.calls);
        if(e.source_type==D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT)fprintf(file,"color_copy_footprint role=source offset=%llu format=%u width=%u height=%u depth=%u row_pitch=%u\n",
            e.source_footprint.Offset,unsigned(e.source_footprint.Footprint.Format),e.source_footprint.Footprint.Width,e.source_footprint.Footprint.Height,e.source_footprint.Footprint.Depth,e.source_footprint.Footprint.RowPitch);
        if(e.destination_type==D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT)fprintf(file,"color_copy_footprint role=destination offset=%llu format=%u width=%u height=%u depth=%u row_pitch=%u\n",
            e.destination_footprint.Offset,unsigned(e.destination_footprint.Footprint.Format),e.destination_footprint.Footprint.Width,e.destination_footprint.Footprint.Height,e.destination_footprint.Footprint.Depth,e.destination_footprint.Footprint.RowPitch);
    }
}
