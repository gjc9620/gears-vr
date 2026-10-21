// Test-only offscreen final-output source. All native game resources and the
// private staging texture remain retained by the pinned context until exit.
namespace ngx_final_xr {
#if defined(GEARS_NGX_FINAL_XR_TRIAL) && GEARS_NGX_FINAL_XR_TRIAL
using namespace gears_ngx_final_xr;
struct State {
    bool enabled{},failed{};unsigned failure{};
    ID3D12Resource* staging{};gears_ngx_stereo::Identity identity{};gears_ngx_stereo::Extent extent{};
    uint64_t source_generation{},feature_rebinds{};
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
    uint64_t queue_id{};
    std::optional<Packet> left;
    Pending pending;
    std::array<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>,Pending::capacity> lists;
    gears_vr::StereoTicketStore tickets;
    uint64_t left_draws{},right_draws{},copies{},matched{},published{},skipped{},pair_rejected{},proof_rejected{},ambiguous{},resets{};
    uint64_t last_ticket{},last_sequence{},last_serial{},last_list{},last_generation{};unsigned last_reason{},last_state{};
};
State& state=*new State; // No late GPU/COM destructors during game teardown.
bool enabled(){return state.enabled;}
void fail(unsigned site){state.failed=true;if(!state.failure)state.failure=site;}
bool enable(bool requested,FILE* file){
    wchar_t runtime[MAX_PATH]{};const auto length=GetEnvironmentVariableW(L"XR_RUNTIME_JSON",runtime,MAX_PATH);
    const wchar_t* name=wcsrchr(runtime,L'\\');if(!name)name=wcsrchr(runtime,L'/');name=name?name+1:runtime;
    const bool simulator_manifest=length&&length<MAX_PATH&&!_wcsicmp(name,L"openxr_simulator.json");
    state.enabled=requested&&simulator_manifest&&continuous_session&&stereo_gpu_publish&&stereo_xr;
    // The continuous runner keeps the strict RuntimeTracked source profile,
    // including when its selected runtime is the simulator. Operator-simulated
    // pose acceptance must never be enabled just to make this route start.
    fprintf(file,"ngx_final_xr_identity compiled=1 enabled=%u requested=%u runtime_manifest_simulator=%u operator_pose_profile=%u default_off=1 native_owned_scene=0 source_idle=common\n",unsigned(state.enabled),unsigned(requested),unsigned(simulator_manifest),unsigned(simulator_operator));
    return state.enabled;
}
bool ensure(ID3D12Resource* color,gears_ngx_stereo::Identity identity,gears_ngx_stereo::Extent extent){
    if(!enabled())return true;auto& s=state;
    if(s.failed||!gears_ngx_stereo::valid(identity)||identity.world!=1||!gears_ngx_stereo::bounded(extent)||extent.width%2||!color)return false;
    if(s.staging)return gears_ngx_stereo::same(s.identity,identity)&&gears_ngx_stereo::same(s.extent,extent);
    Microsoft::WRL::ComPtr<ID3D12Device> device;Microsoft::WRL::ComPtr<IUnknown> id;
    if(FAILED(color->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device.As(&id))||reinterpret_cast<uintptr_t>(id.Get())!=identity.device){fail(__LINE__);return false;}
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=extent.width;desc.Height=extent.height;
    desc.DepthOrArraySize=1;desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R10G10B10A2_UNORM;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;StereoPublishBypass bypass;
    if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&s.staging)))){fail(__LINE__);return false;}
    s.identity=identity;s.extent=extent;s.source_generation=identity.epoch;return true;
}
bool rebind(gears_ngx_stereo::Identity before,gears_ngx_stereo::Identity after,gears_ngx_stereo::Extent extent){
    if(!enabled()||!state.staging)return true;auto& s=state;
    // Called only after the old adapter's actual pair/creation/use retirement.
    // The separately owned staging texture keeps its own generation and DIRECT
    // queue. No shared-slot metadata or GPU completion is manufactured here.
    if(s.failed||!gears_ngx_stereo::same(s.identity,before)||!gears_ngx_stereo::valid(after)||
       after.epoch<=before.epoch||after.world!=before.world||after.device!=before.device||
       !gears_ngx_stereo::same(s.extent,extent)||s.left)return false;
    for(unsigned i=0;i<Pending::capacity;++i)if(s.pending.occupied(i))return false;
    s.identity=after;++s.feature_rebinds;return true;
}
void release_consumed_lists(){for(unsigned i=0;i<Pending::capacity;++i)if(!state.pending.occupied(i))state.lists[i].Reset();}
void reset(uintptr_t list){
    if(!enabled()||!list)return;
    state.pending.reset(list);if(state.left&&state.left->draw.list==list)state.left.reset();
    release_consumed_lists();++state.resets;
}
void draw(ID3D12GraphicsCommandList* list,uintptr_t caller,unsigned elements,unsigned instances){
    if(!enabled()||state.failed||stereo_publish_bypass||!recording.load()||caller!=0x1af92b||elements!=6||instances!=1||!list||
       !state.staging||!original_resource_barrier||!original_color_copy||!dlss_output_control::scoped())return;
    std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);auto& s=state;
    const auto binding=frame_target_stats.binding(reinterpret_cast<uintptr_t>(list));
    ID3D12Resource *source{},*color{};unsigned eye{};gears_ngx_stereo::Rect rect;
    if(!ngx_capture_control::final_target_pixel_pair(binding.target.resource,source,color,eye,rect))return;
    auto* q=dlss_output_control::current_quad;auto const& proof=q->output->proof.draw;
    auto const& frame=consuming_frozen_frame;
    if(!proof.candidate||proof.list!=canonical_identity(list)||proof.generation!=frame_link.current(proof.list).generation||
       proof.source!=canonical_identity(color)||proof.target!=canonical_identity(source)||proof.world!=s.identity.world||
       !frame.enabled||frame.id!=proof.ticket||frame.world_generation!=proof.world||eye!=proof.eye||
       rect.x!=eye*s.extent.width/2||rect.y||rect.width!=s.extent.width/2||rect.height!=s.extent.height){++s.pair_rejected;return;}
    Packet packet;packet.draw={proof.world,proof.ticket,proof.list,proof.generation,proof.source,proof.target,s.extent.width,s.extent.height,eye};
    packet.image.id=frame.id;packet.image.sample=frame.sample;
    for(unsigned i=0;i<2;++i)packet.image.rect[i]={int(i*s.extent.width/2),0,int(s.extent.width/2),int(s.extent.height)};
    if(!valid(packet,simulator_operator)){++s.pair_rejected;return;}
    if(!eye){
        gears_vr::StereoTicketGeometry geometry{frame.source_view,frame.source_projection,frame.expected,frame.finite_aim,frame.aiming};
        const auto retained=s.tickets.retain(frame.id,frame.sample,simulator_operator,&geometry);
        if(retained!=gears_vr::TicketRetainResult::retained||!s.tickets.retire_through(frame.id)){++s.pair_rejected;s.left.reset();return;}
        s.left=packet;++s.left_draws;return;
    }
    ++s.right_draws;
    if(!s.left||!pair(*s.left,packet,simulator_operator)){++s.pair_rejected;s.left.reset();return;}
    s.left.reset();
    const auto desc=source->GetDesc();Microsoft::WRL::ComPtr<ID3D12Device> device;Microsoft::WRL::ComPtr<IUnknown> device_id;
    if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||desc.Format!=DXGI_FORMAT_R10G10B10A2_UNORM||
       desc.Width!=s.extent.width||desc.Height!=s.extent.height||desc.DepthOrArraySize!=1||desc.MipLevels!=1||desc.SampleDesc.Count!=1||
       !(desc.Flags&D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)||list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT||
       FAILED(list->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device.As(&device_id))||reinterpret_cast<uintptr_t>(device_id.Get())!=s.identity.device){fail(__LINE__);return;}
    const auto slot=s.pending.push(packet,simulator_operator);if(slot==Pending::capacity){++s.skipped;return;}
    s.lists[slot]=list;
    D3D12_RESOURCE_BARRIER barriers[2]{};
    for(auto& b:barriers){b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;}
    barriers[0].Transition.pResource=source;barriers[0].Transition.StateBefore=D3D12_RESOURCE_STATE_RENDER_TARGET;barriers[0].Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
    barriers[1].Transition.pResource=s.staging;barriers[1].Transition.StateBefore=D3D12_RESOURCE_STATE_COMMON;barriers[1].Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_DEST;
    // Source state is certified by the just-forwarded, independently guarded
    // native RTV draw. Restore it before returning to the native command stream.
    original_resource_barrier(list,2,barriers);
    D3D12_TEXTURE_COPY_LOCATION from{},to{};from.Type=to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;from.pResource=source;to.pResource=s.staging;
    original_color_copy(list,&to,0,0,0,&from,nullptr);
    for(auto& b:barriers)std::swap(b.Transition.StateBefore,b.Transition.StateAfter);
    original_resource_barrier(list,2,barriers);++s.copies;
}
bool output_proved(Packet const& p){
    auto const* e=dlss_output_control::store.find({p.draw.world,p.draw.ticket});
    if(!e||!gears_dlss_output::complete(*e))return false;
    for(unsigned eye=0;eye<2;++eye){auto const& d=e->payload.eyes[eye].draw;
        if(d.list!=p.draw.list||d.generation!=p.draw.generation||d.source!=p.draw.source||d.target!=p.draw.target||
           d.dimensions[0]!=p.draw.width||d.dimensions[1]!=p.draw.height)return false;}
    return dlss_output_control::publication(p.draw.world,p.draw.ticket)&&vr_dof_compat::publication(p.draw.world,p.draw.ticket);
}
void executed(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* lists){
    if(!enabled()||state.failed||stereo_publish_bypass)return;auto& s=state;
    // Called after the actual original Execute, under API order and the frame
    // lock. No native Execute can overtake the immediate publisher copy.
    std::array<ID3D12CommandList*,64> raw{};std::array<Member,64> members{};
    bool readable=count<=raw.size()&&(!count||safe_copy(raw.data(),lists,count*sizeof(raw[0])));
    if(readable)for(unsigned i=0;i<count;++i){members[i].list=canonical_identity(raw[i]);members[i].generation=frame_link.current(members[i].list).generation;}
    const auto selected=s.pending.take(members.data(),readable?count:0,readable);release_consumed_lists();
    if(selected.match==Match::none)return;
    if(selected.match!=Match::one){++s.ambiguous;return;}++s.matched;
    auto const& p=selected.packet;
    if(!recording.load()||submitted_lifetime.stopping()||p.draw.world!=committed_world_generation.load()||
       !ngx_capture_control::context||ngx_capture_control::context->failed||!output_proved(p)){++s.proof_rejected;return;}
    Microsoft::WRL::ComPtr<ID3D12Device> device;Microsoft::WRL::ComPtr<IUnknown> id;
    const auto qid=canonical_identity(queue);
    const auto sdk_queue=ngx_capture_control::context->gpu.completion();
    if(!queue||queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT||!qid||
       FAILED(queue->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device.As(&id))||reinterpret_cast<uintptr_t>(id.Get())!=s.identity.device||
       sdk_queue.queue!=qid||sdk_queue.device!=s.identity.device||(s.queue_id&&s.queue_id!=qid)){fail(__LINE__);return;}
    if(!s.queue){s.queue=queue;s.queue_id=qid;}
    if(stereo_publisher.state.load()==0&&!stereo_publish_counts.stopped.load()){
        StereoPublishBypass bypass;
        if(stereo_publisher.bind_resource(queue,s.staging,s.source_generation,D3D12_RESOURCE_STATE_COMMON,simulator_operator))++stereo_publish_counts.bind_requests;
        else {++s.skipped;return;}
    }
    gears_vr::GpuCaptureTrace trace{};bool captured{};const auto deadline=stereo_publish_deadline.load();
    ++stereo_publish_counts.capture_attempts;
    if(stereo_publish_counts.stopped.load())trace.reason=gears_vr::GpuCaptureReason::native_stopped;
    else if(deadline&&now()>=deadline)trace.reason=gears_vr::GpuCaptureReason::native_deadline;
    else {StereoPublishBypass bypass;captured=stereo_publisher.capture_resource(s.staging,s.source_generation,D3D12_RESOURCE_STATE_COMMON,p.image,&trace);}
    s.last_ticket=p.image.id;s.last_sequence=p.image.sample.sequence;s.last_serial=trace.serial_after;s.last_reason=unsigned(trace.reason);s.last_state=trace.state;
    s.last_list=p.draw.list;s.last_generation=p.draw.generation;
    if(captured){++s.published;stereo_publish_counts.last_ticket=p.image.id;}
    else {++s.skipped;++stereo_publish_counts.capture_skipped;
        using R=gears_vr::GpuCaptureReason;
        const bool allowed=trace.reason==R::lock_busy||trace.reason==R::no_consumer||trace.reason==R::producer_in_flight||trace.reason==R::consumer_not_done||
            trace.reason==R::native_stopped||trace.reason==R::native_deadline||trace.reason==R::world_quiescing||
            (trace.reason==R::not_ready&&(trace.state==1||(trace.state==4&&stereo_publish_counts.stopped.load())));
        if(!allowed)fail(__LINE__);
    }
}
void print(FILE* file){std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);auto const& s=state;
    fprintf(file,"ngx_final_xr_epochs feature_epoch=%llu staging_generation=%llu feature_rebinds=%llu same_device_extent_world_required=1\n",s.identity.epoch,s.source_generation,s.feature_rebinds);
    fprintf(file,"ngx_final_xr enabled=%u failed=%u failure=%u staging=%llx dimensions=%u,%u format=24 queue=%llx left_draws=%llu right_draws=%llu copies=%llu matched=%llu published=%llu skipped=%llu pair_rejected=%llu proof_rejected=%llu ambiguous=%llu resets=%llu last_ticket=%llu last_sequence=%llu last_serial=%llu last_reason=%u last_state=%u last_list=%llx last_generation=%llu actual_execute_required=1 frozen_pose_required=1 headset_verified=0\n",
        unsigned(s.enabled),unsigned(s.failed),s.failure,reinterpret_cast<uintptr_t>(s.staging),s.extent.width,s.extent.height,s.queue_id,s.left_draws,s.right_draws,s.copies,s.matched,s.published,s.skipped,s.pair_rejected,s.proof_rejected,s.ambiguous,s.resets,s.last_ticket,s.last_sequence,s.last_serial,s.last_reason,s.last_state,s.last_list,s.last_generation);
}
#endif
}
