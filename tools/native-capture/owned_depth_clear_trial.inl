// Private counterfactual only. Uses existing draw/OM hooks and retained owned
// output certification; DSV resource identity/dimensions remain unknown.
namespace owned_depth_clear_trial {
#if defined(GEARS_OWNED_DEPTH_CLEAR_TRIAL) && GEARS_OWNED_DEPTH_CLEAR_TRIAL
#if !defined(GEARS_RUNTIME_RENDER_SIZE) || !GEARS_RUNTIME_RENDER_SIZE || !defined(GEARS_NATIVE_OWNED_PIXELS) || !GEARS_NATIVE_OWNED_PIXELS
#error Owned depth-clear trial requires runtime render size and native owned pixels
#endif
using Input=gears_owned_depth_clear::Input;
bool enabled{},attested{},initialized{};uint64_t attempts{},applied{},noops{},returned{},restored{},accepted_overflow{},rejected_overflow{},unowned_receipts_skipped{};
uintptr_t observer_base{};unsigned module_size{},observer_size{};
std::array<uint64_t,gears_owned_depth_clear::reason_count> rejects{};
struct Row {Input input{};uintptr_t resource{};uint64_t generation{},last_ticket{},calls{};unsigned reason{};};
std::array<Row,16> accepted_rows{},rejected_rows{};unsigned accepted_count{},rejected_count{};
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
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++attempts;
    Input in{};in.enabled=enabled;in.attested=attested;in.module_base=base;in.caller=caller;
    in.module_size=module_size;in.observer_base=observer_base;in.observer_size=observer_size;
    in.vertices=vertices;in.instances=instances;in.start=start;in.first_instance=first_instance;
    const auto& frozen=consuming_frozen_frame;in.ticket=frozen.id;in.world=frozen.world_generation;in.source_output=frozen.source_output;
    in.owned_current=recording.load()&&consuming_owned_scene&&consuming_renderer&&frozen.enabled&&frozen.id&&
        frozen.id==consuming_frame_ticket&&frozen.world_generation&&frozen.world_generation==committed_world_generation.load()&&
        gears_native_render_size::matches_session(frozen.sample.session)&&native_owned_scene_consuming(consuming_renderer,frozen.id)&&
        gears_owned_scene_live::resource_world==frozen.world_generation;
    const auto owned=owned_scene_gpu::tracker.owned_status();const auto& creation=owned_scene_gpu::creation;
    const auto& size_plan=gears_native_render_size::packet.plan;
    in.plan_valid=gears_native_render_size::loaded&&gears_vr::render_size_valid(size_plan)&&
        gears_native_render_size::matches_session(frozen.sample.session);
    in.plan_width=size_plan.source_width;in.plan_height=size_plan.source_height;
    in.owned_active=owned_scene_gpu::watching()&&owned_scene_gpu::birth_ok&&owned.enabled&&owned.certified&&owned_scene_gpu::texture&&
        owned.target.resource&&owned.target.resource==owned_scene_gpu::resource&&owned.target.resource==creation.resource&&
        owned.target.resource==gears_owned_scene_live::trial.resource&&owned.generation==owned_scene_gpu::generation&&owned.generation&&
        creation.desc.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&creation.desc.DepthOrArraySize==1&&creation.desc.MipLevels==1&&
        creation.desc.SampleDesc.Count==1&&!creation.desc.SampleDesc.Quality&&creation.desc.Format==DXGI_FORMAT_R8G8B8A8_UNORM&&
        creation.desc.Flags==D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET&&creation.desc.Width==owned.target.width&&creation.desc.Height==owned.target.height;
    in.width=owned.target.width;in.height=owned.target.height;
    const auto inventory=final_eye_inventory;const auto desktop=swapchain_bindings.inventory(inventory.chain);
    in.desktop_chain=inventory.chain;in.desktop_epoch=inventory.epoch;in.inventory_qpc=inventory.qpc;
    in.desktop_width=inventory.width;in.desktop_height=inventory.height;in.desktop_count=inventory.count;in.inventory_resources=inventory.resources;
    in.desktop_queue=desktop.binding.queue;in.owned_queue=owned.queue;in.desktop_generation=desktop.binding.generation;
    in.binding_count=desktop.count;in.binding_resources=desktop.resources;
    in.desktop_valid=inventory.valid&&desktop.valid&&desktop.binding.chain==inventory.chain&&
        desktop.binding.width==inventory.width&&desktop.binding.height==inventory.height;
    // Equality is against the accessor's exact CURRENT per-chain inventory, not
    // a generation assigned retroactively to a cached Present stamp.
    if(in.desktop_valid&&inventory.count==desktop.count&&inventory.resources==desktop.resources)
        in.matched_generation=desktop.binding.generation;
    in.state=frame_target_stats.draw_state(reinterpret_cast<uintptr_t>(list));
    in.om=owned_ticket_size_trace::attachments.binding(reinterpret_cast<uintptr_t>(list));
    // Retain the route for owned inventory/source-extent failures too. A tiny
    // Present surface can differ from the frozen original engine scene extent.
    auto decision=gears_owned_depth_clear::plan(in);
    if(decision.reason==gears_owned_depth_clear::route||decision.reason==gears_owned_depth_clear::desktop||
       decision.reason==gears_owned_depth_clear::source_extent){std::array<void*,12> frames{};
        in.stack_count=CaptureStackBackTrace(0,static_cast<DWORD>(frames.size()),frames.data(),nullptr);
        for(unsigned i=0;i<in.stack_count;++i)in.stack[i]=reinterpret_cast<uintptr_t>(frames[i]);decision=gears_owned_depth_clear::plan(in);
    }
    const bool receipt=decision.apply()||decision.reason==gears_owned_depth_clear::already_correct;
    // World-zero/ordinary desktop draws previously exhausted all rejection
    // rows before any VR failure could be recorded. Keep their counters, but
    // reserve bounded detail rows for the currently owned scene.
    const bool record_receipt=receipt||in.owned_current;
    if(!record_receipt)++unowned_receipts_skipped;
    auto& rows=receipt?accepted_rows:rejected_rows;auto& row_count=receipt?accepted_count:rejected_count;
    Row* row{};
    for(unsigned i=0;record_receipt&&i<row_count;++i){auto& candidate=rows[i];
        if(candidate.reason==unsigned(decision.reason)&&candidate.input.world==in.world&&candidate.resource==owned.target.resource&&
           candidate.generation==owned.generation&&candidate.input.om.depth_handle==in.om.depth_handle&&
           candidate.input.desktop_chain==in.desktop_chain&&candidate.input.desktop_generation==in.desktop_generation&&
           candidate.input.source_output.rectangle==in.source_output.rectangle&&candidate.input.source_output.target==in.source_output.target&&
           candidate.input.state.viewport==in.state.viewport&&candidate.input.width==in.width&&candidate.input.height==in.height){row=&candidate;break;}
    }
    if(record_receipt&&!row&&row_count<rows.size()){row=&rows[row_count++];row->input=in;row->resource=owned.target.resource;row->generation=owned.generation;row->reason=decision.reason;}
    if(row){++row->calls;row->last_ticket=in.ticket;}else if(receipt)++accepted_overflow;else if(record_receipt)++rejected_overflow;
    if(decision.apply())++applied;else if(decision.reason==gears_owned_depth_clear::already_correct)++noops;else ++rejects[decision.reason];
    auto viewport=[&](gears_owned_depth_clear::Viewport const& value){const auto& r=value.rectangle;
        const D3D12_VIEWPORT native{r.x,r.y,r.width,r.height,value.minimum,value.maximum};original_viewports(list,1,&native);
        if(r==decision.before.rectangle)++restored;
    };
    gears_owned_depth_clear::forward(decision,viewport,[&]{original_draw(list,vertices,instances,start,first_instance);++returned;});
}
void print(FILE* file){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    fprintf(file,"owned_depth_clear_receipt_scope owned_scene_only=1 unowned_skipped=%llu frozen_source_extent=1 present_extent_independent=1\n",unowned_receipts_skipped);
    fprintf(file,"owned_depth_clear_trial compiled=1 enabled=%u attested=%u attempts=%llu applied=%llu noops=%llu returned=%llu restored=%llu accepted_rows=%u accepted_limit=16 accepted_overflow=%llu rejected_rows=%u rejected_limit=16 rejected_overflow=%llu dsv_size_known=0 viewport_only=1 pixels_verified=0\n",unsigned(enabled),unsigned(attested),attempts,applied,noops,returned,restored,accepted_count,accepted_overflow,rejected_count,rejected_overflow);
    for(unsigned reason=1;reason<gears_owned_depth_clear::reason_count;++reason)if(reason!=gears_owned_depth_clear::already_correct)
        fprintf(file,"owned_depth_clear_reject reason=%u count=%llu reasons=disabled:1,identity:2,arguments:3,owner:4,output:5,attachments:6,rectangles:7,route:8,desktop:9,source_extent:11\n",reason,rejects[reason]);
    auto print_rows=[&](auto const& rows,unsigned row_count,const char* kind){
    for(unsigned i=0;i<row_count;++i){auto const& r=rows[i];auto const& in=r.input;auto const& old=in.state.viewport;
        fprintf(file,"owned_depth_clear_row row=%u kind=%s reason=%u calls=%llu world=%llu first_ticket=%llu last_ticket=%llu resource=%llx generation=%llu caller=%llx vertices=%u instances=%u start=%u first_instance=%u owned_current=%u owned_active=%u bind_serial=%llu om_observed=%u targets=%u dsv_present=%u dsv_read=%u dsv_handle=%llx old_viewport=%.9g,%.9g,%.9g,%.9g new_viewport=0,0,%u,%u scissor=%.9g,%.9g,%.9g,%.9g depth=%.9g,%.9g module_base=%llx stack_count=%u stack=",
            i,kind,r.reason,r.calls,in.world,in.ticket,r.last_ticket,static_cast<unsigned long long>(r.resource),r.generation,static_cast<unsigned long long>(in.caller),in.vertices,in.instances,in.start,in.first_instance,
            unsigned(in.owned_current),unsigned(in.owned_active),in.om.serial,unsigned(in.om.observed),in.om.count,unsigned(in.om.depth_present),unsigned(in.om.depth_read),static_cast<unsigned long long>(in.om.depth_handle),
            old.x,old.y,old.width,old.height,in.width,in.height,in.state.scissor.x,in.state.scissor.y,in.state.scissor.width,in.state.scissor.height,in.state.min_depth,in.state.max_depth,static_cast<unsigned long long>(base),in.stack_count);
        for(unsigned frame=0;frame<in.stack_count;++frame)fprintf(file,"%s%llx",frame?",":"",static_cast<unsigned long long>(in.stack[frame]));fprintf(file,"\n");
        fprintf(file,"owned_depth_clear_extents row=%u kind=%s plan_valid=%u plan=%u,%u desktop_valid=%u desktop_chain=%llx desktop_queue=%llx owned_queue=%llx desktop=%u,%u inventory_epoch=%llu inventory_qpc=%lld binding_generation=%llu matched_binding_generation=%llu inventory_count=%u binding_count=%u exact_current_inventory=%u dsv_size_known=0\n",
            i,kind,unsigned(in.plan_valid),in.plan_width,in.plan_height,unsigned(in.desktop_valid),static_cast<unsigned long long>(in.desktop_chain),
            static_cast<unsigned long long>(in.desktop_queue),static_cast<unsigned long long>(in.owned_queue),in.desktop_width,in.desktop_height,
            in.desktop_epoch,in.inventory_qpc,in.desktop_generation,in.matched_generation,in.desktop_count,in.binding_count,
            unsigned(in.matched_generation!=0));
        const auto& source=in.source_output;
        fprintf(file,"owned_depth_clear_source_extent row=%u kind=%s ticket=%llu world=%llu source=%llx family=%llx target=%llx output=%d,%d,%d,%d matches_ticket_world=%u scalar_identity_only=1 dsv_size_known=0\n",
            i,kind,source.ticket,source.world,static_cast<unsigned long long>(source.source),static_cast<unsigned long long>(source.family),
            static_cast<unsigned long long>(source.target),source.rectangle[0],source.rectangle[1],source.rectangle[2],source.rectangle[3],
            unsigned(source.matches(in.ticket,in.world)));
    }
    };
    print_rows(accepted_rows,accepted_count,"accepted");print_rows(rejected_rows,rejected_count,"rejected");
}
#else
bool enable(FILE*){return true;}
void print(FILE*){}
#endif
}
