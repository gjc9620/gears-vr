// Private counterfactual only. Uses existing draw/OM hooks and retained owned
// output certification; DSV resource identity/dimensions remain unknown.
namespace owned_depth_clear_trial {
#if defined(GEARS_OWNED_DEPTH_CLEAR_TRIAL) && GEARS_OWNED_DEPTH_CLEAR_TRIAL
#if !defined(GEARS_RUNTIME_RENDER_SIZE) || !GEARS_RUNTIME_RENDER_SIZE || !defined(GEARS_NATIVE_OWNED_PIXELS) || !GEARS_NATIVE_OWNED_PIXELS
#error Owned depth-clear trial requires runtime render size and native owned pixels
#endif
using Input=gears_owned_depth_clear::Input;
bool enabled{},attested{},initialized{};uint64_t attempts{},applied{},returned{},restored{},row_overflow{};
uintptr_t observer_base{};unsigned module_size{},observer_size{};
std::array<uint64_t,gears_owned_depth_clear::reason_count> rejects{};
struct Row {Input input{};uintptr_t resource{};uint64_t generation{},last_ticket{},calls{};unsigned reason{};};
std::array<Row,32> rows{};unsigned row_count{};
bool image_size(uintptr_t image,unsigned& size){
    IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
    if(!image||!safe_copy(&dos,reinterpret_cast<void*>(image),sizeof(dos))||dos.e_magic!=IMAGE_DOS_SIGNATURE||
       dos.e_lfanew<=0||dos.e_lfanew>=0x100000||image>UINTPTR_MAX-unsigned(dos.e_lfanew)||
       !safe_copy(&nt,reinterpret_cast<void*>(image+unsigned(dos.e_lfanew)),sizeof(nt))||nt.Signature!=IMAGE_NT_SIGNATURE||
       nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC||
       !nt.OptionalHeader.SizeOfImage||image>UINTPTR_MAX-nt.OptionalHeader.SizeOfImage)return false;
    size=nt.OptionalHeader.SizeOfImage;return true;
}
bool enable(FILE* file){
    if(initialized)return false;initialized=true;
    // Historical runtime artifact1 source SHA256 is preserved in the log. Live
    // bytes must match both bounded native chunks before any trial draw occurs.
    const bool first=final_transfer_live::verify_range(file,0x27e580,0x1d2,"dd5aeb84d7bb96ebeb1b0d2f24f37ba12127dac74b872be038632fd94cdf0cea","owned_depth_clear_identity");
    const bool second=final_transfer_live::verify_range(file,0x1ad75c,0x6b4,"34142122907196b48fa7c7b7ab28508837420c5a8e930c7c2b2f38fe57172622","owned_depth_clear_identity");
    HMODULE observer{};
    const bool observer_found=GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&initialized),&observer)!=FALSE;
    observer_base=reinterpret_cast<uintptr_t>(observer);
    const bool bounds=observer_found&&image_size(base,module_size)&&module_size>0x27e752&&image_size(observer_base,observer_size)&&
        (base+module_size<=observer_base||observer_base+observer_size<=base);
    attested=first&&second;enabled=attested&&bounds&&!self_testing&&continuous_session&&frame_link_diagnostics&&owned_bindings_enabled;
    fprintf(file,"owned_depth_clear_trial_identity compiled=1 enabled=%u attested=%u chunks=2 historical_source_sha256=911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6 dsv_size_known=0 viewport_only=1\n",unsigned(enabled),unsigned(attested));
    fprintf(file,"owned_depth_clear_route_bounds valid=%u game_base=%llx game_size=%u observer_base=%llx observer_size=%u route=first_game_returns:1af345,1ade10,27e752 skip_module=capture_only other_native_or_module_rejected=1\n",
        unsigned(bounds),static_cast<unsigned long long>(base),module_size,static_cast<unsigned long long>(observer_base),observer_size);
    return enabled;
}
void draw(ID3D12GraphicsCommandList* list,uintptr_t caller,UINT vertices,UINT instances,UINT start,UINT first_instance){
    if(caller!=base+0x1af345){original_draw(list,vertices,instances,start,first_instance);return;}
    std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);++attempts;
    Input in{};in.enabled=enabled;in.attested=attested;in.module_base=base;in.caller=caller;
    in.module_size=module_size;in.observer_base=observer_base;in.observer_size=observer_size;
    in.vertices=vertices;in.instances=instances;in.start=start;in.first_instance=first_instance;
    const auto& frozen=consuming_frozen_frame;in.ticket=frozen.id;in.world=frozen.world_generation;
    in.owned_current=recording.load()&&consuming_owned_scene&&consuming_renderer&&frozen.enabled&&frozen.id&&
        frozen.id==consuming_frame_ticket&&frozen.world_generation&&frozen.world_generation==committed_world_generation.load()&&
        gears_native_render_size::matches_session(frozen.sample.session)&&native_owned_scene_consuming(consuming_renderer,frozen.id)&&
        gears_owned_scene_live::resource_world==frozen.world_generation;
    const auto owned=owned_scene_gpu::tracker.owned_status();const auto& creation=owned_scene_gpu::creation;
    in.owned_active=owned_scene_gpu::watching()&&owned_scene_gpu::birth_ok&&owned.enabled&&owned.certified&&owned_scene_gpu::texture&&
        owned.target.resource&&owned.target.resource==owned_scene_gpu::resource&&owned.target.resource==creation.resource&&
        owned.target.resource==gears_owned_scene_live::trial.resource&&owned.generation==owned_scene_gpu::generation&&owned.generation&&
        creation.desc.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&creation.desc.DepthOrArraySize==1&&creation.desc.MipLevels==1&&
        creation.desc.SampleDesc.Count==1&&!creation.desc.SampleDesc.Quality&&creation.desc.Format==DXGI_FORMAT_R8G8B8A8_UNORM&&
        creation.desc.Flags==D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET&&creation.desc.Width==owned.target.width&&creation.desc.Height==owned.target.height;
    in.width=owned.target.width;in.height=owned.target.height;
    in.state=frame_target_stats.draw_state(reinterpret_cast<uintptr_t>(list));
    in.om=owned_ticket_size_trace::attachments.binding(reinterpret_cast<uintptr_t>(list));
    // Stack walking is confined to an otherwise admitted candidate clear.
    auto decision=gears_owned_depth_clear::plan(in);
    if(decision.reason==gears_owned_depth_clear::route){std::array<void*,12> frames{};
        in.stack_count=CaptureStackBackTrace(0,static_cast<DWORD>(frames.size()),frames.data(),nullptr);
        for(unsigned i=0;i<in.stack_count;++i)in.stack[i]=reinterpret_cast<uintptr_t>(frames[i]);decision=gears_owned_depth_clear::plan(in);
    }
    Row* row{};
    for(unsigned i=0;i<row_count;++i){auto& candidate=rows[i];
        if(candidate.reason==unsigned(decision.reason)&&candidate.input.world==in.world&&candidate.resource==owned.target.resource&&
           candidate.generation==owned.generation&&candidate.input.om.depth_handle==in.om.depth_handle&&
           candidate.input.state.viewport==in.state.viewport&&candidate.input.width==in.width&&candidate.input.height==in.height){row=&candidate;break;}
    }
    if(!row&&row_count<rows.size()){row=&rows[row_count++];row->input=in;row->resource=owned.target.resource;row->generation=owned.generation;row->reason=decision.reason;}
    if(row){++row->calls;row->last_ticket=in.ticket;}else ++row_overflow;
    if(decision.apply())++applied;else ++rejects[decision.reason];
    auto viewport=[&](gears_owned_depth_clear::Viewport const& value){const auto& r=value.rectangle;
        const D3D12_VIEWPORT native{r.x,r.y,r.width,r.height,value.minimum,value.maximum};original_viewports(list,1,&native);
        if(r==decision.before.rectangle)++restored;
    };
    gears_owned_depth_clear::forward(decision,viewport,[&]{original_draw(list,vertices,instances,start,first_instance);++returned;});
}
void print(FILE* file){
    std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
    fprintf(file,"owned_depth_clear_trial compiled=1 enabled=%u attested=%u attempts=%llu applied=%llu returned=%llu restored=%llu rows=%u limit=32 row_overflow=%llu dsv_size_known=0 viewport_only=1 pixels_verified=0\n",unsigned(enabled),unsigned(attested),attempts,applied,returned,restored,row_count,row_overflow);
    for(unsigned reason=1;reason<rejects.size();++reason)fprintf(file,"owned_depth_clear_reject reason=%u count=%llu reasons=disabled:1,identity:2,arguments:3,owner:4,output:5,attachments:6,rectangles:7,route:8\n",reason,rejects[reason]);
    for(unsigned i=0;i<row_count;++i){auto const& r=rows[i];auto const& in=r.input;auto const& old=in.state.viewport;
        fprintf(file,"owned_depth_clear_row row=%u reason=%u calls=%llu world=%llu first_ticket=%llu last_ticket=%llu resource=%llx generation=%llu caller=%llx vertices=%u instances=%u start=%u first_instance=%u owned_current=%u owned_active=%u bind_serial=%llu om_observed=%u targets=%u dsv_present=%u dsv_read=%u dsv_handle=%llx old_viewport=%.9g,%.9g,%.9g,%.9g new_viewport=0,0,%u,%u scissor=%.9g,%.9g,%.9g,%.9g depth=%.9g,%.9g module_base=%llx stack_count=%u stack=",
            i,r.reason,r.calls,in.world,in.ticket,r.last_ticket,static_cast<unsigned long long>(r.resource),r.generation,static_cast<unsigned long long>(in.caller),in.vertices,in.instances,in.start,in.first_instance,
            unsigned(in.owned_current),unsigned(in.owned_active),in.om.serial,unsigned(in.om.observed),in.om.count,unsigned(in.om.depth_present),unsigned(in.om.depth_read),static_cast<unsigned long long>(in.om.depth_handle),
            old.x,old.y,old.width,old.height,in.width,in.height,in.state.scissor.x,in.state.scissor.y,in.state.scissor.width,in.state.scissor.height,in.state.min_depth,in.state.max_depth,static_cast<unsigned long long>(base),in.stack_count);
        for(unsigned frame=0;frame<in.stack_count;++frame)fprintf(file,"%s%llx",frame?",":"",static_cast<unsigned long long>(in.stack[frame]));fprintf(file,"\n");
    }
}
#else
bool enable(FILE*){return true;}
void print(FILE*){}
#endif
}
