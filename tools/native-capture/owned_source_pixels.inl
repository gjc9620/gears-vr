namespace owned_source_pixels {
#if defined(GEARS_OWNED_SOURCE_PIXELS) && GEARS_OWNED_SOURCE_PIXELS
using ngx_output_pixels::Slot;using ngx_output_pixels::backend;
bool initialized{},enabled{},stopped{};unsigned count{},eyes{},rejected{},failure{};
uint64_t ticket{},world{},execute_order{};uintptr_t renderer{};
std::array<Slot,2> slots{};
// All allocation/map/COM handling belongs to the existing NativeBackend. A
// pending/ambiguous slot is never recycled, including across world/session stop.
#if defined(GEARS_OWNED_SOURCE_PIXELS_TEST)
gears_ngx_route::Ownership owner();bool attest(FILE*);
#else
gears_ngx_route::Ownership owner(){
    gears_ngx_route::Ownership o;
    if(!consuming_owned_scene||!recording.load()||!frame_link_diagnostics||owned_ticket_size_trace::finished||
       !owned_ticket_size_trace::matches(consuming_renderer,consuming_frozen_frame)||
       consuming_frame_ticket!=consuming_frozen_frame.id||consuming_frozen_frame.world_generation<2||
       committed_world_generation.load()!=consuming_frozen_frame.world_generation)return o;
    uintptr_t tls{};int capacity{};o.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);
    if(!safe_copy(&tls,reinterpret_cast<void*>(__readgsqword(0x58)),8)||!tls||
       !safe_copy(&o.context,reinterpret_cast<void*>(tls+0x20),8)||!o.context||
       !safe_copy(&o.view,reinterpret_cast<void*>(o.context+0x14c0),8)||
       !safe_copy(&o.views,reinterpret_cast<void*>(o.renderer+0x68),8)||
       !safe_copy(&o.count,reinterpret_cast<void*>(o.renderer+0x70),4)||
       !safe_copy(&capacity,reinterpret_cast<void*>(o.renderer+0x74),4)||capacity<2||capacity>1024||
       !gears_unpossessed_quad::native_eye_index(o.view,o.views,o.count,o.eye))return o;
    o.associated=true;o.mask=7;o.ticket=consuming_frame_ticket;o.world=consuming_frozen_frame.world_generation;return o;
}
bool attest(FILE* file){
    // Exact full native Copy/state chunks; same historical contract as HostLive.
    struct Chunk{uintptr_t rva;unsigned size;const char* hash;};
    constexpr Chunk chunks[]{
        {0x1c5dd8,0x315,"038bef1d04e09846166ed3dd5479a71f6a9cc548469d5b8f1626553f1f137bf4"},
        {0x897f50,0x239,"ce26c00b6cd9d2148ec2f94f53e3ca11fab8127b943f08386d6d2a7c199e71c7"},
        {0x1c60f0,0x8c,"0c9f3023bb7afd53bf933be7ea725a591e548ba5ec2b2e44e8eab2d790ccc61b"},
        {0x1c617c,0x43,"421a79a0fc79095b9082dae3e29d7ea221705decb0da2fce586c99d23ce5231e"},
        {0x17c0b4,0x10e,"16a728d1d2df0385da04fe63c8bc43d01941c3d34384445489820180e076ae94"},
        {0x17ab94,0x327,"dd18f288f7976969f3be83427e41455328568263995755e1aced455707db0a86"}
    };bool match=true;for(auto const& c:chunks)match=final_transfer_live::verify_range(file,c.rva,c.size,c.hash)&&match;return match;
}
#endif
bool same_owner(gears_ngx_route::Ownership const& a,gears_ngx_route::Ownership const& b){return a.associated&&b.associated&&
    a.world==b.world&&a.ticket==b.ticket&&a.renderer==b.renderer&&a.context==b.context&&a.view==b.view&&a.eye==b.eye;}
void fail(unsigned reason){if(!failure)failure=reason;}
void release_unappended(unsigned i){auto& s=slots[i];if(!s.life.appended&&!s.released){backend().release(i);s.released=true;}}
void copy_passthrough(ID3D12GraphicsCommandList* list,const D3D12_TEXTURE_COPY_LOCATION* dst,UINT x,UINT y,UINT z,
    const D3D12_TEXTURE_COPY_LOCATION* src,const D3D12_BOX* box,uintptr_t caller,CopyFn forward){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);int chosen=-1;
    if(enabled&&!stopped&&!failure&&recording.load()&&count<2&&caller==0x1c6093&&!x&&!y&&!z&&!box){
        auto o=owner();
        if(o.associated&&o.world>=2&&o.eye<2&&(!count||(o.world==world&&o.ticket==ticket&&o.renderer==renderer))){
            // First two eligible copies in observed order. TLS eye is metadata,
            // may repeat, and does not certify left/right source content.
            {D3D12_TEXTURE_COPY_LOCATION a{},b{};
                if(!safe_copy(&a,src,sizeof(a))||!safe_copy(&b,dst,sizeof(b))||!a.pResource||!b.pResource||
                   a.Type!=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX||b.Type!=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX||a.SubresourceIndex||b.SubresourceIndex){++rejected;}
                else{chosen=int(count++);auto& s=slots[unsigned(chosen)];s.before.ownership=o;s.raw_list=list;s.source=a;s.destination=b;
                    s.before.resources[1]=reinterpret_cast<uintptr_t>(a.pResource);s.before.resources[0]=reinterpret_cast<uintptr_t>(b.pResource);
                    s.sequence=count;s.copy_sequence=count;s.copy_qpc=now();s.thread=GetCurrentThreadId();s.caller=caller;
                    if(count==1){ticket=o.ticket;world=o.world;renderer=o.renderer;}
                    s.prepared=backend().prepare(unsigned(chosen),s);
                    if(!s.prepared||s.source_desc.width!=5120||s.source_desc.height!=2800||s.source_desc.format!=26||s.destination_desc.format!=26||!ngx_output_pixels::textures_supported(s.source_desc,s.destination_desc)||
                       (chosen==1&&(s.source_desc.canonical!=slots[0].source_desc.canonical||s.destination_desc.canonical!=slots[0].destination_desc.canonical))){
                        fail(gears_death_hdr_pixels::prepare_failed);release_unappended(unsigned(chosen));chosen=-1;
                    }
                }
            }
        }else if(count&&o.associated){fail(gears_death_hdr_pixels::prepare_failed);}
    }
    try{ngx_output_pixels::copy_passthrough(list,dst,x,y,z,src,box,caller,forward);}
    catch(...){if(chosen>=0){fail(gears_death_hdr_pixels::prepare_failed);release_unappended(unsigned(chosen));}throw;}
    if(chosen<0)return;auto& s=slots[unsigned(chosen)];auto current=backend().list_ref(s.life.list.identity);
    if(stopped||!recording.load()||!same_owner(s.before.ownership,owner())||current.identity!=s.life.list.identity||current.generation!=s.life.list.generation){
        fail(gears_death_hdr_pixels::prepare_failed);release_unappended(unsigned(chosen));return;}
    s.normal=true;s.life.appended=true; // Set before GPU recording: exceptions quarantine.
    try{backend().append(unsigned(chosen),s,forward);}catch(...){s.life.fail(gears_death_hdr_pixels::prepare_failed);fail(s.life.failure);throw;}
    eyes|=1u<<s.before.ownership.eye;
}
void execute_passthrough(ID3D12CommandQueue* queue,UINT n,ID3D12CommandList* const* members,ExecuteFn forward){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);unsigned token{};
    if(count){std::array<ID3D12CommandList*,gears_frame_link::Tracker::list_capacity> copied{};
        const bool readable=n<=copied.size()&&(!n||safe_copy(copied.data(),members,size_t(n)*sizeof(copied[0])));++execute_order;
        for(unsigned i=0;i<count;++i){auto& s=slots[i];if(!s.life.appended||s.life.failure||s.life.saved)continue;
            bool valid=readable,exact=true;unsigned occurrences{},member{};
            if(valid)for(unsigned k=0;k<n;++k){auto id=backend().list_identity(copied[k]);if(!id){valid=false;break;}
                if(id==s.life.held_identity){++occurrences;member=k;auto ref=backend().list_ref(id);exact=exact&&ref.identity==s.life.list.identity&&ref.generation==s.life.list.generation;}}
            uintptr_t id{};if(occurrences&&!backend().queue_valid(i,queue,id)){s.life.fail(gears_death_hdr_pixels::cross_queue);continue;}
            if(s.life.before(valid,occurrences,id,exact)){token|=1u<<i;s.execute_order=execute_order;s.execute_member=member;}
        }
    }
    try{ngx_output_pixels::execute_passthrough(queue,n,members,forward);}catch(...){for(auto& s:slots)if(s.life.appended&&!s.life.saved)s.life.fail(gears_death_hdr_pixels::unknown_membership);throw;}
    for(unsigned i=0;i<count;++i)if(token&(1u<<i))slots[i].life.after(backend().signal(i,queue));
}
HRESULT reset_passthrough(ID3D12GraphicsCommandList* list,ID3D12CommandAllocator* allocator,ID3D12PipelineState* pso,ResetFn forward){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);HRESULT hr{};
    try{hr=ngx_output_pixels::reset_passthrough(list,allocator,pso,forward);}catch(...){for(auto& s:slots)if(s.life.appended&&!s.life.saved)s.life.fail(gears_death_hdr_pixels::reset_failed);throw;}
    if(count){const auto id=backend().list_identity(list);for(auto& s:slots){if(!s.life.appended||s.life.failure||s.life.saved||s.life.retired)continue;
        if(id&&id!=s.life.held_identity&&list!=s.raw_list)continue;auto ref=backend().list_ref(s.life.held_identity);s.reset_hr=hr;s.reset_qpc=now();
        s.life.reset(SUCCEEDED(hr),id,ref.generation,reinterpret_cast<uintptr_t>(list),ref.identity);}}
    return hr;
}
bool enable(FILE* file){if(initialized)return false;
    enabled=!self_testing&&continuous_session&&frame_link_diagnostics&&color_copy_diagnostics&&attest(file);initialized=true;
    fprintf(file,"owned_source_pixels_identity enabled=1 attested=%u chunks=6 source_pixel_only=1 final_match_verified=0 slots=2 required_size=5120,2800 format=26 min_world=2\n",unsigned(enabled));return enabled;
}
void poll(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(stopped)return;
    for(unsigned i=0;i<count;++i){auto& s=slots[i];if(s.life.appended&&!s.life.failure&&!s.life.saved&&backend().poll(i,s)&&s.life.saved){backend().release(i);s.released=true;}}
}
void stop(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);stopped=true;for(auto& s:slots)s.life.stop();}
void print(FILE* file){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    fprintf(file,"owned_source_pixels enabled=1 attested=%u ticket=%llu world=%llu renderer=%llx slots=%u eyes=%u rejected=%u failure=%u source_pixel_only=1 final_match_verified=0 same_copy_destination_is_not_final_content_proof=1 barriers_added=0\n",unsigned(enabled),ticket,world,renderer,count,eyes,rejected,failure);
    for(unsigned i=0;i<count;++i){auto const& s=slots[i];fprintf(file,"owned_source_pixel row=%u eye=%u view=%llx thread=%u copy_order=%llu qpc=%lld source_raw=%llx destination_raw=%llx source_canonical=%llx destination_canonical=%llx size=%llu,%u format=%u list_raw=%llx list_identity=%llx generation=%llu queue=%llx execute_order=%lld member=%u appended=%u executed=%u signalled=%u retired=%u gpu_complete=%u saved=%u failure=%u held_until_process=%u hr=%08lx path=%ls\n",
        i,s.before.ownership.eye,s.before.ownership.view,s.thread,s.sequence,s.copy_qpc,s.before.resources[1],s.before.resources[0],s.source_desc.canonical,s.destination_desc.canonical,s.source_desc.width,s.source_desc.height,s.source_desc.format,
        s.life.list.raw,s.life.list.identity,s.life.list.generation,s.life.queue,s.execute_order,s.execute_member,unsigned(s.life.appended),unsigned(s.life.executed),unsigned(s.life.signalled),unsigned(s.life.retired),unsigned(s.life.gpu_complete),unsigned(s.life.saved),s.life.failure,unsigned(s.life.appended&&!s.released),static_cast<unsigned long>(s.hr),s.path);}
}
#endif
}
