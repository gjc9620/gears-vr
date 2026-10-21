// Private simulator-only SDK integration. Every observation belongs to the
// current consume; the frozen diagnostic banks never supply producer regions.
namespace ngx_trial_producer {
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
using namespace gears_ngx_stereo;
struct Surface {uintptr_t resource{};unsigned color{},depth{};Rect last_color{};bool native_color_pending{};};
struct Binding {
    uintptr_t raw{},canonical{};uint64_t generation{},uav{};
    gears_viewport::Target color{},depth{};
    gears_vr::ComPtr<ID3D12PipelineState> pso;
};
struct Mv {uint64_t gpu{},generation{};uintptr_t list{};Rect rect{};gears_vr::ComPtr<ID3D12PipelineState> pso;};
struct Depth {Resource destination{};unsigned mask{};};
struct State {
    uint64_t world{},ticket{};uintptr_t renderer{};std::array<uintptr_t,2> views{};
    std::array<Rect,2> rectangles{};Extent output{};
    std::array<std::array<int32_t,4>,2> target_rectangles{};
    std::array<std::array<float,4>,2> sampling{};unsigned view_field_mask{};
    std::array<Binding,16> bindings{};std::array<Surface,64> surfaces{};
    std::array<Depth,8> depths{};std::array<Mv,8> motion{};
    std::array<TypedResourceLease,64> candidates{};
    std::array<unsigned,7> motion_stages{};std::array<float,4> last_motion_values{};
    unsigned surface_count{},depth_count{},motion_count{},candidate_count{},failure{};bool valid{};
};
// The disposable test pins its DLL. Quarantined native objects must survive
// process termination without late COM calls from CRT destruction.
std::shared_ptr<State>& state=*new std::shared_ptr<State>;
std::atomic<bool> enabled{};uint64_t tickets{},input_calls{},inputs_accepted{},pso_queries{},pso_verified{};
unsigned last_failure{};
gears_vr::ComPtr<ID3D12PipelineState>& verified_pso=*new gears_vr::ComPtr<ID3D12PipelineState>;
bool active(){return enabled.load()&&consuming_renderer&&consuming_frame_ticket&&state&&state->valid&&recording.load()&&state->world==committed_world_generation.load()&&
    state->ticket==consuming_frame_ticket&&state->renderer==reinterpret_cast<uintptr_t>(consuming_renderer);}
unsigned coverage(Rect clip){unsigned mask{};for(unsigned eye=0;eye<2;++eye){auto r=state->rectangles[eye];
    if(clip.x<=r.x&&clip.y<=r.y&&uint64_t(clip.x)+clip.width>=uint64_t(r.x)+r.width&&uint64_t(clip.y)+clip.height>=uint64_t(r.y)+r.height)mask|=1u<<eye;}return mask;}
void begin(gears_submitted::FrameTicket const& frame,void* renderer){
    if(!enabled||!state)return;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);*state=State{};
    auto& s=*state;if(!frame.enabled||!frame.id||!renderer)return;
    Record built;inspect_completed_renderer(built,renderer);
    if(built.family_count!=2||built.renderer_count!=2)return;
    // renderer+d0 is the union of reduced view regions (e.g.1067x480),
    // whereas the allocations and Create output are the1280x720 buffer.
    // The input callback joins this frozen size to actual Create/typed inputs.
    std::array<uint32_t,2> size{};
    if(!ngx_route_observer::read(size[0],base,0x1c3b3b8)||!ngx_route_observer::read(size[1],base,0x1c3b3bc))return;
    s.output={size[0],size[1]};
#if defined(GEARS_NGX_OWNED_SCENE_TRIAL) && GEARS_NGX_OWNED_SCENE_TRIAL
    // Only the admitted native-owned renderer uses the independent plan.
    // Actual Create and every retained physical resource must still agree.
    if(consuming_owned_scene)s.output={gears_native_render_size::width(),gears_native_render_size::height()};
#endif
    if(!bounded(s.output)||s.output.width%2)return;
    for(unsigned eye=0;eye<2;++eye){std::array<float,4> rect{};s.views[eye]=built.family_views[eye];
        if(!safe_copy(rect.data(),reinterpret_cast<void const*>(s.views[eye]+0x48),sizeof(rect)))return;
        for(float value:rect)if(!std::isfinite(value)||value<0||value>8192||std::floor(value)!=value)return;
        s.rectangles[eye]={uint32_t(rect[0]),uint32_t(rect[1]),uint32_t(rect[2]),uint32_t(rect[3])};
        if(safe_copy(s.target_rectangles[eye].data(),reinterpret_cast<void const*>(s.views[eye]+0x58),16))s.view_field_mask|=1u<<(2*eye);
        if(safe_copy(s.sampling[eye].data(),reinterpret_cast<void const*>(s.views[eye]+0x3d0),16))s.view_field_mask|=2u<<(2*eye);
        if(ngx_capture_control::packed_input_trial()){
            if(!(s.view_field_mask&(1u<<(2*eye))))return;auto const& target=s.target_rectangles[eye];
            for(auto value:target)if(value<0||value>8192)return;
            s.rectangles[eye]={uint32_t(target[0]),uint32_t(target[1]),uint32_t(target[2]),uint32_t(target[3])};
        }
        if(!contains(s.output,s.rectangles[eye]))return;
    }
    s.world=frame.world_generation;s.ticket=frame.id;s.renderer=reinterpret_cast<uintptr_t>(renderer);s.valid=true;++tickets;
}
void observe_resource(ID3D12Resource* resource){
    if(!active()||!resource)return;for(unsigned i=0;i<state->candidate_count;++i)if(state->candidates[i].raw==reinterpret_cast<uintptr_t>(resource))return;
    const auto d=resource->GetDesc();if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||d.Width!=state->output.width||d.Height!=state->output.height)return;
    auto lease=TypedResourceLease::observe(resource);Resource value;if(!lease.materialize(value))return;
    if(state->candidate_count==state->candidates.size()){state->failure=5;return;}
    state->candidates[state->candidate_count++]=std::move(lease);
}
void barriers(UINT count,D3D12_RESOURCE_BARRIER const* values){
    if(!active())return;if(count>1024||(count&&!values)){state->failure=6;return;}
    for(UINT i=0;i<count;++i){D3D12_RESOURCE_BARRIER b{};if(!safe_copy(&b,values+i,sizeof(b))){state->failure=6;return;}
        if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)observe_resource(b.Transition.pResource);
        else if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_UAV)observe_resource(b.UAV.pResource);
        else if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_ALIASING){observe_resource(b.Aliasing.pResourceBefore);observe_resource(b.Aliasing.pResourceAfter);}}
}
Binding* binding(ID3D12GraphicsCommandList* list,bool create=true){
    if(!active()||!list)return nullptr;const auto canonical=canonical_identity(list);const auto ref=frame_link.current(canonical);
    if(!canonical||ref.identity!=canonical||!ref.generation)return nullptr;
    for(auto& b:state->bindings)if(b.canonical==canonical&&b.generation==ref.generation)return &b;
    if(!create)return nullptr;
    for(auto& b:state->bindings)if(b.canonical==canonical&&b.generation!=ref.generation)b={};
    for(auto& b:state->bindings)if(!b.canonical){b.raw=reinterpret_cast<uintptr_t>(list);b.canonical=canonical;b.generation=ref.generation;return &b;}
    state->failure=1;return nullptr;
}
void targets(ID3D12GraphicsCommandList* list,D3D12_CPU_DESCRIPTOR_HANDLE const* depth){
    if(!active())return;gears_viewport::Target color{},dsv{};auto rtv=frame_target_stats.binding(reinterpret_cast<uintptr_t>(list));
    if(!frame_target_stats.descriptor_capacity_lost()&&rtv.targets==1&&!rtv.target.mip&&rtv.target.width==state->output.width&&rtv.target.height==state->output.height)color=rtv.target;
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};if(depth&&safe_copy(&handle,depth,sizeof(handle))){auto t=ngx_depth_capture::lookup(handle.ptr);
        if(t.width==state->output.width&&t.height==state->output.height)dsv=t;}
    if(auto* b=binding(list,color.resource||dsv.resource)){b->color=color;b->depth=dsv;}
}
void draw(ID3D12GraphicsCommandList* list,unsigned elements,unsigned instances){
    if(!elements||!instances||!active())return;auto* b=binding(list,false);if(!b)return;
    // A later untracked/partial draw cannot leave an older region eligible.
    for(unsigned i=0;i<state->surface_count;++i)if(state->surfaces[i].resource==b->color.resource)state->surfaces[i].native_color_pending=false;
    const auto r=frame_target_stats.draw_state(b->raw);const auto& v=r.viewport;const auto& sc=r.scissor;
    if(!r.tracked||r.viewport_count!=1||r.scissor_count!=1)return;
    const float x=std::max(v.x,sc.x),y=std::max(v.y,sc.y),right=std::min(v.x+v.width,sc.x+sc.width),bottom=std::min(v.y+v.height,sc.y+sc.height);
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(right)||!std::isfinite(bottom)||x<0||y<0||right<=x||bottom<=y||right>8192||bottom>8192)return;
    if(std::floor(x)!=x||std::floor(y)!=y||std::floor(right)!=right||std::floor(bottom)!=bottom)return;
    const Rect clip{uint32_t(x),uint32_t(y),uint32_t(right-x),uint32_t(bottom-y)};const auto mask=coverage(clip);
    for(unsigned kind=0;kind<2;++kind){auto t=kind?b->depth:b->color;
        if(!t.resource||t.mip||t.width!=state->output.width||t.height!=state->output.height)continue;
        Surface* surface{};for(unsigned i=0;i<state->surface_count;++i)if(state->surfaces[i].resource==t.resource)surface=&state->surfaces[i];
        if(!surface){if(state->surface_count==state->surfaces.size()){state->failure=2;return;}surface=&state->surfaces[state->surface_count++];surface->resource=t.resource;}
        (kind?surface->depth:surface->color)|=mask;
        if(!kind){surface->last_color=clip;surface->native_color_pending=true;}
    }
}
void sdk_write(uintptr_t output){
    if(!active())return;for(unsigned i=0;i<state->surface_count;++i)
        if(state->surfaces[i].resource==output)state->surfaces[i].native_color_pending=false;
}
bool native_preprocess_region(uintptr_t output,Rect& region){
    region={};if(!active()||state->failure)return false;
    for(unsigned i=0;i<state->surface_count;++i){auto& s=state->surfaces[i];
        if(s.resource!=output||!s.native_color_pending)continue;s.native_color_pending=false;region=s.last_color;
        // The original postprocess draw, rather than stale TLS current_view,
        // selects the exact constructed input rectangle to preserve.
        for(auto const& eye:state->rectangles)if(region.x==eye.x&&region.y==eye.y&&region.width==eye.width&&region.height==eye.height)return true;
        return false;
    }return false;
}
void depth_copy(ID3D12Resource* destination,ID3D12Resource* source,uintptr_t caller){
    if(!active()||caller!=base+0x1c5fe0||!destination||!source)return;
    auto dst=TypedResourceLease::observe(destination),src=TypedResourceLease::observe(source);Resource d{},s{};
    if(!dst.materialize(d)||!src.materialize(s)||!same(d.extent,state->output)||!same(s.extent,state->output)||d.device!=s.device)return;
    unsigned mask{};for(unsigned i=0;i<state->surface_count;++i)if(state->surfaces[i].resource==s.identity)mask|=state->surfaces[i].depth;
    if(!mask)return;for(unsigned i=0;i<state->depth_count;++i)if(same_resource(state->depths[i].destination,d)){state->depths[i].mask|=mask;return;}
    if(state->depth_count==state->depths.size()){state->failure=3;return;}state->depths[state->depth_count++]={d,mask};
}
void table(ID3D12GraphicsCommandList* list,UINT index,uint64_t gpu,uintptr_t caller){
    if(index!=3)return;const bool known=caller==base+0x17b99a;
    if(auto* b=binding(list,known))b->uav=known?gpu:0;
}
void pso(ID3D12GraphicsCommandList* list,ID3D12PipelineState* native,uintptr_t caller){
    const bool known=caller==base+0x17b514;if(auto* b=binding(list,known))b->pso=known?native:nullptr;
}
void motion(ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z,uintptr_t caller,ngx_mv_constants_capture::Binding const& before){
    if(!active()||caller!=base+0x1ac833)return;++state->motion_stages[0];
    if(!before.valid)return;++state->motion_stages[1];if(before.upload.used!=160)return;++state->motion_stages[2];
    if(!ngx_mv_constants_capture::stable(before))return;++state->motion_stages[3];
    auto* b=binding(list,false);if(!b||!b->uav)return;++state->motion_stages[4];if(!b->pso)return;++state->motion_stages[5];
    float values[4]{};std::memcpy(values,before.upload.upload.data()+80,sizeof(values));
    std::copy(values,values+4,state->last_motion_values.begin());
    for(float value:values)if(!std::isfinite(value)||value<0||value>8192||std::floor(value)!=value)return;
    Rect rect{uint32_t(values[2]),uint32_t(values[3]),uint32_t(values[0]),uint32_t(values[1])};
    if(!contains(state->output,rect)||x!=(rect.width+7)/8||y!=(rect.height+7)/8||z!=1)return;
    bool exact{};for(auto r:state->rectangles)exact=exact||(r.x==rect.x&&r.y==rect.y&&r.width==rect.width&&r.height==rect.height);if(!exact)return;
    ++state->motion_stages[6];
    if(state->motion_count==state->motion.size()){state->failure=4;return;}
    state->motion[state->motion_count++]={b->uav,b->generation,b->canonical,rect,b->pso};
}
void executed(UINT count,ngx_mv_compute_capture::ExecuteList const* members,bool readable){
    // Queue submission may run after consume TLS has been restored. Retire
    // only exact submitted generations under the caller's frame lock; this
    // adds no producer observation and does not require consume-thread TLS.
    if(!enabled.load()||!state||!state->valid||!readable||count>64)return;for(auto& b:state->bindings)for(UINT i=0;i<count;++i)
        if(b.canonical==members[i].canonical&&b.generation==members[i].generation)b={};
}
bool shader(ID3D12PipelineState* native){
    if(!native)return false;if(verified_pso.Get()==native)return true;
    if(pso_queries>=8)return false;++pso_queries;gears_vr::ComPtr<ID3DBlob> blob;
    if(FAILED(native->GetCachedBlob(&blob))||!blob||blob->GetBufferSize()>4*1024*1024)return false;
    auto* bytes=static_cast<unsigned char*>(blob->GetBufferPointer());const auto size=blob->GetBufferSize();
    for(size_t i=0;i+32<=size;++i){if(std::memcmp(bytes+i,"DXBC",4))continue;uint32_t length{};std::memcpy(&length,bytes+i+24,4);
        if(length!=2360||length>size-i)continue;
        BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};unsigned char digest[32]{};char hex[65]{};
        bool ok=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
        if(ok)ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
        if(ok)ok=BCryptHashData(hash,bytes+i,length,0)>=0&&BCryptFinishHash(hash,digest,32,0)>=0;
        if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
        for(unsigned j=0;j<32;++j)sprintf_s(hex+2*j,3,"%02x",digest[j]);
        if(ok&&!strcmp(hex,"6d5c211bdf1bc2f5671a841aa1cb02fc49686e1f52420284932b22d396583c01")){verified_pso=native;++pso_verified;return true;}
    }return false;
}
bool input(void*,ngx_stereo_host::ControlCreateBinding const& created,ngx_stereo_host::ControlConsumption used,
    EvaluateShadow const& shadow,Command cmd,std::array<Resource,4> const& retained,ngx_stereo_host::ControlInputTicket& out){
    ++input_calls;auto reject=[](unsigned reason){last_failure=reason;return false;};
    if(!active()||state->failure||state->world!=used.world||state->ticket!=used.ticket||state->renderer!=used.renderer)return reject(10);
    if(!same(state->output,created.layout.full_output)||!same(retained[0].extent,state->output))return reject(11);
    unsigned color{},depth{},mv{};
    for(unsigned i=0;i<state->surface_count;++i)if(state->surfaces[i].resource==retained[0].identity)color|=state->surfaces[i].color;
    for(unsigned i=0;i<state->depth_count;++i)if(same_resource(state->depths[i].destination,retained[1]))depth|=state->depths[i].mask;
    for(unsigned i=0;i<state->motion_count;++i){auto const& m=state->motion[i];auto descriptor=ngx_mv_descriptor_capture::lookup_exact_target(m.gpu);
        if(!descriptor.match||descriptor.resource!=retained[2].identity||!shader(m.pso.Get()))continue;
        for(unsigned eye=0;eye<2;++eye){auto r=state->rectangles[eye];if(m.rect.x==r.x&&m.rect.y==r.y&&m.rect.width==r.width&&m.rect.height==r.height)mv|=1u<<eye;}}
    if(color!=3)return reject(12);if(depth!=3)return reject(13);
    // The early source ledger intentionally excludes visible descriptor copy
    // fanout. On the creation ticket the selected bank has only just seeded;
    // earlier MV Dispatches cannot acquire retrospective binding proof. Keep
    // this one pair unpublished, then require live observed copies next ticket.
    const bool transition=mv==0&&created.creation_ticket==used.ticket;
    if(mv!=3&&!transition)return reject(14);
    out.consumption=used;out.original_views=state->views;out.command=cmd;
    constexpr size_t offsets[]{0,0x18,0x20,8};for(unsigned i=0;i<4;++i)std::memcpy(&out.raw_resources[i],shadow.bytes.data()+offsets[i],8);
    out.native_transition=transition;
    for(unsigned eye=0;eye<2;++eye){auto& p=out.producers[eye];p.use={created.identity,used.ticket,cmd.list,cmd.generation,eye};p.view=state->views[eye];p.resources=retained;
        p.rendered[0]=p.rendered[1]=state->rectangles[eye];if(!transition)p.rendered[2]=state->rectangles[eye];}
    sdk_write(retained[3].identity);++inputs_accepted;return true;
}
void print(FILE* file){fprintf(file,"ngx_isolated_trial_producer enabled=%u tickets=%llu input_calls=%llu accepted=%llu last_failure=%u state_failure=%u surfaces=%u depth_copies=%u mv_dispatches=%u pso_queries=%llu verified_psos=%llu full_shader_hash_required=1 pixel_correctness_verified=0\n",
    unsigned(enabled),tickets,input_calls,inputs_accepted,last_failure,state?state->failure:0,state?state->surface_count:0,state?state->depth_count:0,state?state->motion_count:0,pso_queries,pso_verified);}
#else
bool active(){return false;}
#endif
}
